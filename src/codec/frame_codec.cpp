#include<frame_codec.hpp>

int FrameCodec::encode_write_frame(unsigned int stream_index, int flush) {
	StreamContext* stream = &stream_ctx[stream_index];
	FilteringContext* filter = &filter_ctx[stream_index];
	AVFrame* filt_frame = flush ? NULL : filter->filtered_frame;
	AVPacket* enc_pkt = filter->enc_pkt;
	int ret;

	av_log(NULL, AV_LOG_INFO, "Encoding frame\n");
	av_packet_unref(enc_pkt);

	if (filt_frame && filt_frame->pts != AV_NOPTS_VALUE) {
		//filt_frame->pts = av_rescale_q(filt_frame->pts, filter->buffersink_ctx->inputs[0]->time_base, stream->enc_ctx->time_base);
		filt_frame->pts = av_rescale_q(filt_frame->pts, filt_frame->time_base, stream->enc_ctx->time_base);
	}
	ret = avcodec_send_frame(stream->enc_ctx, filt_frame);
	if (ret < 0) {
		av_log(NULL, AV_LOG_ERROR, "avcodec_send_frame Cannot send frame for encoding %d\n", ret);
		return ret;
	}
	while (ret >= 0) {
		ret = avcodec_receive_packet(stream->enc_ctx, enc_pkt);
		if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
			return 0;
		}
		/*prepare raw packet Todo*/
		enc_pkt->stream_index = stream_index;
		av_packet_rescale_ts(enc_pkt, stream->enc_ctx->time_base, ofmt_ctx->streams[stream_index]->time_base);
		
		av_log(NULL, AV_LOG_INFO, "Muxing frame\n");
		/* mux encoded frame */
		ret = av_interleaved_write_frame(ofmt_ctx, enc_pkt);
	}

	return ret;
}


int FrameCodec::open_input_file(const char* filename) {
	int ret;
	unsigned int i;
	ifmt_ctx = NULL;
	if ((ret = avformat_open_input(&ifmt_ctx, filename, NULL, NULL)) < 0) {
		av_log(NULL, AV_LOG_ERROR, "avformat_open_input Cannot open input file\n");
		return ret;
	}
	if ((ret = avformat_find_stream_info(ifmt_ctx, NULL)) < 0) {
		av_log(NULL, AV_LOG_ERROR, "avformat_find_stream_info Cannot find stream information\n");
		return ret;
	}

	stream_ctx = reinterpret_cast<StreamContext*>(av_calloc(ifmt_ctx->nb_streams, sizeof(*stream_ctx)));
	std::cout<<"size of stream_ctx: "<<sizeof(*stream_ctx)<<std::endl;
	if (!stream_ctx) { return AVERROR(ENOMEM);}

	for (i = 0; i < ifmt_ctx->nb_streams; i++) {
		std::cout << "==== ifmt Stream index: " << i << std::endl;
		AVStream* stream = ifmt_ctx->streams[i];
		if (stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
			std::cout<<"Stream "<<i<<" is video stream\n";
		} else if (stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
			std::cout<<"Stream "<<i<<" is audio stream\n";
		} else {
			std::cout<<"Stream "<<i<<" is other type stream\n";
		}
		
		const AVCodec* dec = avcodec_find_decoder(stream->codecpar->codec_id);
		if (!dec) {
			av_log(NULL, AV_LOG_ERROR, "avcodec_find_decoder Cannot find decoder for stream\n");
		}
		AVCodecContext* codec_ctx = avcodec_alloc_context3(dec);
		if (!codec_ctx) {
			av_log(NULL, AV_LOG_ERROR, "avcodec_alloc_context3 Cannot allocate decoder context\n");
			return AVERROR(ENOMEM);
		}
		ret = avcodec_parameters_to_context(codec_ctx, stream->codecpar);
		if (ret < 0) {
			av_log(NULL, AV_LOG_ERROR, "avcodec_parameters_to_context Cannot copy codec parameters to decoder context\n");
			return ret;
		}
		codec_ctx->pkt_timebase = stream->time_base;
		std::cout<<"codec_ctx->pkt_timebase num/den: "<<codec_ctx->pkt_timebase.num<<"/"<<codec_ctx->pkt_timebase.den<<std::endl;
	
		if (codec_ctx->codec_type == AVMEDIA_TYPE_VIDEO) {
			codec_ctx->framerate = av_guess_frame_rate(ifmt_ctx, stream, NULL);
			std::cout<<"codec_ctx->framerate num/den: "<<codec_ctx->framerate.num<<"/"<<codec_ctx->framerate.den<<std::endl;
			ret = avcodec_open2(codec_ctx, dec, NULL);
			if (ret < 0) {
				av_log(NULL, AV_LOG_ERROR, "avcodec_open2 Cannot open video decoder\n");
				return ret;
			}
			std::cout<<"video codec_ctx name: "<<codec_ctx->codec->name<<" | type: "<<codec_ctx->codec_type<<std::endl;
		}
		else if (codec_ctx->codec_type == AVMEDIA_TYPE_AUDIO) {
			ret = avcodec_open2(codec_ctx, dec, NULL);
			if (ret < 0) {
				av_log(NULL, AV_LOG_ERROR, "avcodec_open2 Cannot open audio decoder\n");
				return ret;
			}
		}
		stream_ctx[i].dec_ctx = codec_ctx;
		stream_ctx[i].dec_frame = av_frame_alloc();
		if (!stream_ctx[i].dec_frame) {
			av_log(NULL, AV_LOG_ERROR, "av_frame_alloc Cannot allocate decoder frame on stream %d\n",i);
			return AVERROR(ENOMEM);
		}

	}
	std::cout<<"=====================\n";
	av_dump_format(ifmt_ctx, 0, filename, 0);	//print input file information
	return 0;
}

