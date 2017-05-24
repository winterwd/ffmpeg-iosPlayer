//
//  zz_decoder.h
//  testFFmpeg
//
//  Created by smart on 2017/5/23.
//  Copyright © 2017年 XiaoWoNiu2014. All rights reserved.
//

#ifndef zz_decoder_h
#define zz_decoder_h

#include "zz_define.h"

typedef struct zz_decoder_s {
    
    zz_queue *buffer_queue;
    AVCodecContext  *codec_ctx;
    AVCodec         *codec;
    AVStream        *stream;
    SwrContext      *swr; //音频格式转换
    struct SwsContext *sws; //图像格式转换上下文
   
}zz_decoder;


typedef struct zz_decode_context_s{
    AVFormatContext *format_ctx;
    int         video_st_index,audio_st_index,subtitle_st_index;
    zz_decoder *video_decoder,*audio_decoder,*subtitle_decoder;
    AVPacket   packet;  ///<保存读取到的包
    AVFrame    *frame;  ///<保存单次解码后的frame
    zz_queue   *frame_queue; ///<保存解码后的frame数据
    int         buffer_size;
    int         decode_status;
    uint8_t     abort_req; ///<处理中断请求
    
    pthread_mutex_t decode_lock;
    pthread_cond_t  decode_cond;
}zz_decode_ctx;


zz_decode_ctx * zz_decode_context_alloc(int buffersize);
int zz_decode_context_open(zz_decode_ctx *decode_ctx,const char *path);
int zz_decode_context_read_packet(zz_decode_ctx *decode);


#endif /* zz_decoder_h */
