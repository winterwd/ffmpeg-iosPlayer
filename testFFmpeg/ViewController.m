//
//  ViewController.m
//  testFFmpeg
//
//  Created by zelong zou on 14-3-13.
//  Copyright (c) 2014年 XiaoWoNiu2014. All rights reserved.
//

#import "ViewController.h"
#include "ffmpegDecoder.h"
#import "AudioPlayer.h"
#include "pthreadTest.h"
#import "FXPlayerView.h"
@interface ViewController ()
{
    AudioPlayer *player;
    FXPlayerView *playerView;
}

@end

@implementation ViewController

- (void)viewDidLoad
{
    [super viewDidLoad];
	
    
    printf("111\n");
//    [self testThread];
    [self testAudioQueuePlay];
//    playerView = [[FXPlayerView alloc]initWithFrame:CGRectMake(0, 200, 320, 480)];
//    [self.view addSubview:playerView];
//    [self testFFmpeg];
    

}
- (IBAction)audioQueuePlay:(UIButton *)sender {
    [player play];
}
- (IBAction)audioQueuePause:(UIButton *)sender {
    [player pause];
}
- (IBAction)audioQueueStop:(id)sender {
    [player stop];
}

- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

static FILE *outFile;

static inline int decodeAudioCallBack(uint8_t *outbuf,int bufsize)
{
    
    fwrite(outbuf, 1, bufsize, outFile);
    return 1;
}


- (void)testAudioQueuePlay
{
      player = [[AudioPlayer alloc]initWithAudio:@"/Users/xiaowoniu/Documents/一些素材/test.flv"];
}


- (void)testFFmpeg
{
    
    NSString *infileName = @"/Users/xiaowoniu/Documents/一些素材/test.flv";
    NSString *outFileName = @"/Users/xiaowoniu/Documents/一些素材/test.pcm";
    
    ffmpegDecoder *decoder = ffmpeg_decoder_alloc_init();
    decoder->decoded_audio_data_callback = decodeAudioCallBack;
    ffmpeg_decoder_decode_file(decoder,[infileName UTF8String]);
    outFile = fopen([outFileName UTF8String], "wb+");
    ffmpeg_decoder_start(decoder);
    
    
}


- (void)testThread
{
    thread_main();
}
@end
