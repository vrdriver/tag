#include <string.h>
#include <string.h>
#include <stdlib.h>
#include <windows.h>
#include "Tag.h"
#include "tags.h"
#include "misc.h"
#include "guess.h"

// Checks and corrects scheme format
void _make_proper_scheme ( char* buffer, const char* scheme )
{
    //const char  identifiers [] = "ATLCGNYX";
    const char  identifiers [] = "ATLCGNYXQD";   // added 'Q' for quality, 'D' for disc

    const char*     s = scheme;
    char*           d = buffer;
    size_t          i;
    int             incorrect = 1;

    do {
        if ( *s == '%' || *s == '*' ) {
            incorrect = 0;
            break;
        }
    } while ( *s++ );

    s = scheme;

    if ( incorrect ) {  // No special signs are used -> correct all identifiers
        do {
            char c = *s;
            if ( c == '\\' || c == '/' ) {
                c = '\\';
                while ( *(s+1) == '\\' || *(s+1) == '/' ) s++;
            } else
            if ( c >= 'a' && c <= 'z' ) {
                c -= 'a' - 'A';
            } else
            if ( c == '*' ) {
                c = '%';
            }
            for ( i = 0; i < sizeof (identifiers); i++ ) {
                if ( c == identifiers[i] ) {
                    if ( s == scheme || *(s-1) != '%' ) {
                        *d++ = '%';
                    }
                    break;
                }
            }
            *d++ = c;
        } while ( *s++ );
    } else {            // Scheme is already correct, only modify path separators
        do {
            char c = *s;
            if ( c == '\\' || c == '/' ) {
                c = '\\';
                while ( *(s+1) == '\\' || *(s+1) == '/' ) s++;
            } else
            if ( c == '*' ) {
                c = '%';
            }
            *d++ = c;
        } while ( *s++ );
    }
}

// checks if str matches separator, returns 0 on match
int _check_separator ( const char* str, const char* sep, const int s_len, const char* scheme )
{
    if ( *sep == ' ' && s_len == 1 ) {
        int bNumeric = 0; // 1 = field ends for number, 0 = field ends for separator
        const char* s = scheme;

        while ( *s && *s != '%' ) s++;
        if ( *s == '%' ) {
            s++;
            if ( *s == 'N' || *s == 'n' || *s == 'Y' || *s == 'y' ) bNumeric = 1;
        }

        if ( *scheme == 'N' || *scheme == 'n' || *scheme == 'Y' || *scheme == 'y' ) {
            return strncmp (str, sep, s_len);
        }

        if ( bNumeric ) {
            if ( *str >= '0' && *str <= '9' ) return 0;
        }

        return 1;
    } else {
        return strncmp (str, sep, s_len);
    }
}

// convert string from ANSI to UTF-8
void _fromAnsiToUtf8 ( char* string )
{
    WCHAR   wszValue[_MAX_PATH];    // Unicode value
    size_t  len;

    // Convert ANSI value to Unicode
    if ( (len = MultiByteToWideChar (CP_ACP, 0, string, -1, wszValue, sizeof (wszValue))) == 0 ) {
        fprintf ( Options.output, "_fromAnsiToUtf8: MultiByteToWideChar failed.\n" );
        exit (1);
    }
    // Convert Unicode value to UTF-8
    if ( (len = unicodeToUtf8 (wszValue, string, len)) == 0 ) {
        fprintf ( Options.output, "_fromAnsiToUtf8: unicodeToUtf8 failed.\n" );
        exit (1);
    }
}

