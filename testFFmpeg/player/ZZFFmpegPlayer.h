//
//  ZZFFmpegPlayer.h
//  testFFmpeg
//
//  Created by zelong zou on 16/8/31.
//  Copyright © 2016年 XiaoWoNiu2014. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "ffmpegDecoder.h"

typedef enum : NSUInteger {
    kffmpegPlayerUnKnow,
    kffmpegOpenFileError,
    kffmpegPrepareToPlay,
} FFmpegPlayerStatus;

@interface ZZFFmpegPlayer : NSObject
{
    ffmpegDecoder *playerDecoder;
}
@property (nonatomic,copy) void(^playStateCallBack)(FFmpegPlayerStatus status);
-(void)openFile:(NSString *)path;
- (void)play;
- (void)pause;
- (void)stop;
- (void)destroy;
- (UIView *)playerView;

@property (nonatomic,assign) double duration;
@property (nonatomic,assign) double curTime;
@end
