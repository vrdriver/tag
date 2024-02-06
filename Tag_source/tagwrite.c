#include "tags.h"
#include "tagwrite.h"

// Moves parts of ID3v1 title to comment field to overcome the 30 char limitation
int fix_id3v1_title_length ( const char* orig_title, char* new_title, char* new_comment )
{
    int i;
    int left_parenthesis = 0;
    int parenthesis      = 0;

    if ( strlen ( orig_title ) <= 30 ) {
        new_title  [0] = '\0';
        new_comment[0] = '\0';
        return 1;
    }

    for ( i = (int)strlen (orig_title)-1; i >= 0; i-- ) {
        if ( orig_title[i] == ')' ) parenthesis++;
        if ( orig_title[i] == '(' ) parenthesis--;

        if ( orig_title[i] == '(' && parenthesis == 0 ) {
            left_parenthesis = i;
            break;
        }
    }

    if ( (left_parenthesis > 0) && (left_parenthesis <= 30) && (((int)strlen (orig_title) - left_parenthesis) <= 30) ) {
        strncpy ( new_title,    orig_title,                     left_parenthesis );
        strncpy ( new_comment,  orig_title + left_parenthesis,  30               );
        new_title  [left_parenthesis] = '\0';
        new_comment[30              ] = '\0';
    } else {
        strncpy ( new_title,   orig_title,      30 );
        strncpy ( new_comment, orig_title + 30, 30 );
        new_title  [30] = '\0';
        new_comment[30] = '\0';
    }

    return 0;
}

void shortenID3v1Fields ( FileInfo* Info )
{
    const char rep[4] = "...";
    char* p;

    p = TagValue ( APE_TAG_FIELD_TITLE, Info );
    if ( p && strlen (p) > 30 ) {
        strcpy ( p+27, rep );
    }
    p = TagValue ( APE_TAG_FIELD_ARTIST, Info );
    if ( p && strlen (p) > 30 ) {
        strcpy ( p+27, rep );
    }
    p = TagValue ( APE_TAG_FIELD_ALBUM, Info );
    if ( p && strlen (p) > 30 ) {
        strcpy ( p+27, rep );
    }
    p = TagValue ( APE_TAG_FIELD_COMMENT, Info );
    if ( p && strlen (p) > 30 ) {
        strcpy ( p+27, rep );
    }
}

// Writes ID3v1.0 / ID3v1.1 tag
int WriteID3v1Tag ( FILE* fp, FileInfo* Info )
{
    unsigned char   tmp [128];

    if ( fseek ( fp, 0L, SEEK_END ) != 0 )
        return 2;
    if ( ftell (fp) != Info->FileSize )
        return 2;
    if ( fseek ( fp, Info->TagOffset, SEEK_SET ) != 0 )
        return 2;

    if ( !Options.ID3Ext ) {
        shortenID3v1Fields ( Info );
    }

    memset ( tmp, 0, sizeof (tmp) );
    memcpy ( tmp + 0, "TAG", 3 );
    CopyTagValue ( tmp +   3, APE_TAG_FIELD_TITLE , Info, 30 );
    CopyTagValue ( tmp +  33, APE_TAG_FIELD_ARTIST, Info, 30 );
    CopyTagValue ( tmp +  63, APE_TAG_FIELD_ALBUM , Info, 30 );
    CopyTagValue ( tmp +  93, APE_TAG_FIELD_YEAR  , Info,  4 );

    // if track# is used, write ID3v1.1 format
    if ( TagValue ( APE_TAG_FIELD_TRACK, Info ) == NULL ) {
        CopyTagValue ( tmp +  97, APE_TAG_FIELD_COMMENT, Info, 30 );
    } else {
        CopyTagValue ( tmp +  97, APE_TAG_FIELD_COMMENT, Info, 28 );
        tmp[126] = atoi ( TagValue ( APE_TAG_FIELD_TRACK, Info ) );
    }

    if ( Options.ID3Ext ) {
        if ( TagValue ( APE_TAG_FIELD_TITLE, Info ) && strlen (TagValue ( APE_TAG_FIELD_TITLE, Info )) > 30 &&
             (!TagValue ( APE_TAG_FIELD_COMMENT, Info ) || TagValue ( APE_TAG_FIELD_COMMENT, Info )[0] == '\0') ) {
            char title  [32];
            char comment[32];

            if ( fix_id3v1_title_length ( TagValue ( APE_TAG_FIELD_TITLE, Info ), title, comment ) == 0 ) {
                memcpy ( tmp + 3, title, 30 );
                if ( TagValue ( APE_TAG_FIELD_TRACK, Info ) == NULL ) {
                    memcpy ( tmp + 97, comment, 30 );
                } else {
                    memcpy ( tmp + 97, comment, 28 );
                }
            }
        }
    }

    if ( TagValue ( APE_TAG_FIELD_GENRE, Info ) )
        tmp[127] = GenreToInteger ( TagValue ( APE_TAG_FIELD_GENRE, Info ) );
    else
        tmp[127] = (unsigned char)255;

    if ( fwrite ( tmp, 1, sizeof (tmp), fp ) != sizeof (tmp) )
        return 2;

    if ( _chsize ( _fileno (fp), ftell (fp) ) != 0 )
        return 1;

    Info->FileSize = Info->TagOffset + sizeof (tmp);

    fprintf ( Options.output, "ID3v1 tag written.\n" );

    return 0; // CheckID3v1InfoLoss ( Info );
}

