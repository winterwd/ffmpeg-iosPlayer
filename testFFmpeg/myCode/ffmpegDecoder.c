//
//  ffmpegDecoder.c
//  testFFmpeg
//
//  Created by zelong zou on 16/8/25.
//  Copyright © 2016年 XiaoWoNiu2014. All rights reserved.
//

#include "ffmpegDecoder.h"
#include <unistd.h>
#define MAX_AUDIO_FRAME_SIZE (44100*4*2) // 1 second of 48khz 32bit audio
static inline void checkError(int ret,int success){
    
    if (ret != success) {
        printf("\n error in line: %d \n",(__LINE__));
    }
}

int ffmpeg_read_packet_Process(ffmpegDecoder *decoder);

ffmpegDecoder *ffmpeg_decoder_alloc_init()
{
    ffmpegDecoder *instance = (ffmpegDecoder *)malloc(sizeof(ffmpegDecoder));
    instance->pAcodec = NULL;
    instance->pVcodec = NULL;
    instance->pAcodectx = NULL;
    instance->pVcodectx = NULL;
    instance->pFormatCtx = NULL;
    instance->videoStreamIndex = -1;
    instance->auidoStreamIndex = -1;
    instance->swr = NULL;
    instance->audioOutBuffer = NULL;
    instance->audio_buffer_size = 0;
    instance->stop = 0;
    instance->decoded_audio_data_callback = NULL;
    instance->decoded_video_data_callback = NULL;
    ffmpegPacketQueue_init(&instance->audioPakcetQueue);
    ffmpegPacketQueue_init(&instance->videoPacketQueue);
    return instance;
}

int ffmpeg_decoder_decode_file(ffmpegDecoder *decoder,const char *path)
{
    av_register_all();
    decoder->pFormatCtx = avformat_alloc_context();
    
    int ret = avformat_open_input(&decoder->pFormatCtx, path, NULL, NULL);
    checkError(ret, 0);
    
    ret =  avformat_find_stream_info(decoder->pFormatCtx, NULL);
    
    if (ret<0) {
        checkError(ret, 0);
        return 0;
    }
    
    av_dump_format(decoder->pFormatCtx, 0, path, 0);
    
    for (int i=0; i<decoder->pFormatCtx->nb_streams; i++) {
        AVCodecContext *pctx =decoder->pFormatCtx->streams[i]->codec;
        if (pctx->codec_type == AVMEDIA_TYPE_VIDEO) {
            decoder->videoStreamIndex = i;
            decoder->pVcodec = avcodec_find_decoder(pctx->codec_id);
            decoder->pVcodectx = avcodec_alloc_context3(decoder->pVcodec);
            ret = avcodec_copy_context(decoder->pVcodectx, pctx);
            checkError(ret, 0);
            
            ret = avcodec_open2(decoder->pVcodectx, decoder->pVcodec, NULL);
            checkError(ret, 0);
            
        }else if (pctx->codec_type == AVMEDIA_TYPE_AUDIO){
            decoder->auidoStreamIndex = i;
            decoder->pAcodec = avcodec_find_decoder(pctx->codec_id);
            decoder->pAcodectx = avcodec_alloc_context3(decoder->pAcodec);
            ret = avcodec_copy_context(decoder->pAcodectx, pctx);
            checkError(ret, 0);
            
            ret = avcodec_open2(decoder->pAcodectx, decoder->pAcodec, NULL);
            checkError(ret, 0);
        }
    }
    
    

    


    
    
    
    // to  AV_SAMPLE_FMT_S16  2 chanels
    SwrContext *swrctx = swr_alloc();
    
    
    int64_t out_ch_layout = AV_CH_LAYOUT_STEREO;
    enum AVSampleFormat out_sample_fmt = AV_SAMPLE_FMT_S16;
    int out_sample_rate = decoder->pAcodectx->sample_rate;
    
    int64_t in_ch_layout = decoder->pAcodectx->channel_layout;
    enum AVSampleFormat in_sample_fmt = decoder->pAcodectx->sample_fmt;
    int in_sample_rate = decoder->pAcodectx->sample_rate;
    
    swr_alloc_set_opts(swrctx, out_ch_layout, out_sample_fmt, out_sample_rate, in_ch_layout, in_sample_fmt, in_sample_rate, 0, NULL);
    
    swr_init(swrctx);
    
    
    decoder->swr = swrctx;
    
    decoder->audio_buffer_size = av_samples_get_buffer_size(NULL, decoder->pAcodectx->channels, decoder->pAcodectx->frame_size, out_sample_fmt, 0);
    
    decoder->audioOutBuffer = malloc(MAX_AUDIO_FRAME_SIZE);
    
    decoder->samplerate = out_sample_rate;
    decoder->nb_channel = decoder->pAcodectx->channels;
    
    
    //video
    decoder->width = decoder->pVcodectx->width;
    decoder->height = decoder->pVcodectx->height;
    
    
//    numBytes=avpicture_get_size(PIX_FMT_RGB24, pCodecCtx->width,
//                                pCodecCtx->height);
//    buffer=(uint8_t *)av_malloc(numBytes*sizeof(uint8_t));
//    
//    // Assign appropriate parts of buffer to image planes in pFrameRGB
//    // Note that pFrameRGB is an AVFrame, but AVFrame is a superset
//    // of AVPicture
    

    
    decoder->video_buffer_size = avpicture_get_size(PIX_FMT_YUV420P, decoder->width, decoder->height);
    decoder->videoOutBuffer = (uint8_t *)av_malloc(decoder->video_buffer_size*(sizeof(uint8_t)));
    
    decoder->pFrameRGB =av_frame_alloc();
    decoder->pFrameRGB->width = decoder->width;
    decoder->pFrameRGB->height = decoder->height;
    avpicture_fill((AVPicture *)decoder->pFrameRGB, decoder->videoOutBuffer, PIX_FMT_YUV420P,
                   decoder->width, decoder->height);

    
    decoder->pVideoFrame = av_frame_alloc();
    
    decoder->sws = sws_getContext(decoder->width, decoder->height, decoder->pVcodectx->pix_fmt, decoder->width, decoder->height, PIX_FMT_YUV420P, SWS_BILINEAR, NULL, NULL, NULL);

    
    return 1;
}


