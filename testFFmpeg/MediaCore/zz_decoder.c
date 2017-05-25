//
//  zz_decoder.c
//  testFFmpeg
//
//  Created by smart on 2017/5/23.
//  Copyright © 2017年 XiaoWoNiu2014. All rights reserved.
//

#include "zz_decoder.h"


static zz_audio_frame *convert_audio_frame(zz_decoder *decoder,AVFrame *srcFrame){
 
    zz_audio_frame *audioFrame = (zz_audio_frame *)malloc(sizeof(zz_audio_frame));
    int buffer_size = av_samples_get_buffer_size(srcFrame->linesize, decoder->codec_ctx->channels, srcFrame->nb_samples, AV_SAMPLE_FMT_S16, 1);
    uint8_t *data = malloc(buffer_size);
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
    audioFrame->data = data;
    audioFrame->size = buffer_size;
    return audioFrame;
}

static void zp_audio_frame_free(void *frame) {
    zz_audio_frame *f = frame;
    if(NULL!=f && NULL!=f->data) {
        free(f->data);
    }
    
    if(NULL!=f) {
        free(f);
    }
}

static zz_video_frame *convert_video_frame(zz_decoder *decoder,AVFrame *srcFrame) {
    zz_video_frame *videoFrame = (zz_video_frame *)malloc(sizeof(zz_video_frame));
    int buffersize = avpicture_get_size(AV_PIX_FMT_YUV420P, decoder->codec_ctx->width, decoder->codec_ctx->height);
    uint8_t *data = malloc(buffersize);
    AVFrame *frame = av_frame_alloc();
    frame->width = decoder->codec_ctx->width;
    frame->height = decoder->codec_ctx->height;
    avpicture_fill((AVPicture *)frame, data, AV_PIX_FMT_YUV420P, frame->width, frame->height);


    sws_scale(decoder->sws, (const uint8_t * const *)srcFrame->data, srcFrame->linesize, 0, decoder->codec_ctx->height, frame->data, frame->linesize);
    
    videoFrame->frame = frame;
    videoFrame->fps = av_q2d(decoder->codec_ctx->framerate);
    return videoFrame;
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

void *zz_decode_loop(void *argc){
    
    zz_decode_ctx *decode_ctx = argc;
    printf(" decode thread start \n ");
    while (1) {
//        if (decode_ctx->audio_decoder->buffer_queue == NULL) {
//            usleep(100*1000);
//        }
        pthread_mutex_lock(&decode_ctx->decode_lock);
        while (zz_queue_size(decode_ctx->audio_decoder->buffer_queue)>=10 /*|| zz_queue_size(decode_ctx->video_decoder->buffer_queue)>=3*/) {
            printf("buffer reached 10 items ,wait.... \n");
            pthread_cond_wait(&decode_ctx->decode_cond, &decode_ctx->decode_lock);
        }
        pthread_mutex_unlock(&decode_ctx->decode_lock);
        
        int ret = zz_decode_context_read_packet(decode_ctx);
        if (ret == AVERROR_EOF) {
            printf("reached end of file\n");
            break;
        }
    }
    
    printf(" decode thread end \n ");
    return NULL;
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
    decoder->swr = NULL;
    decoder->sws = NULL;
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

            decoder->convert_func = (void *)convert_audio_frame;
            decoder->decode_func = avcodec_decode_audio4;
            decoder->buffer_queue = zz_queue_alloc(15, zp_audio_frame_free);

            break;
        }
        case AVMEDIA_TYPE_VIDEO:
            decoder->convert_func = (void *)convert_video_frame;
            decoder->decode_func = avcodec_decode_video2;
//            if (decoder->codec_ctx->pix_fmt != AV_PIX_FMT_YUV420P) {
                decoder->sws = sws_getContext(decoder->codec_ctx->width,
                                              decoder->codec_ctx->height,
                                              decoder->codec_ctx->pix_fmt,
                                              decoder->codec_ctx->width,
                                              decoder->codec_ctx->height,
                                              PIX_FMT_YUV420P,
                                              SWS_BILINEAR, NULL, NULL, NULL);
//            }
            decoder->buffer_queue = zz_queue_alloc(3, NULL);
            
            break;
        case AVMEDIA_TYPE_SUBTITLE:

            break;
        default:
            break;
    }
    
    if(avcodec_open2(decoder->codec_ctx, decoder->codec, NULL)<0){
        printf("open decode codec error occurs.\n");
        goto error_exit;
    }
    
    
    
    printf("decoder open success \n");
    
    return decoder;
    
