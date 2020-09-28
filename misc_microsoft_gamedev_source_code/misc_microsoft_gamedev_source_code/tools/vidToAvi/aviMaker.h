// File: aviMaker.h
#pragma once

#include "rgbaImage.h"
#include "vfw.h"

class BAVIMaker
{
public:
   BAVIMaker();
   ~BAVIMaker();

   // period is in seconds.
   bool                    init(HWND parentHWnd, const char *filename, DWORD width, DWORD height, float period);

   bool                    addFrame(void *data);         // this expects a width*height*24bpp blob of data
   bool                    addFrame(BRGBAImage *image);

   float                   getPeriod(void) const {return(mPeriod);}

   BYTE                    *getTempBuffer(void) {return(mTempBuffer);}

   DWORD                   getWidth(void) const {return(mWidth);}
   DWORD                   getHeight(void) const {return(mHeight);}

protected:
   BAVIMaker(const BAVIMaker&);
   BAVIMaker& operator= (const BAVIMaker&);

   void                    cleanup(void);

   IAVIFile                *mAVIFile;
   WAVEFORMATEX            mWaveFormat;
   float                   mPeriod;
   DWORD                   mWidth;
   DWORD                   mHeight;
   
   IAVIStream              *mVideoStream;
   IAVIStream              *mVideoStreamCompressed;
   DWORD                   mNextFrame;
   DWORD                   mNextSample;
   BYTE                    *mTempBuffer;
};
