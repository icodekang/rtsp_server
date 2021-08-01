#ifndef _RTSP_SERVER_H_
#define _RTSP_SERVER_H_

#include <memory>
#include <string>
#include <mutex>
#include <unordered_map>
#include "tcp_server.h"
#include "rtsp.h"
#include "vlog.h"

class RtspConnection;

class RtspServer : public Rtsp, public TcpServer
{
public:    
	static std::shared_ptr<RtspServer> create(EventLoop *loop);
    MediaSessionId add_session(MediaSession *session);
    void remove_session(MediaSessionId session_id);
    bool push_frame(MediaSessionId session_id, MediaChannelId channel_id, AVData frame);

    ~RtspServer() = default;

private:
    friend class RtspConnection;

	RtspServer(EventLoop* loop);

    MediaSession::Ptr look_media_session(const std::string &suffix);
    MediaSession::Ptr look_media_session(MediaSessionId session_id);
    virtual TcpConnection::Ptr on_connect(SOCKET sockfd);

    std::mutex m_mutex;
    std::unordered_map<MediaSessionId, std::shared_ptr<MediaSession>> m_media_sessions;
    std::unordered_map<std::string, MediaSessionId>                   m_rtsp_suffix_map;
};

#endif // _RTSP_SERVER_H_

