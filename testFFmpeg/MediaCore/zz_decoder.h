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

typedef struct zz_decoder_s zz_decoder;


typedef int (zz_decode_func)(AVCodecContext *avctx, AVFrame *frame,int *gotframe, const AVPacket *avpkt);

typedef void *(zz_convert_frame_func)(zz_decoder *decoder,AVFrame *srcFrame);

typedef void (zz_video_render_callback)(void *opaque,void *frame);

typedef struct zz_decoder_s {
    
    zz_queue        *buffer_queue;
    AVCodecContext  *codec_ctx;
    AVCodec         *codec;
    AVStream        *stream;
    SwrContext      *swr; //音频格式转换
    struct SwsContext *sws; //图像格式转换上下文
    
    zz_decode_func          *decode_func;
    zz_convert_frame_func   *convert_func;
     AVFrame    *frame;  ///<保存单次解码后的frame
    
    
}zz_decoder;


typedef struct zz_decode_context_s{
    AVFormatContext *format_ctx;
    int         video_st_index,audio_st_index,subtitle_st_index;
    zz_decoder *video_decoder,*audio_decoder,*subtitle_decoder;
    
    

    int         audioBytePerSecond;
    
    int         audioFrameBufferSize;
    


    
    int         buffer_size;
    int         decode_status;
    uint8_t     abort_req; ///<处理中断请求
    pthread_t   decodeThreadId;  //解码线程
    pthread_t   videoThreadId;    //视频线程
    pthread_mutex_t decode_lock;
    pthread_cond_t  decode_cond;
    
    
    void *opaque;
    
    zz_video_render_callback *videoCallBack;
    
    zz_queue     *audio_queue;
    zz_queue     *video_queue;
    
    
    float       fps;
    float       video_timebase;
    float       audio_timebase;
    
    int64_t     start_time;           ///< ms
    float       duration;            ///< seconds
    float       first_frame_time;     /// ms
    float       current_frame_time;  ///< ms
    float       current_video_time; ///< seconds
    float       current_audio_time; ///< seconds
    
    int64_t     seek_pos; ///<跳转位置
    uint8_t     seek_flag; ///<跳转方向
    uint8_t     seek_req; ///<是否跳转,0非跳转，1跳转
    
    uint8_t     paused; ///< 是否暂停播放
    

    
    
}zz_decode_ctx;





zz_decode_ctx * zz_decode_context_alloc(int buffersize);
int zz_decode_context_open(zz_decode_ctx *decode_ctx,const char *path);
void zz_decode_context_start(zz_decode_ctx *decode_ctx);
int zz_decode_context_read_packet(zz_decode_ctx *decode);

float zz_decode_context_get_current_time(zz_decode_ctx *decode_ctx);
int   zz_decode_context_seek_to_time(zz_decode_ctx *decode_ctx,float time);
void * zz_decode_context_get_audio_buffer(zz_decode_ctx *decode_ctx);
void * zz_decode_context_get_video_buffer(zz_decode_ctx *decode_ctx);
void zz_decode_context_paused(zz_decode_ctx *decode_ctx);
void zz_decode_context_resume(zz_decode_ctx *decode_ctx);
#endif /* zz_decoder_h */
