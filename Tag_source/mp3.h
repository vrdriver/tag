#ifndef _tag_mp3_h_
#define _tag_mp3_h_

#include <stdio.h>
#include <string.h>
#include <io.h>
#include <stdlib.h>
#include <windows.h>
#include "Tag.h"

typedef struct {
    int             frames;
    int             VBitRate;
    long            fileSize;
    unsigned long   bithdr;
} MP3StreamInfo;

int ReadFileInfoMP3 ( FileInfo* Info );

#endif
