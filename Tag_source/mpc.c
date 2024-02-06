#include <stdio.h>
#include <string.h>
#include <io.h>
#include <string.h>
#include <stdlib.h>
#include <windows.h>
#include "mpc.h"

// Most functions are from MPC Winamp plugin

// searches for a ID3v2-tag and reads the length (in bytes) of it
// -1 on errors of any kind
long JumpID3v2 ( FILE* fp )
{
    unsigned char  tmp [10];
    unsigned int   Unsynchronisation;   // ID3v2.4-flag
    unsigned int   ExtHeaderPresent;    // ID3v2.4-flag
    unsigned int   ExperimentalFlag;    // ID3v2.4-flag
    unsigned int   FooterPresent;       // ID3v2.4-flag
    long           ret;

    fread  ( tmp, 1, sizeof(tmp), fp );

    // check id3-tag
    if ( 0 != memcmp ( tmp, "ID3", 3) )
        return 0;

    // read flags
    Unsynchronisation = tmp[5] & 0x80;
    ExtHeaderPresent  = tmp[5] & 0x40;
    ExperimentalFlag  = tmp[5] & 0x20;
    FooterPresent     = tmp[5] & 0x10;

    if ( tmp[5] & 0x0F )
        return -1;              // not (yet???) allowed
    if ( (tmp[6] | tmp[7] | tmp[8] | tmp[9]) & 0x80 )
        return -1;              // not allowed

    // read HeaderSize (syncsave: 4 * $0xxxxxxx = 28 significant bits)
    ret  = tmp[6] << 21;
    ret += tmp[7] << 14;
    ret += tmp[8] <<  7;
    ret += tmp[9]      ;
    ret += 10;
    if ( FooterPresent )
        ret += 10;

    return ret;
}

static const char*
Stringify ( unsigned int profile )            // profile is 0...15, where 7...13 is used
{
    static const char   na    [] = "unknown";
    static const char*  Names [] = {
        na, "'Unstable/Experimental'", na, na,
        na, "below 'Telephone'", "below 'Telephone'", "'Telephone'",
        "'Thumb'", "'Radio'", "'Standard'", "'Xtreme'",
        "'Insane'", "'BrainDead'", "above 'BrainDead'", "above 'BrainDead'"
    };

    return profile >= sizeof(Names)/sizeof(*Names)  ?  na  :  Names [profile];
}

// read information from SV8 header
int ReadHeaderSV8 ( FILE* fp, StreamInfo* Info )
{
    return 0;
}