int FrameCodec::open_intput_cam(const char* cam_name) {
	int ret;
	unsigned int i;
	avdevice_register_all();

	const AVInputFormat* ifmt = av_find_input_format("dshow");
	AVDictionary* options = NULL;
	av_dict_set(&options, "video_size", "960*540", 0);
	av_dict_set(&options, "framerate", "30", 0);
	av_dict_set(&options, "fflags", "nobuffer", 0);
	av_dict_set(&options, "framedrop", "1", 0);
	av_dict_set(&options, "flags", "low_delay", 0);


	if ((ret = avformat_open_input(&ifmt_ctx, cam_name, ifmt, &options)) < 0) {
		av_log(NULL, AV_LOG_ERROR, "avformat_open_input Cannot open input file\n");
		return ret;
	}
	if ((ret = avformat_find_stream_info(ifmt_ctx, NULL)) < 0) {
		av_log(NULL, AV_LOG_ERROR, "avformat_find_stream_info Cannot find stream information\n");
		return ret;
	}
	stream_ctx = reinterpret_cast<StreamContext*>(av_calloc(ifmt_ctx->nb_streams, sizeof(*stream_ctx)));
	std::cout << "size of stream_ctx: " << sizeof(*stream_ctx) << std::endl;
	if (!stream_ctx) {
		return AVERROR(ENOMEM);
	}
	for (i = 0; i < ifmt_ctx->nb_streams; i++) {
		AVStream* stream = ifmt_ctx->streams[i];
		if (stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
			std::cout << "Stream " << i << " is video stream\n";
		}
		else if (stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
			std::cout << "Stream " << i << " is audio stream\n";
		}
		else {
			std::cout << "Stream " << i << " is other type stream\n";
		}

		const AVCodec* dec = avcodec_find_decoder(stream->codecpar->codec_id);
		if (!dec) {
			av_log(NULL, AV_LOG_ERROR, "avcodec_find_decoder Cannot find decoder for stream\n");
		}
		AVCodecContext* codec_ctx = avcodec_alloc_context3(dec);
		if (!codec_ctx) {
			av_log(NULL, AV_LOG_ERROR, "avcodec_alloc_context3 Cannot allocate decoder context\n");
			return AVERROR(ENOMEM);
		}
		ret = avcodec_parameters_to_context(codec_ctx, stream->codecpar);
		if (ret < 0) {
			av_log(NULL, AV_LOG_ERROR, "avcodec_parameters_to_context Cannot copy codec parameters to decoder context\n");
			return ret;
		}
		codec_ctx->pkt_timebase = stream->time_base;
		std::cout << "codec_ctx->pkt_timebase num/den: " << codec_ctx->pkt_timebase.num << "/" << codec_ctx->pkt_timebase.den << std::endl;

		if (codec_ctx->codec_type == AVMEDIA_TYPE_VIDEO) {
			codec_ctx->framerate = av_guess_frame_rate(ifmt_ctx, stream, NULL);
			std::cout << "codec_ctx->framerate num/den: " << codec_ctx->framerate.num << "/" << codec_ctx->framerate.den << std::endl;
			ret = avcodec_open2(codec_ctx, dec, NULL);
			if (ret < 0) {
				av_log(NULL, AV_LOG_ERROR, "avcodec_open2 Cannot open video decoder\n");
				return ret;
			}
			std::cout << "video codec_ctx name: " << codec_ctx->codec->name << " | type: " << codec_ctx->codec_type << std::endl;
		}
		else if (codec_ctx->codec_type == AVMEDIA_TYPE_AUDIO) {
			ret = avcodec_open2(codec_ctx, dec, NULL);
			if (ret < 0) {
				av_log(NULL, AV_LOG_ERROR, "avcodec_open2 Cannot open audio decoder\n");
				return ret;
			}
		}
		stream_ctx[i].dec_ctx = codec_ctx;
		stream_ctx[i].dec_frame = av_frame_alloc();
		if (!stream_ctx[i].dec_frame) {
			av_log(NULL, AV_LOG_ERROR, "av_frame_alloc Cannot allocate decoder frame on stream %d\n", i);
		}

		std::cout << "=====================\n";
		av_dump_format(ifmt_ctx, 0, cam_name, 0);	//print input file information
		return 0;
	}
}

