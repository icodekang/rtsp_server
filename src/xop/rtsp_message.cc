#if defined(WIN32) || defined(_WIN32)
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif
#endif

#include "rtsp_message.h"
#include "media.h"

bool RtspRequest::parse_request(BufferReader *buffer)
{
	if (buffer->peek()[0] == '$') {
		m_method = RTCP;
		return true;
	}
    
	bool ret = true;
	while (1) {
		if (m_state == ParseRequestLine) {
			const char* first_crlf = buffer->find_first_crlf();
			if(first_crlf != nullptr)
			{
				ret = parse_request_line(buffer->peek(), first_crlf);
				buffer->retrieve_until(first_crlf + 2);
			}

			if (m_state == ParseHeadersLine) {
				continue;
			} else {
				break;
			}           
		} else if(m_state == ParseHeadersLine) {
			const char *last_crlf = buffer->find_last_crlf();
			if (last_crlf != nullptr) {
				ret = parse_headers_line(buffer->peek(), last_crlf);
				buffer->retrieve_until(last_crlf + 2);
			}
			break;
		} else if(m_state == GotAll) {
			buffer->retrieve_all();
			return true;
		}
	}

	return ret;
}

bool RtspRequest::got_all() const
{ 
	return m_state == GotAll; 
}

void RtspRequest::reset()
{
	m_state = ParseRequestLine;
	m_request_line_param.clear();
	m_header_line_param.clear();
}

RtspRequest::Method RtspRequest::get_method() const
{ 
	return m_method; 
}

bool RtspRequest::parse_request_line(const char *begin, const char *end)
{
	std::string message(begin, end);
	char method[64]  = {0};
	char url[512]    = {0};
	char version[64] = {0};

	if (sscanf(message.c_str(), "%s %s %s", method, url, version) != 3) {
		return true; 
	}

	std::string method_str(method);
	if (method_str == "OPTIONS") {
		m_method = OPTIONS;
	} else if(method_str == "DESCRIBE") {
		m_method = DESCRIBE;
	} else if(method_str == "SETUP") {
		m_method = SETUP;
	} else if(method_str == "PLAY") {
		m_method = PLAY;
	} else if(method_str == "TEARDOWN") {
		m_method = TEARDOWN;
	} else if(method_str == "GET_PARAMETER") {
		m_method = GET_PARAMETER;
	} else {
		m_method = NONE;
		return false;
	}

	if (strncmp(url, "rtsp://", 7) != 0) {
		return false;
	}

	// parse url
	uint16_t port       = 0;
	char     ip[64]     = {0};
	char     suffix[64] = {0};

	if(sscanf(url+7, "%[^:]:%hu/%s", ip, &port, suffix) == 3) {

	} else if(sscanf(url+7, "%[^/]/%s", ip, suffix) == 2) {
		port = 554;
	} else {
		return false;
	}

	m_request_line_param.emplace("url",        std::make_pair(std::string(url), 0));
	m_request_line_param.emplace("url_ip",     std::make_pair(std::string(ip), 0));
	m_request_line_param.emplace("url_port",   std::make_pair("", (uint32_t)port));
	m_request_line_param.emplace("url_suffix", std::make_pair(std::string(suffix), 0));
	m_request_line_param.emplace("version",    std::make_pair(std::string(version), 0));
	m_request_line_param.emplace("method",     std::make_pair(std::move(method_str), 0));

	m_state = ParseHeadersLine;

	return true;
}

bool RtspRequest::parse_headers_line(const char *begin, const char *end)
{
	std::string message(begin, end);
	if (!parse_cseq(message)) {
		if (m_header_line_param.find("cseq") == m_header_line_param.end()) {
			return false;
		} 
	}

	if (m_method == DESCRIBE || m_method == SETUP || m_method == PLAY) {
		parse_authorization(message);
	}

	if (m_method == OPTIONS) {
		m_state = GotAll;
		return true;
	}

	if (m_method == DESCRIBE) {
		if (parse_accept(message)) {
			m_state = GotAll;
		}
		return true;
	}

	if (m_method == SETUP) {
		if (parse_transport(message)) {
			parse_media_channel(message);
			m_state = GotAll;
		}

		return true;
	}

	if (m_method == PLAY) {
		if (parse_session_id(message)) {
			m_state = GotAll;
		}
		return true;
	}

	if(m_method == TEARDOWN) {
		m_state = GotAll;
		return true;
	}

	if(m_method == GET_PARAMETER) {
		m_state = GotAll;
		return true;
	}

	return true;
}

