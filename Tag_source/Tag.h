#ifndef _tag_Tag_h_
#define _tag_Tag_h_

#define ID3V2SUPPORT        // comment to disable ID3v2 reading
#define VORBISSUPPORT       // comment to disable Vorbis support
#define FLACSUPPORT         // comment to disable FLAC support

// APE Tag item names
#define APE_TAG_FIELD_TITLE             "Title"
#define APE_TAG_FIELD_SUBTITLE          "Subtitle"
#define APE_TAG_FIELD_ARTIST            "Artist"
#define APE_TAG_FIELD_ALBUM             "Album"
#define APE_TAG_FIELD_DEBUTALBUM        "Debut Album"
#define APE_TAG_FIELD_PUBLISHER         "Publisher"
#define APE_TAG_FIELD_CONDUCTOR         "Conductor"
#define APE_TAG_FIELD_COMPOSER          "Composer"
#define APE_TAG_FIELD_COMMENT           "Comment"
#define APE_TAG_FIELD_YEAR              "Year"
#define APE_TAG_FIELD_RECORDDATE        "Record Date"
#define APE_TAG_FIELD_RECORDLOCATION    "Record Location"
#define APE_TAG_FIELD_TRACK             "Track"
#define APE_TAG_FIELD_GENRE             "Genre"
#define APE_TAG_FIELD_COVER_ART_FRONT   "Cover Art (front)"
#define APE_TAG_FIELD_NOTES             "Notes"
#define APE_TAG_FIELD_LYRICS            "Lyrics"
#define APE_TAG_FIELD_COPYRIGHT         "Copyright"
#define APE_TAG_FIELD_PUBLICATIONRIGHT  "Publicationright"
#define APE_TAG_FIELD_FILE              "File"
#define APE_TAG_FIELD_MEDIA             "Media"
#define APE_TAG_FIELD_EANUPC            "EAN/UPC"
#define APE_TAG_FIELD_ISRC              "ISRC"
#define APE_TAG_FIELD_RELATED_URL       "Related"
#define APE_TAG_FIELD_ABSTRACT_URL      "Abstract"
#define APE_TAG_FIELD_BIBLIOGRAPHY_URL  "Bibliography"
#define APE_TAG_FIELD_BUY_URL           "Buy URL"
#define APE_TAG_FIELD_ARTIST_URL        "Artist URL"
#define APE_TAG_FIELD_PUBLISHER_URL     "Publisher URL"
#define APE_TAG_FIELD_FILE_URL          "File URL"
#define APE_TAG_FIELD_COPYRIGHT_URL     "Copyright URL"
#define APE_TAG_FIELD_INDEX             "Index"
#define APE_TAG_FIELD_INTROPLAY         "Introplay"
#define APE_TAG_FIELD_MJ_METADATA       "Media Jukebox Metadata"
#define APE_TAG_FIELD_DUMMY             "Dummy"

#include <stdio.h>
#include <string.h>
#include <io.h>
#include <direct.h>
#include <time.h>
#include <stdlib.h>
#include <windows.h>
#include <sys/utime.h>
#include "find.h"

enum format {
    unknown     =  0,
    m3u         =  1,
    mp1         =  2,
    mp2         =  2,
    mp3         =  2,
    musepack    =  3,
    mpegplug    =  3,
    mpc         =  3,
    ogg         =  4,
    vorbis      =  4,
    aac         =  5,
    monkey      =  6,
    ape         =  6,
    flac        =  7,
    wavpack     =  8,
    shorten     =  9,
};

enum case_t {
    Unmodified  =   0,
    Capitalize  =   1,
    Sentence    =   2,
    Lower       =   3,
    Upper       =   4
};

typedef struct {
    enum format Format;
    enum tag_t  Tag;
} SupportedTags;

extern const SupportedTags BestTagList[];

typedef struct {
    unsigned char*  Item;                   // Name of item
    unsigned char*  Value;                  // Value of item
    unsigned char*  ValueU;                 // Value coded in UTF-8
    unsigned int    Flags;                  // Flags
    size_t          ItemSize;               // Length of name
    size_t          ValueSize;              // Length of value
    size_t          ValueUSize;             // Length of UTF-8 coded value
} TagItem;

typedef struct {
    enum tag_t      TagType;                // Type of tag
    long            TagOffset;              // Tag offset from file beginning
    long            TagSize;                // Size of tag in bytes
} TagInfo;

typedef struct {
    char            Format[256];            // Format identification string
    char            Quality[32];            // Quality string (MP3: bitrate, MPC: profile)
    long            BitRate;                // Bitrate in kbps
    long            Channels;               // Number of channels
    long            SampleRate;             // Sample rate
    unsigned long   Duration;               // Total duration
} FileDetails;

typedef struct {
    TagItem*        TagItems;               // Stores tag fields
    size_t          TagItemCount;           // Number of fields in tag
    TagInfo*        Tags;                   // Stores information about tags in file
    size_t          TagCount;               // Number of tags in file
    long            TagOffset;              // Offset of last tag

    char            Filename[_MAX_PATH];    // Name of file
    long            FileSize;
    FileDetails     Details;                // Detailed information about file
} FileInfo;

typedef struct {
    char*           Field;
    int             in_utf8;
} NewTagFields;