void *read_packet_thread(void *argc) {
    
    ffmpegDecoder *decoder = (ffmpegDecoder *)argc;
    
    ffmpeg_read_packet_Process(decoder);
    return NULL;
}

void *thread1(void *argc) {
    
    printf("decode video thread is starting ...\n");
    ffmpegDecoder *decoder = (ffmpegDecoder *)argc;
    ffmpeg_decode_video_frame(decoder, NULL);
    printf("decode video thread is ending ...\n");
    return NULL;
}


int ffmpeg_decoder_start(ffmpegDecoder *decoder)
{
    
    
    pthread_t read_thread;
    pthread_create(&read_thread, NULL, read_packet_thread, (void *)decoder);
    
    
//    pthread_t decode_thread1;
//    pthread_create(&decode_thread1, NULL, thread1, (void *)decoder);
    
    printf("\n ffmpeg done! \n");
    
    
    //    pthread_join(decode_thread, NULL);
    return 1;
    
}



int ffmpeg_read_packet_Process(ffmpegDecoder *decoder)
{
    
    printf("\n ffmpeg audio decode thread is starting ! \n");
    
//    AVPacket *packet = malloc(sizeof(AVPacket));
//    av_init_packet(packet);
    
    AVPacket packet;
    while (av_read_frame(decoder->pFormatCtx, &packet)>=0) {
        
        if (packet.stream_index == decoder->auidoStreamIndex) {
            
            
            
            packet_queue_put(&decoder->audioPakcetQueue, &packet);
            
            printf("audio packets in queue: %d \n",decoder->audioPakcetQueue.nb_packets);
            
            /*
             avcodec_get_frame_defaults(audioFrame);
             
             audioLength = avcodec_decode_audio4(decoder->pAcodectx, audioFrame, &gotAudioFrame, packet);
             
             if (audioLength>0 && gotAudioFrame) {
             printf("\n decoder audio success! \n");
             
             swr_convert(decoder->swr, &(decoder->audioOutBuffer), MAX_AUDIO_FRAME_SIZE/2, (const uint8_t **)audioFrame->data, audioFrame->nb_samples);
             
             if (decoder->decoded_audio_data_callback != NULL) {
             decoder->decoded_audio_data_callback(decoder->audioOutBuffer,decoder->audio_buffer_size);
             }
             
             
             
             }else{
             printf("\n decoder audio error! \n");
             }
             */
//            ffmpeg_decode_frame(decoder,NULL,&size);
        }else if (packet.stream_index == decoder->videoStreamIndex){
            packet_queue_put(&decoder->videoPacketQueue, &packet);
            printf("vidoe packets in queue: %d \n",decoder->audioPakcetQueue.nb_packets);
        }else {
            usleep(100000);
        }
  
    }
    

    
    
    printf("\n ffmpeg audio decode thread is ending ! \n");
    
    return 1;
}


