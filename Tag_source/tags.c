#include <stdio.h>
#include <string.h>
#include <io.h>
#include <stdlib.h>
#include <windows.h>
#include "Tag.h"
#include "tags.h"
#include "misc.h"
#include "guess.h"

const char*  ID3v1GenreList[] = {
    "Blues", "Classic Rock", "Country", "Dance", "Disco", "Funk",
    "Grunge", "Hip-Hop", "Jazz", "Metal", "New Age", "Oldies",
    "Other", "Pop", "R&B", "Rap", "Reggae", "Rock",
    "Techno", "Industrial", "Alternative", "Ska", "Death Metal", "Pranks",
    "Soundtrack", "Euro-Techno", "Ambient", "Trip-Hop", "Vocal", "Jazz+Funk",
    "Fusion", "Trance", "Classical", "Instrumental", "Acid", "House",
    "Game", "Sound Clip", "Gospel", "Noise", "AlternRock", "Bass",
    "Soul", "Punk", "Space", "Meditative", "Instrumental Pop", "Instrumental Rock",
    "Ethnic", "Gothic", "Darkwave", "Techno-Industrial", "Electronic", "Pop-Folk",
    "Eurodance", "Dream", "Southern Rock", "Comedy", "Cult", "Gangsta",
    "Top 40", "Christian Rap", "Pop/Funk", "Jungle", "Native American", "Cabaret",
    "New Wave", "Psychadelic", "Rave", "Showtunes", "Trailer", "Lo-Fi",
    "Tribal", "Acid Punk", "Acid Jazz", "Polka", "Retro", "Musical",
    "Rock & Roll", "Hard Rock", "Folk", "Folk/Rock", "National Folk", "Swing",
    "Fast-Fusion", "Bebob", "Latin", "Revival", "Celtic", "Bluegrass", "Avantgarde",
    "Gothic Rock", "Progressive Rock", "Psychedelic Rock", "Symphonic Rock", "Slow Rock", "Big Band",
    "Chorus", "Easy Listening", "Acoustic", "Humour", "Speech", "Chanson",
    "Opera", "Chamber Music", "Sonata", "Symphony", "Booty Bass", "Primus",
    "Porn Groove", "Satire", "Slow Jam", "Club", "Tango", "Samba",
    "Folklore", "Ballad", "Power Ballad", "Rhythmic Soul", "Freestyle", "Duet",
    "Punk Rock", "Drum Solo", "A capella", "Euro-House", "Dance Hall",
    "Goa", "Drum & Bass", "Club House", "Hardcore", "Terror",
    "Indie", "BritPop", "NegerPunk", "Polsk Punk", "Beat",
    "Christian Gangsta", "Heavy Metal", "Black Metal", "Crossover", "Contemporary C",
    "Christian Rock", "Merengue", "Salsa", "Thrash Metal", "Anime", "JPop",
    "SynthPop",
};

const char* TagNames[] = {
    "No",
    "ID3v1",
    "ID3v2",
    "Lyrics3 v2",
    "APE v1.0",
    "APE v2.0",
    "Vorbis",
    "FLAC"
};

// Free tagdata from memory
void FreeTagFields ( FileInfo* Info )
{
    size_t  i;

    for ( i = 0; i < Info->TagItemCount; i++ ) {
        free ( Info->TagItems[i].Item   );
        free ( Info->TagItems[i].Value  );
        free ( Info->TagItems[i].ValueU );
    }

    free ( Info->TagItems );
    free ( Info->Tags     );

    Info->TagItems      = NULL;
    Info->TagItemCount  = 0;
    Info->Tags          = NULL;
    Info->TagCount      = 0;
}

// Return position of first specified item
size_t TagItemNum ( const char* item, const FileInfo* Info )
{
    size_t  i;

    for ( i = 0; i < Info->TagItemCount; i++ ) {
        if ( _stricmp ( item, Info->TagItems[i].Item ) == 0 )
            return i;
    }

    return (size_t)-1;
}

// Return number of identical item names
size_t TagItemCount ( const char* item, const FileInfo* Info )
{
    size_t  i;
    size_t  count = 0;

    for ( i = 0; i < Info->TagItemCount; i++ ) {
        if ( _stricmp ( item, Info->TagItems[i].Item ) == 0 )
            count++;
    }

    return count;
}

