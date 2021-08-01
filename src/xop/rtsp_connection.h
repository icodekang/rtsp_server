#ifndef _CONNECTION_H_
#define _CONNECTION_H_

#include "event_loop.h"
#include "tcp_connection.h"
#include "rtp_connection.h"
#include "rtsp_message.h"
#include "digest_authentication.h"
#include "rtsp.h"
#include "vlog.h"
#include <iostream>
#include <functional>
#include <memory>
#include <vector>
#include <cstdint>

class RtspServer;
class MediaSession;

class RtspConnection : public TcpConnection
{
public:
	using CloseCallback = std::function<void (SOCKET sockfd)>;

	enum ConnectionMode 
	{
		RTSP_SERVER, 
		RTSP_PUSHER,
		//RTSP_CLIENT,
	};

	enum ConnectionState 
	{
		START_CONNECT,
		START_PLAY,
		START_PUSH
	};

	RtspConnection() = delete;
	RtspConnection(std::shared_ptr<Rtsp> rtsp_server, TaskScheduler *task_scheduler, SOCKET sockfd);

	MediaSessionId get_media_session_id();
	TaskScheduler *get_task_scheduler() const;
	void keep_alive();
	bool is_alive() const;
	void reset_alive_count();
	int  get_id() const;
	bool is_play() const;
	bool is_record() const;

	virtual ~RtspConnection() = default;

private:
	friend class RtpConnection;
	friend class MediaSession;
	friend class RtspServer;
	friend class RtspPusher;

	bool on_read(BufferReader &buffer);
	void on_close();
	void handle_rtcp(SOCKET sockfd);
	void handle_rtcp(BufferReader &buffer);   
	bool handle_rtsp_request(BufferReader &buffer);
	bool handle_rtsp_response(BufferReader &buffer);
	void send_rtsp_message(std::shared_ptr<char> buf, uint32_t size);
	void handle_cmd_option();
	void handle_cmd_describe();
	void handle_cmd_setup();
	void handle_cmd_play();
	void handle_cmd_teardown();
	void handle_cmd_get_paramter();
	bool handle_authentication();

	void send_options(ConnectionMode mode = RTSP_SERVER);
	void send_describe();
	void send_announce();
	void send_setup();
	void handle_record();

	std::atomic_int     m_alive_count;
	std::weak_ptr<Rtsp> m_rtsp;
	TaskScheduler      *m_task_scheduler = nullptr;

	ConnectionMode  m_conn_mode  = RTSP_SERVER;
	ConnectionState m_conn_state = START_CONNECT;
	MediaSessionId  m_session_id = 0;

	bool        m_has_auth = true;
	std::string m_nonce;
	std::unique_ptr<DigestAuthentication> m_auth_info;

	std::shared_ptr<Channel>       m_rtp_channel;
	std::shared_ptr<Channel>       m_rtcp_channels[MAX_MEDIA_CHANNEL];
	std::unique_ptr<RtspRequest>   m_rtsp_request;
	std::unique_ptr<RtspResponse>  m_rtsp_response;
	std::shared_ptr<RtpConnection> m_rtp_conn;
};

#endif // _CONNECTION_H_
