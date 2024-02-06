#include <stdio.h>
#include <string.h>
#include <io.h>
#include <string.h>
#include <stdlib.h>
#include <windows.h>
#include "tag.h"

#define COMPRESSION_LEVEL_FAST			1000
#define COMPRESSION_LEVEL_NORMAL		2000
#define COMPRESSION_LEVEL_HIGH			3000
#define COMPRESSION_LEVEL_EXTRA_HIGH	4000

#define MAC_FORMAT_FLAG_8_BIT				1	// is 8-bit
#define MAC_FORMAT_FLAG_CRC					2	// uses the new CRC32 error detection
#define MAC_FORMAT_FLAG_HAS_PEAK_LEVEL		4	// unsigned __int32 Peak_Level after the header
#define MAC_FORMAT_FLAG_24_BIT				8	// is 24-bit
#define MAC_FORMAT_FLAG_HAS_SEEK_ELEMENTS	16	// has the number of seek elements after the peak level
#define MAC_FORMAT_FLAG_CREATE_WAV_HEADER	32  // create the wave header on decompression (not stored)

typedef struct {
    char cID[4];                            // should equal 'MAC '
    unsigned __int16 nVersion;              // version number * 1000 (3.81 = 3810)
    unsigned __int16 nCompressionLevel;     // the compression level
    unsigned __int16 nFormatFlags;          // any format flags (for future use)
    unsigned __int16 nChannels;             // the number of channels (1 or 2)
    unsigned __int32 nSampleRate;           // the sample rate (typically 44100)
    unsigned __int32 nHeaderBytes;          // the bytes after the MAC header that compose the WAV header
    unsigned __int32 nTerminatingBytes;     // the bytes after that raw data (for extended info)
    unsigned __int32 nTotalFrames;          // the number of frames in the file
    unsigned __int32 nFinalFrameBlocks;     // the number of samples in the final frame
} APE_HEADER;

int ReadFileInfoAPE ( FileInfo* Info )
{
    FILE*               fp;
    APE_HEADER          header;
    unsigned long       nBlocksPerFrame;
    int                 nBitsPerSample;
    unsigned __int64    samples;

    if ( (fp = fopen ( Info->Filename, "rb" )) == NULL )
        return 1;

    if ( (fread ( &header, 1, sizeof (header), fp )) != sizeof (header) ) {
        fclose ( fp );
        return 1;
    }
    fclose ( fp );

    if ( memcmp ( header.cID, "MAC ", 4 ) != 0 )
        return 1;

    nBlocksPerFrame = ((header.nVersion >= 3900) || ((header.nVersion >= 3800) && (header.nCompressionLevel == COMPRESSION_LEVEL_EXTRA_HIGH))) ? 73728 : 9216;
    if ((header.nVersion >= 3950)) nBlocksPerFrame = 73728 * 4;
    nBitsPerSample = (header.nFormatFlags & MAC_FORMAT_FLAG_8_BIT) ? 8 : ((header.nFormatFlags & MAC_FORMAT_FLAG_24_BIT) ? 24 : 16);
    samples = (header.nTotalFrames == 0) ? 0 : ((header.nTotalFrames -  1) * nBlocksPerFrame) + header.nFinalFrameBlocks;

    if ( header.nVersion % 10 == 0 )
        sprintf ( Info->Details.Format, "Monkey's Audio %u.%u", header.nVersion/1000, (header.nVersion%1000)/10 );
    else
        sprintf ( Info->Details.Format, "Monkey's Audio %u.%u", header.nVersion/1000, header.nVersion%1000 );

    Info->Details.Duration      = (header.nSampleRate > 0)  ?  (unsigned long)(samples / header.nSampleRate)  :  0;
    Info->Details.SampleRate    = header.nSampleRate;
    Info->Details.Channels      = header.nChannels;
    Info->Details.BitRate       = (Info->Details.Duration > 0)  ?  (Info->FileSize * 8 / Info->Details.Duration + 500) / 1000  :  0;

    return 0;
}