// Return flags of first specified item
unsigned int TagItemFlags ( const char* item, const FileInfo* Info )
{
    size_t  i;

    for ( i = 0; i < Info->TagItemCount; i++ ) {
        if ( _stricmp ( item, Info->TagItems[i].Item ) == 0 )
            return Info->TagItems[i].Flags;
    }

    return 0;
}

// Combines multiple tag values to one ( ITEMx=ab, ITEMx=cd -> dest = ab+cd )
size_t CombineTagValues ( char* dest, const char* item, const FileInfo* Info, const char* separator )
{
    char*   p = dest;
    size_t  i, j;

    for ( i = 0; i < Info->TagItemCount; i++ ) {
        if ( stricmp ( item, Info->TagItems[i].Item ) != 0 )
            continue;

        if ( dest != NULL && Info->TagItems[i].Value != NULL )
            memcpy ( p, Info->TagItems[i].Value, Info->TagItems[i].ValueSize );
        p += Info->TagItems[i].ValueSize;

        for ( j = i + 1; j < Info->TagItemCount; j++ ) {
            if ( stricmp ( item, Info->TagItems[j].Item ) == 0 ) {
                if ( separator != NULL ) {
                    if ( dest != NULL )
                        memcpy ( p, separator, strlen (separator) );
                    p += strlen (separator);
                }
                if ( dest != NULL && Info->TagItems[i].Value != NULL )
                    memcpy ( p, Info->TagItems[j].Value, Info->TagItems[j].ValueSize );
                p += Info->TagItems[j].ValueSize;
            }
        }
        break;
    }

    if ( dest != NULL )
        *p = '\0';

    return p - dest;
}

// Combines multiple UTF-8 tag values to one ( ITEMx=ab, ITEMx=cd -> dest = ab+cd )
size_t CombineTagValuesU ( char* dest, const char* item, const FileInfo* Info, const char* separator )
{
    char*   p = dest;
    size_t  i, j;

    for ( i = 0; i < Info->TagItemCount; i++ ) {
        if ( stricmp ( item, Info->TagItems[i].Item ) != 0 )
            continue;

        if ( dest != NULL && Info->TagItems[i].ValueU != NULL )
            memcpy ( p, Info->TagItems[i].ValueU, Info->TagItems[i].ValueUSize );
        p += Info->TagItems[i].ValueUSize;

        for ( j = i + 1; j < Info->TagItemCount; j++ ) {
            if ( stricmp ( item, Info->TagItems[j].Item ) == 0 ) {
                if ( separator != NULL ) {
                    if ( dest != NULL )
                        memcpy ( p, separator, strlen (separator) );
                    p += strlen (separator);
                }
                if ( dest != NULL && Info->TagItems[i].ValueU != NULL )
                    memcpy ( p, Info->TagItems[j].ValueU, Info->TagItems[j].ValueUSize );
                p += Info->TagItems[j].ValueUSize;
            }
        }
        break;
    }

    if ( dest != NULL )
        *p = '\0';

    return p - dest;
}

// Combines multiple UTF-8 tag values to one. If UTF-8 is empty uses normal value ( ITEMx=ab, ITEMx=cd -> dest = ab+cd )
size_t CombineTagValuesUB ( char* dest, const char* item, const FileInfo* Info, const char* separator )
{
    char*   p = dest;
    size_t  i, j;

    for ( i = 0; i < Info->TagItemCount; i++ ) {
        if ( stricmp ( item, Info->TagItems[i].Item ) != 0 )
            continue;

        if ( Info->TagItems[i].ValueU != NULL ) {
            if ( dest != NULL )
                memcpy ( p, Info->TagItems[i].ValueU, Info->TagItems[i].ValueUSize );
            p += Info->TagItems[i].ValueUSize;
        } else {
            if ( dest != NULL && Info->TagItems[i].Value != NULL )
                memcpy ( p, Info->TagItems[i].Value , Info->TagItems[i].ValueSize  );
            p += Info->TagItems[i].ValueSize;
        }

        for ( j = i + 1; j < Info->TagItemCount; j++ ) {
            if ( stricmp ( item, Info->TagItems[j].Item ) == 0 ) {
                if ( separator != NULL ) {
                    if ( dest != NULL )
                        memcpy ( p, separator, strlen (separator) );
                    p += strlen (separator);
                }
                if ( Info->TagItems[i].ValueU != NULL ) {
                    if ( dest != NULL )
                        memcpy ( p, Info->TagItems[j].ValueU, Info->TagItems[j].ValueUSize );
                    p += Info->TagItems[j].ValueUSize;
                } else {
                    if ( dest != NULL && Info->TagItems[i].Value != NULL )
                        memcpy ( p, Info->TagItems[j].Value , Info->TagItems[j].ValueSize  );
                    p += Info->TagItems[j].ValueSize;
                }
            }
        }
        break;
    }

    if ( dest != NULL )
        *p = '\0';

    return p - dest;
}

