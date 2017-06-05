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
#include "zz_define.h"
#include "zz_decoder.h"
typedef enum {
    ZZ_EVENT_UNKNOW,
    ZZ_EVENT_DESTROY,
    ZZ_EVENT_OPEN,
    ZZ_EVENT_CLOSE,
    ZZ_EVENT_PLAY,
    ZZ_EVENT_STOP,
    ZZ_EVENT_RECEIVE_VIDEO,
    ZZ_EVENT_VIDEO_RENDER
}ZZ_EVENT_TYPE;

typedef void (statusCallback)(void *userData,int status);
typedef void (videoOutRenderCallback)(void *userData,void *frame);
typedef struct zz_event_s {
    ZZ_EVENT_TYPE type;
    void *data;
}zz_event;




typedef  struct zz_controller_s {
    int status;
    int bufferSize;
    int abortRequest;
    char urlPath[1024];
    pthread_t eventThreadId;
    
    int64_t timeStamp;
    zz_queue *eventQueue;
    zz_decode_ctx *decodeCtx;
    void          *opaque;
    statusCallback  *statusCallback;
    videoOutRenderCallback *renderCallBack;
    zz_audio_frame  *audioInfo;
    zz_video_frame  *videoInfo;
    
    
}zz_controller;


zz_controller * zz_controller_alloc(int buffersize,void *userData,statusCallback *callback);

void    zz_controller_init(zz_controller *controller);

void    zz_controller_free(zz_controller *controller);

void    zz_controller_destroy(zz_controller *controller);

void    zz_controller_open(zz_controller *controller,const char *path);

void    zz_controller_close(zz_controller *controller);

void    zz_controller_play(zz_controller *controller);

void    zz_controller_pause(zz_controller *controller);

void    zz_controller_resume(zz_controller *controller);

void    zz_controller_stop(zz_controller *controller);

void    zz_controller_send_cmd(zz_controller *controller,ZZ_EVENT_TYPE type);

void    zz_controller_set_volume(zz_controller *controller,float volume);

void    zz_controller_set_abort_request(zz_controller *controller,int abort);

void    zz_controller_set_area(zz_controller *controller,int x,int y,float width,float height);

double  zz_controller_get_duration(zz_controller *controller);

double  zz_controller_get_cur_time(zz_controller *controller);

int     zz_controller_seek_to_time(zz_controller *controller,float time);


//set method
void zz_controller_set_render_func(zz_controller *controller,videoOutRenderCallback *rendCallBack);





#endif /* zz_controller_h */
