/**********************************************************************

Filename    :   GFxStream.h
Content     :   SWF (Shockwave Flash) player library
Created     :   July 7, 2005
Authors     :   
Copyright   :   (c) 1998-2006 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/


#ifndef INC_GFXSTREAM_H
#define INC_GFXSTREAM_H

#include "GTLTypes.h"
// Include Renderer to load up its data types
#include "GRenderer.h"
// Must include log for proper inheritance
#include "GFxLog.h"

#include "GFxTags.h"

// ***** Declared Classes
class GFxStream;

// ***** External Classes
class GFile;
class GColor;
class GFxParseControl;


struct GFxTagInfo
{
    GFxTagType  TagType;
    int         TagOffset;
    int         TagLength;
    int         TagDataOffset;
};

// Stream is used to encapsulate bit-packed file reads.
class GFxStream : public GFxLogBase<GFxStream>
{
public:
    
    GFxStream(GFile* pinput, GFxLog *plog, GFxParseControl *pparseControl);
    GFxStream(const UByte* pbuffer, UInt bufSize, GFxLog *plog, GFxParseControl *pparseControl);
    ~GFxStream();

    // Re-initializes the stream, similar to opening it
    // from the constructor. Any unused bits are discarded.
    void        Initialize(GFile* pinput, GFxLog *plog, GFxParseControl *pparseControl);



    // *** Stream I/O interface

    // The following routines are used very frequently,
    // so they should be optimized as much as possible.
    // This includes:
    //   - Align / ReadUInt(bits) / ReadSInt(Bits)  - used for packed structures.
    //   - ReadU8/U16/U32/S8/S16/S32                - used for member fields.
    //   - Tell / SetPosition / GetTagPosition      - used for seeking / positioning.
    //   - OpenTag / CloseTag                       - used for tags.

    void            Align()  { UnusedBits = 0; }

    UInt            ReadUInt(UInt bitcount);
    SInt            ReadSInt(UInt bitcount);
    // Special cases for frequently used bit counts.
    UInt            ReadUInt1();
    //UInt            ReadUInt4();
    //UInt            ReadUInt5();

    GINLINE UByte   ReadU8();
    GINLINE SInt8   ReadS8();
    GINLINE UInt16  ReadU16(); 
    GINLINE SInt16  ReadS16();
    GINLINE UInt32  ReadU32();
    GINLINE SInt32  ReadS32();
    GINLINE Float   ReadFloat();

    GINLINE bool    ReadU32(UInt32*);


    // Seeking / positioning.
    GINLINE int     Tell();
    int             GetTagEndPosition();
    void            SetPosition(int pos);      

    // Tag access; there is a max 2-entry tag stack.    
    GFxTagType      OpenTag();
    GFxTagType      OpenTag(GFxTagInfo* ptagInfo);
    void            CloseTag();

    // Special reader for fill/line styles.
    GINLINE UInt    ReadVariableCount();

    // Obtain file interface for the current location.
    GINLINE GFile*  GetUnderlyingFile();
 

    // For null-terminated string.
    // reads *and GALLOC()'s* the string -- ownership passes to caller!
    char*           ReadString();   
    // For string that begins with an 8-bit length code.
    // reads *and GALLOCS()'s* the string -- ownership passes to caller!
    char*           ReadStringWithLength();     

    // Reposition the underlying file to the current stream location
    void            SyncFileStream();

    GFxString& GetFileName() {return FileName;}

    // *** Reading SWF data types

    // Loading functions
    void    ReadMatrix(GRenderer::Matrix *pm);
    void    ReadCxformRgb(GRenderer::Cxform *pcxform);
    void    ReadCxformRgba(GRenderer::Cxform *pcxform);
    void    ReadRect(GRectF *pr);
    void    ReadPoint(GPointF *ppt);
    // Color
    void    ReadRgb(GColor *pc);
    void    ReadRgba(GColor *pc);    
    
    void    ReadToBuffer(UByte* pdestBuf, UInt sz);

    // *** Delegated Logging Support 

    // GFxLogBase will output log messages to the appropriate logging stream,
    // but only if the enable option in context is set. 
    GFxLog* GetLog() const;
    bool    IsVerboseParse() const;
    bool    IsVerboseParseShape() const;
    bool    IsVerboseParseMorphShape() const;
    bool    IsVerboseParseAction() const;
    
#ifdef GFC_NO_FXPLAYER_VERBOSE_PARSE    
    void    LogTagBytes() { }

    void    LogParseClass(GColor color) { GUNUSED(color); }
    void    LogParseClass(const GRectF &rect) { GUNUSED(rect); }
    void    LogParseClass(const GRenderer::Cxform &cxform) { GUNUSED(cxform); }
    void    LogParseClass(const GRenderer::Matrix &matrix) { GUNUSED(matrix); }
#else   
    // Log the contents of the current tag, in hex.
    void    LogTagBytes();

    // *** Helper functions to log contents or required types
    void    LogParseClass(GColor color);
    void    LogParseClass(const GRectF &rect);
    void    LogParseClass(const GRenderer::Cxform &cxform);
    void    LogParseClass(const GRenderer::Matrix &matrix);
#endif

    GFxParseControl*      GetParseControl() const { return pParseControl; }

private:

    // Logging context and parse message control.
    GFxLog*             pLog;
    GFxParseControl*    pParseControl;
    // Cached GFxParseControl flags from the states for more efficient checks.
    UInt                ParseFlags;


    // File used for input
    GPtr<GFile>         pInput;
    // Bytes used for bit I/O.
    UByte               CurrentByte;
    UByte               UnusedBits;
    GFxString           FileName;

    enum {
        Stream_TagStackSize = 2,
        Stream_BufferSize   = 512,
    };

    // *** Tag stack
   
    int             TagStack[Stream_TagStackSize];
    UInt            TagStackEntryCount;  // # of entries in tag stack.
    
    // *** File Buffering        
    UInt            Pos;        // Position in buffer
    UInt            DataSize;   // Data in buffer if reading
    UInt            FilePos;    // Underlying file position 
    UByte*          pBuffer;
    UInt            BufferSize;
    UByte           BuiltinBuffer[Stream_BufferSize];

    // Buffer initialization.
    GINLINE bool    EnsureBufferSize1();
    // Return true, if the requested "size" was actually read from the stream
    GINLINE bool    EnsureBufferSize(SInt size);
    // Makes data available in buffer. Stores zeros if not available.
    bool            PopulateBuffer(SInt size); // returns true, if all requested bytes were read.
    bool            PopulateBuffer1();         // returns true, if byte was read from the stream
};



// *** Inlines

// Buffer initialization.
bool    GFxStream::EnsureBufferSize1()
{
    Align();
    if (((SInt)DataSize-(SInt)Pos) < 1)
        return PopulateBuffer1();
    return true;
}

bool    GFxStream::EnsureBufferSize(SInt size)
{
    Align();
    if (((SInt)DataSize-(SInt)Pos) < size)
        return PopulateBuffer(size);
    return true;
}

// Unsigned reads - access the buffer directly.
UByte       GFxStream::ReadU8()
{        
    EnsureBufferSize1();
    return pBuffer[Pos++];
}

UInt16      GFxStream::ReadU16()
{        
    EnsureBufferSize(sizeof(UInt16));
    UInt16 val = (UInt16(pBuffer[Pos])) | (UInt16(pBuffer[Pos+1]<<8));
    Pos += sizeof(UInt16);
    return val;
}

UInt32      GFxStream::ReadU32()
{
    EnsureBufferSize(sizeof(UInt32));
    UInt32 val = (UInt32(pBuffer[Pos])) | (UInt32(pBuffer[Pos+1])<<8) |
                 (UInt32(pBuffer[Pos+2])<<16) | (UInt32(pBuffer[Pos+3])<<24);
    Pos += sizeof(UInt32);
    return val;
}

bool       GFxStream::ReadU32(UInt32* pdest)
{
    if (!EnsureBufferSize(sizeof(UInt32)))
        return false;
    UInt32 v = ReadU32();
    if (pdest)
        *pdest = v;
    return true;
}

Float       GFxStream::ReadFloat()
{
    EnsureBufferSize(sizeof(Float));
    //AB: for XBox360 and PS3 Float should be aligned on boundary of 4!
    GCOMPILER_ASSERT(sizeof(Float) == sizeof(UInt32));
    // Go through a union to avoid pointer strict aliasing problems.
    union {
        UInt32 ival;
        Float  fval;
    };
    ival = (UInt32(pBuffer[Pos])) | (UInt32(pBuffer[Pos+1])<<8) |
           (UInt32(pBuffer[Pos+2])<<16) | (UInt32(pBuffer[Pos+3])<<24);
    Pos += sizeof(Float);
    return fval;
}

SInt8       GFxStream::ReadS8()
{
    return (SInt8) ReadU8();
}
SInt16      GFxStream::ReadS16()
{
    return (SInt16) ReadU16();
}
SInt32      GFxStream::ReadS32()
{
    return (SInt32) ReadU32();
}

UInt        GFxStream::ReadVariableCount()
{
    UInt count = ReadU8();
    if (count == 0xFF)
        count = ReadU16();
    return count;
}

// Seeking / positioning.
int         GFxStream::Tell()
{
    return FilePos - DataSize + Pos;
}

GFile*      GFxStream::GetUnderlyingFile()
{
    SyncFileStream();
    return pInput;
}


#endif // INC_GFXSTREAM_H