void SaveFrame(AVFrame *pFrame, int width, int height, int iFrame) {
    FILE *pFile;
    char szFilename[64];
    int  y;
    
    // Open file
    sprintf(szFilename, "/Users/xiaowoniu/Documents/一些素材/testFrame/frame%d.ppm", iFrame);
    pFile=fopen(szFilename, "wb");
    if(pFile==NULL)
        return;
    
    // Write header
    fprintf(pFile, "P6\n%d %d\n255\n", width, height);
    
    // Write pixel data
    for(y=0; y<height; y++)
        fwrite(pFrame->data[0]+y*pFrame->linesize[0], 1, width*3, pFile);
    
    // Close file
    fclose(pFile);
}



static int zx = 0;

int ffmpeg_decode_video_frame(ffmpegDecoder *decoder,AVFrame *outVideoFrame)
{
    AVPacket packet;
    av_init_packet(&packet);
    int gotVideoFrame = 0;
    
    


   
    

    int ret = packet_queue_get(&decoder->videoPacketQueue,&packet);
    if (ret == 0) {
        printf("\n queue is empty! \n");
        return  0;
    }else{
        
//            AVFrame *videoFrame = av_frame_alloc();
        ret = avcodec_decode_video2(decoder->pVcodectx, decoder->pVideoFrame, &gotVideoFrame, &packet);
        
        if (ret < 0) {
            printf("\n function avcodec_decode_video2() error! \n");
        }
        if (gotVideoFrame && ret) {
            zx++;
            sws_scale(decoder->sws, ( uint8_t const* const * )decoder->pVideoFrame->data, decoder->pVideoFrame->linesize, 0, decoder->height, decoder->pFrameRGB->data, decoder->pFrameRGB->linesize);
//                outVideoFrame->width = decoder->width;
//                outVideoFrame->height = decoder->height;
            
            printf("\n decoder video success! \n");
//            if (zx%5 == 0) {
//                SaveFrame(decoder->pFrameRGB, decoder->width, decoder->height, zx);
//            }
        }
    }
        
//        usleep(20000);

    
    av_free_packet(&packet);
    
   
    return 1;
}

int   ffmpeg_decode_audio_frame(ffmpegDecoder *decoder, uint8_t **outPcmData,int *outDatasize)
{
    
    
   
        AVPacket packet;
        
        int audioLength = 0,gotAudioFrame = 0;
        
        
        AVFrame *audioFrame = av_frame_alloc();
        
        int ret = packet_queue_get(&decoder->audioPakcetQueue,&packet);
        
        *outDatasize = decoder->audio_buffer_size;
        if (ret == 0) {
            printf("\n queue is empty! \n");
//            usleep(2000);
    
            
            
        }else{
            audioLength = avcodec_decode_audio4(decoder->pAcodectx, audioFrame, &gotAudioFrame, &packet);
            
            if (audioLength>0 && gotAudioFrame ) {
                printf("\n decoder audio success! \n");
                
                swr_convert(decoder->swr, &(decoder->audioOutBuffer), MAX_AUDIO_FRAME_SIZE/2, (const uint8_t **)audioFrame->data, audioFrame->nb_samples);
//                if (decoder->decoded_audio_data_callback != NULL) {
//                    decoder->decoded_audio_data_callback(decoder->audioOutBuffer,decoder->audio_buffer_size);
//                }
                
            }else{
                printf("\n decoder audio error! \n");
                
                ret = 0;
                
                
            }
//            usleep(600);
        }
        

    av_free(audioFrame);

  

    
    return ret;
}

int  ffmpeg_decoder_stop(ffmpegDecoder *decoder)
{
    return 1;
}
