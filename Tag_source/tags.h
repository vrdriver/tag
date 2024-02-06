#ifndef _tag_tags_h_
#define _tag_tags_h_

#include <stdio.h>
#include <string.h>
#include <io.h>
#include <stdlib.h>
#include <windows.h>
#include "Tag.h"
#include "tagread.h"
#include "tagwrite.h"
#ifdef      VORBISSUPPORT
#include "vcedit.h"
#include <vorbis/vorbisfile.h>
#endif  // VORBISSUPPORT
#ifdef      ID3V2SUPPORT
#include <id3.h>
#endif  // ID3V2SUPPORT
#ifdef      FLACSUPPORT
#include <FLAC/metadata.h>
#endif  // FLACSUPPORT

extern const char* ID3v1GenreList [];   // holds the genres available for ID3v1
#define ID3V1GENRES                     148
extern const char* TagNames[];          // names of tags
#define TAGNAMES                        8

#define ListSeparator   "; "

struct APETagFooterStruct {
    unsigned char   ID       [8];
    unsigned char   Version  [4];
    unsigned char   Length   [4];
    unsigned char   TagCount [4];
    unsigned char   Flags    [4];
    unsigned char   Reserved [8];
};

struct Lyrics3TagFooterStruct {
    unsigned char   Length  [6];
    unsigned char   ID      [9];
};

struct Lyrics3TagField {
	unsigned char   ID      [3];
	unsigned char   Length  [5];
};

enum tag_t {
    auto_tag    =  -1,
    no_tag      =   0,
    ID3v1_tag   =   1,
    ID3v2_tag   =   2,
    Lyrics3_tag =   3,
    APE1_tag    =   4,
    APE2_tag    =   5,
    Vorbis_tag  =   6,
    FLAC_tag    =   7,
    guessed_tag = 255,
};

void    FreeTagFields ( FileInfo* Info );                                                   // Free all tag fields from memory

size_t  TagItemNum ( const char* item, const FileInfo* Info );                              // Return position of first specified item
size_t  TagItemCount ( const char* item, const FileInfo* Info );                            // Return number of identical item names
unsigned int TagItemFlags ( const char* item, const FileInfo* Info );                       // Return flags of first specified item

size_t  CombineTagValues   ( char* dest, const char* item, const FileInfo* Info, const char* separator ); // Combines multiple tag values to one ( ITEMx=ab, ITEMx=cd -> dest = ab+cd )
size_t  CombineTagValuesU  ( char* dest, const char* item, const FileInfo* Info, const char* separator ); // Combines multiple UTF-8 tag values to one ( ITEMx=ab, ITEMx=cd -> dest = ab+cd )
size_t  CombineTagValuesUB ( char* dest, const char* item, const FileInfo* Info, const char* separator ); // Combines multiple UTF-8 tag values to one. If UTF-8 is empty uses normal value ( ITEMx=ab, ITEMx=cd -> dest = ab+cd )

int     CopyTagValue   ( char* dest, const char* item, const FileInfo* Info, size_t count ); // Copy value from tag field to dest, returns length
int     CopyTagValueU  ( char* dest, const char* item, const FileInfo* Info, size_t count ); // Copy UTF-8 value from tag field to dest, returns length
int     CopyTagValueUB ( char* dest, const char* item, const FileInfo* Info, size_t count ); // Copy UTF-8 value (or normal when UTF-8 is missing) from tag field to dest, returns length

char*   TagValue   ( const char* item, const FileInfo* Info );                              // Return pointer to value in tag field
char*   TagValueU  ( const char* item, const FileInfo* Info );                              // Return pointer to UFT-8 value in tag field
char*   TagValueUB ( const char* item, const FileInfo* Info );                              // Return pointer to UFT-8 value (or to normal value if UTF-8 is empty) in tag field

int     InsertTagField  ( const char* item, size_t itemsize, const char* value, size_t valuesize, unsigned int flags, FileInfo* Info, int append, int only_if_longer, int occurrence );
int     InsertTagFieldU ( const char* item, size_t itemsize, const char* value, size_t valuesize, unsigned int flags, FileInfo* Info, int append, int only_if_longer, int occurrence );
int     InsertTagFieldB ( const char* item, size_t itemsize, const char* value, size_t valuesize, unsigned int flags, FileInfo* Info, int append, int only_if_longer, int occurrence );

int     ReplaceTagField  ( const size_t itemnum, const char* value, size_t valuesize, unsigned int flags, FileInfo* Info );
int     ReplaceTagFieldU ( const size_t itemnum, const char* value, size_t valuesize, unsigned int flags, FileInfo* Info );
int     ReplaceTagFieldB ( const size_t itemnum, const char* value, size_t valuesize, unsigned int flags, FileInfo* Info );

int     ReplaceListSeparator ( const char* old_sep, const char* new_sep, FileInfo* Info, size_t itemnum );

int     GenreToInteger ( const char* GenreStr);
int     GenreToString  ( char* GenreStr, const int genre );

void    memcpy_crop ( char* dst, const char* src, size_t len );
unsigned long Read_LE_Uint32 ( const unsigned char* p );
void    Write_LE_Uint32 ( unsigned char* p, const unsigned long value );
int     Lyrics3GetNumber5 ( const unsigned char* string );
int     Lyrics3GetNumber6 ( const unsigned char* string );

int unicodeToUtf8 ( const WCHAR* lpWideCharStr, char* lpMultiByteStr, int cwcChars );
int utf8ToUnicode ( const char* lpMultiByteStr, WCHAR* lpWideCharStr, int cmbChars );

#endif