// Gets tag information from filename
int GenerateTagFromName ( const char* filename, const char* naming_scheme, FileInfo* Info )
{
    const char  identifiers [] = "ATLCGNYXD";

    char scheme  [_MAX_PATH * 2 + 1];

    char name    [_MAX_PATH];
    char dir     [_MAX_DIR];
    char fname   [_MAX_FNAME];

    char Title   [_MAX_PATH*3];
    char Artist  [_MAX_PATH*3];
    char Album   [_MAX_PATH*3];
    char Track   [_MAX_PATH*3];
    char Genre   [_MAX_PATH*3];
    char Year    [_MAX_PATH*3];
    char Comment [_MAX_PATH*3];
    char Disc    [_MAX_PATH*3];
    char discard [_MAX_PATH];

    char*           p;
    char*           dest = NULL;    // destination
    char*           sep  = NULL;    // field separator
    unsigned int    s_pos = 0;      // position in scheme
    unsigned int    f_pos = 0;      // position in filename
    unsigned int    d_len = 0;      // length of destination
    unsigned int    s_len = 0;      // length of source
    unsigned long   len;            // length of name
    int             paths = 0;
    unsigned int    i;
    int             disc_hack = 0;

    if ( naming_scheme == NULL || naming_scheme[0] == '\0' )
        return 1;

    strcpy ( name, filename );
    if ( !Options.NoMagic ) {
        for ( i = 0; i < strlen (name) - 5; i++ ) {
            if ( name[i] != '\\' && name[i] != '/' ) continue;

            if ( _strnicmp ( (char *)(name+i+1), "(CD", 3 ) == 0 || _strnicmp ( (char *)(name+i+1), "(DVD", 4 ) == 0 )
                name[i] = ' ';
        }
    }

    _splitpath ( name, NULL, dir, fname, NULL );

    if ( strlen (fname) == 0 || strlen (naming_scheme) >= _MAX_PATH )
        return 1;

    _make_proper_scheme ( scheme, naming_scheme );

    // quick hack
    {
        p = scheme;
        do {
            // %D%Nxxx -> %Nxxx
            if ( (*p == '%' && *(p+1) == 'D') && (*(p+2) == '%' && *(p+3) == 'N') ) {
                memmove ( p, p+2, strlen (p+2)+1 );
                disc_hack = 1;
            }
        } while ( *p++ );
    }

    p = scheme;
    do {
        if ( *p == '\\' ) paths++;
    } while ( *p++ );

    if ( paths > 0 ) {
        int path_pos = strlen ( dir ) - 1;

        while ( paths > 0 && path_pos >= 0 ) {
            if ( dir[--path_pos] == '\\' ) paths--;
        }
        path_pos++;

        strcpy ( name, (char *)(dir+path_pos) );
        strcat ( name, fname );
        len = strlen ( name );
    } else {
        strcpy ( name, fname );
        len = strlen ( name );
    }

    Title  [0] = '\0';
    Artist [0] = '\0';
    Album  [0] = '\0';
    Track  [0] = '\0';
    Genre  [0] = '\0';
    Year   [0] = '\0';
    Comment[0] = '\0';
    Disc   [0] = '\0';

    for ( s_pos = 0; s_pos < strlen (scheme) + 1; s_pos++ ) {
        if ( scheme[s_pos] == '%' ) {
            s_pos++;
            sep = (char *)(scheme+s_pos+1);
            s_len = 0;
            while ( (*(sep+s_len) != '\0') && (*(sep+s_len) != '%') ) s_len++;
            if ( s_len == 0 ) s_len = 1;

            switch ( scheme[s_pos] ) {
            case 't':   // title
            case 'T':
                dest  = Title;
                d_len = sizeof (Title);
                break;
            case 'a':   // artist
            case 'A':
                dest  = Artist;
                d_len = sizeof (Artist);
                break;
            case 'l':   // album
            case 'L':
                dest  = Album;
                d_len = sizeof (Album);
                break;
            case 'c':   // comment
            case 'C':
                dest  = Comment;
                d_len = sizeof (Comment);
                break;
            case 'n':   // track
            case 'N':
                dest  = Track;
                d_len = sizeof (Track);
                break;
            case 'y':   // year
            case 'Y':
                dest  = Year;
                d_len = sizeof (Year);
                break;
            case 'g':   // genre
            case 'G':
                dest  = Genre;
                d_len = sizeof (Genre);
                break;
            case 'd':   // disc
            case 'D':
                dest  = Disc;
                d_len = sizeof (Disc);
                break;
            case 'x':   // disrecard
            case 'X':
                dest  = discard;
                d_len = sizeof (discard);
                break;
            default:
                dest  = NULL;
                d_len = 0;
                break;
            }
        }

        if ( dest != NULL && sep != NULL ) {
            char *bak = dest;
            char *end = (char *)(dest + d_len);

            while ( f_pos < len && _check_separator ((char*)(name+f_pos), sep, s_len, scheme + s_pos) != 0 ) {
            // while ( f_pos < len && strncmp ((char*)(name+f_pos), sep, s_len) != 0 ) {
                if ( dest < end ) {
                    char c = name[f_pos++];
                    if ( c == '_' ) c = ' ';
                    if ( c == '\'' && f_pos < len && name[f_pos] == '\'' ) {    // '' -> "
                        f_pos++;
                        c = '"';
                    }
                    if ( c == ' ' && f_pos < len && name[f_pos] == ' ' ) {      // '  ' -> ' '
                        f_pos++;
                    }
                    *dest++ = c;
                } else {
                    f_pos++;
                }
            }

            if ( dest < end ) *dest++ = '\0';
            while ( dest != bak && *dest == ' ' ) *dest-- = '\0';
            if ( name[f_pos] == sep[0] ) f_pos += s_len;
            while ( name[f_pos] == ' ' ) f_pos++;

            dest = NULL;
        } else {
            if ( name[f_pos] == scheme[s_pos] ) f_pos++;
        }
    }

    if ( Options.CharReplaceForTag ) {
        ReplaceCharactersForTag ( Title,   &ReplaceTag );
        ReplaceCharactersForTag ( Artist,  &ReplaceTag );
        ReplaceCharactersForTag ( Album,   &ReplaceTag );
        ReplaceCharactersForTag ( Track,   &ReplaceTag );
        ReplaceCharactersForTag ( Genre,   &ReplaceTag );
        ReplaceCharactersForTag ( Year,    &ReplaceTag );
        ReplaceCharactersForTag ( Comment, &ReplaceTag );
        ReplaceCharactersForTag ( Disc,    &ReplaceTag );
    }

    // convert fields to UTF-8
    {
        _fromAnsiToUtf8 ( Title   );
        _fromAnsiToUtf8 ( Artist  );
        _fromAnsiToUtf8 ( Album   );
        _fromAnsiToUtf8 ( Track   );
        _fromAnsiToUtf8 ( Genre   );
        _fromAnsiToUtf8 ( Year    );
        _fromAnsiToUtf8 ( Comment );
        _fromAnsiToUtf8 ( Disc    );
    }

    if ( !Options.NoMagic ) {
        if ( strlen ( Album ) > 6 && Year[0] == '\0' ) {
            int l = strlen ( Album );
            if ( (Album[l-6] == '(' || Album[l-6] == '[') && (Album[l-1] == ')' || Album[l-1] == ']') &&
                ((Album[l-5] == '1' && Album[l-4] == '9') || (Album[l-5] == '2' && Album[l-4] == '0')) &&
                (Album[l-3] >= '0' && Album[l-3] <= '9' && Album[l-2] >= '0' && Album[l-2] <= '9') ) {

                memcpy ( Year, (char *)(Album+l-5), 4 );
                Year[4] = '\0';
                if ( Album[l-7] == ' ' )
                    Album[l-7] = '\0';
                else
                    Album[l-6] = '\0';
            }
        }

        if ( !disc_hack ) {
            if ( strlen (Track) == 3 && (Track[0] >= '1' && Track[0] <= '9') ) {
                char temp[32];

                sprintf ( temp, " (CD %c)", Track[0] );
                Track[0] = Track[1];
                Track[1] = Track[2];
                Track[2] = '\0';

                if ( Album[0] == '\0' && TagValue (APE_TAG_FIELD_ALBUM, Info) ) {
                    strncpy ( Album, TagValue (APE_TAG_FIELD_ALBUM, Info), sizeof (Album)-1 );
                    Album[sizeof (Album)-1] = '\0';
                }
                strcat ( Album, temp );
            }
        }

        fix_percentage_sequences ( Title   );
        fix_percentage_sequences ( Artist  );
        fix_percentage_sequences ( Album   );
        fix_percentage_sequences ( Track   );
        fix_percentage_sequences ( Genre   );
        fix_percentage_sequences ( Year    );
        fix_percentage_sequences ( Comment );
    }

    if ( disc_hack ) {
        if ( strlen (Track) == 3 && (Track[0] >= '1' && Track[0] <= '9') ) {
            sprintf ( Disc, "%c", Track[0] );

            Track[0] = Track[1];
            Track[1] = Track[2];
            Track[2] = '\0';
        }
    }

    if ( Title[0] != '\0' )
        InsertTagFieldU ( "Title",   0, Title,   0, 0, Info, 0, 0, 0 );
    if ( Artist[0] != '\0' )
        InsertTagFieldU ( "Artist",  0, Artist,  0, 0, Info, 0, 0, 0 );
    if ( Album[0] != '\0' )
        InsertTagFieldU ( "Album",   0, Album,   0, 0, Info, 0, 0, 0 );
    if ( Track[0] != '\0' )
        InsertTagFieldU ( "Track",   0, Track,   0, 0, Info, 0, 0, 0 );
    if ( Genre[0] != '\0' )
        InsertTagFieldU ( "Genre",   0, Genre,   0, 0, Info, 0, 0, 0 );
    if ( Year[0] != '\0' )
        InsertTagFieldU ( "Year",    0, Year,    0, 0, Info, 0, 0, 0 );
    if ( Comment[0] != '\0' )
        InsertTagFieldU ( "Comment", 0, Comment, 0, 0, Info, 0, 0, 0 );
    if ( Disc[0] != '\0' )
        InsertTagFieldU ( "Disc",    0, Disc,    0, 0, Info, 0, 0, 0 );

    if ( (Info->Tags = (TagInfo *)realloc ( Info->Tags, sizeof (TagInfo) * (Info->TagCount + 1) )) == NULL ) {
        fprintf ( Options.output, "GenerateTagFromName: Memory allocation failed.\n" );
        exit (1);
    }
    Info->Tags[Info->TagCount].TagOffset = Info->FileSize;
    Info->Tags[Info->TagCount].TagSize   = 0;
    Info->Tags[Info->TagCount].TagType   = guessed_tag;
    Info->TagCount++;

    return 0;
}

