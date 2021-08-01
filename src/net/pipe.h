#ifndef _PIPE_H_
#define _PIPE_H_

#include "vlog.h"
#include "tcp_socket.h"

class Pipe
{
public:
	Pipe() = default;

	bool  create();
	int   write(void *buf, int len);
	int   read(void *buf, int len);
	void  close();
	SOCKET read() const;
	SOCKET write() const;

	~Pipe() = default;
	
private:
	SOCKET m_pipe_fd[2];
};

#endif // _PIPE_H_