bool RtspRequest::parse_cseq(std::string &message)
{
	std::size_t pos = message.find("CSeq");
	if (pos != std::string::npos) {
		uint32_t cseq = 0;
		sscanf(message.c_str()+pos, "%*[^:]: %u", &cseq);
		m_header_line_param.emplace("cseq", std::make_pair("", cseq));
		return true;
	}

	return false;
}

bool RtspRequest::parse_accept(std::string &message)
{
	if ((message.rfind("Accept") == std::string::npos) || (message.rfind("sdp") == std::string::npos)) {
		return false;
	}

	return true;
}

bool RtspRequest::parse_transport(std::string &message)
{
	std::size_t pos = message.find("Transport");
	if (pos != std::string::npos) {
		if ((pos=message.find("RTP/AVP/TCP")) != std::string::npos) {
			m_transport = RTP_OVER_TCP;
			uint16_t rtp_channel = 0, rtcp_channel = 0;
			if (sscanf(message.c_str() + pos, "%*[^;];%*[^;];%*[^=]=%hu-%hu", &rtp_channel, &rtcp_channel) != 2) {
				return false;
			}
			m_header_line_param.emplace("rtp_channel",  std::make_pair("", rtp_channel));
			m_header_line_param.emplace("rtcp_channel", std::make_pair("", rtcp_channel));
		} else if ((pos=message.find("RTP/AVP")) != std::string::npos) {
			uint16_t rtp_port = 0, rtcp_port = 0;
			if (((message.find("unicast", pos)) != std::string::npos)) {
				m_transport = RTP_OVER_UDP;
				if (sscanf(message.c_str()+pos, "%*[^;];%*[^;];%*[^=]=%hu-%hu", &rtp_port, &rtcp_port) != 2) {
					return false;
				}
			} else if ((message.find("multicast", pos)) != std::string::npos) {
				m_transport = RTP_OVER_MULTICAST;
			} else {
				return false;
			}
			m_header_line_param.emplace("rtp_port",  std::make_pair("", rtp_port));
			m_header_line_param.emplace("rtcp_port", std::make_pair("", rtcp_port));
		} else {
			return false;
		}

		return true;
	}

	return false;
}

bool RtspRequest::parse_session_id(std::string &message)
{
	std::size_t pos = message.find("Session");
	if (pos != std::string::npos) {
		uint32_t session_id = 0;
		if (sscanf(message.c_str() + pos, "%*[^:]: %u", &session_id) != 1) {
			return false;
		}        
		return true;
	}

	return false;
}

bool RtspRequest::parse_media_channel(std::string &message)
{
	m_channel_id = channel_0;

	auto iter = m_request_line_param.find("url");
	if (iter != m_request_line_param.end()) {
		std::size_t pos = iter->second.first.find("track1");
		if (pos != std::string::npos) {
			m_channel_id = channel_1;
		}       
	}

	return true;
}

bool RtspRequest::parse_authorization(std::string &message)
{	
	std::size_t pos = message.find("Authorization");
	if (pos != std::string::npos) {
		if ((pos = message.find("response=")) != std::string::npos) {
			m_auth_response = message.substr(pos + 10, 32);
			if (m_auth_response.size() == 32) {
				return true;
			}
		}
	}

	m_auth_response.clear();

	return false;
}

uint32_t RtspRequest::get_cseq() const
{
	uint32_t cseq = 0;
	auto iter = m_header_line_param.find("cseq");
	if (iter != m_header_line_param.end()) {
		cseq = iter->second.second;
	}

	return cseq;
}

std::string RtspRequest::get_ip() const
{
	auto iter = m_request_line_param.find("url_ip");
	if (iter != m_request_line_param.end()) {
		return iter->second.first;
	}

	return "";
}

