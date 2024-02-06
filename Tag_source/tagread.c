#include "tags.h"
#include "tagread.h"

// Reads ID3v1.0 / ID3v1.1 tag
int ReadID3v1Tag ( FILE* fp, FileInfo* Info )
{
    unsigned char   tmp [128];
    unsigned char   value [32];

    if ( fseek ( fp, Info->TagOffset - sizeof (tmp), SEEK_SET ) != 0 )
        return 0;
    if ( fread  ( tmp, 1, sizeof (tmp), fp ) != sizeof (tmp) )
        return 0;
    // check for id3-tag
    if ( 0 != memcmp (tmp, "TAG", 3) )
        return 0;

    memcpy_crop ( value, tmp +  3, 30 );
    if ( value[0] != '\0' )
        InsertTagField ( APE_TAG_FIELD_TITLE  , 0, value, 0, 0, Info, 0, 1, 0 );
    memcpy_crop ( value, tmp + 33, 30 );
    if ( value[0] != '\0' )
        InsertTagField ( APE_TAG_FIELD_ARTIST , 0, value, 0, 0, Info, 0, 1, 0 );
    memcpy_crop ( value, tmp + 63, 30 );
    if ( value[0] != '\0' )
        InsertTagField ( APE_TAG_FIELD_ALBUM  , 0, value, 0, 0, Info, 0, 1, 0 );
    memcpy_crop ( value, tmp + 93,  4 );
    if ( value[0] != '\0' )
        InsertTagField ( APE_TAG_FIELD_YEAR   , 0, value, 0, 0, Info, 0, 1, 0 );
    memcpy_crop ( value, tmp + 97, 30 );
    if ( value[0] != '\0' )
        InsertTagField ( APE_TAG_FIELD_COMMENT, 0, value, 0, 0, Info, 0, 1, 0 );
    if ( tmp[125] == 0 && tmp[126] != 0 ) {
        sprintf ( value, "%d", tmp[126] );
        if ( value[0] != '\0' )
            InsertTagField ( APE_TAG_FIELD_TRACK, 0, value, 0, 0, Info, 0, 1, 0 );
    }
    GenreToString ( value, tmp[127] );
    if ( value[0] != '\0' )
        InsertTagField ( APE_TAG_FIELD_GENRE  , 0, value, 0, 0, Info, 0, 1, 0 );

    Info->TagOffset -= sizeof (tmp);
    if ( (Info->Tags = (TagInfo *)realloc ( Info->Tags, sizeof (TagInfo) * (Info->TagCount + 1) )) == NULL ) {
        fprintf ( Options.output, "ReadID3v1Tag: Memory allocation failed.\n" );
        exit (1);
    }
    Info->Tags[Info->TagCount].TagOffset = Info->TagOffset;
    Info->Tags[Info->TagCount].TagSize   = sizeof (tmp);
    Info->Tags[Info->TagCount].TagType   = ID3v1_tag;
    Info->TagCount++;

    return 0;
}