// read information from SV7 header
int ReadHeaderSV7 ( FILE* fp, StreamInfo* Info )
{
    const long samplefreqs [4] = { 44100, 48000, 37800, 32000 };

    unsigned int    HeaderData [8];
    unsigned short  EstimatedPeakTitle = 0;

    if ( fseek ( fp, Info->simple.HeaderPosition, SEEK_SET ) != 0 )         // seek to header start
        return 1;
    if ( fread ( HeaderData, 1, sizeof HeaderData, fp ) != sizeof HeaderData )
        return 1;

    Info->simple.Bitrate          = 0;
    Info->simple.Frames           =  HeaderData[1];
    Info->simple.IS               = 0;
    Info->simple.MS               = (HeaderData[2]>>30)   & 0x0001;
    Info->simple.MaxBand          = (HeaderData[2]>>24)   & 0x003F;
    Info->simple.BlockSize        = 1;
    Info->simple.Profile          = (HeaderData[2]<<8)>>28;
    Info->simple.ProfileName      = Stringify ( Info->simple.Profile );

    Info->simple.SampleFreq       = samplefreqs [(HeaderData[2]>>16) & 0x0003];

    EstimatedPeakTitle            =  HeaderData[2]        & 0xFFFF;         // read the ReplayGain data
    Info->simple.GainTitle        = (HeaderData[3] >> 16) & 0xFFFF;
    Info->simple.PeakTitle        =  HeaderData[3]        & 0xFFFF;
    Info->simple.GainAlbum        = (HeaderData[4] >> 16) & 0xFFFF;
    Info->simple.PeakAlbum        =  HeaderData[4]        & 0xFFFF;

    Info->simple.IsTrueGapless    = (HeaderData[5] >> 31) & 0x0001;         // true gapless: used?
    Info->simple.LastFrameSamples = (HeaderData[5] >> 20) & 0x07FF;         // true gapless: valid samples for last frame

    Info->simple.EncoderVersion   = (HeaderData[6] >> 24) & 0x00FF;
    if ( Info->simple.EncoderVersion == 0 ) {
        sprintf ( Info->simple.Encoder, "Buschmann 1.7.0...9, Klemm 0.90...1.05" );
    } else {
        switch ( Info->simple.EncoderVersion % 10 ) {
        case 0:
            sprintf ( Info->simple.Encoder, "Release %u.%u", Info->simple.EncoderVersion/100, Info->simple.EncoderVersion/10%10 );
            break;
        case 2: case 4: case 6: case 8:
            sprintf ( Info->simple.Encoder, "Beta %u.%02u", Info->simple.EncoderVersion/100, Info->simple.EncoderVersion%100 );
            break;
        default:
            sprintf ( Info->simple.Encoder, "--Alpha-- %u.%02u", Info->simple.EncoderVersion/100, Info->simple.EncoderVersion%100 );
            break;
        }
    }

    if ( Info->simple.PeakTitle == 0 )                                      // there is no correct PeakTitle contained within header
        Info->simple.PeakTitle = (unsigned short)(EstimatedPeakTitle * 1.18);
    if ( Info->simple.PeakAlbum == 0 )
        Info->simple.PeakAlbum = Info->simple.PeakTitle;                    // no correct PeakAlbum, use PeakTitle

    //Info->simple.SampleFreq    = 44100;                                     // AB: used by all files up to SV7
    Info->simple.Channels      = 2;

    return 0;
}

// read information from SV4-SV6 header
int ReadHeaderSV6 ( FILE* fp, StreamInfo* Info )
{
    unsigned int    HeaderData [8];

    if ( fseek ( fp, Info->simple.HeaderPosition, SEEK_SET ) != 0 )         // seek to header start
        return 1;
    if ( fread ( HeaderData, 1, sizeof HeaderData, fp ) != sizeof HeaderData )
        return 1;

    Info->simple.Bitrate          = (HeaderData[0]>>23) & 0x01FF;           // read the file-header (SV6 and below)
    Info->simple.IS               = (HeaderData[0]>>22) & 0x0001;
    Info->simple.MS               = (HeaderData[0]>>21) & 0x0001;
    Info->simple.StreamVersion    = (HeaderData[0]>>11) & 0x03FF;
    Info->simple.MaxBand          = (HeaderData[0]>> 6) & 0x001F;
    Info->simple.BlockSize        = (HeaderData[0]    ) & 0x003F;
    Info->simple.Profile          = 0;
    Info->simple.ProfileName      = "unknown";
    if ( Info->simple.StreamVersion >= 5 )
        Info->simple.Frames       =  HeaderData[1];                         // 32 bit
    else
        Info->simple.Frames       = (HeaderData[1]>>16);                    // 16 bit

    Info->simple.GainTitle        = 0;                                      // not supported
    Info->simple.PeakTitle        = 0;
    Info->simple.GainAlbum        = 0;
    Info->simple.PeakAlbum        = 0;

    Info->simple.LastFrameSamples = 0;
    Info->simple.IsTrueGapless    = 0;
    Info->simple.EncoderVersion   = 0;
    Info->simple.Encoder[0]       = '\0';

    if ( Info->simple.StreamVersion == 7 ) return 1;                        // are there any unsupported parameters used?
    if ( Info->simple.Bitrate       != 0 ) return 1;
    if ( Info->simple.IS            != 0 ) return 1;
    if ( Info->simple.BlockSize     != 1 ) return 1;

    if ( Info->simple.StreamVersion < 6 )                                   // Bugfix: last frame was invalid for up to SV5
        Info->simple.Frames -= 1;

    Info->simple.SampleFreq    = 44100;                                     // AB: used by all files up to SV7
    Info->simple.Channels      = 2;

    if ( Info->simple.StreamVersion < 4  ||  Info->simple.StreamVersion > 7 )
        return 1;

    return 0;
}

