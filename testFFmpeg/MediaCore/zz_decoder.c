//
//  zz_decoder.c
//  testFFmpeg
//
//  Created by smart on 2017/5/23.
//  Copyright © 2017年 XiaoWoNiu2014. All rights reserved.
//

#include "zz_decoder.h"


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
        case AVMEDIA_TYPE_AUDIO:
            decoder->sws = NULL;
            
            break;
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
    
    if(avcodec_open2(decoder->codec_ctx, decoder->codec, NULL)<0)
        goto error_exit;
    
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
int zz_decode_context_read_packet(zz_decode_ctx *decode);
