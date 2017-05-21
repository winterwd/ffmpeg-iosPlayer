//
//  ViewController.m
//  testFFmpeg
//
//  Created by zelong zou on 14-3-13.
//  Copyright (c) 2014年 XiaoWoNiu2014. All rights reserved.
//

#import "ViewController.h"
#include "ffmpegDecoder.h"
#include "pthreadTest.h"
#import "ffmpegPlayer.h"
@interface ViewController ()
{
    ffmpegPlayer *player;
}
@property (strong, nonatomic) IBOutlet UILabel *durationLb;
@property (strong, nonatomic) IBOutlet UILabel *curLb;

@property (strong, nonatomic) IBOutlet UISlider *slierView;
@end

@implementation ViewController

- (void)viewDidLoad
{
    [super viewDidLoad];

    [self testFFmpeg];
    
    
    NSTimer *timer = [NSTimer scheduledTimerWithTimeInterval:0.1 repeats:YES block:^(NSTimer * _Nonnull timer) {
        self.curLb.text = [self timeFormatted:(int)player.curTime];
    }];
    
    [[NSRunLoop currentRunLoop]addTimer:timer forMode:NSRunLoopCommonModes];
    
    

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

- (NSString *)timeFormatted:(int)totalSeconds
{
    
    int seconds = totalSeconds % 60;
    int minutes = (totalSeconds / 60) % 60;
    int hours = totalSeconds / 3600;
    
    return [NSString stringWithFormat:@"%02d:%02d:%02d",hours, minutes, seconds];
}



- (IBAction)endEdit:(UISlider *)sender {
    self.curLb.text = [self timeFormatted:sender.value];
    player.curTime = sender.value;
}

- (IBAction)sliderChanged:(UISlider *)sender {
    self.curLb.text = [self timeFormatted:sender.value];
}


- (void)testFFmpeg
{
    
    NSString *infileName = @"/Users/xiaowoniu/Documents/一些素材/test.flv";
    ///Users/xiaowoniu/Downloads/13.mp4
    ///Users/smart/Desktop/未命名文件夹/output.mp4
//    NSString *infileName = @"/Users/smart/Documents/temp/test.flv";
//    NSString *infileName = @"/Users/smart/Desktop/未命名文件夹/output.mp4";
//    NSString *outFileName = @"/Users/smart/Documents/temp/test.pcm";

    
    player = [[ffmpegPlayer alloc]init];
    [player openFile:infileName];
    __weak ffmpegPlayer *weakPlayer = player;
    __weak ViewController *weakSelf = self;
    [player setPlayStateCallBack:^(FFmpegPlayerStatus status) {
       
        if (status == kffmpegPrepareToPlay) {
            [weakSelf.view addSubview:[weakPlayer playerView]];
            [weakPlayer play];
            
            weakSelf.slierView.maximumValue = weakPlayer.duration;
            weakSelf.slierView.minimumValue = 0.0f;
            
            weakSelf.durationLb.text = [weakSelf timeFormatted:weakPlayer.duration];
            
        }
        
    }];
    
    
}


- (void)testThread
{
    thread_main();
}
@end
