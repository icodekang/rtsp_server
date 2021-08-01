#include "tcp_socket.h"
#include "socket.h"
#include "socket_util.h"

TcpSocket::TcpSocket(SOCKET sockfd) : m_sockfd(sockfd)
{
    
}

SOCKET TcpSocket::create()
{
	m_sockfd = ::socket(AF_INET, SOCK_STREAM, 0);
	return m_sockfd;
}

bool TcpSocket::bind(std::string ip, uint16_t port)
{
	struct sockaddr_in addr = {0};			  
	addr.sin_family         = AF_INET;		  
	addr.sin_addr.s_addr    = inet_addr(ip.c_str()); 
	addr.sin_port           = htons(port);  

	if (::bind(m_sockfd, (struct sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
		log_error(" <socket=%d> bind <%s:%u> failed.\n", m_sockfd, ip.c_str(), port);
		return false;
	}

	return true;
}

bool TcpSocket::listen(int backlog)
{
	if (::listen(m_sockfd, backlog) == SOCKET_ERROR) {
		log_error("<socket=%d> listen failed.\n", m_sockfd);
		return false;
	}

	return true;
}

SOCKET TcpSocket::accept()
{
	struct sockaddr_in addr = {0};
	socklen_t addrlen       = sizeof addr;

	SOCKET socket_fd = ::accept(m_sockfd, (struct sockaddr*)&addr, &addrlen);
	return socket_fd;
}

bool TcpSocket::connect(std::string ip, uint16_t port, int timeout)
{ 
	if(!SocketUtil::connect(m_sockfd, ip, port, timeout)) {
		log_error("<socket=%d> connect failed.\n", m_sockfd);
		return false;
	}

	return true;
}

void TcpSocket::close()
{
#if defined(__linux) || defined(__linux__) 
    ::close(m_sockfd);
#elif defined(WIN32) || defined(_WIN32)
    closesocket(m_sockfd);
#else
	
#endif
	m_sockfd = 0;
}

void TcpSocket::shutdown_write()
{
	shutdown(m_sockfd, SHUT_WR);
	m_sockfd = 0;
}

SOCKET TcpSocket::get_socket() const 
{ 
	return m_sockfd; 
}