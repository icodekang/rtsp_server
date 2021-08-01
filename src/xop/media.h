#ifndef _MEDIA_H_
#define _MEDIA_H_

#include <memory>

/* RTSP 服 务 支 持 的 媒 体 类 型 */
enum MediaType
{
	H264 = 96,
	AAC  = 37,
	H265 = 265,   
	NONE
};	

enum FrameType
{
	VIDEO_FRAME_I = 0x01,	  
	VIDEO_FRAME_P = 0x02,
	VIDEO_FRAME_B = 0x03,    
	AUDIO_FRAME   = 0x11,   
};

struct AVData
{	
	AVData(uint32_t size = 0) :buffer(new uint8_t[size + 1], std::default_delete< uint8_t[]>())
	{
		this->size = size;
		type = 0;
		timestamp = 0;
	}

	std::shared_ptr<uint8_t> buffer; /* 帧 数 据 */
	uint32_t size;				     /* 帧 大 小 */
	uint8_t  type;				     /* 帧 类 型 */	
	uint32_t timestamp;		  	     /* 时 间 戳 */
};

static const int MAX_MEDIA_CHANNEL = 2;

enum MediaChannelId
{
	channel_0,
	channel_1
};

typedef uint32_t MediaSessionId;

#endif // _MEDIA_H_

