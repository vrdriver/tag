// -----------------------------------------------------------------------------
// Tag - Automatic Tag from filename                        (C) Case 2002 - 2003
// Supported tags:
// ID3v1, ID3v2 (reading and removing), APEv1, APEv2, Vorbis, FLAC, Lyrics3 v2.0
//
// To compile with full functionality, Vorbis SDK (http://www.vorbis.com),
// libFLAC (http://flac.sourceforge.net/) and 
// id3lib (http://id3lib.sourceforge.net/) are required.
//
// If you want to compile without some of the above libraries, comment out the
// unwanted functionality from the beginning of Tag.h
// -----------------------------------------------------------------------------

#include "Tag.h"
#include "tags.h"
#include "misc.h"
#include "guess.h"
#include "find.h"
#include "sort.h"
#include "playlist.h"
#include "external.h"
#include "nfofile.h"
//#include "errno.h"
#include "mp3.h"
#include "mpc.h"
#include "ogg.h"
#include "flac.h"
#include "ape.h"
#include <process.h>
#include <locale.h>

#define TNAME				"Tag"
#define TVERSION			"2.0.38"
#define TDATE				"2003-03-01"
#define TCOPYRIGHT			"Copyright (c) 2002-2003 Case"
#define TDESCRIPTION		"Automatic Tag from filename"

const SupportedTags BestTagList[] = {
    mp3,        Lyrics3_tag,
    musepack,   APE1_tag,               // switch --ape2 defaults to APE v2.0 tag
    vorbis,     Vorbis_tag,
    monkey,     APE1_tag,
    aac,        ID3v1_tag,
#ifdef FLACSUPPORT
    flac,       FLAC_tag,
#else
    flac,       ID3v1_tag,
#endif
    wavpack,    ID3v1_tag,
    -1, -1
};

GlobalSettings Options;

typedef struct {
    char**          Presets;
    size_t          PresetsCount;
} PresetsList;

PresetsList Presets;
ExceptionList Exceptions;
CharReplaceList Replace;
CharReplaceForTag  ReplaceTag;
NFOfileTemplate NFOfile;

// -----------------------------------------------------------------------------

// Free all memory allocated by File
void free_FileInfo ( FileInfo* File )
{
    FreeTagFields ( File );
}

// Free all memory allocated by Files
void free_FileList ( FileList* Files )
{
    free ( Files->Files );
}

// Free all memory allocated by Schemes
void free_SchemeList ( SchemeList* Schemes )
{
    size_t i;
    for ( i = 0; i < Schemes->SchemeCount; i++ ) {
        free ( Schemes->Schemes[i] );
    }
    free ( Schemes->Schemes );
}

// Free all memory allocated by Presets
void free_PresetsList ( PresetsList* Presets )
{
    size_t i;
    for ( i = 0; i < Presets->PresetsCount; i++ ) {
        free ( Presets->Presets[i] );
    }
    free ( Presets->Presets );
}

// Free all memory allocated by Exceptions
void free_ExceptionList ( ExceptionList* Exceptions )
{
    size_t i;
    for ( i = 0; i < Exceptions->WordCount; i++ ) {
        free ( Exceptions->Words[i] );
    }
    free ( Exceptions->Words );
}

// Free all memory allocated by Replace
void free_CharReplaceList ( CharReplaceList* Replace )
{
    size_t i;
    for ( i = 0; i < Replace->CharCount; i++ ) {
        free ( Replace->NewChars[i] );
    }
    free ( Replace->NewChars );
    free ( Replace->OldChars );
}

// Free all memory allocated by ReplaceTag
void free_CharReplaceForTag ( CharReplaceForTag* ReplaceTag )
{
    size_t i;
    for ( i = 0; i < ReplaceTag->CharCount; i++ ) {
        free ( ReplaceTag->OldChars[i] );
    }
    free ( ReplaceTag->NewChars );
    free ( ReplaceTag->OldChars );
}

// Free all memory allocated by NFOfile
void free_NFOfileTemplate ( NFOfileTemplate* NFOfile )
{
    size_t i;
    for ( i = 0; i < NFOfile->LineCount; i++ ) {
        free ( NFOfile->Lines[i] );
    }
    free ( NFOfile->Lines );
}

// Free all memory allocated by Details
void free_DetailedList ( DetailedList* Details )
{
    size_t i, j;
    for ( i = 0; i < Details->FileCount; i++ ) {
        for ( j = 0; j < Details->SortValueCount; j++ ) {
            free ( Details->Files[i].SortValues[j] );
        }
        if ( Details->SortValueCount > 0 ) {
            free ( Details->Files[i].SortValues );
        }
        free ( Details->Files[i].Display );
    }
    free ( Details->Files  );
}

// Free all memory allocated by External
void free_ExternalAlbum ( ExternalAlbum* External )
{
    FreeExternalAlbum ( External );
}

// -----------------------------------------------------------------------------

// Display program information
void print_header ()
{
    fprintf ( stderr, TNAME " - " TDESCRIPTION "      " TCOPYRIGHT "\n" );
    fprintf ( stderr, "Version " TVERSION ", Compiled " TDATE "\n\n" );
}

// Returns file format based on extension
enum format FileFormat ( const char* filename )
{
    char ext     [_MAX_EXT];

    _splitpath( filename, NULL, NULL, NULL, ext );

    if ( stricmp ( ext, ".m3u"  ) == 0 ) return m3u;
    if ( stricmp ( ext, ".mp3"  ) == 0 ) return mp3;
    if ( stricmp ( ext, ".mp2"  ) == 0 ) return mp2;
    if ( stricmp ( ext, ".mp1"  ) == 0 ) return mp1;
    if ( stricmp ( ext, ".mpc"  ) == 0 ) return musepack;
    if ( stricmp ( ext, ".mp+"  ) == 0 ) return musepack;
    if ( stricmp ( ext, ".mpp"  ) == 0 ) return musepack;
    if ( stricmp ( ext, ".ogg"  ) == 0 ) return ogg;
    if ( stricmp ( ext, ".aac"  ) == 0 ) return aac;
    if ( stricmp ( ext, ".mac"  ) == 0 ) return monkey;
    if ( stricmp ( ext, ".ape"  ) == 0 ) return ape;
    if ( stricmp ( ext, ".flac" ) == 0 ) return flac;
    if ( stricmp ( ext, ".fla"  ) == 0 ) return flac;
    if ( stricmp ( ext, ".wv"   ) == 0 ) return wavpack;
    if ( stricmp ( ext, ".shn"  ) == 0 ) return shorten;

    return unknown;
}

// Print list of ID3v1 genres
void list_genres ()
{
    size_t i;
    printf ( "ID3v1 genres:\n" );
    for ( i = 0; i < ID3V1GENRES; i++ ) {
        printf ( "%3u: %s\n", i, ID3v1GenreList[i] );
    }
    printf ( "255: (none)\n" );
}

// List presets
void list_presets ()
{
    size_t i;
    printf ( "Presets:\n" );
    for ( i = 0; i < Presets.PresetsCount; i++ ) {
        printf ( "%3u: %s\n", i+1, Presets.Presets[i] );
    }
}

// List default settings
void list_defaults ()
{
    size_t i;
    printf ( "Default settings:\n" );
    for ( i = 0; i < Options._Values.DefaultCount; i++ ) {
        printf ( "%s", Options._Values.Defaults[i] );
        if ( i + 1 < Options._Values.DefaultCount && Options._Values.Defaults[i+1][0] != '-' ) {
            printf ( " " );
        } else {
            printf ( "\n" );
        }
    }
}

// List exceptions
void list_exceptions ()
{
    size_t i;
    printf ( "Exceptions:\n" );
    for ( i = 0; i < Exceptions.WordCount; i++ ) {
        printf ( "%3u: %s\n", i, Exceptions.Words[i] );
    }
}

// Show tags
int PrintTag ( FileInfo* Info )
{
    const char* basicfields[] = {
        "Title:   ", APE_TAG_FIELD_TITLE,
        "Artist:  ", APE_TAG_FIELD_ARTIST,
        "Album:   ", APE_TAG_FIELD_ALBUM,
        "Year:    ", APE_TAG_FIELD_YEAR,
        "Track:   ", APE_TAG_FIELD_TRACK,
        "Genre:   ", APE_TAG_FIELD_GENRE,
        "Comment: ", APE_TAG_FIELD_COMMENT
    };

    size_t  displayed[(sizeof (basicfields) / sizeof (*basicfields)) / 2];
    char*   ascii;
    size_t  i, j;
    size_t  maxlen = 0;

    for ( i = 0; i < Info->TagItemCount; i++ ) {
        if ( Info->TagItems[i].ValueSize > maxlen )
            maxlen = Info->TagItems[i].ValueSize;
    }
    if ( maxlen == 0 )
        return 0;

    if ( (ascii = malloc ( (maxlen + 1) * 2 )) == NULL ) {
        fprintf ( Options.output, "PrintTag: Memory allocation failed.\n" );
        exit (1);
    }

    if ( Options.SimpleDisplay ) {
        for ( i = 0; i < Info->TagItemCount; i++ ) {
            string_l2a ( Info->TagItems[i].Value, ascii );
            fprintf ( Options.output, "%s=%s\n", Info->TagItems[i].Item, ascii );
        }
    } else {
        memset ( &displayed, 0, sizeof (displayed) );

        for ( i = 0; i < (sizeof (basicfields) / sizeof (*basicfields)) / 2; i++ ) {
            int found = 0;

            for ( j = 0; j < Info->TagItemCount; j++ ) {
                if ( stricmp ( Info->TagItems[j].Item, basicfields[i*2+1] ) == 0 ) {
                    found = 1;
                    displayed[i] = j+1;
                    if ( !(Info->TagItems[j].Flags & 1<<1) ) {
                        string_l2a ( Info->TagItems[j].Value, ascii );
                        fprintf ( Options.output, "%s%s\n", basicfields[i*2], ascii );
                    } else {
                        fprintf ( Options.output, "%s(binary)\n", basicfields[i*2] );
                    }
                    break;
                }
            }

            if ( !found ) {
                fprintf ( Options.output, "%s\n", basicfields[i*2] );
            }
        }

        for ( i = 0; i < Info->TagItemCount; i++ ) {
            int show_field = 1;
            for ( j = 0; j < (sizeof (basicfields) / sizeof (*basicfields)) / 2; j++ ) {
                if ( displayed[j] == i+1 ) {
                    show_field = 0;
                    break;
                }
            }
            if ( show_field ) {
                if ( !(Info->TagItems[i].Flags & 1<<1) ) {
                    string_l2a ( Info->TagItems[i].Value, ascii );
                    fprintf ( Options.output, "%s=%s\n", Info->TagItems[i].Item, ascii );
                } else {
                    fprintf ( Options.output, "%s=(binary)\n", Info->TagItems[i].Item );
                }
            }
        }
    }

    free ( ascii );

    return 0;
}

// View file information
int ViewFileInfo ( FileInfo* Info )
{
    size_t  i;

    if ( Info->Details.Format[0] != '\0' ) {
        fprintf ( Options.output, "Format:  %s\n", Info->Details.Format );
    }
    if ( Info->Details.SampleRate || Info->Details.Channels || Info->Details.BitRate || Info->Details.Duration ) {
        fprintf ( Options.output, "Details: " );
        if ( Info->Details.SampleRate ) {
            fprintf ( Options.output, "%u Hz", Info->Details.SampleRate );
        }
        if ( Info->Details.Channels == 2 ) {
            fprintf ( Options.output, " Stereo" );
        } else
        if ( Info->Details.Channels == 1 ) {
            fprintf ( Options.output, " Mono" );
        } else
        if ( Info->Details.Channels != 0 ) {
            fprintf ( Options.output, " %u", Info->Details.Channels );
        }
        if ( Info->Details.BitRate ) {
            fprintf ( Options.output, ", %u kbps", Info->Details.BitRate );
        }
        if ( Info->Details.Duration ) {
            int hrs = (Info->Details.Duration / (60 * 60));
            int min = (Info->Details.Duration / 60) % 60;
            int sec = (Info->Details.Duration) % 60;
            if ( hrs ) {
                fprintf ( Options.output, ", playtime %02u:%02u:%02u", hrs, min, sec );
            } else {
                fprintf ( Options.output, ", playtime %02u:%02u", min, sec );
            }
        }
        fprintf ( Options.output, "\n" );
    }
    if ( Info->TagCount == 0 ) {
        fprintf ( Options.output, "File has no known tags." );
    } else
    if ( Info->TagCount == 1 ) {
        fprintf ( Options.output, "Tag:     " );
    } else {
        fprintf ( Options.output, "Tags:    " );
    }
    for ( i = 0; i < Info->TagCount; i++ ) {
        if ( Info->Tags[i].TagType >= 0 && Info->Tags[i].TagType < TAGNAMES ) {
            fprintf ( Options.output, "%s", TagNames[Info->Tags[i].TagType] );
        } else
        if ( Info->Tags[i].TagType == guessed_tag ) {
            fprintf ( Options.output, "Generated from name" );
        } else {
            fprintf ( Options.output, "unknown" );
        }
        if ( i+1 < Info->TagCount ) fprintf ( Options.output, ", " );
    }
    fprintf ( Options.output, "\n" );

    return 0;
}

// Read basic information about file
int ReadFileInfo ( FileInfo* Info )
{
    memset ( &Info->Details, 0, sizeof (Info->Details) );

    switch ( FileFormat ( Info->Filename ) ) {
    case mp3:       // MPEG-1 Layer 3
        return ReadFileInfoMP3 ( Info );

    case musepack:  // Musepack
        return ReadFileInfoMPC ( Info );

    case monkey:    // Monkey's Audio
        return ReadFileInfoAPE ( Info );

#ifdef  VORBISSUPPORT
    case vorbis:    // Ogg Vorbis
        return ReadFileInfoVorbis ( Info );
#endif  // VORBISSUPPORT

#ifdef  FLACSUPPORT
    case flac:      // FLAC
        return ReadFileInfoFLAC ( Info );
#endif  // FLACSUPPORT

    case aac:       // AAC - Advanced Audio Coding
    case wavpack:   // WavPack
    case unknown:   // unknown
    default:
        return 0;
    }
}

