		
#ifndef __WAVE_H__
#define __WAVE_H__


struct riff_struct
{
  char id[4];   // RIFF 
  unsigned long len;
  char wave_id[4]; // WAVE
};


struct chunk_struct
{
        char id[4];
        unsigned long len;
};

struct common_struct
{
        unsigned short wFormatTag;
        unsigned short wChannels;
        unsigned long dwSamplesPerSec;
        unsigned long dwAvgBytesPerSec;
        unsigned short wBlockAlign;
        unsigned short wBitsPerSample;  // Only for PCM 
};

struct wave_header
{
        struct riff_struct   riff;
        struct chunk_struct  format;
        struct common_struct common;
        struct chunk_struct  data;
};


#endif 
