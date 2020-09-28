/**********************************************************************

Filename    :   GJPEGUtil.h
Content     :   JPEG image I/O utility classes
Created     :   June 24, 2005
Authors     :   

Notes       :   
History     :   

Copyright   :   (c) 1998-2006 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef INC_GJPEGUTIL_H
#define INC_GJPEGUTIL_H

#include "GTypes.h"
#include "GMemory.h"


// ***** Declared Classes
class GJPEGInput;
class GJPEGOutput;

// ***** External Classes
class GFile;



class GJPEGInput : public GNewOverrideBase
{
public:

    // Read header and create a jpeg input object.
    // If GFC_USE_LIBJPEG is not defined, Create will return 0
    static GJPEGInput*  GSTDCALL Create(GFile* pin);

//  // Read SWF JPEG2-style Header (separate encoding
//  // table followed by image data), and create jpeg
//  // input object.
//  static input*   GSTDCALL CreateSwfJpeg2(SDLRWops* in);

    static GJPEGInput*  GSTDCALL CreateSwfJpeg2HeaderOnly(GFile* pin);
    static GJPEGInput*  GSTDCALL CreateSwfJpeg2HeaderOnly(const UByte* pbuffer, size_t bufSize);
    virtual ~GJPEGInput();

    virtual void    DiscardPartialBuffer()  = 0;
    virtual int     StartImage()            = 0;
    virtual int     StartRawImage()         = 0;
    virtual int     FinishImage()           = 0;

    virtual UInt    GetHeight() const       = 0;
    virtual UInt    GetWidth() const        = 0;
    virtual int     ReadScanline(unsigned char* prgbData) = 0;
    virtual int     ReadRawData(void** pprawData) = 0;

    virtual void*   GetCInfo()              = 0; // returns jpeg_decompress_struct*

    virtual bool    IsErrorOccurred() const = 0;
};


// Helper object for writing jpeg image data.
class GJPEGOutput : public GNewOverrideBase
{
public:
    // Create an output object.   Quality goes from 1-100.
    // If GFC_USE_LIBJPEG is not defined, Create will return 0
    static GJPEGOutput* GSTDCALL Create(GFile* pout, int width, int height, int quality);
    static GJPEGOutput* GSTDCALL Create(GFile* pout);

    virtual ~GJPEGOutput();

    // ...
    virtual void    WriteScanline(unsigned char* prgbData)      = 0;
    virtual void    WriteRawData(const void* prawData)          = 0;
    virtual void    CopyCriticalParams(void* pSrcDecompressInfo)= 0;
    virtual void*   GetCInfo()           = 0;  // returns jpeg_compress_struct*
};


#endif // INC_GJPEGUTIL_H
