#include "Tag.h"
#include "tags.h"
#include "playlist.h"
#include "misc.h"

const char* m3u_id     = "#EXTM3U"; // M3U identifier
const char* m3u_info   = "#EXTINF"; // Extended info ( playtime + title )
const char* m3u_artist = "#EXTART"; // AlbumList extension ( artist )
const char* m3u_album  = "#EXTALB"; // AlbumList extension ( album  )

// Adds necessary information to DetailedList structure
int AddToDetailedList ( const FileInfo* File, const FileListEntry* FileListEntry, DetailedList* Details )
{
    const char* variousar = "Various Artists";
    const char* variousal = "Various Albums";
    const char* separator = " - ";

    const char* title;
    const char* artist;
    const char* album;
    const char* comment;
    const char* artista;
    const char* albuma;
    int new_artist  = 0;
    int new_album   = 0;
    size_t i;

    title   = TagValue ( APE_TAG_FIELD_TITLE  , File );
    artist  = TagValue ( APE_TAG_FIELD_ARTIST , File );
    album   = TagValue ( APE_TAG_FIELD_ALBUM  , File );
    comment = TagValue ( APE_TAG_FIELD_COMMENT, File );
    artista = artist;
    albuma  = album;

    if ( Options._Values.AlbumArtist == NULL ) {
        if ( stricmp ( Details->Artist, variousar ) != 0 ) {    // check if various artists
            if ( Details->Artist[0] != '\0' && artist != NULL ) {
                if ( stricmp ( Details->Artist, artist ) != 0 ) {   // different artist -> various artists
                    new_artist = 1;
                    artista = variousar;
                }
            } else 
            if ( artist != NULL ) {
                new_artist = 1;
            }
        }
    } else {
        artista = Options._Values.AlbumArtist;
        new_artist = 1;
    }
    if ( Options._Values.AlbumTitle == NULL ) {
        if ( stricmp ( Details->Album, variousal ) != 0 ) {     // check if various albums
            if ( Details->Album[0] != '\0' && album != NULL ) {
                if ( stricmp ( Details->Album, album ) != 0 ) {     // different album -> various albums
                    new_album = 1;
                    albuma = variousal;
                }
            } else 
            if ( album != NULL ) {
                new_album = 1;
            }
        }
    } else {
        albuma = Options._Values.AlbumTitle;
        new_album = 1;
    }

    if ( new_artist ) {
        strncpy ( Details->Artist, artista, sizeof (Details->Artist)-1 );
        Details->Artist[sizeof (Details->Artist)-1] = '\0';
    }
    if ( new_album ) {
        strncpy ( Details->Album,  albuma,  sizeof (Details->Album )-1 );
        Details->Album [sizeof (Details->Album) -1] = '\0';
    }

    if ( title != NULL ) {
        if ( (Details->Files[Details->FileCount].Title = malloc ( strlen (title) + 1 )) == NULL ) {
            fprintf ( Options.output, "AddToDetailedList: Memory allocation failed.\n" );
            exit (1);
        }
        strcpy ( Details->Files[Details->FileCount].Title, title );
    } else {
        Details->Files[Details->FileCount].Title = NULL;
    }
    if ( artist != NULL ) {
        if ( (Details->Files[Details->FileCount].Artist = malloc ( strlen (artist) + 1 )) == NULL ) {
            fprintf ( Options.output, "AddToDetailedList: Memory allocation failed.\n" );
            exit (1);
        }
        strcpy ( Details->Files[Details->FileCount].Artist, artist );
    } else {
        Details->Files[Details->FileCount].Artist = NULL;
    }
    if ( album != NULL ) {
        if ( (Details->Files[Details->FileCount].Album = malloc ( strlen (album) + 1 )) == NULL ) {
            fprintf ( Options.output, "AddToDetailedList: Memory allocation failed.\n" );
            exit (1);
        }
        strcpy ( Details->Files[Details->FileCount].Album, album );
    } else {
        Details->Files[Details->FileCount].Album = NULL;
    }
    if ( comment != NULL ) {
        if ( (Details->Files[Details->FileCount].Comment = malloc ( strlen (comment) + 1 )) == NULL ) {
            fprintf ( Options.output, "AddToDetailedList: Memory allocation failed.\n" );
            exit (1);
        }
        strcpy ( Details->Files[Details->FileCount].Comment, comment );
    } else {
        Details->Files[Details->FileCount].Comment = NULL;
    }

    // Playlist display format: Artist - Title
    if ( artist != NULL && title != NULL && artist[0] && title[0] ) {
        if ( (Details->Files[Details->FileCount].Display = malloc ( strlen (artist) + strlen (separator) + strlen (title) + 1 )) == NULL ) {
            fprintf ( Options.output, "AddToDetailedList: Memory allocation failed.\n" );
            exit (1);
        }
        strcpy ( Details->Files[Details->FileCount].Display, artist    );
        strcat ( Details->Files[Details->FileCount].Display, separator );
        strcat ( Details->Files[Details->FileCount].Display, title     );
    } else {
        Details->Files[Details->FileCount].Display = NULL;
    }

    Details->Files[Details->FileCount].Info     = FileListEntry;
    Details->Files[Details->FileCount].Duration = File->Details.Duration;

    if ( Details->SortValueCount > 0 ) {
        if ( (Details->Files[Details->FileCount].SortValues = (char **)malloc ( sizeof (*Details->Files->SortValues) * Details->SortValueCount )) == NULL ) {
            fprintf ( Options.output, "main: Memory allocation failed.\n" );
            exit (1);
        }

        for ( i = 0; i < Options._Values.SortCount; i++ ) {
            char        temp[_MAX_PATH];
            const char* p = NULL;

            if ( strcmp ( Options._Values.SortBy[i], "duration" ) == 0 ) {
                sprintf ( temp, "%u", File->Details.Duration );
                p = temp;
            } else
            if ( strcmp ( Options._Values.SortBy[i], "size" ) == 0 ) {
                sprintf ( temp, "%u", File->FileSize );
                p = temp;
            } else
            if ( strcmp ( Options._Values.SortBy[i], "name" ) == 0 ) {
                p = File->Filename;
            } else
            if ( strcmp ( Options._Values.SortBy[i], "path" ) == 0 ) {
                p = FileListEntry->Path;
            } else
            if ( strcmp ( Options._Values.SortBy[i], "basename" ) == 0 ) {
                p = FileListEntry->Info.name;
            } else
            if ( strcmp ( Options._Values.SortBy[i], "extension" ) == 0 ) {
                _splitpath ( File->Filename, NULL, NULL, NULL, temp );
                p = temp;
            } else
            if ( strcmp ( Options._Values.SortBy[i], "attrib" ) == 0 ) {
                sprintf ( temp, "%u", FileListEntry->Info.attrib );
                p = temp;
            } else
            if ( strcmp ( Options._Values.SortBy[i], "time_access" ) == 0 ) {
                sprintf ( temp, "%u", FileListEntry->Info.time_access );
                p = temp;
            } else
            if ( strcmp ( Options._Values.SortBy[i], "time_create" ) == 0 ) {
                sprintf ( temp, "%u", FileListEntry->Info.time_create );
                p = temp;
            } else
            if ( strcmp ( Options._Values.SortBy[i], "time_write" ) == 0 ||
                 strcmp ( Options._Values.SortBy[i], "time"       ) == 0 ||
                 strcmp ( Options._Values.SortBy[i], "date"       ) == 0 ) {
                sprintf ( temp, "%u", FileListEntry->Info.time_write );
                p = temp;
            } else
            if ( strcmp ( Options._Values.SortBy[i], "bitrate" ) == 0 ) {
                sprintf ( temp, "%u", File->Details.BitRate );
                p = temp;
            } else
            if ( strcmp ( Options._Values.SortBy[i], "channels" ) == 0 ) {
                sprintf ( temp, "%u", File->Details.Channels );
                p = temp;
            } else
            if ( strcmp ( Options._Values.SortBy[i], "samplerate" ) == 0 ) {
                sprintf ( temp, "%u", File->Details.SampleRate );
                p = temp;
            } else
            if ( strcmp ( Options._Values.SortBy[i], "format" ) == 0 ) {
                p = File->Details.Format;
            } else {
                p = TagValue ( Options._Values.SortBy[i], File );
            }

            if ( p != NULL ) {
                if ( (Details->Files[Details->FileCount].SortValues[i] = malloc ( strlen (p) + 1 + 1 )) == NULL ) {
                    fprintf ( Options.output, "main: Memory allocation failed.\n" );
                    exit (1);
                }
                strcpy ( Details->Files[Details->FileCount].SortValues[i], p );
                strcat ( Details->Files[Details->FileCount].SortValues[i], "_" );
            } else {
                if ( (Details->Files[Details->FileCount].SortValues[i] = malloc ( 1 + 1 )) == NULL ) {
                    fprintf ( Options.output, "main: Memory allocation failed.\n" );
                    exit (1);
                }
                strcpy ( Details->Files[Details->FileCount].SortValues[i], "_" );
            }
        }
    }
    Details->FileCount++;

    return 0;
}

