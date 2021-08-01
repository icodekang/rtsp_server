#ifndef _TCP_SOCKET_H_
#define _TCP_SOCKET_H_

#include <cstdint>
#include <string>
#include "vlog.h"
#include "socket.h"

class TcpSocket
{
public:
    TcpSocket(SOCKET sockfd=-1);

    SOCKET create();
    bool   bind(std::string ip, uint16_t port);
    bool   listen(int backlog);
    SOCKET accept();
    bool   connect(std::string ip, uint16_t port, int timeout = 0);
    void   close();
    void   shutdown_write();
    SOCKET get_socket() const;

    virtual ~TcpSocket() = default;
    
private:
    SOCKET m_sockfd = -1;
};

#endif // _TCP_SOCKET_H_