// Writes Lyrics3 v2.0 tag
int WriteLyrics3v2Tag ( FILE* fp, FileInfo* Info )
{
    int                             is_lyricstag = 0;
    int	                            len;
    size_t                          size;
    struct Lyrics3TagFooterStruct   T;
    unsigned char                   tmp [128];

    if ( fseek ( fp, 0L, SEEK_END ) != 0 )
        return 2;
    if ( ftell (fp) != Info->FileSize )
        return 2;
    if ( fseek ( fp, Info->TagOffset, SEEK_SET ) != 0 )
        return 2;

    if ( TagValue ( APE_TAG_FIELD_LYRICS,  Info ) ||
        (TagValue ( APE_TAG_FIELD_ARTIST,  Info ) != NULL && strlen (TagValue ( APE_TAG_FIELD_ARTIST,  Info )) > 30 ) ||
        (TagValue ( APE_TAG_FIELD_TITLE,   Info ) != NULL && strlen (TagValue ( APE_TAG_FIELD_TITLE,   Info )) > 30 ) ||
        (TagValue ( APE_TAG_FIELD_ALBUM,   Info ) != NULL && strlen (TagValue ( APE_TAG_FIELD_ALBUM,   Info )) > 30 ) ||
        (TagValue ( APE_TAG_FIELD_COMMENT, Info ) != NULL && strlen (TagValue ( APE_TAG_FIELD_COMMENT, Info )) > 30 ) ) {

        is_lyricstag = 1;
	    len = 11;   // 'LYRICSBEGIN'
        if ( fwrite ( "LYRICSBEGIN", 1, 11, fp ) != 11 )
		    return 2;
	    len += 10;
        if ( TagValue ( APE_TAG_FIELD_LYRICS, Info ) != NULL ) {
            char*   p = TagValue ( APE_TAG_FIELD_LYRICS, Info );
            int     timestamp = 0;
            while ( *p ) {
                if ( *p == '[' && *(p+1) != '\0' && *(p+2) != '\0' && *(p+3) != '\0' ) {
                    if ( *(p+1) >= '0' && *(p+1) <= '9' && *(p+2) >= '0' && *(p+2) <= '9' && *(p+3) == ':' ) {
                        timestamp = 1;
                        break;
                    }
                }
            }
            if ( timestamp == 0 ) {
	            if ( fwrite ( "IND0000210", 1, 10, fp ) != 10 )
                    return 2;
            } else {
	            if ( fwrite ( "IND0000211", 1, 10, fp ) != 10 )
                    return 2;
            }
        } else {
	        if ( fwrite ( "IND0000200", 1, 10, fp ) != 10 )
                return 2;
        }

        // Check if there is lyrics field
        if ( TagValue ( APE_TAG_FIELD_LYRICS, Info ) != NULL ) {
            size = strlen (TagValue ( APE_TAG_FIELD_LYRICS, Info ));
		    if ( size > 99999 ) size = 99999;
		    sprintf ( (char *)tmp, "%05u", size );
		    if ( fwrite ( "LYR", 1, 3, fp ) != 3 )
                return 2;
		    if ( fwrite ( tmp, 1, 5, fp ) != 5 )
                return 2;
		    if ( fwrite ( TagValue ( APE_TAG_FIELD_LYRICS, Info ), 1, size, fp ) != size )
                return 2;
		    len += 8 + size;
        }
        // Check if title field won't fit in ID3 tag
        if ( TagValue ( APE_TAG_FIELD_TITLE, Info ) != NULL ) {
            size = strlen (TagValue ( APE_TAG_FIELD_TITLE, Info ));
	        if ( size > 30 ) {
		        if ( size > 250 ) size = 250;
		        sprintf ( (char *)tmp, "%05u", size );
		        if ( fwrite ( "ETT", 1, 3, fp ) != 3 )
                    return 2;
		        if ( fwrite ( tmp, 1, 5, fp ) != 5 )
                    return 2;
		        if ( fwrite ( TagValue ( APE_TAG_FIELD_TITLE, Info ), 1, size, fp ) != size )
                    return 2;
		        len += 8 + size;
	        }
        }
        // Check if artist field won't fit in ID3 tag
        if ( TagValue ( APE_TAG_FIELD_ARTIST, Info ) != NULL ) {
            size = strlen (TagValue ( APE_TAG_FIELD_ARTIST, Info ));
	        if ( size > 30 ) {
		        if ( size > 250 ) size = 250;
		        sprintf ( (char *)tmp, "%05u", size );
		        if ( fwrite ( "EAR", 1, 3, fp ) != 3 )
                    return 2;
		        if ( fwrite ( tmp, 1, 5, fp ) != 5 )
                    return 2;
		        if ( fwrite ( TagValue ( APE_TAG_FIELD_ARTIST, Info ), 1, size, fp ) != size )
                    return 2;
		        len += 8 + size;
	        }
        }
        // Check if album field won't fit in ID3 tag
        if ( TagValue ( APE_TAG_FIELD_ALBUM, Info ) != NULL ) {
            size = strlen (TagValue ( APE_TAG_FIELD_ALBUM, Info ));
	        if ( size > 30 ) {
		        if ( size > 250 ) size = 250;
		        sprintf ( (char *)tmp, "%05u", size );
		        if ( fwrite ( "EAL", 1, 3, fp ) != 3 )
                    return 2;
		        if ( fwrite ( tmp, 1, 5, fp ) != 5 )
                    return 2;
		        if ( fwrite ( TagValue ( APE_TAG_FIELD_ALBUM, Info ), 1, size, fp ) != size )
                    return 2;
		        len += 8 + size;
	        }
        }
        // Check if comment field won't fit in ID3 tag
        if ( TagValue ( APE_TAG_FIELD_COMMENT, Info ) != NULL ) {
            size = strlen (TagValue ( APE_TAG_FIELD_COMMENT, Info ));
	        if ( size > 30 ) {
		        if ( size > 99999 ) size = 99999;
		        sprintf ( (char *)tmp, "%05u", size );
		        if ( fwrite ( "INF", 1, 3, fp ) != 3 )
                    return 2;
		        if ( fwrite ( tmp, 1, 5, fp ) != 5 )
                    return 2;
		        if ( fwrite ( TagValue ( APE_TAG_FIELD_COMMENT, Info ), 1, size, fp ) != size )
                    return 2;
		        len += 8 + size;
	        }
        }

	    sprintf ( (char *)T.Length, "%06u", len );
	    memcpy ( T.ID, "LYRICS200", 9 );
	    if ( fwrite ( &T, 1, sizeof (T), fp ) != sizeof (T) )
            return 2;
    }

    memset ( tmp, 0, sizeof (tmp) );
    memcpy ( tmp + 0, "TAG", 3 );

    CopyTagValue ( tmp +   3, APE_TAG_FIELD_TITLE , Info, 30 );
    CopyTagValue ( tmp +  33, APE_TAG_FIELD_ARTIST, Info, 30 );
    CopyTagValue ( tmp +  63, APE_TAG_FIELD_ALBUM , Info, 30 );
    CopyTagValue ( tmp +  93, APE_TAG_FIELD_YEAR  , Info,  4 );

    // if track# is used, write ID3v1.1 format
    if ( TagValue ( APE_TAG_FIELD_TRACK, Info ) == NULL ) {
        CopyTagValue ( tmp +  97, APE_TAG_FIELD_COMMENT, Info, 30 );
    } else {
        CopyTagValue ( tmp +  97, APE_TAG_FIELD_COMMENT, Info, 28 );
        tmp[126] = atoi ( TagValue ( APE_TAG_FIELD_TRACK, Info ) );
    }
    if ( TagValue ( APE_TAG_FIELD_GENRE, Info ) )
        tmp[127] = GenreToInteger ( TagValue ( APE_TAG_FIELD_GENRE, Info ) );
    else
        tmp[127] = (unsigned char)255;

    if ( fwrite ( tmp, 1, sizeof (tmp), fp ) != sizeof (tmp) )
        return 2;

    if ( _chsize ( _fileno (fp), ftell (fp) ) != 0 )
        return 1;

    Info->FileSize = Info->TagOffset + len + sizeof (T) + 128;

    if ( is_lyricstag ) {
        fprintf ( Options.output, "Lyrics3 v2.0 tag written.\n" );
    } else {
        fprintf ( Options.output, "ID3v1 tag written.\n" );
    }

    return 0;
}