// Copy value from tag field to dest, returns length
int CopyTagValue ( char* dest, const char* item, const FileInfo* Info, size_t count )
{
    size_t  i;

    for ( i = 0; i < Info->TagItemCount; i++ ) {
        if ( Info->TagItems[i].Value != NULL && _stricmp ( item, Info->TagItems[i].Item ) == 0 ) {
            if ( count > Info->TagItems[i].ValueSize + 1 ) count = Info->TagItems[i].ValueSize + 1;
            memcpy ( dest, Info->TagItems[i].Value, count );
            return count;
        }
    }

    return 0;
}

// Copy UTF-8 value from tag field to dest, returns length
int CopyTagValueU ( char* dest, const char* item, const FileInfo* Info, size_t count )
{
    size_t  i;

    for ( i = 0; i < Info->TagItemCount; i++ ) {
        if ( Info->TagItems[i].ValueU != NULL && _stricmp ( item, Info->TagItems[i].Item ) == 0 ) {
            if ( count > Info->TagItems[i].ValueUSize + 1 ) count = Info->TagItems[i].ValueUSize + 1;
            memcpy ( dest, Info->TagItems[i].ValueU, count );
            return count;
        }
    }

    return 0;
}

// Copy UTF-8 value (or normal when UTF-8 is missing) from tag field to dest, returns length
int CopyTagValueUB ( char* dest, const char* item, const FileInfo* Info, size_t count )
{
    size_t  i;

    for ( i = 0; i < Info->TagItemCount; i++ ) {
        if ( _stricmp ( item, Info->TagItems[i].Item ) == 0 ) {
            if ( Info->TagItems[i].ValueU != NULL ) {
                if ( count > Info->TagItems[i].ValueUSize + 1 ) count = Info->TagItems[i].ValueUSize + 1;
                memcpy ( dest, Info->TagItems[i].ValueU, count );
                return count;
            } else
            if ( Info->TagItems[i].Value  != NULL ) {
                if ( count > Info->TagItems[i].ValueSize  + 1 ) count = Info->TagItems[i].ValueSize  + 1;
                memcpy ( dest, Info->TagItems[i].Value , count );
                return count;
            }
        }
    }

    return 0;
}

// Return pointer to value in tag field
char* TagValue ( const char* item, const FileInfo* Info )
{
    size_t  i;

    for ( i = 0; i < Info->TagItemCount; i++ ) {
        if ( _stricmp ( item, Info->TagItems[i].Item ) == 0 )
            return Info->TagItems[i].Value;
    }

    return NULL;
}

// Return pointer to UFT-8 value in tag field
char* TagValueU ( const char* item, const FileInfo* Info )
{
    size_t  i;

    for ( i = 0; i < Info->TagItemCount; i++ ) {
        if ( _stricmp ( item, Info->TagItems[i].Item ) == 0 )
            return Info->TagItems[i].ValueU;
    }

    return NULL;
}

// Return pointer to UFT-8 value (or to normal value if UTF-8 is empty) in tag field
char* TagValueUB ( const char* item, const FileInfo* Info )
{
    size_t  i;

    for ( i = 0; i < Info->TagItemCount; i++ ) {
        if ( _stricmp ( item, Info->TagItems[i].Item ) == 0 ) {
            if ( Info->TagItems[i].ValueU != NULL ) {
                return Info->TagItems[i].ValueU;
            } else {
                return Info->TagItems[i].Value;
            }
        }
    }

    return NULL;
}