// Reads Lyrics3 v2.0 tag
int ReadLyrics3v2Tag ( FILE* fp, FileInfo* Info )
{
	int                                 len;
    size_t                              size;
	struct Lyrics3TagFooterStruct       T;
    struct Lyrics3TagField              F;
    unsigned char                       tmpid3 [128];
    unsigned char                       tmp [11];
    unsigned char                       value [32];
    unsigned char*                      tagvalue;

    if ( fseek ( fp, Info->TagOffset - 128, SEEK_SET ) != 0 )
        return 0;
    if ( fread  ( tmpid3, 1, 128, fp ) != 128 )
        return 0;
    // check for id3-tag
    if ( 0 != memcmp (tmpid3, "TAG", 3) )
        return 0;
    if ( fseek ( fp, Info->TagOffset - 128 - sizeof (T), SEEK_SET ) != 0 )
        return 0;
    if ( fread  ( &T, 1, sizeof (T), fp ) != sizeof (T) )
        return 0;

    // check for lyrics3 v2.00 tag
    if ( memcmp (T.ID, "LYRICS200", sizeof (T.ID)) != 0 )
        return 0;

	len = Lyrics3GetNumber6 ( T.Length );

	if ( fseek  ( fp, -128 - (int)sizeof (T) - len, SEEK_END ) != 0 )
        return 0;
    if ( fread  ( tmp, 1, 11, fp ) != 11 )
        return 0;
    if ( memcmp (tmp, "LYRICSBEGIN", 11) != 0 )
        return 0;

	len -= 11; // header 'LYRICSBEGIN'

	while ( len > 0 ) {
        if ( fread ( &F, 1, 8, fp ) != 8 )
            return 1;
        len -= 8;
        if ( (size = Lyrics3GetNumber5 ( F.Length )) == 0 )
            continue;
        len -= size;
        if ( (tagvalue = (unsigned char *)malloc ( size + 1 )) == NULL ) {
            fprintf ( Options.output, "ReadLyrics3v2Tag: Memory allocation failed.\n" );
            exit (1);
        }
        if ( fread ( tagvalue, 1, size, fp ) != size )
            return 1;
        tagvalue[size] = '\0';
        if ( tagvalue[0] == '\0' ) {
            free ( tagvalue );
            continue;
        }

		// Extended Title
		if ( memcmp (F.ID, "ETT", 3) == 0 ) {
			if ( memcmp (tagvalue, tmpid3 +  3, 30 ) == 0 ) {
                InsertTagField ( APE_TAG_FIELD_TITLE  , 0, tagvalue, 0, 0, Info, 0, 1, 0 );
			}
		} else
		// Extended Artist
		if ( memcmp (F.ID, "EAR", 3) == 0 ) {
			if ( memcmp (tagvalue, tmpid3 + 33, 30 ) == 0 ) {
                InsertTagField ( APE_TAG_FIELD_ARTIST , 0, tagvalue, 0, 0, Info, 0, 1, 0 );
			}
		} else
		// Extended Album
		if ( memcmp (F.ID, "EAL", 3) == 0 ) {
			if ( memcmp (tagvalue, tmpid3 + 63, 30 ) == 0 ) {
                InsertTagField ( APE_TAG_FIELD_ALBUM  , 0, tagvalue, 0, 0, Info, 0, 1, 0 );
			}
        } else
		// Additional information
		if ( memcmp (F.ID, "INF", 3) == 0 ) {
            InsertTagField ( APE_TAG_FIELD_COMMENT, 0, tagvalue, 0, 0, Info, 0, 1, 0 );
        } else
		// Lyrics
		if ( memcmp (F.ID, "LYR", 3) == 0 ) {
            InsertTagField ( APE_TAG_FIELD_LYRICS , 0, tagvalue, 0, 0, Info, 0, 1, 0 );
        }

        free ( tagvalue );
	}

    memcpy_crop ( value, tmpid3 +  3, 30 );
    if ( value[0] != '\0' )
        InsertTagField ( APE_TAG_FIELD_TITLE  , 0, value, 0, 0, Info, 0, 1, 0 );
    memcpy_crop ( value, tmpid3 + 33, 30 );
    if ( value[0] != '\0' )
        InsertTagField ( APE_TAG_FIELD_ARTIST , 0, value, 0, 0, Info, 0, 1, 0 );
    memcpy_crop ( value, tmpid3 + 63, 30 );
    if ( value[0] != '\0' )
        InsertTagField ( APE_TAG_FIELD_ALBUM  , 0, value, 0, 0, Info, 0, 1, 0 );
    memcpy_crop ( value, tmpid3 + 93,  4 );
    if ( value[0] != '\0' )
        InsertTagField ( APE_TAG_FIELD_YEAR   , 0, value, 0, 0, Info, 0, 1, 0 );
    memcpy_crop ( value, tmpid3 + 97, 30 );
    if ( value[0] != '\0' )
        InsertTagField ( APE_TAG_FIELD_COMMENT, 0, value, 0, 0, Info, 0, 1, 0 );
    if ( tmpid3[125] == 0 && tmpid3[126] != 0 ) {
        sprintf ( value, "%d", tmpid3[126] );
        if ( value[0] != '\0' )
            InsertTagField ( APE_TAG_FIELD_TRACK, 0, value, 0, 0, Info, 0, 1, 0 );
    }
    GenreToString ( value, tmpid3[127] );
    if ( value[0] != '\0' )
        InsertTagField ( APE_TAG_FIELD_GENRE  , 0, value, 0, 0, Info, 0, 1, 0 );

    Info->TagOffset -= 128 + Lyrics3GetNumber6 ( T.Length ) + sizeof (T);
    if ( (Info->Tags = (TagInfo *)realloc ( Info->Tags, sizeof (TagInfo) * (Info->TagCount + 1) )) == NULL ) {
        fprintf ( Options.output, "ReadLyrics3v2Tag: Memory allocation failed.\n" );
        exit (1);
    }
    Info->Tags[Info->TagCount].TagOffset = Info->TagOffset;
    Info->Tags[Info->TagCount].TagSize   = 128 + Lyrics3GetNumber6 ( T.Length ) + sizeof (T);
    Info->Tags[Info->TagCount].TagType   = Lyrics3_tag;
    Info->TagCount++;

    return 0;
}

