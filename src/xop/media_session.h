#ifndef _MEDIA_SESSION_H_
#define _MEDIA_SESSION_H_

#include <memory>
#include <string>
#include <vector>
#include <mutex>
#include <atomic>
#include <random>
#include <cstdint>
#include <unordered_set>
#include "vlog.h"
#include "media.h"
#include "h264_source.h"
#include "h265_source.h"
#include "aac_source.h"
#include "media_source.h"
#include "socket.h"
#include "ring_buffer.h"

class RtpConnection;

class MediaSession
{
public:
	using Ptr                        = std::shared_ptr<MediaSession>;
	using NotifyConnectedCallback    = std::function<void (MediaSessionId sessionId, std::string peer_ip, uint16_t peer_port)> ;
	using NotifyDisconnectedCallback = std::function<void (MediaSessionId sessionId, std::string peer_ip, uint16_t peer_port)> ;

	static MediaSession *create_new(std::string url_suffix = "live");
	bool add_source(MediaChannelId channel_id, MediaSource *source);
	bool remove_source(MediaChannelId channel_id);
	bool start_multicast();
	void add_notify_connected_callback(const NotifyConnectedCallback &callback);
	void add_notify_disconnected_callback(const NotifyDisconnectedCallback &callback);
	std::string get_rtsp_url_suffix() const;
	void set_rtsp_url_suffix(std::string &suffix);
	std::string get_sdp_message(std::string ip, std::string session_name = "");
	MediaSource *get_media_source(MediaChannelId channel_id);
	bool handle_frame(MediaChannelId channel_id, AVData frame);
	bool add_client(SOCKET rtspfd, std::shared_ptr<RtpConnection> rtp_conn);
	void remove_client(SOCKET rtspfd);
	MediaSessionId get_media_session_id();
	uint32_t get_num_client() const;
	bool is_multicast() const;
	std::string get_multicast_ip() const;
	uint16_t get_multicast_port(MediaChannelId channel_id) const;

	virtual ~MediaSession();

private:
	friend class MediaSource;
	friend class RtspServer;
	MediaSession(std::string url_suffxx);

	MediaSessionId m_session_id = 0;
	std::string    m_suffix;
	std::string    m_sdp;

	std::vector<std::unique_ptr<MediaSource>> m_media_sources;
	std::vector<RingBuffer<AVData>>          m_buffer;

	std::vector<NotifyConnectedCallback>           m_notify_connected_callbacks;
	std::vector<NotifyDisconnectedCallback>        m_notify_disconnected_callbacks;
	std::mutex                                     m_mutex;
	std::mutex                                     m_map_mutex;
	std::map<SOCKET, std::weak_ptr<RtpConnection>> m_clients;

	bool             m_is_multicast = false;
	uint16_t         m_multicast_port[MAX_MEDIA_CHANNEL];
	std::string      m_multicast_ip;
	std::atomic_bool m_has_new_client;

	static std::atomic_uint m_last_session_id;
};

class MulticastAddr
{
public:
	MulticastAddr() = default;

	static MulticastAddr &instance();
	std::string get_addr();
	void release(std::string addr);

	~MulticastAddr() = default;

private:
	std::mutex                      m_mutex;
	std::unordered_set<std::string> m_addrs;
};

#endif
