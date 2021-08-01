#include "pipe.h"
#include "socket_util.h"
#include <random>
#include <string>
#include <array>

bool Pipe::create()
{
#if defined(WIN32) || defined(_WIN32) 
	TcpSocket rp(socket(AF_INET, SOCK_STREAM, 0)), wp(socket(AF_INET, SOCK_STREAM, 0));
	std::random_device rd;

	m_pipe_fd[0]   = rp.get_socket();
	m_pipe_fd[1]   = wp.get_socket();
	uint16_t port  = 0;
	int      again = 5;

	while(again--) {
		port = rd(); 
		if (rp.bind("127.0.0.1", port)) {
			break;
		}		
	}

	if (again == 0) {
		return false;
	}
    
	if (!rp.listen(1)) {
		return false;
	}
      
	if (!wp.connect("127.0.0.1", port)) {
		return false;
	}

	m_pipe_fd[0] = rp.accept();
	if (m_pipe_fd[0] < 0) {
		return false;
	}

	SocketUtil::set_non_block(m_pipe_fd[0]);
	SocketUtil::set_non_block(m_pipe_fd[1]);
#elif defined(__linux) || defined(__linux__) 
	if (pipe2(m_pipe_fd, O_NONBLOCK | O_CLOEXEC) < 0) {
		return false;
	}
#endif
	return true;
}

int Pipe::write(void *buf, int len)
{
#if defined(WIN32) || defined(_WIN32) 
    return ::send(m_pipe_fd[1], (char *)buf, len, 0);
#elif defined(__linux) || defined(__linux__) 
    return ::write(m_pipe_fd[1], buf, len);
#endif 
}

int Pipe::read(void *buf, int len)
{
#if defined(WIN32) || defined(_WIN32) 
    return recv(m_pipe_fd[0], (char *)buf, len, 0);
#elif defined(__linux) || defined(__linux__) 
    return ::read(m_pipe_fd[0], buf, len);
#endif 
}

void Pipe::close()
{
#if defined(WIN32) || defined(_WIN32) 
	closesocket(m_pipe_fd[0]);
	closesocket(m_pipe_fd[1]);
#elif defined(__linux) || defined(__linux__) 
	::close(m_pipe_fd[0]);
	::close(m_pipe_fd[1]);
#endif
}

SOCKET Pipe::read() const 
{ 
	return m_pipe_fd[0]; 
}

SOCKET Pipe::write() const 
{ 
	return m_pipe_fd[1]; 
}