// Reads APE v1.0 tag
int ReadAPE1Tag ( FILE* fp, FileInfo* Info )
{
    unsigned long               vsize;
    unsigned long               isize;
    unsigned long               flags;
    unsigned char*              buff;
    unsigned char*              p;
    unsigned char*              end;
    struct APETagFooterStruct   T;
    unsigned long               TagLen;
    unsigned long               TagCount;

    if ( fseek ( fp, Info->TagOffset - sizeof T, SEEK_SET ) != 0 )
        return 0;
    if ( fread ( &T, 1, sizeof T, fp ) != sizeof T )
        return 0;
    if ( memcmp ( T.ID, "APETAGEX", sizeof T.ID ) != 0 )
        return 0;
    if ( Read_LE_Uint32 (T.Version) != 1000 )
        return 0;
    TagLen = Read_LE_Uint32 (T.Length);
    if ( TagLen < sizeof T )
        return 0;
    if ( fseek ( fp, Info->TagOffset - TagLen, SEEK_SET ) != 0 )
        return 0;
    if ( (buff = (unsigned char *)malloc ( TagLen )) == NULL ) {
        fprintf ( Options.output, "ReadAPE1Tag: Memory allocation failed.\n" );
        exit (1);
    }
    if ( fread ( buff, 1, TagLen - sizeof T, fp ) != TagLen - sizeof T ) {
        free ( buff );
        return 0;
    }

    TagCount = Read_LE_Uint32 (T.TagCount);
    end = buff + TagLen - sizeof (T);
    for ( p = buff; p < end && TagCount--; ) {
        vsize = Read_LE_Uint32 ( p ); p += 4;
        flags = Read_LE_Uint32 ( p ); p += 4;
        isize = strlen (p);

        if ( isize > 0 && vsize > 0 )
            InsertTagField ( p, isize, p + isize + 1, 0, 0, Info, 0, 1, 0 );    // flags not used with APE 1.0
        p += isize + 1 + vsize;
    }

    free ( buff );

    Info->TagOffset -= TagLen;
    if ( (Info->Tags = (TagInfo *)realloc ( Info->Tags, sizeof (TagInfo) * (Info->TagCount + 1) )) == NULL ) {
        fprintf ( Options.output, "ReadAPE1Tag: Memory allocation failed.\n" );
        exit (1);
    }
    Info->Tags[Info->TagCount].TagOffset = Info->TagOffset;
    Info->Tags[Info->TagCount].TagSize   = TagLen;
    Info->Tags[Info->TagCount].TagType   = APE1_tag;
    Info->TagCount++;

    return 0;
}

// Reads APE v2.0 tag
int ReadAPE2Tag ( FILE* fp, FileInfo* Info )
{
    unsigned long               vsize;
    unsigned long               isize;
    unsigned long               flags;
    unsigned char*              buff;
    unsigned char*              p;
    unsigned char*              end;
    struct APETagFooterStruct   T;
    unsigned long               TagLen;
    unsigned long               TagCount;
    size_t                      i;

    if ( fseek ( fp, Info->TagOffset - sizeof T, SEEK_SET ) != 0 )
        return 0;
    if ( fread ( &T, 1, sizeof T, fp ) != sizeof T )
        return 0;
    if ( memcmp ( T.ID, "APETAGEX", sizeof T.ID ) != 0 )
        return 0;
    if ( Read_LE_Uint32 (T.Version) != 2000 )
        return 0;
    TagLen = Read_LE_Uint32 (T.Length);
    if ( TagLen < sizeof T )
        return 0;
    if ( fseek ( fp, Info->TagOffset - TagLen, SEEK_SET ) != 0 )
        return 0;
    if ( (buff = (unsigned char *)malloc ( TagLen )) == NULL ) {
        fprintf ( Options.output, "ReadAPE2Tag: Memory allocation failed.\n" );
        exit (1);
    }
    if ( fread ( buff, 1, TagLen - sizeof T, fp ) != TagLen - sizeof T ) {
        free ( buff );
        return 0;
    }

    TagCount = Read_LE_Uint32 (T.TagCount);
    end = buff + TagLen - sizeof (T);
    for ( p = buff; p < end && TagCount--; ) {
        vsize = Read_LE_Uint32 ( p ); p += 4;
        flags = Read_LE_Uint32 ( p ); p += 4;
        isize = strlen (p);

        if ( isize > 0 && vsize > 0 ) {
            if ( !(flags & 1<<1) ) {   // insert UTF-8 string
                InsertTagFieldU ( p, isize, p + isize + 1, vsize, flags, Info, 0, 1, 0 );
            } else {                    // insert binary data
                InsertTagFieldB ( p, isize, p + isize + 1, vsize, flags, Info, 0, 1, 0 );
            }
        }
        p += isize + 1 + vsize;
    }

    free ( buff );

    Info->TagOffset -= TagLen;
    if ( (Info->Tags = (TagInfo *)realloc ( Info->Tags, sizeof (TagInfo) * (Info->TagCount + 1) )) == NULL ) {
        fprintf ( Options.output, "ReadAPE2Tag: Memory allocation failed.\n" );
        exit (1);
    }
    Info->Tags[Info->TagCount].TagOffset = Info->TagOffset;
    Info->Tags[Info->TagCount].TagSize   = TagLen;
    Info->Tags[Info->TagCount].TagType   = APE2_tag;
    Info->TagCount++;

    for ( i = 0; i < Info->TagItemCount; i++ ) {
        if ( ReplaceListSeparator ( "\0", ListSeparator, Info, i ) != 0 )
            return 2;
    }

    if ( Read_LE_Uint32 (T.Flags) & 1<<31 ) {       // Tag contains header
        Info->TagOffset -= sizeof (T);
        Info->Tags[Info->TagCount-1].TagOffset -= sizeof (T);
        Info->Tags[Info->TagCount-1].TagSize   += sizeof (T);
    } else {                                        // Check if footer was incorrect
        if ( fseek ( fp, Info->TagOffset - sizeof T, SEEK_SET ) != 0 )
            return 0;
        if ( fread ( &T, 1, sizeof T, fp ) != sizeof T )
            return 0;
        if ( memcmp ( T.ID, "APETAGEX", sizeof T.ID ) != 0 )
            return 0;
        if ( Read_LE_Uint32 (T.Version) != 2000 )
            return 0;
        if ( Read_LE_Uint32 (T.Flags) & 1<<29 ) {     // This is header
            Info->TagOffset -= sizeof T;
            Info->Tags[Info->TagCount-1].TagOffset -= sizeof (T);
            Info->Tags[Info->TagCount-1].TagSize   += sizeof (T);
        }
    }

    return 0;
}

