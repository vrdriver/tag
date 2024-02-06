#include <stdio.h>
#include <string.h>
#include <io.h>
#include <string.h>
#include <stdlib.h>
#include <windows.h>
#include "Tag.h"
#include "tags.h"
#include "misc.h"

// Get last part of path ( C:\Program Files\Music -> Music )
void _get_path ( const char* path, char* buffer )
{
    if ( path != NULL && path[0] != '\0' ) {
        const char* s = (char *)(path + strlen (path)-1);
        if ( s > path && (*s == '/' || *s == '\\') ) s--;
        while ( s >= path && (*s != '/' && *s != '\\') ) {
            s--;
        }
        if ( s < path ) s = path;
        if ( *s == '/' || *s == '\\' ) s++;

        strcpy ( buffer, s );
        if ( buffer[0] != '\0' && (buffer[strlen (buffer)-1] == '/' || buffer[strlen (buffer)-1] == '\\') )  {
            buffer[strlen (buffer)-1] = '\0';
        }
    } else {
        buffer[0] = '\0';
    }
}

// Checks file existence
int isfile ( const char* filename )
{
    FILE* fp;
    if ( (fp = fopen ( filename, "rb" )) == NULL )
        return 0;

    fclose (fp);
    return 1;
}

// Reads string from file and removes white spaces from the beginning and the end
int fgets_clean ( char* buffer, int maxlen, FILE* fp )
{
    char* p;
    char* temp;

    if ( fgets ( buffer, maxlen, fp ) == NULL )
        return EOF;
    if ( buffer[0] == '\0' )
        return 0;
    buffer[maxlen-1] = '\0';

    if ( buffer[0] == 7 || buffer[0] == 10 || buffer[0] == 13 || buffer[0] == 32 ) {
        if ( (temp = malloc ( maxlen + 1 )) == NULL ) {
            fprintf ( Options.output, "fgets_clean: Memory allocation failed.\n" );
            exit (1);
        }

        p = buffer;
        while ( *p == 7 || *p == 10 || *p == 13 || *p == 32 ) {
            p++;
        }

        strcpy ( temp,   p    );
        strcpy ( buffer, temp );
        free ( temp );
    }

    p = (char *)(buffer + strlen (buffer)-1);
    while ( p >= buffer && (*p == 7 || *p == 10 || *p == 13 || *p == 32) ) {
        *p-- = '\0';
    }

    return 0;
}

// Gets one word from string
size_t get_word ( char* buffer, const char* string, size_t* startpos )
{
    char* s = (char *)(string + *startpos);
    char* d = buffer;
    int quotation = 0;

    while ( *s && *s == ' ' ) s++;
    if ( *s == '\0' ) return 0;

    while ( *s ) {
        char c = *s++;
        if ( c == ' ' && quotation == 0 ) {
            break;
        }
        if ( c == '"' ) {
            quotation = 1 - quotation;
        } else {
            *d++ = c;
        }
    };

    *d++ = '\0';
    *startpos = s - string;

    return s - string;
}

// Converts string from Windows OEM to Windows ANSI
void string_a2l ( const char* src, char* dst )
{
    OemToChar ( src, dst );
}

// Converts string from windows ANSI to Windows OEM
void string_l2a ( const char* src, char* dst )
{
    CharToOem ( src, dst );
}