// Prints usage information on screen
void print_usage ( int longhelp )
{
    char temp[128];

    {
        printf ( "Usage: Tag [options] <filenames / directories / @file_with_parameters>\n"
            "Wildcards are supported in filenames and directories\n"
            "\n"
            "Options:\n"
            " --title   <value> : set title to <value>\n"                                   // done
            " --artist  <value> : set artist to <value>\n"                                  // done
            " --album   <value> : set album to <value>\n"                                   // done
            " --year    <value> : set year to <value>\n"                                    // done
            " --comment <value> : set comment to <value>\n"                                 // done
            " --track   <value> : set track number to <value>\n"                            // done
            " --genre   <value> : set genre to <value>\n"                                   // done
            "\n"
            "Tag fields can also be set with:\n"
            " -t <item=x>       : set tag item <item> to value <x>\n" );                    // done
    }
    if ( !Options.UnicodeOS ) {
        printf (
            " -u <item=x>       : set tag item <item> to value <x> (x is UTF-8 coded)\n" ); // done
    }
    {
        printf (
            "\n"
            " --help            : show full help\n"                                         // done
        );
    }

    if ( longhelp ) {
        printf (
            " --hideinfo        : don't display file information\n"                         // done
            " --hidetags        : don't display tags\n"                                     // done
            " --hidenames       : don't display filenames\n"                                // done
            " --simple          : use simple tag display format\n"                          // done
            " --recursive       : search files recursively in all subdirectories\n"         // done
            " --fromfile <file> : copy tag information from <file>\n"                       // done
            " --allow <item=x>  : allow access to files with specified properties only\n"   // done
            " --auto            : generate tags from filenames\n"                           // done
            " --scheme <scheme> : specify file structure scheme (i.e. L/A - N - T)\n"       // done
            " --autoscheme      : select automatically best scheme from list of schemes\n"  // done
            " --nomagic         : use plain schemes, disable extra automations\n"           // done
            " --chreplace       : use character replacing when tagging\n"                   // done
            " --rentag          : rename files to match scheme, filename from tag data\n"   // done
            " --rename <scheme> : rename files to <scheme>, filename generated from old name\n" // done
            " --move            : when renaming move files to subdirectories\n"             // done
            " --overwrite       : overwrite existing files when renaming\n"                 // done
            " --trackinc        : use incremental track numbering\n"                        // done
            " --caps            : capitalize words\n"                                       // done
            " --Caps            : capitalize only first word\n"                             // done
            " --lower           : convert all characters to lower case\n"                   // done
            " --upper           : convert all characters to upper case\n"                   // done
            " --tcaps <item>    : capitalize words in tag field <item>\n"                   // done
            " --tCaps <item>    : capitalize only first word in tag field <item>\n"         // done
            " --tlower <item>   : convert all characters to lower case in tag field <item>\n" // done
            " --tupper <item>   : convert all characters to upper case in tag field <item>\n" // done
            " --nozero          : ignore leading zeros on track numbers\n"                  // done
            " --zeropad         : add leading zero to track number if missing\n"            // done
            " --commafix        : fix comma separated artist field (x, the -> the x)\n"     // done
            " --spacefix        : replaces underscore (_) and %%20 with space\n"            // done
            " --itemfix         : fix item names in APE v1.0/2.0 tags\n"                    // done
        );

        string_l2a ( " --umlfix          : fix umlauts (ae -> ä, oe -> ö, ue -> ü)\n", temp ); // done
        printf ( "%s", temp );

        printf (
            " --swapta          : swap title <--> artist\n"                                 // done
            " --remove          : remove all tags\n"                                        // done
#ifdef  ID3V2SUPPORT
            " --removeid3v2     : remove only ID3v2 tags\n"                                 // done
            " --removeid3v2u    : remove only unnecessary ID3v2 tags (fields fit in ID3v1)\n" // done
#endif  // ID3V2SUPPORT
            " --playlist        : generate one playlist per directory\n"                    // done
            " --oneplaylist     : generate one playlist from all files\n"                   // done
            " --playlists       : generate one playlist per album in current directory\n"   // done
            " --a-artist <x>    : set album artist to <x>\n"                                // done
            " --a-title <x>     : set album title to <x>\n"                                 // done
            " --onlyfiles       : writes only filenames in playlist, no extra information\n"// done
            " --dirname         : use directory name for playlist naming\n"                 // done
            " --plname <name>   : use <name> as playlist name.\n"                           // done
            " --slashes         : use slashes '/' instead of backslashes '\\' in playlists\n" // done
            " --nospaces        : replaces space by underscore in names of written files\n" // done
            " --sort <by>       : sort playlist only by <x>\n"                              // done
            " --sort+ <x>       : add new playlist sorting property <x>\n"                  // done
            " --sortdesc        : sort in descending order\n"                               // done
            " --newdate         : don't keep files original date/time\n"                    // done
            " --oldtype         : use old tag format\n"                                     // done
            " --ape2            : default to APE v2.0 with MPC\n"                           // done
            " --force <tag>     : force use of selected tag type\n"                         // done
            " --extid3          : extend over long title field into comment field with id3\n" // done
            " --nocheck         : don't check file extension\n"                             // done
            " --test            : test mode, no files will be modified\n"                   // done
            " --tofile <scheme> : save screen output to file, name generated from <scheme>\n" // done
            " --tofileext <ext> : use extension <ext> instead of .txt\n"                    // done
            " --tofilen <name>  : save screen output to file <name>\n"                      // done
            " --nfo <file>      : generate .nfo file and save to <file>\n"                  // done
            " --listdefaults    : view default settings\n"                                  // done
            " --listexceptions  : view list of exceptions in capitalize function\n"         // done
            " --listgenres      : view list of possible ID3v1 tag genres\n"                 // done
            " --listpresets     : view list of defined presets\n"                           // done
            " --decode          : use external decoder and use <scheme> to name output\n"   // done
            " --<number>        : use predefined preset <number>\n"                         // done

            //" --lognames <file> : save new names in <file>\n"
            //" --m3u             : add AlbumList extensions to m3u\n"
            //" --inimode         : reads tag information from tag.ini\n"
            //" --multitag        : no removal of old tag\n"
        );
    }

    printf (
            "\n"
#if defined ID3V2SUPPORT && defined VORBISSUPPORT && defined FLACSUPPORT
            "Supported tags: ID3v1, ID3v2 (reading), APE v1.0, APE v2.0, Vorbis, FLAC, Lyrics3 v2.0.\n"
#endif
#if !defined ID3V2SUPPORT && defined VORBISSUPPORT && defined FLACSUPPORT
            "Supported tags: ID3v1, APE v1.0, APE v2.0, Vorbis, FLAC, Lyrics3 v2.0.\n"
#endif
#if defined ID3V2SUPPORT && !defined VORBISSUPPORT && defined FLACSUPPORT
            "Supported tags: ID3v1, ID3v2 (reading), APE v1.0, APE v2.0, FLAC, Lyrics3 v2.0.\n"
#endif
#if defined ID3V2SUPPORT && defined VORBISSUPPORT && !defined FLACSUPPORT
            "Supported tags: ID3v1, ID3v2 (reading), APE v1.0, APE v2.0, Vorbis, Lyrics3 v2.0.\n"
#endif
#if !defined ID3V2SUPPORT && !defined VORBISSUPPORT && defined FLACSUPPORT
            "Supported tags: ID3v1, APE v1.0, APE v2.0, FLAC, Lyrics3 v2.0.\n"
#endif
#if !defined ID3V2SUPPORT && defined VORBISSUPPORT && !defined FLACSUPPORT
            "Supported tags: ID3v1, APE v1.0, APE v2.0, Vorbis, Lyrics3 v2.0.\n"
#endif
#if defined ID3V2SUPPORT && !defined VORBISSUPPORT && !defined FLACSUPPORT
            "Supported tags: ID3v1, ID3v2 (reading), APE v1.0, APE v2.0, Lyrics3 v2.0.\n"
#endif
#if !defined ID3V2SUPPORT && !defined VORBISSUPPORT && !defined FLACSUPPORT
            "Supported tags: ID3v1, APE v1.0, APE v2.0, Lyrics3 v2.0.\n"
#endif
            "\n" );
}

