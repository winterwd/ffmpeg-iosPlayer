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



typedef struct {
    AVFormatContext *pFormatCtx;
    AVCodec         *pVcodec;
    AVCodec         *pAcodec;
    AVCodecContext  *pVcodectx;
    AVCodecContext  *pAcodectx;
    unsigned   short videoStreamIndex;
    unsigned   short auidoStreamIndex;
    int (*decoded_audio_data_callback)(uint8_t *pcmData,int dataSize);
    int (*decoded_video_data_callback)(uint8_t *yuvData,int dataSize);
    
    
    //audio
    SwrContext *swr;
    int    audio_buffer_size;
    uint8_t *audioOutBuffer;
    
    int    samplerate;
    int    nb_channel;
    
    //control
    int    stop;
    
//    pthread_mutex_t decodeAudioSize_mute;
//    pthread_cond_t  decodeAudioSize_cond;
    
    ffmpegPacketQueue audioPakcetQueue;
    
}ffmpegDecoder;

ffmpegDecoder *ffmpeg_decoder_alloc_init();
int           ffmpeg_decoder_decode_file(ffmpegDecoder *decoder,const char *path);
int           ffmpeg_decoder_start(ffmpegDecoder *decoder);
int           ffmpeg_decoder_stop(ffmpegDecoder *decoder);
int   ffmpeg_decode_frame(ffmpegDecoder *decoder, uint8_t **outPcmData,int *outDatasize);



#endif /* ffmpegDecoder_h */