std::string RtspRequest::get_rtsp_url() const
{
	auto iter = m_request_line_param.find("url");
	if (iter != m_request_line_param.end()) {
		return iter->second.first;
	}

	return "";
}

std::string RtspRequest::get_rtsp_url_suffix() const
{
	auto iter = m_request_line_param.find("url_suffix");
	if (iter != m_request_line_param.end()) {
		return iter->second.first;
	}

	return "";
}

std::string RtspRequest::get_auth_response() const
{
	return m_auth_response;
}

TransportMode RtspRequest::get_transport_mode() const
{ 
	return m_transport; 
}

MediaChannelId RtspRequest::get_channel_id() const
{ 
	return m_channel_id; 
}

uint8_t RtspRequest::get_rtp_channel() const
{
	auto iter = m_header_line_param.find("rtp_channel");
	if (iter != m_header_line_param.end()) {
		return iter->second.second;
	}

	return 0;
}

uint8_t RtspRequest::get_rtcp_channel() const
{
	auto iter = m_header_line_param.find("rtcp_channel");
	if (iter != m_header_line_param.end()) {
		return iter->second.second;
	}

	return 0;
}

uint16_t RtspRequest::get_rtp_port() const
{
	auto iter = m_header_line_param.find("rtp_port");
	if (iter != m_header_line_param.end()) {
		return iter->second.second;
	}

	return 0;
}

uint16_t RtspRequest::get_rtcp_port() const
{
	auto iter = m_header_line_param.find("rtcp_port");
	if (iter != m_header_line_param.end()) {
		return iter->second.second;
	}

	return 0;
}

int RtspRequest::build_option_res(const char *buf, int buf_size)
{
	memset((void*)buf, 0, buf_size);
	snprintf((char*)buf, buf_size,
			"RTSP/1.0 200 OK\r\n"
			"CSeq: %u\r\n"
			"Public: OPTIONS, DESCRIBE, SETUP, TEARDOWN, PLAY\r\n"
			"\r\n",
			this->get_cseq());

	return (int)strlen(buf);
}

int RtspRequest::build_describe_res(const char *buf, int buf_size, const char *sdp)
{
	memset((void*)buf, 0, buf_size);
	snprintf((char*)buf, buf_size,
			"RTSP/1.0 200 OK\r\n"
			"CSeq: %u\r\n"
			"Content-Length: %d\r\n"
			"Content-Type: application/sdp\r\n"
			"\r\n"
			"%s",
			this->get_cseq(), 
			(int)strlen(sdp), 
			sdp);

	return (int)strlen(buf);
}

int RtspRequest::build_setup_multicast_res(const char *buf, int buf_size, const char *multicast_ip, uint16_t port, uint32_t session_id)
{	
	memset((void*)buf, 0, buf_size);
	snprintf((char*)buf, buf_size,
			"RTSP/1.0 200 OK\r\n"
			"CSeq: %u\r\n"
			"Transport: RTP/AVP;multicast;destination=%s;source=%s;port=%u-0;ttl=255\r\n"
			"Session: %u\r\n"
			"\r\n",
			this->get_cseq(),
			multicast_ip,
			this->get_ip().c_str(),
			port,
			session_id);

	return (int)strlen(buf);
}

int RtspRequest::build_setup_udp_res(const char *buf, int buf_size, uint16_t rtp_chn, uint16_t rtcp_chn, uint32_t session_id)
{
	memset((void*)buf, 0, buf_size);
	snprintf((char*)buf, buf_size,
			"RTSP/1.0 200 OK\r\n"
			"CSeq: %u\r\n"
			"Transport: RTP/AVP;unicast;client_port=%hu-%hu;server_port=%hu-%hu\r\n"
			"Session: %u\r\n"
			"\r\n",
			this->get_cseq(),
			this->get_rtp_port(),
			this->get_rtcp_port(),
			rtp_chn, 
			rtcp_chn,
			session_id);

	return (int)strlen(buf);
}

