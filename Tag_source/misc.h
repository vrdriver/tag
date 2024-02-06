#ifndef _tag_misc_h
#define _tag_misc_h

typedef struct {
    char**          Words;
    size_t          WordCount;
} ExceptionList;

typedef struct {
    char*           OldChars;
    char**          NewChars;
    size_t          CharCount;
} CharReplaceList;

extern CharReplaceList Replace;

typedef struct {
    char**          OldChars;
    char*           NewChars;
    size_t          CharCount;
} CharReplaceForTag;

extern CharReplaceForTag ReplaceTag;

void _get_path ( const char* path, char* buffer );      // Get last part of path ( C:\Program Files\Music -> Music )

int isfile ( const char* filename );                    // Checks file existence
int fgets_clean ( char* buffer, int maxlen, FILE* fp ); // Reads string from file and removes white spaces from the beginning and the end
size_t get_word ( char* buffer, const char* string, size_t* startpos ); // Gets one word from string

void string_a2l ( const char* src, char* dst );         // Converts string from Windows OEM to Windows ANSI
void string_l2a ( const char* src, char* dst );         // Converts string from Windows ANSI to Windows OEM

void fix_percentage_sequences ( char* string );         // Replaces % sequences with real characters ( %20 -> ' ' )

void replace_spaces     ( char* string, const char ch );// Replaces spaces by ch in string
void replace_characters ( char* string, const char ch1, const char ch2 );   // Replaces ch1 by ch2 in string
void lower_case         ( char* string );               // Converts string to all lower case
void upper_case         ( char* string );               // Converts string to all upper case
void sentence_case      ( char* string );               // Converts first character to upper case, rest to lower
void capitalize_case    ( char* string );               // Converts first character of every word to upper case
void exceptions_in_case ( char* string, const ExceptionList* Exceptions );    // Checks if word belongs to exceptions and should be left as is
void remove_unsupported_chars ( char* string );         // Removes characters not allowed in filenames
void ReplaceCharacters ( char* string, const CharReplaceList* Replace ); // Replaces characters
void ReplaceCharactersForTag ( char* string, const CharReplaceForTag* ReplaceTag ); // Replaces characters on filename for tagging

#endif
