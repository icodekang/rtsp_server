#include "acceptor.h"
#include "event_loop.h"
#include "socket_util.h"

Acceptor::Acceptor(EventLoop *eventLoop) : m_event_loop(eventLoop), m_tcp_socket(new TcpSocket)
{	
	
}

void Acceptor::set_new_connection_callback(const NewConnectionCallback &cb)
{
	m_new_connection_callback = cb;
}

int Acceptor::listen(std::string ip, uint16_t port)
{
	std::lock_guard<std::mutex> locker(m_mutex);

	if (m_tcp_socket->get_socket() > 0) {
		m_tcp_socket->close();
	}

	SOCKET sockfd = m_tcp_socket->create();
	m_channel_ptr.reset(new Channel(sockfd));
	SocketUtil::set_reuse_addr(sockfd);
	SocketUtil::set_reuse_port(sockfd);
	SocketUtil::set_non_block(sockfd);

	if (!m_tcp_socket->bind(ip, port)) {
		return -1;
	}

	if (!m_tcp_socket->listen(1024)) {
		return -1;
	}

	m_channel_ptr->set_read_callback([this]() { this->on_accept(); });
	m_channel_ptr->enable_reading();
	m_event_loop->update_channel(m_channel_ptr);
	return 0;
}

void Acceptor::close()
{
	std::lock_guard<std::mutex> locker(m_mutex);

	if (m_tcp_socket->get_socket() > 0) {
		m_event_loop->remove_channel(m_channel_ptr);
		m_tcp_socket->close();
	}
}

void Acceptor::on_accept()
{
	std::lock_guard<std::mutex> locker(m_mutex);

	SOCKET socket = m_tcp_socket->accept();
	if (socket > 0) {
		if (m_new_connection_callback) {
			m_new_connection_callback(socket);
		}
		else {
			SocketUtil::close(socket);
		}
	}
}

