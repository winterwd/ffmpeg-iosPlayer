//
//  ffmpegDecoder.c
//  testFFmpeg
//
//  Created by zelong zou on 16/8/25.
//  Copyright © 2016年 XiaoWoNiu2014. All rights reserved.
//

#include "ffmpegDecoder.h"
#include <unistd.h>
#define MAX_AUDIO_FRAME_SIZE (44100*4*2) // 1 second of 44khz 32bit audio

#define MAX_AUDIOQ_SIZE (5 * 16 * 1024)
#define MAX_VIDEOQ_SIZE (5 * 256 * 1024)

static inline void checkError(int ret,int success){
    
    if (ret != success) {
        printf("\n error in line: %d \n",(__LINE__));
    }
}

int ffmpeg_read_packet_Process(ffmpegDecoder *decoder);
double synchronize_video(ffmpegDecoder *decoder,AVFrame *frame,double pts);
ffmpegDecoder *ffmpeg_decoder_alloc_init()
{
    av_register_all();
    ffmpegDecoder *instance = (ffmpegDecoder *)malloc(sizeof(ffmpegDecoder));
    instance->pAcodec = NULL;
    instance->pVcodec = NULL;
    instance->pAcodectx = NULL;
    instance->pVcodectx = NULL;
    instance->pFormatCtx = NULL;
    instance->videoStreamIndex = -1;
    instance->auidoStreamIndex = -1;
    instance->quit = 0;
    instance->swr = NULL;
    instance->audio_buffer_size = 0;
    instance->stop = 0;
    instance->decoded_audio_data_callback = NULL;
    instance->decoded_video_data_callback = NULL;
    ffmpegPacketQueue_init(&instance->audioPakcetQueue);
    ffmpegPacketQueue_init(&instance->videoPacketQueue);
    pthread_mutex_init(&instance->pic_mutex, NULL);
    pthread_cond_init(&instance->pic_cond, NULL);
    instance->vframe_queue_size = 0;
    instance->vframe_queue_windex = 0;
    instance->vframe_queue_rindex = 0;
    
    return instance;
}


int ffmpeg_decoder_open_file(ffmpegDecoder *decoder,const char *path) {
    
    int ret = 0;
    
    if (strlen(path)==0) {
        checkError(ret, 0);
        return 0;
    }
    strcpy(decoder->fileName, path);
    
    decoder->pFormatCtx = avformat_alloc_context();
    
    ret = avformat_open_input(&decoder->pFormatCtx, decoder->fileName, NULL, NULL);
    
    checkError(ret, 0);
    
    ret =  avformat_find_stream_info(decoder->pFormatCtx, NULL);
    
    if (ret<0) {
        checkError(ret, 0);
        return 0;
    }
    
    av_dump_format(decoder->pFormatCtx, 0, decoder->fileName, 0);
    
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
    
    decoder->audio_buffer = (uint8_t *)malloc(decoder->audio_buffer_size);
    
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

    for (int i=0; i<VIDEO_QUEUE_SIZE; i++) {
        AVFrame *rgbFrame = av_frame_alloc();
        uint8_t *videoBuffer = av_malloc(decoder->video_buffer_size);
        rgbFrame->width = decoder->width;
        rgbFrame->height = decoder->height;
        avpicture_fill((AVPicture *)rgbFrame, videoBuffer, PIX_FMT_YUV420P, rgbFrame->width, rgbFrame->height);
        decoder->vFrameQueue[i] = rgbFrame;
    }
    
    
    
    decoder->sws = sws_getContext(decoder->width, decoder->height, decoder->pVcodectx->pix_fmt, decoder->width, decoder->height, PIX_FMT_YUV420P, SWS_BILINEAR, NULL, NULL, NULL);
    
    return 1;
}

void *ffmpeg_decoder_decode_file(void *userData)
{

    ffmpegDecoder *decoder = (ffmpegDecoder *)userData;
        

    
    for(;;){
        
        if (decoder->quit) {
            break;
        }
        
        if (decoder->audioPakcetQueue.size>MAX_AUDIOQ_SIZE || decoder->videoPacketQueue.size>MAX_VIDEOQ_SIZE) {
            usleep(10*1000); //延迟10ms
            continue;
        }
        if (ffmpeg_read_packet_Process(decoder)<0) break;
    }
    
    while (decoder->quit == 0) {
        usleep(100*1000);
    }
    
    printf("decode video thread is ending ...\n");
    return NULL;

    
}