// Get all parameters ( options + files )
int get_parameters ( size_t argc, char** orig_argv, FileList* Files )
{
    char**  new_argv;
    size_t  i;
    size_t  console_argc = argc + Options._Values.DefaultCount;    // number of arguments from command line
    int     errors = 0;

    if ( Options._Values.DefaultCount > 0 ) {       // config file contains defaults
        if ( (new_argv = (char **)malloc ( sizeof (char *) * (argc + Options._Values.DefaultCount) )) == NULL ) {
            fprintf ( stderr, "get_parameters: Memory allocation failed.\n" );
            exit (1);
        }
        new_argv[0] = orig_argv[0];
        for ( i = 0; i < Options._Values.DefaultCount; i++ ) {
            new_argv[i + 1] = Options._Values.Defaults[i];
        }
        for ( i = 1; i < argc; i++ ) {
            new_argv[i + Options._Values.DefaultCount] = orig_argv[i];
        }
        argc += Options._Values.DefaultCount;
    } else {                                        // no defaults
        if ( (new_argv = (char **)malloc ( sizeof (char *) * argc )) == NULL ) {
            fprintf ( stderr, "get_parameters: Memory allocation failed.\n" );
            exit (1);
        }
        for ( i = 0; i < argc; i++ ) {
            new_argv[i] = orig_argv[i];
        }
    }

    // parse presets
    for ( i = 1; i < argc; i++ ) {
        if ( new_argv[i] == NULL ) continue;

        if ( strncmp ( new_argv[i], "--", 2 ) == 0 ) {
            char* param = (char *)(new_argv[i] + 2);
            if ( *param >= '0' && *param <= '9' ) {
                size_t num = atoi (param);
                if ( num > 0 && num <= Presets.PresetsCount && Presets.Presets[num-1] != NULL ) {
                    char   word [4096];
                    size_t wpos     = 0;
                    size_t newargc  = argc;
                    while ( get_word ( word, Presets.Presets[num-1], &wpos ) > 0 ) {
                        if ( (new_argv = (char **)realloc ( new_argv, sizeof (char *) * (newargc + 1) )) == NULL ) {
                            fprintf ( stderr, "get_parameters: Memory allocation failed.\n" );
                            exit (1);
                        }
                        if ( (new_argv[newargc] = malloc ( strlen (word) + 1 )) == NULL ) {
                            fprintf ( stderr, "get_parameters: Memory allocation failed.\n" );
                            exit (1);
                        }
                        strcpy ( new_argv[newargc], word );
                        newargc++;
                    }
                    new_argv[i] = NULL;
                    argc = newargc;
                }
            }
        }
    }

    // read parameters from files
    for ( i = 1; i < argc; i++ ) {
        if ( new_argv[i] == NULL ) continue;

        if ( new_argv[i][0] == '@' ) {
            FILE* fp;
            char* filename = (char *)(new_argv[i] + 1);

            if ( (fp = fopen ( filename, "rb" )) != NULL ) {
                char    temp [4096];
                size_t  newargc = argc;

                while ( fgets_clean ( temp, sizeof (temp), fp ) != EOF ) {
                    if ( temp[0] == '\0' ) continue;

                    if ( temp[0] == '-' || temp[0] == '"' ) {
                        char   word [4096];
                        size_t wpos = 0;

                        while ( get_word ( word, temp, &wpos ) > 0 ) {
                            if ( (new_argv = (char **)realloc ( new_argv, sizeof (char *) * (newargc + 1) )) == NULL ) {
                                fprintf ( stderr, "get_parameters: Memory allocation failed.\n" );
                                exit (1);
                            }
                            if ( (new_argv[newargc] = (char *)malloc ( strlen (word) + 1 )) == NULL ) {
                                fprintf ( stderr, "get_parameters: Memory allocation failed.\n" );
                                exit (1);
                            }
                            strcpy ( new_argv[newargc], word );
                            newargc++;
                        }
                    } else {
                        if ( (new_argv = (char **)realloc ( new_argv, sizeof (char *) * (newargc + 1) )) == NULL ) {
                            fprintf ( stderr, "get_parameters: Memory allocation failed.\n" );
                            exit (1);
                        }
                        if ( (new_argv[newargc] = (char *)malloc ( strlen (temp) + 1 )) == NULL ) {
                            fprintf ( stderr, "get_parameters: Memory allocation failed.\n" );
                            exit (1);
                        }
                        strcpy ( new_argv[newargc], temp );
                        newargc++;
                    }
                }

                fclose (fp);
                new_argv[i] = NULL;
                argc = newargc;
            }
        }
    }

    // check options
    for ( i = 1; i < argc; i++ ) {
        if ( new_argv[i] == NULL ) continue;

        if ( strncmp ( new_argv[i], "--", 2 ) != 0 && strncmp ( new_argv[i], "-",  1 ) == 0 ) { // parameters starting with '-'
            char*  param = (char *)(new_argv[i] + 1);
            int    process = 1;
            size_t orig_i  = i;

            if ( 0 == strcmp ( param, "t" ) ||
                 0 == strcmp ( param, "u" ) ) {
                if ( ++i < argc && new_argv[i][0] != '-'  ) {
                    if ( (Options._Values.TagFields = (NewTagFields *)realloc ( Options._Values.TagFields, sizeof (*Options._Values.TagFields) * (Options._Values.TagFieldCount + 1) )) == NULL ) {
                        fprintf ( stderr, "get_parameters: Memory allocation failed.\n" );
                        exit (1);
                    }
                    if ( (Options._Values.TagFields[Options._Values.TagFieldCount].Field = malloc ( strlen (new_argv[i]) + 1 )) == NULL ) {
                        fprintf ( stderr, "get_parameters: Memory allocation failed.\n" );
                        exit (1);
                    }
                    strcpy ( Options._Values.TagFields[Options._Values.TagFieldCount].Field, new_argv[i] );
                    Options._Values.TagFields[Options._Values.TagFieldCount].in_utf8 = (param[0] == 'u' || (Options.UnicodeOS && i < console_argc));
                    Options._Values.TagFieldCount++;
                } else {
                    fprintf ( stderr, "%s: required argument <item=x> missing.\n", param );
                    exit (1);
                }
            } else
            {
                if ( !isfile (new_argv[i]) ) {
                    fprintf ( stderr, "Invalid parameter: %s\n", param );
                    exit (1);
                }
                process = 0;
            }

            if ( process ) {
                new_argv[orig_i] = NULL;
                new_argv[i]      = NULL;
            }
        } else
        if ( strncmp ( new_argv[i], "--", 2 ) == 0 ) {                                      // parameters starting with '--'
            char*  param = (char *)(new_argv[i] + 2);
            int    process = 1;
            size_t orig_i  = i;

            if ( 0 == strcmp ( param, "help" ) ) {
                print_usage ( 1 );
                exit (0);
            } else
            if ( 0 == strcmp ( param, "listdefaults" ) ) {
                list_defaults ();
                exit (0);
            } else
            if ( 0 == strcmp ( param, "listgenres" ) ) {
                list_genres ();
                exit (0);
            } else
            if ( 0 == strcmp ( param, "listpresets" ) ) {
                list_presets ();
                exit (0);
            } else
            if ( 0 == strcmp ( param, "listexceptions" ) ) {
                list_exceptions ();
                exit (0);
            } else
            if ( 0 == strcmp ( param, "title"   ) ||
                 0 == strcmp ( param, "artist"  ) ||
                 0 == strcmp ( param, "album"   ) ||
                 0 == strcmp ( param, "year"    ) ||
                 0 == strcmp ( param, "track"   ) ||
                 0 == strcmp ( param, "genre"   ) ||
                 0 == strcmp ( param, "comment" ) ) {
                if ( ++i < argc ) {
                    if ( (Options._Values.TagFields = (NewTagFields *)realloc ( Options._Values.TagFields, sizeof (*Options._Values.TagFields) * (Options._Values.TagFieldCount + 1) )) == NULL ) {
                        fprintf ( stderr, "get_parameters: Memory allocation failed.\n" );
                        exit (1);
                    }
                    if ( (Options._Values.TagFields[Options._Values.TagFieldCount].Field = malloc ( strlen (param) + 1 + strlen (new_argv[i]) + 1 )) == NULL ) {
                        fprintf ( stderr, "get_parameters: Memory allocation failed.\n" );
                        exit (1);
                    }
                    strcpy ( Options._Values.TagFields[Options._Values.TagFieldCount].Field, param       );
                    strcat ( Options._Values.TagFields[Options._Values.TagFieldCount].Field, "="         );
                    strcat ( Options._Values.TagFields[Options._Values.TagFieldCount].Field, new_argv[i] );
                    if ( Options._Values.TagFields[Options._Values.TagFieldCount].Field[0] >= 'a' && Options._Values.TagFields[Options._Values.TagFieldCount].Field[0] <= 'z' ) {
                        Options._Values.TagFields[Options._Values.TagFieldCount].Field[0] -= 'a' - 'A';
                    }
                    Options._Values.TagFields[Options._Values.TagFieldCount].in_utf8 = (Options.UnicodeOS && i < console_argc);
                    Options._Values.TagFieldCount++;
                } else {
                    fprintf ( stderr, "%s: required argument <value> missing.\n", param );
                    exit (1);
                }
            } else
            if ( 0 == strcmp ( param, "allow" ) ) {
                Options.Allow = 1;
                if ( ++i < argc && new_argv[i][0] != '-' ) {
                    if ( (Options._Values.Allow = (char **)realloc ( Options._Values.Allow, sizeof (*Options._Values.Allow) * (Options._Values.AllowCount + 1) )) == NULL ) {
                        fprintf ( stderr, "get_parameters: Memory allocation failed.\n" );
                        exit (1);
                    }
                    if ( (Options._Values.Allow[Options._Values.AllowCount] = malloc ( strlen (new_argv[i]) + 1 )) == NULL ) {
                        fprintf ( stderr, "get_parameters: Memory allocation failed.\n" );
                        exit (1);
                    }
                    strcpy ( Options._Values.Allow[Options._Values.AllowCount], new_argv[i] );
                    Options._Values.AllowCount++;
                } else {
                    fprintf ( stderr, "%s: required argument <item=value> missing.\n", param );
                    exit (1);
                }
            } else
            if ( 0 == strcmp ( param, "scheme" ) ) {
                if ( ++i < argc && new_argv[i][0] != '-' ) {
                    free ( Options._Values.Scheme );
                    if ( (Options._Values.Scheme = malloc ( strlen (new_argv[i]) + 1 )) == NULL ) {
                        fprintf ( stderr, "get_parameters: Memory allocation failed.\n" );
                        exit (1);
                    }
                    strcpy ( Options._Values.Scheme, new_argv[i] );
                } else {
                    fprintf ( stderr, "%s: required argument <scheme> missing.\n", param );
                    exit (1);
                }
            } else
            if ( 0 == strcmp ( param, "force" ) ) {
                Options.Force = 1;
                if ( ++i < argc && new_argv[i][0] != '-' ) {
                    if ( (Options._Values.ForceTags = (char **)realloc ( Options._Values.ForceTags, sizeof (*Options._Values.ForceTags) * (Options._Values.ForceTagCount + 1) )) == NULL ) {
                        fprintf ( stderr, "get_parameters: Memory allocation failed.\n" );
                        exit (1);
                    }
                    if ( (Options._Values.ForceTags[Options._Values.ForceTagCount] = malloc ( strlen (new_argv[i]) + 1 )) == NULL ) {
                        fprintf ( stderr, "get_parameters: Memory allocation failed.\n" );
                        exit (1);
                    }
                    strcpy ( Options._Values.ForceTags[Options._Values.ForceTagCount], new_argv[i] );
                    Options._Values.ForceTagCount++;
                } else {
                    fprintf ( stderr, "%s: required argument <tag> missing.\n", param );
                    exit (1);
                }
            } else
            if ( 0 == strcmp ( param, "sort" ) ) {
                Options.Sort = 1;
                if ( ++i < argc && new_argv[i][0] != '-' ) {
                    free ( Options._Values.SortBy );
                    Options._Values.SortCount = 0;
                    if ( (Options._Values.SortBy = (char **)malloc ( sizeof (*Options._Values.SortBy) * (Options._Values.SortCount + 1) )) == NULL ) {
                        fprintf ( stderr, "get_parameters: Memory allocation failed.\n" );
                        exit (1);
                    }
                    if ( (Options._Values.SortBy[Options._Values.SortCount] = malloc ( strlen (new_argv[i]) + 1 )) == NULL ) {
                        fprintf ( stderr, "get_parameters: Memory allocation failed.\n" );
                        exit (1);
                    }
                    strcpy ( Options._Values.SortBy[Options._Values.SortCount], new_argv[i] );
                    Options._Values.SortCount++;
                } else {
                    fprintf ( stderr, "%s: required argument <by> missing.\n", param );
                    exit (1);
                }
            } else
            if ( 0 == strcmp ( param, "sort+" ) ) {
                Options.Sort = 1;
                if ( ++i < argc && new_argv[i][0] != '-' ) {
                    if ( (Options._Values.SortBy = (char **)realloc ( Options._Values.SortBy, sizeof (*Options._Values.SortBy) * (Options._Values.SortCount + 1) )) == NULL ) {
                        fprintf ( stderr, "get_parameters: Memory allocation failed.\n" );
                        exit (1);
                    }
                    if ( (Options._Values.SortBy[Options._Values.SortCount] = malloc ( strlen (new_argv[i]) + 1 )) == NULL ) {
                        fprintf ( stderr, "get_parameters: Memory allocation failed.\n" );
                        exit (1);
                    }
                    strcpy ( Options._Values.SortBy[Options._Values.SortCount], new_argv[i] );
                    Options._Values.SortCount++;
                } else {
                    fprintf ( stderr, "%s: required argument <by> missing.\n", param );
                    exit (1);
                }
            } else
            if ( 0 == strcmp ( param, "sortdesc" ) ) {
                Options.DescendingSort = 1;
            } else
            if ( 0 == strcmp ( param, "auto" ) ) {
                Options.Auto = 1;
            } else
            if ( 0 == strcmp ( param, "autoscheme" ) ) {
                Options.AutoScheme = 1;
            } else
            if ( 0 == strcmp ( param, "nomagic" ) ) {
                Options.NoMagic = 1;
            } else
            if ( 0 == strcmp ( param, "a-artist" ) ) {
                if ( ++i < argc && new_argv[i][0] != '-' ) {
                    free ( Options._Values.AlbumArtist );
                    if ( (Options._Values.AlbumArtist = malloc ( strlen (new_argv[i]) + 1 )) == NULL ) {
                        fprintf ( stderr, "get_parameters: Memory allocation failed.\n" );
                        exit (1);
                    }
                    strcpy ( Options._Values.AlbumArtist, new_argv[i] );
                } else {
                    fprintf ( stderr, "%s: required argument <album artist> missing.\n", param );
                    exit (1);
                }
            } else
            if ( 0 == strcmp ( param, "a-title" ) ) {
                if ( ++i < argc && new_argv[i][0] != '-' ) {
                    free ( Options._Values.AlbumTitle );
                    if ( (Options._Values.AlbumTitle = malloc ( strlen (new_argv[i]) + 1 )) == NULL ) {
                        fprintf ( stderr, "get_parameters: Memory allocation failed.\n" );
                        exit (1);
                    }
                    strcpy ( Options._Values.AlbumTitle, new_argv[i] );
                } else {
                    fprintf ( stderr, "%s: required argument <album title> missing.\n", param );
                    exit (1);
                }
            } else
            if ( 0 == strcmp ( param, "fromfile" ) ) {
                Options.FromFile = 1;
                if ( ++i < argc && new_argv[i][0] != '-' ) {
                    free ( Options._Values.TagSourceFile );
                    if ( (Options._Values.TagSourceFile = malloc ( strlen (new_argv[i]) + 1 )) == NULL ) {
                        fprintf ( stderr, "get_parameters: Memory allocation failed.\n" );
                        exit (1);
                    }
                    strcpy ( Options._Values.TagSourceFile, new_argv[i] );
                } else {
                    fprintf ( stderr, "%s: required argument <file> missing.\n", param );
                    exit (1);
                }
            } else
            if ( 0 == strcmp ( param, "tofile" ) ) {
                Options.OutputToFile = 1;
                if ( ++i < argc && new_argv[i][0] != '-' ) {
                    free ( Options._Values.OutputFileScheme );
                    if ( (Options._Values.OutputFileScheme = malloc ( strlen (new_argv[i]) + 1 )) == NULL ) {
                        fprintf ( stderr, "get_parameters: Memory allocation failed.\n" );
                        exit (1);
                    }
                    strcpy ( Options._Values.OutputFileScheme, new_argv[i] );
                } else {
                    fprintf ( stderr, "%s: required argument <scheme> missing.\n", param );
                    exit (1);
                }
            } else
            if ( 0 == strcmp ( param, "tofileext" ) ) {
                Options.OutputToFileExt = 1;
                if ( ++i < argc && new_argv[i][0] != '-' ) {
                    free ( Options._Values.OutputFileExtension );
                    if ( new_argv[i][0] == '.' ) {  // there is dot
                        if ( (Options._Values.OutputFileExtension = malloc ( strlen (new_argv[i]) + 1 )) == NULL ) {
                            fprintf ( stderr, "get_parameters: Memory allocation failed.\n" );
                            exit (1);
                        }
                        strcpy ( Options._Values.OutputFileExtension, new_argv[i] );
                    } else {                        // there is no dot
                        if ( (Options._Values.OutputFileExtension = malloc ( strlen (new_argv[i]) + 1 + 1 )) == NULL ) {
                            fprintf ( stderr, "get_parameters: Memory allocation failed.\n" );
                            exit (1);
                        }
                        sprintf ( Options._Values.OutputFileExtension, ".%s", new_argv[i] );
                    }
                } else {
                    fprintf ( stderr, "%s: required argument <extension> missing.\n", param );
                    exit (1);
                }
            } else
            if ( 0 == strcmp ( param, "tofilen" ) ) {
                Options.OutputToFile = 1;
                if ( ++i < argc && new_argv[i][0] != '-' ) {
                    free ( Options._Values.OutputFileName );
                    if ( (Options._Values.OutputFileName = malloc ( strlen (new_argv[i]) + 1 )) == NULL ) {
                        fprintf ( stderr, "get_parameters: Memory allocation failed.\n" );
                        exit (1);
                    }
                    strcpy ( Options._Values.OutputFileName, new_argv[i] );
                } else {
                    fprintf ( stderr, "%s: required argument <name> missing.\n", param );
                    exit (1);
                }
            } else
            if ( 0 == strcmp ( param, "decode" ) ) {
                Options.Decode = 1;
            } else
            if ( 0 == strcmp ( param, "hideinfo" ) ) {
                Options.HideFileInfo = 1;
            } else
            if ( 0 == strcmp ( param, "hidetags" ) ) {
                Options.HideTags = 1;
            } else
            if ( 0 == strcmp ( param, "hidenames" ) ) {
                Options.HideNames = 1;
            } else
            if ( 0 == strcmp ( param, "simple" ) ) {
                Options.SimpleDisplay = 1;
            } else
            if ( 0 == strcmp ( param, "test" ) ) {
                Options.TestMode = 1;
            } else
            if ( 0 == strcmp ( param, "nfo" ) ) {
                Options.NfoFile = 1;
                if ( ++i < argc && new_argv[i][0] != '-' ) {
                    free ( Options._Values.NFOname );
                    if ( (Options._Values.NFOname = malloc ( strlen (new_argv[i]) + 1 )) == NULL ) {
                        fprintf ( stderr, "get_parameters: Memory allocation failed.\n" );
                        exit (1);
                    }
                    strcpy ( Options._Values.NFOname, new_argv[i] );
                } else {
                    fprintf ( stderr, "%s: required argument <file> missing.\n", param );
                    exit (1);
                }
            } else
            if ( 0 == strcmp ( param, "id3ext" ) ) {
                Options.ID3Ext = 1;
            } else
            if ( 0 == strcmp ( param, "oldtype" ) ) {
                Options.OldType = 1;
            } else
            if ( 0 == strcmp ( param, "recursive" ) ) {
                Options.Recursive = 1;
            } else
            if ( 0 == strcmp ( param, "rename" ) ) {
                Options.RenameFromName = 1;
                if ( ++i < argc && new_argv[i][0] != '-' ) {
                    free ( Options._Values.RenameToScheme );
                    if ( (Options._Values.RenameToScheme = malloc ( strlen (new_argv[i]) + 1 )) == NULL ) {
                        fprintf ( stderr, "get_parameters: Memory allocation failed.\n" );
                        exit (1);
                    }
                    strcpy ( Options._Values.RenameToScheme, new_argv[i] );
                } else {
                    fprintf ( stderr, "%s: required argument <new scheme> missing.\n", param );
                    exit (1);
                }
            } else
            if ( 0 == strcmp ( param, "rentag" ) ) {
                Options.RenameFromTag = 1;
            } else
            if ( 0 == strcmp ( param, "move" ) ) {
                Options.RenameMove = 1;
            } else
            if ( 0 == strcmp ( param, "overwrite" ) ) {
                Options.RenameOverwrite = 1;
            } else
            if ( 0 == strcmp ( param, "trackinc" ) ) {
                Options.TrackInc = 1;
            } else
            if ( 0 == strcmp ( param, "caps" ) ) {
                Options.CaseConversion = Capitalize;
            } else
            if ( 0 == strcmp ( param, "Caps" ) ) {
                Options.CaseConversion = Sentence;
            } else
            if ( 0 == strcmp ( param, "upper" ) ) {
                Options.CaseConversion = Upper;
            } else
            if ( 0 == strcmp ( param, "lower" ) ) {
                Options.CaseConversion = Lower;
            } else
            if ( 0 == strcmp ( param, "tcaps" ) ) {
                Options.TagCaseConv = 1;
                if ( ++i < argc && new_argv[i][0] != '-' ) {
                    if ( (Options._Values.Items_caps = (char **)realloc ( Options._Values.Items_caps, sizeof (*Options._Values.Items_caps) * (Options._Values.IcapsCount + 1) )) == NULL ) {
                        fprintf ( stderr, "get_parameters: Memory allocation failed.\n" );
                        exit (1);
                    }
                    if ( (Options._Values.Items_caps[Options._Values.IcapsCount] = malloc ( strlen (new_argv[i]) + 1 )) == NULL ) {
                        fprintf ( stderr, "get_parameters: Memory allocation failed.\n" );
                        exit (1);
                    }
                    strcpy ( Options._Values.Items_caps[Options._Values.IcapsCount], new_argv[i] );
                    Options._Values.IcapsCount++;
                } else {
                    fprintf ( stderr, "%s: required argument <item> missing.\n", param );
                    exit (1);
                }
            } else
            if ( 0 == strcmp ( param, "tCaps" ) ) {
                Options.TagCaseConv = 1;
                if ( ++i < argc && new_argv[i][0] != '-' ) {
                    if ( (Options._Values.Items_Caps = (char **)realloc ( Options._Values.Items_Caps, sizeof (*Options._Values.Items_Caps) * (Options._Values.ICapsCount + 1) )) == NULL ) {
                        fprintf ( stderr, "get_parameters: Memory allocation failed.\n" );
                        exit (1);
                    }
                    if ( (Options._Values.Items_Caps[Options._Values.ICapsCount] = malloc ( strlen (new_argv[i]) + 1 )) == NULL ) {
                        fprintf ( stderr, "get_parameters: Memory allocation failed.\n" );
                        exit (1);
                    }
                    strcpy ( Options._Values.Items_Caps[Options._Values.ICapsCount], new_argv[i] );
                    Options._Values.ICapsCount++;
                } else {
                    fprintf ( stderr, "%s: required argument <item> missing.\n", param );
                    exit (1);
                }
            } else
            if ( 0 == strcmp ( param, "tlower" ) ) {
                Options.TagCaseConv = 1;
                if ( ++i < argc && new_argv[i][0] != '-' ) {
                    if ( (Options._Values.Items_lower = (char **)realloc ( Options._Values.Items_lower, sizeof (*Options._Values.Items_lower) * (Options._Values.IlowerCount + 1) )) == NULL ) {
                        fprintf ( stderr, "get_parameters: Memory allocation failed.\n" );
                        exit (1);
                    }
                    if ( (Options._Values.Items_lower[Options._Values.IlowerCount] = malloc ( strlen (new_argv[i]) + 1 )) == NULL ) {
                        fprintf ( stderr, "get_parameters: Memory allocation failed.\n" );
                        exit (1);
                    }
                    strcpy ( Options._Values.Items_lower[Options._Values.IlowerCount], new_argv[i] );
                    Options._Values.IlowerCount++;
                } else {
                    fprintf ( stderr, "%s: required argument <item> missing.\n", param );
                    exit (1);
                }
            } else
            if ( 0 == strcmp ( param, "tupper" ) ) {
                Options.TagCaseConv = 1;
                if ( ++i < argc && new_argv[i][0] != '-' ) {
                    if ( (Options._Values.Items_upper = (char **)realloc ( Options._Values.Items_upper, sizeof (*Options._Values.Items_upper) * (Options._Values.IupperCount + 1) )) == NULL ) {
                        fprintf ( stderr, "get_parameters: Memory allocation failed.\n" );
                        exit (1);
                    }
                    if ( (Options._Values.Items_upper[Options._Values.IupperCount] = malloc ( strlen (new_argv[i]) + 1 )) == NULL ) {
                        fprintf ( stderr, "get_parameters: Memory allocation failed.\n" );
                        exit (1);
                    }
                    strcpy ( Options._Values.Items_upper[Options._Values.IupperCount], new_argv[i] );
                    Options._Values.IupperCount++;
                } else {
                    fprintf ( stderr, "%s: required argument <item> missing.\n", param );
                    exit (1);
                }
            } else
            if ( 0 == strcmp ( param, "nozero" ) ) {
                Options.NoLeadingZero = 1;
            } else
            if ( 0 == strcmp ( param, "zeropad" ) ) {
                Options.ZeroPad = 1;
            } else
            if ( 0 == strcmp ( param, "nocheck" ) ) {
                Options.NoExtensionCheck = 1;
            } else
            if ( 0 == strcmp ( param, "commafix" ) ) {
                Options.CommaFix = 1;
            } else
            if ( 0 == strcmp ( param, "spacefix" ) ) {
                Options.SpaceFix = 1;
            } else
            if ( 0 == strcmp ( param, "itemfix" ) ) {
                Options.ItemFix = 1;
            } else
            if ( 0 == strcmp ( param, "umlfix" ) ) {
                Options.UmlautFix = 1;
            } else
            if ( 0 == strcmp ( param, "nospaces" ) ) {
                Options.ReplaceSpaces = 1;
            } else
            if ( 0 == strcmp ( param, "chreplace" ) ) {
                Options.CharReplaceForTag = 1;
            } else
            if ( 0 == strcmp ( param, "swapta" ) ) {
                Options.SwapTitleAndArtist = 1;
            } else
            if ( 0 == strcmp ( param, "ape2" ) ) {
                Options.DefaultAPE2 = 1;
            } else
            if ( 0 == strcmp ( param, "newdate" ) ) {
                Options.UpdateTime = 1;
            } else
            if ( 0 == strcmp ( param, "remove" ) ) {
                Options.Remove = 1;
            } else
#ifdef  ID3V2SUPPORT
            if ( 0 == strcmp ( param, "removeid3v2" ) ) {
                Options.RemoveID3v2 = 1;
            } else
            if ( 0 == strcmp ( param, "removeid3v2u" ) ) {
                Options.RemoveUnneededID3v2 = 1;
            } else
#endif  // ID3V2SUPPORT
            if ( 0 == strcmp ( param, "oneplaylist" ) ) {
                Options.OnePlaylist = 1;
            } else
            if ( 0 == strcmp ( param, "playlist" ) ) {
                Options.PlaylistPerDir = 1;
            } else
            if ( 0 == strcmp ( param, "playlists" ) ) {
                Options.PlaylistPerAlbum = 1;
            } else
            if ( 0 == strcmp ( param, "onlyfiles" ) ) {
                Options.PlaylistNoExt = 1;
            } else
            if ( 0 == strcmp ( param, "dirname" ) ) {
                Options.PlaylistNameFromDir = 1;
            } else
            if ( 0 == strcmp ( param, "slashes" ) ) {
                Options.PlaylistUnixSlashes = 1;
            } else
            if ( 0 == strcmp ( param, "plname" ) ) {
                Options.PlaylistName = 1;
                if ( ++i < argc && new_argv[i][0] != '-' ) {
                    char fname[_MAX_FNAME];

                    _splitpath ( new_argv[i], NULL, NULL, fname, NULL );
                    free ( Options._Values.PlaylistName );
                    if ( (Options._Values.PlaylistName = malloc ( strlen (fname) + 1 )) == NULL ) {
                        fprintf ( stderr, "get_parameters: Memory allocation failed.\n" );
                        exit (1);
                    }
                    strcpy ( Options._Values.PlaylistName, fname );
                } else {
                    fprintf ( stderr, "%s: required argument <name> missing.\n", param );
                    exit (1);
                }
            } else
            {
                if ( !isfile (new_argv[i]) ) {
                    fprintf ( stderr, "Invalid parameter: %s\n", param );
                    exit (1);
                }
                process = 0;
            }

            if ( process ) {
                new_argv[orig_i] = NULL;
                new_argv[i]      = NULL;
            }
        }
    }

    // find files
    for ( i = 1; i < argc; i++ ) {
        size_t count;
        if ( new_argv[i] == NULL ) continue;

        count = Files->FileCount;
        if ( Options.Recursive ) {
            _find_recurse ( new_argv[i], Files );
        } else {
            _find ( new_argv[i], Files );
        }
        if ( count == Files->FileCount ) {
            char*   asciiname;
            asciiname = (char *)malloc ( strlen (new_argv[i]) + 1 );
            if ( asciiname != NULL ) {
                string_l2a ( new_argv[i], asciiname );
                printf ( "File not found: '%s'\n", asciiname );
                free ( asciiname );
            } else {
                printf ( "File not found: '%s'\n", new_argv[i] );
            }
            errors++;
        }
    }

    // check sorting options
    for ( i = 0; i < Options._Values.SortCount; i++ ) {
        if ( strcmp ( Options._Values.SortBy[i], "none" ) == 0 ) {     // no sorting
            free ( Options._Values.SortBy );
            Options._Values.SortCount = 0;
            Options.Sort = 0;
        }
    }

    // check if scheme == AUTO
    if ( Options._Values.Scheme != NULL ) {
        if ( stricmp ( Options._Values.Scheme, "AUTO" ) == 0 ) {        // autoscheme
            free ( Options._Values.Scheme );
            Options.AutoScheme = 1;
        }
    }

    // check proper usage of options
    if ( Options.RenameFromTag && Options._Values.Scheme == NULL ) {
        fprintf ( stderr, "rentag: Required option --scheme missing.\n" );
        exit (1);
    }
    if ( Options.Decode && Options._Values.Scheme == NULL ) {
        fprintf ( stderr, "decode: Required option --scheme missing.\n" );
        exit (1);
    }
    if ( Options.RenameFromName && Options.RenameFromTag ) {
        fprintf ( stderr, "rename and rentag can't be used simultaneously.\n" );
        exit (1);
    }
    if ( Options.Remove && Options.RemoveID3v2 ) {
        fprintf ( stderr, "remove and removeid3v2 can't be used simultaneously.\n" );
        exit (1);
    }
    if ( Options.Remove && Options.RemoveUnneededID3v2 ) {
        fprintf ( stderr, "remove and removeid3v2u can't be used simultaneously.\n" );
        exit (1);
    }
    if ( Options.RemoveID3v2 && Options.RemoveUnneededID3v2 ) {
        fprintf ( stderr, "removeid3v2 and removeid3v2u can't be used simultaneously.\n" );
        exit (1);
    }
    if ( Options.Force && Options.OldType ) {
        fprintf ( stderr, "force and oldtype can't be used simultaneously.\n" );
        exit (1);
    }
    if ( Options.FromFile && !isfile (Options._Values.TagSourceFile) ) {
        fprintf ( stderr, "fromfile: Can't open file '%s'.\n", Options._Values.TagSourceFile );
        exit (1);
    }
    if ( Options._Values.OutputFileName != NULL && Options._Values.OutputFileScheme != NULL ) {
        fprintf ( stderr, "tofile and tofilen can't be used simultaneously.\n" );
        exit (1);
    }

    // convert ID3v1 genre number to proper name
    for ( i = 0; i < Options._Values.TagFieldCount; i++ ) {
        const char* genrestr = "Genre=";
        if ( strnicmp ( Options._Values.TagFields[i].Field, genrestr, strlen (genrestr) ) == 0 ) {
            const char* value = (char *)(Options._Values.TagFields[i].Field + strlen (genrestr));
            const char* p = value;
            int numerical = 1;

            if ( *p == '\0' ) continue;
            while ( *p ) {
                char c = *p++;
                if ( c < '0' || c > '9' ) {
                    numerical = 0;
                    break;
                }
            }

            if ( numerical ) {
                char genre [1024];
                GenreToString ( genre, atoi (value) );
                free ( Options._Values.TagFields[i].Field );
                if ( (Options._Values.TagFields[i].Field = malloc ( strlen (genrestr) + strlen (genre) + 1 )) == NULL ) {
                    fprintf ( stderr, "get_parameters: Memory allocation failed.\n" );
                    exit (1);
                }
                strcpy ( Options._Values.TagFields[i].Field, genrestr );
                strcat ( Options._Values.TagFields[i].Field, genre    );
            }
        }
    }

    free ( new_argv );

    return errors;
}