error_exit:
    printf("decoder open failure \n");
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
    dctx->buffer_size = buffersize;
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
    
    assert(decode_ctx != NULL);
    
    if (decode_ctx == NULL) {
        printf(" decode context is null \n ");
        return -1;
    }
    
    printf(" open context start \n ");
    
    int ret = 1;
    ret = avformat_open_input(&(decode_ctx->format_ctx), path, NULL, NULL);
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

    decode_ctx->frame = av_frame_alloc();
    printf(" open context success \n ");
    
    return 1;
    
error_exit:
    zz_decode_context_destroy(decode_ctx);
    return -1;
}

void zz_decode_context_start(zz_decode_ctx *decode_ctx) {
    
    pthread_create(&decode_ctx->decodeThreadId, NULL, zz_decode_loop, decode_ctx);
}

int zz_decode_context_read_packet(zz_decode_ctx *decode_ctx){
    
    static AVPacket packet;
    int ret = 0;
    ret = av_read_frame(decode_ctx->format_ctx, &packet);
    if (ret<0) {
        printf("read packet error occurs.\n");
        return -2;
    }
    int size = packet.size;
    
    zz_decoder *decoder =  NULL;
    if (packet.stream_index == decode_ctx->audio_st_index) {
        decoder = decode_ctx->audio_decoder;
        
    }else if (packet.stream_index == decode_ctx->video_st_index){
        decoder = decode_ctx->video_decoder;
        usleep(10*1000);
        return -1;
    }else{
        return -1;
    }
    
//    decode_ctx->frame

    while (size>0) {
        int gotframe = 0;
        int len = 0;
        

        if (decoder->decode_func) {
            
            len = decoder->decode_func(decoder->codec_ctx,decode_ctx->frame,&gotframe,&packet);
            
            if (len < 0) {
                return -1;
            }
            
            if (gotframe) {
                if (decoder->convert_func) {
                    void *data = decoder->convert_func(decoder,decode_ctx->frame);
                    zz_queue_put(decoder->buffer_queue, data);

                }
            }
            
            if (len == 0) {
                break;
            }
            
            size -= len;
        }
        
    }
    

    av_free_packet(&packet);
    

    return ret;
}

void * zz_decode_context_get_audio_buffer(zz_decode_ctx *decode_ctx){
    
    void *data = zz_queue_pop(decode_ctx->audio_decoder->buffer_queue);
    
    pthread_mutex_lock(&decode_ctx->decode_lock);
    if (zz_queue_size(decode_ctx->audio_decoder->buffer_queue) <= 5) {
        printf("audio buffer decrease to 5 items, start read...\n");
        pthread_cond_signal(&decode_ctx->decode_cond);

    }
    pthread_mutex_unlock(&decode_ctx->decode_lock);
    return data;
}


void * zz_decode_context_get_video_buffer(zz_decode_ctx *decode_ctx){
    
    void *data = zz_queue_pop(decode_ctx->video_decoder->buffer_queue);
    
    pthread_mutex_lock(&decode_ctx->decode_lock);
    if (zz_queue_size(decode_ctx->video_decoder->buffer_queue) <= 2) {
        printf("video buffer decrease to 5 items, start read...\n");
        pthread_cond_signal(&decode_ctx->decode_cond);
        
    }
    pthread_mutex_unlock(&decode_ctx->decode_lock);
    return data;
}