// Gets pattern of filename scheme
int GenerateTagFromName_Test ( const char* filename, const char* naming_scheme, char* pattern )
{
    char scheme  [_MAX_PATH];

    char name    [_MAX_PATH];
    char dir     [_MAX_DIR];
    char fname   [_MAX_FNAME];

    char *dest = NULL;      // destination
    char *sep  = NULL;      // field separator
    unsigned int s_pos = 0; // position in scheme
    unsigned int f_pos = 0; // position in filename
    unsigned int d_len = 0; // length of destination
    unsigned int s_len = 0; // length of source
    unsigned int pat_p = 0; // position in pattern
    unsigned long len;      // length of name
    int paths = 0;
    unsigned int i;

    strcpy ( name, filename );
    if ( !Options.NoMagic ) {
        for ( i = 0; i < strlen (name) - 5; i++ ) {
            if ( name[i] != '\\' && name[i] != '/' ) continue;

            if ( _strnicmp ( (char *)(name+i+1), "(CD", 3 ) == 0 || _strnicmp ( (char *)(name+i+1), "(DVD", 4 ) == 0 )
                name[i] = ' ';
        }
    }

    _splitpath( name, NULL, dir, fname, NULL );

    if ( strlen (fname) == 0 || strlen (naming_scheme) >= _MAX_PATH )
        return 1;

    for ( i = 0; i <= strlen (naming_scheme); i++ ) {
        char c = naming_scheme[i];
        if ( c == '\\' || c == '/' ) {
            c = '\\';
            while ( scheme[i+1] == '\\' || scheme[i+1] == '/' ) i++;
            paths++;
        }
        scheme[i] = c;
    }

    if ( paths > 0 ) {
        int path_pos = strlen ( dir ) - 1;

        while ( paths > 0 && path_pos >= 0 ) {
            if ( dir[--path_pos] == '\\' ) paths--;
        }
        path_pos++;

        strcpy ( name, (char *)(dir+path_pos) );
        strcat ( name, fname );
        len = strlen ( name );
    } else {
        strcpy ( name, fname );
        len = strlen ( name );
    }

    *pattern = '\0';
    while ( scheme[s_pos] != '%' ) {
        pattern[pat_p++] = name[f_pos++];
        pattern[pat_p  ] = '\0';
        s_pos++;
    }

    for ( ; s_pos < strlen (scheme) + 1; s_pos++ ) {
        if ( scheme[s_pos] == '%' ) {
            s_pos++;
            sep = (char *)(scheme+s_pos+1);
            s_len = 0;
            while ( (*(sep+s_len) != '\0' ) && (*(sep+s_len) != '%') ) s_len++;
            if ( s_len == 0 ) s_len = 1;

            switch ( scheme[s_pos] ) {
            case 't':   // title
            case 'T':
            case 'a':   // artist
            case 'A':
            case 'l':   // album
            case 'L':
            case 'c':   // comment
            case 'C':
            case 'n':   // track
            case 'N':
            case 'y':   // year
            case 'Y':
            case 'g':   // genre
            case 'G':
            case 'd':   // disc
            case 'D':
            case 'x':   // disrecard
            case 'X':
                dest  = (char *)(pattern + pat_p);
                d_len = 1;
                break;
            default:
                dest  = NULL;
                d_len = 0;
                break;
            }
        }

        if ( dest != NULL && sep != NULL ) {
            char *bak = dest;
            char *end = (char *)(dest + d_len);

            while ( f_pos < len && _check_separator ((char*)(name+f_pos), sep, s_len, scheme + s_pos) != 0 ) {
            //while ( f_pos < len && strncmp ((char*)(name+f_pos), sep, s_len) != 0 ) {
                if ( dest < end ) {
                    char c = name[f_pos++];
                    if ( c == '_' ) c = ' ';
                    if ( c == '\'' && f_pos < len && name[f_pos] == '\'' ) {    // '' -> "
                        f_pos++;
                        c = '"';
                    }
                    if ( c == ' ' && f_pos < len && name[f_pos] == ' ' ) {      // '  ' -> ' '
                        f_pos++;
                    }
                    *dest++ = c;
                } else {
                    f_pos++;
                }
            }

            *dest++ = '\0';
            if ( name[f_pos] == sep[0] ) f_pos += s_len;
            while ( name[f_pos] == ' ' ) f_pos++;
            strncat ( pattern, sep, s_len );
            pat_p = strlen ( pattern );
            dest = NULL;
        }
    }

    return 0;
}

