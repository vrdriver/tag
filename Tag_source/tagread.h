#ifndef _tag_tagread_h
#define _tag_tagread_h

#include <stdio.h>
#include <string.h>
#include <io.h>
#include <stdlib.h>
#include <windows.h>
#include "Tag.h"
#include "tags.h"
#include "misc.h"
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

int ReadTags         ( FileInfo* Info );                        // Reads all supported tags from file

int ReadID3v1Tag     ( FILE* fp, FileInfo* Info );              // Reads ID3v1.0 / ID3v1.1 tag
int ReadLyrics3v2Tag ( FILE* fp, FileInfo* Info );              // Reads Lyrics3 v2.0 tag
int ReadAPE1Tag      ( FILE* fp, FileInfo* Info );              // Reads APE v1.0 tag
int ReadAPE2Tag      ( FILE* fp, FileInfo* Info );              // Reads APE v2.0 tag

#ifdef ID3V2SUPPORT
int ReadID3v2Tag     ( const char* filename, FileInfo* Info );  // Reads ID3v2.x tag
#endif  // ID3V2SUPPORT
#ifdef VORBISSUPPORT
int ReadVorbisTag    ( const char* filename, FileInfo* Info );  // Reads Vorbis comments
#endif  // VORBISSUPPORT
#ifdef      FLACSUPPORT
int ReadFLACTag      ( const char* filename, FileInfo* Info );  // Reads FLAC tags
#endif  // FLACSUPPORT

#endif
