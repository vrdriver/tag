#ifndef _tag_playlist_h
#define _tag_playlist_h

#include "Tag.h"
#include "find.h"

typedef struct {
    char**          SortValues;             // tag values used with sorting
    char*           Display;                // displayed title ( normally: artist - title )
    char*           Artist;                 // artist
    char*           Title;                  // title
    char*           Album;                  // album
    char*           Comment;                // comment
    unsigned long   Duration;               // duration in seconds
    const FileListEntry*    Info;           // pointer to Files->Files[pos]
} DetailsOfFile;

typedef struct {
    DetailsOfFile*  Files;
    size_t          FileCount;
    size_t          SortValueCount;
    char            Artist[1024];
    char            Album [1024];
} DetailedList;

// Adds necessary information to DetailedList structure
int AddToDetailedList ( const FileInfo* File, const FileListEntry* FileListEntry, DetailedList* Details );

// Write multiple playlists, one per directory
int WritePlaylistPerDir ( const char* playlistfile,  const DetailedList* Details );

// Write one big playlist of all files
int WriteOnePlaylist ( const char* playlistfile,  const DetailedList* Details );

// Write one playlist per every album to current working directory
int WritePlaylistPerAlbum ( const char* playlistfile,  const DetailedList* Details );

#endif
