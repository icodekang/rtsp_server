#include "xop/rtsp_server.h"

#ifdef __cplusplus
extern "C" {
#endif
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavfilter/avfilter.h"
#include "libavutil/avutil.h"
#include "libavdevice/avdevice.h"
#ifdef __cplusplus
}
#endif

int main(int argc, char **argv)
{
	std::shared_ptr<EventLoop>  event_loop;
	std::shared_ptr<RtspServer> server;
	MediaSession  *session = NULL;
	MediaSessionId seesion_id;

	AVFormatContext *fmt_ctx = NULL;
	AVCodecContext  *dec_ctx = NULL;
	AVCodec         *dec     = NULL;
	AVDictionary    *opts    = NULL;
	AVPacket         pkt;
	int refcount           = 0;
	int video_stream_idx   = -1;
	AVMediaType media_type = AVMEDIA_TYPE_VIDEO;
	std::string url = "rtsp://admin:1026kang@192.168.0.200:554/Streaming/Channels/1";

	dvlog_init("vlog.conf", "rtsp_server");

	{
		// server
		event_loop = std::make_shared<EventLoop>();
		server = RtspServer::create(event_loop.get());
		if (!server->start("0.0.0.0", 8000)) {
			log_error("server start failed");
			return -1;
		}

		session = MediaSession::create_new("live");
		session->add_source(channel_0, H264Source::create_new());

		session->add_notify_connected_callback([] (MediaSessionId session_id, std::string peer_ip, uint16_t peer_port){
			log_info("RTSP client connect, ip=%s, port=%hu \n", peer_ip.c_str(), peer_port);
		});
	
		session->add_notify_disconnected_callback([](MediaSessionId session_id, std::string peer_ip, uint16_t peer_port) {
			log_info("RTSP client disconnect, ip=%s, port=%hu \n", peer_ip.c_str(), peer_port);
		});

		seesion_id = server->add_session(session);
	}

	{
		// ffmpeg
		av_register_all();
    	avformat_network_init();

		if (avformat_open_input(&fmt_ctx, url.c_str(), NULL, NULL) < 0) {
			log_error("could not open source");
			return -1;
		}

		if (avformat_find_stream_info(fmt_ctx, NULL) < 0) {
			log_error("could not find stream information");
			return -1;
		}

		if ((video_stream_idx = av_find_best_stream(fmt_ctx, media_type, -1, -1, NULL, 0)) < 0) {
			log_error("could not find %s stream in input", av_get_media_type_string(media_type));
			return -1;
		}
		dec_ctx = fmt_ctx->streams[video_stream_idx]->codec;
		dec = avcodec_find_decoder(dec_ctx->codec_id);
		if (!dec) {
			log_error("failed to find %s codec", av_get_media_type_string(media_type));
			return -1;
		}

		av_dict_set(&opts, "refcounted_frames", refcount ? "1": "0", 0);
		if (avcodec_open2(dec_ctx, dec, &opts) < 0) {
			log_error("failed to open %s codec", av_get_media_type_string(media_type));
			return -1;
		}

		av_init_packet(&pkt);
		while (av_read_frame(fmt_ctx, &pkt) >= 0) {
			AVData frame = {0};
			frame.type = 0;
			frame.size = pkt.size;
			frame.timestamp = H264Source::get_timestamp();
			frame.buffer.reset(new uint8_t[frame.size]);
			memcpy(frame.buffer.get(), pkt.data, frame.size);

			server->push_frame(seesion_id, channel_0, frame);

			av_packet_unref(&pkt);
		}
	}

	return 0;
}