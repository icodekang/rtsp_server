#include "rtsp_connection.h"
#include "rtsp_server.h"
#include "media_session.h"
#include "media_source.h"
#include "socket_util.h"

#define USER_AGENT "-_-"
#define RTSP_DEBUG 0
#define MAX_RTSP_MESSAGE_SIZE 2048

RtspConnection::RtspConnection(std::shared_ptr<Rtsp> rtsp, TaskScheduler *task_scheduler, SOCKET sockfd)
	: TcpConnection(task_scheduler, sockfd)
	, m_rtsp(rtsp)
	, m_task_scheduler(task_scheduler)
	, m_rtp_channel(new Channel(sockfd))
	, m_rtsp_request(new RtspRequest)
	, m_rtsp_response(new RtspResponse)
{
	this->set_read_callback([this](std::shared_ptr<TcpConnection> conn, BufferReader &buffer) {
		return this->on_read(buffer);
	});

	this->set_close_callback([this](std::shared_ptr<TcpConnection> conn) {
		this->on_close();
	});

	m_alive_count = 1;

	m_rtp_channel->set_read_callback ([this]() { this->handle_read(); });
	m_rtp_channel->set_write_callback([this]() { this->handle_write(); });
	m_rtp_channel->set_close_callback([this]() { this->handle_close(); });
	m_rtp_channel->set_error_callback([this]() { this->handle_error(); });

	for (int chn = 0; chn < MAX_MEDIA_CHANNEL; chn++) {
		m_rtcp_channels[chn] = nullptr;
	}

	m_has_auth = true;
	if (rtsp->m_has_auth_info) {
		m_has_auth = false;
		m_auth_info.reset(new DigestAuthentication(rtsp->m_realm, rtsp->m_username, rtsp->m_password));
	}
}

MediaSessionId RtspConnection::get_media_session_id()
{ 
	return m_session_id; 
}

TaskScheduler *RtspConnection::get_task_scheduler() const 
{ 
	return m_task_scheduler; 
}

void RtspConnection::keep_alive()
{ 
	m_alive_count++; 
}

bool RtspConnection::is_alive() const
{
	if (is_closed()) {
		return false;
	}

	if (m_rtp_conn != nullptr) {
		if (m_rtp_conn->is_multicast()) {
			return true;
		}			
	}

	return (m_alive_count > 0);
}

void RtspConnection::reset_alive_count()
{ 
	m_alive_count = 0; 
}

int RtspConnection::get_id() const
{ 
	return m_task_scheduler->get_id();
}

bool RtspConnection::is_play() const
{ 
	return m_conn_state == START_PLAY; 
}

bool RtspConnection::is_record() const
{ 
	return m_conn_state == START_PUSH; 
}

bool RtspConnection::on_read(BufferReader &buffer)
{
	keep_alive();

	int size = buffer.readable_bytes();
	if (size <= 0) {
		return false; //close
	}

	if (m_conn_mode == RTSP_SERVER) {
		if (!handle_rtsp_request(buffer)){
			return false; 
		}
	} else if (m_conn_mode == RTSP_PUSHER) {
		if (!handle_rtsp_response(buffer)) {           
			return false;
		}
	}

	if (buffer.readable_bytes() > MAX_RTSP_MESSAGE_SIZE) {
		buffer.retrieve_all(); 
	}

	return true;
}

void RtspConnection::on_close()
{
	if(m_session_id != 0) {
		auto rtsp = m_rtsp.lock();
		if (rtsp) {
			MediaSession::Ptr media_session = rtsp->look_media_session(m_session_id);
			if (media_session) {
				media_session->remove_client(this->get_socket());
			}
		}	
	}

	for(int chn = 0; chn < MAX_MEDIA_CHANNEL; chn++) {
		if(m_rtcp_channels[chn] && !m_rtcp_channels[chn]->is_none_event()) {
			m_task_scheduler->remove_channel(m_rtcp_channels[chn]);
		}
	}
}

