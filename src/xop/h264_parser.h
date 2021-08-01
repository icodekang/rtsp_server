#ifndef _H264_PARSER_H_
#define _H264_PARSER_H_

#include <cstdint> 
#include <utility>
#include "vlog.h"

typedef std::pair<uint8_t*, uint8_t*> Nal; // <nal begin, nal end>

class H264Parser
{
public:    
    H264Parser() = default;

    static Nal find_nal(const uint8_t *data, uint32_t size);

    ~H264Parser() = default;
};

#endif // _H264_PARSER_H_