// Searches and reads config file
int load_config ( const char* filename, SchemeList* Schemes, PresetsList* Presets, ExceptionList* Exceptions, CharReplaceList* Replace, CharReplaceForTag* ReplaceTag, NFOfileTemplate* NFOfile )
{
    enum mode_t {
		MUndefined      = 0,
		MDefault        = 1,   // [Default]
        MSchemes        = 2,   // [Schemes]
		MPresets        = 3,   // [Presets]
		MExceptions     = 4,   // [Exceptions]
        MCharReplace    = 5,   // [CharReplacement]
        MCharReplaceTag = 6,   // [CharReplacementForTag]
		MNFOfile        = 7,   // [NFOfile]
		MNFOtemplate    = 8,   // [NFOtemplate]
	};

    char            path [_MAX_PATH];
    char            drive[_MAX_DRIVE];
    char            dir  [_MAX_DIR];
    char            fname[_MAX_FNAME];
    char            temp [4096];
    char            word [4096];
    char*           p;
    FILE*           fp;
    enum mode_t     mode = MUndefined;
    size_t          wpos;
    size_t          num;

    _splitpath ( filename, drive, dir, fname, NULL );
    strcat ( fname, ".cfg" );
    if ( drive[0] != '\0' || dir[0] != '\0' ) {
        sprintf ( path, "%s%s%s", drive, dir, fname );
    } else {
        _searchenv ( fname, "PATH", path );
    }

    if ( path[0] == '\0' )
        return 1;
    if ( (fp = fopen ( path, "rb" )) == NULL )
        return 1;

    while ( fgets_clean ( temp, sizeof (temp), fp ) != EOF ) {
        if ( temp[0] == '\0' || temp[0] == ';' || temp[0] == '#' )  // skip empty lines and comments
            continue;
        
        if ( temp[0] == '[' ) {
            if ( 0 == stricmp ( temp, "[Default]" ) ) {
                mode = MDefault;
                continue;
            } else
            if ( 0 == stricmp ( temp, "[Schemes]" ) ) {
                mode = MSchemes;
                continue;
            } else
            if ( 0 == stricmp ( temp, "[Presets]" ) ) {
                mode = MPresets;
                continue;
            } else
            if ( 0 == stricmp ( temp, "[Exceptions]" ) ) {
                mode = MExceptions;
                continue;
            } else
            if ( 0 == stricmp ( temp, "[CharReplacement]" ) ) {
                mode = MCharReplace;
                continue;
            } else
            if ( 0 == stricmp ( temp, "[CharReplacementForTag]" ) ) {
                mode = MCharReplaceTag;
                continue;
            } else
            if ( 0 == stricmp ( temp, "[NFOfile]" ) ) {
                mode = MNFOfile;
                continue;
            } else
            if ( 0 == stricmp ( temp, "[NFOtemplate]" ) ) {
                mode = MNFOtemplate;
                continue;
            }
        }

        switch ( mode ) {
        case MDefault:          // settings under [Default]
            wpos = 0;
            while ( get_word ( word, temp, &wpos ) > 0 ) {
                if ( (Options._Values.Defaults = (char **)realloc ( Options._Values.Defaults, sizeof (*Options._Values.Defaults) * (Options._Values.DefaultCount + 1) )) == NULL ) {
                    fprintf ( stderr, "load_config: Memory allocation failed.\n" );
                    exit (1);
                }
                if ( (Options._Values.Defaults[Options._Values.DefaultCount] = malloc ( strlen (word) + 1 )) == NULL ) {
                    fprintf ( stderr, "load_config: Memory allocation failed.\n" );
                    exit (1);
                }
                strcpy ( Options._Values.Defaults[Options._Values.DefaultCount], word );
                Options._Values.DefaultCount++;
            }
            break;

        case MSchemes:          // settings under [Schemes]
            if ( (Schemes->Schemes = (char **)realloc ( Schemes->Schemes, sizeof (*Schemes->Schemes) * (Schemes->SchemeCount + 1) )) == NULL ) {
                fprintf ( stderr, "load_config: Memory allocation failed.\n" );
                exit (1);
            }
            if ( (Schemes->Schemes[Schemes->SchemeCount] = malloc ( strlen (temp) + 1 )) == NULL ) {
                fprintf ( stderr, "load_config: Memory allocation failed.\n" );
                exit (1);
            }
            strcpy ( Schemes->Schemes[Schemes->SchemeCount], temp );
            Schemes->SchemeCount++;
            break;

        case MPresets:          // settings under [Presets]
            num = atoi ( temp );
            if ( num > Presets->PresetsCount ) {
                size_t i;
                p = temp;
                while ( *p && ((*p >= '0' && *p <= '9') || (*p == '=')) ) {
                    p++;
                }
                if ( *p == '\0' ) continue;
                if ( (Presets->Presets = (char **)realloc ( Presets->Presets, sizeof (*Presets->Presets) * num )) == NULL ) {
                    fprintf ( stderr, "load_config: Memory allocation failed.\n" );
                    exit (1);
                }
                for ( i = Presets->PresetsCount; i < num; i++ ) {
                    Presets->Presets[i] = NULL;
                }
                if ( (Presets->Presets[num - 1] = malloc ( strlen (p) + 1 )) == NULL ) {
                    fprintf ( stderr, "load_config: Memory allocation failed.\n" );
                    exit (1);
                }
                strcpy ( Presets->Presets[num - 1], p );
                Presets->PresetsCount = num;
            }
            break;

        case MExceptions:       // settings under [Exceptions]
            if ( (Exceptions->Words = (char **)realloc ( Exceptions->Words, sizeof (*Exceptions->Words) * (Exceptions->WordCount + 1) )) == NULL ) {
                fprintf ( stderr, "load_config: Memory allocation failed.\n" );
                exit (1);
            }
            if ( (Exceptions->Words[Exceptions->WordCount] = malloc ( strlen (temp) + 1 )) == NULL ) {
                fprintf ( stderr, "load_config: Memory allocation failed.\n" );
                exit (1);
            }
            strcpy ( Exceptions->Words[Exceptions->WordCount], temp );
            Exceptions->WordCount++;
            break;

        case MCharReplace:      // settings under [CharReplacement]
            if ( (Replace->OldChars = realloc ( Replace->OldChars, Replace->CharCount + 1 )) == NULL ) {
                fprintf ( stderr, "load_config: Memory allocation failed.\n" );
                exit (1);
            }
            if ( (Replace->NewChars = (char **)realloc ( Replace->NewChars, sizeof (*Replace->NewChars) * (Replace->CharCount + 1) )) == NULL ) {
                fprintf ( stderr, "load_config: Memory allocation failed.\n" );
                exit (1);
            }
            Replace->OldChars[Replace->CharCount] = temp[0];
            p = (char *)(temp + 2);
            if ( temp[1] == '\0' || p[0] == '\0' ) {
                Replace->NewChars[Replace->CharCount] = NULL;
            } else {
                if ( (Replace->NewChars[Replace->CharCount] = malloc ( strlen (p) + 1 )) == NULL ) {
                    fprintf ( stderr, "load_config: Memory allocation failed.\n" );
                    exit (1);
                }
                strcpy ( Replace->NewChars[Replace->CharCount], p );
            }
            Replace->CharCount++;
            break;

        case MCharReplaceTag:   // settings under [CharReplacementForTag]
            if ( (ReplaceTag->OldChars = (char **)realloc ( ReplaceTag->OldChars, sizeof (*ReplaceTag->OldChars) * (ReplaceTag->CharCount + 1) )) == NULL ) {
                fprintf ( stderr, "load_config: Memory allocation failed.\n" );
                exit (1);
            }
            if ( (ReplaceTag->NewChars = (char *)realloc ( ReplaceTag->NewChars, ReplaceTag->CharCount + 1 )) == NULL ) {
                fprintf ( stderr, "load_config: Memory allocation failed.\n" );
                exit (1);
            }
            p = temp;
            while ( *p && *p != ' ' ) p++;
            if ( (ReplaceTag->OldChars[ReplaceTag->CharCount] = malloc ( p - temp + 1 )) == NULL ) {
                fprintf ( stderr, "load_config: Memory allocation failed.\n" );
                exit (1);
            }
            if ( p > temp ) {
                strncpy ( ReplaceTag->OldChars[ReplaceTag->CharCount], temp, p-temp );
            }
            ReplaceTag->OldChars[ReplaceTag->CharCount][p-temp] = '\0';
            if ( *p == ' ' ) p++;
            ReplaceTag->NewChars[ReplaceTag->CharCount] = *p;
            ReplaceTag->CharCount++;
            break;

        case MNFOfile:          // settings under [NFOfile]
            if      ( 1 == sscanf ( temp, "CaseConversion=%s"   , &word    ) ) {
                if ( 0 == strcmp ( word, "caps"  ) ) NFOfile->CaseConversion = Capitalize;
                if ( 0 == strcmp ( word, "Caps"  ) ) NFOfile->CaseConversion = Sentence;
                if ( 0 == strcmp ( word, "lower" ) ) NFOfile->CaseConversion = Lower;
                if ( 0 == strcmp ( word, "upper" ) ) NFOfile->CaseConversion = Upper;
            }
            else if ( 1 == sscanf ( temp, "ReplaceSpaces=%c"    , &word[0] ) ) {
                NFOfile->ReplaceSpaces = word[0];
            }
            else if ( 1 == sscanf ( temp, "ApplyToTracks=%s"    , &word    ) ) {
                if ( 0 == stricmp ( word, "yes" ) ) {
                    NFOfile->ApplyToTracks = 1;
                } else {
                    NFOfile->ApplyToTracks = 0;
                }
            }
            else if ( 0 == strnicmp ( temp, "SchemeTracklist=", 16 ) ) {
                char* p = (char *)(temp + 16);
                free ( NFOfile->Scheme );
                if ( (NFOfile->Scheme = malloc ( strlen (p) + 1 )) == NULL ) {
                    fprintf ( Options.output, "load_config: Memory allocation failed.\n" );
                    exit (1);
                }
                strcpy ( NFOfile->Scheme, p );
            }
            else if ( 0 == strnicmp ( temp, "SchemeTracklistVA=", 18 ) ) {
                char* p = (char *)(temp + 18);
                free ( NFOfile->SchemeVA );
                if ( (NFOfile->SchemeVA = malloc ( strlen (p) + 1 )) == NULL ) {
                    fprintf ( Options.output, "load_config: Memory allocation failed.\n" );
                    exit (1);
                }
                strcpy ( NFOfile->SchemeVA, p );
            }
            break;

        case MNFOtemplate:      // text under [NFOtemplate] is nfo template
            p = temp;
            if ( *p == '"' ) p++;
            if ( temp[strlen (temp)-1] == '"' ) temp[strlen (temp)-1] = '\0';
            if ( (NFOfile->Lines = (char **)realloc ( NFOfile->Lines, sizeof (*NFOfile->Lines) * (NFOfile->LineCount + 1) )) == NULL ) {
                fprintf ( stderr, "load_config: Memory allocation failed.\n" );
                exit (1);
            }
            if ( (NFOfile->Lines[NFOfile->LineCount] = malloc ( strlen (p) + 1 )) == NULL ) {
                fprintf ( stderr, "load_config: Memory allocation failed.\n" );
                exit (1);
            }
            strcpy ( NFOfile->Lines[NFOfile->LineCount], p );
            NFOfile->LineCount++;
            break;

        case MUndefined:
        default:
            break;
        }
    }

    fclose (fp);

    return 0;
}

// Checks if file has allowed properties
int Allowed ( const FileInfo* Info )
{
    size_t  i;
    char    item[4096];
    char*   value;
    int     allowed = 1;

    if ( !Options.Allow ) return 1;

    for ( i = 0; i < Options._Values.AllowCount; i++ ) {
        char* d   = item;
        char* s   = Options._Values.Allow[i];
        char* end = d + sizeof (item) - 1;
        while ( d < end && *s && *s != '=' ) {
            *d++ = *s++;
        }
        *d++ = '\0';
        if ( *s == '=' ) s++;
        value = s;

        if ( TagValue ( item, Info ) ) {
            if ( stricmp ( value, TagValue ( item, Info ) ) != 0 ) {
                allowed = 0;
                break;
            }
        } else {
            allowed = 0;
            break;
        }
    }

    return allowed;
}

