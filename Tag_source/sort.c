#include "sort.h"

int _compare_by_numbers ( const char* arg1, const char* arg2 )
{
    char    t1 [65536];
    char    t2 [65536];
    char    num  [256];
    int     i;

    if ( arg1 == NULL && arg2 == NULL )
        return 0;
    if ( arg1 == NULL )
        return -1;
    if ( arg2 == NULL )
        return +1;

    for ( i = 0; i < 2; i++ ) {
        const char* s;
        char*       d;
        char*       d_end;

        if ( i == 0 ) {
            s     = arg1;
            d     = t1;
            d_end = t1 + sizeof (t1) - 1;
        } else {
            s = arg2;
            d = t2;
            d_end = t2 + sizeof (t2) - 1;
        }

        do {
            if ( *s >= '0' && *s <= '9' ) {
                char* p     = num;
                char* p_end = num + sizeof (num) - 1;
                while ( *s && (*s >= '0' && *s <= '9') && p < p_end ) {
                    *p++ = *s++;
                }
                *p++ = '\0';
                if ( d + 32 < d_end ) {
                    int j;
                    for ( j = 0; j < 32 - (int)strlen (num); j++ ) {
                        *d++ = '0';
                    }
                    d += sprintf ( d, "%s", num );
                } else {
                    break;
                }
            } else {
                *d++ = *s;
            }
        } while ( *s++ && d < d_end );

        *d++ = '\0';
    }

    return lstrcmpi ( t1, t2 );
}
