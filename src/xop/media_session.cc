#include "media_session.h"
#include "rtp_connection.h"
#include <cstring>
#include <ctime>
#include <map>
#include <forward_list>
#include "socket_util.h"

std::atomic_uint MediaSession::m_last_session_id(1);

MediaSession::MediaSession(std::string url_suffxx)
	: m_suffix(url_suffxx)
	, m_media_sources(MAX_MEDIA_CHANNEL)
	, m_buffer(MAX_MEDIA_CHANNEL)
{
	m_has_new_client = false;
	m_session_id     = ++m_last_session_id;

	for (int n = 0; n < MAX_MEDIA_CHANNEL; n++) {
		m_multicast_port[n] = 0;
	}
}

MediaSession* MediaSession::create_new(std::string url_suffix)
{
	return new MediaSession(std::move(url_suffix));
}

void MediaSession::add_notify_connected_callback(const NotifyConnectedCallback &callback)
{
	m_notify_connected_callbacks.push_back(callback);
}

void MediaSession::add_notify_disconnected_callback(const NotifyDisconnectedCallback &callback)
{
	m_notify_disconnected_callbacks.push_back(callback);
}

std::string MediaSession::get_rtsp_url_suffix() const
{ 
	return m_suffix; 
}

void MediaSession::set_rtsp_url_suffix(std::string &suffix)
{ 
	m_suffix = suffix; 
}

bool MediaSession::add_source(MediaChannelId channel_id, MediaSource *source)
{
	source->set_send_frame_callback([this](MediaChannelId channel_id, RtpPacket pkt) {
		std::forward_list<std::shared_ptr<RtpConnection>> clients;
		std::map<int, RtpPacket> packets;
		{
			std::lock_guard<std::mutex> lock(m_map_mutex);
			for (auto iter = m_clients.begin(); iter != m_clients.end();) {
				auto conn = iter->second.lock();
				if (conn == nullptr) {
					m_clients.erase(iter++);
				} else  {				
					int id = conn->get_id();
					if (id >= 0) {
						if (packets.find(id) == packets.end()) {
							RtpPacket tmp_pkt;
							memcpy(tmp_pkt.data.get(), pkt.data.get(), pkt.size);
							tmp_pkt.size = pkt.size;
							tmp_pkt.last = pkt.last;
							tmp_pkt.timestamp = pkt.timestamp;
							tmp_pkt.type = pkt.type;
							packets.emplace(id, tmp_pkt);
						}
						clients.emplace_front(conn);
					}
					iter++;
				}
			}
		}
        
		int count = 0;
		for (auto iter : clients) {
			int ret = 0;
			int id = iter->get_id();
			if (id >= 0) {
				auto iter2 = packets.find(id);
				if (iter2 != packets.end()) {
					count++;
					ret = iter->send_rtp_packet(channel_id, iter2->second);
					if (m_is_multicast && ret == 0) {
						break;
					}				
				}
			}					
		}

		return true;
	});

	m_media_sources[channel_id].reset(source);

	return true;
}

bool MediaSession::remove_source(MediaChannelId channel_id)
{
	m_media_sources[channel_id] = nullptr;
	return true;
}

MediaSessionId MediaSession::get_media_session_id()
{ 
	return m_session_id; 
}

uint32_t MediaSession::get_num_client() const
{ 
	return (uint32_t)m_clients.size();
}

bool MediaSession::is_multicast() const
{ 
	return m_is_multicast;
}

std::string MediaSession::get_multicast_ip() const
{ 
	return m_multicast_ip; 
}

uint16_t MediaSession::get_multicast_port(MediaChannelId channel_id) const
{
	if (channel_id >= MAX_MEDIA_CHANNEL) {
		return 0;
	}         
	return m_multicast_port[channel_id];
}

bool MediaSession::start_multicast()
{  
	if (m_is_multicast) {
		return true;
	}

	m_multicast_ip = MulticastAddr::instance().get_addr();
	if (m_multicast_ip == "") {
		return false;
	}

	std::random_device rd;
	m_multicast_port[channel_0] = htons(rd() & 0xfffe);
	m_multicast_port[channel_1] = htons(rd() & 0xfffe);

	m_is_multicast = true;

	return true;
}