// Inserts user defined items to tag
int ModifyTag ( FileInfo* Info )
{
    size_t  i, j;
    char    item [4096];
    char    item2[4096];
    char*   value;

    for ( i = 0; i < Options._Values.TagFieldCount; i++ ) {
        char* d   = item;
        char* s   = Options._Values.TagFields[i].Field;
        char* end = d + sizeof (item) - 1;
        while ( d < end && *s && *s != '=' ) {
            *d++ = *s++;
        }
        *d++ = '\0';
        if ( *s == '=' ) s++;
        value = s;

        if ( item[0] != '\0' ) {
            int occurrence = 0;

            for ( j = 0; j < i; j++ ) {     // check how many items with same name are there
                char* d2   = item2;
                char* s2   = Options._Values.TagFields[j].Field;
                char* end2 = d2 + sizeof (item2) - 1;
                while ( d2 < end2 && *s2 && *s2 != '=' ) {
                    *d2++ = *s2++;
                }
                *d2++ = '\0';
                if ( item2[0] != '\0' && stricmp ( item, item2 ) == 0 ) {
                    occurrence++;
                }
            }

            if ( Options._Values.TagFields[i].in_utf8 ) {
                InsertTagFieldU ( item, 0, value, 0, 0, Info, 0, 0, occurrence );
            } else {
                InsertTagField  ( item, 0, value, 0, 0, Info, 0, 0, occurrence );
            }
        }
    }

    return 0;
}

// Inserts tags from external source
int InsertExternalFields ( FileInfo* Info, const ExternalAlbum* Album, const size_t tracknum )
{
    if ( tracknum >= Album->TrackCount )        // Incorrect track number
        return 1;

    if ( Album->Tracks[tracknum].Title != NULL && Album->Tracks[tracknum].Title[0] != '\0' ) {
        InsertTagField ( APE_TAG_FIELD_TITLE,   0, Album->Tracks[tracknum].Title,   0, 0, Info, 0, 0, 0 );
    }

    if ( Album->Tracks[tracknum].Artist != NULL && Album->Tracks[tracknum].Artist[0] != '\0' ) {
        InsertTagField ( APE_TAG_FIELD_ARTIST,  0, Album->Tracks[tracknum].Artist,  0, 0, Info, 0, 0, 0 );
    } else
    if ( Album->Artist != NULL && Album->Artist[0] != '\0' ) {
        InsertTagField ( APE_TAG_FIELD_ARTIST,  0, Album->Artist,                   0, 0, Info, 0, 0, 0 );
    }

    if ( Album->Tracks[tracknum].Comment != NULL && Album->Tracks[tracknum].Comment[0] != '\0' ) {
        InsertTagField ( APE_TAG_FIELD_COMMENT, 0, Album->Tracks[tracknum].Comment, 0, 0, Info, 0, 0, 0 );
    }

    if ( Album->Tracks[tracknum].Genre != NULL && Album->Tracks[tracknum].Genre[0] != '\0' ) {
        InsertTagField ( APE_TAG_FIELD_GENRE,   0, Album->Tracks[tracknum].Genre,   0, 0, Info, 0, 0, 0 );
    } else
    if ( Album->Genre != NULL && Album->Genre[0] != '\0' ) {
        InsertTagField ( APE_TAG_FIELD_GENRE,   0, Album->Genre,                    0, 0, Info, 0, 0, 0 );
    }

    if ( Album->Tracks[tracknum].Track != NULL && Album->Tracks[tracknum].Track[0] != '\0' ) {
        InsertTagField ( APE_TAG_FIELD_TRACK,   0, Album->Tracks[tracknum].Track,   0, 0, Info, 0, 0, 0 );
    } else {
        char temp[1024];
        sprintf ( temp, "%u", tracknum + 1 );
        InsertTagField ( APE_TAG_FIELD_TRACK,   0, temp,                            0, 0, Info, 0, 0, 0 );
    }

    if ( Album->Tracks[tracknum].Year != NULL && Album->Tracks[tracknum].Year[0] != '\0' ) {
        InsertTagField ( APE_TAG_FIELD_YEAR,    0, Album->Tracks[tracknum].Year,    0, 0, Info, 0, 0, 0 );
    } else
    if ( Album->Year != NULL && Album->Year[0] != '\0' ) {
        InsertTagField ( APE_TAG_FIELD_YEAR,    0, Album->Year,                     0, 0, Info, 0, 0, 0 );
    }

    if ( Album->Album != NULL && Album->Album[0] != '\0' ) {
        InsertTagField ( APE_TAG_FIELD_ALBUM,   0, Album->Album,                    0, 0, Info, 0, 0, 0 );
    }

    return 0;
}

// Increase track number
int IncreaseTrackNumber ( )
{
    size_t  i;
    char    item [4096];
    char*   value;

    for ( i = 0; i < Options._Values.TagFieldCount; i++ ) {
        char* d   = item;
        char* s   = Options._Values.TagFields[i].Field;
        char* end = d + sizeof (item) - 1;
        while ( d < end && *s && *s != '=' ) {
            *d++ = *s++;
        }
        *d++ = '\0';
        if ( *s == '=' ) s++;
        value = s;

        if ( item[0] != '\0' && stricmp (item, APE_TAG_FIELD_TRACK) == 0 ) {
            int num = atoi ( value );

            if ( num > 0 ) {
                char temp[256];

                sprintf ( temp, "%u", num + 1 );

                if ( (Options._Values.TagFields[i].Field = realloc ( Options._Values.TagFields[i].Field, strlen (item) + 1 + strlen (temp) + 1 )) == NULL ) {
                    fprintf ( Options.output, "IncreaseTrackNumber: Memory allocation failed.\n" );
                    exit (1);
                }
                sprintf ( Options._Values.TagFields[i].Field, "%s=%s", item, temp );
            }
        }
    }

    return 0;
}

// Compare for qsort
int _compare ( const void* arg1, const void* arg2 )
{
    DetailsOfFile*  a1 = (DetailsOfFile *)arg1;
    DetailsOfFile*  a2 = (DetailsOfFile *)arg2;
    char*           s1;
    char*           s2;
    char*           t1;
    char*           t2;
    size_t          len1 = 0;
    size_t          len2 = 0;
    size_t          i;

    for ( i = 0; i < Options._Values.SortCount; i++ ) {
        if ( a1->SortValues[i] != NULL ) {
            len1 += strlen (a1->SortValues[i]);
        }
        if ( a2->SortValues[i] != NULL ) {
            len2 += strlen (a2->SortValues[i]);
        }
    }
    if ( (t1 = malloc ( len1 + 1 )) == NULL ) {
        fprintf ( Options.output, "_compare: Memory allocation failed.\n" );
        exit (1);
    }
    if ( (t2 = malloc ( len2 + 1 )) == NULL ) {
        fprintf ( Options.output, "_compare: Memory allocation failed.\n" );
        exit (1);
    }

    s1 = t1;
    s2 = t2;

    for ( i = 0; i < Options._Values.SortCount; i++ ) {
        if ( a1->SortValues[i] != NULL ) {
            s1 += sprintf ( s1, "%s", a1->SortValues[i] );
        }
        if ( a2->SortValues[i] != NULL ) {
            s2 += sprintf ( s2, "%s", a2->SortValues[i] );
        }
    }

    if ( !Options.DescendingSort ) {
        return _compare_by_numbers ( t1, t2 );
    } else {
        return _compare_by_numbers ( t2, t1 );
    }
}

// Removes leading zeros from track number
void RemoveLeadingZeros ( FileInfo* Info )
{
    size_t i;
    size_t zeros;

    if ( TagValue ( APE_TAG_FIELD_TRACK, Info ) ) {
        char* p = TagValue ( APE_TAG_FIELD_TRACK, Info );
        zeros = 0;

        do {
            if ( *p == '0' ) {
                zeros++;
            } else {
                break;
            }
        } while ( *p++ );

        if ( zeros > 0 ) {
            char* value;

            if ( (value = malloc ( strlen (TagValue ( APE_TAG_FIELD_TRACK, Info )) + 1 )) == NULL ) {
                fprintf ( Options.output, "RemoveLeadingZeros: Memory allocation failed.\n" );
                exit (1);
            }

            p = TagValue ( APE_TAG_FIELD_TRACK, Info );
            for ( i = 0; i < strlen (p) - zeros + 1; i++ ) {
                value[i] = p[i + zeros];
            }

            InsertTagField ( APE_TAG_FIELD_TRACK, 0, value, 0, TagItemFlags ( APE_TAG_FIELD_TRACK, Info ), Info, 0, 0, 0 );

            free ( value );
        }
    }
}

// Pads tracknumber with zeros
void ZeroPadTrack ( FileInfo* Info )
{
    char* p;

    if ( (p = TagValue ( APE_TAG_FIELD_TRACK, Info )) != NULL ) {
        if ( strlen (p) < 2 ) {
            char temp[4];

            temp[0] = '0';
            temp[1] = *p;
            temp[2] = '\0';
            InsertTagField ( APE_TAG_FIELD_TRACK, 0, temp, 0, TagItemFlags ( APE_TAG_FIELD_TRACK, Info ), Info, 0, 0, 0 );
        }
    }
}

// Fixes comma separations ( x, the -> the x )
int CommaFix ( const char* item, FileInfo* Info )
{
    int i;
    int cpos = -1;
    int ccount = 0; // number of commas
    int len;
    size_t itemnum;
    char* temp;
    char* p = TagValueU ( item, Info );

    if ( p == NULL ) {
        return 0;
    }

    len = (int)strlen (p);

    if ( (temp = malloc ( len + 128 )) == NULL ) {
        fprintf ( Options.output, "CommaFix: Memory allocation failed.\n" );
        exit (1);
    }

    for ( i = 0; i < len; i++ ) {
        if ( p[i] == ',' ) ccount++;

        if ( p[i] == '&' && ccount > 0 ) {
            return 0;
        }

        if ( i + 2 < len &&
             (p[i]   == ' ' || p[i]   == '_') &&
             (p[i+4] == ' ' || p[i+4] == '_') &&
             strnicmp ( (char *)(p+i+1), "AND", 3 ) == 0 &&
             ccount > 0 ) {
            return 0;
        }
    }

    for ( i = len-1; i >= 0; i-- ) {
        if ( p[i] == ',' ) {
            cpos = i;
            break;
        }
    }

    if ( cpos < 1 || cpos == len - 1 ) return 0;

    strcpy  ( temp, (char *)(p+cpos+1) );
    strcat  ( temp, " " );
    strncat ( temp, (char *)p, cpos );

    cpos = 0;

    for ( i = 0; i < len-cpos; i++ ) {
        if ( temp[i] != ' ' && temp[i] != '_' ) {
            cpos = i;
            break;
        }
    }

    itemnum = TagItemNum ( item, Info );
    InsertTagFieldU ( item, 0, temp + cpos, strlen (temp)-cpos+1, TagItemFlags ( item, Info ), Info, 0, 0, 0 );

    free ( temp );

    return 0;
}

/*
// Replace '_' by ' '
int SpaceFix ( char* item, FileInfo* Info )
{
    char* temp;
    char* s = TagValueU ( item, Info );
    char* d;

    if ( s == NULL ) {
        return 0;
    }

    if ( (temp = malloc ( strlen (s) + 1 )) == NULL ) {
        fprintf ( Options.output, "SpaceFix: Memory allocation failed.\n" );
        exit (1);
    }

    d = temp;
    do {
        char c = *s;
        if ( c == '_' ) c = ' ';
        *d++ = c;
    } while ( *s++ );

    fix_percentage_sequences ( temp );

    InsertTagFieldU ( item, 0, temp, 0, TagItemFlags ( item, Info ), Info, 0, 0, 0 );

    return 0;
}
*/

// Replace '_' by ' '
int FixSpaces ( FileInfo* Info )
{
    char*   temp;
    char*   s;
    char*   d;
    size_t  i;

    for ( i = 0; i < Info->TagItemCount; i++ ) {
        if ( Info->TagItems[i].ValueUSize > 0 ) {
            if ( (temp = malloc ( Info->TagItems[i].ValueUSize + 1 )) == NULL ) {
                fprintf ( Options.output, "SpaceFix: Memory allocation failed.\n" );
                exit (1);
            }

            s = Info->TagItems[i].ValueU;
            d = temp;
            do {
                char c = *s;
                if ( c == '_' ) c = ' ';
                *d++ = c;
            } while ( *s++ );

            fix_percentage_sequences ( temp );

            ReplaceTagFieldU ( i, temp, 0, Info->TagItems[i].Flags, Info );

            free ( temp );
        }
    }

    return 0;
}

// Fix item name case use in APE v1.0/2.0 tags
int ItemFix ( FileInfo* Info )
{
    const char* itemnames[] = {
        APE_TAG_FIELD_TITLE,
        APE_TAG_FIELD_SUBTITLE,
        APE_TAG_FIELD_ARTIST,
        APE_TAG_FIELD_ALBUM,
        APE_TAG_FIELD_DEBUTALBUM,
        APE_TAG_FIELD_PUBLISHER,
        APE_TAG_FIELD_CONDUCTOR,
        APE_TAG_FIELD_COMPOSER,
        APE_TAG_FIELD_COMMENT,
        APE_TAG_FIELD_YEAR,
        APE_TAG_FIELD_RECORDDATE,
        APE_TAG_FIELD_RECORDLOCATION,
        APE_TAG_FIELD_TRACK,
        APE_TAG_FIELD_GENRE,
        APE_TAG_FIELD_COVER_ART_FRONT,
        APE_TAG_FIELD_NOTES,
        APE_TAG_FIELD_LYRICS,
        APE_TAG_FIELD_COPYRIGHT,
        APE_TAG_FIELD_PUBLICATIONRIGHT,
        APE_TAG_FIELD_FILE,
        APE_TAG_FIELD_MEDIA,
        APE_TAG_FIELD_EANUPC,
        APE_TAG_FIELD_ISRC,
        APE_TAG_FIELD_RELATED_URL,
        APE_TAG_FIELD_ABSTRACT_URL,
        APE_TAG_FIELD_BIBLIOGRAPHY_URL,
        APE_TAG_FIELD_BUY_URL,
        APE_TAG_FIELD_ARTIST_URL,
        APE_TAG_FIELD_PUBLISHER_URL,
        APE_TAG_FIELD_FILE_URL,
        APE_TAG_FIELD_COPYRIGHT_URL,
        APE_TAG_FIELD_INDEX,
        APE_TAG_FIELD_INTROPLAY,
        APE_TAG_FIELD_MJ_METADATA,
        APE_TAG_FIELD_DUMMY,
    };

    size_t  i, j;

    for ( i = 0; i < Info->TagItemCount; i++ ) {
        for ( j = 0; j < sizeof (itemnames) / sizeof (*itemnames); j++ ) {
            if ( stricmp ( Info->TagItems[i].Item, itemnames[j] ) == 0 ) {
                strcpy ( Info->TagItems[i].Item, itemnames[j] );
                break;
            }
        }
    }

    return 0;
}

// Fix umlauts in tags (ae -> ä, oe -> ö, ue -> ü)
int UmlautFix ( FileInfo* Info )
{
    const char* itemnames[] = {
        APE_TAG_FIELD_TITLE,
        //APE_TAG_FIELD_SUBTITLE,
        APE_TAG_FIELD_ARTIST,
        APE_TAG_FIELD_ALBUM,
        //APE_TAG_FIELD_DEBUTALBUM,
        //APE_TAG_FIELD_PUBLISHER,
        //APE_TAG_FIELD_CONDUCTOR,
        //APE_TAG_FIELD_COMPOSER,
        //APE_TAG_FIELD_COMMENT,
        //APE_TAG_FIELD_RECORDLOCATION,
        //APE_TAG_FIELD_GENRE,
        //APE_TAG_FIELD_NOTES,
        //APE_TAG_FIELD_LYRICS,
        //APE_TAG_FIELD_COPYRIGHT,
        //APE_TAG_FIELD_PUBLICATIONRIGHT,
    };

    size_t  i, j;
    char*   temp;
    char*   s;
    char*   d;

    // check that tag items contain only ASCII characters
    for ( i = 0; i < Info->TagItemCount; i++ ) {
        for ( j = 0; j < sizeof (itemnames) / sizeof (*itemnames); j++ ) {
            if ( stricmp ( Info->TagItems[i].Item, itemnames[j] ) == 0 ) {
                // not for binary tags
                if ( Info->TagItems[i].Flags & 1<<1 )
                    break;

                // not plain ascii -> don't convert
                if ( Info->TagItems[i].ValueSize != Info->TagItems[i].ValueUSize )
                    return 0;
            }
        }
    }

    for ( i = 0; i < Info->TagItemCount; i++ ) {
        for ( j = 0; j < sizeof (itemnames) / sizeof (*itemnames); j++ ) {
            if ( stricmp ( Info->TagItems[i].Item, itemnames[j] ) == 0 ) {
                // not for binary tags
                if ( Info->TagItems[i].Flags & 1<<1 ) break;

                if ( (temp = malloc ( Info->TagItems[i].ValueSize + 1 )) == NULL ) {
                    fprintf ( Options.output, "UmlautFix: memory allocation failed.\n" );
                    exit (1);
                }

                s = Info->TagItems[i].Value;
                d = temp;

                do {
                    if ( (*(s+1) == 'e') || (*(s+1) == 'E') ) {
                        if ( *s == 'a' ) {
                            *d++ = 'ä'; s++;
                        } else
                        if ( *s == 'A' ) {
                            *d++ = 'Ä'; s++;
                        } else
                        if ( *s == 'o' ) {
                            *d++ = 'ö'; s++;
                        } else
                        if ( *s == 'O' ) {
                            *d++ = 'Ö'; s++;
                        } else
                        if ( *s == 'u' ) {
                            *d++ = 'ü'; s++;
                        } else
                        if ( *s == 'U' ) {
                            *d++ = 'Ü'; s++;
                        } else {
                            *d++ = *s;
                        }
                    } else {
                        *d++ = *s;
                    }
                } while ( *s++ );

                ReplaceTagField ( i, temp, 0, Info->TagItems[i].Flags, Info );

                free ( temp );
                break;
            }
        }
    }

    return 0;
}

