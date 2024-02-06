#include <stdio.h>
#include <string.h>
#include <io.h>
#include <string.h>
#include <stdlib.h>
#include <windows.h>
#include <vorbis/vorbisfile.h>
#include "tag.h"
#include "ogg.h"

int ReadFileInfoVorbis ( FileInfo* Info )
{
    char*           temp;
    FILE*           fp;
    OggVorbis_File  vf;
    vorbis_info*    vi;
    vorbis_comment* vc;
    double          duration;
    long            bitrate;

    if ( (fp = fopen ( Info->Filename, "rb" )) == NULL ) {
        return 1;
    }
    if ( ov_open (fp, &vf, NULL, 0 ) != 0 ) {
        fclose (fp);
        return 0;
    }

    vi          = ov_info       ( &vf, -1 );
    duration    = ov_time_total ( &vf, -1 );
    bitrate     = ov_bitrate    ( &vf, -1 );
    vc          = ov_comment    ( &vf, -1 );

    if ( vi == NULL || vc == NULL ) {
        ov_clear (&vf);
        return 1;
    }
    if ( duration == OV_EINVAL )
        duration = 0;
    if ( bitrate == OV_EINVAL || bitrate == OV_FALSE )
        bitrate = 0;

    if ( (temp = malloc ( strlen (vc->vendor) + strlen ("Ogg Vorbis () 1000000000 kbps nominal") )) != NULL ) {
        sprintf ( temp, "Ogg Vorbis (%s) %u kbps nominal", vc->vendor, vi->bitrate_nominal / 1000 );
        strncpy ( Info->Details.Format, temp, sizeof (Info->Details.Format)-1 );
        Info->Details.Format[sizeof (Info->Details.Format)-1] = '\0';
        free ( temp );
    } else {
        sprintf ( Info->Details.Format, "Ogg Vorbis %u kbps nominal", vi->bitrate_nominal / 1000 );
    }

    Info->Details.BitRate       = (bitrate + 500) / 1000;
    Info->Details.Duration      = (unsigned long)duration;
    Info->Details.Channels      = vi->channels;
    Info->Details.SampleRate    = vi->rate;
    Info->Details.Format[sizeof (Info->Details.Format)-1] = '\0';

    sprintf ( Info->Details.Quality, "VBR %u", vi->bitrate_nominal / 1000 );

    ov_clear (&vf);
    return 0;
}
