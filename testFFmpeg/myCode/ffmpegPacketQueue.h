//
//  ffmpegPacketQueue.h
//  testFFmpeg
//
//  Created by zelong zou on 16/8/26.
//  Copyright © 2016年 XiaoWoNiu2014. All rights reserved.
//

#ifndef ffmpegPacketQueue_h
#define ffmpegPacketQueue_h

#include <stdio.h>
#include <libavformat/avformat.h>
#include <pthread.h>
typedef struct
{
    AVPacketList *first,*last;
    int nb_packets;
    int size;
    pthread_mutex_t mute_lock;
    pthread_cond_t  cond_lock;
    
}ffmpegPacketQueue;

int ffmpegPacketQueue_init(ffmpegPacketQueue *q);
int packet_queue_put(ffmpegPacketQueue *q, AVPacket *pkt);
int packet_queue_get(ffmpegPacketQueue *q, AVPacket *pkt);
int packet_queue_block_get(ffmpegPacketQueue *q,AVPacket *pkt,int block);
void packet_quque_flush(ffmpegPacketQueue *q);
#endif /* ffmpegPacketQueue_h */
