//
//  zz_decoder.c
//  testFFmpeg
//
//  Created by smart on 2017/5/23.
//  Copyright © 2017年 XiaoWoNiu2014. All rights reserved.
//

#include "zz_decoder.h"
#include <sys/time.h>
#define ZZ_AUDIO_QUEUE_SIZE 256
#define ZZ_VIDEO_QUEUE_SIZE 100

#define ZZ_PACKET_QUEUE_SIZE 256
#define ZZ_PACKET_QUEUE_CACHE_SIZE ZZ_PACKET_QUEUE_SIZE*0.75

int64_t zz_decode_context_get_audio_timestamp(zz_decode_ctx *ctx);
int zz_decode_context_read_packet1(zz_decode_ctx *decode_ctx);



int64_t zz_gettime(void)
 {

    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (int64_t)tv.tv_sec * 1000000 + tv.tv_usec;
}

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

static void zz_audio_frame_free(void *frame) {
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
    videoFrame->timebase = av_frame_get_best_effort_timestamp(srcFrame);
//    printf("videoframe->timebase =  %lld  \n",videoFrame->timebase);
    videoFrame->size = buffersize;
    return videoFrame;
}

static void zz_video_frame_free(void *frame) {
    zz_video_frame *f = frame;
    if (f!= NULL && f->frame!=NULL) {
        avpicture_free((AVPicture *)f->frame);
    }
    
    if(NULL!=f) {
        free(f);
    }
}


static void zz_packet_free(void *data){
    AVPacket *packet = data;
    if (packet != NULL) {
        av_free_packet(packet);
        packet = NULL;
    }
    
}



static int zz_decode_packet(zz_decode_ctx *decode_ctx,AVPacket *packet){
    
    zz_decoder *decoder = NULL;
    if (packet->stream_index == decode_ctx->audio_st_index) {
        decoder = decode_ctx->audio_decoder;
    }else if (packet->stream_index == decode_ctx->video_st_index){
        decoder = decode_ctx->video_decoder;
    }
    
    while (packet->size>0) {
        int gotframe = 0;
        int len = 0;
        
        
        if (decoder->decode_func) {
            
            len = decoder->decode_func(decoder->codec_ctx,decoder->frame,&gotframe,packet);
            
            if (len < 0) {
                return -1;
            }
            
            if (gotframe) {
                if (decoder->convert_func) {
                    void *data = decoder->convert_func(decoder,decoder->frame);
                    if (decode_ctx->packet.stream_index == decode_ctx->audio_st_index) {
                        zz_audio_frame *frame = (zz_audio_frame *)data;
                        frame->timebase = decode_ctx->packet.pts;
                    }
                    zz_queue_put(decoder->buffer_queue, data);
                }
            }
            
            if (len == 0) {
                break;
            }
            
            packet->size -= len;
        }
    }
    
    return 1;
    
}



void zz_decoder_free(zz_decoder *decoder) {
    if (decoder) {
        if (decoder->buffer_queue) {
            zz_quque_free(decoder->buffer_queue);
            decoder->buffer_queue = NULL;
        }
        if (decoder->codec_ctx != NULL) {
            avcodec_close(decoder->codec_ctx);
            avcodec_free_context(&decoder->codec_ctx);
        }
        
        if (decoder->swr != NULL) {
            swr_free(&decoder->swr);
        }
       
        if (decoder->sws != NULL) {
            sws_freeContext(decoder->sws);
            decoder->sws = NULL;
        }
        free(decoder);
    }
}


void *zz_video_loop(void *argc) {
    zz_decode_ctx *decode_ctx = argc;
    
    while (1) {

        if (decode_ctx->audioTimestamp ==0 ) {
        
            continue;
        }
        
        
        
        zz_video_frame *pframe = zz_queue_pop_block(decode_ctx->video_decoder->buffer_queue,1);
        if ( pframe == NULL) {
            break;
        }
        
        float presentTime = pframe->timebase*decode_ctx->video_timebase*1000;
        
        
        
     
        
        if (decode_ctx->start_time == 0) {
            decode_ctx->start_time = zz_gettime();
            decode_ctx->first_frame_time = presentTime;
            decode_ctx->current_frame_time = presentTime;
        }
        
        int64_t ctime = zz_gettime();
        float interval = (ctime -  decode_ctx->start_time)/1000.0;
        
        float playdt = presentTime - decode_ctx->first_frame_time;
        
//        printf("ctime: %lld starttime:%lld frame_time:%f first_frame_time:%f    \ndt = %lld   playdt = %lld  \n",ctime,decode_ctx->start_time,presentTime,decode_ctx->first_frame_time,dt,playdt);
        //ctime: 1496375016525682 starttime:1496375016525623 frame_time:0.187500 first_frame_time:0.000000
//        dt = 59   playdt = 0
        
        float delay = presentTime - decode_ctx->current_frame_time;
        
        float diff = interval - playdt;
        
        printf("diff  %f",diff);
        
        if (diff>=200) {
            zz_video_frame_free(pframe);
            continue;
        }
        
        
        printf("interval : %f  ,playdt: %f  \n",interval,playdt);
        
        if (decode_ctx->videoCallBack) {
            
            usleep(delay*1000);
            
            decode_ctx->videoCallBack(decode_ctx->opaque,pframe);
            
            decode_ctx->current_frame_time = presentTime;
        }
        
        



    }
    return NULL;
}