// Writes APE v1.0 tag
int WriteAPE1Tag ( FILE* fp, FileInfo* Info )
{
    unsigned char               temp[4];
    struct APETagFooterStruct   T;
    char*                       value;
    size_t                      valuelen;
    size_t                      TagCount;
    size_t                      TagSize;
    size_t                      i;

    if ( fseek ( fp, 0, SEEK_END ) != 0 )
        return 2;
    if ( ftell (fp) != Info->FileSize )
        return 2;
    if ( fseek ( fp, Info->TagOffset, SEEK_SET ) != 0 )
        return 2;

    TagCount = 0;

    for ( i = 0; i < Info->TagItemCount; i++ ) {
        if ( Info->TagItems[i].ItemSize > 0 && Info->TagItems[i].ValueSize > 0 ) { // don't write empty items
            if ( TagItemNum ( Info->TagItems[i].Item, Info ) != i )             // only work with first occurrence
                continue;
            if ( (valuelen = CombineTagValues ( NULL, Info->TagItems[i].Item, Info, ListSeparator )) == 0 )
                continue;

            if ( (value = malloc ( valuelen + 1 )) == NULL ) {
                fprintf ( Options.output, "WriteAPE1Tag: Memory allocation failed.\n" );
                exit (1);
            }
            CombineTagValues ( value, Info->TagItems[i].Item, Info, ListSeparator );
            Write_LE_Uint32 ( temp, valuelen + 1 );
            if ( fwrite ( temp, 1, 4, fp ) != 4 ) {
                free ( value );
                return 2;
            }
            memset ( temp, 0, sizeof (temp) );
            if ( fwrite ( temp, 1, 4, fp ) != 4 ) {
                free ( value );
                return 2;
            }
            if ( fwrite ( Info->TagItems[i].Item, 1, Info->TagItems[i].ItemSize + 1, fp ) != Info->TagItems[i].ItemSize + 1 ) {
                free ( value );
                return 2;
            }
            if ( fwrite ( value, 1, valuelen + 1, fp ) != valuelen + 1 ) {
                free ( value );
                return 2;
            }
            TagCount++;
            free ( value );
        }
    }

    TagSize = ftell (fp) - Info->TagOffset + sizeof (T);

    memcpy ( T.ID, "APETAGEX", sizeof (T.ID) );     // ID String
    Write_LE_Uint32 ( T.Version , 1000     );       // Version 1.000
    Write_LE_Uint32 ( T.Length  , TagSize  );       // Tag size
    Write_LE_Uint32 ( T.TagCount, TagCount );       // Number of fields
    memset ( T.Flags   , 0, sizeof (T.Flags   ) );  // Flags
    memset ( T.Reserved, 0, sizeof (T.Reserved) );  // Reserved

    if ( fwrite ( &T, 1, sizeof (T), fp) != sizeof (T) )
        return 2;

    if ( _chsize ( _fileno (fp), ftell (fp) ) != 0 )
        return 2;

    Info->FileSize = Info->TagOffset + TagSize;

    fprintf ( Options.output, "APE v1.0 tag written.\n" );

    return 0;   // no data loss
}

// Writes APE v2.0 tag
int WriteAPE2Tag ( FILE* fp, FileInfo* Info )
{
    unsigned char               temp[4];
    unsigned char*              buff;
    unsigned char*              p;
    struct APETagFooterStruct   T;
    unsigned int                flags;
    char*                       value;
    size_t                      valuelen;
    size_t                      TagCount;
    size_t                      TagSize;
    size_t                      i;

    if ( fseek ( fp, 0L, SEEK_END ) != 0 )
        return 2;
    if ( ftell (fp) != Info->FileSize )
        return 2;
    if ( fseek ( fp, Info->TagOffset, SEEK_SET ) != 0 )
        return 2;

    for ( i = 0; i < Info->TagItemCount; i++ ) {
        if ( ReplaceListSeparator ( ListSeparator, "\0", Info, i ) != 0 )
            return 2;
    }

    TagCount = 0;

    TagSize = sizeof (T);                                                   // calculate size of buffer needed
    for ( i = 0; i < Info->TagItemCount; i++ ) {
        if ( Info->TagItems[i].ItemSize > 0 && Info->TagItems[i].ValueSize > 0 ) {
            if ( TagItemNum ( Info->TagItems[i].Item, Info ) == i ) {
                if ( Info->TagItems[i].Flags & 1<<1 ) {
                    valuelen = Info->TagItems[i].ValueSize;
                } else {
                    if ( (valuelen = CombineTagValuesU /*B*/ ( NULL, Info->TagItems[i].Item, Info, ListSeparator )) == 0 )
                        continue;
                }
                TagSize += 8 + Info->TagItems[i].ItemSize + 1 + valuelen;
                TagCount++;
            }
        }
    }

    if ( (buff = malloc ( TagSize + sizeof (T) )) == NULL ) {
        fprintf ( Options.output, "WriteAPE2Tag: Memory allocation failed.\n" );
        exit (1);
    }

    p = buff;

    flags  = 1<<31;                                     // contains header
    flags |= 1<<29;                                     // this is the header
    memcpy ( T.ID, "APETAGEX", sizeof (T.ID) );         // ID String
    Write_LE_Uint32 ( T.Version , 2000     );           // Version 2.000
    Write_LE_Uint32 ( T.Length  , TagSize  );           // Tag size
    Write_LE_Uint32 ( T.TagCount, TagCount );           // Number of fields
    Write_LE_Uint32 ( T.Flags   , flags    );           // Flags
    memset ( T.Reserved, 0, sizeof (T.Reserved) );      // Reserved
    memcpy ( p, &T, sizeof (T) );   p += sizeof (T);                        // insert header

    for ( i = 0; i < Info->TagItemCount; i++ ) {
        if ( Info->TagItems[i].ItemSize > 0 && Info->TagItems[i].ValueSize > 0 ) {  // don't write empty items
            if ( TagItemNum ( Info->TagItems[i].Item, Info ) != i )         // only work with first occurrence
                continue;
            if ( Info->TagItems[i].Flags & 1<<1 ) {
                valuelen = Info->TagItems[i].ValueSize;
                value    = NULL;
            } else {
                if ( (valuelen = CombineTagValuesU /*B*/ ( NULL, Info->TagItems[i].Item, Info, ListSeparator )) == 0 )
                    continue;

                if ( (value = malloc ( valuelen + 1 )) == NULL ) {
                    fprintf ( Options.output, "WriteAPE2Tag: Memory allocation failed.\n" );
                    exit (1);
                }
                CombineTagValuesU /*B*/ ( value, Info->TagItems[i].Item, Info, ListSeparator );
            }
            Write_LE_Uint32 ( temp, valuelen );
            memcpy ( p, temp, 4 );  p += 4;
            Write_LE_Uint32 ( temp, Info->TagItems[i].Flags );
            memcpy ( p, temp, 4 );  p += 4;

            memcpy ( p, Info->TagItems[i].Item, Info->TagItems[i].ItemSize + 1 );
            p += Info->TagItems[i].ItemSize + 1;

            if ( value ) {
                memcpy ( p, value, valuelen );
            } else {
                memcpy ( p, Info->TagItems[i].Value, valuelen );
            }
            p += valuelen;
            if ( value ) free ( value );
        }
    }

    flags  = 1<<31;                                     // contains header
    memcpy ( T.ID, "APETAGEX", sizeof (T.ID) );         // ID String
    Write_LE_Uint32 ( T.Version , 2000     );           // Version 2.000
    Write_LE_Uint32 ( T.Length  , TagSize  );           // Tag size - header
    Write_LE_Uint32 ( T.TagCount, TagCount );           // Number of fields
    Write_LE_Uint32 ( T.Flags   , flags    );           // Flags
    memset ( T.Reserved, 0, sizeof (T.Reserved) );      // Reserved
    memcpy ( p, &T, sizeof (T) );                                           // insert footer

    if ( fwrite ( buff, 1, TagSize + sizeof (T), fp) != TagSize + sizeof (T) ) {
        free ( buff );
        return 2;
    }

    if ( _chsize ( _fileno (fp), ftell (fp) ) != 0 ) {
        free ( buff );
        return 2;
    }

    Info->FileSize = Info->TagOffset + TagSize + sizeof (T);

    fprintf ( Options.output, "APE v2.0 tag written.\n" );
    free ( buff );

    return 0;   // no data loss
}

