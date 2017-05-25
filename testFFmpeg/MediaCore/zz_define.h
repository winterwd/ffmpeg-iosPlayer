//
//  zz_define.h
//  testFFmpeg
//
//  Created by smart on 2017/5/24.
//  Copyright © 2017年 XiaoWoNiu2014. All rights reserved.
//

#ifndef zz_define_h
#define zz_define_h

#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
#include <pthread.h>
#include "zz_queue.h"

typedef struct zz_decode_frame_s {
    int type;
    uint64_t timebase;
    void *frame;
}zz_decode_frame;

typedef struct zz_audio_frame_s{
    int channel,samplerate,byte_per_second,fmt;
    uint8_t *data;
    uint64_t timebase;
}zz_audio_frame;


typedef struct zz_video_frame_s {
    
    int width,height;
    uint64_t timebase;
    uint8_t *data;
}zz_video_frame;

#endif /* zz_define_h */
