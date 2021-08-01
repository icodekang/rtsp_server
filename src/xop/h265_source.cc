#if defined(WIN32) || defined(_WIN32) 
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif
#endif

#include "h265_source.h"
#include <cstdio>
#include <chrono>
#if defined(__linux) || defined(__linux__) 
#include <sys/time.h>
#endif

H265Source::H265Source(uint32_t framerate) : m_framerate(framerate)
{
	m_payload    = 96;
	m_media_type = H265;
	m_clock_rate = 90000;
}

H265Source* H265Source::create_new(uint32_t framerate)
{
	return new H265Source(framerate);
}

void H265Source::set_framerate(uint32_t framerate)
{ 
	m_framerate = framerate; 
}

uint32_t H265Source::get_framerate() const 
{ 
	return m_framerate; 
}

std::string H265Source::get_media_description(uint16_t port)
{
	char buf[100] = {0};
	sprintf(buf, "m=video %hu RTP/AVP 96", port);
	return std::string(buf);
}
	
std::string H265Source::get_attribute()
{
	return std::string("a=rtpmap:96 H265/90000");
}

bool H265Source::handle_frame(MediaChannelId channel_id, AVData frame)
{
	uint8_t *frame_buf  = frame.buffer.get();
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
				return false;
			}          
		}
	} else {	
		char FU[3] = {0};	
		char nal_unit_type = (frame_buf[0] & 0x7E) >> 1; 
		FU[0] = (frame_buf[0] & 0x81) | (49<<1); 
		FU[1] = frame_buf[1]; 
		FU[2] = (0x80 | nal_unit_type); 
        
		frame_buf  += 2;
		frame_size -= 2;
        
		while (frame_size + 3 > MAX_RTP_PAYLOAD_SIZE) {
			RtpPacket rtp_pkt;
			rtp_pkt.type = frame.type;
			rtp_pkt.timestamp = frame.timestamp;
			rtp_pkt.size = 4 + RTP_HEADER_SIZE + MAX_RTP_PAYLOAD_SIZE;
			rtp_pkt.last = 0;

			rtp_pkt.data.get()[RTP_HEADER_SIZE+4] = FU[0];
			rtp_pkt.data.get()[RTP_HEADER_SIZE+5] = FU[1];
			rtp_pkt.data.get()[RTP_HEADER_SIZE+6] = FU[2];
			memcpy(rtp_pkt.data.get() + 4 + RTP_HEADER_SIZE + 3, frame_buf, MAX_RTP_PAYLOAD_SIZE - 3);
            
			if (m_send_frame_callback) {
				if (!m_send_frame_callback(channel_id, rtp_pkt)) {
					return false;
				}                
			}
            
			frame_buf  += (MAX_RTP_PAYLOAD_SIZE - 3);
			frame_size -= (MAX_RTP_PAYLOAD_SIZE - 3);
        
			FU[2] &= ~0x80;						
		}
        
		{
			RtpPacket rtp_pkt;
			rtp_pkt.type = frame.type;
			rtp_pkt.timestamp = frame.timestamp;
			rtp_pkt.size = 4 + RTP_HEADER_SIZE + 3 + frame_size;
			rtp_pkt.last = 1;

			FU[2] |= 0x40;
			rtp_pkt.data.get()[RTP_HEADER_SIZE+4] = FU[0];
			rtp_pkt.data.get()[RTP_HEADER_SIZE+5] = FU[1];
			rtp_pkt.data.get()[RTP_HEADER_SIZE+6] = FU[2];
			memcpy(rtp_pkt.data.get() + 4 + RTP_HEADER_SIZE + 3, frame_buf, frame_size);
            
			if (m_send_frame_callback) {
				if (!m_send_frame_callback(channel_id, rtp_pkt)) {
					return false;
				}               
			}
		}            
	}

	return true;
}


uint32_t H265Source::get_timestamp()
{
/* #if defined(__linux) || defined(__linux__) 
	struct timeval tv = {0};
	gettimeofday(&tv, NULL);
	uint32_t ts = ((tv.tv_sec*1000)+((tv.tv_usec+500)/1000))*90; // 90: _clockRate/1000;
	return ts;
#else */
	//auto time_point = chrono::time_point_cast<chrono::milliseconds>(chrono::system_clock::now());
	//auto time_point = chrono::time_point_cast<chrono::milliseconds>(chrono::steady_clock::now());
	auto time_point = std::chrono::time_point_cast<std::chrono::microseconds>(std::chrono::steady_clock::now());
	return (uint32_t)((time_point.time_since_epoch().count() + 500) / 1000 * 90);
//#endif 
}
