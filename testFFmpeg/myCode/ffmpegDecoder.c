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
    instance->seek_req = 0;
    instance->video_clock = 0.0;
    instance->audio_clock = 0.0;
    instance->audioDelay = 0.0;
    instance->byte_per_seconds = 0;
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
    
    if (decoder->pFormatCtx->duration!= AV_NOPTS_VALUE) {
        decoder->duration = decoder->pFormatCtx->duration/AV_TIME_BASE;
    }else if (decoder->videoStreamIndex != -1){
        AVStream *st = decoder->pFormatCtx->streams[decoder->videoStreamIndex];
        decoder->duration = st->duration*av_q2d(st->time_base);
    }else if (decoder->auidoStreamIndex!= -1){
        AVStream *st = decoder->pFormatCtx->streams[decoder->auidoStreamIndex];
        decoder->duration = st->duration*av_q2d(st->time_base);
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
    
    //每秒的音频数据大小
    decoder->byte_per_seconds = decoder->nb_channel*decoder->samplerate*2;
    
    //音频初始延迟 （3是auido quque初始buffer个数）
    decoder->audioDelay = 0x10000/(float)decoder->byte_per_seconds * 3;
    
    printf("audio delay is %f\n",decoder->audioDelay);

    
    //video
    decoder->width = decoder->pVcodectx->width;
    decoder->height = decoder->pVcodectx->height;
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
        

    AVPacket flush_pkt;
    av_init_packet(&flush_pkt);
    flush_pkt.data = (uint8_t*)"flush";
    
    for(;;){
        
        if (decoder->quit) {
            break;
        }
        
        if (decoder->seek_req == 1) {
            int stream_index = -1;
            int64_t seek_target = decoder->seek_pos;
            if (decoder->videoStreamIndex>=0) {
                stream_index = decoder->videoStreamIndex;
            }else if (decoder->auidoStreamIndex>=0){
                stream_index = decoder->auidoStreamIndex;
            }
            
            if (stream_index>=0) {
                seek_target = av_rescale_q(seek_target, AV_TIME_BASE_Q, decoder->pFormatCtx->streams[stream_index]->time_base);
            }
            
            
            printf("\n begin seek to : %lld \n",seek_target);
            //快进》》
            if (av_seek_frame(decoder->pFormatCtx, stream_index, seek_target, decoder->seek_flag)<0) {
                printf("\n error: when seeking ... \n");
            }
            
            
            if (decoder->videoStreamIndex>=0) {
                packet_quque_flush(&decoder->videoPacketQueue);
//                packet_queue_put(&decoder->videoPacketQueue, &flush_pkt);
                
            }
            
            if(decoder->auidoStreamIndex>=0){
                packet_quque_flush(&decoder->audioPakcetQueue);
//                packet_queue_put(&decoder->audioPakcetQueue, &flush_pkt);
            }
            
            decoder->seek_req = 0;
            decoder->video_clock = decoder->curTime;
            decoder->audio_clock = decoder->curTime;
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
        pts = av_frame_get_best_effort_timestamp(pframe);
        if(pts == AV_NOPTS_VALUE) {
            pts = 0;
        }
        pts *= av_q2d(decoder->pFormatCtx->streams[decoder->videoStreamIndex]->time_base);
//        printf("pts1 = %f ",pts);

        if (gotFrame){

            pts = synchronize_video(decoder, pframe, pts);
            
//            printf("   pts2 = %f \n",pts);
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
    
    decoder->curTime = decoder->video_clock;
    
//    printf("cur time %f  ",decoder->curTime);
    
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
        printf("video clock: %f   audio_clock : %f   diff:[ %f ] \n",decoder->video_clock,decoder->audio_clock,diff);
        double delay = 0;
        if (diff < 0) {  //视频比音频慢
            delay = 0;
        }else{ //视频比音频快
            delay = diff;
        }
        if (delay>=1.0) {
            printf("----------delay > 1.0 --------- \n");
            delay = 1.0;
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

            
        }else if (packet.stream_index == decoder->videoStreamIndex){ //视频包
            packet_queue_put(&decoder->videoPacketQueue, &packet);

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

    static AVPacket packet;
    static AVFrame  frame;
    
    int audioLength = 0,gotAudioFrame = 0;

    

    
    int ret = packet_queue_get(&decoder->audioPakcetQueue,&packet);


    if (ret == 0) {
        printf("\n audio queue is empty! \n");\
        *outDatasize = decoder->audio_buffer_size;
        memset(outPcmData, 0, decoder->audio_buffer_size);
        decoder->audio_clock += decoder->audio_buffer_size/(float)decoder->byte_per_seconds;
        decoder->audio_clock -= decoder->audioDelay;
        return ret;


    }else{
        

        if (packet.pts != AV_NOPTS_VALUE) {
            decoder->audio_clock = packet.pts*av_q2d((decoder->pFormatCtx->streams[decoder->auidoStreamIndex]->time_base));
            //audio queue 有三个buffer， 当前音频时间 需要减去初始三个音频缓冲buffer的时间 , 每个buffer大小为0x10000
            decoder->audio_clock -= decoder->audioDelay; //当前正在播放的音频播放时间 (当前正在播放时间 = 当前解出祯的时间 - 剩余buffer所需时间)
            decoder->audio_clock = decoder->audio_clock <0 ?  0 : decoder->audio_clock;
        }
        
        
        audioLength = avcodec_decode_audio4(decoder->pAcodectx, &frame, &gotAudioFrame, &packet);
        
        if (audioLength>0 && gotAudioFrame ) {
            *outDatasize = decoder->audio_buffer_size;
            swr_convert(decoder->swr, &outPcmData, MAX_AUDIO_FRAME_SIZE/2, (const uint8_t **)frame.data, frame.nb_samples);

            
        }else{
            printf("\n decoder audio error! \n");
            
            ret = 0;

        }
    }


    if (packet.data != NULL) {
        av_free_packet(&packet);
    }
    
    return ret;
}

void stream_seek(ffmpegDecoder *decoder,int64_t pos,int rel){
    
    if (!decoder->seek_req) {
        decoder->seek_pos = pos;
        printf("\n seek to >>> %lld \n",decoder->seek_pos);
        decoder->seek_flag = rel<0?AVSEEK_FLAG_BACKWARD:0;
        decoder->seek_req = 1;
    }
    
}

void  seek_to_time(ffmpegDecoder *decoder, double time){
    
    int diff = time - decoder->curTime;
    stream_seek(decoder, (int64_t)(time * AV_TIME_BASE), diff);
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