bool RtspConnection::handle_rtsp_request(BufferReader &buffer)
{
#if RTSP_DEBUG
	string str(buffer.peek(), buffer.readable_bytes());
	if (str.find("rtsp") != string::npos || str.find("RTSP") != string::npos) {
		std::cout << str << std::endl;
	}
#endif

	if (m_rtsp_request->parse_request(&buffer)) {
		RtspRequest::Method method = m_rtsp_request->get_method();
		if (method == RtspRequest::RTCP) {
			handle_rtcp(buffer);
			return true;
		} else if(!m_rtsp_request->got_all()) {
			return true;
		}
        
		switch (method) {

		case RtspRequest::OPTIONS:
			handle_cmd_option();
			break;
		case RtspRequest::DESCRIBE:
			handle_cmd_describe();
			break;
		case RtspRequest::SETUP:
			handle_cmd_setup();
			break;
		case RtspRequest::PLAY:
			handle_cmd_play();
			break;
		case RtspRequest::TEARDOWN:
			handle_cmd_teardown();
			break;
		case RtspRequest::GET_PARAMETER:
			handle_cmd_get_paramter();
			break;
		default:
			break;
		}

		if (m_rtsp_request->got_all()) {
			m_rtsp_request->reset();
		}
	} else {
		return false;
	}

	return true;
}

bool RtspConnection::handle_rtsp_response(BufferReader &buffer)
{
#if RTSP_DEBUG
	string str(buffer.peek(), buffer.readable_bytes());
	if (str.find("rtsp") != string::npos || str.find("RTSP") != string::npos) {
		cout << str << endl;
	}		
#endif

	if (m_rtsp_response->parse_response(&buffer)) {
		RtspResponse::Method method = m_rtsp_response->get_method();
		switch (method) {

		case RtspResponse::OPTIONS:
			if (m_conn_mode == RTSP_PUSHER) {
				send_announce();
			}             
			break;
		case RtspResponse::ANNOUNCE:
		case RtspResponse::DESCRIBE:
			send_setup();
			break;
		case RtspResponse::SETUP:
			send_setup();
			break;
		case RtspResponse::RECORD:
			handle_record();
			break;
		default:            
			break;
		}
	} else {
		return false;
	}

	return true;
}

void RtspConnection::send_rtsp_message(std::shared_ptr<char> buf, uint32_t size)
{
#if RTSP_DEBUG
	cout << buf.get() << endl;
#endif

	this->send(buf, size);
	return;
}

void RtspConnection::handle_rtcp(BufferReader &buffer)
{    
	char *peek = buffer.peek();
	if (peek[0] == '$' &&  buffer.readable_bytes() > 4) {
		uint32_t pkt_size = peek[2] << 8 | peek[3];
		if (pkt_size + 4 >=  buffer.readable_bytes()) {
			buffer.retrieve(pkt_size + 4);  
		}
	}
}
 
void RtspConnection::handle_rtcp(SOCKET sockfd)
{
	char buf[1024] = {0};
	if (recv(sockfd, buf, 1024, 0) > 0) {
		keep_alive();
	}
}

void RtspConnection::handle_cmd_option()
{
	std::shared_ptr<char> res(new char[2048], std::default_delete<char[]>());
	int size = m_rtsp_request->build_option_res(res.get(), 2048);
	this->send_rtsp_message(res, size);	
}

void RtspConnection::handle_cmd_describe()
{
	if (m_auth_info!=nullptr && !handle_authentication()) {
		return;
	}

	if (m_rtp_conn == nullptr) {
		m_rtp_conn.reset(new RtpConnection(shared_from_this()));
	}

	int size = 0;
	std::shared_ptr<char> res(new char[4096], std::default_delete<char[]>());
	MediaSession::Ptr media_session = nullptr;

	auto rtsp = m_rtsp.lock();
	if (rtsp) {
		media_session = rtsp->look_media_session(m_rtsp_request->get_rtsp_url_suffix());
	}
	
	if(!rtsp || !media_session) {
		size = m_rtsp_request->build_not_found_res(res.get(), 4096);
	} else {
		m_session_id = media_session->get_media_session_id();
		media_session->add_client(this->get_socket(), m_rtp_conn);

		for (int chn = 0; chn < MAX_MEDIA_CHANNEL; chn++) {
			MediaSource* source = media_session->get_media_source((MediaChannelId)chn);
			if(source != nullptr) {
				m_rtp_conn->set_clock_rate((MediaChannelId)chn, source->get_clock_rate());
				m_rtp_conn->set_payload_type((MediaChannelId)chn, source->get_payload_type());
			}
		}

		std::string sdp = media_session->get_sdp_message(SocketUtil::get_socket_ip(this->get_socket()), rtsp->get_version());
		if (sdp == "") {
			size = m_rtsp_request->build_server_error_res(res.get(), 4096);
		} else {
			size = m_rtsp_request->build_describe_res(res.get(), 4096, sdp.c_str());		
		}
	}

	send_rtsp_message(res, size);

	return ;
}