// Get relative path
void _get_relative_path ( const char* current, const char* path, char* buffer )
{
    char drive1 [_MAX_DRIVE];
    char drive2 [_MAX_DRIVE];
    char dir1   [_MAX_DIR];
    char dir2   [_MAX_DIR];

    if ( current == NULL || path == NULL ) {
        buffer[0] = '\0';
        return;
    }

    _splitpath ( current, drive1, dir1, NULL, NULL );
    _splitpath ( path   , drive2, dir2, NULL, NULL );

    if ( stricmp ( drive1, drive2 ) != 0 ) {                // different drives
        strcpy ( buffer, path );
        if ( buffer[0] != '\0' && buffer[strlen (buffer)-1] != '/' && buffer[strlen (buffer)-1] != '\\' ) {
            strcat ( buffer, "\\" );
        }
        return;
    }

    if ( dir1[0] != '\0' && dir1[strlen (dir1)-1] != '/' && dir1[strlen (dir1)-1] != '\\' ) {
        strcat ( dir1, "\\" );
    }
    if ( dir2[0] != '\0' && dir2[strlen (dir2)-1] != '/' && dir2[strlen (dir2)-1] != '\\' ) {
        strcat ( dir2, "\\" );
    }

    if ( stricmp ( dir1, dir2 ) == 0 ) {                    // no difference
        buffer[0] = '\0';
        return;
    }

    if ( strlen (dir2) > strlen (dir1) ) {                  // subdirectory
        if ( strnicmp ( dir1, dir2, strlen (dir1) ) == 0 ) {
            strcpy ( buffer, dir2 + strlen (dir1) );
            return;
        }
    }

    strcpy ( buffer, path + 2 );
    if ( buffer[0] != '\0' && buffer[strlen (buffer)-1] != '/' && buffer[strlen (buffer)-1] != '\\' ) {
        strcat ( buffer, "\\" );
    }
}

