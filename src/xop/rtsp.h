// PHZ
// 2018-6-8

#ifndef _RTSP_H_
#define _RTSP_H_

#include <cstdio>
#include <string>
#include "media_session.h"
#include "acceptor.h"
#include "event_loop.h"
#include "socket.h"
#include "timer.h"
#include "vlog.h"

struct RtspUrlInfo
{
	std::string url;
	std::string ip;
	uint16_t port;
	std::string suffix;
};

class Rtsp : public std::enable_shared_from_this<Rtsp>
{
public:
	Rtsp();

	virtual void set_auth_config(std::string realm, std::string username, std::string password);
	virtual void set_version(std::string version);
	virtual std::string get_version();
	virtual std::string get_rtsp_url();
	bool parse_rtsp_url(std::string url);

	virtual ~Rtsp() = default;

protected:
	friend class RtspConnection;

	virtual MediaSession::Ptr look_media_session(const std::string &suffix);
	virtual MediaSession::Ptr look_media_session(MediaSessionId session_id);

	bool m_has_auth_info = false;
	std::string m_realm;
	std::string m_username;
	std::string m_password;
	std::string m_version;
	struct RtspUrlInfo m_rtsp_url_info;
};

#endif // _RTSP_H_


