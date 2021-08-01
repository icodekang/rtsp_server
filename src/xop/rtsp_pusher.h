#ifndef _RTSP_PUSHER_H_
#define _RTSP_PUSHER_H_

#include <mutex>
#include <map>
#include "vlog.h"
#include "rtsp.h"

class RtspConnection;

class RtspPusher : public Rtsp
{
public:
	static std::shared_ptr<RtspPusher> create(EventLoop *loop);
	void add_session(MediaSession *session);
	void remove_session(MediaSessionId session_id);
	int  open_url(std::string url, int msec = 3000);
	void close();
	bool is_connected();
	bool push_frame(MediaChannelId channel_id, AVData frame);

	~RtspPusher();

private:
	friend class RtspConnection;

	RtspPusher(EventLoop *event_loop);
	
	MediaSession::Ptr look_media_session(MediaSessionId session_id);

	EventLoop     *m_event_loop     = nullptr;
	TaskScheduler *m_task_scheduler = nullptr;
	std::mutex     m_mutex;
	std::shared_ptr<RtspConnection> m_rtsp_conn;
	std::shared_ptr<MediaSession>   m_media_session;
};

#endif // _RTSP_PUSHER_H_