//
//  zz_queue.c
//  testFFmpeg
//
//  Created by smart on 2017/5/23.
//  Copyright © 2017年 XiaoWoNiu2014. All rights reserved.
//

#include "zz_queue.h"
#include <string.h>
#include <stdlib.h>
#include <pthread.h>

static void zz_queue_default_free_callback(void *data){
    free(data);
}

static zz_node * zz_node_alloc(void *data) {
    zz_node *node = (zz_node *)malloc(sizeof(zz_node));
    node->pre = NULL;
    node->next = NULL;
    node->data = data;
    return node;
}

static void zz_node_free(zz_node *node){
    free(node);
}

static void zz_queue_node_free(zz_node *node,zz_node_free_callback *callback) {
    
    if (callback) {
        callback(node->data);
    }
    
    zz_node *next = node->next;
    zz_node_free(node);
    if (next!=NULL) {
        zz_queue_node_free(next, callback);
    }
    
}


zz_queue * zz_queue_alloc(int capacity,zz_node_free_callback *callback) {
    
    zz_queue *quque = malloc(sizeof(zz_queue));
    memset(quque,0,sizeof(zz_queue));
    quque->capacity = capacity;
    quque->first = NULL;
    quque->last = NULL;
    quque->size = 0;
    quque->callbackFunc = callback != NULL ? callback : zz_queue_default_free_callback;
    pthread_mutex_init(&quque->lock, NULL);
    return quque;
}

void zz_queue_put(zz_queue *queue,void *data){
    
    zz_node *node = zz_node_alloc(data);
    
    pthread_mutex_lock(&queue->lock);
    if (queue->last == NULL || queue->last == queue->first) {
        if (queue->first == NULL) {
            queue->first = node;
        }
        
        queue->last = node;
        queue->first->next = queue->last;
        queue->first->pre= queue->last;
        queue->last->next = queue->first;
        queue->last->pre = queue->first;
    } else {
        queue->last->next = node;
        node->pre = queue->last;
        node->next = queue->first;
        queue->last = node;
    }
    queue->size += 1;
    
    if (queue->capacity == -1) {
        pthread_mutex_unlock(&queue->lock);
        return;
    }
    
    if (queue->size > queue->capacity) {
        pthread_mutex_unlock(&queue->lock);
        queue->callbackFunc(zz_queue_pop(queue));
    }
    return;
}

void * zz_queue_pop(zz_queue *queue) {
    if (queue == NULL) {
        return NULL;
    }
    
    zz_node *node;
    void *data = NULL;
    
    pthread_mutex_lock(&queue->lock);
    switch(queue->size) {
        case 0:
            goto exit;
            break;
        case 1:
            node = queue->first;
            queue->first = NULL;
            queue->last = NULL;
            break;
        default:
            node = queue->first;
            queue->first = node->next;
            queue->last->next = queue->first;
            break;
    }
    data = node->data;
    zz_node_free(node);
    queue->size -= 1;
exit:
    pthread_mutex_unlock(&queue->lock);
    return data;
}

void zz_quque_free(zz_queue *queue) {
    
    if (queue == NULL) {
        return;
    }
    
    pthread_mutex_lock(&queue->lock);
    if (queue->size>0) { //删除所有的结点
        queue->last->next = NULL;
        zz_queue_node_free(queue->first, queue->callbackFunc);
    }
    pthread_mutex_unlock(&queue->lock);
    pthread_mutex_destroy(&queue->lock);
    free(queue);
    queue = NULL;
    
}


