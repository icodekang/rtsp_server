#if defined(WIN32) || defined(_WIN32)
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif
#endif

#include "h264_source.h"
#include <cstdio>
#include <chrono>
#if defined(__linux) || defined(__linux__)
#include <sys/time.h>
#endif

H264Source::H264Source(uint32_t framerate)
	: m_framerate(framerate)
{
    m_payload    = 96; 
    m_media_type = H264;
    m_clock_rate = 90000;
}

H264Source* H264Source::create_new(uint32_t framerate)
{
    return new H264Source(framerate);
}

void H264Source::set_framerate(uint32_t framerate)
{ 
    m_framerate = framerate; 
}

uint32_t H264Source::get_framerate() const 
{ 
    return m_framerate; 
}

std::string H264Source::get_media_description(uint16_t port)
{
    char buf[100] = {0};
    sprintf(buf, "m=video %hu RTP/AVP 96", port); // \r\nb=AS:2000
    return std::string(buf);
}

std::string H264Source::get_attribute()
{
    return std::string("a=rtpmap:96 H264/90000");
}

bool H264Source::handle_frame(MediaChannelId channel_id, AVData frame)
{
    uint8_t* frame_buf  = frame.buffer.get();
    uint32_t frame_size = frame.size;

    if (frame.timestamp == 0) {
	    frame.timestamp = get_timestamp();
    }    

    if (frame_size <= MAX_RTP_PAYLOAD_SIZE) {
        RtpPacket rtp_pkt;
	    rtp_pkt.type = frame.type;
	    rtp_pkt.timestamp = frame.timestamp;
	    rtp_pkt.size = frame_size + 4 + RTP_HEADER_SIZE;
	    rtp_pkt.last = 1;
        memcpy(rtp_pkt.data.get() + 4 + RTP_HEADER_SIZE, frame_buf, frame_size); 

        if (m_send_frame_callback) {
		    if (!m_send_frame_callback(channel_id, rtp_pkt)) {
                log_error("m_send_frame_callback failed");
			    return false;
		    }               
        }
    } else {
        char FU_A[2] = {0};

        FU_A[0] = (frame_buf[0] & 0xE0) | 28;
        FU_A[1] = 0x80 | (frame_buf[0] & 0x1f);

        frame_buf  += 1;
        frame_size -= 1;

        while (frame_size + 2 > MAX_RTP_PAYLOAD_SIZE) {
            RtpPacket rtp_pkt;
            rtp_pkt.type = frame.type;
            rtp_pkt.timestamp = frame.timestamp;
            rtp_pkt.size = 4 + RTP_HEADER_SIZE + MAX_RTP_PAYLOAD_SIZE;
            rtp_pkt.last = 0;

            rtp_pkt.data.get()[RTP_HEADER_SIZE+4] = FU_A[0];
            rtp_pkt.data.get()[RTP_HEADER_SIZE+5] = FU_A[1];
            memcpy(rtp_pkt.data.get() + 4 + RTP_HEADER_SIZE + 2, frame_buf, MAX_RTP_PAYLOAD_SIZE - 2);

            if (m_send_frame_callback) {
                if (!m_send_frame_callback(channel_id, rtp_pkt)) {
                    log_error("m_send_frame_callback failed");
                    return false;
                }
            }

            frame_buf  += MAX_RTP_PAYLOAD_SIZE - 2;
            frame_size -= MAX_RTP_PAYLOAD_SIZE - 2;

            FU_A[1] &= ~0x80;
        }

        {
            RtpPacket rtp_pkt;
            rtp_pkt.type = frame.type;
            rtp_pkt.timestamp = frame.timestamp;
            rtp_pkt.size = 4 + RTP_HEADER_SIZE + 2 + frame_size;
            rtp_pkt.last = 1;

            FU_A[1] |= 0x40;
            rtp_pkt.data.get()[RTP_HEADER_SIZE+4] = FU_A[0];
            rtp_pkt.data.get()[RTP_HEADER_SIZE+5] = FU_A[1];
            memcpy(rtp_pkt.data.get() + 4 + RTP_HEADER_SIZE + 2, frame_buf, frame_size);

            if (m_send_frame_callback) {
			    if (!m_send_frame_callback(channel_id, rtp_pkt)) {
                    log_error("m_send_frame_callback failed");
				    return false;
			    }              
            }
        }
    }

    return true;
}

uint32_t H264Source::get_timestamp()
{
/* #if defined(__linux) || defined(__linux__)
    struct timeval tv = {0};
    gettimeofday(&tv, NULL);
    uint32_t ts = ((tv.tv_sec*1000)+((tv.tv_usec+500)/1000))*90; // 90: _clockRate/1000;
    return ts;
#else  */
    auto time_point = std::chrono::time_point_cast<std::chrono::microseconds>(std::chrono::steady_clock::now());
    return (uint32_t)((time_point.time_since_epoch().count() + 500) / 1000 * 90 );
//#endif
}
 