// reads file header and tags
int ReadStreamInfo ( FILE* fp, StreamInfo* Info )
{
    unsigned int    HeaderData[1];
    int             Error = 0;

    memset ( Info, 0, sizeof (StreamInfo) );                                // Reset Info-Data

    if ( (Info->simple.HeaderPosition =  JumpID3v2 (fp)) < 0 )              // get header position
        return 1;

    if ( fseek ( fp, Info->simple.HeaderPosition, SEEK_SET ) != 0 )         // seek to first byte of mpc data
        return 1;
    if ( fread ( HeaderData, 1, sizeof HeaderData, fp ) != sizeof HeaderData )
        return 1;
    if ( fseek ( fp, 0L, SEEK_END ) != 0 )                                  // get filelength
        return 1;
    Info->simple.TotalFileLength = ftell (fp);
    Info->simple.TagOffset = Info->simple.TotalFileLength;

    if ( memcmp ( HeaderData, "MP+", 3 ) == 0 ) {                           // check version
        Info->simple.StreamVersion = HeaderData[0] >> 24;

        if ( (Info->simple.StreamVersion & 0x0F) >= 8 )                     // StreamVersion 8
            Error = ReadHeaderSV8 ( fp, Info );
        else if ( (Info->simple.StreamVersion & 0x0F) == 7 )                // StreamVersion 7
            Error = ReadHeaderSV7 ( fp, Info );
    } else {                                                                // StreamVersion 4-6
        Error = ReadHeaderSV6 ( fp, Info );
    }

    Info->simple.PCMSamples = 1152 * Info->simple.Frames - 576;             // estimation, exact value needs too much time
    if ( Info->simple.PCMSamples != 0 )
        Info->simple.AverageBitrate = (Info->simple.TagOffset - Info->simple.HeaderPosition) * 8. * Info->simple.SampleFreq / Info->simple.PCMSamples;
    else
        Info->simple.AverageBitrate = 0;

    return Error;
}

unsigned long getlength (StreamInfo* Info)
{
    #define FRAMELEN        (36 * 32)                       // samples per frame
    return (unsigned long) (( (Info->simple.Frames-0.5) * FRAMELEN / Info->simple.SampleFreq + 0.0005f ));
}

int ReadFileInfoMPC ( FileInfo* Info )
{
    FILE*       fp;
    StreamInfo  MPCinfo;

    if ( (fp = fopen ( Info->Filename, "rb" )) == NULL ) {
        return 1;
    }
    if ( ReadStreamInfo ( fp, &MPCinfo ) != 0 ) {
        fclose (fp);
        return 1;
    }
    fclose (fp);

    Info->Details.BitRate    = (long)( MPCinfo.simple.AverageBitrate * 1.e-3 + 0.5 );
    Info->Details.Channels   = MPCinfo.simple.Channels;
    Info->Details.Duration   = getlength ( &MPCinfo );
    Info->Details.SampleRate = (long)MPCinfo.simple.SampleFreq;

    {
        char *p = Info->Details.Format;
        p += sprintf ( p, "Musepack SV%u.%u", MPCinfo.simple.StreamVersion & 0x0F, MPCinfo.simple.StreamVersion >> 4 );
        if ( MPCinfo.simple.ProfileName )
            p += sprintf ( p, " profile %s", MPCinfo.simple.ProfileName );
        if ( MPCinfo.simple.EncoderVersion > 0 && MPCinfo.simple.Encoder[0] )
            p += sprintf ( p, " (%s)", MPCinfo.simple.Encoder );
    }

    if ( MPCinfo.simple.ProfileName ) {
        char *sp = (char *)MPCinfo.simple.ProfileName;
        char *dp = (char *)Info->Details.Quality;

        do {
            if ( *sp != '\'' ) *dp++ = *sp;
        } while ( *sp++ != '\0' );
    }

    return 0;
}
