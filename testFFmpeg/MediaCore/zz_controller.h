//
//  zz_controller.h
//  testFFmpeg
//
//  Created by smart on 2017/5/23.
//  Copyright © 2017年 XiaoWoNiu2014. All rights reserved.
//

#ifndef zz_controller_h
#define zz_controller_h

#include <stdio.h>
#include "zz_queue.h"
#include <pthread.h>

typedef enum {
    ZZ_COMMAND_DESTROY,
    ZZ_COMMAND_OPEN,
    ZZ_COMMAND_CLOSE,
    ZZ_COMMAND_PLAY,
    ZZ_COMMAND_STOP,
    ZZ_COMMAND_RECEIVE_VIDEO,
}ZZ_COMMAND_TYPE;


typedef struct zz_command_s {
    ZZ_COMMAND_TYPE type;
    void *data;
}zz_command;

typedef  struct zz_controller_s {
    int status;
    int bufferSize;
    int abortRequest;
    pthread_t eventThreadId;
    
    int64_t timeStamp;
    zz_queue *commandQueue;
}zz_controller;


zz_controller * zz_controller_alloc(int buffersize);

void    zz_controller_init(zz_controller *controller);

void    zz_controller_free(zz_controller *controller);

void    zz_controller_destroy(zz_controller *controller);

void    zz_controller_open(zz_controller *controller,const char *path);

void    zz_controller_close(zz_controller *controller);

void    zz_controller_play(zz_controller *controller);

void    zz_controller_pause(zz_controller *controller);

void    zz_controller_resume(zz_controller *controller);

void    zz_controller_stop(zz_controller *controller);

void    zz_controller_set_volume(zz_controller *controller,float volume);

void    zz_controller_set_abort_request(zz_controller *controller,int abort);

void    zz_controller_set_area(zz_controller *controller,int x,int y,float width,float height);

double  zz_controller_get_duration(zz_controller *controller);

double  zz_controller_get_cur_time(zz_controller *controller);







#endif /* zz_controller_h */
