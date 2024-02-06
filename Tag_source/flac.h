#ifndef _tag_flac_h
#define _tag_flac_h
#ifdef      FLACSUPPORT

#include <stdio.h>
#include <string.h>
#include <io.h>
#include <stdlib.h>
#include <windows.h>
#include "Tag.h"

int ReadFileInfoFLAC ( FileInfo* Info );

#endif
#endif
