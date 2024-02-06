#include <stdio.h>
#include <string.h>
#include <io.h>
#include <string.h>
#include <stdlib.h>
#include <windows.h>
#include <FLAC/metadata.h>
#include "tag.h"
#include "flac.h"

int ReadFileInfoFLAC ( FileInfo* Info )
{
    FLAC__StreamMetadata    info;

    if ( !FLAC__metadata_get_streaminfo ( Info->Filename, &info) )
        return 1;

    if ( info.type != FLAC__METADATA_TYPE_STREAMINFO )
        return 1;

    // bitspersample = info.data.stream_info.bits_per_sample;

    sprintf ( Info->Details.Format, "FLAC" );

    Info->Details.Duration      = (info.data.stream_info.sample_rate > 0)  ?  (unsigned long)(info.data.stream_info.total_samples / info.data.stream_info.sample_rate)  :  0;
    Info->Details.SampleRate    = info.data.stream_info.sample_rate;
    Info->Details.Channels      = info.data.stream_info.channels;
    Info->Details.BitRate       = (Info->Details.Duration > 0)  ?  (Info->FileSize * 8 / Info->Details.Duration + 500) / 1000  :  0;

    return 0;
}