void *ffmpeg_video_thread(void *argc){
    ffmpegDecoder *decoder = (ffmpegDecoder *)argc;
    AVPacket packet;
    AVFrame *pframe = av_frame_alloc();
    int gotFrame = 0;
    while (1) {
        
        if (packet_queue_block_get(&(decoder->videoPacketQueue), &packet,1)<1) {
            break;
        }
        
        //解码
        avcodec_decode_video2(decoder->pVcodectx, pframe, &gotFrame, &packet);
        double pts = 0;
        
        if (packet.dts == AV_NOPTS_VALUE && pframe->opaque&& *(uint64_t *)pframe->opaque != AV_NOPTS_VALUE) {
            pts = *(uint64_t *)pframe->opaque;
        }else if (packet.dts != AV_NOPTS_VALUE){
            pts = packet.dts;
        }else{
            pts = 0;
        }
        printf("pts = %f ",pts);
        pts *= av_q2d(decoder->pFormatCtx->streams[decoder->videoStreamIndex]->time_base);
        
        printf("pts = %f ",pts);
        if (gotFrame) {
            
            pts = synchronize_video(decoder, pframe, pts);
            
            printf("pts = %f \n",pts);
            //放入缓冲队列里
            if(quque_picture(decoder, pframe)<0) break;

        }
        
        av_free_packet(&packet);
    }
    
    av_frame_free(&pframe);
    return NULL;
}

double synchronize_video(ffmpegDecoder *decoder,AVFrame *frame,double pts){
    
    double frame_delay;
    if (pts == 0) {
        pts = decoder->video_clock;
    }else{
        decoder->video_clock = pts;
    }
    
    frame_delay = av_q2d(decoder->pFormatCtx->streams[decoder->videoStreamIndex]->time_base);
    
    frame_delay += frame->repeat_pict * (frame_delay*0.5);
    
    decoder->video_clock += frame_delay;
    
    return pts;
    
}



AVFrame * quque_picture_get_frame(ffmpegDecoder *decoder){
    
    if (decoder->vframe_queue_size == 0) {
        return NULL;
    }
    AVFrame *rgbFrame = decoder->vFrameQueue[0];
    pthread_mutex_lock(&decoder->pic_mutex);
    decoder->vframe_queue_size = 0;
    decoder->vframe_queue_windex = 0;
    pthread_cond_signal(&decoder->pic_cond);
    pthread_mutex_unlock(&decoder->pic_mutex);
    
    return rgbFrame;
}

int quque_picture(ffmpegDecoder *decoder,AVFrame *frame){
    
    
    pthread_mutex_lock(&decoder->pic_mutex);
    while (!decoder->quit && decoder->vframe_queue_size>= VIDEO_QUEUE_SIZE) {
        //数组空间不够填充视频祯数据 ，等待
        pthread_cond_wait(&decoder->pic_cond, &decoder->pic_mutex);
        
    }
    pthread_mutex_unlock(&decoder->pic_mutex);
    
    AVFrame *rgbFrame = decoder->vFrameQueue[decoder->vframe_queue_windex];
    sws_scale(decoder->sws, ( uint8_t const* const * )frame->data, frame->linesize, 0, decoder->height, rgbFrame->data, rgbFrame->linesize);

    pthread_mutex_lock(&decoder->pic_mutex);
    decoder->vframe_queue_windex++;
    decoder->vframe_queue_size++;
    pthread_mutex_unlock(&decoder->pic_mutex);
    
    
    if (decoder->decoded_video_data_callback) {
        
        
        
        double diff = decoder->video_clock - decoder->audio_clock;
        
        double delay = 0;
        if (diff < 0) {  //视频比音频慢
            delay = 0;
        }else{ //视频比音频快
            delay = diff;
        }
        
        if (delay!=0) {
            usleep(delay*1000000);
        }
        decoder->decoded_video_data_callback(rgbFrame,decoder->audio_buffer_size);
        
        
        pthread_mutex_lock(&decoder->pic_mutex);
        decoder->vframe_queue_size = 0;
        decoder->vframe_queue_windex = 0;
        pthread_cond_signal(&decoder->pic_cond);
        pthread_mutex_unlock(&decoder->pic_mutex);
        
    }
    
    
    return 1;
}