#ifdef VORBISSUPPORT

// Writes Vorbis comments
int WriteVorbisTag ( const char* filename, FileInfo* Info )
{
    const char* Items_OggApe[] = {
        "TITLE=",           APE_TAG_FIELD_TITLE,
        "VERSION=",         APE_TAG_FIELD_SUBTITLE,
        "ARTIST=",          APE_TAG_FIELD_ARTIST,
        "ALBUM=",           APE_TAG_FIELD_ALBUM,
        "TRACKNUMBER=",     APE_TAG_FIELD_TRACK,
        //"ORGANIZATION=",    APE_TAG_FIELD_PUBLISHER,
        //"DESCRIPTION=",     APE_TAG_FIELD_COMMENT,
        "GENRE=",           APE_TAG_FIELD_GENRE,
        "DATE=",            APE_TAG_FIELD_YEAR,
        //"LOCATION=",        APE_TAG_FIELD_RECORDLOCATION,
        //"COPYRIGHT=",       APE_TAG_FIELD_COPYRIGHT,
        //"ISRC=",            APE_TAG_FIELD_ISRC
    };

    char                tempname_newogg [L_tmpnam];
    char                tempname_orig   [L_tmpnam];
    char*               item;
    char*               p;
    size_t              item_len;
	vcedit_state*       state;
    vorbis_comment*     vc;
    FILE*               in  = NULL;
    FILE*               out = NULL;
    size_t              i, j;

    if ( (in = fopen ( filename, "rb" )) == NULL )
        return 1;
    state = vcedit_new_state ();
    if ( vcedit_open ( state, in ) < 0 ) {
        // fprintf ( Options.output, "Failed to open file as vorbis: %s\n", vcedit_error (state) );
        if ( in  != NULL ) fclose (in);
        return 1;
    }
    tmpnam ( tempname_newogg );
    if ( (out = fopen ( tempname_newogg, "wb" )) == NULL ) {
        vcedit_clear ( state );
        if ( in  != NULL ) fclose (in);
        return 1;
    }

    vc = vcedit_comments ( state );
    vorbis_comment_clear ( vc );
    vorbis_comment_init  ( vc );

    for ( i = 0; i < Info->TagItemCount; i++ ) {
        int replaced_item = 0;
        if ( Info->TagItems[i].Item[0] == '\0' || Info->TagItems[i].ValueU[0] == '\0' )
            continue;

        for ( j = 0; j < (sizeof (Items_OggApe) / sizeof (*Items_OggApe)) / 2; j++ ) {
            if ( stricmp ( Info->TagItems[i].Item, Items_OggApe[j*2+1] ) == 0 ) {
                replaced_item = 1;
                item_len = strlen (Items_OggApe[j*2]) - 1;
                if ( (item  = malloc ( item_len + 1 )) == NULL ) {
                    fprintf ( Options.output, "WriteVorbisTag: Memory allocation failed.\n" );
                    exit (1);
                }
                strncpy ( item, Items_OggApe[j*2], item_len );
                item[item_len] = '\0';
                break;
            }
        }

        if ( !replaced_item ) {
            item_len = Info->TagItems[i].ItemSize;
            if ( (item  = malloc ( item_len + 1 )) == NULL ) {
                fprintf ( Options.output, "WriteVorbisTag: Memory allocation failed.\n" );
                exit (1);
            }
            strcpy ( item, Info->TagItems[i].Item );
        }

        p = item;
        while ( *p != '\0' ) {
            int ch = *p;
            if ( ch == '=' || ch < 0x20 || ch > 0x7D ) ch = '_';
            *p++ = ch;
        }

        vorbis_comment_add_tag ( vc, item, Info->TagItems[i].ValueU );
        free ( item );
    }

    if ( vcedit_write ( state, out ) < 0 ) {
        fprintf ( Options.output, "Failed to write comments to file: %s\n", vcedit_error (state) );
        vcedit_clear ( state );
        if ( in  != NULL ) fclose (in);
        if ( out != NULL ) fclose (out);
        remove ( tempname_newogg );
        return 1;
    }

    vcedit_clear ( state );
    if ( in  != NULL ) fclose (in);
    if ( out != NULL ) fclose (out);

    tmpnam ( tempname_orig );
    if ( rename ( filename, tempname_orig ) != 0 ) {
        fprintf ( Options.output, "Failed to write comments to file.\n" );
        remove ( tempname_newogg );
        return 1;
    }
    if ( rename ( tempname_newogg, filename ) != 0 ) {
        fprintf ( Options.output, "Failed to write comments to file.\n" );
        rename ( tempname_orig, filename );
        remove ( tempname_newogg );
        return 1;
    }
    remove ( tempname_orig );

    fprintf ( Options.output, "Vorbis comments written.\n" );

    return 0;
}

#endif  // VORBISSUPPORT

#ifdef      FLACSUPPORT

