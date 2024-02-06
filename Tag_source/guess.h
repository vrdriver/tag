#ifndef _tag_guess_h
#define _tag_guess_h

#include "Tag.h"

typedef struct {
    char**          Schemes;
    size_t          SchemeCount;
} SchemeList;

// Gets tag information from filename
int GenerateTagFromName ( const char* filename, const char* naming_scheme, FileInfo* Info );

// Guesses tag information from filename
int GuessTag ( FileInfo* Info, SchemeList* Schemes );

// Generates name from tag
int GenerateNameFromTag ( char* filename, const char* naming_scheme, const FileInfo* Info );

// Generates path from tag
int GeneratePathFromTag ( char* path, const char* naming_scheme, const FileInfo* Info );

#endif