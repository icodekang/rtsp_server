#ifndef _NET_INTERFACE_H_
#define _NET_INTERFACE_H_

#include <string>
#include "vlog.h"

class NetInterface
{
public:
    NetInterface() = default;

    static std::string get_local_ip_address();

    ~NetInterface() = default;
};

#endif // _NET_INTERFACE_H_