int FrameCodec::open_output_file(const char* filename) {
	std::cout<<"=====================\n";
	std::cout<<"open_output_file\n";
	AVStream *out_stream;
	AVStream *in_stream;
	AVCodecContext *dec_ctx, *enc_ctx;
	const AVCodec *encoder;
	int ret;
	unsigned int i;

	ofmt_ctx = NULL;

	avformat_alloc_output_context2(&ofmt_ctx, NULL, NULL, filename);
	if (!ofmt_ctx) {
		av_log(NULL, AV_LOG_ERROR, "avformat_alloc_output_context2 Cannot allocate output context\n");
		return AVERROR_UNKNOWN;
	}
	for (i = 0; i < ifmt_ctx->nb_streams; i++) {
		out_stream = avformat_new_stream(ofmt_ctx, NULL);
		if (!out_stream) {
			av_log(NULL, AV_LOG_ERROR, "avformat_new_stream Cannot create new stream\n");
			return AVERROR_UNKNOWN;
		}

		in_stream = ifmt_ctx->streams[i];
		std::cout<<"number of stream: "<<i<<std::endl;
		dec_ctx = stream_ctx[i].dec_ctx;
		if (dec_ctx->codec_type == AVMEDIA_TYPE_VIDEO || dec_ctx->codec_type == AVMEDIA_TYPE_AUDIO) {
			if (dec_ctx->codec_type == AVMEDIA_TYPE_VIDEO) {
				//If raw video from CAM, choose a encoder
				if (dec_ctx->codec_id == AV_CODEC_ID_RAWVIDEO) {
					std::cout<<"==== dev: dec_ctx rawvideo\n";
					//encoder = avcodec_find_encoder(AV_CODEC_ID_MPEG4);
					encoder = avcodec_find_encoder(AV_CODEC_ID_H264);
				}
				else {
					encoder = avcodec_find_encoder(dec_ctx->codec_id); //Use the same encoder as the input stream
				}
			}
			else if (dec_ctx->codec_type == AVMEDIA_TYPE_AUDIO) {
				encoder = avcodec_find_encoder(dec_ctx->codec_id); //Use the same encoder as the input stream
			}
			if (!encoder) {
				av_log(NULL, AV_LOG_ERROR, "avcodec_find_encoder Cannot find encoder\n");
				return AVERROR_INVALIDDATA;
			}

			if (encoder->id == AV_CODEC_ID_H264) {
				CoUninitialize();
			}

			enc_ctx = avcodec_alloc_context3(encoder);
			if (!enc_ctx) {
				av_log(NULL, AV_LOG_ERROR, "avcodec_alloc_context3 Cannot allocate encoder context\n");
				return AVERROR(ENOMEM);
			}

			if (dec_ctx->codec_type == AVMEDIA_TYPE_VIDEO) {
				enc_ctx->height = dec_ctx->height;
				enc_ctx->width = dec_ctx->width;
				enc_ctx->sample_aspect_ratio = dec_ctx->sample_aspect_ratio;
				enc_ctx->time_base = av_inv_q(dec_ctx->framerate);
				//enc_ctx->framerate = dec_ctx->framerate;
				//enc_ctx->gop_size = 0;
				//enc_ctx->max_b_frames = 0;
				//enc_ctx->bit_rate = 400000;
				//enc_ctx->codec_tag = 0;
				if (enc_ctx->pix_fmt) {
					enc_ctx->pix_fmt = encoder->pix_fmts[0];
				}
				else {
					enc_ctx->pix_fmt = dec_ctx->pix_fmt;
				}
			}
			else if (dec_ctx->codec_type == AVMEDIA_TYPE_AUDIO) {
				enc_ctx->sample_rate = dec_ctx->sample_rate;
				ret = av_channel_layout_copy(&enc_ctx->ch_layout, &dec_ctx->ch_layout);
				if (ret < 0) {
					av_log(NULL, AV_LOG_ERROR, "av_channel_layout_copy Cannot copy channel layout\n");
					return ret;
				}
				enc_ctx->sample_fmt = encoder->sample_fmts[0];
				enc_ctx->time_base = { 1, enc_ctx->sample_rate };
				//enc_ctx->bit_rate = 64000;
				//enc_ctx->codec_tag = 0;
			}

			if (ofmt_ctx->oformat->flags & AVFMT_GLOBALHEADER) {
				enc_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
			}

			ret = avcodec_open2(enc_ctx, encoder, NULL);
			if (ret < 0) {
				av_log(NULL, AV_LOG_ERROR, "avcodec_open2 Cannot open encoder\n");
				return ret;
			}
			ret = avcodec_parameters_from_context(out_stream->codecpar, enc_ctx);
			if (ret < 0) {
				av_log(NULL, AV_LOG_ERROR, "avcodec_parameters_from_context Failed to copy codec parameters to output stream\n");
				return ret;
			}
			out_stream->time_base = enc_ctx->time_base;
			stream_ctx[i].enc_ctx = enc_ctx;
		} 
		else if (dec_ctx->codec_type == AVMEDIA_TYPE_UNKNOWN) {
			av_log(NULL, AV_LOG_FATAL, "Elementary stream #%d is of unknown type, cannot proceed\n", i);
			return AVERROR_INVALIDDATA;
		}
		else {
			std::cout<<"Unknown type, but still proceed to muxing"<<std::endl;
			ret = avcodec_parameters_copy(out_stream->codecpar, in_stream->codecpar);
			if (ret < 0) {
				av_log(NULL, AV_LOG_ERROR, "avcodec_parameters_copy Failed to copy codec parameters to output stream\n");
				return ret;
			}
			out_stream->time_base = in_stream->time_base;
		}
	}
	std::cout<<"=====================av_dump_format\n";
	av_dump_format(ofmt_ctx, 0, filename, 1);
	std::cout << "=====================av_dump_format\n";
	if (!(ofmt_ctx->oformat->flags & AVFMT_NOFILE)) {
		ret = avio_open(&ofmt_ctx->pb, filename, AVIO_FLAG_WRITE);
		if (ret < 0) {
			av_log(NULL, AV_LOG_ERROR, "avio_open Cannot open output file '%s'\n", filename);
			return ret;
		}
	}
	ret = avformat_write_header(ofmt_ctx, NULL);
	if (ret < 0) {
		av_log(NULL, AV_LOG_ERROR, "avformat_write_header Cannot write header\n");
		return ret;
	}
	//End of main
	return 0;
}


