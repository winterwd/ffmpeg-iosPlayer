//
//  zz_decoder.c
//  testFFmpeg
//
//  Created by smart on 2017/5/23.
//  Copyright © 2017年 XiaoWoNiu2014. All rights reserved.
//

#include "zz_decoder.h"


static void *convert_audio_frame(zz_decoder *decoder,AVFrame *srcFrame){
 
    uint8_t *data;
    int buffer_size = av_samples_get_buffer_size(srcFrame->linesize, decoder->codec_ctx->channels, srcFrame->nb_samples, AV_SAMPLE_FMT_S16, 1);
    data = malloc(buffer_size);
    if (decoder->swr != NULL) {
        int ret = 0;
        ret = swr_convert(
                      decoder->swr,
                      (uint8_t **)&data,
                      srcFrame->nb_samples,
                      (const uint8_t **) srcFrame->data,
                      srcFrame->nb_samples);
    }else{
        memcpy(data, srcFrame->data[0], buffer_size);
    }
    return data;
}

static void zz_decode_free(zz_decoder *decoder) {
    if (decoder) {
        if (decoder->buffer_queue) {
            zz_quque_free(decoder->buffer_queue);
            decoder->buffer_queue = NULL;
        }
        free(decoder);
    }
}

static zz_decoder * zz_decoder_alloc(AVFormatContext *fctx,enum AVMediaType type){
    if (type == AVMEDIA_TYPE_UNKNOWN) {
        return NULL;
    }
    
    zz_decoder *decoder = (zz_decoder *)malloc(sizeof(zz_decoder));
    
    int ret = av_find_best_stream(fctx, type, -1, -1, &decoder->codec, 0);
    if (ret == AVERROR_STREAM_NOT_FOUND || decoder->codec == NULL) {
        return NULL;
    }
    
    decoder->stream = fctx->streams[ret];
    decoder->codec_ctx = decoder->stream->codec;
    decoder->codec = avcodec_find_decoder(decoder->stream->codec->codec_id);
    if (!decoder->codec) {
        goto error_exit;
    }
    switch (type) {
        case AVMEDIA_TYPE_AUDIO:{
            
            if (decoder->codec_ctx->sample_fmt != AV_SAMPLE_FMT_S16) {
                // to  AV_SAMPLE_FMT_S16  2 chanels
                SwrContext *swrctx = swr_alloc();
                
                
                int64_t out_ch_layout = AV_CH_LAYOUT_STEREO;
                enum AVSampleFormat out_sample_fmt = AV_SAMPLE_FMT_S16;
                int out_sample_rate = decoder->codec_ctx->sample_rate;
                
                int64_t in_ch_layout = decoder->codec_ctx->channel_layout;
                enum AVSampleFormat in_sample_fmt = decoder->codec_ctx->sample_fmt;
                int in_sample_rate = decoder->codec_ctx->sample_rate;
                
                swr_alloc_set_opts(swrctx, out_ch_layout, out_sample_fmt, out_sample_rate, in_ch_layout, in_sample_fmt, in_sample_rate, 0, NULL);
                
                swr_init(swrctx);
                
                decoder->swr = swrctx;
            }

            
//            decoder->audio_buffer_size = av_samples_get_buffer_size(NULL, decoder->pAcodectx->channels, decoder->pAcodectx->frame_size, out_sample_fmt, 0);
//            decoder->samplerate = out_sample_rate;
//            decoder->nb_channel = decoder->pAcodectx->channels;
//            
//            //每秒的音频数据大小
//            decoder->byte_per_seconds = decoder->nb_channel*decoder->samplerate*2;
//            decoder->swr = swrctx;
            decoder->convert_func = convert_audio_frame;
            break;
        }
        case AVMEDIA_TYPE_VIDEO:
            decoder->swr = NULL;
            break;
        case AVMEDIA_TYPE_SUBTITLE:
            decoder->swr = NULL;
            decoder->sws = NULL;
            break;
        default:
            break;
    }
    
    if(avcodec_open2(decoder->codec_ctx, decoder->codec, NULL)<0){
        printf("open decode codec error occurs.\n");
        goto error_exit;
    }
    
    decoder->buffer_queue = zz_queue_alloc(10, NULL);
    
    
    return decoder;
    
error_exit:
    zz_decode_free(decoder);
    return NULL;
}


