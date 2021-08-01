#if defined(WIN32) || defined(_WIN32)
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif
#endif
#include "aac_source.h"
#include <stdlib.h>
#include <cstdio>
#include <chrono>
#if defined(__linux) || defined(__linux__)
#include <sys/time.h>
#endif

AACSource::AACSource(uint32_t samplerate, uint32_t channels, bool has_adts)
	: m_samplerate(samplerate)
	, m_channels(channels)
	, m_has_adts(has_adts)
{
	m_payload    = 97;
	m_media_type = AAC;
	m_clock_rate = samplerate;
}

uint32_t AACSource::get_samplerate() const
{ 
	return m_samplerate; 
}

uint32_t AACSource::get_channels() const
{ 
	return m_channels; 
}

AACSource* AACSource::create_new(uint32_t samplerate, uint32_t channels, bool has_adts)
{
    return new AACSource(samplerate, channels, has_adts);
}

std::string AACSource::get_media_description(uint16_t port)
{
	char buf[100] = { 0 };
	sprintf(buf, "m=audio %hu RTP/AVP 97", port); // \r\nb=AS:64
	return std::string(buf);
}

static uint32_t AACSampleRate[16] =
{
	97000, 88200, 64000, 48000,
	44100, 32000, 24000, 22050,
	16000, 12000, 11025, 8000,
	7350, 0, 0, 0 /*reserved */
};

std::string AACSource::get_attribute()  // RFC 3640
{
	char buf[500] = { 0 };
	sprintf(buf, "a=rtpmap:97 MPEG4-GENERIC/%u/%u\r\n", m_samplerate, m_channels);

	uint8_t index = 0;
	for (index = 0; index < 16; index++) {
		if (AACSampleRate[index] == m_samplerate) {
			break;
		}
	}

	if (index == 16) {
		return ""; // error
	}
     
	uint8_t profile = 1;
	char config[10] = {0};

	sprintf(config, "%02x%02x", (uint8_t)((profile+1) << 3) | (index >> 1), (uint8_t)((index << 7) | (m_channels << 3)));
	sprintf(buf+strlen(buf),
			"a=fmtp:97 profile-level-id=1;"
			"mode=AAC-hbr;"
			"sizelength=13;indexlength=3;indexdeltalength=3;"
			"config=%04u",
			atoi(config));

	return std::string(buf);
}



bool AACSource::handle_frame(MediaChannelId channel_id, AVData frame)
{
	if (frame.size > (MAX_RTP_PAYLOAD_SIZE - m_AU_SIZE)) {
		return false;
	}

	int adts_size = 0;
	if (m_has_adts) {
		adts_size = m_ADTS_SIZE;
	}

	uint8_t *frame_buf = frame.buffer.get() + adts_size; 
	uint32_t frame_size = frame.size - adts_size;

	char AU[m_AU_SIZE] = { 0 };
	AU[0] = 0x00;
	AU[1] = 0x10;
	AU[2] = (frame_size & 0x1fe0) >> 5;
	AU[3] = (frame_size & 0x1f)   << 3;

	RtpPacket rtp_pkt;
	rtp_pkt.type = frame.type;
	rtp_pkt.timestamp = frame.timestamp;
	rtp_pkt.size = frame_size + 4 + RTP_HEADER_SIZE + m_AU_SIZE;
	rtp_pkt.last = 1;

	rtp_pkt.data.get()[4 + RTP_HEADER_SIZE + 0] = AU[0];
	rtp_pkt.data.get()[4 + RTP_HEADER_SIZE + 1] = AU[1];
	rtp_pkt.data.get()[4 + RTP_HEADER_SIZE + 2] = AU[2];
	rtp_pkt.data.get()[4 + RTP_HEADER_SIZE + 3] = AU[3];

	memcpy(rtp_pkt.data.get() + 4 + RTP_HEADER_SIZE + m_AU_SIZE, frame_buf, frame_size);

	if (m_send_frame_callback) {
		m_send_frame_callback(channel_id, rtp_pkt);
	}

	return true;
}

uint32_t AACSource::get_timestamp(uint32_t sample_rate)
{
	//auto time_point = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now());
	//return (uint32_t)(time_point.time_since_epoch().count() * sampleRate / 1000);

	// ???
	auto time_point = std::chrono::time_point_cast<std::chrono::microseconds>(std::chrono::steady_clock::now());
	return (uint32_t)((time_point.time_since_epoch().count()+500) / 1000 * sample_rate / 1000);
}
