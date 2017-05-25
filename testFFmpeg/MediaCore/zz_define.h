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
#include <unistd.h>
#include <assert.h>


typedef struct zz_config_s {

}zz_config;

typedef struct zz_decode_frame_s {
    int type;
    uint64_t timebase;
    void *frame;
}zz_decode_frame;

typedef struct zz_audio_frame_s{
    int channels,samplerate,byte_per_second,nbsamples,size;
    enum AVSampleFormat format;
    uint8_t *data;
//    uint8_t **extentData;
    uint64_t timebase;
}zz_audio_frame;


typedef struct zz_video_frame_s {
    int width,height;
    int keyframe;
    uint64_t timebase;
    uint8_t *data;
    int size;
    float fps;
    AVFrame *frame;
}zz_video_frame;

#endif /* zz_define_h */