// Finds best matching naming scheme
int FindBestGuess ( const char* filename, SchemeList* Schemes )
{
    const char  identifiers [] = "ATLCGNYXD";

    char    scheme[1024];
    char    test[_MAX_PATH];
    char    dir[_MAX_PATH];
    char    fname[_MAX_PATH];
    int     closest_match = -1;
    int     no_dirs;
    size_t  i, j, k;

    if ( Schemes->SchemeCount == 0 )
        return closest_match;

    _splitpath ( filename, NULL, dir, NULL, NULL );
    if ( dir[0] == '\0' || strcmp ( dir, "\\" ) == 0 ) {
        no_dirs = 1;
        _get_path ( filename, fname );
    } else {
        no_dirs = 0;
        strcpy ( fname, filename );
    }

    for ( i = 0; i < Schemes->SchemeCount; i++ ) {
        if ( no_dirs ) {
            _get_path ( Schemes->Schemes[i], scheme );
            GenerateTagFromName_Test ( fname, scheme, test );
        } else {
            GenerateTagFromName_Test ( fname, Schemes->Schemes[i], test );
        }

        k = 0;
        for ( j = 0; j <= strlen (Schemes->Schemes[i]); j++ ) {
            if ( Schemes->Schemes[i][j] == '%' ) continue;
            scheme[k++] = Schemes->Schemes[i][j];
        }
        if ( no_dirs ) {
            char temp[1024];
            _get_path ( scheme, temp );
            strcpy ( scheme, temp );
        }

        if ( strlen ( test ) == strlen ( scheme ) ) {
            int match = 1;
            for ( j = 0; j < strlen (test); j++ ) {
                int is_identifier = 0;
                int tc = test[j];
                int sc = scheme[j];

                if ( sc >= 'a' && sc <= 'z' ) sc -= 'a' - 'A';
                for ( k = 0; k < sizeof (identifiers); k++ ) {
                    if ( sc == identifiers[k] ) {
                        is_identifier = 1;
                        break;
                    }
                }

                if ( !is_identifier && sc == tc )   // field separator
                    continue;

                switch (sc) {
                case '/':   // path separator
                case '\\':
                    sc = '/';
                    break;
                case 'N':   // numerical
                case 'Y':
                case 'D':
                    sc = 'N';
                    break;
                default:    // text
                    sc = 'A';
                    break;
                }

                if ( tc == '/' || tc == '\\' ) {
                    tc = '/';
                } else
                if ( tc >= '0' && tc <= '9' ) {
                    tc = 'N';
                } else {
                    tc = 'A';
                }

                if ( closest_match == -1 && (sc == 'A' && tc == 'N') )
                    closest_match = i;

                if ( sc != tc ) {
                    if ( sc == 'N' && tc == 'A' && closest_match == (int)i ) {
                        closest_match = -1;
                    }
                    match = 0;
                }
            }

            if ( match ) return i;
        }
    }

    return closest_match;
}