zz_decode_ctx * zz_decode_context_alloc(int buffersize){
    zz_decode_ctx *dctx = (zz_decode_ctx *)malloc(sizeof(zz_decode_ctx));
    av_register_all();
    avformat_network_init();
    dctx->format_ctx = avformat_alloc_context();
    dctx->video_st_index = AVMEDIA_TYPE_UNKNOWN;
    dctx->audio_st_index = AVMEDIA_TYPE_UNKNOWN;
    dctx->subtitle_st_index = AVMEDIA_TYPE_UNKNOWN;
    dctx->abort_req = 0;
    dctx->video_decoder = NULL;
    dctx->audio_decoder = NULL;
    dctx->subtitle_decoder = NULL;
    pthread_mutex_init(&dctx->decode_lock, NULL);
    pthread_cond_init(&dctx->decode_cond, NULL);
    return dctx;
}


void zz_decode_context_destroy(zz_decode_ctx *decode_ctx){
    
    if (decode_ctx == NULL) {
        return;
    }
    
    pthread_mutex_lock(&decode_ctx->decode_lock);
    avformat_network_deinit();
    avformat_close_input(&decode_ctx->format_ctx);
    if (decode_ctx->video_decoder) {
        zz_decode_free(decode_ctx->video_decoder);
    }
    if (decode_ctx->audio_decoder) {
        zz_decode_free(decode_ctx->audio_decoder);
    }
    if (decode_ctx->subtitle_decoder) {
        zz_decode_free(decode_ctx->subtitle_decoder);
    }
    pthread_mutex_unlock(&decode_ctx->decode_lock);
    pthread_mutex_destroy(&decode_ctx->decode_lock);
    pthread_cond_destroy(&decode_ctx->decode_cond);
    
}

int zz_decode_context_open(zz_decode_ctx *decode_ctx,const char *path){
    if (strlen(path) == 0) {
        return -1;
    }
    int ret = 0;
    ret = avformat_open_input(&decode_ctx->format_ctx, path, NULL, NULL);
    if (ret<0) {
        printf("open file:%s error : %s\n",path,av_err2str(ret));
        goto error_exit;
    }
    
    //find stream
    ret = avformat_find_stream_info(decode_ctx->format_ctx, NULL);
    if (ret<0) {
        goto error_exit;
    }
    
    //print stream info
    av_dump_format(decode_ctx->format_ctx, 0, path, 0);
    
    for (int i=0; i<decode_ctx->format_ctx->nb_streams; i++) {
        AVStream *st = decode_ctx->format_ctx->streams[i];
        if (st->codec->codec_type == AVMEDIA_TYPE_VIDEO) { //视频
            decode_ctx->video_decoder = zz_decoder_alloc(decode_ctx->format_ctx, st->codec->codec_type);
            decode_ctx->video_st_index = i;
        }else if (st->codec->codec_type == AVMEDIA_TYPE_AUDIO){ //音频
            decode_ctx->audio_decoder = zz_decoder_alloc(decode_ctx->format_ctx, st->codec->codec_type);
            decode_ctx->audio_st_index = i;
        }else if (st->codec->codec_type ==  AVMEDIA_TYPE_SUBTITLE){//文本
            decode_ctx->subtitle_decoder = zz_decoder_alloc(decode_ctx->format_ctx, st->codec->codec_type);
            decode_ctx->subtitle_st_index = i;
        }
    }
    
    
    
error_exit:
    zz_decode_context_destroy(decode_ctx);
    return 0;
}
int zz_decode_context_read_packet(zz_decode_ctx *decode_ctx){
    int ret = 0;
    ret = av_read_frame(decode_ctx->format_ctx, &decode_ctx->packet);
    if (ret<0) {
        printf("read packet error occurs.\n");
        return -2;
    }
    int size = decode_ctx->packet.size;
    while (size>0) {
        int gotframe = 0;
        zz_decoder *decoder =  NULL;
        if (decode_ctx->packet.stream_index == decode_ctx->audio_st_index) {
            if (decoder->decode_func) {
                decoder->decode_func(decoder->codec_ctx,decode_ctx->frame,&gotframe,&decode_ctx->packet);
                if (gotframe) {
                    if (decoder->convert_func) {
                        void *data = decoder->convert_func(decoder,decode_ctx->frame);
                        zz_queue_put(decoder->buffer_queue, data);
                    }
                }
            }
        }else if (decode_ctx->packet.stream_index == decode_ctx->video_st_index){
            
        }else{
            break;
        }
        
    }
    return ret;
}













#pragma mark ---