void RtspConnection::handle_cmd_setup()
{
	if (m_auth_info != nullptr && !handle_authentication()) {
		return;
	}

	int size = 0;
	std::shared_ptr<char> res(new char[4096], std::default_delete<char[]>());
	MediaChannelId channel_id = m_rtsp_request->get_channel_id();
	MediaSession::Ptr media_session = nullptr;

	auto rtsp = m_rtsp.lock();
	if (rtsp) {
		media_session = rtsp->look_media_session(m_session_id);
	}

	if(!rtsp || !media_session)  {
		goto server_error;
	}

	if(media_session->is_multicast())  {
		std::string multicast_ip = media_session->get_multicast_ip();
		if (m_rtsp_request->get_transport_mode() == RTP_OVER_MULTICAST) {
			uint16_t port = media_session->get_multicast_port(channel_id);
			uint16_t session_id = m_rtp_conn->get_rtp_session_id();
			if (!m_rtp_conn->setup_rtp_over_multicast(channel_id, multicast_ip.c_str(), port)) {
				goto server_error;
			}

			size = m_rtsp_request->build_setup_multicast_res(res.get(), 4096, multicast_ip.c_str(), port, session_id);
		} else {
			goto transport_unsupport;
		}
	} else {
		if(m_rtsp_request->get_transport_mode() == RTP_OVER_TCP) {
			uint16_t rtp_channel  = m_rtsp_request->get_rtp_channel();
			uint16_t rtcp_channel = m_rtsp_request->get_rtcp_channel();
			uint16_t session_id   = m_rtp_conn->get_rtp_session_id();

			m_rtp_conn->setup_rtp_over_tcp(channel_id, rtp_channel, rtcp_channel);
			size = m_rtsp_request->build_setup_tcp_res(res.get(), 4096, rtp_channel, rtcp_channel, session_id);
		} else if(m_rtsp_request->get_transport_mode() == RTP_OVER_UDP) {
			uint16_t peer_rtp_port  = m_rtsp_request->get_rtp_port();
			uint16_t peer_rtcp_port = m_rtsp_request->get_rtcp_port();
			uint16_t session_id     = m_rtp_conn->get_rtp_session_id();

			if (m_rtp_conn->setup_rtp_over_udp(channel_id, peer_rtp_port, peer_rtcp_port)) {
				SOCKET rtcp_fd = m_rtp_conn->get_rtcp_socket(channel_id);
				m_rtcp_channels[channel_id].reset(new Channel(rtcp_fd));
				m_rtcp_channels[channel_id]->set_read_callback([rtcp_fd, this]() { this->handle_rtcp(rtcp_fd); });
				m_rtcp_channels[channel_id]->enable_reading();
				m_task_scheduler->update_channel(m_rtcp_channels[channel_id]);
			} else {
				goto server_error;
			}

			uint16_t ser_rtp_port  = m_rtp_conn->get_rtp_port(channel_id);
			uint16_t ser_rtcp_port = m_rtp_conn->get_rtcp_port(channel_id);
			size = m_rtsp_request->build_setup_udp_res(res.get(), 4096, ser_rtp_port, ser_rtcp_port, session_id);
		} else {          
			goto transport_unsupport;
		}
	}

	send_rtsp_message(res, size);
	return ;

transport_unsupport:
	size = m_rtsp_request->build_unsupported_res(res.get(), 4096);
	send_rtsp_message(res, size);
	return ;

server_error:
	size = m_rtsp_request->build_server_error_res(res.get(), 4096);
	send_rtsp_message(res, size);
	return ;
}

void RtspConnection::handle_cmd_play()
{
	if (m_auth_info != nullptr) {
		if (!handle_authentication()) {
			return;
		}
	}

	if (m_rtp_conn == nullptr) {
		return;
	}

	m_conn_state = START_PLAY;
	m_rtp_conn->play();

	uint16_t session_id = m_rtp_conn->get_rtp_session_id();
	std::shared_ptr<char> res(new char[2048], std::default_delete<char[]>());

	int size = m_rtsp_request->build_play_res(res.get(), 2048, nullptr, session_id);
	send_rtsp_message(res, size);
}

void RtspConnection::handle_cmd_teardown()
{
	if (m_rtp_conn == nullptr) {
		return;
	}

	m_rtp_conn->teardown();

	uint16_t session_id = m_rtp_conn->get_rtp_session_id();
	std::shared_ptr<char> res(new char[2048], std::default_delete<char[]>());
	int size = m_rtsp_request->build_teardown_res(res.get(), 2048, session_id);
	send_rtsp_message(res, size);

	//HandleClose();
}