#ifdef ID3V2SUPPORT

// Helper function to convert ID3v2 frame ID to APE tag item name
int ConvertID3v2FrameToAPE ( const ID3Frame *frame, char* item, size_t item_len, char* value, size_t value_len, size_t* valuesize )
{
    ID3Field*       text;
    ID3Field*       desc;
    ID3Field*       url;
    ID3Field*       bin;
    ID3_FrameID     frameid = ID3Frame_GetID (frame);

    if ( item_len == 0 || value_len == 0 )
        return 0;

    *valuesize = 0;
    item  [0]  = '\0';
    value [0]  = '\0';

    switch ( frameid )
    {
    case ID3FID_ALBUM:
    case ID3FID_BPM:
    case ID3FID_COMPOSER:
    case ID3FID_CONTENTTYPE:
    case ID3FID_COPYRIGHT:
    case ID3FID_DATE:
    case ID3FID_PLAYLISTDELAY:
    case ID3FID_ENCODEDBY:
    case ID3FID_LYRICIST:
    case ID3FID_FILETYPE:
    case ID3FID_TIME:
    case ID3FID_CONTENTGROUP:
    case ID3FID_TITLE:
    case ID3FID_SUBTITLE:
    case ID3FID_INITIALKEY:
    case ID3FID_LANGUAGE:
    case ID3FID_SONGLEN:
    case ID3FID_MEDIATYPE:
    case ID3FID_ORIGALBUM:
    case ID3FID_ORIGFILENAME:
    case ID3FID_ORIGLYRICIST:
    case ID3FID_ORIGARTIST:
    case ID3FID_ORIGYEAR:
    case ID3FID_FILEOWNER:
    case ID3FID_LEADARTIST:
    case ID3FID_BAND:
    case ID3FID_CONDUCTOR:
    case ID3FID_MIXARTIST:
    case ID3FID_PARTINSET:
    case ID3FID_PUBLISHER:
    case ID3FID_TRACKNUM:
    case ID3FID_RECORDINGDATES:
    case ID3FID_NETRADIOSTATION:
    case ID3FID_NETRADIOOWNER:
    case ID3FID_SIZE:
    case ID3FID_ISRC:
    case ID3FID_ENCODERSETTINGS:
    case ID3FID_YEAR:
    case ID3FID_COMMENT:
    case ID3FID_UNSYNCEDLYRICS:
        if ( (text = ID3Frame_GetField ( frame, ID3FN_TEXT )) != NULL ) {
            ID3Field_GetASCII ( text, value, value_len );
        }
        break;
    default:
    case ID3FID_USERTEXT:
        if ( (text = ID3Frame_GetField ( frame, ID3FN_TEXT )) != NULL ) {
            ID3Field_GetASCII ( text, value, value_len );
        }
        if ( (desc = ID3Frame_GetField ( frame, ID3FN_DESCRIPTION )) != NULL ) {
            ID3Field_GetASCII ( desc, item, item_len );
        }
        break;
    case ID3FID_WWWAUDIOFILE:
    case ID3FID_WWWARTIST:
    case ID3FID_WWWAUDIOSOURCE:
    case ID3FID_WWWCOMMERCIALINFO:
    case ID3FID_WWWCOPYRIGHT:
    case ID3FID_WWWPUBLISHER:
    case ID3FID_WWWPAYMENT:
    case ID3FID_WWWRADIOPAGE:
        if ( (url = ID3Frame_GetField ( frame, ID3FN_URL )) != NULL ) {
            ID3Field_GetASCII ( url, value, value_len );
        }
        break;
    case ID3FID_WWWUSER:
        if ( (url = ID3Frame_GetField ( frame, ID3FN_URL )) != NULL ) {
            ID3Field_GetASCII ( url, value, value_len );
        }
        if ( (desc = ID3Frame_GetField ( frame, ID3FN_DESCRIPTION )) != NULL ) {
            ID3Field_GetASCII ( desc, item, item_len );
        }
        break;
    case ID3FID_PICTURE:
        if ( (bin = ID3Frame_GetField ( frame, ID3FN_DATA )) != NULL ) {
            ID3Field_GetBINARY ( bin, value, value_len );
            *valuesize = ID3Field_Size ( bin );
        }
        if ( (desc = ID3Frame_GetField ( frame, ID3FN_DESCRIPTION )) != NULL ) {
            ID3Field_GetASCII ( desc, item, item_len );
        }
        break;
    }

    switch ( frameid ) {
    case ID3FID_COMMENT:
        strncpy ( item, APE_TAG_FIELD_COMMENT, item_len );
        break;
    case ID3FID_LEADARTIST:
        strncpy ( item, APE_TAG_FIELD_ARTIST, item_len );
        break;
    case ID3FID_TITLE:
        strncpy ( item, APE_TAG_FIELD_TITLE, item_len );
        break;
    case ID3FID_SUBTITLE:
        strncpy ( item, APE_TAG_FIELD_SUBTITLE, item_len );
        break;
    case ID3FID_ALBUM:
        strncpy ( item, APE_TAG_FIELD_ALBUM, item_len );
        break;
    case ID3FID_COMPOSER:
        strncpy ( item, APE_TAG_FIELD_COMPOSER, item_len );
        break;
    case ID3FID_COPYRIGHT:
        strncpy ( item, APE_TAG_FIELD_COPYRIGHT, item_len );
        break;
    case ID3FID_DATE:
        strncpy ( item, APE_TAG_FIELD_RECORDDATE, item_len );
        break;
    case ID3FID_YEAR:
        strncpy ( item, APE_TAG_FIELD_YEAR, item_len );
        break;
    case ID3FID_CONDUCTOR:
        strncpy ( item, APE_TAG_FIELD_CONDUCTOR, item_len );
        break;
    case ID3FID_PUBLISHER:
        strncpy ( item, APE_TAG_FIELD_PUBLISHER, item_len );
        break;
    case ID3FID_TRACKNUM:
        strncpy ( item, APE_TAG_FIELD_TRACK, item_len );
        break;
    case ID3FID_RECORDINGDATES:
        strncpy ( item, APE_TAG_FIELD_RECORDDATE, item_len );
        break;
    case ID3FID_CONTENTTYPE:
        strncpy ( item, APE_TAG_FIELD_GENRE, item_len );
        break;
    case ID3FID_MEDIATYPE:
        strncpy ( item, APE_TAG_FIELD_MEDIA, item_len );
        break;
    case ID3FID_ISRC:
        strncpy ( item, APE_TAG_FIELD_ISRC, item_len );
        break;
    case ID3FID_WWWCOMMERCIALINFO:
        strncpy ( item, APE_TAG_FIELD_BUY_URL, item_len );
        break;
    case ID3FID_WWWCOPYRIGHT:
        strncpy ( item, APE_TAG_FIELD_COPYRIGHT_URL, item_len );
        break;
    case ID3FID_WWWAUDIOFILE:
        strncpy ( item, APE_TAG_FIELD_FILE_URL, item_len );
        break;
    case ID3FID_WWWARTIST:
        strncpy ( item, APE_TAG_FIELD_ARTIST_URL, item_len );
        break;
    case ID3FID_WWWPUBLISHER:
        strncpy ( item, APE_TAG_FIELD_PUBLISHER_URL, item_len );
        break;
    case ID3FID_WWWUSER:
        strncpy ( item, APE_TAG_FIELD_RELATED_URL, item_len );
        break;
    //case ID3FID_SYNCEDLYRICS:
        //return APE_TAG_FIELD_LYRICS;
        //break;
    case ID3FID_UNSYNCEDLYRICS:
        strncpy ( item, APE_TAG_FIELD_LYRICS, item_len );
        break;
    case ID3FID_PARTINSET:
        strncpy ( item, "Part", item_len );
        break;
    //case ID3FID_INVOLVEDPEOPLE:
    //    return "Involved People";
    case ID3FID_WWWRADIOPAGE:
        strncpy ( item, "Web Radio URL", item_len );
        break;
    case ID3FID_WWWAUDIOSOURCE:
        strncpy ( item, "Source URL", item_len );
        break;
    case ID3FID_CDID:
        strncpy ( item, "CDID", item_len );
        break;
    case ID3FID_ENCODEDBY:
        strncpy ( item, "Encoded By", item_len );
        break;
    case ID3FID_CONTENTGROUP:
        strncpy ( item, "Content Group", item_len );
        break;
    case ID3FID_FILEOWNER:
        strncpy ( item, "File Owner", item_len );
        break;
    case ID3FID_MIXARTIST:
        strncpy ( item, "Mix Artist", item_len );
        break;
    case ID3FID_NETRADIOSTATION:
        strncpy ( item, "Internet Radio Station", item_len );
        break;
    case ID3FID_NETRADIOOWNER:
        strncpy ( item, "Internet Radio Owner", item_len );
        break;
    case ID3FID_ENCODERSETTINGS:
        strncpy ( item, "Encoder", item_len );
        break;
    case ID3FID_TERMSOFUSE:
        strncpy ( item, "Terms Of Use", item_len );
        break;
    case ID3FID_LYRICIST:
        strncpy ( item, "Lyricist", item_len );
        break;
    case ID3FID_ORIGALBUM:
        strncpy ( item, "Original Album", item_len );
        break;
    case ID3FID_ORIGLYRICIST:
        strncpy ( item, "Original Lyricist", item_len );
        break;
    case ID3FID_ORIGARTIST:
        strncpy ( item, "Original Artist", item_len );
        break;
    case ID3FID_ORIGYEAR:
        strncpy ( item, "Original Year", item_len );
        break;
    case ID3FID_BAND:
        strncpy ( item, "Band", item_len );
        break;
    case ID3FID_PICTURE:
        strncpy ( item, APE_TAG_FIELD_COVER_ART_FRONT, item_len );
        break;
    }

    return 0;
}

