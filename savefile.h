#ifndef SAVEFILE_H
#define SAVEFILE_H
#include <QObject>

extern "C"
{
#include "libavformat/avformat.h"
}

void SaveFrameAsJepg(const char* path, AVFrame* frame, int width, int height, int index);

#endif // SAVEFILE_H