void RtspConnection::handle_cmd_get_paramter()
{
	if (m_rtp_conn == nullptr) {
		return;
	}

	uint16_t session_id = m_rtp_conn->get_rtp_session_id();
	std::shared_ptr<char> res(new char[2048], std::default_delete<char[]>());
	int size = m_rtsp_request->build_get_paramter_res(res.get(), 2048, session_id);
	send_rtsp_message(res, size);
}

bool RtspConnection::handle_authentication()
{
	if (m_auth_info != nullptr && !m_has_auth) {
		std::string cmd = m_rtsp_request->MethodToString[m_rtsp_request->get_method()];
		std::string url = m_rtsp_request->get_rtsp_url();

		if (m_nonce.size() > 0 && (m_auth_info->get_response(m_nonce, cmd, url) == m_rtsp_request->get_auth_response())) {
			m_nonce.clear();
			m_has_auth = true;
		} else {
			std::shared_ptr<char> res(new char[4096], std::default_delete<char[]>());
			m_nonce  = m_auth_info->get_nonce();
			int size = m_rtsp_request->build_unauthorized_res(res.get(), 4096, m_auth_info->get_realm().c_str(), m_nonce.c_str());
			send_rtsp_message(res, size);
			return false;
		}
	}

	return true;
}

void RtspConnection::send_options(ConnectionMode mode)
{
	if (m_rtp_conn == nullptr) {
		m_rtp_conn.reset(new RtpConnection(shared_from_this()));
	}

	auto rtsp = m_rtsp.lock();
	if (!rtsp) {
		handle_close();
		return;
	}	

	m_conn_mode = mode;
	m_rtsp_response->set_user_agent(USER_AGENT);
	m_rtsp_response->set_rtsp_url(rtsp->get_rtsp_url().c_str());

	std::shared_ptr<char> req(new char[2048], std::default_delete<char[]>());
	int size = m_rtsp_response->build_option_req(req.get(), 2048);
	send_rtsp_message(req, size);
}

void RtspConnection::send_announce()
{
	MediaSession::Ptr media_session = nullptr;

	auto rtsp = m_rtsp.lock();
	if (rtsp) {
		media_session = rtsp->look_media_session(1);
	}

	if (!rtsp || !media_session) {
		handle_close();
		return;
	} else {
		m_session_id = media_session->get_media_session_id();
		media_session->add_client(this->get_socket(), m_rtp_conn);

		for (int chn = 0; chn < 2; chn++) {
			MediaSource *source = media_session->get_media_source((MediaChannelId)chn);
			if (source != nullptr) {
				m_rtp_conn->set_clock_rate((MediaChannelId)chn, source->get_clock_rate());
				m_rtp_conn->set_payload_type((MediaChannelId)chn, source->get_payload_type());
			}
		}
	}

	std::string sdp = media_session->get_sdp_message(SocketUtil::get_socket_ip(this->get_socket()), rtsp->get_version());
	if (sdp == "") {
		handle_close();
		return;
	}

	std::shared_ptr<char> req(new char[4096], std::default_delete<char[]>());
	int size = m_rtsp_response->build_announce_req(req.get(), 4096, sdp.c_str());
	send_rtsp_message(req, size);
}

void RtspConnection::send_describe()
{
	std::shared_ptr<char> req(new char[2048], std::default_delete<char[]>());
	int size = m_rtsp_response->build_describe_req(req.get(), 2048);
	send_rtsp_message(req, size);
}

void RtspConnection::send_setup()
{
	int size = 0;
	std::shared_ptr<char> buf(new char[2048], std::default_delete<char[]>());
	MediaSession::Ptr media_session = nullptr;

	auto rtsp = m_rtsp.lock();
	if (rtsp) {
		media_session = rtsp->look_media_session(m_session_id);
	}
	
	if (!rtsp || !media_session) {
		handle_close();
		return;
	}

	if (media_session->get_media_source(channel_0) && !m_rtp_conn->is_setup(channel_0)) {
		m_rtp_conn->setup_rtp_over_tcp(channel_0, 0, 1);
		size = m_rtsp_response->build_setup_tcp_req(buf.get(), 2048, channel_0);
	} else if (media_session->get_media_source(channel_1) && !m_rtp_conn->is_setup(channel_1)) {
		m_rtp_conn->setup_rtp_over_tcp(channel_1, 2, 3);
		size = m_rtsp_response->build_setup_tcp_req(buf.get(), 2048, channel_1);
	} else {
		size = m_rtsp_response->build_record_req(buf.get(), 2048);
	}

	send_rtsp_message(buf, size);
}

void RtspConnection::handle_record()
{
	m_conn_state = START_PUSH;
	m_rtp_conn->record();
}