int FrameCodec::init_filter(FilteringContext* fctx, AVCodecContext* dec_ctx, AVCodecContext* enc_ctx, const char* filter_spec) {
	char args[512];
	int ret = 0;
	const AVFilter* buffersrc = NULL;
	const AVFilter* buffersink = NULL;
	AVFilterContext* buffersrc_ctx = NULL;
	AVFilterContext* buffersink_ctx = NULL;
	AVFilterInOut* outputs = avfilter_inout_alloc();
	AVFilterInOut* inputs = avfilter_inout_alloc();
	AVFilterGraph* filter_graph = avfilter_graph_alloc();
	
	if (!outputs || !inputs || !filter_graph) {
		ret = AVERROR(ENOMEM);
		goto end;
	}

	if (dec_ctx->codec_type == AVMEDIA_TYPE_VIDEO) {
		std::cout<<"==== Video filter ====\n";
		buffersrc = avfilter_get_by_name("buffer");
		buffersink = avfilter_get_by_name("buffersink");
		if (!buffersrc || !buffersink) {
			av_log(NULL, AV_LOG_ERROR, "avfilter_get_by_name Cannot find buffer source or sink\n");
			ret = AVERROR_UNKNOWN;
			goto end;
		}
		snprintf(args, sizeof(args),
			"video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
			dec_ctx->width, dec_ctx->height, dec_ctx->pix_fmt,
			dec_ctx->pkt_timebase.num, dec_ctx->pkt_timebase.den,
			dec_ctx->sample_aspect_ratio.num,
			dec_ctx->sample_aspect_ratio.den);

		ret = avfilter_graph_create_filter(&buffersrc_ctx, buffersrc, "in", args, NULL, filter_graph);
		//ret = avfilter_graph_create_filter(&buffersrc_ctx, buffersrc, "in", NULL, NULL, filter_graph);
		if (ret < 0) {
			av_log(NULL, AV_LOG_ERROR, "avfilter_graph_create_filter Cannot create buffer source\n");
			goto end;
		}

		ret = avfilter_graph_create_filter(&buffersink_ctx, buffersink, "out", NULL, NULL, filter_graph);
		if (ret < 0) {
			av_log(NULL, AV_LOG_ERROR, "avfilter_graph_create_filter Cannot create buffer sink\n");
			goto end;
		}

		ret = av_opt_set_bin(buffersink_ctx, "pix_fmts", (uint8_t*)&enc_ctx->pix_fmt, sizeof(enc_ctx->pix_fmt), AV_OPT_SEARCH_CHILDREN);
		if (ret < 0) {
			av_log(NULL, AV_LOG_ERROR, "av_opt_set_bin Cannot set output pixel format\n");
			goto end;
		}
	} else if(dec_ctx->codec_type == AVMEDIA_TYPE_AUDIO) {
		std::cout<<"==== Audio filter ====\n";
		char buf[64];
		buffersrc = avfilter_get_by_name("abuffer");
		buffersink = avfilter_get_by_name("abuffersink");
		if (!buffersrc || !buffersink) {
			av_log(NULL, AV_LOG_ERROR, "avfilter_get_by_name Cannot find audio buffer source or sink\n");
			ret = AVERROR_UNKNOWN;
			goto end;
		}

		if (dec_ctx->ch_layout.order == AV_CHANNEL_ORDER_UNSPEC) {
			av_channel_layout_default(&dec_ctx->ch_layout, dec_ctx->ch_layout.nb_channels);
		}
		av_channel_layout_describe(&dec_ctx->ch_layout, buf, sizeof(buf));
		
		snprintf(args, sizeof(args),
			"time_base=%d/%d:sample_rate=%d:sample_fmt=%s:channel_layout=%s",
			dec_ctx->pkt_timebase.num, dec_ctx->pkt_timebase.den, dec_ctx->sample_rate,
			av_get_sample_fmt_name(dec_ctx->sample_fmt), buf);

		ret = avfilter_graph_create_filter(&buffersrc_ctx, buffersrc, "in", args, NULL, filter_graph);
		//ret = avfilter_graph_create_filter(&buffersrc_ctx, buffersrc, "in", args, NULL, filter_graph);
		if (ret < 0) {
			av_log(NULL, AV_LOG_ERROR, "avfilter_graph_create_filter Cannot create audio buffer source\n");
			goto end;
		}
		ret = avfilter_graph_create_filter(&buffersink_ctx, buffersink, "out", NULL, NULL, filter_graph);
		if (ret < 0) {
			av_log(NULL, AV_LOG_ERROR, "avfilter_graph_create_filter Cannot create audio buffer sink\n");
			goto end;
		}
		ret = av_opt_set_bin(buffersink_ctx, "sample_fmts", (uint8_t*)&enc_ctx->sample_fmt, sizeof(enc_ctx->sample_fmt), AV_OPT_SEARCH_CHILDREN);
		if (ret < 0) {
			av_log(NULL, AV_LOG_ERROR, "av_opt_set_bin Cannot set output sample format\n");
			goto end;
		}
		av_channel_layout_describe(&enc_ctx->ch_layout, buf, sizeof(buf));
		ret = av_opt_set(buffersink_ctx, "ch_layouts", buf, AV_OPT_SEARCH_CHILDREN);
		if (ret < 0) {
			av_log(NULL, AV_LOG_ERROR, "av_opt_set Cannot set output channel layout\n");
			goto end;
		}
		ret = av_opt_set_bin(buffersink_ctx, "sample_rates",(uint8_t*)&enc_ctx->sample_rate, sizeof(enc_ctx->sample_rate),AV_OPT_SEARCH_CHILDREN);
		if (ret < 0) {
			av_log(NULL, AV_LOG_ERROR, "av_opt_set_bin CCannot set output sample rate\n");
			goto end;
		}
	} else {
		ret = AVERROR_UNKNOWN;
		goto end;
	}
	/* Endpoints for the filter graph. */
	outputs->name = av_strdup("in");
	outputs->filter_ctx = buffersrc_ctx;
	outputs->pad_idx = 0;
	outputs->next = NULL;

	inputs->name = av_strdup("out");
	inputs->filter_ctx = buffersink_ctx;
	inputs->pad_idx = 0;
	inputs->next = NULL;

	if (!outputs->name || !inputs->name) {
		ret = AVERROR(ENOMEM);
		goto end;
	}
	if ((ret = avfilter_graph_parse_ptr(filter_graph, filter_spec, &inputs, &outputs, NULL)) < 0) {
		av_log(NULL, AV_LOG_ERROR, "avfilter_graph_parse_ptr Cannot parse filter graph\n");
		goto end;
	}
	if ((ret = avfilter_graph_config(filter_graph, NULL)) < 0) {
		av_log(NULL, AV_LOG_ERROR, "avfilter_graph_config Cannot configure filter graph\n");
		goto end;
	}
	fctx->buffersrc_ctx = buffersrc_ctx;
	fctx->buffersink_ctx = buffersink_ctx;
	fctx->filter_graph = filter_graph;

end:
	avfilter_inout_free(&inputs);
	avfilter_inout_free(&outputs);

	return ret;
}