void *zz_video_loop1(void *argc) {
    zz_decode_ctx *decode_ctx = argc;
    
    while (1) {
        AVPacket *packet = zz_queue_pop_block(decode_ctx->video_queue,1);
        zz_decode_packet(decode_ctx, packet);
        zz_packet_free(packet);
        zz_video_frame *pframe = zz_queue_pop(decode_ctx->video_decoder->buffer_queue);
        
        if (pframe == NULL) {
            usleep(50000);
            continue;
        }
        
        float presentTime = FFMAX(0, pframe->timebase*decode_ctx->video_timebase*1000-3000);
        
        
        
        
        
        if (decode_ctx->start_time == 0) {
            decode_ctx->start_time = zz_gettime();
            decode_ctx->first_frame_time = presentTime;
            decode_ctx->current_frame_time = presentTime;
        }
        
        int64_t ctime = zz_gettime();
        float interval = (ctime -  decode_ctx->start_time)/1000.0;
        
        float playdt = presentTime - decode_ctx->first_frame_time;
        
        //        printf("ctime: %lld starttime:%lld frame_time:%f first_frame_time:%f    \ndt = %lld   playdt = %lld  \n",ctime,decode_ctx->start_time,presentTime,decode_ctx->first_frame_time,dt,playdt);
        //ctime: 1496375016525682 starttime:1496375016525623 frame_time:0.187500 first_frame_time:0.000000
        //        dt = 59   playdt = 0
        
        float delay = presentTime - decode_ctx->current_frame_time;
        
        
        float diff = interval - playdt;
        
        if (diff>delay) {
            delay *= 0.8;
        }
        printf("diff  %f delay %f  ",diff,delay);
        

        printf("interval : %f  ,playdt: %f  \n",interval,playdt);
        
        if (decode_ctx->videoCallBack) {
            
            usleep(delay*1000);
            
            decode_ctx->videoCallBack(decode_ctx->opaque,pframe);
            
            decode_ctx->current_frame_time = presentTime;
        }
    }
    return NULL;
}