// Guesses tag information from filename
int GuessTag ( FileInfo* Info, SchemeList* Schemes )
{
    int best;

    if ( (best = FindBestGuess ( Info->Filename, Schemes )) < 0 )
        return 1;

    return GenerateTagFromName ( Info->Filename, Schemes->Schemes[best], Info );
}

// Generates name from tag
int GenerateNameFromTag ( char* filename, const char* naming_scheme, const FileInfo* Info )
{
    const char  identifiers [] = "ATLCGNYXD";

    const char* targets [] = {
        "T", APE_TAG_FIELD_TITLE,
        "A", APE_TAG_FIELD_ARTIST,
        "L", APE_TAG_FIELD_ALBUM,
        "C", APE_TAG_FIELD_COMMENT,
        "G", APE_TAG_FIELD_GENRE,
        "N", APE_TAG_FIELD_TRACK,
        "Y", APE_TAG_FIELD_YEAR,
        "D", "Disc",
        "Q", 0                          // quality (not in tags)
    };

    char    scheme  [_MAX_PATH * 2 + 1];
    char    temp    [_MAX_PATH * 2 + 1];
    char    tempname[4096];
    char*   src   = NULL;
    char*   f;
    size_t  s_pos = 0;
    size_t  i;

    filename[0] = '\0';

    if ( strlen (naming_scheme) >= _MAX_PATH )
        return 1;

    _get_path ( naming_scheme, temp );
    _make_proper_scheme ( scheme, temp );

    f = tempname;

    for ( s_pos = 0; s_pos < strlen (scheme) + 1; s_pos++ ) {
        src = NULL;

        if ( scheme[s_pos] == '%' ) {
            s_pos++;
            for ( i = 0; i < (sizeof (targets) / sizeof (*targets)) / 2; i++ ) {
                if ( strnicmp ( targets[i*2], scheme + s_pos, 1 ) == 0 ) {
                    if ( i < 8 ) {  // Tag
                        src = TagValue ( targets[i*2+1], Info );
                    } else {        // Quality
                        src = (char *)Info->Details.Quality;
                    }
                    if ( src == NULL || src[0] == '\0' ) {
                        return 1;
                    }
                    break;
                }
            }
        }

        if ( src != NULL ) {
            if ( src == TagValue ( APE_TAG_FIELD_TRACK, Info ) && (strlen (src) == 1) ) {
                *f++ = '0';
            }

            f += sprintf ( f, "%s", src );
        } else {
            *f++ = scheme[s_pos];
        }
    }

    *f++ = '\0';

    ReplaceCharacters ( tempname, &Replace );
    remove_unsupported_chars ( tempname );
    tempname[_MAX_PATH-1] = '\0';
    strcpy ( filename, tempname );

    return 0;
}

