#include "util.h"
#include <iostream>
using namespace std;

double avio_r2d(AVRational ration)
{
    return ration.den == 0? 0 : (double)ration.num / (double)ration.den;
}

void write_one_frame(const char* path, AVFrame* frame)
{
    FILE *file = fopen(path, "ab+");
    if(!file)
    {
        cout << "can not open file: " << path << endl;
    }
    for(int i = 0; i < frame->height; i++)
    {
        fwrite(frame->data[0] + frame->linesize[0] * i, frame->linesize[0], 1, file);
    }
    for(int i = 0; i < frame->height / 2; i++)
    {
        fwrite(frame->data[1] + frame->linesize[1] * i, frame->linesize[1], 1, file);
    }
    for(int i = 0; i < frame->height / 2; i++)
    {
        fwrite(frame->data[2] + frame->linesize[2] * i, frame->linesize[2], 1, file);
    }
    fclose(file);
}

void write_rgb24_frame(const char* path, const uint8_t *data, const uint64_t size)
{
    FILE *file = fopen(path, "ab+");
    if(!file)
    {
        cout << "can not open file: " << path << endl;
    }
    fwrite(data, size, 1, file);
    fclose(file);
}

void SaveFrameAsJepg(const char* path, AVFrame* frame, int width, int height, int index)
{
    char filePath[255] = {0};
#ifndef _WIN32
    sprintf(filePath, "%s/image%d.jpg", path, index);
#else
    sprintf(filePath, "%s\\image%d.jpg", path, index);
#endif

    // 分配AVFormatContext对象
    AVFormatContext *pFormatCtx = avformat_alloc_context();

    // 设置输出文件格式
    pFormatCtx->oformat = av_guess_format("mjpeg", NULL, NULL);

    // 创建并初始化AVIOContext
    if (avio_open(&pFormatCtx->pb, filePath, AVIO_FLAG_WRITE) < 0)
    {
        printf("Can not opem output file: %s\r\n", filePath);
        return;
    }

    // 构建一个新的stream
    AVStream *pStream = avformat_new_stream(pFormatCtx, 0);
    if(pStream == NULL)
    {
        printf("Create new AVStream failed!\r\n");
        return;
    }

    // 设置stream信息
    pStream->time_base.den = 25;
    pStream->time_base.num = 1;
    AVCodecContext *pCodecCtx = pStream->codec;
    pCodecCtx->codec_id = pFormatCtx->oformat->video_codec;
    pCodecCtx->codec_type = AVMEDIA_TYPE_VIDEO;
    pCodecCtx->pix_fmt = AV_PIX_FMT_YUVJ420P;
    pCodecCtx->width = width;
    pCodecCtx->height = height;
    pCodecCtx->framerate.den = 1;
    pCodecCtx->framerate.num = 25;
    pCodecCtx->time_base.den = 25;
    pCodecCtx->time_base.num = 1;

    av_dump_format(pFormatCtx, 0, filePath, 1);

    // 查找编码器
    AVCodec *pCodec = avcodec_find_encoder(pCodecCtx->codec_id);
    if(pCodec == NULL)
    {
        printf("Can not find codec!\r\n");
        return;
    }

    int ret = avcodec_open2(pCodecCtx, pCodec, NULL);

    if(ret < 0)
    {
        printf("Can not open codec@\n");
        char buf[1024] = { 0 };
        av_strerror(ret, buf, sizeof(buf) - 1);
        getchar();
        return;
    }

    // 写头部信息
    avformat_write_header(pFormatCtx, NULL);

    int y_size = pCodecCtx->width * pCodecCtx->height;

    // 给AVPacket
    AVPacket pkt;
    av_new_packet(&pkt, y_size * 3);


    int got_pic = 0;
    ret = avcodec_encode_video2(pCodecCtx, &pkt, frame, &got_pic);
    if(ret<0)
    {
        printf("Encode erroor\n");
        return;
    }
    if(got_pic == 1)
    {
        ret = av_write_frame(pFormatCtx, &pkt);
    }

    av_free_packet(&pkt);

    av_write_trailer(pFormatCtx);

    printf("Encode Successful.\n");

    if(pStream)
    {
        avcodec_close(pStream->codec);
    }
    avformat_free_context(pFormatCtx);
}
