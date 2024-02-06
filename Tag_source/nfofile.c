#include "nfofile.h"
#include "tags.h"

const char months3[12][4] = { "Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec" };
const char months[12][10] = { "January","February","March","April","May","June","July","August","September","October","November","December" };

typedef struct {
    const char*             filename;
    const char*             artist;
    const char*             album;
    const char*             scheme;
    unsigned long           totaltime;
    size_t                  c_date;
    size_t                  c_month;
    size_t                  c_year;

    const ExceptionList*    Exceptions;
    const NFOfileTemplate*  NFOfile;

    size_t                  max_linelen;
} NFOfields;

NFOfields NFO;

// %FILENAME%       : name of nfo-file
// %ARTIST%         : album artist
// %ALBUM%          : album title
// %GENRE%          : genre
// %YEAR%           : release year
// %TRACKLIST%      : tracklist by scheme or by v.a.scheme when various artists
// %TOTALTIME%      : total album duration
// %RIGHT%          : right justify
// %D               : current date
// %0D              : current date, prefixed with zero if less than 10
// %M               : current month (numeric)
// %0M              : current month (numeric), prefixed with zero if less than 10
// %MM              : current month (short name)
// %MMM             : current month (entire name)
// %Y               : current year

void _conv_chars ( char* buffer )
{
    switch ( NFO.NFOfile->CaseConversion ) {
    case Capitalize:
        capitalize_case ( buffer );
        exceptions_in_case ( buffer, NFO.Exceptions );
        break;
    case Sentence:
        sentence_case ( buffer );
        exceptions_in_case ( buffer, NFO.Exceptions );
        break;
    case Lower:
        lower_case ( buffer );
        break;
    case Upper:
        upper_case ( buffer );
        break;
    default:
        break;
    }

    if ( NFO.NFOfile->ReplaceSpaces ) {
        replace_spaces ( buffer, NFO.NFOfile->ReplaceSpaces );
    }
}

// Checks and corrects scheme format
void _nfo_make_proper_scheme ( char* buffer, const char* scheme )
{
    const char  identifiers [] = "NATLD";

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
            if ( c == '*' ) {
                c = '%';
            }
            *d++ = c;
        } while ( *s++ );
    }
}

