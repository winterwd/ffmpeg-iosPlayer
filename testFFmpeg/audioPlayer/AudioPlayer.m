//
//  AudioPlayer.m
//  testFFmpeg
//
//  Created by zelong zou on 16/8/26.
//  Copyright © 2016年 XiaoWoNiu2014. All rights reserved.
//

#import "AudioPlayer.h"

@implementation AudioPlayer

static UInt32 gBufferSizeBytes=0x10000;//It muse be pow(2,x)

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
    AudioPlayer* player=(__bridge AudioPlayer*)inUserData;
    [player audioQueueOutputWithQueue:inAQ queueBuffer:buffer];
}

//缓存数据读取方法的实现
-(void) audioQueueOutputWithQueue:(AudioQueueRef)audioQueue queueBuffer:(AudioQueueBufferRef)audioQueueBuffer{
    OSStatus status;
    
    
    int outsize = 0;
    uint8_t *temp;
    ffmpeg_decode_frame(playerDecoder, &temp, &outsize);

    memcpy(audioQueueBuffer->mAudioData, playerDecoder->audioOutBuffer, outsize);
    
    printf("out size is %d  \n",outsize);
    
//    fread(audioQueueBuffer->mAudioData, 1, 4096, infile);
    audioQueueBuffer->mAudioDataByteSize = outsize;
    status = AudioQueueEnqueueBuffer(audioQueue, audioQueueBuffer, 0, NULL);
    
}



-(id) initWithAudio:(NSString *)path{
    if (!(self=[super init])) return nil;

    //打开音频文件

//    infile = fopen([path UTF8String], "rb+");
    
    playerDecoder = ffmpeg_decoder_alloc_init();
    
    ffmpeg_decoder_decode_file(playerDecoder, [path UTF8String]);

    
    ffmpeg_decoder_start(playerDecoder);
    //取得音频数据格式
    
 
    [self setAudioDataFormatWithSmapleRate:playerDecoder->samplerate numChannel:playerDecoder->nb_channel format:kAudioFormatFlagIsSignedInteger isInterleaved:YES];
    

    //创建播放用的音频队列


    CheckError(AudioQueueNewOutput(&dataFormat, BufferCallback, (__bridge void * _Nullable)(self),
                                   nil, nil, 0, &queue), "AudioQueueNewOutput error");
    //创建并分配缓冲空间
    packetIndex=0;
    for (int i=0; i<NUM_BUFFERS; i++) {
        AudioQueueAllocateBuffer(queue, gBufferSizeBytes, &buffers[i]);
        BufferCallback((__bridge void *)(self), queue, buffers[i]);
    }
    
    Float32 gain=1.0;
    //设置音量
    AudioQueueSetParameter(queue, kAudioQueueParam_Volume, gain);
    
    return self;
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
//    ffmpeg_decoder_start(playerDecoder);
    CheckError(AudioQueueStart(queue, nil), "AudioQueueStart error");
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
