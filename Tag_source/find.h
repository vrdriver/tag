#ifndef _tag_find_h
#define _tag_find_h

#include <io.h>
#include <direct.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <windows.h>

typedef struct {
    struct _finddata_t  Info;
    char                Path[_MAX_PATH];
} FileListEntry;

typedef struct {
    FileListEntry*      Files;
    size_t              FileCount;
} FileList;

// Returns long name of current working directory
char* _getcwd_long ( char* buffer, int maxlen );

// Finds recursively all files specified by mask
int _find_recurse ( const char* mask, FileList* Files );

// Finds all files specified by mask
int _find ( const char* mask, FileList* Files );

#endif
