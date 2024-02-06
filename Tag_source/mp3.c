#include "mp3.h"
#include "Tag.h"

// -------------------------------------------------
// This file is modified version of CM3Info class by
// Gustav "Grim Reaper" Munkby
// http://home.swipnet.se/grd/
// grd@swipnet.se
// -------------------------------------------------

MP3StreamInfo MP3info;

int VBitRate_getNumberOfFrames()
{
    return MP3info.frames;
};

int VBitRate_loadHeader ( char inputheader[12] )
{
    #define FRAMES_FLAG    0x0001
    #define BYTES_FLAG     0x0002
    #define TOC_FLAG       0x0004
    #define VBR_SCALE_FLAG 0x0008

    int flags;

    // The Xing VBR headers always begin with the four
    // chars "Xing" so this tests wether we have a VBR
    // header or not
    if ( memcmp ( inputheader, "Xing", 4 ) ) {
        MP3info.frames = -1;
        return 0;
    }

    // now we will get the flags and number of frames,
    // this is done in the same way as the FrameHeader
    // is generated in the CFrameHeader class
    // if you're curious about how it works, go and look
    // there

    // here we get the flags from the next four bytes
    flags = (int)(( (inputheader[4] & 255) << 24) |
                  ( (inputheader[5] & 255) << 16) |
                  ( (inputheader[6] & 255) <<  8) |
                  ( (inputheader[7] & 255)      )); 

    // if this tag contains the number of frames, load
    // that number into storage, if not something will
    // be wrong when calculating the bitrate and length
    // of the music
    if ( flags & FRAMES_FLAG ) {
        MP3info.frames = (int)( ( (inputheader[ 8] & 255) << 24) |
                                ( (inputheader[ 9] & 255) << 16) |
                                ( (inputheader[10] & 255) <<  8) |
                                ( (inputheader[11] & 255)      ) );
        if (MP3info.frames == 0) MP3info.frames = -1;
    } else {
        // returning -1 so an error would be obvious
        // not many people would believe in a bitrate
        // -21 kbps :)
        MP3info.frames = -1;

        // this function was returning false before
        // as there is an error occuring, but in that
        // case the bitrate wouldn't be unbelievable
        // so that's why I changed my mind and let it
        // return true instead
        return 1;
    }

    // if it gets this far, everything went according
    // to plans, so we should return true!
    return 1;
}

void FrameHeader_loadHeader(char c[4]);
int FrameHeader_isValidHeader();

int FrameHeader_getFrameSync()     { return ((MP3info.bithdr>>21) & 2047); }
int FrameHeader_getVersionIndex()  { return ((MP3info.bithdr>>19) & 3);  }
int FrameHeader_getLayerIndex()    { return ((MP3info.bithdr>>17) & 3);  }
int FrameHeader_getProtectionBit() { return ((MP3info.bithdr>>16) & 1);  }
int FrameHeader_getBitrateIndex()  { return ((MP3info.bithdr>>12) & 15); }
int FrameHeader_getFrequencyIndex(){ return ((MP3info.bithdr>>10) & 3);  }
int FrameHeader_getPaddingBit()    { return ((MP3info.bithdr>> 9) & 1);  }
int FrameHeader_getPrivateBit()    { return ((MP3info.bithdr>> 8) & 1);  }
int FrameHeader_getModeIndex()     { return ((MP3info.bithdr>> 6) & 3);  }
int FrameHeader_getModeExtIndex()  { return ((MP3info.bithdr>> 4) & 3);  }
int FrameHeader_getCoprightBit()   { return ((MP3info.bithdr>> 3) & 1);  }
int FrameHeader_getOrginalBit()    { return ((MP3info.bithdr>> 2) & 1);  }
int FrameHeader_getEmphasisIndex() { return ((MP3info.bithdr    ) & 3);  }

// this returns the MPEG version [1.0-2.5]
float FrameHeader_getVersion();

// this returns the Layer [1-3]
int FrameHeader_getLayer();

// this returns the current bitrate [8-448 kbps]
int FrameHeader_getBitrate();

// this returns the current frequency [8000-48000 Hz]
int FrameHeader_getFrequency();

// the purpose of getMode is to get information about
// the current playing mode, such as:
// "Joint Stereo"
void FrameHeader_getMode(char* input);