// Add new field to tagdata
int _NewTagField ( const char* item, const size_t itemsize, const char* value, const size_t valuesize, const char* valueU, const size_t valueUsize, const unsigned int flags, FileInfo* Info )
{
    size_t itemnum;

    if ( item == NULL || itemsize == 0 ) return 1;

    itemnum = Info->TagItemCount++;

    if ( (Info->TagItems = realloc ( Info->TagItems, Info->TagItemCount * sizeof (TagItem) )) == NULL ) {
        fprintf ( Options.output, "_NewTagField: Memory allocation failed.\n" );
        exit (1);
    }

    if ( (Info->TagItems[itemnum].Item   = malloc ( itemsize   + 1 )) == NULL ) {
        fprintf ( Options.output, "_NewTagField: Memory allocation failed.\n" );
        exit (1);
    }
    memcpy ( Info->TagItems[itemnum].Item, item, itemsize );
    Info->TagItems[itemnum].Item[itemsize]         = '\0';
    Info->TagItems[itemnum].ItemSize               = itemsize;
    Info->TagItems[itemnum].Flags                  = flags;

    if ( value  != NULL ) {
        if ( (Info->TagItems[itemnum].Value  = malloc ( valuesize  + 1 )) == NULL ) {
            fprintf ( Options.output, "_NewTagField: Memory allocation failed.\n" );
            exit (1);
        }
        memcpy ( Info->TagItems[itemnum].Value , value , valuesize  );
        Info->TagItems[itemnum].Value [valuesize ] = '\0';
        Info->TagItems[itemnum].ValueSize          = valuesize;
    } else {
        Info->TagItems[itemnum].Value              = NULL;
        Info->TagItems[itemnum].ValueSize          = 0;
    }

    if ( valueU != NULL ) {
        if ( (Info->TagItems[itemnum].ValueU = malloc ( valueUsize + 1 )) == NULL ) {
            fprintf ( Options.output, "_NewTagField: Memory allocation failed.\n" );
            exit (1);
        }
        memcpy ( Info->TagItems[itemnum].ValueU, valueU, valueUsize );
        Info->TagItems[itemnum].ValueU[valueUsize] = '\0';
        Info->TagItems[itemnum].ValueUSize         = valueUsize;
    } else {
        Info->TagItems[itemnum].ValueU             = NULL;
        Info->TagItems[itemnum].ValueUSize         = 0;
    }

    return 0;
}

// Replace old values in tagdata field
int _ReplaceTagFieldValues ( const char* value, const size_t valuesize, const char* valueU, const size_t valueUsize, const unsigned int flags, FileInfo* Info, const size_t itemnum )
{
    free ( Info->TagItems[itemnum].Value  );
    free ( Info->TagItems[itemnum].ValueU );

    Info->TagItems[itemnum].Flags = flags;

    if ( value  != NULL ) {
        if ( (Info->TagItems[itemnum].Value  = malloc ( valuesize  + 1 )) == NULL ) {
            fprintf ( Options.output, "_ReplaceTagFieldValues: Memory allocation failed.\n" );
            exit (1);
        }
        memcpy ( Info->TagItems[itemnum].Value , value , valuesize  );
        Info->TagItems[itemnum].Value [valuesize ] = '\0';
        Info->TagItems[itemnum].ValueSize          = valuesize;
    } else {
        Info->TagItems[itemnum].Value              = NULL;
        Info->TagItems[itemnum].ValueSize          = 0;
    }

    if ( valueU != NULL ) {
        if ( (Info->TagItems[itemnum].ValueU = malloc ( valueUsize + 1 )) == NULL ) {
            fprintf ( Options.output, "_ReplaceTagFieldValues: Memory allocation failed.\n" );
            exit (1);
        }
        memcpy ( Info->TagItems[itemnum].ValueU, valueU, valueUsize );
        Info->TagItems[itemnum].ValueU[valueUsize] = '\0';
        Info->TagItems[itemnum].ValueUSize         = valueUsize;
    } else {
        Info->TagItems[itemnum].ValueU             = NULL;
        Info->TagItems[itemnum].ValueUSize         = 0;
    }

    return 0;
}

// Convert UNICODE to UTF-8
// Return number of bytes written
int unicodeToUtf8 ( const WCHAR* lpWideCharStr, char* lpMultiByteStr, int cwcChars )
{
    const unsigned short*   pwc = (unsigned short *)lpWideCharStr;
    unsigned char*          pmb = (unsigned char  *)lpMultiByteStr;
    const unsigned short*   pwce;
    size_t  cBytes = 0;

    if ( cwcChars >= 0 ) {
        pwce = pwc + cwcChars;
    } else {
        pwce = NULL;
    }

    while ( (pwce == NULL) || (pwc < pwce) ) {
        unsigned short  wc = *pwc++;

        if ( wc < 0x00000080 ) {
            *pmb++ = (char)wc;
            cBytes++;
        } else
        if ( wc < 0x00000800 ) {
            *pmb++ = (char)(0xC0 | ((wc >>  6) & 0x1F));
            cBytes++;
            *pmb++ = (char)(0x80 |  (wc        & 0x3F));
            cBytes++;
        } else
        if ( wc < 0x00010000 ) {
            *pmb++ = (char)(0xE0 | ((wc >> 12) & 0x0F));
            cBytes++;
            *pmb++ = (char)(0x80 | ((wc >>  6) & 0x3F));
            cBytes++;
            *pmb++ = (char)(0x80 |  (wc        & 0x3F));
            cBytes++;
        }
        if ( wc == L'\0' )
            return cBytes;
    }

    return cBytes;
}