int FrameCodec::init_filters(void) {
	const char* filter_spec;
	unsigned int i;
	int ret;
	filter_ctx = reinterpret_cast<FilteringContext*>(av_malloc_array(ifmt_ctx->nb_streams, sizeof(*filter_ctx)));
	if (!filter_ctx)
		return AVERROR(ENOMEM);

	for (i = 0; i < ifmt_ctx->nb_streams; i++) {
		filter_ctx[i].buffersrc_ctx = NULL;
		filter_ctx[i].buffersink_ctx = NULL;
		filter_ctx[i].filter_graph = NULL;
		if (!(ifmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO
			|| ifmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO))
			continue;


		if (ifmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
			filter_spec = "null"; /* passthrough (dummy) filter for video */
		else
			filter_spec = "anull"; /* passthrough (dummy) filter for audio */
		ret = init_filter(&filter_ctx[i], stream_ctx[i].dec_ctx,
			stream_ctx[i].enc_ctx, filter_spec);
		if (ret)
			return ret;

		filter_ctx[i].enc_pkt = av_packet_alloc();
		if (!filter_ctx[i].enc_pkt)
			return AVERROR(ENOMEM);

		filter_ctx[i].filtered_frame = av_frame_alloc();
		if (!filter_ctx[i].filtered_frame)
			return AVERROR(ENOMEM);
	}
	return 0;
}


int FrameCodec::filter_encode_write_frame(AVFrame* frame, unsigned int stream_index) {
	FilteringContext* filter = &filter_ctx[stream_index];
	int ret;

	av_log(NULL, AV_LOG_INFO, "Pushing decoded frame to filters\n");
	ret = av_buffersrc_add_frame_flags(filter->buffersrc_ctx, frame, 0);
	if (ret < 0) {
		av_log(NULL, AV_LOG_ERROR, "av_buffersrc_add_frame_flags Cannot add frame to buffer source\n");
		return ret;
	}
	/* pull filtered frames from the filtergraph */
	while (1) {
		av_log(NULL, AV_LOG_INFO, "Pulling filtered frame from filters\n");
		ret = av_buffersink_get_frame(filter->buffersink_ctx, filter->filtered_frame);
		if (ret < 0) {
			/* if no more frames for output - returns AVERROR(EAGAIN)
				* if flushed and no more frames for output - returns AVERROR_EOF
				* rewrite retcode to 0 to show it as normal procedure completion
				*/
			if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) return 0;
			break;
		}
		filter->filtered_frame->time_base = av_buffersink_get_time_base(filter->buffersink_ctx);
		filter->filtered_frame->pict_type = AV_PICTURE_TYPE_NONE;
		ret = encode_write_frame(stream_index, 0);

		av_frame_unref(filter->filtered_frame);
		if (ret < 0) break;
	}
	return ret;
}


