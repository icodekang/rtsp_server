#ifndef _H264_SOURCE_H_
#define _H264_SOURCE_H_

#include "media_source.h"
#include "rtp.h"
#include "vlog.h"

class H264Source : public MediaSource
{
public:
	static H264Source *create_new(uint32_t framerate = 25);

	void set_framerate(uint32_t framerate);
	uint32_t get_framerate() const;
	virtual std::string get_media_description(uint16_t port); 
	virtual std::string get_attribute(); 
	bool handle_frame(MediaChannelId channel_id, AVData frame);
	static uint32_t get_timestamp();

	~H264Source() = default;
	
private:
	H264Source(uint32_t framerate);

	uint32_t m_framerate = 25;
};

#endif // _H264_SOURCE_H_



