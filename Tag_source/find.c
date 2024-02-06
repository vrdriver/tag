#include "find.h"
#include "misc.h"

int _find_loadm3u ( const char* filename, const char* path, FileList* Files )
{
    char    cwdpath [_MAX_PATH];
    char    temppath[_MAX_PATH];
    char    dir     [_MAX_DIR];
    FILE*   fp;

    _getcwd ( cwdpath, _MAX_PATH );
    _chdir ( path );

    if ( (fp = fopen ( filename, "rb" )) != NULL ) {
        char                temp [4096];
        struct _finddata_t  fd;
        long                hFile;

        while ( fgets_clean ( temp, 4096, fp ) != EOF ) {
            if ( temp[0] == '\0' || temp[0] == '#' ) continue;

            _splitpath ( temp, temppath, dir, NULL, NULL );
            strcat ( temppath, dir );

            if ( temppath[0] != '\0' ) {    // file in different directory
                _chdir ( temppath );
                _getcwd_long ( temppath, _MAX_PATH );
                if ( temppath[0] != '\0' && temppath[strlen (temppath)-1] != '/' && temppath[strlen (temppath)-1] != '\\' ) {
                    strcat ( temppath, "\\" );
                }
                _chdir ( path );
            } else {
                strcpy ( temppath, path );
            }

            if ( (hFile = _findfirst ( temp, &fd )) != -1L ) {
                if ( !(fd.attrib & _A_SUBDIR) ) {
                    if ( (Files->Files = (FileListEntry *)realloc ( Files->Files, sizeof (FileListEntry) * (++Files->FileCount) )) == NULL ) {
                        fprintf ( stderr, "_find_loadm3u: Memory allocation failed.\n" );
                        exit (1);
                    }
                    memcpy ( &Files->Files[Files->FileCount-1].Info, &fd, sizeof (fd) );
                    strcpy ( Files->Files[Files->FileCount-1].Path, temppath );
                }

                while ( _findnext ( hFile, &fd ) == 0 ) {
                    if ( !(fd.attrib & _A_SUBDIR) ) {
                        if ( (Files->Files = (FileListEntry *)realloc ( Files->Files, sizeof (FileListEntry) * (++Files->FileCount) )) == NULL ) {
                            fprintf ( stderr, "_find_loadm3u: Memory allocation failed.\n" );
                            exit (1);
                        }
                        memcpy ( &Files->Files[Files->FileCount-1].Info, &fd, sizeof (fd) );
                        strcpy ( Files->Files[Files->FileCount-1].Path, temppath );
                    }
                }

                _findclose ( hFile );
            }
        }

        fclose (fp);
    }

    _chdir ( cwdpath );

    return 0;
}

