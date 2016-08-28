//
//  AudioPlayer.h
//  testFFmpeg
//
//  Created by zelong zou on 16/8/26.
//  Copyright © 2016年 XiaoWoNiu2014. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <AudioToolbox/AudioQueue.h>
#include "ffmpegDecoder.h"
#define NUM_BUFFERS 3
@interface AudioPlayer : NSObject
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
    
    ffmpegDecoder  *playerDecoder;
    
    
    
    
}

//定义队列为实例属性
@property AudioQueueRef queue;
//播放方法定义
-(id)initWithAudio:(NSString *) path;
- (void)play;
- (void)pause;
- (void)stop;

@end