// Replaces % sequences with real characters ( %20 -> ' ' )
void fix_percentage_sequences ( char* string )
{
    char*   temp;
    char*   s;
    char*   d;
    char*   t;
    WCHAR   ustring[2];
    int     b1, b2, i, len;
    int     czeros;
    int     chex;

    if ( (temp = malloc (strlen (string)*2 + 1)) == NULL ) {
        fprintf ( Options.output, "fix_percentage_sequences: Memory allocation failed.\n" );
        exit (1);
    }

    d = temp;
    s = string;
    ustring[1] = L'\0';

    do {
        if ( *s != '%' ) {
            *d++ = *s;
        } else {
            s++;
            t = s;
            czeros = 0;
            chex = 0;

            // count number of '0' characters starting the code
            while ( *t && *t == '0' ) {
                czeros++;
                t++;
            }
            if ( czeros & 1 ) {
                czeros--;
                t--;
            }

            // count hex codes
            while ( *t && (chex < czeros+2) && ((*t >= '0' && *t <= '9') || (*t >= 'a' && *t <= 'f') || (*t >= 'A' && *t <= 'F')) ) {
                chex++;
                t++;
            }

            // check if code is invalid
            if ( !((czeros == 0 && chex == 2) ||
                   (czeros == 2 && chex == 4) ||
                   (czeros == 4 && chex == 6) ||
                   (czeros == 6 && chex == 8)) ) {
                *d++ = '%';
                *d++ = *s;
            } else
            // only supports 16 bit Unicode
            if ( chex > 4 ) {
                *d++ = '?';
                s += czeros + chex - 1;
            } else {
                unsigned long code = 0;
                t = (char *)(s + czeros);

                for ( i = 0; i < chex/2; i++ ) {
                    int t0 = *t++;
                    int t1 = *t++;

                    if      ( t0 >= '0' && t0 <= '9' ) b1 = t0 -'0';
                    else if ( t0 >= 'a' && t0 <= 'f' ) b1 = t0 -'a' + 10;
                    else if ( t0 >= 'A' && t0 <= 'F' ) b1 = t0 -'A' + 10;
                    else b1 = 0;

                    if      ( t1 >= '0' && t1 <= '9' ) b2 = t1 -'0';
                    else if ( t1 >= 'a' && t1 <= 'f' ) b2 = t1 -'a' + 10;
                    else if ( t1 >= 'A' && t1 <= 'F' ) b2 = t1 -'A' + 10;
                    else b2 = 0;

                    code |= ((b1 << 4) + b2) << ((chex/2 - i - 1)*8);
                }

                ustring[0] = (WCHAR)code;

                if ( (len = unicodeToUtf8 ( ustring, d, 1 )) == 0 ) {
                    *d++ = '%';
                    *d++ = *s;
                } else {
                    d += len;
                    s += czeros + chex - 1;
                }
            }
        }
    } while ( *s++ != '\0' );

    strcpy ( string, temp );

    free ( temp );
}

// Replaces spaces by ch in string
void replace_spaces ( char* string, const char ch )
{
    char* p = string;

    do {
        if ( *p == ' ' ) *p = ch;
    } while ( *p++ );
}

// Replaces ch1 by ch2 in string
void replace_characters ( char* string, const char ch1, const char ch2 )
{
    char* p = string;

    do {
        if ( *p == ch1 ) *p = ch2;
    } while ( *p++ );
}

// Converts string to all lower case
void lower_case ( char* string )
{
    CharLower ( string );
}

// Converts string to all upper case
void upper_case ( char* string )
{
    CharUpper ( string );
}

// Converts first character to upper case, rest to lower
void sentence_case ( char* string )
{
    char* p = string;
    char tmp[2];
    tmp[1] = '\0';

    // skip spaces
    while ( *p == ' ' ) p++;
    // skip '(', '[' and '{'
    while ( *p == '(' || *p == '[' || *p == '{' ) p++;

    tmp[0] = *p;
    CharUpper ( tmp );
    *p++ = tmp[0];
    while ( *p ) {
        tmp[0] = *p;
        CharLower ( tmp );
        *p++ = tmp[0];
    }
}

// Checks if character should be capital
int _should_be_capital ( const char* string, int pos )
{
    const char cmarks[] = "@œ#$%&´`'";

    size_t i;
    if ( pos == 0 ) return 1;                                           // First letter always capital
    if ( string[pos-1] >= '0' && string[pos-1] <= '9' ) return 0;       // Not after digits (eg. 10th)
    if ( IsCharAlpha (string[pos-1]) ) return 0;                        // Not after letters
    if ( string[pos-1] == ' ' ) return 1;                               // After space
    if ( string[pos-1] == '_' ) return 1;                               // After underscore
    if ( string[pos-1] == '"' ) return 1;                               // After quotation mark
    if ( string[pos-1] == '\'' ) {
        if ( pos > 1 ) {
            if ( string[pos-2] == '\'' ) return 1;                      // Capital after quotation (")
            else return 0;                                              // If only "'", then not
        }
    }

    for ( i = 0; i < sizeof (cmarks); i++ ) {                           // Not after these characters
        if ( string[pos-1] == cmarks[i] ) return 0;
    }

    return 1;                                                           // Else capital
}