int WriteFLACTag ( const char* filename, FileInfo* Info )
{
    const char* Items_OggApe[] = {
        "TITLE=",           APE_TAG_FIELD_TITLE,
        "VERSION=",         APE_TAG_FIELD_SUBTITLE,
        "ARTIST=",          APE_TAG_FIELD_ARTIST,
        "ALBUM=",           APE_TAG_FIELD_ALBUM,
        "TRACKNUMBER=",     APE_TAG_FIELD_TRACK,
        //"ORGANIZATION=",    APE_TAG_FIELD_PUBLISHER,
        //"DESCRIPTION=",     APE_TAG_FIELD_COMMENT,
        "GENRE=",           APE_TAG_FIELD_GENRE,
        "DATE=",            APE_TAG_FIELD_YEAR,
        //"LOCATION=",        APE_TAG_FIELD_RECORDLOCATION,
        //"COPYRIGHT=",       APE_TAG_FIELD_COPYRIGHT,
        //"ISRC=",            APE_TAG_FIELD_ISRC
    };

    char*               item;
    char*               value;
    char*               p;
    size_t              item_len;
    size_t              i, j;
    int                 tag_written = 0;
    int                 needs_new_block = 1;

    struct FLAC__Metadata_SimpleIterator*       si;

    si = FLAC__metadata_simple_iterator_new ();
    if ( !FLAC__metadata_simple_iterator_init ( si, filename, 0, 0 ) ) {
        FLAC__metadata_simple_iterator_delete ( si );
        return 1;
    }
    if ( !FLAC__metadata_simple_iterator_is_writable ( si ) ) {
        FLAC__metadata_simple_iterator_delete ( si );
        return 1;
    }

    do {
        FLAC__MetadataType  type = FLAC__metadata_simple_iterator_get_block_type ( si );

        if ( type == FLAC__METADATA_TYPE_VORBIS_COMMENT || type == FLAC__METADATA_TYPE_PADDING ) {
            needs_new_block = 0;
            break;
        }
    } while ( FLAC__metadata_simple_iterator_next ( si ) );

    if ( !needs_new_block ) {
        FLAC__metadata_simple_iterator_delete ( si );

        si = FLAC__metadata_simple_iterator_new ();
        if ( !FLAC__metadata_simple_iterator_init ( si, filename, 0, 0 ) ) {
            FLAC__metadata_simple_iterator_delete ( si );
            return 1;
        }
        if ( !FLAC__metadata_simple_iterator_is_writable ( si ) ) {
            FLAC__metadata_simple_iterator_delete ( si );
            return 1;
        }
    }

    do {
        FLAC__MetadataType  type = FLAC__metadata_simple_iterator_get_block_type ( si );

        // Vorbis comment block or padding block
        if ( needs_new_block || (type == FLAC__METADATA_TYPE_VORBIS_COMMENT || type == FLAC__METADATA_TYPE_PADDING) ) {
            if ( !tag_written ) {
                FLAC__StreamMetadata*                       data;
                FLAC__StreamMetadata_VorbisComment_Entry    entry;
                size_t  comments = 0;

                if ( (data = FLAC__metadata_object_new ( FLAC__METADATA_TYPE_VORBIS_COMMENT )) == NULL ) {
                    fprintf ( Options.output, "Failed to create FLAC metadata block.\n" );
                    FLAC__metadata_simple_iterator_delete ( si );
                    return 1;
                }

                for ( i = 0; i < Info->TagItemCount; i++ ) {
                    if ( Info->TagItems[i].Item[0] != '\0' && Info->TagItems[i].ValueU[0] != '\0' )
                        comments++;
                }

                if ( !FLAC__metadata_object_vorbiscomment_resize_comments ( data, comments ) ) {
                    fprintf ( Options.output, "Failed to create FLAC metadata block.\n" );
                    FLAC__metadata_object_delete ( data );
                    FLAC__metadata_simple_iterator_delete ( si );
                    return 1;
                }

                comments = 0;

                for ( i = 0; i < Info->TagItemCount; i++ ) {
                    int replaced_item = 0;
                    if ( Info->TagItems[i].Item[0] == '\0' || Info->TagItems[i].ValueU[0] == '\0' )
                        continue;

                    for ( j = 0; j < (sizeof (Items_OggApe) / sizeof (*Items_OggApe)) / 2; j++ ) {
                        if ( stricmp ( Info->TagItems[i].Item, Items_OggApe[j*2+1] ) == 0 ) {
                            replaced_item = 1;
                            item_len = strlen (Items_OggApe[j*2]) - 1;
                            if ( (item  = malloc ( item_len + 1 )) == NULL ) {
                                fprintf ( Options.output, "WriteFLACTag: Memory allocation failed.\n" );
                                exit (1);
                            }
                            strncpy ( item, Items_OggApe[j*2], item_len );
                            item[item_len] = '\0';
                            break;
                        }
                    }

                    if ( !replaced_item ) {
                        item_len = Info->TagItems[i].ItemSize;
                        if ( (item  = malloc ( item_len + 1 )) == NULL ) {
                            fprintf ( Options.output, "WriteFLACTag: Memory allocation failed.\n" );
                            exit (1);
                        }
                        strcpy ( item, Info->TagItems[i].Item );
                    }

                    p = item;
                    while ( *p != '\0' ) {
                        int ch = *p;
                        if ( ch == '=' || ch < 0x20 || ch > 0x7D ) ch = '_';
                        *p++ = ch;
                    }

                    entry.length = strlen (item) + 1 + Info->TagItems[i].ValueUSize;
                    if ( (value = malloc ( entry.length + 1 )) == NULL ) {
                        fprintf ( Options.output, "WriteFLACTag: Memory allocation failed.\n" );
                        exit (1);
                    }
                    sprintf ( value, "%s=%s", item, Info->TagItems[i].ValueU );
                    entry.entry = value;

                    if ( !FLAC__metadata_object_vorbiscomment_set_comment ( data, comments++, entry, 1 ) ) {
                        fprintf ( Options.output, "Failed to create FLAC metadata block.\n" );
                        FLAC__metadata_object_delete ( data );
                        FLAC__metadata_simple_iterator_delete ( si );
                        return 1;
                    }

                    free ( item );
                    free ( value );
                }

                //FLAC__metadata_object_vorbiscomment_set_vendor_string ( data, entry, 1 );
                /*
                FLAC__metadata_object_vorbiscomment_resize_comments ( FLAC__StreamMetadata *object, unsigned new_num_comments);
                FLAC__metadata_object_vorbiscomment_set_comment ( FLAC__StreamMetadata *object, unsigned comment_num, FLAC__StreamMetadata_VorbisComment_Entry entry, FLAC__bool copy);
                FLAC__metadata_object_vorbiscomment_insert_comment (FLAC__StreamMetadata *object, unsigned comment_num, FLAC__StreamMetadata_VorbisComment_Entry entry, FLAC__bool copy);
                FLAC__metadata_object_vorbiscomment_delete_comment (FLAC__StreamMetadata *object, unsigned comment_num);
                */

                if ( !needs_new_block ) {
                    if ( !FLAC__metadata_simple_iterator_set_block ( si, data, 1 ) ) {  // 1 = use padding
                        fprintf ( Options.output, "Failed to write FLAC metadata block.\n" );
                        FLAC__metadata_object_delete ( data );
                        FLAC__metadata_simple_iterator_delete ( si );
                        return 1;
                    }
                } else {
                    if ( !FLAC__metadata_simple_iterator_insert_block_after ( si, data, 1 ) ) {  // 1 = use padding
                        fprintf ( Options.output, "Failed to write FLAC metadata block.\n" );
                        FLAC__metadata_object_delete ( data );
                        FLAC__metadata_simple_iterator_delete ( si );
                        return 1;
                    }
                }

                FLAC__metadata_object_delete ( data );

                tag_written = 1;
            } else if ( type == FLAC__METADATA_TYPE_VORBIS_COMMENT ) {
                if ( !FLAC__metadata_simple_iterator_delete_block ( si, 0 ) ) {     // 0 = no padding
                    fprintf ( Options.output, "Failed to remove FLAC metadata block.\n" );
                    FLAC__metadata_simple_iterator_delete ( si );
                    return 1;
                }
            }
        }
    } while ( FLAC__metadata_simple_iterator_next ( si ) );

    FLAC__metadata_simple_iterator_delete ( si );

    fprintf ( Options.output, "FLAC tag written.\n" );

    return 0;
}

