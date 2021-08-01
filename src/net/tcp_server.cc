#include "tcp_server.h"
#include "acceptor.h"
#include "event_loop.h"
#include <cstdio>  

TcpServer::TcpServer(EventLoop* event_loop)
	: m_event_loop(event_loop)
	, m_port(0)
	, m_acceptor(new Acceptor(m_event_loop))
	, m_is_started(false)
{
	m_acceptor->set_new_connection_callback([this](SOCKET sockfd) {
		TcpConnection::Ptr conn = this->on_connect(sockfd);
		if (conn) {
			this->add_connection(sockfd, conn);
			conn->set_disconnect_callback([this](TcpConnection::Ptr conn) {
				auto scheduler = conn->get_task_scheduler();
				SOCKET sockfd = conn->get_socket();
				if (!scheduler->add_trigger_event([this, sockfd] {this->remove_connection(sockfd); })) {
					scheduler->add_timer([this, sockfd]() {this->remove_connection(sockfd); return false; }, 100);
				}
			});
		}
	});
}

bool TcpServer::start(std::string ip, uint16_t port)
{
	stop();

	if (!m_is_started) {
		if (m_acceptor->listen(ip, port) < 0) {
			return false;
		}

		m_port       = port;
		m_ip         = ip;
		m_is_started = true;

		return true;
	}

	return false;
}

void TcpServer::stop()
{
	if (m_is_started) {		
		m_mutex.lock();
		for (auto iter : m_connections) {
			iter.second->disconnect();
		}
		m_mutex.unlock();

		m_acceptor->close();
		m_is_started = false;

		while (1) {
			Timer::sleep(10);
			if (m_connections.empty()) {
				break;
			}
		}
	}	
}

std::string TcpServer::get_ip_address() const
{ 
	return m_ip; 
}

uint16_t TcpServer::get_port() const 
{ 
	return m_port; 
}

TcpConnection::Ptr TcpServer::on_connect(SOCKET sockfd)
{
	return std::make_shared<TcpConnection>(m_event_loop->get_task_scheduler().get(), sockfd);
}

void TcpServer::add_connection(SOCKET sockfd, TcpConnection::Ptr conn)
{
	std::lock_guard<std::mutex> locker(m_mutex);
	m_connections.emplace(sockfd, conn);
}

void TcpServer::remove_connection(SOCKET sockfd)
{
	std::lock_guard<std::mutex> locker(m_mutex);
	m_connections.erase(sockfd);
}

TcpServer::~TcpServer()
{
	stop();
}