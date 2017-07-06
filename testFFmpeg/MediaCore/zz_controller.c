//
//  zz_controller.c
//  testFFmpeg
//
//  Created by smart on 2017/5/23.
//  Copyright © 2017年 XiaoWoNiu2014. All rights reserved.
//

#include "zz_controller.h"

static zz_status *zz_status_alloc(ZZ_PLAY_STATUS code,const char *msg){
    zz_status *s = (zz_status *)malloc(sizeof(zz_status));
    s->statusCode = code;
    if (msg) {
        strcpy(s->msg, msg);
    }
    return s;
}
static void zz_status_free(zz_status *status){
    free(status);
}


static zz_event *zz_event_alloc(ZZ_EVENT_TYPE type,void *data) {
    zz_event *event = (zz_event *)malloc(sizeof(zz_event));
    event->data = data;
    event->type = type;
    return event;
}


static void zz_video_render(void *argc,void *data){
    zz_controller *c = (zz_controller *)argc;
    zz_event *event = zz_event_alloc(ZZ_EVENT_VIDEO_RENDER, NULL);
    event->data = data;
    zz_queue_put(c->eventQueue, event);
}

static void *zz_controller_EVENT_loop(void *argc) {
    
    zz_controller *c = (zz_controller *)argc;
    zz_event *event = NULL;
    pthread_setname_np("evnet_loop_thread");
    while(1) {
        event = zz_queue_pop_block(c->eventQueue,1);
        if (event!=NULL) { //处理命令
            if (event->type == ZZ_EVENT_OPEN) {
                
                if (c->decodeCtx == NULL) {
                    c->decodeCtx = zz_decode_context_alloc(c->bufferSize);
                    c->decodeCtx->opaque = c;
                }
                char *path = (char *)event->data;
                int ret = zz_decode_context_open(c->decodeCtx, path);
                
                c->decodeCtx->videoCallBack = zz_video_render;
                
                c->audioInfo->channels = c->decodeCtx->audio_decoder->codec_ctx->channels;
                c->audioInfo->samplerate = c->decodeCtx->audio_decoder->codec_ctx->sample_rate;
                c->audioInfo->format = c->decodeCtx->audio_decoder->codec_ctx->sample_fmt;
                c->audioInfo->byte_per_frame = av_get_bytes_per_sample(c->audioInfo->format);
                c->audioInfo->isplanar = av_get_planar_sample_fmt(c->audioInfo->format);
                
                
                c->videoInfo->width = c->decodeCtx->video_decoder->codec_ctx->width;
                c->videoInfo->height = c->decodeCtx->video_decoder->codec_ctx->height;
                c->videoInfo->fps = av_q2d(c->decodeCtx->video_decoder->codec_ctx->framerate);
                
                zz_decode_context_start(c->decodeCtx);
                
                if (ret>0) {
                    
                    c->statusCallback(c->opaque,zz_status_alloc(ZZ_PLAY_STATUS_OPEN, NULL));
                }else{
                    c->statusCallback(c->opaque,zz_status_alloc(ZZ_PLAY_STATUS_ERROR, "error: file open failure"));
                }
                
                c->status = ZZ_PLAY_STATUS_OPEN;
                
                
            }else if (event->type == ZZ_EVENT_VIDEO_RENDER){
                if (c->renderCallBack ) {
                
                    zz_video_frame *frame = event->data;
                    if (event->data!= NULL) {
                        c->renderCallBack(c->opaque,frame);
                        avpicture_free((AVPicture*)frame->frame);
                        free(frame);
                    }
                }
            }else if (event->type == ZZ_EVENT_PAUSED) {
                
                zz_decode_context_paused(c->decodeCtx);
                
                c->statusCallback(c->opaque,zz_status_alloc(ZZ_PLAY_STATUS_PAUSED, NULL));
                
            }else if (event->type == ZZ_EVENT_RESUME) {
                
                zz_decode_context_resume(c->decodeCtx);
                c->statusCallback(c->opaque,zz_status_alloc(ZZ_PLAY_STATUS_RESUME, NULL));
            }

            free(event);
            
        }
    }
    
    return NULL;
}

zz_controller * zz_controller_alloc(int buffersize,void *userData,statusCallback *callback) {
    
    zz_controller *controller = malloc(sizeof(zz_controller));
    controller->bufferSize  = buffersize;
    controller->timeStamp = 0;
    controller->statusCallback = callback;
    controller->decodeCtx = NULL;
    controller->opaque = userData;
    zz_controller_init(controller);
    return controller;
    
}


void zz_controller_init(zz_controller *controller) {
    if (controller == NULL) {
        return;
    }
    controller->eventQueue = zz_queue_alloc(8, NULL);
    controller->status = 0;
    controller->audioInfo = malloc(sizeof(zz_audio_frame));
    controller->videoInfo = malloc(sizeof(zz_video_frame));
    pthread_create(&controller->eventThreadId, NULL, zz_controller_EVENT_loop, controller);
}

void zz_controller_free(zz_controller *controller){
    
}

void zz_controller_destroy(zz_controller *controller){
    
}

void zz_controller_open(zz_controller *controller,const char *path) {

    zz_event *event = zz_event_alloc(ZZ_EVENT_OPEN, (void *)path);
    zz_queue_put(controller->eventQueue, event);
}

void zz_controller_close(zz_controller *controller) {
    zz_event *event = zz_event_alloc(ZZ_EVENT_CLOSE, NULL);
    zz_queue_put(controller->eventQueue, event);
}

void zz_controller_play(zz_controller *controller) {
    zz_event *event = zz_event_alloc(ZZ_EVENT_PLAY, NULL);
    zz_queue_put(controller->eventQueue, event);
}

void zz_controller_pause(zz_controller *controller){
    zz_event *event = zz_event_alloc(ZZ_EVENT_PAUSED, NULL);
    zz_queue_put(controller->eventQueue, event);
}

void zz_controller_resume(zz_controller *controller) {
    zz_event *event = zz_event_alloc(ZZ_EVENT_RESUME, NULL);
    zz_queue_put(controller->eventQueue, event);
}

void zz_controller_stop(zz_controller *controller) {
    zz_event *event = zz_event_alloc(ZZ_EVENT_PLAY, NULL);
    zz_queue_put(controller->eventQueue, event);
}

void zz_controller_send_event(zz_controller *controller,ZZ_EVENT_TYPE type) {
    zz_event *event = zz_event_alloc(type, NULL);
    zz_queue_put(controller->eventQueue, event);
}

void zz_controller_set_volume(zz_controller *controller,float volume){
    
}

void zz_controller_set_abort_request(zz_controller *controller,int abort) {
    
}

void zz_controller_set_area(zz_controller *controller,int x,int y,float width,float height) {
    
}




double zz_controller_get_duration(zz_controller *controller) {
    if (controller->decodeCtx != NULL) {
        return controller->decodeCtx->duration;
    }
    return 0.0;
}

double zz_controller_get_cur_time(zz_controller *controller) {
    if (controller->decodeCtx != NULL) {
        return zz_decode_context_get_current_time(controller->decodeCtx);
    }
    return 0.0;
}

int zz_controller_seek_to_time(zz_controller *controller,float time){
    if (controller->decodeCtx != NULL) {
        zz_decode_context_seek_to_time(controller->decodeCtx, time);
    }
    return -1;
}

#pragma mark - set method
void zz_controller_set_render_func(zz_controller *controller,videoOutRenderCallback *rendCallBack){
    if (rendCallBack) {
        controller->renderCallBack = rendCallBack;
    }
}
