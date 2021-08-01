#include "rtsp.h"

Rtsp::Rtsp() : m_has_auth_info(false) 
{

}

void Rtsp::set_auth_config(std::string realm, std::string username, std::string password)
{
    m_realm    = realm;
    m_username = username;
    m_password = password;
    m_has_auth_info = true;

    if (m_realm == "" || username == "") {
        m_has_auth_info = false;
    }
}

void Rtsp::set_version(std::string version) // SDP Session Name
{ 
    m_version = std::move(version); 
}

std::string Rtsp::get_version()
{ 
    return m_version; 
}

std::string Rtsp::get_rtsp_url()
{ 
    return m_rtsp_url_info.url; 
}

bool Rtsp::parse_rtsp_url(std::string url)
{
    char     ip[100]     = { 0 };
    char     suffix[100] = { 0 };
    uint16_t port        = 0;

#if defined(__linux) || defined(__linux__)
    if (sscanf(url.c_str() + 7, "%[^:]:%hu/%s", ip, &port, suffix) == 3)
#elif defined(WIN32) || defined(_WIN32)
    if (sscanf_s(url.c_str() + 7, "%[^:]:%hu/%s", ip, 100, &port, suffix, 100) == 3)
#endif
    {
        m_rtsp_url_info.port = port;
    }
#if defined(__linux) || defined(__linux__)
    else if (sscanf(url.c_str() + 7, "%[^/]/%s", ip, suffix) == 2)
#elif defined(WIN32) || defined(_WIN32)
    else if (sscanf_s(url.c_str() + 7, "%[^/]/%s", ip, 100, suffix, 100) == 2)
#endif
    {
        m_rtsp_url_info.port = 554;
    } else {
        //LOG("%s was illegal.\n", url.c_str());
        return false;
    }

    m_rtsp_url_info.ip     = ip;
    m_rtsp_url_info.suffix = suffix;
    m_rtsp_url_info.url    = url;

    return true;
}

MediaSession::Ptr Rtsp::look_media_session(const std::string &suffix)
{ 
    return nullptr; 
}

MediaSession::Ptr Rtsp::look_media_session(MediaSessionId session_id)
{ 
    return nullptr; 
}