int _generateNameFromTag ( char* filename, const char* naming_scheme, const DetailedList* Details, size_t track )
{
    const char  identifiers [] = "NATLD";

    const char* targets [] = {
        "T", APE_TAG_FIELD_TITLE,
        "A", APE_TAG_FIELD_ARTIST,
        "L", APE_TAG_FIELD_ALBUM,
        "C", APE_TAG_FIELD_COMMENT,
        "G", APE_TAG_FIELD_GENRE,
        "N", APE_TAG_FIELD_TRACK,
        "Y", APE_TAG_FIELD_YEAR
    };

    char    scheme  [_MAX_PATH * 2 + 1];
    char    temp    [_MAX_PATH * 2 + 1];
    char    tmpval  [1024];
    char*   src   = NULL;
    char*   sep   = NULL;
    // char*   p;
    char*   f;
    char*   tempname;
    int     t_pos = 0;
    size_t  s_pos = 0;
    size_t  s_len = 0;
    size_t  temp_len;
    // size_t  i, j;

    filename[0] = '\0';
    tmpval[0] = '\0';

    if ( strlen (naming_scheme) >= _MAX_PATH )
        return 1;

    _get_path ( naming_scheme, temp );

    /*
    p = scheme;
    for ( i = 0; i <= strlen (temp); i++ ) {
        char c = temp[i];
        if ( c == '\\' || c == '/' ) {
            c = '\\';
            while ( i < strlen (temp) && (temp[i + 1] == '/' || temp[i + 1] == '\\') ) {
                i++;
            }
        } else
        if ( c >= 'a' && c <= 'z' ) {
            c -= 'a' - 'A';
        }
        for ( j = 0; j < sizeof (identifiers); j++ ) {
            if ( c != '\0' && c == identifiers[j] ) {
                if ( i == 0 || temp[i - 1] != '%' ) {
                    *p++ = '%';
                }
                break;
            }
        }
        *p++ = c;
    }
    */
    _nfo_make_proper_scheme ( scheme, naming_scheme );

    temp_len = 0;
    for ( s_pos = 0; s_pos < strlen (scheme) + 1; s_pos++ ) {
        if ( scheme[s_pos] == '%' ) {
            s_pos++;
            sep = (char *)(scheme+s_pos+1);
            s_len = 0;
            while ( (sep < (char *)(scheme+strlen(scheme)+1)) && ((*(sep+s_len) != '%' && *(sep+s_len) != '\0')) ) s_len++;
            if ( s_len == 0 ) s_len = 1;

            switch ( scheme[s_pos] ) {
            case 'a':
            case 'A':
                src = Details->Files[track].Artist;
                if ( src == NULL || src[0] == '\0' ) return 1;
                temp_len += strlen (src) + s_len;
                if ( strlen (src) < 2 ) temp_len++;
                break;

            case 't':
            case 'T':
                src = Details->Files[track].Title;
                if ( src == NULL || src[0] == '\0' ) return 1;
                temp_len += strlen (src) + s_len;
                if ( strlen (src) < 2 ) temp_len++;
                break;

            case 'l':
            case 'L':
                src = Details->Files[track].Album;
                if ( src == NULL || src[0] == '\0' ) return 1;
                temp_len += strlen (src) + s_len;
                if ( strlen (src) < 2 ) temp_len++;
                break;

            case 'c':
            case 'C':
                src = Details->Files[track].Comment;
                if ( src == NULL || src[0] == '\0' ) return 1;
                temp_len += strlen (src) + s_len;
                if ( strlen (src) < 2 ) temp_len++;
                break;

            case 'd':
            case 'D':
                sprintf ( tmpval, "%u:%02u", Details->Files[track].Duration / 60, Details->Files[track].Duration % 60 );
                src = tmpval;
                if ( src == NULL || src[0] == '\0' ) return 1;
                temp_len += strlen (src) + s_len;
                if ( strlen (src) < 2 ) temp_len++;
                break;

            case 'n':
            case 'N':
                sprintf ( tmpval, "%02u", track + 1 );
                src = tmpval;
                if ( src == NULL || src[0] == '\0' ) return 1;
                temp_len += strlen (src) + s_len;
                if ( strlen (src) < 2 ) temp_len++;
                break;
            }
        } else {
            temp_len++;
        }
    }

    if ( (tempname = malloc ( temp_len + 1 )) == NULL ) {
        fprintf ( Options.output, "_generateNameFromTag: Memory allocation failed.\n" );
        exit (1);
    }
    f = tempname;

    for ( s_pos = 0; s_pos < strlen (scheme) + 1; s_pos++ ) {
        src = NULL;

        if ( scheme[s_pos] == '%' ) {
            s_pos++;
            sep = (char *)(scheme+s_pos+1);
            s_len = 0;
            while ( (sep < (char *)(scheme+strlen(scheme)+1)) && ((*(sep+s_len) != '%' && *(sep+s_len) != '\0')) ) s_len++;
            if ( s_len == 0 ) s_len = 1;

            switch ( scheme[s_pos] ) {
            case 'a':
            case 'A':
                src = Details->Files[track].Artist;
                if ( src == NULL || src[0] == '\0' ) {
                    free ( tempname );
                    return 1;
                }
                break;

            case 't':
            case 'T':
                src = Details->Files[track].Title;
                if ( src == NULL || src[0] == '\0' ) {
                    free ( tempname );
                    return 1;
                }
                break;

            case 'l':
            case 'L':
                src = Details->Files[track].Album;
                if ( src == NULL || src[0] == '\0' ) {
                    free ( tempname );
                    return 1;
                }
                break;

            case 'c':
            case 'C':
                src = Details->Files[track].Comment;
                if ( src == NULL || src[0] == '\0' ) {
                    free ( tempname );
                    return 1;
                }
                break;

            case 'd':
            case 'D':
                sprintf ( tmpval, "%u:%02u", Details->Files[track].Duration / 60, Details->Files[track].Duration % 60 );
                src = tmpval;
                if ( src == NULL || src[0] == '\0' ) {
                    free ( tempname );
                    return 1;
                }
                break;

            case 'n':
            case 'N':
                sprintf ( tmpval, "%02u", track + 1 );
                src = tmpval;
                if ( src == NULL || src[0] == '\0' ) {
                    free ( tempname );
                    return 1;
                }
                break;
            }
        }

        if ( src != NULL ) {
            if ( !NFO.NFOfile->ApplyToTracks ) {
                f += sprintf ( f, "%s", src );
            } else {
                size_t l = sprintf ( f, "%s", src );
                _conv_chars ( f );
                f += l;
            }

            src = NULL;
        } else {
            *f++ = scheme[s_pos];
        }
    }

    *f++ = '\0';

    strcpy ( filename, tempname );

    free ( tempname );

    return 0;
}

