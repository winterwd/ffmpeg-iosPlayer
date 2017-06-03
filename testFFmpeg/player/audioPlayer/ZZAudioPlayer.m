//
//  ZZAudioPlayer.m
//  testFFmpeg
//
//  Created by zelong zou on 16/8/26.
//  Copyright © 2016年 XiaoWoNiu2014. All rights reserved.
//

#import "ZZAudioPlayer.h"

@implementation ZZAudioPlayer

static UInt32 gBufferSizeBytes=8192;//It muse be pow(2,x)

@synthesize queue;

static void CheckError(OSStatus error, const char *operation)
{
    if (error == noErr) return;
    
    char str[20];
    // see if it appears to be a 4-char-code
    *(UInt32 *)(str + 1) = CFSwapInt32HostToBig(error);
    if (isprint(str[1]) && isprint(str[2]) && isprint(str[3]) && isprint(str[4])) {
        str[0] = str[5] = '\'';
        str[6] = '\0';
    } else
        // no, format it as an integer
        sprintf(str, "%d", (int)error);
    
    fprintf(stderr, "Error: %s (%s)\n", operation, str);
    
}

//回调函数(Callback)的实现
static void BufferCallback(void *inUserData,AudioQueueRef inAQ,
                           AudioQueueBufferRef buffer){
    ZZAudioPlayer* player=(__bridge ZZAudioPlayer*)inUserData;
    
    
//    static AudioTimeStamp timestamp;
//    AudioQueueGetCurrentTime(inAQ, NULL, &timestamp, NULL);
//    
//    NSTimeInterval playedTime = timestamp.mSampleTime / player->dataFormat.mSampleRate;
//    
//    printf("===sameple time : %f    playedTime: %f \n",timestamp.mSampleTime,playedTime);
    
//    printf("audio queue buffersize : %d",buffer->mAudioDataByteSize);
    [player audioQueueOutputWithQueue:inAQ queueBuffer:buffer];
}

//缓存数据读取方法的实现
-(void) audioQueueOutputWithQueue:(AudioQueueRef)audioQueue queueBuffer:(AudioQueueBufferRef)audioQueueBuffer{
    
    if (self.delegate && [self.delegate respondsToSelector:@selector(audioQueueOutputWithQueue:queueBuffer:)]) {
        [self.delegate audioQueueOutputWithQueue:audioQueue queueBuffer:audioQueueBuffer];
    }
    
}


- (id)initWithAudioSamplate:(int )samplerate
                 numChannel:(int )channel
                     format:(AudioFormatFlags)format
              isInterleaved:(BOOL)isInterleaved
{
    if (!(self=[super init])) return nil;

    [self setAudioDataFormatWithSmapleRate:samplerate numChannel:channel format:format isInterleaved:isInterleaved];
    
    printf("setup audio fromat samplerate : %d  chnnel: %d  \n ",samplerate,channel);
    //创建播放用的音频队列
    
    
    CheckError(AudioQueueNewOutput(&dataFormat, BufferCallback, (__bridge void * _Nullable)(self),
                                   nil, nil, 0, &queue), "AudioQueueNewOutput error");
    [self audioPrepare];

    Float32 gain=1.0;
    //设置音量
    AudioQueueSetParameter(queue, kAudioQueueParam_Volume, gain);
    
    return self;
}

- (void)audioPrepare
{
    //创建并分配缓冲空间
    packetIndex=0;
    for (int i=0; i<NUM_BUFFERS; i++) {
        AudioQueueAllocateBuffer(queue, gBufferSizeBytes, &buffers[i]);
        memset(buffers[i]->mAudioData, 0, gBufferSizeBytes);
        buffers[i]->mAudioDataByteSize = gBufferSizeBytes;
        AudioQueueEnqueueBuffer(queue, buffers[i], 0, NULL);
        
    }
    
}

- (void)setAudioDataFormatWithSmapleRate:(double)samplerate
                              numChannel:(int)channels
                                  format:(AudioFormatFlags)fmt
                           isInterleaved:(bool)isInterleaved
{
    unsigned wordsize;
    dataFormat.mSampleRate = samplerate;
    dataFormat.mFormatID = kAudioFormatLinearPCM;
    dataFormat.mFormatFlags = kAudioFormatFlagsNativeEndian | kAudioFormatFlagIsPacked | fmt;
    dataFormat.mChannelsPerFrame = channels;
    dataFormat.mFramesPerPacket = 1;
    
    dataFormat.mReserved = 0;
    
    if (fmt == kAudioFormatFlagIsFloat) {
        wordsize = 4;
    }
    if (fmt == kAudioFormatFlagIsSignedInteger) {
        wordsize = 2;
    }
    
    dataFormat.mBitsPerChannel = 8*wordsize;
    
    if (isInterleaved) {
        dataFormat.mBytesPerFrame=dataFormat.mBytesPerPacket = wordsize*dataFormat.mChannelsPerFrame;
    }else{
        dataFormat.mFormatFlags |= kAudioFormatFlagIsNonInterleaved;
        dataFormat.mBytesPerFrame=dataFormat.mBytesPerPacket = wordsize;
    }
    
    //0.5 seconds data
    bufferByteSize = samplerate * channels * wordsize *0.5;
    
    
}


- (void)play
{
    //队列处理开始，此后系统开始自动调用回调(Callback)函数
    AudioQueueFlush(queue);
    CheckError(AudioQueueStart(queue, nil), "AudioQueueStart error");
//    AudioQueueReset(queue);
    
}
- (void)pause
{
    AudioQueuePause(queue);
    
    
}
- (void)stop
{
    AudioQueueFlush(queue);
    AudioQueueStop(queue, YES);
    
}


@end