// This function is quite easy to understand, it loads
// 4 chars of information into the CFrameHeader class
// The validity is not tested, so qith this function
// an invalid FrameHeader could be retrieved
void FrameHeader_loadHeader(char c[4])
{
    MP3info.bithdr = (unsigned long)(( (c[0] & 255) << 24) |
                                     ( (c[1] & 255) << 16) |
                                     ( (c[2] & 255) <<  8) |
                                     ( (c[3] & 255)      ) ); 
}

// This function is a supplement to the loadHeader
// function, the only purpose is to detect if the
// header loaded by loadHeader is a valid header
// or just four different chars
int FrameHeader_isValidHeader()
{
    return ( ((FrameHeader_getFrameSync()      & 2047)==2047) &&
             ((FrameHeader_getVersionIndex()   &    3)!=   1) &&
             ((FrameHeader_getLayerIndex()     &    3)!=   0) && 
             ((FrameHeader_getBitrateIndex()   &   15)!=   0) &&  // due to lack of support of the .mp3 format
                                                                  // no "public" .mp3's should contain information
                                                                  // like this anyway... :)
             ((FrameHeader_getBitrateIndex()   &   15)!=  15) &&
             ((FrameHeader_getFrequencyIndex() &    3)!=   3) &&
             ((FrameHeader_getEmphasisIndex()  &    3)!=   2) );
}

// this returns the MPEG version [1.0-2.5]
float FrameHeader_getVersion()
{
    const float table[4] = { 2.5, 0.0, 2.0, 1.0 };
    return table[FrameHeader_getVersionIndex()];
}

// this returns the Layer [1-3]
int FrameHeader_getLayer()
{
    return ( 4 - FrameHeader_getLayerIndex() );
}

// this returns the current bitrate [8-448 kbps]
int FrameHeader_getBitrate()
{
    const int table[2][3][16] = {
        {         //MPEG 2 & 2.5
            {0,  8, 16, 24, 32, 40, 48, 56, 64, 80, 96,112,128,144,160,0}, //Layer III
            {0,  8, 16, 24, 32, 40, 48, 56, 64, 80, 96,112,128,144,160,0}, //Layer II
            {0, 32, 48, 56, 64, 80, 96,112,128,144,160,176,192,224,256,0}  //Layer I
        },{       //MPEG 1
            {0, 32, 40, 48, 56, 64, 80, 96,112,128,160,192,224,256,320,0}, //Layer III
            {0, 32, 48, 56, 64, 80, 96,112,128,160,192,224,256,320,384,0}, //Layer II
            {0, 32, 64, 96,128,160,192,224,256,288,320,352,384,416,448,0}  //Layer I
        }
    };

    return table[(FrameHeader_getVersionIndex() & 1)][(FrameHeader_getLayerIndex() -1)][FrameHeader_getBitrateIndex()];
}

// this returns the current frequency [8000-48000 Hz]
int FrameHeader_getFrequency()
{
    const int table[4][3] = {
        {32000, 16000,  8000}, //MPEG 2.5
        {    0,     0,     0}, //reserved
        {22050, 24000, 16000}, //MPEG 2
        {44100, 48000, 32000}  //MPEG 1
    };

    return table[FrameHeader_getVersionIndex()][FrameHeader_getFrequencyIndex()];
}

// the purpose of getMode is to get information about
// the current playing mode, such as:
// "Joint Stereo"
void FrameHeader_getMode(char* input)
{
    // here you could use a array of strings instead
    // but I think this method is nicer, at least
    // when not dealing with that many variations
    switch(FrameHeader_getModeIndex()) {
        default:
            strcpy(input, "Stereo");
            break;
        case 1:
            strcpy(input, "Joint Stereo");
            break;
        case 2:
            strcpy(input, "Dual Channel");
            break;
        case 3:
            strcpy(input, "Single Channel");
            break;
    }
}

// function to load a file into this structure
// the argument passed is the path to a MP3 file
int MP3Info_loadInfo( FILE* fp );

// functions used to get information about the "file"
int  MP3Info_getFileSize() { return MP3info.fileSize; }
void MP3Info_getFileName(char* input);

// information that is avaliable in FrameHeader & VBR header
void  MP3Info_getVersion(char* input);
double /*int*/ MP3Info_getBitrate();
int   MP3Info_getFrequency() { return FrameHeader_getFrequency(); }
void  MP3Info_getMode(char* input);

int   MP3Info_getNumberOfFrames();

// functions to calculate the length of the song
// and to present it nicely
int   MP3Info_getLengthInSeconds();

