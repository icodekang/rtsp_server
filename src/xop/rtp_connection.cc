#include "rtp_connection.h"
#include "rtsp_connection.h"
#include "socket_util.h"

RtpConnection::RtpConnection(std::weak_ptr<TcpConnection> rtsp_connection) : m_rtsp_connection(rtsp_connection)
{
	std::random_device rd;

	for(int chn = 0; chn < MAX_MEDIA_CHANNEL; chn++) {
		m_rtpfd[chn] = 0;
		m_rtcpfd[chn] = 0;
		memset(&m_media_channel_info[chn], 0, sizeof(m_media_channel_info[chn]));
		m_media_channel_info[chn].rtp_header.version = RTP_VERSION;
		m_media_channel_info[chn].packet_seq         = rd()&0xffff;
		m_media_channel_info[chn].rtp_header.seq     = 0; //htons(1);
		m_media_channel_info[chn].rtp_header.ts      = htonl(rd());
		m_media_channel_info[chn].rtp_header.ssrc    = htonl(rd());
	}

	auto conn   = m_rtsp_connection.lock();
	m_rtsp_ip   = conn->get_ip();
	m_rtsp_port = conn->get_port();
}

void RtpConnection::set_clock_rate(MediaChannelId channel_id, uint32_t clock_rate)
{ 
	m_media_channel_info[channel_id].clock_rate = clock_rate; 
}

void RtpConnection::set_payload_type(MediaChannelId channel_id, uint32_t payload)
{ 
	m_media_channel_info[channel_id].rtp_header.payload = payload; 
}

int RtpConnection::get_id() const
{
	auto conn = m_rtsp_connection.lock();
	if (!conn) {
		return -1;
	}
	RtspConnection *rtsp_conn = (RtspConnection *)conn.get();
	return rtsp_conn->get_id();
}

bool RtpConnection::setup_rtp_over_tcp(MediaChannelId channel_id, uint16_t rtp_channel, uint16_t rtcp_channel)
{
	auto conn = m_rtsp_connection.lock();
	if (!conn) {
		return false;
	}

	m_media_channel_info[channel_id].rtp_channel  = rtp_channel;
	m_media_channel_info[channel_id].rtcp_channel = rtcp_channel;
	m_rtpfd[channel_id]  = conn->get_socket();
	m_rtcpfd[channel_id] = conn->get_socket();
	m_media_channel_info[channel_id].is_setup = true;
	m_transport_mode = RTP_OVER_TCP;

	return true;
}

bool RtpConnection::setup_rtp_over_udp(MediaChannelId channel_id, uint16_t rtp_port, uint16_t rtcp_port)
{
	auto conn = m_rtsp_connection.lock();
	if (!conn) {
		return false;
	}

	if(SocketUtil::get_peer_addr(conn->get_socket(), &m_peer_addr) < 0) {
		return false;
	}

	m_media_channel_info[channel_id].rtp_port = rtp_port;
	m_media_channel_info[channel_id].rtcp_port = rtcp_port;

	std::random_device rd;
	for (int n = 0; n <= 10; n++) {
		if (n == 10) {
			return false;
		}
        
		m_local_rtp_port[channel_id]  = rd() & 0xfffe;
		m_local_rtcp_port[channel_id] = m_local_rtp_port[channel_id] + 1;

		m_rtpfd[channel_id] = ::socket(AF_INET, SOCK_DGRAM, 0);
		if (!SocketUtil::bind(m_rtpfd[channel_id], "0.0.0.0",  m_local_rtp_port[channel_id])) {
			SocketUtil::close(m_rtpfd[channel_id]);
			continue;
		}

		m_rtcpfd[channel_id] = ::socket(AF_INET, SOCK_DGRAM, 0);
		if (!SocketUtil::bind(m_rtcpfd[channel_id], "0.0.0.0", m_local_rtcp_port[channel_id])) {
			SocketUtil::close(m_rtpfd[channel_id]);
			SocketUtil::close(m_rtcpfd[channel_id]);
			continue;
		}

		break;
	}

	SocketUtil::set_send_buf_size(m_rtpfd[channel_id], 50 * 1024);

	m_peer_rtp_addr[channel_id].sin_family = AF_INET;
	m_peer_rtp_addr[channel_id].sin_addr.s_addr = m_peer_addr.sin_addr.s_addr;
	m_peer_rtp_addr[channel_id].sin_port = htons(m_media_channel_info[channel_id].rtp_port);

	m_peer_rtcp_sddr[channel_id].sin_family = AF_INET;
	m_peer_rtcp_sddr[channel_id].sin_addr.s_addr = m_peer_addr.sin_addr.s_addr;
	m_peer_rtcp_sddr[channel_id].sin_port = htons(m_media_channel_info[channel_id].rtcp_port);

	m_media_channel_info[channel_id].is_setup = true;
	m_transport_mode = RTP_OVER_UDP;

	return true;
}

