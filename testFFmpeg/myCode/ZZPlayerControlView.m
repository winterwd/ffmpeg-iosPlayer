//
//  ZZPlayerControlView.m
//  testFFmpeg
//
//  Created by smart on 2017/7/12.
//  Copyright © 2017年 XiaoWoNiu2014. All rights reserved.
//

#import "ZZPlayerControlView.h"
#import <Masonry.h>

@interface ZZPlayerControlView ()
@property (nonatomic,strong) UIView   *topView;
@property (nonatomic,strong) UIView   *bottomView;
@property (nonatomic,strong) UIView   *contentView;
@property (nonatomic,strong) UISlider *sliderView;
@property (nonatomic,strong) UIProgressView *curProgressView;
@property (nonatomic,strong) UIProgressView *cacheProgressView;
@property (nonatomic,strong) UIButton       *playButton;
@property (nonatomic,strong) UILabel        *curTimeLabel;
@property (nonatomic,strong) UILabel        *totalTimeLabel;
@end

@implementation ZZPlayerControlView

- (instancetype)init {
    if (self = [super init]) {
        
        [self setupView];
        
    }
    return self;
}


- (void)setupView {
    _topView = [UIView new];
    [self addSubview:_topView];
    
    _contentView = [UIView new];
    [self addSubview:_contentView];
    
    _bottomView = [UIView new];
    [self addSubview:_bottomView];
    
    
}

- (void)setupTopView {
    
}

- (void)setupBottomView {
    
    _sliderView = [UISlider new];
    [self.bottomView addSubview:_sliderView];
    
    
    _playButton = [UIButton buttonWithType:UIButtonTypeCustom];
    [self.bottomView addSubview:_playButton];
    
    
    _cacheProgressView = [UIProgressView new];
    [self.bottomView addSubview:_cacheProgressView];
    
    _curProgressView = [UIProgressView new];
    [self.bottomView addSubview:_curProgressView];
    
    _curTimeLabel = [UILabel new];
    [self.bottomView addSubview:_curTimeLabel];
    
    _totalTimeLabel = [UILabel new];
    [self.bottomView addSubview:_totalTimeLabel];
    
    
}



@end
