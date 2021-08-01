#ifndef _H265_SOURCE_H_
#define _H265_SOURCE_H_

#include "media_source.h"
#include "rtp.h"
#include "vlog.h"

class H265Source : public MediaSource
{
public:
	static H265Source* create_new(uint32_t framerate = 25);
	void set_framerate(uint32_t framerate);
	uint32_t get_framerate() const;
	virtual std::string get_media_description(uint16_t port = 0); 
	virtual std::string get_attribute(); 
	bool handle_frame(MediaChannelId channel_id, AVData frame);
	static uint32_t get_timestamp();

	~H265Source() = default;
	 
private:
	H265Source(uint32_t framerate);

	uint32_t m_framerate = 25;
};

#endif // _H265_SOURCE_H_