std::string MediaSession::get_sdp_message(std::string ip, std::string session_name)
{
	if (m_sdp != "") {
		return m_sdp;
	}
    
	if (m_media_sources.empty()) {
		return "";
	}

	char buf[2048] = {0};

	snprintf(buf, sizeof(buf),
			"v=0\r\n"
			"o=- 9%ld 1 IN IP4 %s\r\n"
			"t=0 0\r\n"
			"a=control:*\r\n" ,
			(long)std::time(NULL), ip.c_str()); 

	if(session_name != "") {
		snprintf(buf+strlen(buf), sizeof(buf)-strlen(buf), 
				"s=%s\r\n",
				session_name.c_str());
	}
    
	if(m_is_multicast) {
		snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf),
				"a=type:broadcast\r\n"
				"a=rtcp-unicast: reflection\r\n");
	}
		
	for (uint32_t chn=0; chn < m_media_sources.size(); chn++) {
		if(m_media_sources[chn]) {	
			if(m_is_multicast) {
				snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), 
						"%s\r\n",
						m_media_sources[chn]->get_media_description(m_multicast_port[chn]).c_str()); 
                     
				snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), 
						"c=IN IP4 %s/255\r\n",
						m_multicast_ip.c_str()); 
			} else {
				snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), 
						"%s\r\n",
						m_media_sources[chn]->get_media_description(0).c_str());
			}
            
			snprintf(buf+strlen(buf), sizeof(buf)-strlen(buf), 
					"%s\r\n",
					m_media_sources[chn]->get_attribute().c_str());
                     
			snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf),											
					"a=control:track%d\r\n", chn);	
		}
	}

	m_sdp = buf;

	return m_sdp;
}

MediaSource *MediaSession::get_media_source(MediaChannelId channel_id)
{
	if (m_media_sources[channel_id]) {
		return m_media_sources[channel_id].get();
	}

	return nullptr;
}

bool MediaSession::handle_frame(MediaChannelId channel_id, AVData frame)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	if (m_media_sources[channel_id]) {
		m_media_sources[channel_id]->handle_frame(channel_id, frame);
	} else {
		return false;
	}

	return true;
}

bool MediaSession::add_client(SOCKET rtspfd, std::shared_ptr<RtpConnection> rtp_conn)
{
	std::lock_guard<std::mutex> lock(m_map_mutex);

	auto iter = m_clients.find (rtspfd);
	if (iter == m_clients.end()) {
		std::weak_ptr<RtpConnection> rtp_conn_weak_ptr = rtp_conn;
		m_clients.emplace(rtspfd, rtp_conn_weak_ptr);
		for (auto& callback : m_notify_connected_callbacks) {
			callback(m_session_id, rtp_conn->get_ip(), rtp_conn->get_port());
		}			
        
		m_has_new_client = true;
		return true;
	}
            
	return false;
}

void MediaSession::remove_client(SOCKET rtspfd)
{  
	std::lock_guard<std::mutex> lock(m_map_mutex);

	auto iter = m_clients.find(rtspfd);
	if (iter != m_clients.end()) {
		auto conn = iter->second.lock();
		if (conn) {
			for (auto& callback : m_notify_disconnected_callbacks) {
				callback(m_session_id, conn->get_ip(), conn->get_port());
			}				
		}
		m_clients.erase(iter);
	}
}


MediaSession::~MediaSession()
{
	if (m_multicast_ip != "") {
		MulticastAddr::instance().release(m_multicast_ip);
	}
}

/***********************MulticastAddr***********************/

MulticastAddr &MulticastAddr::instance()
{
	static MulticastAddr s_multi_addr;
	return s_multi_addr;
}

std::string MulticastAddr::get_addr()
{
	std::lock_guard<std::mutex> lock(m_mutex);
	std::string        addr_str;
	struct sockaddr_in addr = { 0 };
	std::random_device rd;

	for (int n = 0; n <= 10; n++) {
		uint32_t range = 0xE8FFFFFF - 0xE8000100;
		addr.sin_addr.s_addr = htonl(0xE8000100 + (rd()) % range);
		addr_str = inet_ntoa(addr.sin_addr);

		if (m_addrs.find(addr_str) != m_addrs.end()) {
			addr_str.clear();
		}
		else {
			m_addrs.insert(addr_str);
			break;
		}
	}

	return addr_str;
}

void MulticastAddr::release(std::string addr) {
	std::lock_guard<std::mutex> lock(m_mutex);
	m_addrs.erase(addr);
}