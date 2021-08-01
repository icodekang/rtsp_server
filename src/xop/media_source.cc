#include "media_source.h"

MediaType MediaSource::get_media_type() const
{ 
    return m_media_type; 
}

void MediaSource::set_send_frame_callback(const SendFrameCallback callback)
{ 
    m_send_frame_callback = callback; 
}

uint32_t MediaSource::get_payload_type() const
{ 
    return m_payload; 
}

uint32_t MediaSource::get_clock_rate() const
{ 
    return m_clock_rate; 
}