// Reads ID3v2.x tag
int ReadID3v2Tag ( const char* filename, FileInfo* Info )
{
    ID3Tag*             tag;
    ID3Frame*           frame;
    ID3TagIterator*     iter;
    char                item [4096];
    char*               value;
    int                 value_max;

    if ( (tag = ID3Tag_New ()) == NULL )
        return 1;

    ID3Tag_LinkWithFlags ( tag, filename, ID3TT_ID3V2 );
    if ( tag == NULL ) {
        ID3Tag_Delete (tag);
        return 0;
    }

    if ( !ID3Tag_HasTagType ( tag, ID3TT_ID3V2 ) ) {
        ID3Tag_Delete (tag);
        return 0;
    }

    if ( (iter = ID3Tag_CreateIterator (tag)) == NULL ) {
        ID3Tag_Delete (tag);
        return 0;
    }

    value_max = 8;
    while (value_max > 0 ) {
        if ( (value = malloc ( value_max * 1024 * 1024 )) == NULL ) {
            value_max--;
        } else {
            break;
        }
    }
    if ( value == NULL ) {
        fprintf ( Options.output, "ReadID3v2Tag: Memory allocation failed.\n" );
        exit (1);
    }
    value_max *= 1024*1024;

    while ( (frame = ID3TagIterator_GetNext (iter)) != NULL ) {
        unsigned int    flags;
        size_t          size;
        ConvertID3v2FrameToAPE ( frame, item, sizeof (item), value, value_max, &size );

        if ( item[0] == '\0' || value[0] == '\0' )
            continue;

        if ( stricmp ( item, APE_TAG_FIELD_GENRE ) == 0 ) {
            char *p = value;
            if ( *p == '(' ) {
                p++;
                while ( *p && (*p >= '0' && *p <= '9' ) )
                    p++;
                if ( *p && *p == ')' ) {
                    p++;
                } else {
                    p = value;
                }
                if ( *p != '\0' ) {
                    InsertTagField ( item, 0, p, 0, 0, Info, 0, 1, 0 );
                }
                continue;
            }
        }

        if ( stricmp ( item, APE_TAG_FIELD_COVER_ART_FRONT ) == 0 ) {
            flags = 1<<1;   // binary
        } else {
            flags = 0;
        }
        if ( value[0] != '\0' ) {
            if ( !(flags & 1<<1) ) {
                InsertTagField  ( item, 0, value, size, flags, Info, 0, 1, 0 );
            } else {
                InsertTagFieldB ( item, 0, value, size, flags, Info, 0, 1, 0 );
            }
        }
    }

    free ( value );

    if ( (Info->Tags = (TagInfo *)realloc ( Info->Tags, sizeof (TagInfo) * (Info->TagCount + 1) )) == NULL ) {
        fprintf ( Options.output, "ReadID3v2Tag: Memory allocation failed.\n" );
        exit (1);
    }
    Info->Tags[Info->TagCount].TagOffset = 0;
    Info->Tags[Info->TagCount].TagSize   = 0;
    Info->Tags[Info->TagCount].TagType   = ID3v2_tag;
    Info->TagCount++;

    ID3TagIterator_Delete (iter);
    ID3Tag_Delete (tag);

    return 0;
}