#endif  // FLACSUPPORT

// Removes all tags from end of file
int WriteNoTag ( FILE* fp, FileInfo* Info )
{
    if ( Info->TagOffset <= 0 )
        return 2;
    if ( fseek ( fp, 0L, SEEK_END ) != 0 )
        return 2;
    if ( ftell (fp) != Info->FileSize )
        return 2;
    if ( fseek ( fp, Info->TagOffset, SEEK_SET ) != 0)
        return 2;
    if ( _chsize ( _fileno (fp), Info->TagOffset ) != 0 )                   // truncate file
        return 2;

    Info->FileSize = Info->TagOffset;                                       // store new file size

    return 0;
}

// Writes most suitable tag
int WriteAutoTag ( FileInfo* Info )
{
    const SupportedTags* t = BestTagList;
    enum format format = FileFormat ( Info->Filename );
    enum tag_t  tag    = ID3v1_tag;

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

    return WriteTag ( Info, tag );
}

// Writes specified tag
int WriteTag ( FileInfo* Info, enum tag_t tag )
{
    FILE*   fp;
    int     error;

    if ( (fp = fopen ( Info->Filename, "rb+" )) == NULL ) {
        fprintf ( Options.output, "Failed to open file.\n" );
        return 1;
    }

    switch ( tag ) {
    case ID3v1_tag:
        if ( (error = WriteID3v1Tag ( fp, Info )) != 0 ) {
            fprintf ( Options.output, "Writing of ID3v1 tag failed.\n" );
        }
        break;
    case Lyrics3_tag:
        if ( (error = WriteLyrics3v2Tag ( fp, Info )) != 0 ) {
            fprintf ( Options.output, "Writing of Lyrics3 v2.0 tag failed.\n" );
        }
        break;
    case ID3v2_tag:
        error = 1;
        break;
    case APE1_tag:
        if ( (error = WriteAPE1Tag  ( fp, Info )) != 0 ) {
            fprintf ( Options.output, "Writing of APE v1.0 tag failed.\n" );
        }
        break;
    case APE2_tag:
        if ( (error = WriteAPE2Tag  ( fp, Info )) != 0 ) {
            fprintf ( Options.output, "Writing of APE v2.0 tag failed.\n" );
        }
        break;
#ifdef  VORBISSUPPORT
    case Vorbis_tag:
        fclose (fp);
        if ( (error = WriteVorbisTag   ( Info->Filename, Info )) != 0 ) {
            fprintf ( Options.output, "Writing of Vorbis comments failed.\n" );
        }
        if ( (fp = fopen ( Info->Filename, "rb+" )) == NULL ) {
            return 1;
        }
        break;
#endif  // VORBISSUPPORT
#ifdef  FLACSUPPORT
    case FLAC_tag:
        fclose (fp);
        if ( (error = WriteFLACTag   ( Info->Filename, Info )) != 0 ) {
            fprintf ( Options.output, "Writing of FLAC tag failed.\n" );
        }
        if ( (fp = fopen ( Info->Filename, "rb+" )) == NULL ) {
            return 1;
        }
        break;
#endif  // FLACSUPPORT
    case auto_tag:
        error = WriteAutoTag ( Info );
        break;
    }

    fclose (fp);

    return error;
}

// Writes tags specified in FileInfo
int WriteTags ( FileInfo* Info )
{
    FILE*   fp;
    size_t  i;
    long    TagOffs;

    if ( Info->TagCount == 0 )
        return 0;

    if ( (fp = fopen ( Info->Filename, "rb+" )) == NULL ) {
        return 1;
    }

    TagOffs = Info->TagOffset;

    for ( i = 0; i < Info->TagCount; i++ ) {
        size_t  tag = (Info->TagCount - 1) - i;
        switch ( Info->Tags[tag].TagType ) {
        case ID3v1_tag:
            if ( WriteID3v1Tag ( fp, Info ) != 0 ) {
                fprintf ( Options.output, "Writing of ID3v1 tag failed.\n" );
                Info->TagOffset = TagOffs;
                fclose (fp);
                return 1;
            }
            Info->TagOffset = Info->FileSize;
            break;
        case Lyrics3_tag:
            if ( WriteLyrics3v2Tag ( fp, Info ) != 0 ) {
                fprintf ( Options.output, "Writing of Lyrics3 v2.0 tag failed.\n" );
                Info->TagOffset = TagOffs;
                fclose (fp);
                return 1;
            }
            Info->TagOffset = Info->FileSize;
            break;
        case APE1_tag:
            if ( WriteAPE1Tag  ( fp, Info ) != 0 ) {
                fprintf ( Options.output, "Writing of APE v1.0 tag failed.\n" );
                Info->TagOffset = TagOffs;
                fclose (fp);
                return 1;
            }
            Info->TagOffset = Info->FileSize;
            break;
        case APE2_tag:
            if ( WriteAPE2Tag  ( fp, Info ) != 0 ) {
                fprintf ( Options.output, "Writing of APE v2.0 tag failed.\n" );
                Info->TagOffset = TagOffs;
                fclose (fp);
                return 1;
            }
            Info->TagOffset = Info->FileSize;
            break;
#ifdef  VORBISSUPPORT
        case Vorbis_tag:
            fclose (fp);
            if ( WriteVorbisTag   ( Info->Filename, Info ) != 0 ) {
                fprintf ( Options.output, "Writing of Vorbis comments failed.\n" );
                Info->TagOffset = TagOffs;
                fclose (fp);
                return 1;
            }
            if ( (fp = fopen ( Info->Filename, "rb+" )) == NULL ) {
                return 1;
            }
            Info->TagOffset = Info->FileSize;
            break;
#endif  // VORBISSUPPORT
#ifdef  FLACSUPPORT
        case FLAC_tag:
            fclose (fp);
            if ( WriteFLACTag   ( Info->Filename, Info ) != 0 ) {
                fprintf ( Options.output, "Writing of FLAC tag failed.\n" );
                Info->TagOffset = TagOffs;
                fclose (fp);
                return 1;
            }
            if ( (fp = fopen ( Info->Filename, "rb+" )) == NULL ) {
                return 1;
            }
            Info->TagOffset = Info->FileSize;
            break;
#endif  // FLACSUPPORT
        case auto_tag:
        case no_tag:
        case guessed_tag:
            if ( Info->TagCount == 1 ) {
                if ( WriteAutoTag  ( Info ) != 0 ) {
                    Info->TagOffset = TagOffs;
                    fclose (fp);
                    return 1;
                }
                Info->TagOffset = Info->FileSize;
            }
            break;
        case ID3v2_tag:
            if ( WriteAutoTag  ( Info ) != 0 ) {
                Info->TagOffset = TagOffs;
                fclose (fp);
                return 1;
            }
            Info->TagOffset = Info->FileSize;
            break;
        }
    }

    Info->TagOffset = TagOffs;
    fclose (fp);

    return 0;
}

