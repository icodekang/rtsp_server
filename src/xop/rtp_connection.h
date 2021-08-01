#ifndef _RTP_CONNECTION_H_
#define _RTP_CONNECTION_H_

#include <cstdint>
#include <vector>
#include <string>
#include <memory>
#include <random>
#include "rtp.h"
#include "vlog.h"
#include "media.h"
#include "socket.h"
#include "tcp_connection.h"

class RtspConnection;

class RtpConnection
{
public:
    RtpConnection(std::weak_ptr<TcpConnection> rtsp_connection);

    void set_clock_rate(MediaChannelId channel_id, uint32_t clock_rate);
    void set_payload_type(MediaChannelId channel_id, uint32_t payload);
    bool setup_rtp_over_tcp(MediaChannelId channel_id, uint16_t rtp_channel, uint16_t rtcp_channel);
    bool setup_rtp_over_udp(MediaChannelId channel_id, uint16_t rtp_port, uint16_t rtcp_port);
    bool setup_rtp_over_multicast(MediaChannelId channel_id, std::string ip, uint16_t port);
    uint32_t get_rtp_session_id() const;
    uint16_t get_rtp_port(MediaChannelId channel_id) const;
    uint16_t get_rtcp_port(MediaChannelId channel_id) const;
    SOCKET   get_rtp_socket(MediaChannelId channel_id) const;
    SOCKET   get_rtcp_socket(MediaChannelId channel_id) const;
    std::string get_ip();
    uint16_t    get_port();
    bool is_multicast() const;
    bool is_setup(MediaChannelId channel_id) const;
    std::string get_multicast_ip(MediaChannelId channel_id) const;
    void play();
    void record();
    void teardown();
    std::string get_rtp_info(const std::string &rtsp_url);
    int  send_rtp_packet(MediaChannelId channel_id, RtpPacket pkt);
    bool is_closed() const;
    int  get_id() const;
    bool has_key_frame() const;

    virtual ~RtpConnection();

private:
    friend class RtspConnection;
    friend class MediaSession;

    void set_frame_type(uint8_t frame_type = 0);
    void set_rtp_header(MediaChannelId channel_id, RtpPacket pkt);
    int  send_rtp_over_tcp(MediaChannelId channel_id, RtpPacket pkt);
    int  send_rtp_over_udp(MediaChannelId channel_id, RtpPacket pkt);

	std::weak_ptr<TcpConnection> m_rtsp_connection;
    std::string                  m_rtsp_ip;
    uint16_t                     m_rtsp_port;

    TransportMode  m_transport_mode;
    bool           m_is_multicast = false;

	bool m_is_closed      = false;
	bool m_has_key_frame  = false;

    uint8_t  m_frame_type = 0;
    uint16_t m_local_rtp_port[MAX_MEDIA_CHANNEL];
    uint16_t m_local_rtcp_port[MAX_MEDIA_CHANNEL];
    SOCKET   m_rtpfd[MAX_MEDIA_CHANNEL];
    SOCKET   m_rtcpfd[MAX_MEDIA_CHANNEL];

    struct sockaddr_in m_peer_addr;
    struct sockaddr_in m_peer_rtp_addr[MAX_MEDIA_CHANNEL];
    struct sockaddr_in m_peer_rtcp_sddr[MAX_MEDIA_CHANNEL];
    MediaChannelInfo   m_media_channel_info[MAX_MEDIA_CHANNEL];
};

#endif // _RTP_CONNECTION_H_
