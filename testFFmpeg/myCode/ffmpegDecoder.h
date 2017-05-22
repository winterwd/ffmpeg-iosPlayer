//
//  ffmpegDecoder.h
//  testFFmpeg
//
//  Created by zelong zou on 16/8/25.
//  Copyright © 2016年 XiaoWoNiu2014. All rights reserved.
//

#ifndef ffmpegDecoder_h
#define ffmpegDecoder_h

#include <stdio.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
#include "ffmpegPacketQueue.h"


#define VIDEO_QUEUE_SIZE 1

typedef struct {
    AVFormatContext *pFormatCtx;
    AVCodec         *pVcodec;
    AVCodec         *pAcodec;
    AVCodecContext  *pVcodectx;
    AVCodecContext  *pAcodectx;
    AVStream        *pvideo_st;
    AVStream        *paudio_st;
    int8_t videoStreamIndex;
    int8_t auidoStreamIndex;
    int (*decoded_audio_data_callback)(uint8_t *pcmData,int dataSize);
    int (*decoded_video_data_callback)(AVFrame *yuvData,int dataSize);
    
    pthread_t       decodeThread;
    pthread_t       videoThread;
    
    pthread_mutex_t pic_mutex;
    pthread_cond_t  pic_cond;
    
    //audio
    SwrContext *swr;
    int    audio_buffer_size;
    uint8_t *audio_buffer;
    int    samplerate;
    int    nb_channel;
    int    byte_per_seconds;
    double audioDelay; ///< audio queue 初始buffer 的时间延迟
    
    //video
    struct SwsContext *sws;
    int    video_buffer_size;
    uint8_t *videoOutBuffer;
    int    width;
    int    height;
    
    AVFrame *vFrameQueue[VIDEO_QUEUE_SIZE];
    int    vframe_queue_size;
    int    vframe_queue_windex;
    int    vframe_queue_rindex;
    
    double video_clock;
    
    double audio_clock;
    
    
    AVFrame *pFrameRGB;
    AVFrame *pVideoFrame;

    //control
    int    stop;
    short   quit;
    
    char   fileName[1024];
    
    
    ffmpegPacketQueue audioPakcetQueue;
    ffmpegPacketQueue videoPacketQueue;
    
    int8_t  seek_req;
    int64_t seek_pos;
    int8_t  seek_flag;
    
    double  duration;
    double  curTime;
    
}ffmpegDecoder;

ffmpegDecoder   *ffmpeg_decoder_alloc_init();
int             ffmpeg_decoder_open_file(ffmpegDecoder *decoder,const char *path);
int             ffmpeg_decoder_star(ffmpegDecoder *decoder);
void            *ffmpeg_decoder_decode_file(void *userData);

int             ffmpeg_decoder_stop(ffmpegDecoder *decoder);
int             ffmpeg_decode_audio_frame(ffmpegDecoder *decoder, uint8_t *outPcmData,int *outDatasize);
int             ffmpeg_decode_video_frame(ffmpegDecoder *decoder,AVFrame *frame);
AVFrame * quque_picture_get_frame(ffmpegDecoder *decoder);

int         quque_picture(ffmpegDecoder *decoder,AVFrame *frame);

void        seek_to_time(ffmpegDecoder *decoder, double time);
#endif /* ffmpegDecoder_h */