bool RtpConnection::setup_rtp_over_multicast(MediaChannelId channel_id, std::string ip, uint16_t port)
{
    std::random_device rd;
    for (int n = 0; n <= 10; n++) {
		if (n == 10) {
			return false;
		}
       
		m_local_rtp_port[channel_id] = rd() & 0xfffe;
		m_rtpfd[channel_id] = ::socket(AF_INET, SOCK_DGRAM, 0);
		if (!SocketUtil::bind(m_rtpfd[channel_id], "0.0.0.0", m_local_rtp_port[channel_id])) {
			SocketUtil::close(m_rtpfd[channel_id]);
			continue;
		}

		break;
    }

	m_media_channel_info[channel_id].rtp_port = port;

	m_peer_rtp_addr[channel_id].sin_family = AF_INET;
	m_peer_rtp_addr[channel_id].sin_addr.s_addr = inet_addr(ip.c_str());
	m_peer_rtp_addr[channel_id].sin_port = htons(port);

	m_media_channel_info[channel_id].is_setup = true;
	m_transport_mode = RTP_OVER_MULTICAST;
	m_is_multicast   = true;

	return true;
}

uint32_t RtpConnection::get_rtp_session_id() const
{ 
	return (uint32_t)((size_t)(this)); 
}

uint16_t RtpConnection::get_rtp_port(MediaChannelId channel_id) const
{ 
	return m_local_rtp_port[channel_id]; 
}

uint16_t RtpConnection::get_rtcp_port(MediaChannelId channel_id) const
{ 
	return m_local_rtcp_port[channel_id];
}

SOCKET RtpConnection::get_rtp_socket(MediaChannelId channel_id) const
{ 
	return m_rtpfd[channel_id]; 
}

SOCKET RtpConnection::get_rtcp_socket(MediaChannelId channel_id) const
{ 
	return m_rtcpfd[channel_id]; 
}

std::string RtpConnection::get_ip()
{ 
	return m_rtsp_ip; 
}

uint16_t RtpConnection::get_port()
{ 
	return m_rtsp_port; 
}

bool RtpConnection::is_multicast() const
{ 
	return m_is_multicast; 
}

bool RtpConnection::is_setup(MediaChannelId channel_id) const
{ 
	return m_media_channel_info[channel_id].is_setup; 
}

void RtpConnection::play()
{
	for (int chn = 0; chn < MAX_MEDIA_CHANNEL; chn++) {
		if (m_media_channel_info[chn].is_setup) {
			m_media_channel_info[chn].is_play = true;
		}
	}
}

void RtpConnection::record()
{
	for (int chn = 0; chn < MAX_MEDIA_CHANNEL; chn++) {
		if (m_media_channel_info[chn].is_setup) {
			m_media_channel_info[chn].is_record = true;
			m_media_channel_info[chn].is_play   = true;
		}
	}
}

void RtpConnection::teardown()
{
	if(!m_is_closed) {
		m_is_closed = true;
		for (int chn = 0; chn < MAX_MEDIA_CHANNEL; chn++) {
			m_media_channel_info[chn].is_play   = false;
			m_media_channel_info[chn].is_record = false;
		}
	}
}

std::string RtpConnection::get_multicast_ip(MediaChannelId channel_id) const
{
	return std::string(inet_ntoa(m_peer_rtp_addr[channel_id].sin_addr));
}

