#include "savefile.h"

void SaveFrameAsJepg(char* path, AVFrame* frame, int width, int height, int index)
{
    char filePath[255] = {0};
    sprintf(filePath, "%s\\image%d", path, index);

    // 分配AVFormatContext对象
    AVFormatContext *pFormatCtx = avformat_alloc_context();

    // 设置输出文件格式
    pFormatCtx->oformat = av_guess_format("mjpeg", NULL, NULL);

    // 创建并初始化AVIOContext
    if (avio_open(&pFormatCtx->pb, filepath, AVIO_FLAG_WRITE) < 0)
    {
        printf("Can not opem output file: %s\r\n", filepath);
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
    AVCodecContext *pCodecCtx = pStream->codec;
    pCodecCtx->codec_id = pFormatCtx->oformat->video_codec;
    pCodecCtx->codec_type = AVMEDIA_TYPE_VIDEO;
    pCodecCtx->pix_fmt = PIX_FMT_YUVJ420P;
    pCodecCtx->width = width;
    pCodecCtx->height = height;
    pCodecCtx->time_base.den = 25;
    pCodecCtx->time_base.num = 1;

    av_dump_format(pFormatCtx, 0, filepath, 1);

    // 查找编码器
    AVCodec *pCodec = avcodec_find_encoder(pCodecCtx->codec_id);
    if(pCodec == NULL)
    {
        printf("Can not open codec!\r\n");
        return;
    }

    // 写头部信息
    avformat_write_header(pFormatCtx, NULL);

    int y_size = pCodecCtx->width * pCodecCtx->height;

    // 给AVPacket
    AVPacket pkt;
    av_new_packet(&pkt, y_size * 3);
    //https://blog.csdn.net/oldmtn/article/details/46742555


}