size_t _insert ( char* buffer, const char* item, size_t pos, size_t lenleft, const DetailedList* Details, const FileInfo* Info )
{
    const char*     none = "";
    char            track[_MAX_PATH];
    char            buf [65536];
    char            temp[1024];
    char*           d = temp;
    const char*     s = item;
    char*           p = buffer;
    const char*     n = NULL;

    while ( *s && *s != '|' ) {
        *d++ = *s++;
    }
    *d = '\0';
    if ( *s == '|'  ) s++;

    if ( 0 == stricmp ( temp, "FILENAME" ) ) {
        n = NFO.filename;
    } else
    if ( 0 == stricmp ( temp, "ARTIST" ) ) {
        n = NFO.artist;
    } else
    if ( 0 == stricmp ( temp, "ALBUM" ) ) {
        n = NFO.album;
    } else
    if ( 0 == stricmp ( temp, "TRACKLIST" ) ) {
        size_t  i;
        for ( i = 0; i < Details->FileCount; i++ ) {
            if ( i > 0 ) {
                size_t j;
                for ( j = 0; j < pos; j++ ) {
                    *p++ = ' ';
                }
            }
            if ( _generateNameFromTag ( track, NFO.scheme, Details, i ) != 0 ) {
                sprintf ( buf, "%s", Details->Files[i].Info->Info.name );
                if ( NFO.NFOfile->ApplyToTracks ) {
                    _conv_chars ( buf );
                }
                p += sprintf ( p, "%s", buf );
            } else {
                p += sprintf ( p, "%s", track );
            }
            if ( i+1 < Details->FileCount ) p += sprintf ( p, "\n" );
        }
        n = none;
    } else
    if ( 0 == stricmp ( temp, "TOTALTIME" ) ) {
        size_t th = (NFO.totaltime / (60 * 60));
        size_t tm = (NFO.totaltime / 60) % 60;
        size_t ts = (NFO.totaltime) % 60;
        if ( th > 0 ) {
            p += sprintf ( p, "%02u:%02u:%02u", th, tm, ts );
        } else {
            p += sprintf ( p, "%02u:%02u", tm, ts );
        }
        n = none;
    } else
    if ( 0 == stricmp ( temp, "D" ) ) {
        p += sprintf ( p, "%u", NFO.c_date );
        n = none;
    } else
    if ( 0 == stricmp ( temp, "0D" ) ) {
        p += sprintf ( p, "%02u", NFO.c_date );
        n = none;
    } else
    if ( 0 == stricmp ( temp, "M" ) ) {
        p += sprintf ( p, "%u", NFO.c_month+1 );
        n = none;
    } else
    if ( 0 == stricmp ( temp, "0M" ) ) {
        p += sprintf ( p, "%02u", NFO.c_month+1 );
        n = none;
    } else
    if ( 0 == stricmp ( temp, "MM" ) ) {
        sprintf ( buf, "%s", months3[NFO.c_month] );
        _conv_chars ( buf );
        p += sprintf ( p, "%s", buf );
        n = none;
    } else
    if ( 0 == stricmp ( temp, "MMM" ) ) {
        sprintf ( buf, "%s", months[NFO.c_month] );
        _conv_chars ( buf );
        p += sprintf ( p, "%s", buf );
        n = none;
    } else
    if ( 0 == stricmp ( temp, "Y" ) ) {
        p += sprintf ( p, "%u", NFO.c_year );
        n = none;
    } else
    if ( 0 == stricmp ( temp, "RIGHT" ) ) {
        int j;
        for ( j = pos; j < (int)(NFO.max_linelen - lenleft); j++ ) {
            *p++ = ' ';
        }
        n = none;
    } else {
        n = TagValue ( temp, Info );
    }

    if ( n != NULL ) {
        sprintf ( buf, "%s", n );
        _conv_chars ( buf );
        p += sprintf ( p, "%s", buf );
    } else {
        sprintf ( buf, "%s", s );
        _conv_chars ( buf );
        p += sprintf ( p, "%s", buf );
    }

    return p - buffer;
}

