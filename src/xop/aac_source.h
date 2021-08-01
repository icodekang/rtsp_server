#ifndef _AAC_SOURCE_H_
#define _AAC_SOURCE_H_

#include "media_source.h"
#include "rtp.h"
#include "vlog.h"

class AACSource : public MediaSource
{
public:
    static AACSource* create_new(uint32_t samplerate = 44100, uint32_t channels = 2, bool has_adts = true);

    uint32_t get_samplerate() const;
    uint32_t get_channels() const;
    virtual std::string get_media_description(uint16_t port = 0);
    virtual std::string get_attribute();
    bool handle_frame(MediaChannelId channel_id, AVData frame);
    static uint32_t get_timestamp(uint32_t samplerate = 44100);

    virtual ~AACSource() = default;

private:
    AACSource(uint32_t samplerate, uint32_t channels, bool has_adts);

    uint32_t m_samplerate = 44100;  
    uint32_t m_channels   = 2;         
    bool     m_has_adts   = true;

    static const int m_ADTS_SIZE = 7;
    static const int m_AU_SIZE   = 4;
};

#endif // _AAC_SOURCE_H_
