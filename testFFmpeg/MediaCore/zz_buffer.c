//
//  zz_buffer.c
//  testFFmpeg
//
//  Created by smart on 2017/5/23.
//  Copyright © 2017年 XiaoWoNiu2014. All rights reserved.
//

#include "zz_buffer.h"
#include <stdlib.h>
#include <string.h>


zz_buffer * zz_buffer_alloc(unsigned int capacity){
    zz_buffer *zb = (zz_buffer *)malloc(sizeof(zz_buffer));
    memset(zb,0,sizeof(zz_buffer));
    zb->capacity = capacity;
    zb->buf = malloc(sizeof(uint8_t)*capacity);\
    zb->len = 0;
    pthread_mutex_init(&zb->lock, NULL);
    return zb;
}


int zz_buffer_get(zz_buffer *zb, uint8_t *dst, int len){
    
    pthread_mutex_lock(&zb->lock);
    if (zb->len<len) {
        len = zb->len;
    }
    memcpy(dst,zb->buf,len);
    memmove(zb->buf, zb->buf+len, zb->len);
    zb->len -= len;
    pthread_mutex_unlock(&zb->lock);
    
    return len;
}

int zz_buffer_put(zz_buffer *zb, uint8_t *buf, int len){
    pthread_mutex_lock(&zb->lock);
    if (len+zb->len >  zb->capacity) {
        //overflow realloc
        zb->buf = realloc(zb->buf, (zb->len+len)*2);
        zb->capacity = (zb->len+len)*2;
    }
    memcpy(zb->buf+zb->len, buf, len);
    zb->len += len;
    pthread_mutex_unlock(&zb->lock);
    return len;
}

int zz_buffer_size(zz_buffer *zb){
    pthread_mutex_lock(&zb->lock);
    int len = zb->len;
    pthread_mutex_unlock(&zb->lock);
    return len;
}


void zz_buffer_free(zz_buffer *zb){
    
    pthread_mutex_destroy(&zb->lock);
    free(zb->buf);
    free(zb);
}