// Write multiple playlists, one per directory
int WritePlaylistPerDir (  const char* playlistname, const DetailedList* Details )
{
    char   cwdpath [_MAX_PATH];
    char** dirs;
    size_t dcount = 0;
    size_t i, j;
    FILE*  fp;

    if ( (dirs = (char **)malloc ( sizeof (char *) * Details->FileCount )) == NULL ) {
        fprintf ( Options.output, "WritePlaylistPerDir: Memory allocation failed.\n" );
        exit (1);
    }

    _getcwd_long ( cwdpath, _MAX_PATH );
    if ( cwdpath[0] != '\0' && cwdpath[strlen (cwdpath)-1] != '/' && cwdpath[strlen (cwdpath)-1] != '\\' ) {
        strcat ( cwdpath, "\\" );
    }

    for ( i = 0; i < Details->FileCount; i++ ) {     // find all directories
        int new_dir = 1;
        for ( j = 0; j < dcount; j++ ) {
            if ( stricmp ( Details->Files[i].Info->Path, dirs[j] ) == 0 ) {
                new_dir = 0;
                break;
            }
        }

        if ( new_dir ) {
            if ( (dirs[dcount] = malloc ( _MAX_DIR )) == NULL ) {
                fprintf ( Options.output, "WritePlaylistPerDir: Memory allocation failed.\n" );
                exit (1);
            }
            strcpy ( dirs[dcount], Details->Files[i].Info->Path );
            dcount++;
        }
    }

    for ( i = 0; i < dcount; i++ ) {
        char title [1024];
        char artist[1024];
        char album [1024];
        char name[_MAX_PATH];
        char fullname[_MAX_PATH];
        int various_artists = 0;

        title [0] = '\0';
        artist[0] = '\0';
        album [0] = '\0';

        for ( j = 0; j < Details->FileCount; j++ ) {     // find artist
            if ( stricmp ( dirs[i], Details->Files[j].Info->Path ) == 0 ) {
                if ( title[0] == '\0' && Details->Files[j].Title != NULL ) {
                    strncpy ( title, Details->Files[j].Title, sizeof (title) );
                    title[sizeof (title)-1] = '\0';
                }
                if ( album[0] == '\0' && Details->Files[j].Album != NULL ) {
                    strncpy ( album, Details->Files[j].Album, sizeof (album) );
                    album[sizeof (album)-1] = '\0';
                }
                if ( !various_artists ) {
                    if ( artist[0] == '\0' && Details->Files[j].Artist != NULL ) {
                        strncpy ( artist, Details->Files[j].Artist, sizeof (artist) );
                        artist[sizeof (artist)-1] = '\0';
                    } else
                    if ( artist[0] != '\0' && Details->Files[j].Artist != NULL && stricmp ( artist, Details->Files[j].Artist ) != 0 ) {
                        various_artists = 1;
                    }
                } else {
                    if ( title[0] != '\0' && album[0] != '\0' ) {
                        break;
                    }
                }
            }
        }

        if ( various_artists ) {
            strcpy ( artist, "Various Artists" );
        }

        if ( Options._Values.AlbumArtist != NULL ) {
            strncpy ( artist, Options._Values.AlbumArtist, sizeof (artist)-1 );
            artist[sizeof (artist)-1] = '\0';
        }
        if ( Options._Values.AlbumTitle != NULL ) {
            strncpy ( album, Options._Values.AlbumTitle, sizeof (album)-1 );
            album[sizeof (album)-1] = '\0';
        }

        if ( playlistname == NULL ) {
            if ( Options.PlaylistNameFromDir ||
                 (artist[0] == '\0' || album[0] == '\0') ||
                 ((strlen (artist) + strlen (" - ") + strlen (album) + 1) > _MAX_PATH) ) {
                _get_path ( dirs[i], name );
            } else {
                if ( !various_artists ) {
                    sprintf ( name, "%s - %s", artist, album );
                } else {
                    sprintf ( name, "%s", album );
                }
            }
        } else {
            strcpy ( name, playlistname );
        }

        ReplaceCharacters ( name, &Replace );
        remove_unsupported_chars ( name );
        if ( Options.ReplaceSpaces ) {
            replace_spaces ( name, '_' );
        }
        strcat ( name, ".m3u" );
        sprintf ( fullname, "%s%s", dirs[i], name );

        if ( !Options.TestMode ) {
            if ( (fp = fopen ( fullname, "wt" )) == NULL ) {
                fprintf ( Options.output, "Failed to open playlist '%s' for writing.\n", fullname );
                for ( i = 0; i < dcount; i++ ) {
                    free ( dirs[i] );
                }

                free ( dirs );
                return 1;
            }
        } else {
            fp = stdout;
            fprintf ( Options.output, "Playlist: %s\n", fullname );
        }

        if ( !Options.PlaylistNoExt ) {
            fprintf ( fp, "%s\n", m3u_id );
            if ( artist[0] != '\0' && album[0] != '\0' ) {
                fprintf ( fp, "%s:%s\n", m3u_artist, artist );
                fprintf ( fp, "%s:%s\n", m3u_album,  album  );
            }
        }

        for ( j = 0; j < Details->FileCount; j++ ) {
            if ( stricmp ( dirs[i], Details->Files[j].Info->Path ) == 0 ) {
                if ( !Options.PlaylistNoExt ) {
                    if ( Details->Files[j].Duration > 0 ) {
                        fprintf ( fp, "%s:%u,", m3u_info, Details->Files[j].Duration );

                        if ( Details->Files[j].Display != NULL ) {
                            fprintf ( fp, "%s\n", Details->Files[j].Display );
                        } else {
                            char fname[_MAX_PATH];
                            _splitpath ( Details->Files[j].Info->Info.name, NULL, NULL, fname, NULL );
                            fprintf ( fp, "%s\n", fname );
                        }
                    }
                }
                fprintf ( fp, "%s\n", Details->Files[j].Info->Info.name );
            }
        }

        if ( !Options.TestMode ) {
            fclose (fp);
            if ( stricmp ( cwdpath, dirs[i] ) == 0 ) {
                fprintf ( Options.output, "Playlist '%s' written.\n\n", name );        // playlist in current directory
            } else {
                fprintf ( Options.output, "Playlist '%s' written.\n\n", fullname );    // playlist in other directory
            }
        } else {
            printf ( "\n" );
        }
    }

    for ( i = 0; i < dcount; i++ ) {
        free ( dirs[i] );
    }

    free ( dirs );

    return 0;
}