std::string RtpConnection::get_rtp_info(const std::string &rtsp_url)
{
	char buf[2048] = { 0 };
	snprintf(buf, 1024, "RTP-Info: ");

	int num_channel = 0;

	auto time_point = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now());
	auto ts = time_point.time_since_epoch().count();
	for (int chn = 0; chn < MAX_MEDIA_CHANNEL; chn++) {
		uint32_t rtpTime = (uint32_t)(ts * m_media_channel_info[chn].clock_rate / 1000);
		if (m_media_channel_info[chn].is_setup) {
			if (num_channel != 0) {
				snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), ",");
			}			

			snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf),
					"url=%s/track%d;seq=0;rtptime=%u",
					rtsp_url.c_str(), chn, rtpTime);
			num_channel++;
		}
	}

	return std::string(buf);
}

void RtpConnection::set_frame_type(uint8_t frame_type)
{
	m_frame_type = frame_type;
	if(!m_has_key_frame && (frame_type == 0 || frame_type == VIDEO_FRAME_I)) {
		m_has_key_frame = true;
	}
}

void RtpConnection::set_rtp_header(MediaChannelId channel_id, RtpPacket pkt)
{
	if ((m_media_channel_info[channel_id].is_play || m_media_channel_info[channel_id].is_record) && m_has_key_frame) {
		m_media_channel_info[channel_id].rtp_header.marker = pkt.last;
		m_media_channel_info[channel_id].rtp_header.ts     = htonl(pkt.timestamp);
		m_media_channel_info[channel_id].rtp_header.seq    = htons(m_media_channel_info[channel_id].packet_seq++);
		memcpy(pkt.data.get() + 4, &m_media_channel_info[channel_id].rtp_header, RTP_HEADER_SIZE);
	}
}

int RtpConnection::send_rtp_packet(MediaChannelId channel_id, RtpPacket pkt)
{    
	if (m_is_closed) {
		return -1;
	}
   
	auto conn = m_rtsp_connection.lock();
	if (!conn) {
		return -1;
	}
	RtspConnection *rtsp_conn = (RtspConnection *)conn.get();
	bool ret = rtsp_conn->m_task_scheduler->add_trigger_event([this, channel_id, pkt] {
		this->set_frame_type(pkt.type);
		this->set_rtp_header(channel_id, pkt);
		if ((m_media_channel_info[channel_id].is_play || m_media_channel_info[channel_id].is_record) && m_has_key_frame) {            
			if(m_transport_mode == RTP_OVER_TCP) {
				send_rtp_over_tcp(channel_id, pkt);
			} else {
				send_rtp_over_udp(channel_id, pkt);
			}
                   
			//m_media_channel_info[channel_id].octetCount  += pkt.size;
			//m_media_channel_info[channel_id].packetCount += 1;
		}
	});

	return ret ? 0 : -1;
}

bool RtpConnection::is_closed() const
{ 
	return m_is_closed; 
}

bool RtpConnection::has_key_frame() const
{ 
	return m_has_key_frame; 
}

int RtpConnection::send_rtp_over_tcp(MediaChannelId channel_id, RtpPacket pkt)
{
	auto conn = m_rtsp_connection.lock();
	if (!conn) {
		return -1;
	}

	uint8_t *rtp_pkt_ptr = pkt.data.get();
	rtp_pkt_ptr[0] = '$';
	rtp_pkt_ptr[1] = (char)m_media_channel_info[channel_id].rtp_channel;
	rtp_pkt_ptr[2] = (char)(((pkt.size-4)&0xFF00)>>8);
	rtp_pkt_ptr[3] = (char)((pkt.size -4)&0xFF);

	conn->send((char*)rtp_pkt_ptr, pkt.size);

	return pkt.size;
}

int RtpConnection::send_rtp_over_udp(MediaChannelId channel_id, RtpPacket pkt)
{
	int ret = sendto(m_rtpfd[channel_id], (const char*)pkt.data.get()+4, pkt.size-4, 0,
					(struct sockaddr *)&(m_peer_rtp_addr[channel_id]), sizeof(struct sockaddr_in));
                   
	if (ret < 0) {        
		teardown();
		return -1;
	}

	return ret;
}


RtpConnection::~RtpConnection()
{
	for (int chn = 0; chn < MAX_MEDIA_CHANNEL; chn++) {
		if (m_rtpfd[chn] > 0) {
			SocketUtil::close(m_rtpfd[chn]);
		}

		if (m_rtpfd[chn] > 0) {
			SocketUtil::close(m_rtpfd[chn]);
		}
	}
}