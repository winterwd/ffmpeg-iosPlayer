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


static void *zz_controller_event_loop(void *argc) {
    
    zz_controller *c = (zz_controller *)argc;
    zz_command *cmd = NULL;
    while(1) {
        cmd = zz_queue_pop(c->commandQueue);
        if (cmd!=NULL) { //处理命令
            if (cmd->type == ZZ_COMMAND_OPEN) {
                
                if (c->decodeCtx == NULL) {
                    c->decodeCtx = zz_decode_context_alloc(c->bufferSize);
                }
                char *path = (char *)cmd->data;
                int ret = zz_decode_context_open(c->decodeCtx, path);
                zz_decode_context_start(c->decodeCtx);
                c->statusCallback(c->opaque,ret);
            }else if (cmd->type == ZZ_COMMAND_DESTROY){
                break;
            }
        }else{
            
            usleep(10*1000);
            /*

            if (c->renderCallBack) {
                zz_video_frame *frame = zz_decode_context_get_video_buffer(c->decodeCtx);
                if (frame!= NULL) {
                    c->renderCallBack(c->opaque,frame);
                    int seconds = (1/frame->fps)*AV_TIME_BASE;
                    printf("seconds = %d \n",seconds);
                    usleep(seconds);
                    av_frame_free(&frame->frame);
                    free(frame);
                }
                
            }
             */
            
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