// Converts first character of every word to upper case
void capitalize_case ( char* string )
{
    size_t i;
    char tmp[2];
    tmp[1] = '\0';

    for ( i = 0; i < strlen (string); i++ ) {
        tmp[0] = string[i];
        if ( _should_be_capital (string, i) ) {
            CharUpper ( tmp );
        } else { // rest of the word lower case
            CharLower ( tmp );
        }
        string[i] = tmp[0];
    }
}

// Checks if word belongs to exceptions and should be left as is
void exceptions_in_case ( char* string, const ExceptionList* Exceptions )
{
    size_t i, j;
    if ( Exceptions->Words == NULL || Exceptions->WordCount == 0 ) return;

    for ( i = 0; i < strlen (string); i++ ) {
        if ( _should_be_capital ( string, i ) ) {
            for ( j = 0; j < Exceptions->WordCount; j++ ) {
                size_t e_len = strlen (Exceptions->Words[j]);
                if ( i + e_len > strlen (string) )
                    continue;

                if ( string[i + e_len] != '\0' ) {
                    if ( IsCharAlpha (string[i + e_len]) ) continue;
                }
                if ( strnicmp ( Exceptions->Words[j], string + i, e_len ) == 0 ) {
                    memcpy ( string + i, Exceptions->Words[j], e_len );
                    break;
                }
            }
        }
    }
}

// Removes characters that are not allowed in filenames
void remove_unsupported_chars ( char* string )
{
    const char unsupported[]    = { '\\', '/', ':', '*',  '?',  '"',  '<',  '>',  '|'  };
    const char replacement[][3] = { "-",  "-", "-", "\0", "\0", "''", "-",  "-",  "\0" };

    char    temp[_MAX_PATH];
    size_t  t_pos = 0;
    size_t  i, j;

    for ( i = 0; i < strlen (string); i++ ) {
        int unsupported_char = 0;

        for ( j = 0; j < sizeof (unsupported); j++ ) {
            if ( string[i] == unsupported[j] ) {
                unsupported_char = 1;
                if ( replacement[j][0] ) temp[t_pos++] = replacement[j][0];
                if ( replacement[j][1] ) temp[t_pos++] = replacement[j][1];
                break;
            }
        }
        if ( !unsupported_char ) {
            temp[t_pos++] = string[i];
        }
    }
    temp[t_pos] = '\0';
    strcpy ( string, temp );
}

// Replaces characters
void ReplaceCharacters ( char* string, const CharReplaceList* Replace )
{
    char    temp[_MAX_PATH * 2];
    char*   p = temp;
    size_t  i, j;


    for ( i = 0; i < strlen (string); i++ ) {
        int replaced = 0;

        for ( j = 0; j < Replace->CharCount; j++ ) {
            if ( string[i] == Replace->OldChars[j] ) {
                replaced = 1;
                if ( Replace->NewChars[j] != NULL ) {
                    p += sprintf ( p, "%s", Replace->NewChars[j] );
                }
                break;
            }
        }

        if ( !replaced ) {
            *p++ = string[i];
        }
    }
    *p = '\0';
    strcpy ( string, temp );
}

// Replaces characters on filename for tagging
void ReplaceCharactersForTag ( char* string, const CharReplaceForTag* ReplaceTag )
{
    char    temp[_MAX_PATH * 2];
    char*   p = temp;
    size_t  i, j;

    for ( i = 0; i < strlen (string); i++ ) {
        int replaced = 0;

        for ( j = 0; j < ReplaceTag->CharCount; j++ ) {
            if ( strncmp ( string + i, ReplaceTag->OldChars[j], strlen (ReplaceTag->OldChars[j]) ) == 0 ) {
                replaced = 1;
                if ( ReplaceTag->NewChars[j] != '\0' ) {
                    *p++ = ReplaceTag->NewChars[j];
                }
                i += strlen (ReplaceTag->OldChars[j]) - 1;
                break;
            }
        }

        if ( !replaced ) {
            *p++ = string[i];
        }
    }
    *p = '\0';
    strcpy ( string, temp );
}
