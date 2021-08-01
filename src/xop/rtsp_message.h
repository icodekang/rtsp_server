#ifndef _RTSP_MESSAGE_H_
#define _RTSP_MESSAGE_H_

#include <utility>
#include <unordered_map>
#include <string>
#include <cstring>
#include "vlog.h"
#include "rtp.h"
#include "media.h"
#include "buffer_reader.h"

class RtspRequest
{
public:
	enum Method
	{
		OPTIONS = 0, DESCRIBE, SETUP, PLAY, TEARDOWN, GET_PARAMETER, 
		RTCP, NONE,
	};

	const char *MethodToString[8] =
	{
		"OPTIONS", "DESCRIBE", "SETUP", "PLAY", "TEARDOWN", "GET_PARAMETER",
		"RTCP", "NONE"
	};

	enum RtspRequestParseState
	{
		ParseRequestLine,
		ParseHeadersLine,
		//ParseBody,	
		GotAll,
	};

	bool   parse_request(BufferReader *buffer);
	bool   got_all() const;
	void   reset();
	Method   get_method() const;
	uint32_t get_cseq() const;
	std::string get_rtsp_url() const;
	std::string get_rtsp_url_suffix() const;
	std::string get_ip() const;
	std::string get_auth_response() const;
	TransportMode  get_transport_mode() const;
	MediaChannelId get_channel_id() const;
	uint8_t  get_rtp_channel() const;
	uint8_t  get_rtcp_channel() const;
	uint16_t get_rtp_port() const;
	uint16_t get_rtcp_port() const;
	int build_option_res(const char *buf, int buf_size);
	int build_describe_res(const char *buf, int buf_size, const char *sdp);
	int build_setup_multicast_res(const char *buf, int buf_size, const char *multicast_ip, uint16_t port, uint32_t session_id);
	int build_setup_tcp_res(const char *buf, int buf_size, uint16_t rtp_chn, uint16_t rtcp_chn, uint32_t session_id);
	int build_setup_udp_res(const char *buf, int buf_size, uint16_t rtp_chn, uint16_t rtcp_chn, uint32_t session_id);
	int build_play_res(const char *buf, int buf_size, const char *rtp_info, uint32_t session_id);
	int build_teardown_res(const char *buf, int buf_size, uint32_t session_id);
	int build_get_paramter_res(const char *buf, int buf_size, uint32_t session_id);
	int build_not_found_res(const char *buf, int buf_size);
	int build_server_error_res(const char *buf, int buf_size);
	int build_unsupported_res(const char *buf, int buf_size);
	int build_unauthorized_res(const char *buf, int buf_size, const char *realm, const char *nonce);

private:
	bool parse_request_line(const char *begin, const char *end);
	bool parse_headers_line(const char *begin, const char *end);
	bool parse_cseq(std::string &message);
	bool parse_accept(std::string &message);
	bool parse_transport(std::string &message);
	bool parse_session_id(std::string &message);
	bool parse_media_channel(std::string &message);
	bool parse_authorization(std::string &message);

	Method m_method;
	MediaChannelId m_channel_id;
	TransportMode  m_transport;
	std::string    m_auth_response;
	std::unordered_map<std::string, std::pair<std::string, uint32_t>> m_request_line_param;
	std::unordered_map<std::string, std::pair<std::string, uint32_t>> m_header_line_param;

	RtspRequestParseState m_state = ParseRequestLine;
};

class RtspResponse
{
public:
	enum Method
	{
		OPTIONS=0, DESCRIBE, ANNOUNCE, SETUP, RECORD, RTCP,
		NONE, 
	};

	bool   parse_response(BufferReader *buffer);
	Method get_method() const;
	uint32_t get_cseq() const;
	std::string get_session() const;
	void set_user_agent(const char *user_agent);
	void set_rtsp_url(const char *url);
	int build_option_req(const char *buf, int buf_size);
	int build_describe_req(const char *buf, int buf_size);
	int build_announce_req(const char *buf, int buf_size, const char *sdp);
	int build_setup_tcp_req(const char *buf, int buf_size, int channel);
	int build_record_req(const char *buf, int buf_size);

private:
	Method      m_method;
	uint32_t    m_cseq = 0;
	std::string m_user_agent;
	std::string m_rtsp_url;
	std::string m_session;
};

#endif // _RTSP_MESSAGE_H_
