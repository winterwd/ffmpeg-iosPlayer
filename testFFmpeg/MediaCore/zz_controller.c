//
//  zz_controller.c
//  testFFmpeg
//
//  Created by smart on 2017/5/23.
//  Copyright © 2017年 XiaoWoNiu2014. All rights reserved.
//

#include "zz_controller.h"
#include <string.h>
#include <stdlib.h>



static zz_command *zz_command_alloc(ZZ_COMMAND_TYPE type,void *data) {
    zz_command *cmd = (zz_command *)malloc(sizeof(zz_command));
    cmd->data = data;
    cmd->type = type;
    return cmd;
}


static void *zz_controller_event_loop(void *argc) {
    
    zz_controller *c = (zz_controller *)argc;
    zz_command *cmd = NULL;
    for (;;) {
        cmd = zz_queue_pop(c->commandQueue);
        if (cmd!=NULL) { //处理命令
            
        }else{ //将解码到的AVFrame放入到视频和音频队列
            
        }
    }
    
    return NULL;
}

static void *zz_controller_decode_loop(void *argc) {
    zz_controller *c = (zz_controller *)argc;
    for(;;){
        
    }
    return NULL;
}

zz_controller * zz_controller_alloc(int buffersize) {
    
    zz_controller *controller = malloc(sizeof(zz_controller));
    controller->bufferSize  = buffersize;
    controller->timeStamp = 0;
    zz_controller_init(controller);
    return controller;
    
}


void zz_controller_init(zz_controller *controller) {
    if (controller == NULL) {
        return;
    }
    controller->commandQueue = zz_queue_alloc(8, NULL);
    pthread_create(&controller->eventThreadId, NULL, zz_controller_event_loop, controller);
    pthread_create(&controller->decodeThreadId, NULL, zz_controller_decode_loop, controller);
    
}

void zz_controller_free(zz_controller *controller);

void zz_controller_destroy(zz_controller *controller);

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

void zz_controller_pause(zz_controller *controller);

void    zz_controller_resume(zz_controller *controller);

void    zz_controller_stop(zz_controller *controller);

void    zz_controller_set_volume(zz_controller *controller,float volume);

void    zz_controller_set_abort_request(zz_controller *controller,int abort);

void    zz_controller_set_area(zz_controller *controller,int x,int y,float width,float height);

double  zz_controller_get_duration(zz_controller *controller);

double  zz_controller_get_cur_time(zz_controller *controller);
