#ifndef _TIMESTAMP_H_
#define _TIMESTAMP_H_

#include <string>
#include <functional>
#include <cstdint>
#include <chrono>
#include <thread>
#include "vlog.h"

class Timestamp
{
public:
    Timestamp();

    void    reset();
    int64_t elapsed();
    static std::string localtime();

    ~Timestamp() = default;

private:
    std::chrono::time_point<std::chrono::high_resolution_clock> m_begin_time_point;
};

#endif // _TIMESTAMP_H_
