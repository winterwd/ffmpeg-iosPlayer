//
//  ZZFFmpegPlayer.m
//  testFFmpeg
//
//  Created by zelong zou on 16/8/31.
//  Copyright © 2016年 XiaoWoNiu2014. All rights reserved.
//

#import "ZZFFmpegPlayer.h"
#import "ZZAudioPlayer.h"
#import "ZZVideoPlayerView.h"
#import "zz_controller.h"

extern void *ffmpeg_videooutput_init();
extern void ffmpeg_videooutput_render(AVFrame *frame);
@interface ZZFFmpegPlayer () <AudioPlayerDelegate>

@end

@implementation ZZFFmpegPlayer
{
    ZZAudioPlayer *audioPlayer;
    ZZVideoPlayerView *playView;
    zz_controller      *playerController;
}


static void player_statuse(void *userData,int status){
    
    ZZFFmpegPlayer *player = (__bridge ZZFFmpegPlayer *)userData;
    if (status == 1) {
        printf("打开成功了 \n");
        [player play];
        
    }
    
}

- (instancetype)init
{
    if (self = [super init]) {
//        playerDecoder = ffmpeg_decoder_alloc_init();
        playerController = zz_controller_alloc(10, (__bridge void *)self,player_statuse);
    }
    return self;
}

int handleVideoCallback(AVFrame *frame,int datasize){
    ffmpeg_videooutput_render(frame);
    return 1;
}


void handleVideoCallback2(void *userData,void  *data){
    zz_video_frame *frame = (zz_video_frame *)data;
    ffmpeg_videooutput_render(frame->frame);
    
    
}

-(void)openFile:(NSString *)path
{
    [self openFile2:path];
    return;
    dispatch_async(dispatch_get_global_queue(0, 0), ^{
        

        int ret = ffmpeg_decoder_open_file(playerDecoder, [path UTF8String]);
        if (ret>=0) {
            playerDecoder->decoded_video_data_callback = handleVideoCallback;
            audioPlayer = [[ZZAudioPlayer alloc]initWithAudioSamplate:playerDecoder->samplerate numChannel:playerDecoder->nb_channel format:kAudioFormatFlagIsSignedInteger isInterleaved:YES];
            audioPlayer.delegate = self;
            
            
            ffmpeg_videooutput_init();
            dispatch_async(dispatch_get_main_queue(), ^{
                float   vheight = playerDecoder->height;
                CGFloat rate = playerDecoder->width/vheight;
                CGFloat height = [UIScreen mainScreen].bounds.size.width/rate;
                playView = [[ZZVideoPlayerView alloc]initWithFrame:CGRectMake(0, 200, [UIScreen mainScreen].bounds.size.width, height)];
                if (self.playStateCallBack) {
                    
                    self.playStateCallBack(kffmpegPrepareToPlay);
                }
                
            });
            
        }
        


    });
    

    
}

-(void)openFile2:(NSString *)path
{
    
    zz_controller_open(playerController, [path UTF8String]);
    
    //        playerDecoder->decoded_video_data_callback = handleVideoCallback;
    audioPlayer = [[ZZAudioPlayer alloc]initWithAudioSamplate:44100 numChannel:2 format:kAudioFormatFlagIsSignedInteger isInterleaved:YES];
    audioPlayer.delegate = self;
    ffmpeg_videooutput_init();
    zz_controller_set_render_func(playerController, handleVideoCallback2);

    dispatch_async(dispatch_get_main_queue(), ^{
//        float   vheight = playerController->decodeCtx->video_decoder->codec_ctx->height;
        CGFloat rate = 1.333;
        CGFloat height = [UIScreen mainScreen].bounds.size.width/rate;
        playView = [[ZZVideoPlayerView alloc]initWithFrame:CGRectMake(0, 200, [UIScreen mainScreen].bounds.size.width, height)];
        
        
    });
    
    
    
}


//audio
- (void)audioQueueOutputWithQueue:(AudioQueueRef)audioQueue queueBuffer:(AudioQueueBufferRef)audioQueueBuffer
{
/*
    OSStatus status;
    int outsize = 0;
    
    static uint8_t audioBuffer[0x100000];
    
    uint32_t len = audioQueueBuffer->mAudioDataBytesCapacity;

    static unsigned int buffer_index = 0;
    buffer_index = 0;
    while (len>0) {
        ffmpeg_decode_audio_frame(playerDecoder, audioBuffer, &outsize);
        memcpy(audioQueueBuffer->mAudioData+buffer_index, audioBuffer, outsize);
        len-= outsize;
        buffer_index+= outsize;
        
    }
    audioQueueBuffer->mAudioDataByteSize = audioQueueBuffer->mAudioDataBytesCapacity;
    status = AudioQueueEnqueueBuffer(audioQueue, audioQueueBuffer, 0, NULL);
*/
    
    OSStatus status;
//    int outsize = 0;
    
    //    static uint8_t audioBuffer[0x100000];
    
    
//    uint32_t len = audioQueueBuffer->mAudioDataBytesCapacity;
    
//    static unsigned int buffer_index = 0;
//    buffer_index = 0;
//    zz_audio_frame *frame;
//    while (len>0) {
        //        ffmpeg_decode_audio_frame(playerDecoder, audioBuffer, &outsize);
        
    zz_audio_frame *frame = zz_decode_context_get_audio_buffer(playerController->decodeCtx);
    
    if (frame && frame->data != NULL) {
        memcpy(audioQueueBuffer->mAudioData, frame->data, frame->size);
        free(frame->data);
        free(frame);
    }
        

//    }
    audioQueueBuffer->mAudioDataByteSize = audioQueueBuffer->mAudioDataBytesCapacity;
    status = AudioQueueEnqueueBuffer(audioQueue, audioQueueBuffer, 0, NULL);
}




- (void)play
{
//    ffmpeg_decoder_star(playerDecoder);
    [audioPlayer play];
}


- (double)duration
{
    return  0.0;
//    playerDecoder->duration;
}

- (double)curTime{
    return  0.0;
//    playerDecoder->curTime;
}

- (void)setCurTime:(double)curTime
{
    seek_to_time(playerDecoder, curTime);
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
