#ifndef _tag_tagwrite_h
#define _tag_tagwrite_h

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

int     WriteTag ( FileInfo* Info, enum tag_t tag );                // Writes specified tag
int     WriteTags ( FileInfo* Info );                               // Writes tags specified in FileInfo
int     WriteAutoTag ( FileInfo* Info );                            // Writes most suitable tag
int     RemoveTags ( FileInfo* Info );                              // Removes all tags

int     WriteID3v1Tag ( FILE* fp, FileInfo* Info );                 // Writes ID3v1.0 / ID3v1.1 tag
int     WriteLyrics3v2Tag ( FILE* fp, FileInfo* Info );             // Writes Lyrics3 v2.0 tag
int     WriteAPE1Tag ( FILE* fp, FileInfo* Info );                  // Writes APE v1.0 tag
int     WriteAPE2Tag ( FILE* fp, FileInfo* Info );                  // Writes APE v2.0 tag

#ifdef VORBISSUPPORT
int     WriteVorbisTag ( const char* filename, FileInfo* Info );    // Writes Vorbis comments
#endif  // VORBISSUPPORT

#ifdef FLACSUPPORT
int     WriteFLACTag ( const char* filename, FileInfo* Info );      // Writes FLAC tag
#endif  // FLACSUPPORT

#ifdef ID3V2SUPPORT
int     RemoveID3v2Tag ( const char* filename, FileInfo* Info, int silent ); // Removes ID3v2.x tag
#endif  // ID3V2SUPPORT

#ifdef VORBISSUPPORT
int     RemoveVorbisTag ( const char* filename, FileInfo* Info );   // Removes Vorbis comments
#endif  // VORBISSUPPORT

#ifdef FLACSUPPORT
int     RemoveFLACTag ( const char* filename, FileInfo* Info );     // Removes FLAC tag
#endif  // FLACSUPPORT

int     WriteNoTag ( FILE* fp, FileInfo* Info );                    // Removes all tags from end of file

int     CheckID3v1InfoLoss ( const FileInfo* Info );                // Checks if ID3v1 tag can store all tag information

#endif
