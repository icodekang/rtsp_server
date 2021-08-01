#include "h264_parser.h"
#include <cstring>

Nal H264Parser::find_nal(const uint8_t *data, uint32_t size)
{
    Nal nal(nullptr, nullptr);

    if (size < 5) {
        return nal;
    }

    nal.second = const_cast<uint8_t*>(data) + (size-1);

    uint32_t start_code = 0;
    uint32_t pos       = 0;
    uint8_t prefix[3]  = {0};
    memcpy(prefix, data, 3);
    size -= 3;
    data += 2;

    while (size--) {
        if ((prefix[pos % 3] == 0) && (prefix[(pos + 1) % 3] == 0) && (prefix[(pos + 2) % 3] == 1)) {
            if (nal.first == nullptr) {
                nal.first = const_cast<uint8_t*>(data) + 1;
                start_code = 3;
            } else if(start_code == 3) {
                nal.second = const_cast<uint8_t*>(data) - 3;
                break;
            }               
        } else if ((prefix[pos % 3] == 0) && (prefix[(pos + 1) % 3] == 0) && (prefix[(pos + 2) % 3] == 0)) {
            if (*(data+1) == 0x01) {              
                if (nal.first == nullptr) {
                    if (size >= 1) {
                        nal.first = const_cast<uint8_t*>(data) + 2;
                    } else {
                        break;  
                    }                  
                    start_code = 4;
                } else if(start_code == 4) {
                    nal.second = const_cast<uint8_t*>(data) - 3;
                    break;
                }                    
            }
        }

        prefix[(pos++) % 3] = *(++data);
    }
        
    if(nal.first == nullptr)
        nal.second = nullptr;

    return nal;
}


