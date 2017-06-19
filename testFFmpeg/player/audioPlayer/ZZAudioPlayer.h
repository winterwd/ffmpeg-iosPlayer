//
//  ZZAudioPlayer.h
//  testFFmpeg
//
//  Created by zelong zou on 16/8/26.
//  Copyright © 2016年 XiaoWoNiu2014. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <AudioToolbox/AudioQueue.h>
#define NUM_BUFFERS 3

@protocol AudioPlayerDelegate ;


@interface ZZAudioPlayer : NSObject
{

    //音频流描述对象
    AudioStreamBasicDescription dataFormat;
    //音频队列
    AudioQueueRef queue;
    SInt64 packetIndex;
    UInt32 numPacketsToRead;
    UInt32 bufferByteSize;
    AudioStreamPacketDescription *packetDescs;
    AudioQueueBufferRef buffers[NUM_BUFFERS];
    
    FILE        *infile;
    

    
    
    
    
}

//定义队列为实例属性
@property AudioQueueRef queue;
@property (nonatomic,assign) id<AudioPlayerDelegate> delegate;

- (id)initWithAudioFormat:(AudioStreamBasicDescription )format;
//播放方法定义
- (id)initWithAudioSamplate:(int )samplerate
                 numChannel:(int )channel
                     format:(AudioFormatFlags)format
              isInterleaved:(BOOL)isInterleaved;
- (void)audioPrepare;

- (void)play;
- (void)pause;
- (void)stop;

@end


@protocol AudioPlayerDelegate <NSObject>

-(void)audioQueueOutputWithQueue:(AudioQueueRef)audioQueue queueBuffer:(AudioQueueBufferRef)audioQueueBuffer;

@end