// Finds all files specified by mask recursively
int _find_recurse ( const char* mask, FileList* Files )
{
    char                drive   [_MAX_DRIVE];
    char                dir     [_MAX_DIR];
    char                fname   [_MAX_FNAME];
    char                ext     [_MAX_EXT];
    char                cwdpath [_MAX_PATH];
    char                temppath[_MAX_PATH];
    char                newmask [_MAX_PATH];
    struct _finddata_t  fd;
    long                hFile;
    int                 errors = 0;

    if ( mask == NULL )
        return 0;

    _getcwd ( cwdpath, _MAX_PATH );
    _splitpath ( mask, drive, dir, fname, ext );

    strcpy ( temppath, drive );
    strcat ( temppath, dir   );
    strcpy ( newmask , fname );
    strcat ( newmask , ext   );

    if ( temppath[0] != '\0' && _chdir  ( temppath ) != 0 ) {
        return 1;
    }
    if ( newmask[0] != '\0' && _chdir ( newmask ) == 0 ) {
        strcpy ( newmask, "*" );
    }
    if ( newmask[0] == '\0' || strcmp ( newmask, "." ) == 0 ) {
        strcpy ( newmask, "*" );
    }

    _getcwd_long ( temppath, _MAX_PATH );
    if ( temppath[0] != '\0' && temppath[strlen (temppath)-1] != '/' && temppath[strlen (temppath)-1] != '\\' )
        strcat ( temppath, "\\" );

    if ( (hFile = _findfirst ( newmask, &fd )) != -1L ) {
        if ( !(fd.attrib & _A_SUBDIR) && !(fd.attrib & _A_HIDDEN) && !(fd.attrib & _A_SYSTEM) ) {
            _splitpath ( fd.name, NULL, NULL, NULL, ext );
            if ( stricmp ( ext, ".m3u" ) == 0 ) {
                _find_loadm3u ( fd.name, temppath, Files );
            } else {
                if ( (Files->Files = (FileListEntry *)realloc ( Files->Files, sizeof (FileListEntry) * (++Files->FileCount) )) == NULL ) {
                    fprintf ( stderr, "_find_recurse: Memory allocation failed.\n" );
                    exit (1);
                }
                memcpy ( &Files->Files[Files->FileCount-1].Info, &fd, sizeof (fd) );
                strcpy ( Files->Files[Files->FileCount-1].Path, temppath );
            }
        }

        while ( _findnext ( hFile, &fd ) == 0 ) {
            if ( !(fd.attrib & _A_SUBDIR) && !(fd.attrib & _A_HIDDEN) && !(fd.attrib & _A_SYSTEM) ) {
                _splitpath ( fd.name, NULL, NULL, NULL, ext );
                if ( stricmp ( ext, ".m3u" ) == 0 ) {
                    _find_loadm3u ( fd.name, temppath, Files );
                } else {
                    if ( (Files->Files = (FileListEntry *)realloc ( Files->Files, sizeof (FileListEntry) * (++Files->FileCount) )) == NULL ) {
                        fprintf ( stderr, "_find_recurse: Memory allocation failed.\n" );
                        exit (1);
                    }
                    memcpy ( &Files->Files[Files->FileCount-1].Info, &fd, sizeof (fd) );
                    strcpy ( Files->Files[Files->FileCount-1].Path, temppath );
                }
            }
        }

        _findclose ( hFile );
    }

    if ( (hFile = _findfirst ( "*", &fd )) != -1L ) {
        if ( (fd.attrib & _A_SUBDIR) && !(fd.attrib & _A_HIDDEN) && !(fd.attrib & _A_SYSTEM) ) {
            if ( strcmp ( fd.name, "." ) != 0 && strcmp ( fd.name, ".." ) != 0 ) {
                _chdir ( fd.name  );
                errors += _find_recurse ( newmask, Files );
                _chdir ( temppath );
            }
        }

        while ( _findnext ( hFile, &fd ) == 0 ) {
            if ( (fd.attrib & _A_SUBDIR) && !(fd.attrib & _A_HIDDEN) && !(fd.attrib & _A_SYSTEM) ) {
                if ( strcmp ( fd.name, "." ) != 0 && strcmp ( fd.name, ".." ) != 0 ) {
                    _chdir ( fd.name  );
                    errors += _find_recurse ( newmask, Files );
                    _chdir ( temppath );
                }
            }
        }

        _findclose ( hFile );
    }

    _chdir ( cwdpath );

    return errors;
}