#endif  // ID3V2SUPPORT

#ifdef VORBISSUPPORT

// Reads Vorbis comments
int ReadVorbisTag ( const char* filename, FileInfo* Info )
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

    int                 ItemsFound [(sizeof (Items_OggApe) / sizeof (*Items_OggApe)) / 2];
    OggVorbis_File      vf;
    vorbis_comment*     vc;
    FILE*               fp;
    size_t              item_len;
    size_t              value_len;
    size_t              i, j;

    if ( (fp = fopen ( filename, "rb" )) == NULL )
        return 1;
    if ( ov_open (fp, &vf, NULL, 0 ) != 0 ) {
        fclose (fp);
        return 1;
    }
    if ( (vc = ov_comment ( &vf, -1 )) == NULL ) {
        ov_clear (&vf);
        return 1;
    }

    memset ( &ItemsFound, 0, sizeof (ItemsFound) );

    for ( i = 0; i < (size_t)vc->comments; i++ ) {
        int new_item = 1;
        if ( vc->user_comments[i] == NULL || vc->user_comments[i][0] == '\0' )  // empty field
            continue;

        for ( j = 0; j < (sizeof (Items_OggApe) / sizeof (*Items_OggApe)) / 2; j++ ) {
            item_len = strlen (Items_OggApe[j*2]);
            if ( strnicmp ( vc->user_comments[i], Items_OggApe[j*2], item_len ) == 0 ) {
                new_item = 0;
                if ( vc->user_comments[i][item_len] != '\0' ) {
                    if ( ItemsFound[j] == 0 ) {
                        ItemsFound[j] = 1;
                        InsertTagFieldU ( Items_OggApe[j*2+1], 0, (char *)(vc->user_comments[i] + item_len), 0, 0, Info, 0, 1, 0 );
                    } else {
                        InsertTagFieldU ( Items_OggApe[j*2+1], 0, (char *)(vc->user_comments[i] + item_len), 0, 0, Info, 1, 1, 0 );
                    }
                }
                break;
            }
        }

        if ( !new_item )
            continue;
        item_len = 0;
        while ( vc->user_comments[i][item_len] != '=' && vc->user_comments[i][item_len] != '\0' )
            item_len++;
        if ( vc->user_comments[i][item_len] == '\0' || vc->user_comments[i][item_len + 1] == '\0' )
            continue;
        value_len = strlen ( (char *)(vc->user_comments[i] + item_len + 1) );
        InsertTagFieldU ( vc->user_comments[i], item_len, (char *)(vc->user_comments[i] + item_len + 1), value_len, 0, Info, 1, 1, 0 );
    }

    ov_clear (&vf);

    if ( (Info->Tags = (TagInfo *)realloc ( Info->Tags, sizeof (TagInfo) * (Info->TagCount + 1) )) == NULL ) {
        fprintf ( Options.output, "ReadVorbisTag: Memory allocation failed.\n" );
        exit (1);
    }
    Info->Tags[Info->TagCount].TagOffset = 0;
    Info->Tags[Info->TagCount].TagSize   = 0;
    Info->Tags[Info->TagCount].TagType   = Vorbis_tag;
    Info->TagCount++;

    return 0;
}

