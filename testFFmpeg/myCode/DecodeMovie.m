//
//  DecodeMovie.m
//  testFFmpeg
//
//  Created by zelong zou on 14-3-14.
//  Copyright (c) 2014年 XiaoWoNiu2014. All rights reserved.
//

#import "DecodeMovie.h"
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
@implementation DecodeMovie

int saveFrame(AVFrame *frame,int iFrame,int width,int height,char *path)
{
    FILE *pFile;
    char frameName[100];
    sprintf(frameName, "/Users/xiaowoniu/Desktop/FFMPEG/test/frame%d.ppm",iFrame);
    pFile = fopen(frameName, "wb");
    
    // Write header
    fprintf(pFile, "P6\n%d %d\n255\n", width, height);

    for (int y=0; y<height; y++) {
        fwrite(frame->data[0]+y*frame->linesize[0], 1, width*3, pFile);
    }
    fclose(pFile);
    return 0;
}
int decoderMoive(int argc,char **argv)
{
    //0.register all
    av_register_all();
    //1.open file
    AVFormatContext *pFormatctx;
    pFormatctx = avformat_alloc_context();
    avformat_open_input(&pFormatctx, argv[1], NULL, NULL);
    
    //2.stream inforamtion
    avformat_find_stream_info(pFormatctx, NULL);
    
    //3.display format info
    av_dump_format(pFormatctx, 0, argv[1], 0);
    
    //4.finde the video stream
    
    int viedoStreamIndex = -1;
    for (int i=0; i<pFormatctx->nb_streams; i++) {
        if (pFormatctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
            viedoStreamIndex = i;
            break;
        }
    }
    
    if (viedoStreamIndex == -1) {
        return -1;
    }
    //5.get the codec context
    AVCodecContext *pCodecCtx = pFormatctx->streams[viedoStreamIndex]->codec;
    
    
    //6.find the codec for video
    AVCodec *pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
    
    if (pCodec == NULL) {
        return  -1;
    }
    
    //7.open the codec
    
    if (avcodec_open2(pCodecCtx, pCodec, NULL)<0) {
        return  -1;
    }
    
    //8. out: pframeRgb  in: pFrame
    AVFrame  *pFrame = av_frame_alloc();
    AVFrame  *pFrameRgb = av_frame_alloc();
    
    uint8_t *buffer;
    int numberBytes = avpicture_get_size(AV_PIX_FMT_BGR24, pCodecCtx->width, pCodecCtx->height);
    buffer = malloc(sizeof(uint8_t)*numberBytes);
    

    
    
    //8. decode data
    
    AVPacket packet;
    int gotFrame;
    
    struct SwsContext *img_convert_ctx;
    img_convert_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt,  pCodecCtx->width, pCodecCtx->height, AV_PIX_FMT_RGB24, SWS_BILINEAR, NULL, NULL, NULL);
    
        avpicture_fill((AVPicture *)pFrameRgb, buffer, AV_PIX_FMT_RGB24, pCodecCtx->width, pCodecCtx->height);
    if(img_convert_ctx == NULL)
    {
        fprintf(stderr, "Cannot initialize the conversion context!\n");
        exit(1);
    }
    
    int step = 0;
    while (av_read_frame(pFormatctx, &packet)>=0) {
        if (packet.stream_index == viedoStreamIndex) {
            avcodec_decode_video2(pCodecCtx, pFrame, &gotFrame, &packet);
            
            if (gotFrame) {
                sws_scale(img_convert_ctx,
                          (const uint8_t* const
                            *)pFrame->data, pFrame->linesize, 0, pCodecCtx->height,
                          pFrameRgb->data, pFrameRgb->linesize);
            }
            
            if (step++<20) {
                saveFrame(pFrameRgb, step,pCodecCtx->width,pCodecCtx->height,argv[0]);
            }
        }
        av_free_packet(&packet);
    }
    

    av_free(buffer);
    av_free(pFrameRgb);
    av_free(pFrame);
    
    avcodec_close(pCodecCtx);
    
    avformat_close_input(&pFormatctx);
    
    
    
    
    return 0;
}
@end
