#ifndef SAVEFILE_H
#define SAVEFILE_H
#include <QObject>

extern "C"
{
#include "libavformat/avformat.h"
}

double avio_r2d(AVRational ration);

void write_one_frame(const char* path, AVFrame* frame);

void SaveFrameAsJepg(const char* path, AVFrame* frame, int width, int height, int index);

void write_rgb24_frame(const char* path, const uint8_t *data, const uint64_t size);

#endif // SAVEFILE_H