// just to know what kind of file it is.
int  MP3Info_isVBitRate() { return MP3info.VBitRate; }

int MP3Info_loadInfo( FILE* fp )
{
	char id3v2[3];
    char headerchars[4]; // char variable used for header-loading
    char vbrchars[12];
    long pos;
	long id3v2pos;
	long endpos;

    // get file size, by setting the pointer in the end and tell the position
    if ( fseek ( fp, 0, SEEK_END ) != 0 )
        return 1;
    MP3info.fileSize = ftell (fp);

    pos = 0; // current position in file...

    /******************************************************/
    /* search and skip ID3v2 tag if exist at the beginning*/
    /******************************************************/

    do {
        // if no header has been found after 512Bytes
        // or the end of the file has been reached
        // then there's probably no mp3-file
        if ( pos > (512) || feof (fp) ) {
			break;
        }

        // read in four characters
        if ( fseek ( fp, pos, SEEK_SET ) != 0 )
            return 1;
        if ( fread (id3v2, 1, 3, fp ) != 3 )
            return 1;

        // move file-position forward
        pos++;
    } while ( memcmp(id3v2, "ID3", 3) != 0 );  // test for correct header

	// ID3v2/file identifier      "ID3"		(3bytes)
	// ID3v2 version              $04 00	(2bytes)
	// ID3v2 flags                %abcd0000	(1byte)
	// ID3v2 size             4 * %0xxxxxxx (4bytes)
	id3v2pos = -1;
	endpos = 0;
	if (memcmp(id3v2, "ID3", 3) == 0)
	{
		char tagFlags[1];
		char tagSize[4];
        int bExtendedHeader;
        long dwSize;

		pos = 5;	// skip to the fifth byte

		// read ID3v2 flags
		if ( fseek (fp, pos, SEEK_SET ) != 0 )
            return 1;
		if ( fread (&tagFlags, 1, 1, fp) != 1 )
            return 1;
		pos += 1;

		bExtendedHeader = ((tagFlags[0] & 0x40) == 0x40);

		// read ID3v2 size
		if ( fseek (fp, pos, SEEK_SET ) != 0 )
            return 1;
		if ( fread ( &tagSize, 1, 4, fp ) != 4 )
            return 1;
		pos += 4;

		dwSize = tagSize[3];
		dwSize |= (((long)tagSize[2]) << 7);
		dwSize |= (((long)tagSize[1]) << 14);
		dwSize |= (((long)tagSize[0]) << 21);

		endpos = pos + dwSize;
		MP3info.fileSize -= dwSize + 10;

		// skip the extended header if present
		if (bExtendedHeader)
		{
			if ( fseek ( fp, pos, SEEK_SET ) != 0 )
                return 1;
			if ( fread ( &tagSize, 1, 4, fp) != 4 )
                return 1;
			pos += 4;

			dwSize = tagSize[3];
			dwSize |= (((DWORD)tagSize[2]) << 7);
			dwSize |= (((DWORD)tagSize[1]) << 14);
			dwSize |= (((DWORD)tagSize[0]) << 21);

			pos += dwSize - 4;
		}

		id3v2pos = pos;		// update ID3v2 frame position in file
		pos = endpos;
    } else {
		pos = 0;
	}

    /******************************************************/
    /* search and load the first frame-header in the file */
    /******************************************************/
    
    do {
        // if no header has been found after 200kB
        // or the end of the file has been reached
        // then there's probably no mp3-file
        if ( pos > (endpos + 1024*200) || feof (fp) ) {
            return 1;
        }

        // read in four characters
        if ( fseek ( fp, pos, SEEK_SET ) != 0 )
            return 1;
        if ( fread ( &headerchars, 1, 4, fp ) != 4 )
            return 1;

        // move file-position forward
        pos++;
        
        // convert four chars to CFrameHeader structure
        FrameHeader_loadHeader(headerchars);
    } while ( !FrameHeader_isValidHeader() );  // test for correct header

    // to correct the position to be right after the frame-header
    // we'll need to add another 3 to the current position
    pos += 3;

    /******************************************************/
    /* check for an vbr-header, to ensure the info from a */
    /* vbr-mp3 is correct                                 */
    /******************************************************/

    // determine offset from first frame-header
    // it depends on two things, the mpeg-version
    // and the mode(stereo/mono)

    if( FrameHeader_getVersionIndex()==3 ) {  // mpeg version 1
        if( FrameHeader_getModeIndex()==3 ) pos += 17; // Single Channel
        else                                pos += 32;
    } else {                             // mpeg version 2 or 2.5
        if( FrameHeader_getModeIndex()==3 ) pos +=  9; // Single Channel
        else                                pos += 17;
    }

    // read next twelve bits in
    if ( fseek ( fp, pos, SEEK_SET ) != 0 )
        return 1;
    if ( fread ( &vbrchars, 1, 12, fp ) != 12 )
        return 1;

    // turn 12 chars into a CVBitRate class structure
    MP3info.VBitRate = VBitRate_loadHeader(vbrchars);

    return 0;
}