void _parse ( char* buffer, const char* string, const DetailedList* Details, const FileInfo* Info )
{
    char*           d = buffer;
    const char*     s = string;
    char*           p;
    char            item[1024];
    size_t          pos;

    do {
        if ( *s == '%' ) {
            pos = s - string;
            s++;
            p = item;
            while ( *s && *s != '%' ) {
                *p++ = *s++;
            }
            *p = '\0';
            if ( *s == '%' ) s++;
            d += _insert ( d, item, pos, strlen (s), Details, Info );
        }
        if ( *s != '%' ) {
            *d++ = *s;
        }
    } while ( *s++ );
    *d++ = '\0';
}

size_t _get_length ( const char* string )
{
    const char*     p       = string;
    size_t          len     = 0;
    size_t          max_len = 0;

    while ( *p ) {
        if ( *p == '\n' ) {
            if ( len > max_len ) max_len = len;
            len = 0;
        } else {
            len++;
        }
        p++;
    }
    if ( len > max_len ) max_len = len;

    return max_len;
}

int WriteNFOfile ( const char* nfoname, const NFOfileTemplate* NFOfile, const DetailedList* Details, const FileInfo *Info, const ExceptionList* Exceptions )
{
    FILE*       fp;
    time_t      clock;
    struct tm*  current;
    char*       temp;
    size_t      i;
    size_t      lines   = 0;
    size_t      max_len = 0;
    size_t      len     = 0;

    if ( Details->FileCount == 0 ) {
        fprintf ( Options.output, "No files.\n" );
        return 1;
    }
    if ( NFOfile->LineCount == 0 ) {
        fprintf ( Options.output, ".nfo template empty.\n" );
        return 1;
    }

    if ( (temp = malloc ( 1024*1024 )) == NULL ) {
        fprintf ( Options.output, "WriteNFOfile: Memory allocation failed.\n" );
        exit (1);
    }
    memset ( &NFO, 0, sizeof (NFO) );

    if ( !Options.TestMode ) {
        if ( (fp = fopen ( nfoname, "wt" )) == NULL ) {
            fprintf ( Options.output, "Failed to open .nfo file '%s' for writing.\n" );
            free ( temp );
            return 1;
        }
    } else {
        fp = stdout;
        fprintf ( Options.output, ".nfo file: '%s'\n", nfoname );
    }

    for ( i = 0; i < Details->FileCount; i++ ) {
        NFO.totaltime += Details->Files[i].Duration;
    }

    time ( &clock );
    current = localtime ( &clock );

    NFO.filename    = nfoname;
    NFO.album       = Details->Album;
    NFO.artist      = Details->Artist;

    NFO.c_date      = current->tm_mday;
    NFO.c_month     = current->tm_mon;
    NFO.c_year      = current->tm_year + 1900;

    if ( stricmp ( NFO.artist, "Various Artists" ) != 0 ) {
        NFO.scheme = NFOfile->Scheme;
    } else {
        NFO.scheme = NFOfile->SchemeVA;
    }

    NFO.Exceptions  = Exceptions;
    NFO.NFOfile     = NFOfile;

    // get maximum line length
    for ( i = 0; i < NFOfile->LineCount; i++ ) {
        size_t  len;
        _parse ( temp, NFOfile->Lines[i], Details, Info );
        len = _get_length ( temp );
        if ( len > max_len ) max_len = len;
    }
    NFO.max_linelen = max_len;

    for ( i = 0; i < NFOfile->LineCount; i++ ) {
        _parse ( temp, NFOfile->Lines[i], Details, Info );
        fprintf ( fp, "%s\n", temp );
    }

    free ( temp );

    if ( !Options.TestMode ) {
        fclose (fp);
        fprintf ( Options.output, ".nfo file '%s' written.\n\n", nfoname );
    }

    return 0;
}