// Write one big playlist of all files
int WriteOnePlaylist ( const char* playlistname, const DetailedList* Details )
{
    char    temp[_MAX_PATH];
    char    name[_MAX_PATH];
    char    cwdpath[_MAX_PATH];
    char    relative[_MAX_PATH];
    size_t  i;
    FILE*   fp;

    if ( playlistname == NULL ) {
        if ( Options.PlaylistNameFromDir ||
             (Details->Artist[0] == '\0' || Details->Album[0] == '\0') ||
             ((strlen (Details->Artist) + strlen (" - ") + strlen (Details->Album) + 1) > _MAX_PATH) ) {
            _getcwd_long ( temp, _MAX_PATH );
            _get_path ( temp, name );
        } else {
            if ( stricmp ( Details->Artist, "Various Artists" ) != 0 ) {
                sprintf ( name, "%s - %s", Details->Artist, Details->Album );
            } else {
                sprintf ( name, "%s", Details->Album );
            }
        }
    } else {
        strcpy ( name, playlistname );
    }

    ReplaceCharacters ( name, &Replace );
    remove_unsupported_chars ( name );
    if ( Options.ReplaceSpaces ) {
        replace_spaces ( name, '_' );
    }
    strcat ( name, ".m3u" );

    if ( !Options.TestMode ) {
        if ( (fp = fopen ( name, "wt" )) == NULL ) {
            fprintf ( Options.output, "Failed to open playlist '%s' for writing.\n", name );
            return 1;
        }
    } else {
        fp = stdout;
        fprintf ( Options.output, "Playlist: %s\n", name );
    }

    _getcwd_long ( cwdpath, _MAX_PATH );
    if ( cwdpath[0] != '\0' && cwdpath[strlen (cwdpath)-1] != '/' && cwdpath[strlen (cwdpath)-1] != '\\' ) {
        strcat ( cwdpath, "\\" );
    }

    if ( !Options.PlaylistNoExt ) {
        fprintf ( fp, "%s\n", m3u_id );
        if ( Details->Artist[0] != '\0' && Details->Album[0] != '\0' ) {
            fprintf ( fp, "%s:%s\n", m3u_artist, Details->Artist );
            fprintf ( fp, "%s:%s\n", m3u_album,  Details->Album  );
        }
    }
    for ( i = 0; i < Details->FileCount; i++ ) {
        if ( !Options.PlaylistNoExt ) {
            if ( Details->Files[i].Duration > 0 ) {
                fprintf ( fp, "%s:%u,", m3u_info, Details->Files[i].Duration );

                if ( Details->Files[i].Display != NULL ) {
                    fprintf ( fp, "%s\n", Details->Files[i].Display );
                } else {
                    char fname[_MAX_PATH];
                    _splitpath ( Details->Files[i].Info->Info.name, NULL, NULL, fname, NULL );
                    fprintf ( fp, "%s\n", fname );
                }
            }
        }
        _get_relative_path ( cwdpath, Details->Files[i].Info->Path, relative );
        if ( Options.PlaylistUnixSlashes ) {
            replace_characters ( relative, '\\', '/' );
        }
        fprintf ( fp, "%s%s\n", relative, Details->Files[i].Info->Info.name );
    }

    if ( !Options.TestMode ) {
        fclose (fp);
        fprintf ( Options.output, "Playlist '%s' written.\n\n", name );
    }

    return 0;
}

