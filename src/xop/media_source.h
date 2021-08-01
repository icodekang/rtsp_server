#ifndef _MEDIA_SOURCE_H_
#define _MEDIA_SOURCE_H_

#include "media.h"
#include "vlog.h"
#include "rtp.h"
#include "socket.h"
#include <string>
#include <memory>
#include <cstdint>
#include <functional>
#include <map>

class MediaSource
{
public:
	using SendFrameCallback = std::function<bool (MediaChannelId channel_id, RtpPacket pkt)>;

	MediaSource() = default;

	virtual MediaType get_media_type() const;
	virtual std::string get_media_description(uint16_t port=0) = 0;
	virtual std::string get_attribute()  = 0;
	virtual bool handle_frame(MediaChannelId channel_id, AVData frame) = 0;
	virtual void set_send_frame_callback(const SendFrameCallback callback);
	virtual uint32_t get_payload_type() const;
	virtual uint32_t get_clock_rate() const;

	virtual ~MediaSource() = default;

protected:
	MediaType m_media_type = NONE;
	uint32_t  m_payload    = 0;
	uint32_t  m_clock_rate = 0;
	SendFrameCallback m_send_frame_callback;
};

#endif // _MEDIA_SOURCE_H_
