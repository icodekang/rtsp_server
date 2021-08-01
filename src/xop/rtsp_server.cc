#include "rtsp_server.h"
#include "rtsp_connection.h"
#include "socket_util.h"

RtspServer::RtspServer(EventLoop* loop) : TcpServer(loop)
{

}

std::shared_ptr<RtspServer> RtspServer::create(EventLoop *loop)
{
	std::shared_ptr<RtspServer> server(new RtspServer(loop));
	return server;
}

MediaSessionId RtspServer::add_session(MediaSession *session)
{
    std::lock_guard<std::mutex> locker(m_mutex);

    if (m_rtsp_suffix_map.find(session->get_rtsp_url_suffix()) != m_rtsp_suffix_map.end()) {
        return 0;
    }

    std::shared_ptr<MediaSession> media_session(session); 
    MediaSessionId session_id = media_session->get_media_session_id();
	m_rtsp_suffix_map.emplace(std::move(media_session->get_rtsp_url_suffix()), session_id);
	m_media_sessions.emplace(session_id, std::move(media_session));

    return session_id;
}

void RtspServer::remove_session(MediaSessionId session_id)
{
    std::lock_guard<std::mutex> locker(m_mutex);

    auto iter = m_media_sessions.find(session_id);
    if (iter != m_media_sessions.end()) {
        m_rtsp_suffix_map.erase(iter->second->get_rtsp_url_suffix());
        m_media_sessions.erase(session_id);
    }
}

MediaSession::Ptr RtspServer::look_media_session(const std::string &suffix)
{
    std::lock_guard<std::mutex> locker(m_mutex);

    auto iter = m_rtsp_suffix_map.find(suffix);
    if (iter != m_rtsp_suffix_map.end()) {
        MediaSessionId id = iter->second;
        return m_media_sessions[id];
    }

    return nullptr;
}

MediaSession::Ptr RtspServer::look_media_session(MediaSessionId session_id)
{
    std::lock_guard<std::mutex> locker(m_mutex);

    auto iter = m_media_sessions.find(session_id);
    if (iter != m_media_sessions.end()) {
        return iter->second;
    }

    return nullptr;
}

bool RtspServer::push_frame(MediaSessionId session_id, MediaChannelId channel_id, AVData frame)
{
    std::shared_ptr<MediaSession> session_ptr = nullptr;

    {
        std::lock_guard<std::mutex> locker(m_mutex);
        auto iter = m_media_sessions.find(session_id);
        if (iter != m_media_sessions.end()) {
            session_ptr = iter->second;
        } else {
            log_error("can't find media session");
            return false;
        }
    }

    if (session_ptr != nullptr && session_ptr->get_num_client() != 0) {
        return session_ptr->handle_frame(channel_id, frame);
    }

    return false;
}

TcpConnection::Ptr RtspServer::on_connect(SOCKET sockfd)
{	
	return std::make_shared<RtspConnection>(shared_from_this(), m_event_loop->get_task_scheduler().get(), sockfd);
}