#ifdef ID3V2SUPPORT

int FixTagOffsets ( FileInfo* Info, long offd )
{
    size_t i;

    if ( offd == 0 ) return 0;

    for ( i = 0; i < Info->TagCount; i++ ) {
        if ( Info->Tags[i].TagOffset > 0 ) {
            Info->Tags[i].TagOffset -= offd;
        }
    }

    Info->TagOffset -= offd;

    return 0;
}

// Removes ID3v2.x tag
int RemoveID3v2Tag ( const char* filename, FileInfo* Info, int silent )
{
    ID3Tag*             tag;
    long                fsize;
    long                offd;
    FILE*               fp;

    if ( (tag = ID3Tag_New ()) == NULL )
        return 1;

    ID3Tag_LinkWithFlags ( tag, filename, ID3TT_ID3V2 );
    if ( !ID3Tag_HasTagType ( tag, ID3TT_ID3V2 ) ) {
        fprintf ( Options.output, "No ID3v2 tag.\n" );
        ID3Tag_Delete (tag);
        return 0;
    }
    if ( tag == NULL ) return 0;

    fsize = Info->FileSize;
    ID3Tag_Strip ( tag, ID3TT_ID3V2 );
    ID3Tag_Delete (tag);

    if ( (fp = fopen ( filename, "rb" )) == NULL ) {
        fprintf ( Options.output, "RemoveID3v2Tag: file open failed.\n" );
        return 1;
    }

    if ( fseek ( fp, 0, SEEK_END ) != 0 ) {
        fprintf ( Options.output, "RemoveID3v2Tag: fseek failed.\n" );
        fclose (fp);
        return 1;
    }
    Info->FileSize = ftell (fp);
    fclose (fp);

    offd = fsize - Info->FileSize;

    if ( offd == 0 ) {
        if ( !silent ) fprintf ( Options.output, "ID3v2 tag removing failed.\n" );
        return 1;
    } else {
        if ( !silent ) fprintf ( Options.output, "ID3v2 tag removed.\n" );
    }

    FixTagOffsets ( Info, offd );

    return 0;
}

#endif  // ID3V2SUPPORT

#ifdef VORBISSUPPORT

// Removes Vorbis comments
int RemoveVorbisTag ( const char* filename, FileInfo* Info )
{
    char                tempname_newogg [L_tmpnam];
    char                tempname_orig   [L_tmpnam];
	vcedit_state*       state;
    vorbis_comment*     vc;
    FILE*               in  = NULL;
    FILE*               out = NULL;
    long                fsize;
    long                offd;

    if ( (in = fopen ( filename, "rb+" )) == NULL )             // check if file can be opened for writing
        return 1;
    fclose (in);
    if ( (in = fopen ( filename, "rb" )) == NULL )
        return 1;
    state = vcedit_new_state ();
    if ( vcedit_open ( state, in ) < 0 ) {
        // fprintf ( Options.output, "Failed to open file as vorbis: %s\n", vcedit_error (state) );
        if ( in  != NULL ) fclose (in);
        return 1;
    }
    tmpnam ( tempname_newogg );
    if ( (out = fopen ( tempname_newogg, "wb" )) == NULL ) {
        vcedit_clear ( state );
        if ( in  != NULL ) fclose (in);
        return 1;
    }

    fsize = Info->FileSize;

    vc = vcedit_comments ( state );
    vorbis_comment_clear ( vc );
    vorbis_comment_init  ( vc );

    if ( vcedit_write ( state, out ) < 0 ) {
        fprintf ( Options.output, "Failed to write comments to file: %s\n", vcedit_error (state) );
        vcedit_clear ( state );
        if ( in  != NULL ) fclose (in);
        if ( out != NULL ) fclose (out);
        remove ( tempname_newogg );
        return 1;
    }

    vcedit_clear ( state );
    if ( in  != NULL ) fclose (in);
    if ( out != NULL ) fclose (out);

    tmpnam ( tempname_orig );
    if ( rename ( filename, tempname_orig ) != 0 ) {
        fprintf ( Options.output, "Failed to write comments to file.\n" );
        remove ( tempname_newogg );
        return 1;
    }
    if ( rename ( tempname_newogg, filename ) != 0 ) {
        fprintf ( Options.output, "Failed to write comments to file.\n" );
        rename ( tempname_orig, filename );
        remove ( tempname_newogg );
        return 1;
    }
    remove ( tempname_orig );

    in = fopen (filename, "rb" );
    if ( in != NULL ) {
        Info->FileSize = ftell ( in );
        fclose ( in );
    }

    offd = fsize - Info->FileSize;

    FixTagOffsets ( Info, offd );

    return 0;
}

#endif  // VORBISSUPPORT

#ifdef      FLACSUPPORT

int RemoveFLACTag ( const char* filename, FileInfo* Info )
{
    struct FLAC__Metadata_SimpleIterator*       si;
    int tag_found = 0;

    si = FLAC__metadata_simple_iterator_new ();
    if ( !FLAC__metadata_simple_iterator_init ( si, filename, 0, 0 ) ) {
        FLAC__metadata_simple_iterator_delete ( si );
        return 1;
    }
    if ( !FLAC__metadata_simple_iterator_is_writable ( si ) ) {
        FLAC__metadata_simple_iterator_delete ( si );
        return 1;
    }

    do {
        FLAC__MetadataType  type = FLAC__metadata_simple_iterator_get_block_type ( si );

        // Vorbis comment block
        if ( type == FLAC__METADATA_TYPE_VORBIS_COMMENT ) {
            tag_found = 1;

            if ( !FLAC__metadata_simple_iterator_delete_block ( si, 0 ) ) {     // 0 = no padding
                fprintf ( Options.output, "Failed to remove FLAC metadata block.\n" );
                FLAC__metadata_simple_iterator_delete ( si );
                return 1;
            }
        }
    } while ( FLAC__metadata_simple_iterator_next ( si ) );

    FLAC__metadata_simple_iterator_delete ( si );

    return 0;
}

#endif  // FLACSUPPORT

