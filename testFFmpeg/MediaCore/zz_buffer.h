//
//  zz_buffer.h
//  testFFmpeg
//
//  Created by smart on 2017/5/23.
//  Copyright © 2017年 XiaoWoNiu2014. All rights reserved.
//

#ifndef zz_buffer_h
#define zz_buffer_h

#include <pthread.h>

typedef struct zz_buffer{
    
    uint8_t *buf;
    int     len;
    unsigned int capacity;
    pthread_mutex_t lock;
    
}zz_buffer;


zz_buffer * zz_buffer_alloc(unsigned int capacity);

int         zz_buffer_get(zz_buffer *zb, uint8_t *dst, int len);

int         zz_buffer_put(zz_buffer *zb, uint8_t *buf, int len);

int         zz_buffer_size(zz_buffer *zb);

void        zz_buffer_free(zz_buffer *zb);




#endif /* zz_buffer_h */