// Swaps title <--> artist
int SwapTitleAndArtist ( FileInfo* Info )
{
    char*  a = TagValueU ( APE_TAG_FIELD_ARTIST, Info );
    char*  t = TagValueU ( APE_TAG_FIELD_TITLE , Info );
    char*  temp;
    size_t a_num;
    size_t t_num;
    size_t t_len;
    size_t t_flags;

    if ( a == NULL || t == NULL ) {
        return 0;
    }

    a_num = TagItemNum ( APE_TAG_FIELD_ARTIST, Info );
    t_num = TagItemNum ( APE_TAG_FIELD_TITLE , Info );

    if ( (temp = malloc ( Info->TagItems[a_num].ValueUSize + 1 )) == NULL ) {
        fprintf ( Options.output, "SwapTitleAndArtist: Memory allocation failed.\n" );
        exit (1);
    }

    t_len   = Info->TagItems[a_num].ValueUSize;
    t_flags = Info->TagItems[a_num].Flags;
    memcpy ( temp, Info->TagItems[a_num].ValueU, Info->TagItems[a_num].ValueUSize + 1 );

    InsertTagField ( APE_TAG_FIELD_ARTIST, Info->TagItems[a_num].ItemSize, t   , Info->TagItems[t_num].ValueUSize, Info->TagItems[t_num].Flags, Info, 0, 0, 0 );
    InsertTagField ( APE_TAG_FIELD_TITLE , Info->TagItems[a_num].ItemSize, temp, t_len                           , t_flags                    , Info, 0, 0, 0 );

    free ( temp );

    return 0;
}

// Checks if file extension is correct
int CorrectType ( const FileInfo* Info )
{
    if ( Options.NoExtensionCheck ) {
        return 1;
    }

    if ( FileFormat ( Info->Filename ) == unknown ) {
        return 0;
    }

    return 1;
}

// Creates all directories in path
int CreateDirectories ( const char* path )
{
    char        cwdpath[_MAX_PATH];
    char        temp[_MAX_PATH];
    const char* s = path;
    char*       d;

    _getcwd ( cwdpath, _MAX_PATH );

    while (1) {
        int dirfail = 0;
        d = temp;
        while ( *s && *s == ' ' ) s++;
        while ( *s && *s != '/' && *s != '\\' ) {
            *d++ = *s++;
        }
        if ( *s == '/' || *s == '\\' ) s++;
        *d = '\0';
        while ( d >= temp && *d == ' ' ) *d-- = '\0';
        ReplaceCharacters ( temp, &Replace );
        remove_unsupported_chars ( temp );
        if ( _mkdir ( temp ) != 0 ) {
            dirfail = 1;
            /*
            if ( errno != EEXIST ) {
                fprintf ( Options.output, "Directory creation failed.\n" );
                _chdir ( cwdpath );
                return 1;
            }
            */
        }
        if ( _chdir ( temp ) != 0 ) {
            if ( dirfail ) {
                fprintf ( Options.output, "Directory creation failed.\n" );
                _chdir ( cwdpath );
                return 1;
            } else {
                fprintf ( Options.output, "Directory changing failed.\n" );
                _chdir ( cwdpath );
                return 1;
            }
        }
        if ( *s == '\0' || temp[0] == '\0' ) break;
    };

    _chdir ( cwdpath );

    return 0;
}

// Decodes file, names output using scheme
int Decode ( FileInfo* Info )
{
    const char* supported = "Only mpc, mp3, aac, ape, flac and wavpack are supported.\n";
    char            outputname[_MAX_PATH + 4];
    char            inputname [_MAX_PATH + 4];
    const char*     args[10];
    enum format     format = FileFormat ( Info->Filename );

    if ( GenerateNameFromTag ( outputname+1, Options._Values.Scheme, Info ) != 0 ) {
        fprintf ( Options.output, "Name generation failed.\n" );
        return 1;
    } else {
        if ( !Options.TestMode ) {
            sprintf ( inputname, "\"%s\"", Info->Filename );
            printf ( "\nDecoding to: '%s.wav'\n", (char *)(outputname+1) );
            outputname[0] = '"';
            strcat ( outputname, ".wav\"" );
        } else {
            fprintf ( Options.output, "Name of decoded file: '%s.wav'\n", (char *)(outputname+1) );
        }
    }

    switch ( format ) {
    case musepack:
        args[0] = "mppdec";
        args[1] = inputname;
        args[2] = outputname;
        args[3] = NULL;
        _spawnvp ( _P_WAIT, args[0], args );
        break;

    case mp3:
        args[0] = "lame";
        args[1] = "--decode";
        args[2] = inputname;
        args[3] = outputname;
        args[4] = NULL;
        _spawnvp ( _P_WAIT, args[0], args );
        break;

    case ape:
        args[0] = "mac";
        args[1] = inputname;
        args[2] = outputname;
        args[3] = "-d";
        args[4] = NULL;
        _spawnvp ( _P_WAIT, args[0], args );
        break;

    case aac:
        args[0] = "faad";
        args[1] = "-o";
        args[2] = outputname;
        args[3] = inputname;
        args[4] = NULL;
        _spawnvp ( _P_WAIT, args[0], args );
        return 1;

    case flac:
        args[0] = "flac";
        args[1] = "-d";
        args[2] = "-o";
        args[3] = outputname;
        args[4] = inputname;
        args[5] = NULL;
        _spawnvp ( _P_WAIT, args[0], args );
        break;

    case wavpack:
        args[0] = "wvunpack";
        args[1] = inputname;
        args[2] = outputname;
        args[3] = NULL;
        _spawnvp ( _P_WAIT, args[0], args );
        break;

    case vorbis:
        args[0] = "oggenc";
        args[1] = "-o";
        args[2] = outputname;
        args[3] = inputname;
        args[4] = NULL;
        _spawnvp ( _P_WAIT, args[0], args );
        break;

    default:
        fprintf ( Options.output, supported );
        return 1;
        break;
    }

    return 0;
}

// Appends file to appendto
int append ( const char* file, const char* appendto )
{
    FILE*   apto;
    FILE*   from;
    int     ch;

    if ( (apto = fopen ( appendto, "ab" )) == NULL ) {
        fprintf ( Options.output, "Failed to open '%s' for appending.\n" );
        return 1;
    }

    if ( (from = fopen ( file, "rb" )) == NULL ) {
        fclose (apto);
        fprintf ( Options.output, "Failed to open '%s' for reading.\n" );
        return 1;
    }

    while ( (ch = fgetc (from)) != EOF ) {
        fputc ( ch, apto );
    }

    fclose (from);
    fclose (apto);

    return 0;
}

// Main ------------------------------------------------------------------------