// Removes all tags
int RemoveTags ( FileInfo* Info )
{
    FILE*   fp;
    size_t  i;
    int     errors = 0;
    int     error;

    if ( Info->TagCount == 0 ) {
        fprintf ( Options.output, "No tags to remove.\n" );
        return 1;
    }
    if ( (fp = fopen ( Info->Filename, "rb+" )) == NULL ) {
        fprintf ( Options.output, "Failed to open file.\n" );
        return 1;
    }

    if ( WriteNoTag ( fp, Info ) != 0 ) {
        fclose (fp);
        return 1;
    }

    fclose (fp);

    for ( i = 0; i < Info->TagCount; i++ ) {
#ifdef  ID3V2SUPPORT
        if ( Info->Tags[i].TagType == ID3v2_tag ) {
            error = RemoveID3v2Tag ( Info->Filename, Info, 1 );
            if ( error != 0 ) {
                fprintf ( Options.output, "Failed to remove ID3v2 tag.\n" );
                return 1;
            }
            errors += error;
            //Info->Tags[i].TagType = no_tag;
        }
#endif  // ID3V2SUPPORT
#ifdef  VORBISSUPPORT
        if ( Info->Tags[i].TagType == Vorbis_tag ) {
            error = RemoveVorbisTag   ( Info->Filename, Info );
            if ( error != 0 ) {
                fprintf ( Options.output, "Failed to remove Vorbis comments.\n" );
                return 1;
            }
            errors += error;
            //Info->Tags[i].TagType = no_tag;
        }
#endif
#ifdef  FLACSUPPORT
        if ( Info->Tags[i].TagType == FLAC_tag ) {
            error = RemoveFLACTag   ( Info->Filename, Info );
            if ( error != 0 ) {
                fprintf ( Options.output, "Failed to remove FLAC tag.\n" );
                return 1;
            }
            errors += error;
            //Info->Tags[i].TagType = no_tag;
        }
#endif
    }

    if ( errors == 0 ) {
        size_t count = 0;

        for ( i = 0; i < Info->TagCount; i++ ) {
            if ( Info->Tags[i].TagType >= 0 && Info->Tags[i].TagType < TAGNAMES ) {

#if defined ID3V2SUPPORT && defined VORBISSUPPORT && defined FLACSUPPORT
                fprintf ( Options.output, "%s", TagNames[Info->Tags[i].TagType] );
                count++;
#endif
#if !defined ID3V2SUPPORT && defined VORBISSUPPORT && defined FLACSUPPORT
                if ( Info->Tags[i].TagType != ID3v2_tag ) {
                    fprintf ( Options.output, "%s", TagNames[Info->Tags[i].TagType] );
                    count++;
                }
#endif
#if defined ID3V2SUPPORT && !defined VORBISSUPPORT && defined FLACSUPPORT
                if ( Info->Tags[i].TagType != Vorbis_tag ) {
                    fprintf ( Options.output, "%s", TagNames[Info->Tags[i].TagType] );
                    count++;
                }
#endif
#if defined ID3V2SUPPORT && defined VORBISSUPPORT && !defined FLACSUPPORT
                if ( Info->Tags[i].TagType != FLAC_tag ) {
                    fprintf ( Options.output, "%s", TagNames[Info->Tags[i].TagType] );
                    count++;
                }
#endif
#if !defined ID3V2SUPPORT && !defined VORBISSUPPORT && defined FLACSUPPORT
                if ( Info->Tags[i].TagType != ID3v2_tag && Info->Tags[i].TagType != Vorbis_tag ) {
                    fprintf ( Options.output, "%s", TagNames[Info->Tags[i].TagType] );
                    count++;
                }
#endif
#if !defined ID3V2SUPPORT && defined VORBISSUPPORT && !defined FLACSUPPORT
                if ( Info->Tags[i].TagType != ID3v2_tag && Info->Tags[i].TagType != FLAC_tag ) {
                    fprintf ( Options.output, "%s", TagNames[Info->Tags[i].TagType] );
                    count++;
                }
#endif
#if defined ID3V2SUPPORT && !defined VORBISSUPPORT && !defined FLACSUPPORT
                if ( Info->Tags[i].TagType != Vorbis_tag && Info->Tags[i].TagType != FLAC_tag ) {
                    fprintf ( Options.output, "%s", TagNames[Info->Tags[i].TagType] );
                    count++;
                }
#endif
#if !defined ID3V2SUPPORT && !defined VORBISSUPPORT && !defined FLACSUPPORT
                if ( Info->Tags[i].TagType != ID3v2_tag && Info->Tags[i].TagType != Vorbis_tag && Info->Tags[i].TagType != FLAC_tag ) {
                    fprintf ( Options.output, "%s", TagNames[Info->Tags[i].TagType] );
                    count++;
                }
#endif

            }
            if ( i+1 < Info->TagCount ) fprintf ( Options.output, ", " );
        }
        if ( count == 1 ) {
            fprintf ( Options.output, " tag removed.\n" );
        } else
        if ( count > 1 ) {
            fprintf ( Options.output, " tags removed.\n" );
        }
    }

    return 0;
}

// Checks if ID3v1 tag can store all tag information
// return 0 if can, 1 if can't
int CheckID3v1InfoLoss ( const FileInfo* Info )
{
    const char* items [] = {
        APE_TAG_FIELD_TITLE,
        APE_TAG_FIELD_ARTIST,
        APE_TAG_FIELD_ALBUM,
        APE_TAG_FIELD_YEAR,
        APE_TAG_FIELD_COMMENT,
        APE_TAG_FIELD_TRACK,
        APE_TAG_FIELD_GENRE
    };

    size_t lengths [] = {
        30, // Title
        30, // Artist
        30, // Album
        4,  // Year
        30, // Comment
        3,  // Track
        100 // Genre
    };

    char*       p;
    int         itemfound;
    size_t      i, j;

    if ( TagValue ( APE_TAG_FIELD_TRACK, Info ) ) lengths[4] = 28;

    // check if any value is truncated or left out
    if ( TagValue ( APE_TAG_FIELD_TRACK, Info ) ) {
        p = TagValue ( APE_TAG_FIELD_TRACK, Info );
        for ( i = 0; i < strlen ( TagValue ( APE_TAG_FIELD_TRACK, Info ) ); i++ ) {
            if ( *p < '0' || *p > '9' ) return 1;
            p++;
        }
        if ( atoi ( TagValue ( APE_TAG_FIELD_TRACK, Info ) ) <   0 || atoi ( TagValue ( APE_TAG_FIELD_TRACK, Info ) ) > 255 )
             return 1;
    }

    if ( TagValue ( APE_TAG_FIELD_GENRE, Info ) ) {
        if ( strcmp ( TagValue ( APE_TAG_FIELD_GENRE, Info ), "" ) != 0 &&
             GenreToInteger ( TagValue ( APE_TAG_FIELD_GENRE, Info ) ) >= ID3V1GENRES )     // sizeof (ID3v1GenreList) / sizeof (*ID3v1GenreList) )
            return 1;
    }

    for ( i = 0; i < Info->TagItemCount; i++ ) {
        itemfound = 0;
        for ( j = 0; j < sizeof (items) / sizeof (*items); j++ ) {
            if ( strcmp ( items[j], Info->TagItems[i].Item ) == 0 ) {
                itemfound = 1;
                if ( Info->TagItems[i].ValueSize > lengths[j] ) return 1;
                break;
            }
        }
        if ( !itemfound ) return 1;
    }

    return 0;   // no data loss
}
