//
//  ffmpegPacketQueue.c
//  testFFmpeg
//
//  Created by zelong zou on 16/8/26.
//  Copyright © 2016年 XiaoWoNiu2014. All rights reserved.
//

#include "ffmpegPacketQueue.h"

int ffmpegPacketQueue_init(ffmpegPacketQueue *q)
{
    memset(q, 0, sizeof(ffmpegPacketQueue));
    pthread_mutex_init(&q->mute_lock, NULL);
    pthread_cond_init(&q->cond_lock, NULL);
    q->first = q->last = NULL;
    return 1;
}

int packet_queue_put(ffmpegPacketQueue *q, AVPacket *pkt)
{
    AVPacketList *pkt1;
    if(av_dup_packet(pkt) < 0)
    {
        return -1;
    }
    pkt1 = av_malloc(sizeof(AVPacketList));
    if (!pkt1)
        return -1;
    pkt1->pkt = *pkt;
    pkt1->next = NULL;
    
    pthread_mutex_lock(&q->mute_lock);
    
    while (q->nb_packets>50) {
        printf("queue has 50 items , wait..   \n");
        pthread_cond_wait(&q->cond_lock, &q->mute_lock);
        
        
    }
    
    if (!q->last)	//刚开始若队列q为空，则q->first_pkt=q->last_pkt
        q->first = pkt1;
    else	//插入队列，从尾部插入
        q->last->next = pkt1;
    q->last = pkt1;
    q->nb_packets++;
    q->size += pkt1->pkt.size;
  


    
    pthread_mutex_unlock(&q->mute_lock);
    return 0;
}

int quit = 0;

int packet_queue_get(ffmpegPacketQueue *q, AVPacket *pkt)
{
    AVPacketList *pkt1;
    int ret = 0;
    
    pthread_mutex_lock(&q->mute_lock);
    
    if (q->nb_packets == 0) {
        pthread_mutex_unlock(&q->mute_lock);
        return ret;
    }
    
    pkt1 = q->first;
    q->first = pkt1->next;
    if (!q->first)
        q->last = NULL;
    q->nb_packets--;
    q->size -= pkt1->pkt.size;
    *pkt = pkt1->pkt;
    av_free(pkt1);
    
    if (q->nb_packets==15) {
        pthread_cond_signal(&q->cond_lock);
        printf("queue has 5 items , start read...   \n");
    }
    ret = 1;
    pthread_mutex_unlock(&q->mute_lock);
    return ret;
}