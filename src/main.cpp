#include <iostream>

#ifdef __cplusplus
extern "C"
{
    #define __STDC_CONSTANT_MACROS
    #ifndef INT64_C
    #define INT64_C
    #define UINT64_C
    #endif
    #include <stdint.h>

    #include "libavutil/avutil.h"
    #include "libavcodec/avcodec.h"
    #include "libavformat/avformat.h"
    #include "libavdevice/avdevice.h"


    #include "libswscale/swscale.h"
    #include "libswresample/swresample.h"
}
#endif

#include <iostream>
using namespace std;



int add_stream(AVStream *st, AVFormatContext *fmt_ctx, AVCodec **codec, enum AVCodecID codec_id){
    int                 id = -1;
    AVCodecContext      *codec_ctx;

    // find the encoder 
    *codec = avcodec_find_encoder(codec_id);
    if (!(*codec)) {
        fprintf(stderr, "Could not find encoder for '%s'\n", avcodec_get_name(codec_id));
        return id;
    }

    // create new stream 
    st = avformat_new_stream(fmt_ctx, *codec);
    if (!st) {
        fprintf(stderr, "Could not allocate stream\n");
        return id;
    }
    // printf("add_stream codec_id:%d nb_streams:%ld\n", codec_id, fmt_ctx->nb_streams);
    id = st->id = fmt_ctx->nb_streams-1;
    codec_ctx = st->codec;

    switch ((*codec)->type) {
        case AVMEDIA_TYPE_AUDIO:
          //codec_ctx->sample_fmt   = AV_SAMPLE_FMT_FLTP;
            codec_ctx->sample_fmt   = AV_SAMPLE_FMT_S16P;
            codec_ctx->bit_rate     = 128000;
            codec_ctx->sample_rate  = 44100;
            codec_ctx->channels     = 2;
            break;
            /*
             case AVMEDIA_TYPE_VIDEO:
                 codec_ctx->codec_id = codec_id;

                 codec_ctx->bit_rate = 400000;
                 codec_ctx->time_base.den = STREAM_FRAME_RATE;
                 codec_ctx->time_base.num = 1;
                 codec_ctx->gop_size = 12;
                 codec_ctx->pix_fmt = STREAM_PIX_FMT;
                 break;
              */
        default:
            break;
    }

    /* Some formats want stream headers to be separate. */
    if (fmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
        codec_ctx->flags |= CODEC_FLAG_GLOBAL_HEADER;

    return id;
}



// #define VIDEO_MODE
#define AUDIO_MODE

int main(int argc,char **argv) {
    const char *input_file = "wocao";
    const char *output_file = "wocao.mp3";
    input_file = argv[1];
    printf("############### %s\n", input_file);

    int ret;
    AVFormatContext *in_fmt_ctx = NULL;
#ifdef VIDEO_MODE
    int             video_idx = -1;
    // AVStream      *video_stream = NULL;
    AVCodecContext  *video_codec_ctx = NULL;
    AVCodec         *video_codec = NULL;
#endif
#ifdef AUDIO_MODE
    int             audio_idx = -1;
    // AVStream      *audio_stream = NULL;
    AVCodecContext  *audio_codec_ctx = NULL;
    AVCodec         *audio_codec = NULL;
    AVCodecContext  *tmp_codec_ctx = NULL;
    char            audio_outbuf[1024*1024];
#endif
    int             end_of_stream = 0;
    int             got_audio_frame = 0;
    AVFrame         *frame = NULL;
    AVPacket        pkt;
    
    // IN init  
    av_register_all();
    avformat_network_init();
    // IN open file
    if (avformat_open_input(&in_fmt_ctx, input_file, NULL, NULL)!=0) {
        fprintf(stderr, "Could not open source file %s\n", input_file);
        exit(-1);
    }
    // IN retrieve stream information
    if (avformat_find_stream_info(in_fmt_ctx, NULL) < 0) {
        fprintf(stderr, "Could not find stream information\n");
        exit(1);
    }
    printf("input file format:\n");
    av_dump_format(in_fmt_ctx, 0, input_file, 0);

#ifdef VIDEO_MODE
    video_idx = av_find_best_stream(in_fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
    video_codec_ctx = in_fmt_ctx->streams[video_idx]->codec;
    printf("video_stream AVMEDIA_TYPE_VIDEO:%p video_idx:%d codec_id:%d\n", AVMEDIA_TYPE_VIDEO, video_idx, video_codec_ctx->codec_id);
    video_codec = avcodec_find_decoder(video_codec_ctx->codec_id);
#endif

#ifdef AUDIO_MODE
    // IN find audio stream 
    audio_idx = av_find_best_stream(in_fmt_ctx, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
    audio_codec_ctx = in_fmt_ctx->streams[audio_idx]->codec;
    printf("audio_stream AVMEDIA_TYPE_AUDIO:%d audio_idx:%d codec_id:%d\n", AVMEDIA_TYPE_AUDIO, audio_idx, audio_codec_ctx->codec_id);
    // IN find audio decoder codec
    audio_codec = avcodec_find_decoder(audio_codec_ctx->codec_id);
    if (audio_codec == NULL){
        fprintf(stderr, "Could not find audio codec\n");
        exit(-1);
    }
    /*
    tmp_codec_ctx = avcodec_alloc_context3(audio_codec);
    if (tmp_codec_ctx == NULL) {
        fprintf(stderr, "Could not avcodec_alloc_context3\n");
        exit(-1);
    }

    ret = avcodec_copy_context(tmp_codec_ctx, audio_codec_ctx);
    if (ret < 0) {
        fprintf(stderr, "Could not avcodec_copy_context\n");
        exit(-1);
    }
    */
    // IN open audio decoder codec
    ret = avcodec_open2(audio_codec_ctx, audio_codec, NULL);
    if (ret < 0) {
        fprintf(stderr, "Could not avcodec_open2\n");
        exit(-1);
    }
    /*
    SwrContext *swr_ctx = NULL;
    swr_ctx = swr_alloc();
    if (swr_ctx == NULL) {
        exit(-1);
    }*/

    // allocate the output media context
    // 输出相关定义
    AVFormatContext         *out_fmt_ctx = NULL;
    AVOutputFormat          *out_fmt = NULL;
    AVStream                *out_audio_st = NULL;
    // OUT alloc AVFormatContent 
    ret = avformat_alloc_output_context2(&out_fmt_ctx, NULL, NULL, output_file);
    if (!out_fmt_ctx) {
        fprintf(stderr, "Could not avformat_alloc_output_context2\n");
        exit(-1);
    }
    out_fmt = out_fmt_ctx->oformat;
    
    // OUT open the output file, if needed
    if (!(out_fmt->flags & AVFMT_NOFILE)) {
        if (avio_open(&out_fmt_ctx->pb, output_file, AVIO_FLAG_WRITE) < 0) {
            fprintf(stderr, "Error: Could not open '%s'\n", output_file);
            exit(-1);
        }
    }
    
    AVCodec *t_out_audio_codec = NULL;
    //printf("codec_id:%d AV_CODEC_ID_MP3:%d AV_CODEC_ID_AAC:%d\n", audio_codec_ctx->codec_id, AV_CODEC_ID_MP3, AV_CODEC_ID_AAC);
    //printf("sample_fmt:%d\n", audio_codec_ctx->sample_fmt);
    out_fmt->audio_codec = AV_CODEC_ID_MP3;
    //out_fmt->audio_codec = AV_CODEC_ID_AAC;
    if (out_fmt->audio_codec != AV_CODEC_ID_NONE) {
        add_stream(out_audio_st, out_fmt_ctx, &t_out_audio_codec, out_fmt->audio_codec);
    }

    // Write the stream header, if any.
    ret = avformat_write_header(out_fmt_ctx, NULL);
    if (ret < 0) {
        //fprintf(stderr, "Error occurred when opening output file: %s\n", av_err2str(ret));
        exit(-1);
    }
    av_dump_format(out_fmt_ctx, 0, output_file, 1);
#endif

    frame = av_frame_alloc();
    if (frame == NULL) {
        fprintf(stderr, "Could not alloc frame\n");
        exit(-1);
    }

    int count = 0;
    while (!end_of_stream) {
        av_init_packet(&pkt);
        
        if (av_read_frame(in_fmt_ctx, &pkt)<0) {
            end_of_stream = 1;
            av_packet_unref(&pkt);
        }
#ifdef AUDIO_MODE
        if (pkt.stream_index == audio_idx) {
            // write_audio_frame(out_fmt_ctx, out_audio_st);
            /*  // 可输出文件
            pkt.stream_index = 0;
            //ret = av_interleaved_write_frame(out_fmt_ctx, &pkt);
            ret = av_write_frame(out_fmt_ctx, &pkt);
            if (ret < 0){
                fprintf(stderr, "Error while writing audio frame");
                exit(1);
            }*/ 
            
            // 开始解码 然后重新编码
            avcodec_decode_audio4(audio_codec_ctx, frame, &got_audio_frame, &pkt);
            printf("decode audio:%d pts:%ld\n", got_audio_frame, frame->pts);
            if (got_audio_frame) {
                if (count++ >= 100 )
                   break;
                 
                printf("##:src_ch_layout:%d src_nb_channels:%d src_nb_samples:%-4d src_sample_fmt:%d src_sample_rate:%d src_linesize:%d %d\n", audio_codec_ctx->channel_layout, audio_codec_ctx->channels, frame->nb_samples, audio_codec_ctx->sample_fmt, audio_codec_ctx->sample_rate, frame->linesize[0], frame->linesize[1]);
                //int data_size = audio_resampling_ex(swr_ctx, audio_codec_ctx, frame, AV_SAMPLE_FMT_S16, 1, 16000, audio_outbuf);
                //int data_size = audio_resampling_ex(swr_ctx, audio_codec_ctx, frame, AV_SAMPLE_FMT_S16P, 2, 44100, audio_outbuf);
            }

            /*
            if (got_audio_frame) {
                printf("##:src_ch_layout:%d src_nb_channels:%d src_nb_samples:%-4d src_sample_fmt:%d src_sample_rate:%d src_linesize:%d %d\n", audio_codec_ctx->channel_layout, audio_codec_ctx->channels, frame->nb_samples, audio_codec_ctx->sample_fmt, audio_codec_ctx->sample_rate, frame->linesize[0], frame->linesize[1]);
                //int data_size = audio_resampling_ex(swr_ctx, audio_codec_ctx, frame, AV_SAMPLE_FMT_S16, 1, 16000, audio_outbuf);
                int data_size = audio_resampling_ex(swr_ctx, audio_codec_ctx, frame, AV_SAMPLE_FMT_S16P, 2, 44100, audio_outbuf);
            }
            */
        }
#endif
    }

    // 转码结束
    av_write_trailer(out_fmt_ctx);


    if (in_fmt_ctx) {
        avformat_close_input(&in_fmt_ctx);
    }
    if (frame) {
        av_frame_free(&frame);
    }
    /*
    if (tmp_codec_ctx) {
        avcodec_close(tmp_codec_ctx);
        av_free(tmp_codec_ctx);
    }
    */
#ifdef AUDIO_MODE
    /*
    if (swr_ctx) {
        swr_free(&swr_ctx);
    }*/
#endif

    return 0;
}