double /*int*/ MP3Info_getBitrate()
{
    if (MP3info.VBitRate) {
        float medFrameSize = (float)MP3info.fileSize / (float)MP3Info_getNumberOfFrames();
        //return (int)(( medFrameSize * (float)FrameHeader_getFrequency() ) / ( 1000.0 * ( (FrameHeader_getLayerIndex()==3) ? 12.0 : 144.0)));
        return ( medFrameSize * (float)FrameHeader_getFrequency() ) / ( 1000.0 * ((FrameHeader_getLayerIndex()==3) ? 12.0 : 144.0) );
    } else {
        return FrameHeader_getBitrate();
    }
}

int MP3Info_getLengthInSeconds()
{
    int kiloBitFileSize = (8 * MP3info.fileSize) / 1000;
    return (int)(kiloBitFileSize/MP3Info_getBitrate());
}

int MP3Info_getNumberOfFrames()
{
    if (!MP3info.VBitRate) {
        float medFrameSize = (float)(( (FrameHeader_getLayerIndex()==3) ? 12 : 144 ) * ((1000.0 * (float)FrameHeader_getBitrate() ) / (float)FrameHeader_getFrequency()));
        return (int)(MP3info.fileSize/medFrameSize);
    } else {
        return VBitRate_getNumberOfFrames();
    }
}

void MP3Info_getVersion(char* input)
{
    char versionchar[32]; // temporary string
    char tempchar2[4]; // layer

    // call CFrameHeader member function
    float ver = FrameHeader_getVersion();
    //int i;

    // create the layer information with the amounts of I
    /*
    for ( i = 0; i < FrameHeader_getLayer(); i++ ) tempchar2[i] = 'I';
    tempchar2[i] = '\0';
    */
    sprintf ( tempchar2, "%u", FrameHeader_getLayer() );

    // combine strings
    sprintf(versionchar,"MPEG %g Layer %s", (double)ver, tempchar2);

    // copy result into inputstring
    strcpy(input, versionchar);
}

void MP3Info_getMode(char* input)
{
    char modechar[32]; // temporary string

    // call CFrameHeader member function
    FrameHeader_getMode(modechar);

    // copy result into inputstring
    strcpy(input, modechar);
}


int ReadFileInfoMP3 ( FileInfo* Info )
{
    char    temp[1024];
    FILE*   fp;

    if ( (fp = fopen ( Info->Filename, "rb" )) == NULL ) {
        fprintf ( Options.output, "ReadFileInfoMP3: fopen failed.\n" );
        fprintf ( Options.output, "%u files closed\n", _fcloseall () );
        return 1;
    }

    if ( MP3Info_loadInfo ( fp ) != 0 ) {
        fprintf ( Options.output, "ReadFileInfoMP3: MP3Info_loadInfo failed.\n" );
        fclose (fp);
        return 1;
    }
    fclose (fp);

    MP3Info_getVersion ( Info->Details.Format );
    FrameHeader_getMode ( temp );
    strcat ( Info->Details.Format, ", "  );
    strcat ( Info->Details.Format, temp );

    Info->Details.BitRate       = (long)MP3Info_getBitrate ();
    Info->Details.Duration      = MP3Info_getLengthInSeconds ();
    Info->Details.SampleRate    = MP3Info_getFrequency ();
    Info->Details.Format[sizeof (Info->Details.Format)-1] = '\0';

    if ( MP3info.VBitRate ) {   // VBR
        sprintf ( Info->Details.Quality, "VBR %u", Info->Details.BitRate );
    } else {
        sprintf ( Info->Details.Quality, "CBR %u", Info->Details.BitRate );
    }

    if ( strnicmp ( temp, "single", 6 ) == 0 )  Info->Details.Channels = 1;
    else                                        Info->Details.Channels = 2;

    return 0;
}
