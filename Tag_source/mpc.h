#ifndef _tag_mpc_h
#define _tag_mpc_h

#include <stdio.h>
#include <string.h>
#include <io.h>
#include <stdlib.h>
#include <windows.h>
#include "Tag.h"
#include "tags.h"

typedef struct {
    double              SampleFreq;
    unsigned int        Channels;
    long                HeaderPosition;     // byte position of header
    unsigned int        StreamVersion;      // Streamversion of current file
    unsigned int        Bitrate;            // bitrate of current file (bps)
    double              AverageBitrate;     // Average bitrate in bits/sec
    unsigned int        Frames;             // number of frames contained
    __int64             PCMSamples;
    unsigned int        MaxBand;            // maximum band-index used (0...31)
    unsigned int        IS;                 // Intensity Stereo (0: off, 1: on)
    unsigned int        MS;                 // Mid/Side Stereo (0: off, 1: on)
    unsigned int        BlockSize;          // only needed for SV4...SV6 -> not supported
    unsigned int        Profile;            // quality profile
    const char*         ProfileName;

    // ReplayGain related data
    short               GainTitle;
    short               GainAlbum;
    unsigned short      PeakAlbum;
    unsigned short      PeakTitle;

    // true gapless stuff
    unsigned int        IsTrueGapless;      // is true gapless used?
    unsigned int        LastFrameSamples;   // number of valid samples within last frame

    unsigned int        EncoderVersion;     // version of encoder used
    char                Encoder[256];

    long TagOffset;
    long TotalFileLength;
} BasicData;

typedef struct {
    BasicData       simple;
} StreamInfo;

int ReadFileInfoMPC ( FileInfo* Info );

#endif