int ffmpeg_decoder_star(ffmpegDecoder *decoder){
    
    printf("decode video thread is starting ...\n");
    //启动解码线程
    pthread_create(&decoder->decodeThread, NULL, ffmpeg_decoder_decode_file, (void*)decoder);
    pthread_create(&decoder->videoThread, NULL, ffmpeg_video_thread, (void*)decoder);
    return 1;
}



int ffmpeg_read_packet_Process(ffmpegDecoder *decoder)
{
    
    
    AVPacket packet;
    if (av_read_frame(decoder->pFormatCtx, &packet)>=0) {
        
        if (packet.stream_index == decoder->auidoStreamIndex) { //音频包

            packet_queue_put(&decoder->audioPakcetQueue, &packet);
            
//            printf("audio packets in queue: %d \n",decoder->audioPakcetQueue.nb_packets);
            
        }else if (packet.stream_index == decoder->videoStreamIndex){ //视频包
            packet_queue_put(&decoder->videoPacketQueue, &packet);
//            printf("vidoe packets in queue: %d \n",decoder->audioPakcetQueue.nb_packets);
        }else {
            av_free_packet(&packet); //释放 非音视频包
        }
  
    }else{
        if(decoder->pFormatCtx->pb->error == 0){
            usleep(100*1000);
        }else{
            //read frame error
            return -1;
        }
    }

    
    return 1;
}






int ffmpeg_decode_video_frame(ffmpegDecoder *decoder,AVFrame *frame)
{
    AVPacket packet;

    int gotVideoFrame = 0;
    AVFrame *pframe = av_frame_alloc();
    int ret = packet_queue_get(&decoder->videoPacketQueue,&packet);
    if (ret == 0) {
        printf("\n queue is empty! \n");
        return  0;
    }else{
        

        ret = avcodec_decode_video2(decoder->pVcodectx, pframe, &gotVideoFrame, &packet);
        
        if (ret < 0) {
            printf("\n function avcodec_decode_video2() error! \n");
        }
        if (gotVideoFrame && ret) {
            sws_scale(decoder->sws, ( uint8_t const* const * )decoder->pVideoFrame->data, decoder->pVideoFrame->linesize, 0, decoder->height, decoder->pFrameRGB->data, decoder->pFrameRGB->linesize);

        }
    }

    av_free_packet(&packet);
    
   
    return 1;
}

int   ffmpeg_decode_audio_frame(ffmpegDecoder *decoder, uint8_t *outPcmData,int *outDatasize)
{
    
    
   
//    AVPacket packet;
    
    static AVPacket packet;
    static AVFrame  frame;
    
    int audioLength = 0,gotAudioFrame = 0;

    

    
    int ret = packet_queue_get(&decoder->audioPakcetQueue,&packet);


    if (ret == 0) {
        printf("\n audio queue is empty! \n");\
        *outDatasize = decoder->audio_buffer_size;
        memset(outPcmData, 0, decoder->audio_buffer_size);
        return ret;


    }else{
        
//        AVFrame *audioFrame = av_frame_alloc();
        if (packet.pts != AV_NOPTS_VALUE) {
            decoder->audio_clock = packet.pts*av_q2d((decoder->pFormatCtx->streams[decoder->auidoStreamIndex]->time_base));
        }
        
        printf("audio pts = %f  \n",decoder->audio_clock);
        
        audioLength = avcodec_decode_audio4(decoder->pAcodectx, &frame, &gotAudioFrame, &packet);
        
        if (audioLength>0 && gotAudioFrame ) {
            *outDatasize = decoder->audio_buffer_size;
            swr_convert(decoder->swr, &outPcmData, MAX_AUDIO_FRAME_SIZE/2, (const uint8_t **)frame.data, frame.nb_samples);

            
        }else{
            printf("\n decoder audio error! \n");
            
            ret = 0;

        }
        
//        av_free(audioFrame);
    }


    if (packet.data != NULL) {
        av_free_packet(&packet);
    }
    
    return ret;
}

int  ffmpeg_decoder_stop(ffmpegDecoder *decoder)
{
    decoder->quit = 1;
    pthread_cancel(decoder->decodeThread);
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
