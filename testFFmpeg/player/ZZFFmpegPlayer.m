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
@property (nonatomic,strong) ZZAudioPlayer *audioPlayer;

@end

@implementation ZZFFmpegPlayer
{

    ZZVideoPlayerView *playView;
    zz_controller      *playerController;
}


static void player_statuse(void *userData,zz_status *status){
    
    ZZFFmpegPlayer *player = (__bridge ZZFFmpegPlayer *)userData;
    if (status->statusCode == ZZ_PLAY_STATUS_OPEN) {
        printf("打开成功了 \n");

        dispatch_async(dispatch_get_main_queue(), ^{
            [player open];
            if (player.playStateCallBack) {
                
                player.playStateCallBack(kffmpegPrepareToPlay);
            }
            
        });
        
       
    }else if (status->statusCode == ZZ_PLAY_STATUS_PAUSED){
        
        [player.audioPlayer pause];
        
    }else if (status->statusCode == ZZ_PLAY_STATUS_RESUME){
        
        [player.audioPlayer play];
        
    }
    
    free(status);
    
}

- (instancetype)init
{
    if (self = [super init]) {
//        playerDecoder = ffmpeg_decoder_alloc_init();
        playerController = zz_controller_alloc(60, (__bridge void *)self,player_statuse);
        zz_controller_set_render_func(playerController, handleVideoCallback2);
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
            self.audioPlayer = [[ZZAudioPlayer alloc]initWithAudioSamplate:playerDecoder->samplerate numChannel:playerDecoder->nb_channel format:kAudioFormatFlagIsSignedInteger isInterleaved:YES];
            self.audioPlayer.delegate = self;
            
            
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
    
}

- (void)open{
    //        playerDecoder->decoded_video_data_callback = handleVideoCallback;
    self.audioPlayer = [[ZZAudioPlayer alloc]initWithAudioSamplate:playerController->audioInfo->samplerate numChannel:playerController->audioInfo->channels format:kAudioFormatFlagIsSignedInteger isInterleaved:YES];
    self.audioPlayer.delegate = self;
    
    
    
    ffmpeg_videooutput_init();
    CGFloat height = [UIScreen mainScreen].bounds.size.width*0.75;
    playView = [[ZZVideoPlayerView alloc]initWithFrame:CGRectMake(0, 200, [UIScreen mainScreen].bounds.size.width, height)];
    
    
    
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
    
    /*
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
    */
    
    
    uint32_t len = audioQueueBuffer->mAudioDataBytesCapacity;
    int buffer_index = 0;
    while (len>0) {
        zz_audio_frame *frame = zz_decode_context_get_audio_buffer(playerController->decodeCtx);
   
        memcpy(audioQueueBuffer->mAudioData+buffer_index, frame->data, frame->size);
        len -= frame->size;
        buffer_index += frame->size;
        free(frame->data);
        free(frame);
    }
    
    audioQueueBuffer->mAudioDataByteSize = audioQueueBuffer->mAudioDataBytesCapacity;
    AudioQueueEnqueueBuffer(audioQueue, audioQueueBuffer, 0, NULL);
    
}




- (void)play
{
//    ffmpeg_decoder_star(playerDecoder);
    [self.audioPlayer play];
}


- (double)duration
{
    return  zz_controller_get_duration(playerController);
//    playerDecoder->duration;
}

- (double)curTime{
    return zz_controller_get_cur_time(playerController);
//    playerDecoder->curTime;
}

- (void)setCurTime:(double)curTime
{
//    seek_to_time(playerDecoder, curTime);
    
    zz_controller_seek_to_time(playerController, curTime);
}


- (void)pause
{
    zz_controller_pause(playerController);
}

- (void)resume{
    
    zz_controller_resume(playerController);
}

- (void)stop
{
    zz_controller_stop(playerController);
}

- (void)destroy{

}

- (UIView *)playerView
{
    if (!playView) {
        
        CGFloat height = [UIScreen mainScreen].bounds.size.width*0.75;
        playView = [[ZZVideoPlayerView alloc]initWithFrame:CGRectMake(0, 200, [UIScreen mainScreen].bounds.size.width, height)];
    }
    return playView;
}
@end