int RtspRequest::build_setup_tcp_res(const char *buf, int buf_size, uint16_t rtp_chn, uint16_t rtcp_chn, uint32_t session_id)
{
	memset((void*)buf, 0, buf_size);
	snprintf((char*)buf, buf_size,
			"RTSP/1.0 200 OK\r\n"
			"CSeq: %u\r\n"
			"Transport: RTP/AVP/TCP;unicast;interleaved=%d-%d\r\n"
			"Session: %u\r\n"
			"\r\n",
			this->get_cseq(),
			rtp_chn, rtcp_chn,
			session_id);

	return (int)strlen(buf);
}

int RtspRequest::build_play_res(const char *buf, int buf_size, const char* rtpInfo, uint32_t session_id)
{
	memset((void*)buf, 0, buf_size);
	snprintf((char*)buf, buf_size,
			"RTSP/1.0 200 OK\r\n"
			"CSeq: %d\r\n"
			"Range: npt=0.000-\r\n"
			"Session: %u; timeout=60\r\n",
			this->get_cseq(),
			session_id);

	if (rtpInfo != nullptr) {
		snprintf((char*)buf + strlen(buf), buf_size - strlen(buf), "%s\r\n", rtpInfo);
	}

	snprintf((char*)buf + strlen(buf), buf_size - strlen(buf), "\r\n");

	return (int)strlen(buf);
}

int RtspRequest::build_teardown_res(const char *buf, int buf_size, uint32_t session_id)
{
	memset((void*)buf, 0, buf_size);
	snprintf((char*)buf, buf_size,
			"RTSP/1.0 200 OK\r\n"
			"CSeq: %d\r\n"
			"Session: %u\r\n"
			"\r\n",
			this->get_cseq(),
			session_id);

	return (int)strlen(buf);
}

int RtspRequest::build_get_paramter_res(const char *buf, int buf_size, uint32_t session_id)
{
	memset((void*)buf, 0, buf_size);
	snprintf((char*)buf, buf_size,
			"RTSP/1.0 200 OK\r\n"
			"CSeq: %d\r\n"
			"Session: %u\r\n"
			"\r\n",
			this->get_cseq(),
			session_id);

	return (int)strlen(buf);
}

int RtspRequest::build_not_found_res(const char *buf, int buf_size)
{
	memset((void*)buf, 0, buf_size);
	snprintf((char*)buf, buf_size,
			"RTSP/1.0 404 Stream Not Found\r\n"
			"CSeq: %u\r\n"
			"\r\n",
			this->get_cseq());

	return (int)strlen(buf);
}

int RtspRequest::build_server_error_res(const char *buf, int buf_size)
{
	memset((void*)buf, 0, buf_size);
	snprintf((char*)buf, buf_size,
			"RTSP/1.0 500 Internal Server Error\r\n"
			"CSeq: %u\r\n"
			"\r\n",
			this->get_cseq());

	return (int)strlen(buf);
}

int RtspRequest::build_unsupported_res(const char *buf, int buf_size)
{
	memset((void*)buf, 0, buf_size);
	snprintf((char*)buf, buf_size,
			"RTSP/1.0 461 Unsupported transport\r\n"
			"CSeq: %d\r\n"
			"\r\n",
			this->get_cseq());

	return (int)strlen(buf);
}

int RtspRequest::build_unauthorized_res(const char *buf, int buf_size, const char *realm, const char *nonce)
{
	memset((void*)buf, 0, buf_size);
	snprintf((char*)buf, buf_size,
			"RTSP/1.0 401 Unauthorized\r\n"
			"CSeq: %d\r\n"
			"WWW-Authenticate: Digest realm=\"%s\", nonce=\"%s\"\r\n"
			"\r\n",
			this->get_cseq(),
			realm,
			nonce);

	return (int)strlen(buf);
}

bool RtspResponse::parse_response(BufferReader *buffer)
{
	if (strstr(buffer->peek(), "\r\n\r\n") != NULL) {
		if (strstr(buffer->peek(), "OK") == NULL) {
			return false;
		}

		char *ptr = strstr(buffer->peek(), "Session");
		if (ptr != NULL) {
			char session_id[50] = {0};
			if (sscanf(ptr, "%*[^:]: %s", session_id) == 1)
				m_session = session_id;
		}

		m_cseq++;
		buffer->retrieve_until("\r\n\r\n");
	}

	return true;
}

