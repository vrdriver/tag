#ifndef _tag_ogg_h
#define _tag_ogg_h
#ifdef      VORBISSUPPORT

#include <stdio.h>
#include <string.h>
#include <io.h>
#include <stdlib.h>
#include <windows.h>
#include "Tag.h"

int ReadFileInfoVorbis ( FileInfo* Info );

#endif
#endif