// Convert UTF-8 coded string to UNICODE
// Return number of characters converted
int utf8ToUnicode ( const char* lpMultiByteStr, WCHAR* lpWideCharStr, int cmbChars )
{
    const unsigned char*    pmb = (unsigned char  *)lpMultiByteStr;
    unsigned short*         pwc = (unsigned short *)lpWideCharStr;
    const unsigned char*    pmbe;
    size_t  cwChars = 0;

    if ( cmbChars >= 0 ) {
        pmbe = pmb + cmbChars;
    } else {
        pmbe = NULL;
    }

    while ( (pmbe == NULL) || (pmb < pmbe) ) {
        char            mb = *pmb++;
        unsigned int    cc = 0;
        unsigned int    wc;

        while ( (cc < 7) && (mb & (1 << (7 - cc)))) {
            cc++;
        }

        if ( cc == 1 || cc > 6 )                    // illegal character combination for UTF-8
            continue;

        if ( cc == 0 ) {
            wc = mb;
        } else {
            wc = (mb & ((1 << (7 - cc)) - 1)) << ((cc - 1) * 6);
            while ( --cc > 0 ) {
                if ( pmb == pmbe )                  // reached end of the buffer
                    return cwChars;
                mb = *pmb++;
                if ( ((mb >> 6) & 0x03) != 2 )      // not part of multibyte character
                    return cwChars;
                wc |= (mb & 0x3F) << ((cc - 1) * 6);
            }
        }

        if ( wc & 0xFFFF0000 )
            wc = L'?';
        *pwc++ = wc;
        cwChars++;
        if ( wc == L'\0' )
            return cwChars;
    }

    return cwChars;
}

// Insert item to tagdata
int InsertTagField ( const char* item, size_t itemsize, const char* value, size_t valuesize, unsigned int flags, FileInfo* Info, int append, int only_if_longer, int occurrence )
{
    WCHAR*  wszValue;           // Unicode value
    char*   uszValue;           // UTF-8 value
    size_t  len;
    size_t  i;
    int     error;

    if ( itemsize  == 0 ) itemsize  = strlen ( item  );                     // autodetect size
    if ( valuesize == 0 ) valuesize = strlen ( value );                     // autodetect size

    if ( (wszValue = (WCHAR *)malloc ( (valuesize + 1) * 2 )) == NULL ) {
        fprintf ( Options.output, "InsertTagField: Memory allocation failed.\n" );
        exit (1);
    }
    if ( (uszValue = (char *) malloc ( (valuesize + 1) * 3 )) == NULL ) {
        fprintf ( Options.output, "InsertTagField: Memory allocation failed.\n" );
        exit (1);
    }
    if ( valuesize > 0 ) {
        // Convert ANSI value to Unicode
        if ( (len = MultiByteToWideChar ( CP_ACP, 0, value, valuesize /*+ 1*/, wszValue, (valuesize + 1) * 2 )) == 0 ) {
            fprintf ( Options.output, "InsertTagField: MultiByteToWideChar failed.\n" );
            exit (1);
        }
        wszValue[len++] = L'\0';
        // Convert Unicode value to UTF-8
        if ( (len = unicodeToUtf8 ( wszValue, uszValue, len )) == 0 ) {
        //if ( (len = WideCharToMultiByte ( CP_UTF8, 0, wszValue, len, uszValue, (valuesize + 1) * 2, NULL, NULL )) == 0 ) {
            fprintf ( Options.output, "InsertTagField: unicodeToUtf8 failed.\n" );
            exit (1);
        }
    } else {
        len = 1;
        uszValue[0] = '\0';
    }
    free ( wszValue );

    if ( append == 0 ) {
        for ( i = 0; i < Info->TagItemCount; i++ ) {
            if ( _stricmp ( item, Info->TagItems[i].Item ) == 0 ) {
                if ( --occurrence < 0 ) {
                    if ( !only_if_longer || (len-1 > Info->TagItems[i].ValueUSize) ) {
                        error = _ReplaceTagFieldValues ( value, valuesize, uszValue, len-1, flags, Info, i );
                        free ( uszValue );
                        return error;
                    } else {
                        free ( uszValue );
                        return 0;
                    }
                }
            }
        }
    }

    error = _NewTagField ( item, itemsize, value, valuesize, uszValue, len-1, flags, Info );   // insert new field
    free ( uszValue );

    return error;
}

