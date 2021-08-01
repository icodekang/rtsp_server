#include "timestamp.h"
#include <iostream>
#include <iomanip>  
#include <sstream>

Timestamp::Timestamp() : m_begin_time_point(std::chrono::high_resolution_clock::now())
{

}

void Timestamp::reset()
{
    m_begin_time_point = std::chrono::high_resolution_clock::now();
}

int64_t Timestamp::elapsed()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - m_begin_time_point).count();
}

std::string Timestamp::localtime()
{
    std::ostringstream stream;
    auto   now = std::chrono::system_clock::now();
    time_t tt  = std::chrono::system_clock::to_time_t(now);
	
#if defined(WIN32) || defined(_WIN32)
    struct tm tm;
    localtime_s(&tm, &tt);
    stream << std::put_time(&tm, "%F %T");
#elif  defined(__linux) || defined(__linux__) 
    char buffer[200] = {0};
    std::string timeString;
    std::strftime(buffer, 200, "%F %T", std::localtime(&tt));
    stream << buffer;
#endif	

    return stream.str();
}