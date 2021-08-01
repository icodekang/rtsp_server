#ifndef _TCPSERVER_H_
#define _TCPSERVER_H_

#include <memory>
#include <string>
#include <mutex>
#include <unordered_map>
#include "vlog.h"
#include "socket.h"
#include "tcp_connection.h"

class Acceptor;
class EventLoop;

class TcpServer
{
public:	
	TcpServer(EventLoop* event_loop);

	virtual bool start(std::string ip, uint16_t port);
	virtual void stop();
	std::string  get_ip_address() const;
	uint16_t     get_port() const;

	virtual ~TcpServer();  

protected:
	virtual TcpConnection::Ptr on_connect(SOCKET sockfd);
	virtual void add_connection(SOCKET sockfd, TcpConnection::Ptr tcp_conn);
	virtual void remove_connection(SOCKET sockfd);

	EventLoop                *m_event_loop;
	uint16_t                  m_port;
	std::string               m_ip;
	std::unique_ptr<Acceptor> m_acceptor; 
	bool                      m_is_started;
	std::mutex                m_mutex;
	std::unordered_map<SOCKET, TcpConnection::Ptr> m_connections;
};

#endif // _TCPSERVER_H_