void *zz_decode_loop(void *argc){
    
    zz_decode_ctx *decode_ctx = argc;
    printf(" decode thread start \n ");
    while (1) {

        pthread_mutex_lock(&decode_ctx->decode_lock);
        while (zz_queue_size(decode_ctx->video_decoder->buffer_queue)>=ZZ_VIDEO_QUEUE_SIZE/2 /*|| zz_queue_size(decode_ctx->video_decoder->buffer_queue)>=40*/) {
//            printf("buffer reached 10 items ,wait.... \n");
            printf("audio queue size: %d , video queue size %d \n",zz_queue_size(decode_ctx->audio_decoder->buffer_queue),zz_queue_size(decode_ctx->video_decoder->buffer_queue));
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

void *zz_decode_loop1(void *argc){
    
    zz_decode_ctx *decode_ctx = argc;
    printf(" decode thread start \n ");
    while (1) {
        
        pthread_mutex_lock(&decode_ctx->decode_lock);
        while (zz_queue_size(decode_ctx->audio_queue)>=ZZ_PACKET_QUEUE_CACHE_SIZE) {
            //            printf("buffer reached 10 items ,wait.... \n");
            printf("packet queue size: %d \n",zz_queue_size(decode_ctx->audio_queue));
            pthread_cond_wait(&decode_ctx->decode_cond, &decode_ctx->decode_lock);
        }
        pthread_mutex_unlock(&decode_ctx->decode_lock);
        
        
        
        int ret = zz_decode_context_read_packet1(decode_ctx);
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
    
    decoder->codec = avcodec_find_decoder(decoder->stream->codec->codec_id);
    
    if (!decoder->codec) { //未找到解码器
        goto error_exit;
    }
    decoder->codec_ctx =  avcodec_alloc_context3(decoder->codec); //创建解码器上下文
    ret = avcodec_copy_context(decoder->codec_ctx, decoder->stream->codec); //拷贝流里面的解码信息
    
    if (ret != 0) {
        goto error_exit;
    }
    
    if(avcodec_open2(decoder->codec_ctx, decoder->codec, NULL)<0){ //打开解码器
        printf("open decode codec error occurs.\n");
        goto error_exit;
    }
    
    decoder->swr = NULL;
    decoder->sws = NULL;


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
            decoder->buffer_queue = zz_queue_alloc(ZZ_AUDIO_QUEUE_SIZE, zz_audio_frame_free);
            decoder->frame = av_frame_alloc();

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
            decoder->buffer_queue = zz_queue_alloc(ZZ_VIDEO_QUEUE_SIZE, zz_video_frame_free);
            decoder->frame = av_frame_alloc();
            break;
        case AVMEDIA_TYPE_SUBTITLE:

            break;
        default:
            break;
    }
    

    

    printf("decoder open success \n");
    
    return decoder;
    
error_exit:
    printf("decoder open failure :%s \n", av_err2str(ret));
    zz_decoder_free(decoder);
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
    av_init_packet(&dctx->packet);
    dctx->audioTimestamp = 0;
    dctx->videoTimestamp = 0;
    dctx->duration = 0;
    dctx->audioBytePerSecond = 0;
    dctx->audioFrameBufferSize = 0;
    dctx->lastVideoTimeStamp = 0;
    dctx->start_time = 0;
    dctx->audio_queue = zz_queue_alloc(ZZ_PACKET_QUEUE_SIZE, zz_packet_free);
    dctx->video_queue = zz_queue_alloc(ZZ_PACKET_QUEUE_SIZE, zz_packet_free);
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
    if (decode_ctx->video_decoder) {
        zz_decoder_free(decode_ctx->video_decoder);
    }
    if (decode_ctx->audio_decoder) {
        zz_decoder_free(decode_ctx->audio_decoder);
    }
    if (decode_ctx->subtitle_decoder) {
        zz_decoder_free(decode_ctx->subtitle_decoder);
    }
    
    if (decode_ctx->frame) {
        av_frame_free(&decode_ctx->frame);
    }
    
    avformat_close_input(&decode_ctx->format_ctx);
    pthread_mutex_unlock(&decode_ctx->decode_lock);
    pthread_mutex_destroy(&decode_ctx->decode_lock);
    pthread_cond_destroy(&decode_ctx->decode_cond);
    
}


static void zz_decode_context_init_fps_timebase(AVStream *st,float default_timebase,float *fps,float *timebase){
    if (st->time_base.den && st->time_base.num)
        *timebase = av_q2d(st->time_base);
    else if (st->codec->time_base.den && st->codec->time_base.num)
        *timebase = av_q2d(st->codec->time_base);
    else
        *timebase = default_timebase;
    
    if (fps){
        if (st->avg_frame_rate.den && st->avg_frame_rate.num)
            *fps = av_q2d(st->avg_frame_rate);
        else if (st->r_frame_rate.den && st->r_frame_rate.num)
            *fps = av_q2d(st->r_frame_rate);
        else
            *fps = 1.0 / *timebase;
    }
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
            zz_decode_context_init_fps_timebase(decode_ctx->video_decoder->stream, 0.04f, &(decode_ctx->fps), &(decode_ctx->video_timebase));
        }else if (st->codec->codec_type == AVMEDIA_TYPE_AUDIO){ //音频
            decode_ctx->audio_decoder = zz_decoder_alloc(decode_ctx->format_ctx, st->codec->codec_type);
            decode_ctx->audio_st_index = i;
            decode_ctx->audioBytePerSecond = decode_ctx->audio_decoder->codec_ctx->channels*decode_ctx->audio_decoder->codec_ctx->sample_rate*2;
            zz_decode_context_init_fps_timebase(decode_ctx->audio_decoder->stream, 0.025, 0, &(decode_ctx->audio_timebase));
       
        }else if (st->codec->codec_type ==  AVMEDIA_TYPE_SUBTITLE){//文本
            decode_ctx->subtitle_decoder = zz_decoder_alloc(decode_ctx->format_ctx, st->codec->codec_type);
            decode_ctx->subtitle_st_index = i;
        }
    }

    
    printf("video_timebase  = %f  fps = %f  audio_timebase = %f\n",decode_ctx->video_timebase,decode_ctx->fps,decode_ctx->audio_timebase);
    
    decode_ctx->frame = av_frame_alloc();
    

    
    if (decode_ctx->format_ctx->duration != AV_NOPTS_VALUE) {//采用文件的时长
        decode_ctx->duration = decode_ctx->format_ctx->duration/AV_TIME_BASE;
    }else if (decode_ctx->video_st_index != AVMEDIA_TYPE_UNKNOWN){ //采用视频的时长
        AVStream *st =  decode_ctx->video_decoder->stream;
        decode_ctx->duration = st->duration * av_q2d(st->time_base);
    }else if (decode_ctx->audio_st_index != AVMEDIA_TYPE_UNKNOWN){//采用音频的时长
        AVStream *st =  decode_ctx->audio_decoder->stream;
        decode_ctx->duration = st->duration * av_q2d(st->time_base);
    }else{ //时长获取失败
        decode_ctx->duration = 0;
    }
    
    
    

    printf(" open context success \n ");
    
    return 1;
    
error_exit:
    zz_decode_context_destroy(decode_ctx);
    return -1;
}

void zz_decode_context_start(zz_decode_ctx *decode_ctx) {
    
    pthread_create(&decode_ctx->decodeThreadId, NULL, zz_decode_loop1, decode_ctx);
    pthread_create(&decode_ctx->videoThreadId, NULL, zz_video_loop1, decode_ctx);
}

int zz_decode_context_read_packet(zz_decode_ctx *decode_ctx){
    

    int ret = 0;
    
    
    ret = av_read_frame(decode_ctx->format_ctx, &decode_ctx->packet);
    if (ret<0) {
        printf("read packet error occurs.\n");
        return -2;
    }
    int size = decode_ctx->packet.size;
    
    zz_decoder *decoder =  NULL;
    if (decode_ctx->packet.stream_index == decode_ctx->audio_st_index) {
        decoder = decode_ctx->audio_decoder;

        
    }else if (decode_ctx->packet.stream_index == decode_ctx->video_st_index){
        
        decoder = decode_ctx->video_decoder;
        
    }else{
        return -1;
    }

    while (size>0) {
        int gotframe = 0;
        int len = 0;
        

        if (decoder->decode_func) {
            
            len = decoder->decode_func(decoder->codec_ctx,decode_ctx->frame,&gotframe,&decode_ctx->packet);
            
            if (len < 0) {
                return -1;
            }
            
            if (gotframe) {
                if (decoder->convert_func) {
                    void *data = decoder->convert_func(decoder,decode_ctx->frame);
                    if (decode_ctx->packet.stream_index == decode_ctx->audio_st_index) {
                        zz_audio_frame *frame = (zz_audio_frame *)data;
                        frame->timebase = decode_ctx->packet.pts;
                    }
                    zz_queue_put(decoder->buffer_queue, data);
                }
            }
            
            if (len == 0) {
                break;
            }
            
            size -= len;
        }
        
    }
    
    av_free_packet(&decode_ctx->packet);
    av_freep(&decode_ctx->packet);

    return ret;
}

void * zz_decode_context_get_audio_buffer(zz_decode_ctx *decode_ctx){
    

    
    AVPacket *packet = zz_queue_pop_block(decode_ctx->audio_queue,1);
    zz_decode_packet(decode_ctx, packet);
    zz_packet_free(packet);
    zz_audio_frame *pframe = zz_queue_pop(decode_ctx->audio_decoder->buffer_queue);
    if (pframe == NULL) {
        return NULL;
    }
    pthread_mutex_lock(&decode_ctx->decode_lock);
   
    if (zz_queue_size(decode_ctx->audio_decoder->buffer_queue) < ZZ_PACKET_QUEUE_CACHE_SIZE) {
//        printf("audio buffer decrease to 50 items, start read...\n");
        pthread_cond_signal(&decode_ctx->decode_cond);

    }
    pthread_mutex_unlock(&decode_ctx->decode_lock);
    return (void *)pframe;
}



void * zz_decode_context_get_video_buffer(zz_decode_ctx *decode_ctx){
    
   
    AVPacket *packet = zz_queue_pop(decode_ctx->video_queue);
    zz_decode_packet(decode_ctx, packet);
    zz_packet_free(packet);
    void *data = zz_queue_pop(decode_ctx->video_decoder->buffer_queue);
    
    
    pthread_mutex_lock(&decode_ctx->decode_lock);
    if (zz_queue_size(decode_ctx->video_decoder->buffer_queue) <ZZ_VIDEO_QUEUE_SIZE/2) {
        printf("video buffer decrease to 5 items, start read...\n");
        pthread_cond_signal(&decode_ctx->decode_cond);
        
    }
    pthread_mutex_unlock(&decode_ctx->decode_lock);
    
    return data;
}

int zz_decode_context_read_packet1(zz_decode_ctx *decode_ctx){
    
    AVPacket *packet  = av_malloc(sizeof(AVPacket));
    

    int ret = av_read_frame(decode_ctx->format_ctx, packet);
    if (ret>=0) {
        
        if (packet->stream_index == decode_ctx->audio_st_index) { //audio packet
            zz_queue_put(decode_ctx->audio_queue,packet);
        }else if (packet->stream_index == decode_ctx->video_st_index){ //video packet
            zz_queue_put(decode_ctx->video_queue, packet);
        }else if (packet->stream_index == decode_ctx->subtitle_st_index){//subtitle packet
            av_free_packet(packet);
        }

        
    }else{
        av_free_packet(packet);
        if (decode_ctx->format_ctx->pb->error == 0) {
            usleep(10*1000);
        }
        
    }
    return ret;
}