int main ( size_t argc, char** argv )
{
    struct Errors {
        int     Errors;
        int     Warnings;
        int     FileNotFound;
        int     IncorrectExt;
        int     Protected;
    } Errors;

    FileInfo            File;           // Information about one file (tags, bitrate, size...)
    FileList            Files;          // List of files (name, path, attributes)
    SchemeList          Schemes;        // List of schemes used for tag field autodetection
    DetailedList        Details;        // Detailed list of files for playlist (needed tags + name + path )
    ExternalAlbum       External;       // External tag resource (read from file, possibly from freedb in future)
    struct _utimbuf     tFile;
    size_t              i;
    int                 view_tags;      // display tag contents
    int                 edit_tags;      // save tags
    int                 view_filename;  // display filename
    char                outputfilename[_MAX_PATH];
    char                nfofilename[_MAX_PATH];
    int                 nfoname_is_scheme;
    char                playlistfilename[_MAX_PATH];
    int                 playlistname_is_scheme;
    LPWSTR*             wideargv;
    size_t              newargc;
    char**              newargv;
    OSVERSIONINFO       OSversion;

    memset ( &Errors, 0, sizeof (Errors) );

    OSversion.dwOSVersionInfoSize = sizeof ( OSVERSIONINFO );
    GetVersionEx ( &OSversion );

    Options.UnicodeOS = ( OSversion.dwPlatformId == VER_PLATFORM_WIN32_NT );
    Options.output = stderr;

    print_header ();

    if ( argc < 2 ) {
        print_usage ( 0 );
        exit (1);
    }

	setlocale ( LC_ALL, "" );

    memset ( &Options   , 0, sizeof (Options   ) );
    memset ( &File      , 0, sizeof (File      ) );
    memset ( &Files     , 0, sizeof (Files     ) );
    memset ( &Schemes   , 0, sizeof (Schemes   ) );
    memset ( &Presets   , 0, sizeof (Presets   ) );
    memset ( &Exceptions, 0, sizeof (Exceptions) );
    memset ( &Replace   , 0, sizeof (Replace   ) );
    memset ( &ReplaceTag, 0, sizeof (ReplaceTag) );
    memset ( &NFOfile   , 0, sizeof (NFOfile   ) );
    memset ( &Details   , 0, sizeof (Details   ) );
    memset ( &External  , 0, sizeof (External  ) );

    Options.output = stderr;

    load_config ( argv[0], &Schemes, &Presets, &Exceptions, &Replace, &ReplaceTag, &NFOfile );

    // Convert UNICODE parameters to UTF-8 on NT based OS
    Options.UnicodeOS = ( OSversion.dwPlatformId == VER_PLATFORM_WIN32_NT );
    if ( Options.UnicodeOS ) {
        wideargv = CommandLineToArgvW ( GetCommandLineW(), &newargc );
        if ( wideargv == NULL ) {
            fprintf ( Options.output, "main: CommandLineToArgvW failed.\n" );
            exit (1);
        } else {
            if ( (newargv = (char **)malloc ( newargc * sizeof (char*) )) == NULL ) {
                fprintf ( Options.output, "main: Memory allocation failed.\n" );
                exit (1);
            }
            for ( i = 0; i < newargc; i++ ) {
                int     buffersize;
                int     in_utf8 = 0;
                if ( i > 0 ) {  // only convert wide character parameter to UTF-8 when it's tag value
                    if      ( 0 == strcmp ( argv[i-1], "-t"        ) ) in_utf8 = 1;
                    else if ( 0 == strcmp ( argv[i-1], "-u"        ) ) in_utf8 = 1;
                    else if ( 0 == strcmp ( argv[i-1], "--title"   ) ) in_utf8 = 1;
                    else if ( 0 == strcmp ( argv[i-1], "--artist"  ) ) in_utf8 = 1;
                    else if ( 0 == strcmp ( argv[i-1], "--album"   ) ) in_utf8 = 1;
                    else if ( 0 == strcmp ( argv[i-1], "--year"    ) ) in_utf8 = 1;
                    else if ( 0 == strcmp ( argv[i-1], "--comment" ) ) in_utf8 = 1;
                    else if ( 0 == strcmp ( argv[i-1], "--track"   ) ) in_utf8 = 1;
                    else if ( 0 == strcmp ( argv[i-1], "--genre"   ) ) in_utf8 = 1;
                }
                if ( in_utf8 ) {
                    if ( (buffersize = WideCharToMultiByte ( CP_UTF8, 0, wideargv[i], -1, NULL, 0, NULL, NULL )) == 0 ) {
                        fprintf ( Options.output, "main: WideCharToMultiByte failed.\n" );
                        exit (1);
                    }
                    if ( (newargv[i] = malloc( buffersize + 1 )) == NULL ) {
                        fprintf ( Options.output, "main: Memory allocation failed.\n" );
                        exit (1);
                    }
                    WideCharToMultiByte ( CP_UTF8, 0, wideargv[i], -1, newargv[i], buffersize + 1, NULL, NULL );
                } else {
                    newargv[i] = argv[i];
                }
            }
            GlobalFree ( wideargv );
        }
    } else {
        newargv = argv;
        newargc = argc;
    }

    Errors.FileNotFound = get_parameters ( newargc, newargv, &Files );

    if ( Files.FileCount == 0 ) {
        fprintf ( stderr, "No files.\n" );
        // if ( errors > 0 ) fprintf ( stderr, "ERRORS: %u\n", errors );
        exit (1);
    }

    if ( Options.OutputToFile ) {
        outputfilename[0] = '\0';
        if ( Options._Values.OutputFileName != NULL ) {
            char drive [_MAX_DRIVE];
            char dir   [_MAX_DIR  ];
            char fname [_MAX_FNAME];
            char ext   [_MAX_EXT  ];

            _splitpath ( Options._Values.OutputFileName, drive, dir, fname, ext );
            ReplaceCharacters ( fname, &Replace );
            ReplaceCharacters ( ext  , &Replace );
            remove_unsupported_chars ( fname );
            remove_unsupported_chars ( ext   );
            if ( Options.ReplaceSpaces ) {
                replace_spaces ( fname, '_' );
                replace_spaces ( ext,   '_' );
            }
            _makepath ( Options._Values.OutputFileName, drive, dir, fname, ext );

            if ( (Options.output = fopen ( Options._Values.OutputFileName, "at" )) == NULL ) {
                fprintf ( stderr, "Failed to open outputfile '%s' for appending.\n\n", Options._Values.OutputFileName );
                Options.output = stdout;
            }
        } else {
            if ( (Options._Values.OutputFileName = malloc ( _MAX_PATH )) == NULL ) {
                fprintf ( stderr, "main: Memory allocation failed.\n" );
                exit (1);
            }
            tmpnam ( Options._Values.OutputFileName );
            if ( (Options.output = fopen ( Options._Values.OutputFileName, "wt" )) == NULL ) {
                fprintf ( stderr, "Failed to open temporary outputfile '%s' for writing.\n\n", Options._Values.OutputFileName );
                Options.output = stdout;
                free ( Options._Values.OutputFileName );
                Options._Values.OutputFileName = NULL;
            }
        }
    }

    if ( Options.NfoFile ) {
        char*   p = Options._Values.NFOname;

        nfofilename[0]    = '\0';
        nfoname_is_scheme = 0;
        while ( *p ) {
            char c = *p++;
            if ( c == '%' || c == '*' ) { // scheme used in name
                nfoname_is_scheme = 1;
                break;
            }
        }
    }

    if ( Options.PlaylistName ) {
        char*   p = Options._Values.PlaylistName;

        playlistfilename[0]    = '\0';
        playlistname_is_scheme = 0;
        while ( *p ) {
            char c = *p++;
            if ( c == '%' || c == '*' ) { // scheme used in name
                playlistname_is_scheme = 1;
                break;
            }
        }
    }

    if ( Options.TestMode ) {
        fprintf ( Options.output, "Test mode activated, no changes to files will be made.\n\n" );
    }

    if ( (Details.Files = (DetailsOfFile *)malloc ( sizeof (*Details.Files) * Files.FileCount )) == NULL ) {
        fprintf ( Options.output, "main: Memory allocation failed.\n" );
        exit (1);
    }
    Details.SortValueCount = Options._Values.SortCount;

    view_tags     = 1;
    edit_tags     = 0;
    view_filename = 1;

    if ( Options.Remove ) {
        view_tags = 0;
        edit_tags = 0;
    }
    if ( Options.RemoveID3v2 || Options.RemoveUnneededID3v2 ) {
        view_tags = 0;
        edit_tags = 0;
    }
    if ( Options.Auto )
        edit_tags = 1;
    if ( Options.FromFile )
        edit_tags = 1;
    if ( Options.CaseConversion != Unmodified )
        edit_tags = 1;
    if ( Options.TagCaseConv )
        edit_tags = 1;
    if ( Options.CommaFix && !Options.Remove )
        edit_tags = 1;
    if ( Options.SpaceFix && !Options.Remove )
        edit_tags = 1;
    if ( Options.ItemFix && !Options.Remove )
        edit_tags = 1;
    if ( Options.UmlautFix && !Options.Remove )
        edit_tags = 1;
    if ( Options.Force )
        edit_tags = 1;
    if ( Options._Values.TagFieldCount > 0 )
        edit_tags = 1;
    //if ( Options.NoLeadingZero )
    //    edit_tags = 1;
    if ( Options.ZeroPad )
        edit_tags = 1;
    if ( Options.OnePlaylist )
        view_tags = 0;
    if ( Options.PlaylistPerDir )
        view_tags = 0;
    if ( Options.PlaylistPerAlbum )
        view_tags = 0;
    if ( Options.RenameFromName )
        view_tags = 0;
    if ( Options.RenameFromTag )
        view_tags = 0;
    if ( Options.SwapTitleAndArtist )
        edit_tags = 1;

    if ( edit_tags )
        view_tags = 1;

    if ( Options.HideNames )
        view_filename = 0;

    // main loop, everything happens here
    for ( i = 0; i < Files.FileCount; i++ ) {
        int     correct_type;
        int     external_source = 0;
        size_t  filecount;
        size_t  n;

        strcpy ( File.Filename, Files.Files[i].Path );
        strcat ( File.Filename, Files.Files[i].Info.name );

        if ( view_filename ) {
            char    asciiname[_MAX_PATH*2];
            string_l2a ( File.Filename, asciiname );
            fprintf ( Options.output, "%s\n", asciiname );
        }

        correct_type = CorrectType ( &File );

        if ( !correct_type ) {
            int error;
            error = LoadTagIni ( File.Filename, &External );        // try to load as Tag.ini
            if ( error == 1 ) {
                error = LoadCDDBFile ( File.Filename, &External );  // try to load as CDDB file
            }
            if ( error == 0 ) {     // tag information comes from external source
                external_source = 1;
                fprintf ( Options.output, "Using file as source for tags.\n" );
            } else {
                Errors.IncorrectExt++;
                fprintf ( Options.output, "Incorrect extension. Use option --nocheck to force processing.\n\n" );
                continue;
            }
        }

        if ( !external_source ) {
            filecount = 1;
        } else {
            edit_tags = 1;
            filecount = External.TrackCount;

            if ( External.Artist != NULL )
                fprintf ( Options.output, "Artist: %s\n", External.Artist     );
            if ( External.Album != NULL )
                fprintf ( Options.output, "Album:  %s\n", External.Album      );
            if ( External.Year != NULL )
                fprintf ( Options.output, "Year:   %s\n", External.Year       );
            if ( External.Genre != NULL )
                fprintf ( Options.output, "Genre:  %s\n", External.Genre      );
            fprintf ( Options.output, "Tracks: %u\n", External.TrackCount );
            fprintf ( Options.output, "\n" );
        }

        for ( n = 0; n < filecount; n++ ) {
            if ( external_source ) {
                char                asciiname[_MAX_PATH*2];
                char                temp[_MAX_PATH];
                struct _finddata_t  fd;
                long                hFile;
                int                 file_found = 0;

                sprintf ( temp, "%sTrack%02u.*", Files.Files[i].Path, n + 1 );
                File.Filename[0] = '\0';

                if ( (hFile = _findfirst ( temp, &fd )) != -1L ) {
                    if ( !(fd.attrib & _A_SUBDIR) ) {
                        strcpy ( File.Filename, Files.Files[i].Path );
                        strcat ( File.Filename, fd.name );
                        if ( CorrectType ( &File ) ) {
                            file_found = 1;
                            memcpy ( &Files.Files[i].Info, &fd, sizeof (fd) );
                        }
                    }

                    if ( !file_found ) {
                        while ( _findnext ( hFile, &fd ) == 0 ) {
                            if ( !(fd.attrib & _A_SUBDIR) ) {
                                strcpy ( File.Filename, Files.Files[i].Path );
                                strcat ( File.Filename, fd.name );
                                if ( CorrectType ( &File ) ) {
                                    file_found = 1;
                                    memcpy ( &Files.Files[i].Info, &fd, sizeof (fd) );
                                    break;
                                }
                            }
                        }
                    }

                    _findclose ( hFile );
                } else {
                    string_l2a ( temp, asciiname );
                    Errors.FileNotFound++;
                    fprintf ( Options.output, "File '%s' not found.\n\n", asciiname );
                    continue;
                }
                if ( !file_found ) {
                    string_l2a ( temp, asciiname );
                    Errors.Errors++;
                    fprintf ( Options.output, "Couldn't find supported '%s' file.\n\n", asciiname );
                    continue;
                }

                if ( view_filename ) {
                    string_l2a ( File.Filename, asciiname );
                    fprintf ( Options.output, "%s\n", asciiname );
                }
            }

            FreeTagFields ( &File );
            if ( ReadTags ( &File ) != 0 ) {
                Errors.Errors++;
                fprintf ( Options.output, "ReadTags failed.\n" );
            }
            if ( !Allowed ( &File ) ) {
                fprintf ( Options.output, "Skipped...\n\n" );
                continue;
            }

            if ( external_source ) {
                InsertExternalFields ( &File, &External, n );
            }

            if ( Options.FromFile ) {
                FileInfo FromFile;
                memset ( &FromFile, 0, sizeof (FromFile) );

                strcpy ( FromFile.Filename, Options._Values.TagSourceFile );

                if ( ReadTags ( &FromFile ) != 0 ) {
                    Errors.Errors++;
                    fprintf ( Options.output, "ReadTags failed.\n" );
                } else {
                    size_t t, tf;

                    // copy all new tag items from FromFile to File
                    for ( tf = 0; tf < FromFile.TagItemCount; tf++ ) {
                        int item_exists = 0;

                        if ( FromFile.TagItems[tf].Item == NULL || FromFile.TagItems[tf].ItemSize == 0 || FromFile.TagItems[tf].Item[0] == '\0' )
                            continue;

                        for ( t = 0; t < File.TagItemCount; t++ ) {
                            if ( stricmp ( File.TagItems[t].Item, FromFile.TagItems[tf].Item ) == 0 ) {
                                item_exists = 1;
                                break;
                            }
                        }

                        if ( !item_exists ) {
                            if ( FromFile.TagItems[tf].Flags & 1<<1 ) { // binary
                                InsertTagFieldB ( FromFile.TagItems[tf].Item, FromFile.TagItems[tf].ItemSize, FromFile.TagItems[tf].Value, FromFile.TagItems[tf].ValueSize, FromFile.TagItems[tf].Flags, &File, 1, 0, 0 );
                            } else {
                                if ( FromFile.TagItems[tf].ValueUSize > 0 )
                                    InsertTagFieldU ( FromFile.TagItems[tf].Item, FromFile.TagItems[tf].ItemSize, FromFile.TagItems[tf].ValueU, FromFile.TagItems[tf].ValueUSize, FromFile.TagItems[tf].Flags, &File, 1, 0, 0 );
                                else
                                    InsertTagField ( FromFile.TagItems[tf].Item, FromFile.TagItems[tf].ItemSize, FromFile.TagItems[tf].Value, FromFile.TagItems[tf].ValueSize, FromFile.TagItems[tf].Flags, &File, 1, 0, 0 );
                            }
                        }
                    }
                }

                FreeTagFields ( &FromFile );

                /*
                char        backupname[_MAX_PATH];
                long        backupoffset;
                long        backupsize;
                char*       title;
                char*       track;
                char*       p;
                FileDetails backupdetails;

                backupoffset = File.TagOffset;
                backupsize   = File.FileSize;
                memcpy ( &backupdetails, &File.Details, sizeof (FileDetails) );
                strcpy ( backupname, File.Filename );

                if ( (p = TagValueU ( APE_TAG_FIELD_TITLE, &File )) && (p[0] != '\0') ) {
                    if ( (title = malloc ( strlen (p) + 1 )) == NULL ) {
                        fprintf ( Options.output, "main: Memory allocation failed.\n" );
                        exit (1);
                    }
                    strcpy ( title, p );
                } else {
                    title = NULL;
                }

                if ( (p = TagValueU ( APE_TAG_FIELD_TRACK, &File )) && (p[0] != '\0') ) {
                    if ( (track = malloc ( strlen (p) + 1 )) == NULL ) {
                        fprintf ( Options.output, "main: Memory allocation failed.\n" );
                        exit (1);
                    }
                    strcpy ( track, p );
                } else {
                    track = NULL;
                }

                strcpy ( File.Filename, Options._Values.TagSourceFile );
                FreeTagFields ( &File );
                if ( ReadTags ( &File ) != 0 ) {
                    Errors.Errors++;
                    fprintf ( Options.output, "ReadTags failed.\n" );
                } else {
                    if ( title != NULL ) {
                        InsertTagFieldU ( APE_TAG_FIELD_TITLE, 0, title, 0, 0, &File, 0, 0, 0 );
                    }
                    if ( track != NULL ) {
                        InsertTagFieldU ( APE_TAG_FIELD_TRACK, 0, track, 0, 0, &File, 0, 0, 0 );
                    }
                }

                free ( title );
                free ( track );

                File.TagOffset = backupoffset;
                File.FileSize  = backupsize;
                memcpy ( &File.Details, &backupdetails, sizeof (FileDetails) );
                strcpy ( File.Filename, backupname );
                */
            }

            if ( !Options.TestMode && !Options.UpdateTime ) {   // if not testmode save original time stamp
                tFile.actime  = Files.Files[i].Info.time_access;
                tFile.modtime = Files.Files[i].Info.time_write;
            }

            if ( Options.Remove ) {
                if ( !Options.TestMode ) {
                    if ( Files.Files[i].Info.attrib & _A_RDONLY ) {     // write protected
                        Errors.Protected++;
                        fprintf ( Options.output, "File is read-only, can't remove tags.\n" );
                    } else {
                        Errors.Errors += RemoveTags ( &File );
                    }
                } else {
                    fprintf ( Options.output, "No tags were removed.\n" );
                }
            }
#ifdef  ID3V2SUPPORT
            if ( Options.RemoveID3v2 ) {
                if ( !Options.TestMode ) {
                    if ( Files.Files[i].Info.attrib & _A_RDONLY ) {     // write protected
                        Errors.Protected++;
                        fprintf ( Options.output, "File is read-only, can't remove tags.\n" );
                    } else {
                        Errors.Errors += RemoveID3v2Tag ( File.Filename, &File, 0 );
                    }
                } else {
                    fprintf ( Options.output, "No tags were removed.\n" );
                }
            }
            if ( Options.RemoveUnneededID3v2 ) {
                char*   p;
                size_t  k;
                int     needed = 0;
                int     hasid3 = 0;

                for ( k = 0; k < File.TagCount; k++ ) {
                    if ( File.Tags[k].TagType == ID3v2_tag ) {
                        hasid3 = 1;
                        break;
                    }
                }

                if ( hasid3 ) {
                    enum format format = FileFormat ( File.Filename );

                    if ( format == mp3 || format == aac || format == flac ) {
                        p = TagValue ( APE_TAG_FIELD_TITLE, &File );
                        if ( p && strlen (p) > 30 ) needed = 1;
                        if ( !needed ) {
                            p = TagValue ( APE_TAG_FIELD_ARTIST, &File );
                            if ( p && strlen (p) > 30 ) needed = 1;
                        }
                        if ( !needed ) {
                            p = TagValue ( APE_TAG_FIELD_ALBUM, &File );
                            if ( p && strlen (p) > 30 ) needed = 1;
                        }
                        if ( !needed ) {
                            p = TagValue ( APE_TAG_FIELD_YEAR, &File );
                            if ( p && strlen (p) > 4 ) needed = 1;
                        }
                        if ( !needed ) {
                            p = TagValue ( APE_TAG_FIELD_TRACK, &File );
                            if ( p ) {
                                while ( *p ) {
                                    if ( *p < '0' || *p > '9' ) {
                                        needed = 1;
                                        break;
                                    }
                                    p++;
                                }

                                p = TagValue ( APE_TAG_FIELD_TRACK, &File );
                                if ( p && (atoi(p) < 0 || atoi(p) > 255) ) needed = 1;
                            }
                        }
                        if ( !needed ) {
                            p = TagValue ( APE_TAG_FIELD_COMMENT, &File );
                            if ( (p && TagValue (APE_TAG_FIELD_TRACK, &File)) && strlen (p) > 28 ) needed = 1;
                            if ( (p && !TagValue (APE_TAG_FIELD_TRACK, &File)) && strlen (p) > 30 ) needed = 1;
                        }
                        if ( !needed ) {
                            p = TagValue ( APE_TAG_FIELD_GENRE, &File );
                            if ( p && strcmp (p, "") != 0 && GenreToInteger (p) >= ID3V1GENRES ) needed = 1;
                        }
                    }

                    if ( !Options.TestMode ) {
                        if ( Files.Files[i].Info.attrib & _A_RDONLY ) {     // write protected
                            Errors.Protected++;
                            fprintf ( Options.output, "File is read-only, can't remove tags.\n" );
                        } else {
                            if ( !needed ) {
                                Errors.Errors += RemoveID3v2Tag ( File.Filename, &File, 0 );
                            } else {
                                fprintf ( Options.output, "ID3v2 tag is needed.\n" );
                            }
                        }
                    } else {
                        if ( !needed ) {
                            fprintf ( Options.output, "No tags were removed.\n" );
                        } else {
                            fprintf ( Options.output, "ID3v2 tag is needed.\n" );
                        }
                    }
                }
            }
#endif  // ID3V2SUPPORT

            if ( Options.Auto || Options.RenameFromName ) {
                if ( Options.AutoScheme || Options._Values.Scheme == NULL ) {
                    if ( GuessTag ( &File, &Schemes ) != 0 ) {
                        Errors.Errors++;
                        fprintf ( Options.output, "Tag guessing failed.\n" );
                    }
                } else {
                    if ( GenerateTagFromName ( File.Filename, Options._Values.Scheme, &File ) != 0 ){
                        Errors.Errors++;
                        fprintf ( Options.output, "Tag generation failed.\n" );
                    }
                }
            }

            ModifyTag ( &File );
            if ( Options.NoLeadingZero ) {
                RemoveLeadingZeros ( &File );
            }
            if ( Options.ZeroPad ) {
                ZeroPadTrack ( &File );
            }
            if ( Options.CommaFix ) {
                CommaFix ( APE_TAG_FIELD_TITLE,  &File );
                CommaFix ( APE_TAG_FIELD_ARTIST, &File );
            }
            if ( Options.SpaceFix ) {
                FixSpaces ( &File );
                // SpaceFix ( APE_TAG_FIELD_TITLE,  &File );
                // SpaceFix ( APE_TAG_FIELD_ARTIST, &File );
            }
            if ( Options.ItemFix ) {
                ItemFix ( &File );
            }
            if ( Options.UmlautFix ) {
                UmlautFix ( &File );
            }
            if ( Options.SwapTitleAndArtist ) {
                SwapTitleAndArtist ( &File );
            }

            if ( Options.CaseConversion != Unmodified ) {
                size_t j;

                for ( j = 0; j < File.TagItemCount; j++ ) {
                    char*   value;
                    char    str [2];
                    str[1] = '\0';
                    if ( File.TagItems[j].ValueSize > 0 && !(File.TagItems[j].Flags & 1<<1) ) {
                        if ( (value = malloc ( File.TagItems[j].ValueSize + 1 )) == NULL ) {
                            fprintf ( Options.output, "main: Memory allocation failed.\n" );
                            exit (1);
                        }
                        memcpy ( value, File.TagItems[j].Value, File.TagItems[j].ValueSize + 1 );

                        switch ( Options.CaseConversion ) {
                        case Capitalize:
                            str[0] = value[0];
                            capitalize_case ( value );
                            exceptions_in_case ( value, &Exceptions );
                            upper_case ( str );
                            value[0] = str[0];
                            break;
                        case Sentence:
                            str[0] = value[0];
                            sentence_case ( value );
                            exceptions_in_case ( value, &Exceptions );
                            upper_case ( str );
                            value[0] = str[0];
                            break;
                        case Lower:
                            lower_case ( value );
                            break;
                        case Upper:
                            upper_case ( value );
                            break;
                        default:
                            break;
                        }

                        ReplaceTagField ( j, value, File.TagItems[j].ValueSize, File.TagItems[j].Flags, &File );

                        free ( value );
                    }
                }
            }

            if ( Options.TagCaseConv ) {
                size_t j, k;

                for ( j = 0; j < File.TagItemCount; j++ ) {
                    char*       value;
                    char        str [2];
                    enum case_t conversion = 0;
                    str[1] = '\0';

                    if ( File.TagItems[j].ValueSize > 0 && !(File.TagItems[j].Flags & 1<<1) ) {
                        if ( !conversion ) {
                            for ( k = 0; k < Options._Values.IcapsCount; k++ ) {
                                if ( stricmp ( Options._Values.Items_caps[k], File.TagItems[j].Item ) == 0 ) {
                                    conversion = Capitalize;
                                    break;
                                }
                            }
                        }
                        if ( !conversion ) {
                            for ( k = 0; k < Options._Values.ICapsCount; k++ ) {
                                if ( stricmp ( Options._Values.Items_Caps[k], File.TagItems[j].Item ) == 0 ) {
                                    conversion = Sentence;
                                    break;
                                }
                            }
                        }
                        if ( !conversion ) {
                            for ( k = 0; k < Options._Values.IlowerCount; k++ ) {
                                if ( stricmp ( Options._Values.Items_lower[k], File.TagItems[j].Item ) == 0 ) {
                                    conversion = Lower;
                                    break;
                                }
                            }
                        }
                        if ( !conversion ) {
                            for ( k = 0; k < Options._Values.IupperCount; k++ ) {
                                if ( stricmp ( Options._Values.Items_upper[k], File.TagItems[j].Item ) == 0 ) {
                                    conversion = Upper;
                                    break;
                                }
                            }
                        }
                        if ( !conversion ) continue;

                        if ( (value = malloc ( File.TagItems[j].ValueSize + 1 )) == NULL ) {
                            fprintf ( Options.output, "main: Memory allocation failed.\n" );
                            exit (1);
                        }
                        memcpy ( value, File.TagItems[j].Value, File.TagItems[j].ValueSize + 1 );

                        switch ( conversion ) {
                        case Capitalize:
                            str[0] = value[0];
                            capitalize_case ( value );
                            exceptions_in_case ( value, &Exceptions );
                            upper_case ( str );
                            value[0] = str[0];
                            break;
                        case Sentence:
                            str[0] = value[0];
                            sentence_case ( value );
                            exceptions_in_case ( value, &Exceptions );
                            upper_case ( str );
                            value[0] = str[0];
                            break;
                        case Lower:
                            lower_case ( value );
                            break;
                        case Upper:
                            upper_case ( value );
                            break;
                        default:
                            break;
                        }

                        ReplaceTagField ( j, value, File.TagItems[j].ValueSize, File.TagItems[j].Flags, &File );

                        free ( value );
                    }
                }
            }

            if ( ReadFileInfo ( &File ) != 0 ) {
                Errors.Errors++;
                fprintf ( Options.output, "ReadFileInfo failed.\n" );
            }
            if ( view_tags ) {
                if ( !Options.HideFileInfo ) {
                    if ( ViewFileInfo ( &File ) != 0 ) {
                        fprintf ( Options.output, "ViewFileInfo failed.\n" );
                    }
                }
                if ( !Options.HideTags ) {
                    if ( PrintTag ( &File ) != 0 ) {
                        fprintf ( Options.output, "PrintTags failed.\n" );
                    }
                }
            }

            if ( edit_tags ) {
                if ( Files.Files[i].Info.attrib & _A_RDONLY ) {     // write protected
                    Errors.Protected++;
                    fprintf ( Options.output, "File is read-only, can't save new tag.\n" );
                } else {
                    if ( File.TagItems == 0 ) {
                        fprintf ( Options.output, "Empty tag, not written.\n" );
                    } else {
                        if ( !Options.TestMode ) {
                            if ( Options.Force ) {  // user has specified tag types
                                size_t j;
                                free ( File.Tags );
                                File.TagCount = 0;
                                if ( (File.Tags = malloc ( sizeof (*File.Tags) * Options._Values.ForceTagCount )) == NULL ) {
                                    fprintf ( Options.output, "main: Memory allocation failed.\n" );
                                    exit (1);
                                }
                                for ( j = 0; j < Options._Values.ForceTagCount; j++ ) {
                                    File.Tags[File.TagCount].TagOffset = File.TagOffset;
                                    File.Tags[File.TagCount].TagSize   = 0;
                                    if ( 0 == stricmp ( Options._Values.ForceTags[j], "ID3" ) ||
                                         0 == stricmp ( Options._Values.ForceTags[j], "ID3v1" ) ) {
                                        File.Tags[File.TagCount++].TagType = ID3v1_tag;
                                    } else
                                    if ( 0 == stricmp  ( Options._Values.ForceTags[j], "APE"       ) ||
                                         0 == strnicmp ( Options._Values.ForceTags[j], "APE1",   4 ) ||
                                         0 == strnicmp ( Options._Values.ForceTags[j], "APEv1",  5 ) ||
                                         0 == strnicmp ( Options._Values.ForceTags[j], "APE v1", 6 ) ) {
                                        File.Tags[File.TagCount++].TagType = APE1_tag;
                                    } else
                                    if ( 0 == strnicmp ( Options._Values.ForceTags[j], "APE2",   4 ) ||
                                         0 == strnicmp ( Options._Values.ForceTags[j], "APEv2",  5 ) ||
                                         0 == strnicmp ( Options._Values.ForceTags[j], "APE v2", 6 ) ) {
                                        File.Tags[File.TagCount++].TagType = APE2_tag;
                                    } else
                                    if ( 0 == stricmp ( Options._Values.ForceTags[j], "Ogg"    ) ||
                                         0 == stricmp ( Options._Values.ForceTags[j], "Vorbis" ) ) {
                                        File.Tags[File.TagCount++].TagType = Vorbis_tag;
                                    } else
                                    if ( 0 == stricmp ( Options._Values.ForceTags[j], "FLAC" ) ) {
                                        File.Tags[File.TagCount++].TagType = FLAC_tag;
                                    } else
                                    if ( 0 == strnicmp ( Options._Values.ForceTags[j], "Lyrics", 6 ) ) {
                                        File.Tags[File.TagCount++].TagType = Lyrics3_tag;
                                    } else
                                    {
                                        Errors.Errors++;
                                        fprintf ( Options.output, "Unsupported tag: %s\n", Options._Values.ForceTags[j] );
                                    }
                                }
                            }

                            if ( !Options.Force && File.TagCount == 0 ) {   // get best tag
                                const SupportedTags*    t      = BestTagList;
                                enum format             format = FileFormat ( File.Filename );
                                enum tag_t              tag    = ID3v1_tag;

                                if ( format == musepack && Options.DefaultAPE2 ) {
                                    tag = APE2_tag;
                                } else {
                                    while ( t->Format >= 0 && t->Tag >= 0 ) {
                                        if ( format == t->Format ) {
                                            tag = t->Tag;
                                            break;
                                        }
                                        t++;
                                    }
                                }

                                if ( (File.Tags = (TagInfo *)realloc ( File.Tags, sizeof (TagInfo) * (File.TagCount + 1) )) == NULL ) {
                                    fprintf ( Options.output, "main: Memory allocation failed.\n" );
                                    exit (1);
                                }
                                File.Tags[File.TagCount].TagOffset = File.FileSize;
                                File.Tags[File.TagCount].TagSize   = 0;
                                File.Tags[File.TagCount].TagType   = tag;
                                File.TagCount++;
                            }

                            if ( File.TagCount > 0 ) {  // there are tags to be written
                                if ( Options.OldType || Options.Force ) {   // write specific tag(s)
                                    if ( WriteTags ( &File ) != 0 ) {
                                        Errors.Errors++;
                                    }
                                } else {                                    // write most suitable tag
                                    const SupportedTags*    t      = BestTagList;
                                    enum format             format = FileFormat ( File.Filename );
                                    enum tag_t              tag    = ID3v1_tag;

                                    if ( format == musepack && Options.DefaultAPE2 ) {
                                        tag = APE2_tag;
                                    } else {
                                        while ( t->Format >= 0 && t->Tag >= 0 ) {
                                            if ( format == t->Format ) {
                                                tag = t->Tag;
                                                break;
                                            }
                                            t++;
                                        }
                                    }

                                    if ( File.TagCount > 0 ) {  // check if file has better tag than is about to be written
                                        size_t  j;
                                        for ( j = 0; j < File.TagCount; j++ ) {
                                            if ( File.Tags[j].TagType == APE2_tag )
                                                tag = APE2_tag;
                                            if ( File.Tags[j].TagType == APE1_tag && tag == ID3v1_tag )
                                                tag = APE1_tag;
                                            if ( File.Tags[j].TagType == Vorbis_tag && tag == ID3v1_tag )
                                                tag = Vorbis_tag;
                                        }
                                    }

                                    if ( WriteTag ( &File, tag ) != 0 ) {
                                        Errors.Errors++;
                                    }
                                }
                            } else {
                                fprintf ( Options.output, "No tags to write.\n" );
                            }
                        }
                    } // empty tag
                }
            }

            if ( Options.RenameFromName || Options.RenameFromTag ) {
                const char* scheme;
                char name[_MAX_PATH];
                char path[_MAX_PATH];
                char ext [_MAX_EXT];
                char newname[_MAX_PATH];

                if ( Options.RenameFromName ) {
                    scheme = Options._Values.RenameToScheme;
                } else
                if ( Options.RenameFromTag ) {
                    scheme = Options._Values.Scheme;
                }

                if ( GenerateNameFromTag ( name, scheme, &File ) != 0 ) {
                    fprintf ( Options.output, "Name generation failed.\n" );
                } else {
                    _splitpath ( File.Filename, NULL, NULL, NULL, ext );
                    if ( Options.ReplaceSpaces ) {
                        replace_spaces ( name, '_' );
                        replace_spaces ( ext,  '_' );
                    }
                    strcat ( name, ext );
                    if ( strcmp ( name, Files.Files[i].Info.name ) == 0 ) { // same name
                        fprintf ( Options.output, "Name already matches scheme.\n" );
                    } else {
                        int failure = 0;

                        sprintf ( newname, "%s%s", Files.Files[i].Path, name );
                        if ( !Options.TestMode ) {
                            if ( Options.RenameOverwrite ) {
                                remove ( newname );
                            }
                            if ( rename ( File.Filename, newname ) != 0 ) {
                                Errors.Errors++;
                                fprintf ( Options.output, "File renaming failed.\n" );
                                failure = 1;
                            }
                        }

                        if ( !failure ) {
                            char asciiname[_MAX_PATH*2];
                            strcpy ( Files.Files[i].Info.name, name );
                            strcpy ( File.Filename, newname );
                            string_l2a ( name, asciiname );
                            if ( !Options.TestMode ) {
                                fprintf ( Options.output, "File renamed to: %s\n", asciiname );
                            } else {
                                fprintf ( Options.output, "New name: %s\n", asciiname );
                            }
                        }
                    }
                }

                if ( Options.RenameMove ) {
                    if ( GeneratePathFromTag ( path, scheme, &File ) != 0 ) {
                        Errors.Errors++;
                        fprintf ( Options.output, "Path generation failed.\n" );
                    } else {
                        if ( path[0] != '\0' ) {
                            char cwdpath[_MAX_PATH];
                            int  failure = 0;

                            if ( path[strlen (path)-1] != '/' && path[strlen (path)-1] != '\\' ) {
                                strcat ( path, "\\" );
                            }
                            sprintf ( newname, "%s%s%s", Files.Files[i].Path, path, name );

                            if ( !Options.TestMode ) {
                                _getcwd ( cwdpath, _MAX_PATH );
                                _chdir ( Files.Files[i].Path );
                                CreateDirectories ( path );
                                _chdir ( cwdpath );

                                if ( Options.RenameOverwrite ) {
                                    remove ( newname );
                                }
                                if ( rename ( File.Filename, newname ) != 0 ) {
                                    Errors.Errors++;
                                    fprintf ( Options.output, "File moving failed.\n" );
                                    failure = 1;
                                }
                            }

                            if ( !failure ) {
                                char asciipath[_MAX_PATH*2];
                                strcat ( Files.Files[i].Path, path );
                                string_l2a ( Files.Files[i].Path, asciipath );
                                if ( !Options.TestMode ) {
                                    fprintf ( Options.output, "File moved to: %s\n", asciipath );
                                } else {
                                    fprintf ( Options.output, "New path: %s\n", asciipath );
                                }
                            }
                        }
                    }
                }
            }

            if ( !Options.TestMode && !Options.UpdateTime ) {   // restore file's time stamp
                if ( Options.Remove || Options.RemoveID3v2 || edit_tags ) {
                    if ( !(Files.Files[i].Info.attrib & _A_RDONLY) ) {  // not write protected
                        if ( _utime ( File.Filename, &tFile ) != 0 ) {
                            Errors.Errors++;
                            fprintf ( Options.output, "File time stamp modification failed.\n" );
                        }
                    }
                }
            }

            if ( Options.Decode ) {
                Errors.Errors += Decode ( &File );
            }

            if ( Options._Values.OutputFileScheme != NULL && outputfilename[0] == '\0' ) {
                if ( GenerateNameFromTag ( outputfilename, Options._Values.OutputFileScheme, &File ) != 0 ) {
                    Errors.Errors++;
                    fprintf ( Options.output, "Output file name generation failed.\n" );
                } else {
                    if ( Options.ReplaceSpaces ) {
                        replace_spaces ( outputfilename, '_' );
                    }
                    if ( Options._Values.OutputFileExtension != NULL ) {
                        strcat ( outputfilename, Options._Values.OutputFileExtension );
                    } else {
                        strcat ( outputfilename, ".txt" );
                    }
                }
            }

            if ( Options.NfoFile && nfoname_is_scheme && nfofilename[0] == '\0' ) {
                if ( GenerateNameFromTag ( nfofilename, Options._Values.NFOname, &File ) != 0 ) {
                    Errors.Errors++;
                    fprintf ( Options.output, ".nfo file name generation failed.\n" );
                    strcpy ( nfofilename, Options._Values.NFOname );
                    remove_unsupported_chars ( nfofilename );
                } else {
                    if ( Options.ReplaceSpaces ) {
                        replace_spaces ( nfofilename, '_' );
                    }
                }
            }

            if ( Options.PlaylistName && playlistname_is_scheme && playlistfilename[0] == '\0' ) {
                if ( GenerateNameFromTag ( playlistfilename, Options._Values.PlaylistName, &File ) != 0 ) {
                    Errors.Errors++;
                    fprintf ( Options.output, "Playlist file name generation failed.\n" );
                    strcpy ( playlistfilename, Options._Values.PlaylistName );
                }
            }

            if ( !external_source ) {
                Errors.Errors += AddToDetailedList ( &File, &Files.Files[i], &Details );
            }

            if ( view_filename ) {
                fprintf ( Options.output, "\n" );
            }

            if ( Options.TrackInc ) {
                IncreaseTrackNumber ();
            }
        }
    }

    if ( Options.PlaylistPerDir || Options.OnePlaylist || Options.PlaylistPerAlbum || Options.NfoFile ) {
        if ( Options.Sort ) {
            qsort ( Details.Files, Details.FileCount, sizeof (*Details.Files), _compare );
        }
    }

    if ( Options.PlaylistPerDir ) {
        if ( Options.PlaylistName && playlistname_is_scheme ) {
            WritePlaylistPerDir ( playlistfilename, &Details );
        } else {
            WritePlaylistPerDir ( Options._Values.PlaylistName, &Details );
        }
    }
    if ( Options.PlaylistPerAlbum ) {
        if ( Options.PlaylistName && playlistname_is_scheme ) {
            WritePlaylistPerAlbum ( playlistfilename, &Details );
        } else {
            WritePlaylistPerAlbum ( Options._Values.PlaylistName, &Details );
        }
    }
    if ( Options.OnePlaylist ) {
        if ( Options.PlaylistName && playlistname_is_scheme ) {
            WriteOnePlaylist ( playlistfilename, &Details );
        } else {
            WriteOnePlaylist ( Options._Values.PlaylistName, &Details );
        }
    }
    if ( Options.NfoFile ) {
        if ( nfoname_is_scheme ) {
            WriteNFOfile ( nfofilename, &NFOfile, &Details, &File, &Exceptions );
        } else {
            WriteNFOfile ( Options._Values.NFOname, &NFOfile, &Details, &File, &Exceptions );
        }
    }

    free_FileInfo         ( &File       );
    free_FileList         ( &Files      );
    free_SchemeList       ( &Schemes    );
    free_PresetsList      ( &Presets    );
    free_ExceptionList    ( &Exceptions );
    free_CharReplaceList  ( &Replace    );
    free_CharReplaceForTag( &ReplaceTag );
    free_NFOfileTemplate  ( &NFOfile    );
    free_DetailedList     ( &Details    );
    free_ExternalAlbum    ( &External   );

    if ( Errors.Errors || Errors.Warnings || Errors.FileNotFound || Errors.IncorrectExt || Errors.Protected ) {
        fprintf ( Options.output, "** ERRORS:\n" );

        if ( Errors.Errors > 0 ) {
            fprintf ( Options.output, "General errors: %u\n", Errors.Errors );
        }
        if ( Errors.Warnings > 0 ) {
            fprintf ( Options.output, "Warnings: %u\n", Errors.Warnings );
        }
        if ( Errors.FileNotFound > 0 ) {
            fprintf ( Options.output, "Files Not Found: %u \n", Errors.FileNotFound );
        }
        if ( Errors.IncorrectExt > 0 ) {
            fprintf ( Options.output, "Incorrect extensions: %u \n", Errors.IncorrectExt );
        }
        if ( Errors.Protected > 0 ) {
            fprintf ( Options.output, "Write protected files: %u \n", Errors.Protected );
        }
    }

    if ( Options.output != stdout && Options.output != stderr ) {
        char asciiname[_MAX_PATH];
        fclose (Options.output );
        if ( Options._Values.OutputFileScheme != NULL ) {
            if ( rename ( Options._Values.OutputFileName, outputfilename ) != 0 ) {
                if ( append ( Options._Values.OutputFileName, outputfilename ) != 0 ) {
                    fprintf ( stderr, "Failed to append '%s' to '%s'.\n", Options._Values.OutputFileName, outputfilename );
                    exit (1);
                } else {
                    remove ( Options._Values.OutputFileName );
                }
            }
            string_l2a ( outputfilename, asciiname );
        } else {
            string_l2a ( Options._Values.OutputFileName, asciiname );
        }
        printf ( "Output saved to '%s'\n", asciiname );
    }

    exit (0);
}
