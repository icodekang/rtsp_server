#include "rtsp_pusher.h"
#include "rtsp_connection.h"
#include "tcp_socket.h"
#include "timestamp.h"
#include <memory>

RtspPusher::RtspPusher(EventLoop *event_loop) : m_event_loop(event_loop)
{

}

std::shared_ptr<RtspPusher> RtspPusher::create(EventLoop *loop)
{
	std::shared_ptr<RtspPusher> pusher(new RtspPusher(loop));
	return pusher;
}

void RtspPusher::add_session(MediaSession *session)
{
    std::lock_guard<std::mutex> locker(m_mutex);
    m_media_session.reset(session);
}

void RtspPusher::remove_session(MediaSessionId session_id)
{
	std::lock_guard<std::mutex> locker(m_mutex);
	m_media_session = nullptr;
}

MediaSession::Ptr RtspPusher::look_media_session(MediaSessionId session_id)
{
	return m_media_session;
}

int RtspPusher::open_url(std::string url, int msec)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	static Timestamp timestamp;
	int timeout = msec;
	if (timeout <= 0) {
		timeout = 10000;
	}

	timestamp.reset();

	if (!this->parse_rtsp_url(url)) {
		log_error("rtsp url(%s) was illegal.\n", url.c_str());
		return -1;
	}

	if (m_rtsp_conn != nullptr) {
		std::shared_ptr<RtspConnection> rtsp_conn = m_rtsp_conn;
		SOCKET sockfd = rtsp_conn->get_socket();
		m_task_scheduler->add_trigger_event([sockfd, rtsp_conn]() {
			rtsp_conn->disconnect();
		});
		m_rtsp_conn = nullptr;
	}

	TcpSocket tcp_socket;
	tcp_socket.create();
	if (!tcp_socket.connect(m_rtsp_url_info.ip, m_rtsp_url_info.port, timeout)) {
		tcp_socket.close();
		return -1;
	}

	m_task_scheduler = m_event_loop->get_task_scheduler().get();
	m_rtsp_conn.reset(new RtspConnection(shared_from_this(), m_task_scheduler, tcp_socket.get_socket()));
    m_event_loop->add_trigger_event([this]() {
		m_rtsp_conn->send_options(RtspConnection::RTSP_PUSHER);
    });

	timeout -= (int)timestamp.elapsed();
	if (timeout < 0) {
		timeout = 1000;
	}

	do {
		Timer::sleep(100);
		timeout -= 100;
	} while (!m_rtsp_conn->is_record() && timeout > 0);

	if (!m_rtsp_conn->is_record()) {
		std::shared_ptr<RtspConnection> rtsp_conn = m_rtsp_conn;
		SOCKET sockfd = rtsp_conn->get_socket();
		m_task_scheduler->add_trigger_event([sockfd, rtsp_conn]() {
			rtsp_conn->disconnect();
		});
		m_rtsp_conn = nullptr;
		return -1;
	}

	return 0;
}

void RtspPusher::close()
{
	std::lock_guard<std::mutex> lock(m_mutex);

	if (m_rtsp_conn != nullptr) {
		std::shared_ptr<RtspConnection> rtsp_conn = m_rtsp_conn;
		SOCKET sockfd = rtsp_conn->get_socket();
		m_task_scheduler->add_trigger_event([sockfd, rtsp_conn]() {
			rtsp_conn->disconnect();
		});
		m_rtsp_conn = nullptr;
	}
}

bool RtspPusher::is_connected()
{
	std::lock_guard<std::mutex> lock(m_mutex);

	if (m_rtsp_conn != nullptr) {
		return (!m_rtsp_conn->is_closed());
	}

	return false;
}

bool RtspPusher::push_frame(MediaChannelId channel_id, AVData frame)
{
	std::lock_guard<std::mutex> locker(m_mutex);
	if (!m_media_session || !m_rtsp_conn) {
		return false;
	}

	return m_media_session->handle_frame(channel_id, frame);
}


RtspPusher::~RtspPusher()
{
	this->close();
}