typedef struct {
    NewTagFields*   TagFields;
    char**          Allow;                  // allow access to files with specified properties only
    char*           Scheme;                 // naming scheme ( when not auto )
    char*           RenameToScheme;         // new scheme used with renaming
    char**          SortBy;                 // sort by
    char**          ForceTags;              // force writing of tags
    char**          Defaults;               // default options from config file
    char*           TagSourceFile;          // name of file to copy tags from
    char*           OutputFileName;         // name of file to output screen output
    char*           OutputFileScheme;       // scheme to name outputfile by
    char*           OutputFileExtension;    // extension of scheme-generated outputfile
    char*           AlbumArtist;            // artist of album
    char*           AlbumTitle;             // title of album
    char*           NFOname;                // name of .nfo file
    char*           PlaylistName;           // name of playlist

    char**          Items_caps;             // list of tag items to capitalize
    char**          Items_Caps;             // list of tag items to capitalize in sentence style
    char**          Items_lower;            // list of tag items to convert lowercase
    char**          Items_upper;            // list of tag items to convert uppercase

    size_t          TagFieldCount;
    size_t          AllowCount;
    size_t          ForceTagCount;
    size_t          DefaultCount;
    size_t          SortCount;

    size_t          IcapsCount;
    size_t          ICapsCount;
    size_t          IlowerCount;
    size_t          IupperCount;
} SettingValues;

typedef struct {
    unsigned int    Recursive:1;            // recursive mode
    unsigned int    TestMode:1;             // test mode, no file will be changed
    unsigned int    HideFileInfo:1;         // don't display file information
    unsigned int    HideTags:1;             // don't display tags
    unsigned int    HideNames:1;            // don't display filename
    unsigned int    SimpleDisplay:1;        // simple tag display format
    unsigned int    NoExtensionCheck:1;     // don't check file extension
    unsigned int    Sort:1;                 // playlist sorting
    unsigned int    DescendingSort:1;       // descending sort order
    unsigned int    NoLeadingZero:1;        // no leading zero on track numbers
    unsigned int    ZeroPad:1;              // pad track number with zeros
    unsigned int    SpaceFix:1;             // replace underscore with space
    unsigned int    CommaFix:1;             // fix comma separated artist and title fields
    unsigned int    ItemFix:1;              // fix APE v1.0/2.0 item names' case use
    unsigned int    UmlautFix:1;            // fix umlauts (ae -> ä, oe -> ö, ue -> ü)
    unsigned int    SwapTitleAndArtist:1;   // swap title <--> artist
    unsigned int    Allow:1;                // allow only specified genre
    unsigned int    AutoScheme:1;           // automatic scheme selection mode
    unsigned int    Auto:1;                 // automatically generate tag from name
    unsigned int    NoMagic:1;              // disable automatic CD number and year detector
    unsigned int    FromFile:1;             // copy tags from file
    unsigned int    RenameFromTag:1;        // rename files based on tag information
    unsigned int    RenameFromName:1;       // rename files using old name as basis
    unsigned int    RenameMove:1;           // move files to directories when renaming
    unsigned int    TrackInc:1;             // increase track number after every file
    unsigned int    Remove:1;               // remove tags
    unsigned int    RemoveID3v2:1;          // remove ID3v2
    unsigned int    RemoveUnneededID3v2:1;  // remove ID3v2 if basic fields fit to ID3v1
    unsigned int    DefaultAPE2:1;          // default to APE2 with MPC
    unsigned int    OnePlaylist:1;          // generate one playlist of all files
    unsigned int    PlaylistPerDir:1;       // generate playlist in every directory
    unsigned int    PlaylistPerAlbum:1;     // generate playlist from every album to root
    unsigned int    PlaylistNoExt:1;        // plain playlists, no extended informations
    unsigned int    PlaylistUnixSlashes:1;  // use '/' in playlists instead of '\'
    unsigned int    PlaylistNameFromDir:1;  // use directory name for playlist
    unsigned int    PlaylistName:1;         // specify name of playlist
    unsigned int    ReplaceSpaces:1;        // replace space by underscore in names
    unsigned int    CharReplaceForTag:1;    // use character replacing when tagging
    unsigned int    UpdateTime:1;           // update files date/time information
    unsigned int    OldType:1;              // use old tag format
    unsigned int    Force:1;                // force tag format
    unsigned int    ID3Ext:1;               // Extend too long title field into comment field
    unsigned int    OutputToFile:1;         // print screen output to file
    unsigned int    OutputToFileExt:1;      // use defined extension (instead of .txt)
    unsigned int    NfoFile:1;              // generate .nfo file from tags
    unsigned int    Decode:1;               // decode files
    unsigned int	UnicodeOS:1;			// native support for unicode
    unsigned int    TagCaseConv:1;          // tag item based case conversion
    unsigned int    RenameOverwrite:1;      // overwrite existing files when renaming
    unsigned int    reserved:17;            // reserved bits...

    enum case_t     CaseConversion;         // case conversion

    SettingValues   _Values;

    FILE*           output;                 // file to output text to (stdout)
} GlobalSettings;

extern GlobalSettings Options;

enum format FileFormat ( const char* filename );    // Returns file format based on extension

#endif