#endif  // VORBISSUPPORT

#ifdef      FLACSUPPORT

int ReadFLACTag ( const char* filename, FileInfo* Info )
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

    int                 ItemsFound [(sizeof (Items_OggApe) / sizeof (*Items_OggApe)) / 2];
    size_t              item_len;
    size_t              value_len;
    size_t              i, j;
    int                 tag_found = 0;

    struct FLAC__Metadata_SimpleIterator*       si;

    si = FLAC__metadata_simple_iterator_new ();
    if ( !FLAC__metadata_simple_iterator_init ( si, filename, 0, 0 ) ) {
        FLAC__metadata_simple_iterator_delete ( si );
        return 1;
    }

    memset ( &ItemsFound, 0, sizeof (ItemsFound) );

    do {
        FLAC__MetadataType      type;
        FLAC__StreamMetadata*   data;

        type = FLAC__metadata_simple_iterator_get_block_type ( si );

        // Stream info block
        /*
        if ( type == FLAC__METADATA_TYPE_STREAMINFO ) {
            data = FLAC__metadata_simple_iterator_get_block ( si );

            if ( data != NULL ) {
                FLAC__StreamMetadata_StreamInfo*            info = &data->data.stream_info;

                unsigned samplerate = info->sample_rate;
                unsigned channels = info->channels;
                unsigned bitspersample = info->bits_per_sample;
                FLAC__uint64 samples = info->total_samples;
                unsigned long length = (unsigned long)(samples / samplerate);

                FLAC__metadata_object_delete ( data );
            }
        }
        */

        // Vorbis comment block
        if ( type == FLAC__METADATA_TYPE_VORBIS_COMMENT ) {
            tag_found = 1;
            data = FLAC__metadata_simple_iterator_get_block ( si );

            if ( data != NULL ) {
                FLAC__StreamMetadata_VorbisComment_Entry*   vc = data->data.vorbis_comment.comments;
                // unsigned int    i;

                /*
                printf ( "vendor_string = '%s'\n", data->data.vorbis_comment.vendor_string );
                printf ( "num_comments  = %u\n", data->data.vorbis_comment.num_comments );

                for ( i = 0; i < data->data.vorbis_comment.num_comments; i++ ) {
                    char* p = vc[i].entry;
                    unsigned int j;

                    printf ( "comment[%u] = '", i );
                    for ( j = 0; j < vc[i].length; j++ ) {
                        printf ( "%c", *p++ );
                    }
                    printf ( "'\n" );
                }
                */

                for ( i = 0; i < data->data.vorbis_comment.num_comments; i++ ) {
                    int new_item = 1;
                    if ( vc[i].entry == NULL || vc[i].entry[0] == '\0' )  // empty field
                        continue;

                    for ( j = 0; j < (sizeof (Items_OggApe) / sizeof (*Items_OggApe)) / 2; j++ ) {
                        item_len = strlen (Items_OggApe[j*2]);
                        if ( strnicmp ( vc[i].entry, Items_OggApe[j*2], item_len ) == 0 ) {
                            new_item = 0;
                            if ( vc[i].entry[item_len] != '\0' ) {
                                if ( ItemsFound[j] == 0 ) {
                                    ItemsFound[j] = 1;
                                    InsertTagFieldU ( Items_OggApe[j*2+1], 0, (char *)(vc[i].entry + item_len), vc[i].length - item_len, 0, Info, 0, 1, 0 );
                                } else {
                                    InsertTagFieldU ( Items_OggApe[j*2+1], 0, (char *)(vc[i].entry + item_len), vc[i].length - item_len, 0, Info, 1, 1, 0 );
                                }
                            }
                            break;
                        }
                    }

                    if ( !new_item )
                        continue;
                    item_len = 0;
                    while ( vc[i].entry[item_len] != '=' && vc[i].entry[item_len] != '\0' )
                        item_len++;
                    if ( vc[i].entry[item_len] == '\0' || vc[i].entry[item_len + 1] == '\0' )
                        continue;
                    value_len = vc[i].length - item_len - 1;
                    InsertTagFieldU ( vc[i].entry, item_len, (char *)(vc[i].entry + item_len + 1), value_len, 0, Info, 1, 1, 0 );
                }

                
                FLAC__metadata_object_delete ( data );
            }
        }
    } while ( FLAC__metadata_simple_iterator_next ( si ) );

    FLAC__metadata_simple_iterator_delete ( si );

    if ( tag_found ) {
        if ( (Info->Tags = (TagInfo *)realloc ( Info->Tags, sizeof (TagInfo) * (Info->TagCount + 1) )) == NULL ) {
            fprintf ( Options.output, "ReadFLACTag: Memory allocation failed.\n" );
            exit (1);
        }
        Info->Tags[Info->TagCount].TagOffset = 0;
        Info->Tags[Info->TagCount].TagSize   = 0;
        Info->Tags[Info->TagCount].TagType   = FLAC_tag;
        Info->TagCount++;
    }

    return 0;
}

#endif  // FLACSUPPORT

// Reads all supported tags from file
int ReadTags ( FileInfo* Info )
{
    FILE*   fp;
    long    TagOffs;

    FreeTagFields ( Info );

    if ( (fp = fopen ( Info->Filename, "rb" )) == NULL )
        return 1;

    if ( fseek ( fp, 0L, SEEK_END ) != 0 ) {
        fclose (fp);
        return 1;
    }
    Info->FileSize  = ftell (fp);
    Info->TagOffset = Info->FileSize;

    do {
        TagOffs = Info->TagOffset;
        ReadAPE1Tag      ( fp, Info );
        ReadAPE2Tag      ( fp, Info );
        ReadLyrics3v2Tag ( fp, Info );
        ReadID3v1Tag     ( fp, Info );
    } while ( TagOffs != Info->TagOffset );

    fclose (fp);

#ifdef      ID3V2SUPPORT
    ReadID3v2Tag ( Info->Filename, Info );
#endif

#ifdef      VORBISSUPPORT
    ReadVorbisTag ( Info->Filename, Info );
#endif

#ifdef      FLACSUPPORT
    ReadFLACTag ( Info->Filename, Info );
#endif

    return 0;
}
