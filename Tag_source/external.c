#include "external.h"

// Free memory allocated by ExternalAlbum
void FreeExternalAlbum ( ExternalAlbum* Album )
{
    size_t  i;

    for ( i = 0; i < Album->TrackCount; i++ ) {
        free ( Album->Tracks[i].Title   );
        free ( Album->Tracks[i].Artist  );
        free ( Album->Tracks[i].Comment );
        free ( Album->Tracks[i].Genre   );
        free ( Album->Tracks[i].Track   );
        free ( Album->Tracks[i].Year    );
    }

    free ( Album->Tracks  );
    free ( Album->Album   );
    free ( Album->Artist  );
    free ( Album->Comment );
    free ( Album->Genre   );
    free ( Album->Year    );

    Album->Tracks     = NULL;
    Album->Album      = NULL;
    Album->Artist     = NULL;
    Album->Comment    = NULL;
    Album->Genre      = NULL;
    Album->Year       = NULL;
    Album->TrackCount = 0;
}

// Load tag information from CDDB-file
int LoadCDDBFile ( const char* filename, ExternalAlbum* Album )
{
    FILE*   fp;
    int     is_our_file     = 0;
    int     various_artists = 0;
    char    item[4096];
    char    temp[4096];
    char    tmp [4096];
    char*   p;
    char*   s;
    char*   d;
    int     num;
    size_t  i;

    FreeExternalAlbum ( Album );

    if ( (fp = fopen ( filename, "rb" )) == NULL ) {
        return -1;
    }

	while ( fgets_clean ( temp, sizeof (temp), fp ) != EOF ) {
        if ( temp[0] == '\0' || temp[0] == '#' )        // skip comments and empty lines
            continue;

        p = temp;                       // copy part before '=' to item
        d = item;
        while ( *p && *p != '=' ) {
            *d++ = *p++;
        }
        *d = '\0';
        while ( d >= item && (*d == ' ' || *d == '\0') ) *d-- = '\0';
        if ( *p == '=' ) p++;           // skip '=' character
        while ( *p && *p == ' ' ) p++;  // skip space

	    if ( 0 == stricmp ( item, "DISCID" ) ) {        // CDDB disc identifier
            is_our_file = 1;
            free ( Album->Comment );
            if ( (Album->Comment = malloc ( strlen (p) + 1 )) == NULL ) {
                fprintf ( Options.output, "LoadCDDBFile: Memory allocation failed.\n" );
                exit (1);
            }
            strcpy ( Album->Comment, p );
        } else
	    if ( 0 == stricmp ( item, "DTITLE" ) ) {        // Album artist / Album title
            is_our_file = 1;
            s = p;
            d = tmp;
            while ( *s ) {
                if ( *s == ' ' && *(s+1) == '/' && *(s+2) == ' ' ) break;
                *d++ = *s++;
            }
            *d = '\0';
            while ( d >= tmp && (*d == ' ' || *d == '\0' ) ) *d-- = '\0';
            if ( *s == ' ' && *(s+1) == '/' && *(s+2) == ' ' ) s += 3;
            while ( *s && *s == ' ' ) s++;

            free ( Album->Artist );
            if ( (Album->Artist = malloc ( strlen (tmp) + 1 )) == NULL ) {
                fprintf ( Options.output, "LoadCDDBFile: Memory allocation failed.\n" );
                exit (1);
            }
            strcpy ( Album->Artist, tmp );

            free ( Album->Album );
            if ( (Album->Album = malloc ( strlen (s) + 1 )) == NULL ) {
                fprintf ( Options.output, "LoadCDDBFile: Memory allocation failed.\n" );
                exit (1);
            }
            strcpy ( Album->Album, s );

            if ( stricmp ( tmp, "Various Artists" ) == 0 ) {
                various_artists = 1;
            }
        } else
	    if ( 0 == stricmp ( item, "DYEAR" ) ) {         // Album year
            is_our_file = 1;
            free ( Album->Year );
            if ( (Album->Year = malloc ( strlen (p) + 1 )) == NULL ) {
                fprintf ( Options.output, "LoadCDDBFile: Memory allocation failed.\n" );
                exit (1);
            }
            strcpy ( Album->Year, p );
        } else
	    if ( 0 == stricmp ( item, "DGENRE" ) ) {        // Album genre
            is_our_file = 1;
            free ( Album->Genre );
            if ( (Album->Genre = malloc ( strlen (p) + 1 )) == NULL ) {
                fprintf ( Options.output, "LoadCDDBFile: Memory allocation failed.\n" );
                exit (1);
            }
            strcpy ( Album->Genre, p );
        } else
	    if ( 0 == strnicmp ( item, "TTITLE", 6 ) ) {    // Track title (and artist when various artists)
            is_our_file = 1;
            d = (char *)(item + strlen (item)-1);
            while ( d >= item && (*d >= '0' && *d <= '9') ) d--;
            if ( *d < '0' || *d > '9' ) d++;
            num = atoi (d);

            if ( num < 0 ) continue;

            if ( various_artists ) {    // Track artist / Track title
                int only_title = 1;

                s = p;
                d = tmp;
                while ( *s ) {
                    if ( *s == ' ' && *(s+1) == '/' && *(s+2) == ' ' ) {
                        only_title = 0;
                        break;
                    }
                    if ( *s == ' ' && *(s+1) == '-' && *(s+2) == ' ' ) {
                        only_title = 0;
                        break;
                    }
                    *d++ = *s++;
                }
                *d = '\0';
                while ( d >= tmp && (*d == ' ' || *d == '\0' ) ) *d-- = '\0';
                if ( !only_title ) {
                    if ( *s == ' ' && *(s+1) == '/' && *(s+2) == ' ' ) {
                        s += 3;
                    } else
                    if ( *s == ' ' && *(s+1) == '-' && *(s+2) == ' ' ) {
                        s += 3;
                    }
                    while ( *s && *s == ' ' ) s++;
                }

                if ( (size_t)num + 1 > Album->TrackCount ) {    // need to allocate more tracks
                    if ( (Album->Tracks = (ExternalTrack *)realloc ( Album->Tracks, sizeof (*Album->Tracks) * (num + 1) )) == NULL ) {
                        fprintf ( Options.output, "LoadCDDBFile: Memory allocation failed.\n" );
                        exit (1);
                    }
                    for ( i = Album->TrackCount; i < (size_t)num + 1; i++ ) {
                        memset ( &Album->Tracks[i], 0, sizeof (ExternalTrack) );
                    }
                    Album->TrackCount = num + 1;
                }

                if ( !only_title ) {    // Track artist + Track title
                    free ( Album->Tracks[num].Artist );
                    if ( (Album->Tracks[num].Artist = malloc ( strlen (tmp) + 1 )) == NULL ) {
                        fprintf ( Options.output, "LoadCDDBFile: Memory allocation failed.\n" );
                        exit (1);
                    }
                    strcpy ( Album->Tracks[num].Artist, tmp );

                    free ( Album->Tracks[num].Title );
                    if ( (Album->Tracks[num].Title = malloc ( strlen (s) + 1 )) == NULL ) {
                        fprintf ( Options.output, "LoadCDDBFile: Memory allocation failed.\n" );
                        exit (1);
                    }
                    strcpy ( Album->Tracks[num].Title, s );
                } else {                // Only track title
                    free ( Album->Tracks[num].Title );
                    if ( (Album->Tracks[num].Title = malloc ( strlen (p) + 1 )) == NULL ) {
                        fprintf ( Options.output, "LoadCDDBFile: Memory allocation failed.\n" );
                        exit (1);
                    }
                    strcpy ( Album->Tracks[num].Title, p );
                }
            } else {                    // Track title
                if ( (size_t)num + 1 > Album->TrackCount ) {    // need to allocate more tracks
                    if ( (Album->Tracks = (ExternalTrack *)realloc ( Album->Tracks, sizeof (*Album->Tracks) * (num + 1) )) == NULL ) {
                        fprintf ( Options.output, "LoadCDDBFile: Memory allocation failed.\n" );
                        exit (1);
                    }
                    for ( i = Album->TrackCount; i < (size_t)num + 1; i++ ) {
                        memset ( &Album->Tracks[i], 0, sizeof (ExternalTrack) );
                    }
                    Album->TrackCount = num + 1;
                }

                free ( Album->Tracks[num].Title );
                if ( (Album->Tracks[num].Title = malloc ( strlen (p) + 1 )) == NULL ) {
                    fprintf ( Options.output, "LoadCDDBFile: Memory allocation failed.\n" );
                    exit (1);
                }
                strcpy ( Album->Tracks[num].Title, p );
            }
        } else
        if ( 0 == stricmp ( item, "EXTD" ) ) {          // Album information
            is_our_file = 1;
            for ( i = 0; i < strlen (p)-5; i++ ) {
                if ( strnicmp ( p + i, "YEAR:", 5 ) == 0 ) {
                    s = (char *)(p + i + 5);
                    while ( *s && *s == ' ' ) s++;
                    d = tmp;
                    while ( *s && *s != ' ' ) {
                        *d++ = *s++;
                    }
                    *d = '\0';

                    free ( Album->Year );
                    if ( (Album->Year = malloc ( strlen (tmp) + 1 )) == NULL ) {
                        fprintf ( Options.output, "LoadCDDBFile: Memory allocation failed.\n" );
                        exit (1);
                    }
                    strcpy ( Album->Year, tmp );
                } else
                if ( strnicmp ( p + i, "ID3G:", 5 ) == 0 ) {
                    s = (char *)(p + i + 5);
                    while ( *s && *s == ' ' ) s++;
                    GenreToString ( tmp, atoi (s) );

                    free ( Album->Genre );
                    if ( (Album->Genre = malloc ( strlen (tmp) + 1 )) == NULL ) {
                        fprintf ( Options.output, "LoadCDDBFile: Memory allocation failed.\n" );
                        exit (1);
                    }
                    strcpy ( Album->Genre, tmp );
                }
            }
        } else
        if ( 0 == strnicmp ( item, "EXTT", 4 ) ) {      // Track information
            is_our_file = 1;
            d = (char *)(item + strlen (item)-1);
            while ( d >= item && (*d >= '0' && *d <= '9') ) d--;
            if ( *d < '0' || *d > '9' ) d++;
            num = atoi (d);

            if ( num < 0 ) continue;

            if ( (size_t)num + 1 > Album->TrackCount ) {    // need to allocate more tracks
                size_t i;
                if ( (Album->Tracks = (ExternalTrack *)realloc ( Album->Tracks, sizeof (*Album->Tracks) * (num + 1) )) == NULL ) {
                    fprintf ( Options.output, "LoadCDDBFile: Memory allocation failed.\n" );
                    exit (1);
                }
                for ( i = Album->TrackCount; i < (size_t)num + 1; i++ ) {
                    memset ( &Album->Tracks[i], 0, sizeof (ExternalTrack) );
                }
                Album->TrackCount = num + 1;
            }

            free ( Album->Tracks[num].Comment );
            if ( (Album->Tracks[num].Comment = malloc ( strlen (p) + 1 )) == NULL ) {
                fprintf ( Options.output, "LoadCDDBFile: Memory allocation failed.\n" );
                exit (1);
            }
            strcpy ( Album->Tracks[num].Comment, p );
        } else
        if ( 0 == stricmp ( item, "PLAYORDER" ) ) {     // Playing order of tracks
            is_our_file = 1;
        }

        if ( !is_our_file ) {
            fclose (fp);
            return 1;
        }
    }

    fclose (fp);

    if ( !is_our_file ) return 1;

    if ( various_artists ) {    // fix artist fields on various artists album
        size_t  i;
        for ( i = 0; i < Album->TrackCount; i++ ) {
            // artist field empty -> get artist from comment
            if ( Album->Tracks[i].Artist == NULL || Album->Tracks[i].Artist[0] == '\0' ) {
                if ( Album->Tracks[i].Comment != NULL ) {
                    free ( Album->Tracks[i].Artist );
                    if ( (Album->Tracks[i].Artist = malloc ( strlen (Album->Tracks[i].Comment) + 1 )) == NULL ) {
                        fprintf ( Options.output, "LoadCDDBFile: Memory allocation failed.\n" );
                        exit (1);
                    }
                    strcpy ( Album->Tracks[i].Artist, Album->Tracks[i].Comment );
                    free ( Album->Tracks[i].Comment );
                    Album->Tracks[i].Comment = NULL;
                }
            } else
            // artist and comment are the same -> clear comment
            if ( Album->Tracks[i].Artist && Album->Tracks[i].Comment && strcmp ( Album->Tracks[i].Artist, Album->Tracks[i].Comment ) == 0 ) {
                free ( Album->Tracks[i].Comment );
                Album->Tracks[i].Comment = NULL;
            }
        }
    } else                      // on normal album copy album artist to every track
    if ( Album->Artist != NULL ) {
        size_t  i;
        for ( i = 0; i < Album->TrackCount; i++ ) {
            free ( Album->Tracks[i].Artist );
            if ( (Album->Tracks[i].Artist = malloc ( strlen (Album->Artist) + 1 )) == NULL ) {
                fprintf ( Options.output, "LoadCDDBFile: Memory allocation failed.\n" );
                exit (1);
            }
            strcpy ( Album->Tracks[i].Artist, Album->Artist );
        }
    }

    return 0;
}

