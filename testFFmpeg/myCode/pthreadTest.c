//
//  pthreadTest.c
//  testFFmpeg
//
//  Created by zelong zou on 16/8/28.
//  Copyright © 2016年 XiaoWoNiu2014. All rights reserved.
//

#include "pthreadTest.h"
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

pthread_t thread1;
pthread_t thread2;

pthread_mutex_t mutex;
pthread_cond_t  cond;

int counter = 0;
void * test_thread1(void *a)
{
    counter++;
    printf("thread 1 process : counter = :%d \n",counter);
    
//    if (counter>150) {
//        printf("thread 1 done! \n");
//        
//    }
    pthread_exit(0);
    return a;
}

void * test_thread2(void *a)
{
    counter++;
    printf("thread 2 process : counter = :%d  \n",counter);
    pthread_exit(0);
    return a;
}


void * increment_count(void *argc)
{
    while (1) {
        pthread_mutex_lock(&mutex);
        counter ++;
        printf("increment_count : counter = :%d  \n",counter);
        if (counter == 10) {

            pthread_cond_signal(&cond);
            printf("pthread_cond_signal  wake up !\n");
        }
        pthread_mutex_unlock(&mutex);
        
        
        sleep(1);
        
    }

   
    return NULL;
}

void * decrement_count(void *argc)
{
    printf("decrement_count thread start .....\n");
    while (1) {
        
        
        printf("waiting ....\n");
        
        pthread_mutex_lock(&mutex);
        
        while (counter<10) {

            pthread_cond_wait(&cond, &mutex);
            printf("pthread_cond_wait  i am waked up !!!\n");
        }
        

        
        counter=0;
        printf("decrement_count : counter = :%d  \n",counter);
        pthread_mutex_unlock(&mutex);
    }
    printf("decrement_count thread end .....\n");
    
    return NULL;
}


int thread_main()
{
    
    int test = 100;
    pthread_create(&thread1, NULL, increment_count, &test);
    pthread_create(&thread2, NULL, decrement_count, &test);

    
    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&cond, 0);
    
    
    
    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);
    
    printf("\n thread_main  done \n");
    return 1;
}