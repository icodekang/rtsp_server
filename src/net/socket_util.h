#ifndef _SOCKET_UTIL_H_
#define _SOCKET_UTIL_H_

#include "vlog.h"
#include "socket.h"
#include <string>
  
class SocketUtil
{
public:
    static bool bind(SOCKET sockfd, std::string ip, uint16_t port);
    static void set_non_block(SOCKET fd);
    static void set_block(SOCKET fd, int write_timeout=0);
    static void set_reuse_addr(SOCKET fd);
    static void set_reuse_port(SOCKET sockfd);
    static void set_no_delay(SOCKET sockfd);
    static void set_keep_alive(SOCKET sockfd);
    static void set_no_sigpipe(SOCKET sockfd);
    static void set_send_buf_size(SOCKET sockfd, int size);
    static void set_recv_buf_size(SOCKET sockfd, int size);
    static std::string get_peer_ip(SOCKET sockfd);
    static std::string get_socket_ip(SOCKET sockfd);
    static int get_socket_addr(SOCKET sockfd, struct sockaddr_in* addr);
    static uint16_t get_peer_port(SOCKET sockfd);
    static int get_peer_addr(SOCKET sockfd, struct sockaddr_in *addr);
    static void close(SOCKET sockfd);
    static bool connect(SOCKET sockfd, std::string ip, uint16_t port, int timeout=0);
};

#endif // _SOCKET_UTIL_H_