// Finds all files specified by mask
int _find ( const char* mask, FileList* Files )
{
    char                drive   [_MAX_DRIVE];
    char                dir     [_MAX_DIR];
    char                fname   [_MAX_FNAME];
    char                ext     [_MAX_EXT];
    char                cwdpath [_MAX_PATH];
    char                temppath[_MAX_PATH];
    char                path    [_MAX_PATH];
    char                dirmask [_MAX_PATH];
    char                newmask [_MAX_PATH];
    struct _finddata_t  fd;
    long                hFile;
    int                 errors      = 0;
    int                 wilddirs    = 0;
    size_t              i;

    if ( mask == NULL )
        return 0;

    _getcwd_long ( cwdpath, _MAX_PATH );
    _splitpath ( mask, drive, dir, fname, ext );

    for ( i = 0; i < strlen (dir); i++ ) {
        if ( dir[i] == '?' || dir[i] == '*' ) {
            wilddirs = 1;
            break;
        }
    }

    strcpy ( path, drive );
    if ( !wilddirs ) {
        strcat ( path, dir );
    } else if ( dir[0] != '\0' ) {
        char* p = (char *)(path + strlen (path));
        char* m;
        for ( i = 0; i < strlen (dir); i++ ) {
            if ( dir[i] != '?' && dir[i] != '*' ) {
                *p++ = dir[i];
            } else {
                p[i] = '\0';
                break;
            }
        }
        while ( p >= path && (*p != '/' && *p != '\\' && *p != ':' ) ) {
            *p-- = '\0';
        }

        m = (char *)(mask + strlen (path) );
        for ( i = 0; i < strlen (m); i++ ) {
            int ch = *m++;
            if ( ch == '/' || ch == '\\' )
                break;
        }
        if ( *m == '/' || *m == '\\' ) m++;
        strcpy ( newmask, m );
    }

    if ( path[0] != '\0' )
        _chdir ( path );

    if ( wilddirs ) {   // search for directories
        _getcwd_long ( temppath, _MAX_PATH );
        strcpy ( dirmask, (char *)(mask + strlen (path)) );
        for ( i = 0; i < strlen (dirmask); i++ ) {
            if ( dirmask[i] == '/' || dirmask[i] == '\\' ) {
                dirmask[i] = '\0';
                break;
            }
        }

        if ( (hFile = _findfirst ( dirmask, &fd )) != -1L ) {
            if ( (fd.attrib & _A_SUBDIR) && !(fd.attrib & _A_HIDDEN) && !(fd.attrib & _A_SYSTEM) ) {
                if ( strcmp ( fd.name, "." ) != 0 && strcmp ( fd.name, ".." ) != 0 ) {
                    _chdir ( fd.name );
                    errors += _find ( newmask, Files );
                    _chdir ( temppath );
                }
            }

            while ( _findnext ( hFile, &fd ) == 0 ) {
                if ( (fd.attrib & _A_SUBDIR) && !(fd.attrib & _A_HIDDEN) && !(fd.attrib & _A_SYSTEM) ) {
                    if ( strcmp ( fd.name, "." ) != 0 && strcmp ( fd.name, ".." ) != 0 ) {
                        _chdir ( fd.name );
                        errors += _find ( newmask, Files );
                        _chdir ( temppath );
                    }
                }
            }

            _findclose ( hFile );
        }
    } else {            // search for files
        _getcwd_long ( path, _MAX_PATH );
        _chdir ( cwdpath );
        if ( path[0] != '\0' && path[strlen (path)-1] != '/' && path[strlen (path)-1] != '\\' )
            strcat ( path, "\\" );

        if ( mask[0] != '\0' && _chdir (mask) != 0 ) {  // not a directory
            if ( (hFile = _findfirst ( mask, &fd )) != -1L ) {
                if ( !(fd.attrib & _A_SUBDIR) && !(fd.attrib & _A_HIDDEN) && !(fd.attrib & _A_SYSTEM) ) {
                    _splitpath ( fd.name, NULL, NULL, NULL, ext );
                    if ( stricmp ( ext, ".m3u" ) == 0 ) {
                        _find_loadm3u ( fd.name, path, Files );
                    } else {
                        if ( (Files->Files = (FileListEntry *)realloc ( Files->Files, sizeof (FileListEntry) * (++Files->FileCount) )) == NULL ) {
                            fprintf ( stderr, "_find: Memory allocation failed.\n" );
                            exit (1);
                        }
                        memcpy ( &Files->Files[Files->FileCount-1].Info, &fd, sizeof (fd) );
                        strcpy ( Files->Files[Files->FileCount-1].Path, path );
                    }
                }

                while ( _findnext ( hFile, &fd ) == 0 ) {
                    if ( !(fd.attrib & _A_SUBDIR) && !(fd.attrib & _A_HIDDEN) && !(fd.attrib & _A_SYSTEM) ) {
                        _splitpath ( fd.name, NULL, NULL, NULL, ext );
                        if ( stricmp ( ext, ".m3u" ) == 0 ) {
                            _find_loadm3u ( fd.name, path, Files );
                        } else {
                            if ( (Files->Files = (FileListEntry *)realloc ( Files->Files, sizeof (FileListEntry) * (++Files->FileCount) )) == NULL ) {
                                fprintf ( stderr, "_find: Memory allocation failed.\n" );
                                exit (1);
                            }
                            memcpy ( &Files->Files[Files->FileCount-1].Info, &fd, sizeof (fd) );
                            strcpy ( Files->Files[Files->FileCount-1].Path, path );
                        }
                    }
                }

                _findclose ( hFile );
            }
        } else {                                        // target is a directory
            _getcwd_long ( path, _MAX_PATH );
            if ( path[0] != '\0' && path[strlen (path)-1] != '/' && path[strlen (path)-1] != '\\' )
                strcat ( path, "\\" );

            if ( (hFile = _findfirst ( "*", &fd )) != -1L ) {
                if ( !(fd.attrib & _A_SUBDIR) && !(fd.attrib & _A_HIDDEN) && !(fd.attrib & _A_SYSTEM) ) {
                    _splitpath ( fd.name, NULL, NULL, NULL, ext );
                    if ( stricmp ( ext, ".m3u" ) == 0 ) {
                        _find_loadm3u ( fd.name, path, Files );
                    } else {
                        if ( (Files->Files = (FileListEntry *)realloc ( Files->Files, sizeof (FileListEntry) * (++Files->FileCount) )) == NULL ) {
                            fprintf ( stderr, "_find: Memory allocation failed.\n" );
                            exit (1);
                        }
                        memcpy ( &Files->Files[Files->FileCount-1].Info, &fd, sizeof (fd) );
                        strcpy ( Files->Files[Files->FileCount-1].Path, path );
                    }
                }

                while ( _findnext ( hFile, &fd ) == 0 ) {
                    if ( !(fd.attrib & _A_SUBDIR) && !(fd.attrib & _A_HIDDEN) && !(fd.attrib & _A_SYSTEM) ) {
                        _splitpath ( fd.name, NULL, NULL, NULL, ext );
                        if ( stricmp ( ext, ".m3u" ) == 0 ) {
                            _find_loadm3u ( fd.name, path, Files );
                        } else {
                            if ( (Files->Files = (FileListEntry *)realloc ( Files->Files, sizeof (FileListEntry) * (++Files->FileCount) )) == NULL ) {
                                fprintf ( stderr, "_find: Memory allocation failed.\n" );
                                exit (1);
                            }
                            memcpy ( &Files->Files[Files->FileCount-1].Info, &fd, sizeof (fd) );
                            strcpy ( Files->Files[Files->FileCount-1].Path, path );
                        }
                    }
                }

                _findclose ( hFile );
            }

            _chdir ( cwdpath );
        }
    }

    return errors;
}

// Returns long name of current working directory
char* _getcwd_long ( char* buffer, int maxlen )
{
    struct          _finddata_t fd;
    long            hFile;
    char            path[_MAX_PATH];
    char            temp[_MAX_PATH];
    char*           p;
    char*           t;

    if ( _getcwd ( path, maxlen ) == NULL )
        return NULL;

    p = path;
    t = temp;

    while ( *p ) {
        int ch = *p++;
        *t++ = ch;
        if ( ch == '\\' || ch == '/' || ch == '\0' ) {
            *t++ = '\0';
            t = temp;
            _chdir ( temp );
            break;
        }
    };

    while ( 1 ) {
        int ch = *p++;
        if ( ch == '\\' || ch == '/' || ch == '\0' ) {
            *t++ = '\0';
            if ( (hFile = _findfirst ( temp, &fd )) != -1L ) {
                if ( fd.attrib & _A_SUBDIR ) _chdir ( fd.name );
                _findclose ( hFile );
            }
            t = temp;
        } else {
            *t++ = ch;
        }
        if ( ch == '\0' ) break;
    };

    if ( _getcwd ( buffer, maxlen ) == NULL )
        return NULL;
    else
        return buffer;
}