// Insert item in UTF-8 to tagdata
int InsertTagFieldU ( const char* item, size_t itemsize, const char* value, size_t valuesize, unsigned int flags, FileInfo* Info, int append, int only_if_longer, int occurrence )
{
    WCHAR*  wszValue;          // Unicode value
    char*   Value;             // ANSI Value
    size_t  len;
    size_t  i;
    int     error;

    if ( itemsize  == 0 ) itemsize  = strlen ( item  );                     // autodetect size
    if ( valuesize == 0 ) valuesize = strlen ( value );                     // autodetect size

    if ( (wszValue = (WCHAR *)malloc ( (valuesize + 1) * 2 )) == NULL ) {
        fprintf ( Options.output, "InsertTagFieldU: Memory allocation failed.\n" );
        exit (1);
    }
    if ( (Value    = (char *) malloc ( (valuesize + 1) * 2 )) == NULL ) {
        fprintf ( Options.output, "InsertTagFieldU: Memory allocation failed.\n" );
        exit (1);
    }
    if ( valuesize > 0 ) {
        // Convert UTF-8 value to Unicode
        len = utf8ToUnicode ( value, wszValue, valuesize );
        /*
        if ( (len = utf8ToUnicode ( value, wszValue, valuesize )) == 0 ) {
            fprintf ( Options.output, "InsertTagFieldU: utf8ToUnicode failed.\n" );
            exit (1);
        }
        */
        wszValue[len++] = L'\0';
        // Convert Unicode value to ANSI
        if ( (len = WideCharToMultiByte ( CP_ACP, 0, wszValue, len, Value, (valuesize + 1) * 2, NULL, NULL )) == 0 ) {
            fprintf ( Options.output, "InsertTagFieldU: WideCharToMultiByte failed.\n" );
            exit (1);
        }
    } else {
        len = 1;
        Value[0] = '\0';
    }
    free ( wszValue );

    if ( append == 0 ) {
        for ( i = 0; i < Info->TagItemCount; i++ ) {
            if ( _stricmp ( item, Info->TagItems[i].Item ) == 0 ) {
                if ( --occurrence < 0 ) {
                    if ( !only_if_longer || (len-1 > Info->TagItems[i].ValueSize) ) {
                        error = _ReplaceTagFieldValues ( Value, len-1, value, valuesize, flags, Info, i );
                        free ( Value );
                        return error;
                    } else {
                        free ( Value );
                        return 0;
                    }
                }
            }
        }
    }

    error = _NewTagField ( item, itemsize, Value, len-1, value, valuesize, flags, Info );   // insert new field
    free ( Value );

    return error;
}

// Insert item in binary to tagdata
int InsertTagFieldB ( const char* item, size_t itemsize, const char* value, size_t valuesize, unsigned int flags, FileInfo* Info, int append, int only_if_longer, int occurrence )
{
    size_t  i;

    if ( itemsize  == 0 ) itemsize  = strlen ( item );                      // autodetect size
    if ( valuesize == 0 ) return 1;

    flags |= 1<<1;  // set as binary

    if ( append == 0 ) {
        for ( i = 0; i < Info->TagItemCount; i++ ) {
            if ( _stricmp ( item, Info->TagItems[i].Item ) == 0 ) {
                if ( --occurrence < 0 ) {
                    if ( !only_if_longer || (valuesize > Info->TagItems[i].ValueSize) ) {
                        return _ReplaceTagFieldValues ( value, valuesize, NULL, 0, flags, Info, i );
                    } else {
                        return 0;
                    }
                }
            }
        }
    }

    return _NewTagField ( item, itemsize, value, valuesize, NULL, 0, flags, Info );    // insert new field
}