// Write one playlist per every album to current working directory
int WritePlaylistPerAlbum ( const char* playlistname, const DetailedList* Details )
{
    char    cwdpath [_MAX_PATH];
    char    relative[_MAX_PATH];
    char**  dirs;
    size_t  dcount = 0;
    size_t  i, j;
    FILE*   fp;

    if ( (dirs = (char **)malloc ( sizeof (char *) * Details->FileCount )) == NULL ) {
        fprintf ( Options.output, "WritePlaylistPerAlbum: Memory allocation failed.\n" );
        exit (1);
    }

    _getcwd_long ( cwdpath, _MAX_PATH );
    if ( cwdpath[0] != '\0' && cwdpath[strlen (cwdpath)-1] != '/' && cwdpath[strlen (cwdpath)-1] != '\\' ) {
        strcat ( cwdpath, "\\" );
    }

    for ( i = 0; i < Details->FileCount; i++ ) {     // find all directories
        int new_dir = 1;
        for ( j = 0; j < dcount; j++ ) {
            if ( stricmp ( Details->Files[i].Info->Path, dirs[j] ) == 0 ) {
                new_dir = 0;
                break;
            }
        }

        if ( new_dir ) {
            if ( (dirs[dcount] = malloc ( _MAX_DIR )) == NULL ) {
                fprintf ( Options.output, "WritePlaylistPerAlbum: Memory allocation failed.\n" );
                exit (1);
            }
            strcpy ( dirs[dcount], Details->Files[i].Info->Path );
            dcount++;
        }
    }

    for ( i = 0; i < dcount; i++ ) {
        char title [1024];
        char artist[1024];
        char album [1024];
        char name[_MAX_PATH];
        char fullname[_MAX_PATH];
        int various_artists = 0;

        title [0] = '\0';
        artist[0] = '\0';
        album [0] = '\0';

        for ( j = 0; j < Details->FileCount; j++ ) {     // find artist
            if ( stricmp ( dirs[i], Details->Files[j].Info->Path ) == 0 ) {
                if ( title[0] == '\0' && Details->Files[j].Title != NULL ) {
                    strncpy ( title, Details->Files[j].Title, sizeof (title) );
                    title[sizeof (title)-1] = '\0';
                }
                if ( album[0] == '\0' && Details->Files[j].Album != NULL ) {
                    strncpy ( album, Details->Files[j].Album, sizeof (album) );
                    album[sizeof (album)-1] = '\0';
                }
                if ( !various_artists ) {
                    if ( artist[0] == '\0' && Details->Files[j].Artist != NULL ) {
                        strncpy ( artist, Details->Files[j].Artist, sizeof (artist) );
                        artist[sizeof (artist)-1] = '\0';
                    } else
                    if ( artist[0] != '\0' && Details->Files[j].Artist != NULL && stricmp ( artist, Details->Files[j].Artist ) != 0 ) {
                        various_artists = 1;
                    }
                } else {
                    if ( title[0] != '\0' && album[0] != '\0' ) {
                        break;
                    }
                }
            }
        }

        if ( various_artists ) {
            strcpy ( artist, "Various Artists" );
        }

        if ( Options._Values.AlbumArtist != NULL ) {
            strncpy ( artist, Options._Values.AlbumArtist, sizeof (artist)-1 );
            artist[sizeof (artist)-1] = '\0';
        }
        if ( Options._Values.AlbumTitle != NULL ) {
            strncpy ( album, Options._Values.AlbumTitle, sizeof (album)-1 );
            album[sizeof (album)-1] = '\0';
        }

        if ( playlistname == NULL ) {
            if ( Options.PlaylistNameFromDir ||
                 (artist[0] == '\0' || album[0] == '\0') ||
                 ((strlen (artist) + strlen (" - ") + strlen (album) + 1) > _MAX_PATH) ) {
                _get_path ( dirs[i], name );
            } else {
                if ( !various_artists ) {
                    sprintf ( name, "%s - %s", artist, album );
                } else {
                    sprintf ( name, "%s", album );
                }
            }
        } else {
            strcpy ( name, playlistname );
        }

        ReplaceCharacters ( name, &Replace );
        remove_unsupported_chars ( name );
        if ( Options.ReplaceSpaces ) {
            replace_spaces ( name, '_' );
        }
        strcat ( name, ".m3u" );
        // sprintf ( fullname, "%s%s", dirs[i], name );
        strcpy ( fullname, name );

        if ( !Options.TestMode ) {
            if ( (fp = fopen ( fullname, "wt" )) == NULL ) {
                fprintf ( Options.output, "Failed to open playlist '%s' for writing.\n", fullname );
                for ( i = 0; i < dcount; i++ ) {
                    free ( dirs[i] );
                }

                free ( dirs );
                return 1;
            }
        } else {
            fp = stdout;
            fprintf ( Options.output, "Playlist: %s\n", fullname );
        }

        if ( !Options.PlaylistNoExt ) {
            fprintf ( fp, "%s\n", m3u_id );
            if ( artist[0] != '\0' && album[0] != '\0' ) {
                fprintf ( fp, "%s:%s\n", m3u_artist, artist );
                fprintf ( fp, "%s:%s\n", m3u_album,  album  );
            }
        }

        for ( j = 0; j < Details->FileCount; j++ ) {
            if ( stricmp ( dirs[i], Details->Files[j].Info->Path ) == 0 ) {
                if ( !Options.PlaylistNoExt ) {
                    if ( Details->Files[i].Duration > 0 ) {
                        fprintf ( fp, "%s:%u,", m3u_info, Details->Files[j].Duration );

                        if ( Details->Files[j].Display != NULL ) {
                            fprintf ( fp, "%s\n", Details->Files[j].Display );
                        } else {
                            char fname[_MAX_PATH];
                            _splitpath ( Details->Files[j].Info->Info.name, NULL, NULL, fname, NULL );
                            fprintf ( fp, "%s\n", fname );
                        }
                    }
                }
                _get_relative_path ( cwdpath, Details->Files[j].Info->Path, relative );
                if ( Options.PlaylistUnixSlashes ) {
                    replace_characters ( relative, '\\', '/' );
                }
                fprintf ( fp, "%s%s\n", relative, Details->Files[j].Info->Info.name );
            } // end of new
        }

        if ( !Options.TestMode ) {
            fclose (fp);
            fprintf ( Options.output, "Playlist '%s' written.\n\n", fullname );
        } else {
            printf ( "\n" );
        }
    }

    for ( i = 0; i < dcount; i++ ) {
        free ( dirs[i] );
    }

    free ( dirs );

    return 0;
}
