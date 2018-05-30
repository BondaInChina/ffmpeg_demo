#include <QCoreApplication>
#include <iostream>
#include "savefile.h"
extern "C"{
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libswscale/swscale.h"
}
using namespace std;

#ifndef _WIN32
static const char* mp4File = "/home/bonda/demo/ffmpeg_demo/test.mp4";
static const char* yuvFile = "/home/bonda/demo/ffmpeg_demo/yuv.yuv";
static const char* rgbFile = "/home/bonda/demo/ffmpeg_demo/rgb.yuv";
static const char* jpegFile = "/home/bonda/demo/ffmpeg_demo/jpeg.jpg";
#else
static const char* mp4File = "E:\\test.mp4";
static const char* yuvFile = "E:\\yuv.yuv";
static const char* rgbFile = "E:\\rgb.yuv";
static const char* jpegFile = "E:\\jpeg.jpg";
#endif

static double avio_r2d(AVRational ration)
{
    return ration.den == 0? 0 : (double)ration.num / (double)ration.den;
}

static void write_one_frame(const char* path, AVFrame* frame)
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

static void write_rgb24_frame(const char* path, const uint8_t *data, const uint64_t size)
{
    FILE *file = fopen(path, "ab+");
    if(!file)
    {
        cout << "can not open file: " << path << endl;
    }
    fwrite(data, size, 1, file);
    fclose(file);
}

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    printf("%s\n",avutil_configuration());
    const char *path = mp4File;
    //初始化封装库
    av_register_all();

    //解封装上下文
    AVFormatContext *ic = NULL;
    int re = avformat_open_input(
        &ic,
        path,
        NULL,  // 0表示自动选择解封器
        NULL //参数设置，比如rtsp的延时时间
    );
    if (re != 0)
    {
        char buf[1024] = { 0 };
        av_strerror(re, buf, sizeof(buf) - 1);
        cout << "open " << path << " failed! :" << buf << endl;
        getchar();
        return -1;
    }
    cout << "open " << path << " success! " << endl;
    avformat_find_stream_info(ic, NULL);
    av_dump_format(ic,0,path,0);

    int videoIndex = -1;
    int audioIndex = -1;
    for(int i = 0; i < ic->nb_streams; i++)
    {
        if(ic->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            videoIndex = i;
        }
        else if(ic->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO)
        {
            audioIndex = i;
        }
    }

    AVStream* videoStream = ic->streams[videoIndex];
    AVStream* audioStream = ic->streams[audioIndex];
    cout << "video duration(ms): " << videoStream->duration * avio_r2d(videoStream->time_base) << endl;
    cout << "audio duration(ms): " << audioStream->duration * avio_r2d(audioStream->time_base) << endl;
    cout << "video frame rate: " << avio_r2d(videoStream->avg_frame_rate) << endl;
    cout << "width: " << videoStream->codec->width << ", height: " << videoStream->codec->height <<endl;
    cout << "sample_rate = " << audioStream->codec->sample_rate << endl;
    //AVSampleFormat;
    cout << "channels = " << audioStream->codec->channels << endl;


    int64_t time = 200;
    av_seek_frame(ic, videoIndex,
                  (double)time / (double)avio_r2d(videoStream->time_base),
                  AVSEEK_FLAG_BACKWARD|AVSEEK_FLAG_FRAME);
    // 找视频解码器
    AVCodecContext *videoCodecContext= videoStream->codec;
    AVCodec *videoCodec = avcodec_find_decoder
            (videoCodecContext->codec_id);
    if(videoCodec == NULL)
    {
        cout << "can not find codec id: " << videoCodecContext->codec_id << endl;
        getchar();
        return -1;
    }
    int ret = avcodec_open2(videoCodecContext, videoCodec, NULL);
    if(ret != 0)
    {
        char msg[1024] = {0};
        av_strerror(ret, msg, 1024);
        cout << msg << endl;
        getchar();
        return -1;
    }
    cout << "open video codec success" << endl;

    // 找音频解码器
    AVCodecContext *audioCodecContext= audioStream->codec;
    AVCodec *audioCodec = avcodec_find_decoder(audioCodecContext->codec_id);
    if(!audioCodec)
    {
        cout << "can not find codec id: " << audioCodecContext->codec_id << endl;
        getchar();
        return -1;
    }
    ret = avcodec_open2(audioCodecContext, audioCodec, NULL);
    if(ret != 0)
    {
        char msg[1024] = {0};
        av_strerror(ret, msg, 1024);
        getchar();
        return -1;
    }
    cout << "open audio codec success" << endl;
    int got_pic = 0;
    AVFrame *vFrame = av_frame_alloc();
    AVPacket *pkt = (AVPacket *) malloc(sizeof(AVPacket)); //分配一个packet
    av_new_packet(pkt, videoCodecContext->width * videoCodecContext->height);
    //av_init_packet(pkt);
    SwsContext *sws = sws_getContext(videoStream->codec->width,
                                     videoStream->codec->height,
                                     videoStream->codec->pix_fmt,
                                     videoStream->codec->width,
                                     videoStream->codec->height,
                                     AV_PIX_FMT_BGR24,
                                     SWS_BICUBIC,
                                     NULL,
                                     NULL,
                                     NULL
                                     );
    uint8_t* rgb[2] = {0};
    int index = 0;
    while(1)
    {
        if(av_read_frame(ic,pkt) != 0)
        {
            break;
        }
        if(pkt->stream_index == videoIndex)
        {
            cout << "packet size: " << pkt->size << endl;
            cout << "pts: " << pkt->pts * avio_r2d(videoStream->time_base) << endl;
            cout << "dts: " << pkt->dts * avio_r2d(videoStream->time_base)<< endl;

            avcodec_decode_video2(videoCodecContext, vFrame, &got_pic, pkt);
            if(got_pic != 0)
            {
                write_one_frame(yuvFile, vFrame);
                if(index++ % 10 == 0)
                {
                    SaveFrameAsJepg(jpegFile, vFrame, vFrame->width, vFrame->height, index);
                }

                if(rgb[0] == NULL)
                {
                    rgb[0] = new unsigned char[vFrame->width * vFrame->height * 3];
                }
                int lines[2] = {vFrame->width * 3};
                // 转换图像翻转,进行如下修正，参考http://blog.sina.com.cn/s/blog_4ae178ba0100s7er.html
                vFrame->data[0] += vFrame->linesize[0] * (vFrame->height - 1);
                vFrame->linesize[0] *= -1;
                vFrame->data[1] += vFrame->linesize[1] * (vFrame->height / 2 - 1);
                vFrame->linesize[1] *= -1;
                vFrame->data[2] += vFrame->linesize[2] * (vFrame->height / 2 - 1);
                vFrame->linesize[2] *= -1;

                int ret = sws_scale(sws, vFrame->data, vFrame->linesize,0,vFrame->height,rgb,lines);
                write_rgb24_frame(rgbFile, rgb[0], vFrame->width * vFrame->height * 3);
            }
        }
    }

    if (ic)
    {
        avformat_close_input(&ic);
    }
    av_free_packet(pkt);

    getchar();
    return a.exec();
}