// Replace item in tagdata
int ReplaceTagField ( const size_t itemnum, const char* value, size_t valuesize, unsigned int flags, FileInfo* Info )
{
    WCHAR*  wszValue;          // Unicode value
    char*   uszValue;          // UTF-8 value
    size_t  len;
    int     error;

    if ( itemnum >= Info->TagItemCount )                                    // incorrect item number
        return 1;

    if ( valuesize == 0 ) valuesize = strlen ( value );                     // autodetect size

    if ( (wszValue = (WCHAR *)malloc ( (valuesize + 1) * 2 )) == NULL ) {
        fprintf ( Options.output, "ReplaceTagField: Memory allocation failed.\n" );
        exit (1);
    }
    if ( (uszValue = (char *) malloc ( (valuesize + 1) * 3 )) == NULL ) {
        fprintf ( Options.output, "ReplaceTagField: Memory allocation failed.\n" );
        exit (1);
    }
    // Convert ANSI value to Unicode
    if ( (len = MultiByteToWideChar ( CP_ACP, 0, value, valuesize + 1, wszValue, (valuesize + 1) * 2 )) == 0 ) {
        fprintf ( Options.output, "ReplaceTagField: MultiByteToWideChar failed.\n" );
        exit (1);
    }
    // Convert Unicode value to UTF-8
    if ( (len = unicodeToUtf8 ( wszValue, uszValue, len )) == 0 ) {
    // if ( (len = WideCharToMultiByte ( CP_UTF8, 0, wszValue, len, uszValue, (valuesize + 1) * 2, NULL, NULL )) == 0 ) {
        fprintf ( Options.output, "unicodeToUtf8: WideCharToMultiByte failed.\n" );
        exit (1);
    }
    free ( wszValue );

    error = _ReplaceTagFieldValues ( value, valuesize, uszValue, len-1, flags, Info, itemnum );
    free ( uszValue );

    return error;
}

// Replace item in tagdata, value in UTF-8
int ReplaceTagFieldU ( const size_t itemnum, const char* value, size_t valuesize, unsigned int flags, FileInfo* Info )
{
    WCHAR*  wszValue;          // Unicode value
    char*   Value;             // ANSI Value
    size_t  len;
    int     error;

    if ( itemnum >= Info->TagItemCount )
        return 1;

    if ( valuesize == 0 ) valuesize = strlen ( value );                     // autodetect size

    if ( (wszValue = (WCHAR *)malloc ( (valuesize + 1) * 2 )) == NULL ) {
        fprintf ( Options.output, "ReplaceTagFieldU: Memory allocation failed.\n" );
        exit (1);
    }
    if ( (Value    = (char *) malloc ( (valuesize + 1) * 2 )) == NULL ) {
        fprintf ( Options.output, "ReplaceTagFieldU: Memory allocation failed.\n" );
        exit (1);
    }
    // Convert UTF-8 value to Unicode
    len = utf8ToUnicode ( value, wszValue, valuesize + 1 );
    /*
    if ( (len = utf8ToUnicode ( value, wszValue, valuesize + 1 )) == 0 ) {
        fprintf ( Options.output, "ReplaceTagFieldU: utf8ToUnicode failed.\n" );
        exit (1);
    }
    */
    // Convert Unicode value to ANSI
    if ( (len = WideCharToMultiByte ( CP_ACP, 0, wszValue, len, Value, (valuesize + 1) * 2, NULL, NULL )) == 0 ) {
        fprintf ( Options.output, "ReplaceTagFieldU: WideCharToMultiByte failed.\n" );
        exit (1);
    }
    free ( wszValue );

    error = _ReplaceTagFieldValues ( Value, len-1, value, valuesize, flags, Info, itemnum );
    free ( Value );

    return error;
}

// Replace item in tagdata, value in binary
int ReplaceTagFieldB ( const size_t itemnum, const char* value, size_t valuesize, unsigned int flags, FileInfo* Info )
{
    if ( itemnum >= Info->TagItemCount )                                    // incorrect item number
        return 1;
    if ( valuesize == 0 )                                                   // binary length can't be autodetected
        return 1;

    flags |= 1<<1;  // set as binary

    return _ReplaceTagFieldValues ( value, valuesize, NULL, 0, flags, Info, itemnum );
}