// Generates path from tag
int GeneratePathFromTag ( char* path, const char* naming_scheme, const FileInfo* Info )
{
    const char  identifiers [] = "ATLCGNYXD";

    const char* targets [] = {
        "T", APE_TAG_FIELD_TITLE,
        "A", APE_TAG_FIELD_ARTIST,
        "L", APE_TAG_FIELD_ALBUM,
        "C", APE_TAG_FIELD_COMMENT,
        "G", APE_TAG_FIELD_GENRE,
        "N", APE_TAG_FIELD_TRACK,
        "Y", APE_TAG_FIELD_YEAR,
        "D", "Disc"
    };

    char    scheme  [_MAX_PATH * 2 + 1];
    char    temp    [_MAX_PATH * 2 + 1];
    char    tempname[4096];
    char*   src   = NULL;
    char*   p;
    char*   f;
    size_t  s_pos = 0;
    size_t  i;

    path[0] = '\0';

    if ( strlen (naming_scheme) >= _MAX_PATH )
        return 1;

    strcpy ( temp, naming_scheme );
    if ( temp[0] != '\0' ) {
        p = (char *)(temp + strlen (temp)-1);
        while ( p >= temp && *p != '/' && *p != '\\' ) {
            *p-- = '\0';
        }
        if ( *p == '/' || *p == '\\' ) *p-- = '\0';
    }

    _make_proper_scheme ( scheme, temp );
    f = tempname;

    for ( s_pos = 0; s_pos < strlen (scheme) + 1; s_pos++ ) {
        src = NULL;

        if ( scheme[s_pos] == '%' ) {
            s_pos++;
            for ( i = 0; i < (sizeof (targets) / sizeof (*targets)) / 2; i++ ) {
                if ( strnicmp ( targets[i*2], scheme + s_pos, 1 ) == 0 ) {
                    src = TagValue ( targets[i*2+1], Info );
                    if ( src == NULL || src[0] == '\0' ) {
                        return 1;
                    }
                    break;
                }
            }
        }

        if ( src != NULL ) {
            char    temppath[4096];
            p = temppath;
            if ( src == TagValue ( APE_TAG_FIELD_TRACK, Info ) && (strlen (src) == 1) ) {
                *p++ = '0';
            }
            p += sprintf ( p, "%s", src );

            ReplaceCharacters ( temppath, &Replace );
            remove_unsupported_chars ( temppath );
            f += sprintf ( f, "%s", temppath );
            // remove dots from the end of generated path, those are not supported by file system
            while ( (f-1 >= tempname) && (*(f-1) == '.') ) {
                *(--f) = '\0';
            }
        } else {
            *f++ = scheme[s_pos];
        }
    }

    *f = '\0';

    tempname[_MAX_PATH-1] = '\0';
    strcpy ( path, tempname );

    return 0;
}
