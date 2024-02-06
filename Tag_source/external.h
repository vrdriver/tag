#ifndef _tag_external_h
#define _tag_external_h

#include "Tag.h"
#include "misc.h"
#include "tags.h"

typedef struct {
    char*           Title;      // Track title
    char*           Artist;     // Track artist
    char*           Year;       // Track year
    char*           Genre;      // Track genre
    char*           Track;      // Track number
    char*           Comment;    // Track comment
} ExternalTrack;

typedef struct {
    ExternalTrack*  Tracks;     // Album track information
    size_t          TrackCount; // Number of tracks

    char*           Artist;     // Album artist
    char*           Album;      // Album name
    char*           Year;       // Album Year
    char*           Genre;      // Album Genre
    char*           Comment;    // Album comment (unneeded?)
} ExternalAlbum;

void    FreeExternalAlbum ( ExternalAlbum* Album );                     // Free memory allocated by ExternalAlbum
int     LoadCDDBFile ( const char* filename, ExternalAlbum* Album );    // Load tag information from CDDB-file
int     LoadTagIni   ( const char* filename, ExternalAlbum* Album );    // Load tag information from file "Tag.ini"

#endif
