#ifndef _ACCEPTOR_H_
#define _ACCEPTOR_H_

#include <functional>
#include <memory>
#include <mutex>
#include "vlog.h"
#include "channel.h"
#include "tcp_socket.h"

typedef std::function<void(SOCKET)> NewConnectionCallback;

class EventLoop;

class Acceptor
{
public:	
	Acceptor(EventLoop* event_loop);

	void set_new_connection_callback(const NewConnectionCallback &cb);
	int  listen(std::string ip, uint16_t port);
	void close();

	virtual ~Acceptor() = default;

private:
	void on_accept();

	EventLoop*                 m_event_loop = nullptr;
	std::mutex                 m_mutex;
	std::unique_ptr<TcpSocket> m_tcp_socket;
	ChannelPtr                 m_channel_ptr;
	NewConnectionCallback      m_new_connection_callback;
};

#endif // _ACCEPTOR_H_