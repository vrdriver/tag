#ifndef _tag_nfofile_h
#define _tag_nfofile_h

#include "Tag.h"
#include "misc.h"
#include "playlist.h"

typedef struct {
    char**          Lines;
    size_t          LineCount;

    enum case_t     CaseConversion;
    char            ReplaceSpaces;
    int             ApplyToTracks;
    char*           Scheme;
    char*           SchemeVA;
} NFOfileTemplate;

int WriteNFOfile ( const char* nfofile, const NFOfileTemplate* NFOfile, const DetailedList* Details, const FileInfo *Info, const ExceptionList* Exceptions );

#endif