// Replace list separator characters in tag field
int ReplaceListSeparator ( const char* old_sep, const char* new_sep, FileInfo* Info, size_t itemnum )
{
    unsigned char*  new_value;
    unsigned char*  p;
    size_t          os_len;
    size_t          ns_len;
    size_t          count;
    size_t          new_len;
    size_t          i;

    if ( Info->TagItems[itemnum].Flags & 1<<1 )                             // data in binary
        return 0;

    os_len = strlen ( old_sep );
    ns_len = strlen ( new_sep );
    if ( os_len == 0 ) os_len = 1;                                          // allow null character
    if ( ns_len == 0 ) ns_len = 1;

    if ( Info->TagItems[itemnum].ValueU     == NULL ||                      // nothing to do
         Info->TagItems[itemnum].ValueUSize == 0 )
        return 0;

    count = 0;
    for ( i = 0; i < Info->TagItems[itemnum].ValueUSize - os_len + 1; i++ ) {
        if ( memcmp ( Info->TagItems[itemnum].ValueU+i, old_sep, os_len ) == 0 )
            count++;
    }

    if ( count == 0 )
        return 0;

    new_len = Info->TagItems[itemnum].ValueUSize - (count * os_len) + (count * ns_len);
    if ( (new_value = (unsigned char *)malloc ( new_len + 1 )) == NULL ) {
        fprintf ( Options.output, "ReplaceListSeparator: Memory allocation failed.\n" );
        exit (1);
    }

    p = new_value;
    for ( i = 0; i < Info->TagItems[itemnum].ValueUSize; i++ ) {
        if ( i + os_len - 1 >= Info->TagItems[itemnum].ValueUSize ||
             memcmp ( Info->TagItems[itemnum].ValueU+i, old_sep, os_len ) != 0 ) {
            *p++ = Info->TagItems[itemnum].ValueU[i];
        } else {
            memcpy ( p, new_sep, ns_len );
            p += ns_len;
            i += os_len - 1;
        }
    }

    if ( new_len == 0 ) return 0;

    return InsertTagFieldU ( Info->TagItems[itemnum].Item, Info->TagItems[itemnum].ItemSize, new_value, new_len, Info->TagItems[itemnum].Flags, Info, 0, 0, 0 );
}

// Return ID3v1 genre number
int GenreToInteger ( const char* GenreStr)
{
    size_t  i;

    for ( i = 0; i < sizeof (ID3v1GenreList) / sizeof (*ID3v1GenreList); i++ ) {
        if ( 0 == _stricmp ( GenreStr, ID3v1GenreList[i] ) )
            return i;
    }

    return 255;
}

// Get ID3v1 genre name
int GenreToString ( char* GenreStr, const int genre )
{
    if ( genre >= 0 && genre < sizeof (ID3v1GenreList) / sizeof (*ID3v1GenreList) ) {
        strcpy ( GenreStr, ID3v1GenreList[genre] );
        return 0;
    } else {
        GenreStr[0] = '\0';
        return 1;
    }
}

void memcpy_crop ( char* dst, const char* src, size_t len )
{
    size_t i;

    for ( i = 0; i < len; i++ )
        if ( src[i] != '\0' )
            dst[i] = src[i];
        else
            break;

    // dst[i] points behind the string contents
    while ( i > 0 && dst[i-1] == ' ' )
        i--;

    dst[i] = '\0';
}

unsigned long Read_LE_Uint32 ( const unsigned char* p )
{
    return ((unsigned long)p[0] <<  0) |
           ((unsigned long)p[1] <<  8) |
           ((unsigned long)p[2] << 16) |
           ((unsigned long)p[3] << 24);
}

void Write_LE_Uint32 ( unsigned char* p, const unsigned long value )
{
    p[0] = (unsigned char) (value >>  0);
    p[1] = (unsigned char) (value >>  8);
    p[2] = (unsigned char) (value >> 16);
    p[3] = (unsigned char) (value >> 24);
}

int Lyrics3GetNumber5 ( const unsigned char* string )
{
	return ( string[0] - '0') * 10000 +
		   ( string[1] - '0') * 1000 +
		   ( string[2] - '0') * 100 +
		   ( string[3] - '0') * 10 +
		   ( string[4] - '0') * 1;
}

int Lyrics3GetNumber6 ( const unsigned char* string )
{
	return ( string[0] - '0') * 100000 +
		   ( string[1] - '0') * 10000 +
		   ( string[2] - '0') * 1000 +
		   ( string[3] - '0') * 100 +
		   ( string[4] - '0') * 10 +
		   ( string[5] - '0') * 1;
}
