//
//  zz_controller.c
//  testFFmpeg
//
//  Created by smart on 2017/5/23.
//  Copyright © 2017年 XiaoWoNiu2014. All rights reserved.
//

#include "zz_controller.h"




static zz_command *zz_command_alloc(ZZ_COMMAND_TYPE type,void *data) {
    zz_command *cmd = (zz_command *)malloc(sizeof(zz_command));
    cmd->data = data;
    cmd->type = type;
    return cmd;
}


static void zz_video_render(void *argc,void *data){
    zz_controller *c = (zz_controller *)argc;
    zz_command *cmd = zz_command_alloc(ZZ_COMMAND_VIDEO_RENDER, NULL);
    cmd->data = data;
    zz_queue_put(c->commandQueue, cmd);
}

static void *zz_controller_event_loop(void *argc) {
    
    zz_controller *c = (zz_controller *)argc;
    zz_command *cmd = NULL;
    while(1) {
        cmd = zz_queue_pop_block(c->commandQueue,1);
        if (cmd!=NULL) { //处理命令
            if (cmd->type == ZZ_COMMAND_OPEN) {
                
                if (c->decodeCtx == NULL) {
                    c->decodeCtx = zz_decode_context_alloc(c->bufferSize);
                    c->decodeCtx->opaque = c;
                }
                char *path = (char *)cmd->data;
                int ret = zz_decode_context_open(c->decodeCtx, path);
                
                c->decodeCtx->videoCallBack = zz_video_render;
                
                c->audioInfo->channels = c->decodeCtx->audio_decoder->codec_ctx->channels;
                c->audioInfo->samplerate = c->decodeCtx->audio_decoder->codec_ctx->sample_rate;
                c->audioInfo->format = c->decodeCtx->audio_decoder->codec_ctx->sample_fmt;
                
                zz_decode_context_start(c->decodeCtx);
                c->statusCallback(c->opaque,ret);
                
                c->status = 1;
            }else if (cmd->type == ZZ_COMMAND_VIDEO_RENDER){
                if (c->renderCallBack ) {
                
                    zz_video_frame *frame = cmd->data;
                    if (cmd->data!= NULL) {
                        c->renderCallBack(c->opaque,frame);
                        avpicture_free((AVPicture*)frame->frame);
                        free(frame);
                    }
                }
            }

            free(cmd);
            
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
    controller->commandQueue = zz_queue_alloc(8, NULL);
    controller->status = 0;
    controller->audioInfo = malloc(sizeof(zz_audio_frame));
    controller->videoInfo = malloc(sizeof(zz_video_frame));
    pthread_create(&controller->eventThreadId, NULL, zz_controller_event_loop, controller);
}

void zz_controller_free(zz_controller *controller){
    
}

void zz_controller_destroy(zz_controller *controller){
    
}

void zz_controller_open(zz_controller *controller,const char *path) {

    zz_command *cmd = zz_command_alloc(ZZ_COMMAND_OPEN, (void *)path);
    zz_queue_put(controller->commandQueue, cmd);
}

void zz_controller_close(zz_controller *controller) {
    zz_command *cmd = zz_command_alloc(ZZ_COMMAND_CLOSE, NULL);
    zz_queue_put(controller->commandQueue, cmd);
}

void zz_controller_play(zz_controller *controller) {
    zz_command *cmd = zz_command_alloc(ZZ_COMMAND_PLAY, NULL);
    zz_queue_put(controller->commandQueue, cmd);
}

void zz_controller_pause(zz_controller *controller){
    
}

void zz_controller_resume(zz_controller *controller) {
    
}

void zz_controller_stop(zz_controller *controller) {
    zz_command *cmd = zz_command_alloc(ZZ_COMMAND_PLAY, NULL);
    zz_queue_put(controller->commandQueue, cmd);
}

void zz_controller_send_cmd(zz_controller *controller,ZZ_COMMAND_TYPE type) {
    zz_command *cmd = zz_command_alloc(type, NULL);
    zz_queue_put(controller->commandQueue, cmd);
}

void zz_controller_set_volume(zz_controller *controller,float volume){
    
}

void zz_controller_set_abort_request(zz_controller *controller,int abort) {
    
}

void zz_controller_set_area(zz_controller *controller,int x,int y,float width,float height) {
    
}




double zz_controller_get_duration(zz_controller *controller) {
    return 0.0;
}

double zz_controller_get_cur_time(zz_controller *controller) {
    return 0.0;
}


#pragma mark - set method
void zz_controller_set_render_func(zz_controller *controller,videoOutRenderCallback *rendCallBack){
    if (rendCallBack) {
        controller->renderCallBack = rendCallBack;
    }
}