// Load tag information from file "Tag.ini"
int LoadTagIni ( const char* filename, ExternalAlbum* Album )
{
    enum mode_t {
        Unspecified = 0,
        Global      = 1,
        Files       = 2
    };
    FILE*       fp;
    int         is_our_file = 0;
    char        item[4096];
    char        temp[4096];
    char*       p;
    char*       d;
    int         num;
    enum mode_t mode = Unspecified;

    FreeExternalAlbum ( Album );

    if ( (fp = fopen ( filename, "rb" )) == NULL ) {
        return -1;
    }

    while ( fgets_clean ( temp, sizeof (temp), fp ) != EOF ) {
        if ( temp[0] == '\0' || temp[0] == ';' )        // skip empty lines and comments
            continue;

        if ( temp[0] == '[' ) {
            if ( 0 == stricmp ( temp, "[Global]" ) ) {
                is_our_file = 1;
                mode = Global;
            } else
            if ( 0 == stricmp ( temp, "[Files]"  ) ) {
                is_our_file = 1;
                mode = Files;
            }
            continue;
        }

        if ( temp[strlen (temp)-1] == '"' ) temp[strlen (temp)-1] = '\0';   // remove quotation mark from the end of the string

        p = temp;                       // copy part before '=' to item
        d = item;
        while ( *p && *p != '=' ) {
            *d++ = *p++;
        }
        *d = '\0';
        while ( d >= item && (*d == ' ' || *d == '\0') ) *d-- = '\0';
        if ( *p == '=' ) p++;           // skip '=' character
        while ( *p && *p == ' ' ) p++;  // skip space
        if ( *p == '"' ) p++;           // skip '"' character

        switch ( mode ) {
        case Global:        // album settings
            if ( 0 == stricmp ( item, "Artist" ) ) {        // Album artist
                free ( Album->Artist );
                if ( (Album->Artist = malloc ( strlen (p) + 1 )) == NULL ) {
                    fprintf ( Options.output, "LoadTagIni: Memory allocation failed.\n" );
                    exit (1);
                }
                strcpy ( Album->Artist, p );
            } else
            if ( 0 == stricmp ( item, "Album" ) ) {         // Album name
                free ( Album->Album );
                if ( (Album->Album = malloc ( strlen (p) + 1 )) == NULL ) {
                    fprintf ( Options.output, "LoadTagIni: Memory allocation failed.\n" );
                    exit (1);
                }
                strcpy ( Album->Album, p );
            } else
            if ( 0 == stricmp ( item, "Year" ) ) {          // Album year
                free ( Album->Year );
                if ( (Album->Year = malloc ( strlen (p) + 1 )) == NULL ) {
                    fprintf ( Options.output, "LoadTagIni: Memory allocation failed.\n" );
                    exit (1);
                }
                strcpy ( Album->Year, p );
            } else
            if ( 0 == stricmp ( item, "Music_style" ) ) {   // Album genre
                free ( Album->Genre );
                if ( *p >= '0' && *p <= '9' ) {     // numerical genre is converted to real name
                    char genre [1024];
                    GenreToString ( genre, atoi (p) );
                    if ( (Album->Genre = malloc ( strlen (genre) + 1 )) == NULL ) {
                        fprintf ( Options.output, "LoadTagIni: Memory allocation failed.\n" );
                        exit (1);
                    }
                    strcpy ( Album->Genre, genre );
                } else {
                    if ( (Album->Genre = malloc ( strlen (p) + 1 )) == NULL ) {
                        fprintf ( Options.output, "LoadTagIni: Memory allocation failed.\n" );
                        exit (1);
                    }
                    strcpy ( Album->Genre, p );
                }
            }
            break;

        case Files:         // track settings
            d = (char *)(item + strlen (item)-1);
            while ( d >= item && (*d >= '0' && *d <= '9') ) d--;
            if ( *d < '0' || *d > '9' ) d++;
            num = atoi (d);

            if ( num < 1 ) continue;    // skip incorrect number, should always be >= 1

            if ( (size_t)num > Album->TrackCount ) {    // need to allocate more tracks
                size_t i;
                if ( (Album->Tracks = (ExternalTrack *)realloc ( Album->Tracks, sizeof (*Album->Tracks) * num )) == NULL ) {
                    fprintf ( Options.output, "LoadTagIni: Memory allocation failed.\n" );
                    exit (1);
                }
                for ( i = Album->TrackCount; i < (size_t)num; i++ ) {
                    memset ( &Album->Tracks[i], 0, sizeof (ExternalTrack) );
                }
                Album->TrackCount = num;
            }

            num--;  // Track number starts from 1 -> ease handling

            if ( 0 == strnicmp ( item, "T_TRACK", 7 ) ) {       // Track title
                free ( Album->Tracks[num].Title );
                if ( (Album->Tracks[num].Title = malloc ( strlen (p) + 1 )) == NULL ) {
                    fprintf ( Options.output, "LoadTagIni: Memory allocation failed.\n" );
                    exit (1);
                }
                strcpy ( Album->Tracks[num].Title, p );
            } else
            if ( 0 == strnicmp ( item, "A_TRACK", 7 ) ) {       // Track artist
                free ( Album->Tracks[num].Artist );
                if ( (Album->Tracks[num].Artist = malloc ( strlen (p) + 1 )) == NULL ) {
                    fprintf ( Options.output, "LoadTagIni: Memory allocation failed.\n" );
                    exit (1);
                }
                strcpy ( Album->Tracks[num].Artist, p );
            } else
            if ( 0 == strnicmp ( item, "C_TRACK", 7 ) ) {       // Track comment
                free ( Album->Tracks[num].Comment );
                if ( (Album->Tracks[num].Comment = malloc ( strlen (p) + 1 )) == NULL ) {
                    fprintf ( Options.output, "LoadTagIni: Memory allocation failed.\n" );
                    exit (1);
                }
                strcpy ( Album->Tracks[num].Comment, p );
            } else {
                continue;
            }
	        break;

        default:
        case Unspecified:
            break;
        }

        if ( !is_our_file ) {
            fclose (fp);
            return 1;
        }
    }

    fclose (fp);

    if ( !is_our_file ) return 1;

    if ( Album->Artist != NULL ) {
        size_t  i;
        for ( i = 0; i < Album->TrackCount; i++ ) {
            // artist field empty -> use album artist
            if ( Album->Tracks[i].Artist == NULL || Album->Tracks[i].Artist[0] == '\0' ) {
                free ( Album->Tracks[i].Artist );
                if ( (Album->Tracks[i].Artist = malloc ( strlen (Album->Artist) + 1 )) == NULL ) {
                    fprintf ( Options.output, "LoadTagIni: Memory allocation failed.\n" );
                    exit (1);
                }
                strcpy ( Album->Tracks[i].Artist, Album->Artist );
            }
        }
    }

    return 0;
}