int FrameCodec::flush_encoder(unsigned int stream_index) {
	if (!(stream_ctx[stream_index].enc_ctx->codec->capabilities & AV_CODEC_CAP_DELAY)) {
		return 0;
	}
	av_log(NULL, AV_LOG_INFO, "Flushing stream #%u encoder\n", stream_index);
	return encode_write_frame(stream_index, 1);
}


void FrameCodec::free_resource() {
	unsigned int i;
	for (i = 0; i < ifmt_ctx->nb_streams; i++) {
		avcodec_free_context(&stream_ctx[i].dec_ctx);
		if (ofmt_ctx && ofmt_ctx->nb_streams > i && ofmt_ctx->streams[i] && stream_ctx[i].enc_ctx)
			avcodec_free_context(&stream_ctx[i].enc_ctx);
		if (filter_ctx && filter_ctx[i].filter_graph) {
			avfilter_graph_free(&filter_ctx[i].filter_graph);
			av_packet_free(&filter_ctx[i].enc_pkt);
			av_frame_free(&filter_ctx[i].filtered_frame);
		}

		av_frame_free(&stream_ctx[i].dec_frame);
	}
	av_free(filter_ctx);
	av_free(stream_ctx);
	avformat_close_input(&ifmt_ctx);
	if (ofmt_ctx && !(ofmt_ctx->oformat->flags & AVFMT_NOFILE))
		avio_closep(&ofmt_ctx->pb);
	avformat_free_context(ofmt_ctx);
}