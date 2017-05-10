//
//  ffmpegPlayer.m
//  testFFmpeg
//
//  Created by zelong zou on 16/8/31.
//  Copyright © 2016年 XiaoWoNiu2014. All rights reserved.
//

#import "ffmpegPlayer.h"
#import "AudioPlayer.h"
#import "FXPlayerView.h"

extern void *ffmpeg_videooutput_init();
extern void ffmpeg_videooutput_render(AVFrame *frame);
@interface ffmpegPlayer () <AudioPlayerDelegate>

@end

@implementation ffmpegPlayer
{
    AudioPlayer *audioPlayer;
    FXPlayerView *playView;
}
- (instancetype)init
{
    if (self = [super init]) {
        playerDecoder = ffmpeg_decoder_alloc_init();
    }
    return self;
}


-(void)openFile:(NSString *)path
{
    dispatch_async(dispatch_get_global_queue(0, 0), ^{
        int ret =  ffmpeg_decoder_decode_file(playerDecoder, [path UTF8String]);
        
        if (ret>0) {
            audioPlayer = [[AudioPlayer alloc]initWithAudioSamplate:playerDecoder->samplerate numChannel:playerDecoder->nb_channel format:kAudioFormatFlagIsSignedInteger isInterleaved:YES];
            audioPlayer.delegate = self;

        }
        dispatch_async(dispatch_get_main_queue(), ^{
            ffmpeg_videooutput_init();
            CGFloat rate = playerDecoder->width/playerDecoder->height;
            CGFloat height = 320/rate;
            playView = [[FXPlayerView alloc]initWithFrame:CGRectMake(0, 200, 320, height)];
            if (self.playStateCallBack) {
                
                self.playStateCallBack(kffmpegPrepareToPlay);
            }
        });
    });
    
}


//audio
- (void)audioQueueOutputWithQueue:(AudioQueueRef)audioQueue queueBuffer:(AudioQueueBufferRef)audioQueueBuffer
{
    OSStatus status;
    int outsize = 0;
//    uint8_t *temp;
    ffmpeg_decode_audio_frame(playerDecoder, NULL, &outsize);

    memcpy(audioQueueBuffer->mAudioData, playerDecoder->audioOutBuffer, outsize);
    
    printf("out size is %d  \n",outsize);
    
    
    audioQueueBuffer->mAudioDataByteSize = outsize;
    status = AudioQueueEnqueueBuffer(audioQueue, audioQueueBuffer, 0, NULL);

    /*
    int ret = ffmpeg_decode_video_frame(playerDecoder, NULL);
    if (ret ==1 ) {
        ffmpeg_videooutput_render(playerDecoder->pFrameRGB);
        
    }
     */
}

- (void)play
{
    ffmpeg_decoder_start(playerDecoder);
    [audioPlayer play];

}
- (void)pause
{
    
}
- (void)stop
{
    
}
- (void)destroy
{
    
}

- (UIView *)playerView
{
    return playView;
}
@end
