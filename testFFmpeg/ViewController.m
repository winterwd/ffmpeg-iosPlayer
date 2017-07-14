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
#import "ZZFFmpegPlayer.h"

#define SCREEN_WIDT [UIScreen mainScreen].bounds.size.width
#define SCREEN_HEIGHT [UIScreen mainScreen].bounds.size.height
@interface ViewController ()
{
    ZZFFmpegPlayer *player;
    BOOL          isTouched;
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
    
    isTouched = NO;
    
    NSTimer *timer = [NSTimer scheduledTimerWithTimeInterval:0.1 repeats:YES block:^(NSTimer * _Nonnull timer) {
        if (!isTouched) {
            self.curLb.text = [self timeFormatted:(int)player.curTime];
            self.slierView.value = player.curTime;
        }
        
    }];
    
    [[NSRunLoop currentRunLoop]addTimer:timer forMode:NSRunLoopCommonModes];
    
    
    

}
- (IBAction)audioQueuePlay:(UIButton *)sender {
    [player resume];
}
- (IBAction)audioQueuePause:(UIButton *)sender {
    [player pause];
}
- (IBAction)audioQueueStop:(UIButton *)sender {
//    [player stop];
    
    UIView *view =[player playerView];
     sender.selected = !sender.selected;
    if (sender.selected) {
        view.transform = CGAffineTransformMakeRotation(M_PI_2);
        view.frame = CGRectMake(0,0 , SCREEN_WIDT, SCREEN_HEIGHT*0.75);
//        
//        ;
        
    }else{
        view.transform = CGAffineTransformIdentity;
        view.frame = CGRectMake(0, 200, SCREEN_WIDT, SCREEN_WIDT/1.33);
        
    }
   
    

    
    
    
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
    isTouched = NO;
    self.curLb.text = [self timeFormatted:sender.value];
    player.curTime = sender.value;
}

- (IBAction)sliderChanged:(UISlider *)sender {
    self.curLb.text = [self timeFormatted:sender.value];
}

- (IBAction)touchDown:(UISlider *)sender {
    
    isTouched = YES;
}



- (void)testFFmpeg
{

    NSArray *videourls = @[@"/Users/smart/Documents/temp/douyu/2002312ryyDsNuoe.flv",
                           @"/Users/smart/Documents/temp/douyu/qiaoer.flv",
                           @"/Users/smart/Documents/temp/douyu/1/fx_hifi_269530607.flv",
                          @"/Users/smart/Documents/temp/douyu/output.mp4",
                           @"/Users/smart/Documents/temp/qianxun/qianxun1.flv",
                           @"http://bobolive.nosdn.127.net/bobo_1477399116609_39807374.mp4",
                           @"rtmp://flvbohetec1.live.126.net/live/1b609928586a448ab77ba1d958ca7a0c.flv?netease=flvbohetec1.live.126.net?t=1497871394285"];

    player = [[ZZFFmpegPlayer alloc]init];
    [player openFile:videourls[1]];
    __weak ZZFFmpegPlayer *weakPlayer = player;
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

@end