RtspResponse::Method RtspResponse::get_method() const
{ 
	return m_method; 
}

uint32_t RtspResponse::get_cseq() const
{ 
	return m_cseq;  
}

std::string RtspResponse::get_session() const
{ 
	return m_session; 
}

void RtspResponse::set_user_agent(const char *user_agent) 
{ 
	m_user_agent = std::string(user_agent); 
}

void RtspResponse::set_rtsp_url(const char *url)
{ 
	m_rtsp_url = std::string(url); 
}

int RtspResponse::build_option_req(const char *buf, int buf_size)
{
	memset((void*)buf, 0, buf_size);
	snprintf((char*)buf, buf_size,
			"OPTIONS %s RTSP/1.0\r\n"
			"CSeq: %u\r\n"
			"User-Agent: %s\r\n"
			"\r\n",
			m_rtsp_url.c_str(),
			this->get_cseq() + 1,
			m_user_agent.c_str());

	m_method = OPTIONS;

	return (int)strlen(buf);
}

int RtspResponse::build_announce_req(const char *buf, int buf_size, const char *sdp)
{
	memset((void*)buf, 0, buf_size);
	snprintf((char*)buf, buf_size,
			"ANNOUNCE %s RTSP/1.0\r\n"
			"Content-Type: application/sdp\r\n"
			"CSeq: %u\r\n"
			"User-Agent: %s\r\n"
			"Session: %s\r\n"
			"Content-Length: %d\r\n"
			"\r\n"
			"%s",
			m_rtsp_url.c_str(),
			this->get_cseq() + 1, 
			m_user_agent.c_str(),
			this->get_session().c_str(),
			(int)strlen(sdp),
			sdp);

	m_method = ANNOUNCE;

	return (int)strlen(buf);
}

int RtspResponse::build_describe_req(const char *buf, int buf_size)
{
	memset((void*)buf, 0, buf_size);
	snprintf((char*)buf, buf_size,
			"DESCRIBE %s RTSP/1.0\r\n"
			"CSeq: %u\r\n"
			"Accept: application/sdp\r\n"
			"User-Agent: %s\r\n"
			"\r\n",
			m_rtsp_url.c_str(),
			this->get_cseq() + 1,
			m_user_agent.c_str());

	m_method = DESCRIBE;

	return (int)strlen(buf);
}

int RtspResponse::build_setup_tcp_req(const char *buf, int buf_size, int track_id)
{
	int interleaved[2] = { 0, 1 };
	if (track_id == 1) {
		interleaved[0] = 2;
		interleaved[1] = 3;
	}

	memset((void*)buf, 0, buf_size);
	snprintf((char*)buf, buf_size,
			"SETUP %s/track%d RTSP/1.0\r\n"
			"Transport: RTP/AVP/TCP;unicast;mode=record;interleaved=%d-%d\r\n"
			"CSeq: %u\r\n"
			"User-Agent: %s\r\n"
			"Session: %s\r\n"
			"\r\n",
			m_rtsp_url.c_str(), 
			track_id,
			interleaved[0],
			interleaved[1],
			this->get_cseq() + 1,
			m_user_agent.c_str(), 
			this->get_session().c_str());

	m_method = SETUP;

	return (int)strlen(buf);
}

int RtspResponse::build_record_req(const char *buf, int buf_size)
{
	memset((void*)buf, 0, buf_size);
	snprintf((char*)buf, buf_size,
			"RECORD %s RTSP/1.0\r\n"
			"Range: npt=0.000-\r\n"
			"CSeq: %u\r\n"
			"User-Agent: %s\r\n"
			"Session: %s\r\n"
			"\r\n",
			m_rtsp_url.c_str(), 
			this->get_cseq() + 1,
			m_user_agent.c_str(), 
			this->get_session().c_str());

	m_method = RECORD;
	
	return (int)strlen(buf);
}
