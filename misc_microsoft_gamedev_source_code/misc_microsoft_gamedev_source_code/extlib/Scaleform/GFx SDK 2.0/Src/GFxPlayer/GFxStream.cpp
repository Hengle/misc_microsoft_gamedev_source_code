/**********************************************************************

Filename    :   GFxStream.cpp
Content     :   Byte/bit packed stream used for reading SWF files.
Created     :   
Authors     :   

Copyright   :   (c) 2001-2006 Scaleform Corp. All Rights Reserved.

Notes       :   wrapper class, for loading variable-length data from a
                GFxStream, and keeping track of SWF tag boundaries.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/


#include "GFile.h"
#include "GFxStream.h"
#include "GFxLoader.h"
#include "GFxLog.h"

#include <string.h>


GFxStream::GFxStream(GFile* pinput, GFxLog *plog, GFxParseControl *pparseControl)   
{
    pInput      = pinput;
    pLog        = plog;
    pParseControl = pparseControl;
    ParseFlags  = pparseControl ? pparseControl->GetParseFlags() : 0;
    CurrentByte = 0;
    UnusedBits  = 0;
    
    TagStackEntryCount = 0;
    TagStack[0] = 0;
    TagStack[1] = 0;

    Pos         = 0;
    DataSize    = 0;
    FilePos     = pinput ? pinput->Tell() : 0;
}

GFxStream::~GFxStream()
{
}


void    GFxStream::Initialize(GFile* pinput, GFxLog *plog, GFxParseControl *pparseControl)
{    
    pInput      = pinput;
    pLog        = plog;
    pParseControl = pparseControl;
    ParseFlags  = pparseControl ? pparseControl->GetParseFlags() : 0;
    CurrentByte = 0;
    UnusedBits  = 0;

    TagStackEntryCount = 0;
    TagStack[0] = 0;
    TagStack[1] = 0;

    Pos         = 0;
    DataSize    = 0;
    FilePos     = pinput ? pinput->Tell() : 0;
}

    
// Reads a bit-packed unsigned integer from the GFxStream
// and returns it.  The given bitcount determines the
// number of bits to read.
UInt    GFxStream::ReadUInt(UInt bitcount)
{
    GASSERT(bitcount <= 32);
        
    UInt32  value      = 0;
    int     bitsNeeded = bitcount;

    while (bitsNeeded > 0)
    {
        if (UnusedBits) 
        {
            if (bitsNeeded >= UnusedBits) 
            {
                // Consume all the unused bits.
                value |= (CurrentByte << (bitsNeeded - UnusedBits));

                bitsNeeded -= UnusedBits;                
                UnusedBits = 0;
            } 
            else 
            {
                int ubits = UnusedBits - bitsNeeded;

                // Consume some of the unused bits.
                value |= (CurrentByte >> ubits);
                // mask off the bits we consumed.
                CurrentByte &= ((1 << ubits) - 1);
                
                // We're done.
                UnusedBits = (UByte)ubits;
                bitsNeeded = 0;
            }
        } 
        else 
        {
            CurrentByte = ReadU8();
            UnusedBits  = 8;
        }
    }

    GASSERT(bitsNeeded == 0);

    return value;
}


UInt    GFxStream::ReadUInt1()
{    
    UInt32  value;
   
    if (UnusedBits == 0)
    {
        CurrentByte = ReadU8();
        UnusedBits  = 7;
        value       = (CurrentByte >> 7);
        CurrentByte &= ((1 << 7) - 1);
    }
    else
    {
        UnusedBits--;
        // Consume some of the unused bits.
        value       = (CurrentByte >> UnusedBits);
        CurrentByte &= ((1 << UnusedBits) - 1);
    }
    
    return value;
}


// Reads a bit-packed little-endian signed integer
// from the GFxStream.  The given bitcount determines the
// number of bits to read.
SInt    GFxStream::ReadSInt(UInt bitcount)
{
    GASSERT(bitcount <= 32);

    SInt32  value = (SInt32) ReadUInt(bitcount);

    // Sign extend...
    if (value & (1 << (bitcount - 1)))
    {
        value |= -1 << bitcount;
    }

    return value;
}


// Makes data available in buffer. Stores zeros if not available.
void    GFxStream::PopulateBuffer(SInt size)
{
    GUNUSED(size);

    if (DataSize == 0)
    {
        // In case Underlying file position was changed.
        FilePos = pInput->Tell();
    }

    // move data to front
    if (Pos < DataSize)
    {
        memmove(Buffer, Buffer + Pos, DataSize - Pos);
        DataSize = DataSize - Pos;
        Pos      = 0;        
    }
    else if (Pos >= DataSize)
    {
        DataSize = 0;
        Pos      = 0;
    }
    
    SInt readSize = Stream_BufferSize - (SInt)DataSize;
    SInt bytes    = pInput->Read(Buffer + DataSize, readSize);

    if (bytes < readSize)
    {
        if (bytes > 0)
        {
            DataSize += (UInt) bytes;
            FilePos  += (UInt) bytes;
        }
        // Did not read enough. Set to zeros - error case.
        memset(Buffer + DataSize, 0, Stream_BufferSize - DataSize);
    }
    else
    {
        DataSize += (UInt) bytes;
        FilePos  += (UInt) bytes;
    } 

    GASSERT(DataSize <= Stream_BufferSize );
}

void    GFxStream::PopulateBuffer1()
{
    // Non-inline to simplify the call in ReadU8.
    PopulateBuffer(1);
}

// Reposition the underlying file to the desired location
void    GFxStream::SyncFileStream()
{
    SInt pos = pInput->Seek(Tell());
    if (pos != -1)
    {
        Pos     = 0;
        FilePos = (UInt) pos;
        DataSize= 0;
    }   
}


// Reads *and GALLOC()'s* the string from the given file.
// Ownership passes to the caller; caller must delete[] the
// string when it is done with it.
char*   GFxStream::ReadString()
{
    Align();

    GTL::garray<char>   buffer;
    char    c;
    while ((c = ReadU8()) != 0)
    {
        buffer.push_back(c);
    }
    buffer.push_back(0);

    if (buffer.size() == 0)
    {
        return NULL;
    }

    char*   retval = (char*)GALLOC(buffer.size());
    memcpy(retval, &buffer[0], buffer.size());

    return retval;
}


// Reads *and new[]'s* the string from the given file.
// Ownership passes to the caller; caller must delete[] the
// string when it is done with it.
char*   GFxStream::ReadStringWithLength()
{
    Align();

    int len = ReadU8();
    if (len <= 0)
    {
        return NULL;
    }
    else
    {
        char*   buffer = (char*)GALLOC(len + 1);
        int     i;
        for (i = 0; i < len; i++)        
            buffer[i] = ReadU8();
        buffer[i] = 0;  // terminate.

        return buffer;
    }
}




// Set the file position to the given value.
void    GFxStream::SetPosition(int pos)
{
    Align();

    // If we're in a tag, make sure we're not seeking outside the tag.
    if (TagStackEntryCount > 0)
    {
        GASSERT(pos <= GetTagEndPosition());
        // @@ check start pos somehow???
    }
    
    if (DataSize &&
        (pos >= (int)(FilePos - DataSize) && pos < (int)FilePos))
    {
        Pos = UInt(pos - FilePos + DataSize);     
    }
    else
    {
        // Do the seek (if already there, don't call system).
        // The (Tell() == pos) case happens frequently enough
        // in the end of the buffer.
        if ((Tell() == pos) || (pInput->Seek(pos) >= 0))
        {
            Pos         = 0;
            DataSize    = 0;
            FilePos     = pos;
        }
    }
}



// *** Tag handling.

// Return the tag type.
GFxTagType GFxStream::OpenTag(GFxTagInfo* pTagInfo)
{
    Align();
    int tagOffset   = Tell();
    int tagHeader   = ReadU16();
    int tagType     = tagHeader >> 6;
    int tagLength   = tagHeader & 0x3F;
    
    GASSERT(UnusedBits == 0);
    if (tagLength == 0x3F)
        tagLength = ReadS32();
    
    pTagInfo->TagOffset     = tagOffset;
    pTagInfo->TagType       = (GFxTagType)tagType;
    pTagInfo->TagLength     = tagLength;
    pTagInfo->TagDataOffset = Tell();

    if (IsVerboseParse())
        LogParse("---------------Tag type = %d, Tag length = %d\n", tagType, tagLength);
        
    // Remember where the end of the tag is, so we can
    // fast-forward past it when we're done reading it.
    GASSERT(TagStackEntryCount < Stream_TagStackSize);
    TagStack[TagStackEntryCount] = Tell() + tagLength;
    TagStackEntryCount++;
    
    return (GFxTagType)tagType;
}

// Simplified OpenTag - no info reported back.
GFxTagType GFxStream::OpenTag()
{
    Align();
    int tagHeader   = ReadU16();
    int tagType     = tagHeader >> 6;
    int tagLength   = tagHeader & 0x3F;

    GASSERT(UnusedBits == 0);
    if (tagLength == 0x3F)
        tagLength = ReadS32();
    
    if (IsVerboseParse())
        LogParse("---------------Tag type = %d, Tag length = %d\n", tagType, tagLength);

    // Remember where the end of the tag is, so we can
    // fast-forward past it when we're done reading it.
    GASSERT(TagStackEntryCount < Stream_TagStackSize);
        TagStack[TagStackEntryCount] = Tell() + tagLength;
    TagStackEntryCount++;

    return (GFxTagType)tagType;
}

// Seek to the end of the most-recently-opened tag.
void    GFxStream::CloseTag()
{
    GASSERT(TagStackEntryCount > 0);
    TagStackEntryCount--;
    SetPosition(TagStack[TagStackEntryCount]);
    UnusedBits = 0;
}

// Return the file position of the end of the current tag.
int GFxStream::GetTagEndPosition()
{
    // Bounds check - assumes UInt.
    GASSERT((TagStackEntryCount-1) < Stream_TagStackSize); 
    return ((TagStackEntryCount-1) < Stream_TagStackSize) ?
           TagStack[TagStackEntryCount-1] : 0;
}



// *** Reading SWF data types

// Loading functions
void    GFxStream::ReadMatrix(GRenderer::Matrix *pm)
{
    Align();
    pm->SetIdentity();

    int hasScale = ReadUInt1();
    if (hasScale)
    {
        UInt    ScaleNbits = ReadUInt(5);
        pm->M_[0][0] = ReadSInt(ScaleNbits) / 65536.0f;
        pm->M_[1][1] = ReadSInt(ScaleNbits) / 65536.0f;
    }
    int hasRotate = ReadUInt1();
    if (hasRotate)
    {
        UInt    rotateNbits = ReadUInt(5);
        pm->M_[1][0] = ReadSInt(rotateNbits) / 65536.0f;
        pm->M_[0][1] = ReadSInt(rotateNbits) / 65536.0f;
    }

    int translateNbits = ReadUInt(5);
    if (translateNbits > 0)
    {
        pm->M_[0][2] = (Float) ReadSInt(translateNbits);
        pm->M_[1][2] = (Float) ReadSInt(translateNbits);
    }
}

void    GFxStream::ReadCxformRgb(GRenderer::Cxform *pcxform)
{
    Align();
    UInt    hasAdd = ReadUInt1();
    UInt    hasMult = ReadUInt1();
    UInt    nbits = ReadUInt(4);

    if (hasMult)
    {
        // The divisor value must be 256,
        // since multiply factor 1.0 has value 0x100, not 0xFF
        pcxform->M_[0][0] = ReadSInt(nbits) / 256.0f;
        pcxform->M_[1][0] = ReadSInt(nbits) / 256.0f;
        pcxform->M_[2][0] = ReadSInt(nbits) / 256.0f;
        pcxform->M_[3][0] = 1.0f;
    }
    else
    {
        for (UInt i = 0; i < 4; i++) { pcxform->M_[i][0] = 1.0f; }
    }
    if (hasAdd)
    {
        pcxform->M_[0][1] = (Float) ReadSInt(nbits);
        pcxform->M_[1][1] = (Float) ReadSInt(nbits);
        pcxform->M_[2][1] = (Float) ReadSInt(nbits);
        pcxform->M_[3][1] = 1.0f;
    }
    else
    {
        for (UInt i = 0; i < 4; i++) { pcxform->M_[i][1] = 0.0f; }
    }
}

void    GFxStream::ReadCxformRgba(GRenderer::Cxform *pcxform)
{
    Align();

    UInt    hasAdd = ReadUInt1();
    UInt    hasMult = ReadUInt1();
    UInt    nbits = ReadUInt(4);

    if (hasMult)
    {
        // The divisor value must be 256,
        // since multiply factor 1.0 has value 0x100, not 0xFF
        pcxform->M_[0][0] = ReadSInt(nbits) / 256.0f;
        pcxform->M_[1][0] = ReadSInt(nbits) / 256.0f;
        pcxform->M_[2][0] = ReadSInt(nbits) / 256.0f;
        pcxform->M_[3][0] = ReadSInt(nbits) / 256.0f;
    }
    else
    {
        for (int i = 0; i < 4; i++) { pcxform->M_[i][0] = 1.0f; }
    }
    if (hasAdd)
    {
        pcxform->M_[0][1] = (Float) ReadSInt(nbits);
        pcxform->M_[1][1] = (Float) ReadSInt(nbits);
        pcxform->M_[2][1] = (Float) ReadSInt(nbits);
        pcxform->M_[3][1] = (Float) ReadSInt(nbits);
    }
    else
    {
        for (int i = 0; i < 4; i++) { pcxform->M_[i][1] = 0.0f; }
    }       
}


void    GFxStream::ReadRect(GRectF *pr)
{
    Align();
    int nbits = ReadUInt(5);
    pr->Left    = (Float) ReadSInt(nbits);
    pr->Right   = (Float) ReadSInt(nbits);
    pr->Top     = (Float) ReadSInt(nbits);
    pr->Bottom  = (Float) ReadSInt(nbits);
}


void    GFxStream::ReadPoint(GPointF *ppt)
{
    GUNUSED(ppt);
    GFC_DEBUG_ERROR(1, "GFxStream::ReadPoint not implemented");
}


// Color
void    GFxStream::ReadRgb(GColor *pc)
{
    // Read: R, G, B
    pc->SetRed(ReadU8());
    pc->SetGreen(ReadU8());
    pc->SetBlue(ReadU8());
    pc->SetAlpha(0xFF); 
}

void    GFxStream::ReadRgba(GColor *pc)
{
    // Read: RGB, A
    ReadRgb(pc);
    pc->SetAlpha(ReadU8()); 
}




// GFxLogBase impl
GFxLog* GFxStream::GetLog() const
    { return pLog; }
bool    GFxStream::IsVerboseParse() const
    { return (ParseFlags & GFxParseControl::VerboseParse) != 0; }
bool    GFxStream::IsVerboseParseShape() const
    { return (ParseFlags & GFxParseControl::VerboseParseShape) != 0; }
bool    GFxStream::IsVerboseParseMorphShape() const
    { return (ParseFlags & GFxParseControl::VerboseParseMorphShape) != 0; }
bool    GFxStream::IsVerboseParseAction() const
    { return (ParseFlags & GFxParseControl::VerboseParseAction) != 0; }


    
// *** Helper functions to log contents or required types

#ifndef GFC_NO_FXPLAYER_VERBOSE_PARSE

#define GFX_STREAM_BUFFER_SIZE  512

void    GFxStream::LogParseClass(GColor color)
{
    char buff[GFX_STREAM_BUFFER_SIZE];
    color.Format(buff);
    LogParse(buff);
}

void    GFxStream::LogParseClass(const GRectF &rc)
{
    char buff[GFX_STREAM_BUFFER_SIZE];


    gfc_sprintf(buff, GFX_STREAM_BUFFER_SIZE,
        "xmin = %g, ymin = %g, xmax = %g, ymax = %g\n",
        TwipsToPixels(rc.Left),
        TwipsToPixels(rc.Top),
        TwipsToPixels(rc.Right),
        TwipsToPixels(rc.Bottom));

    LogParse(buff);
}

void    GFxStream::LogParseClass(const GRenderer::Cxform &cxform)
{
    char buff[GFX_STREAM_BUFFER_SIZE];
    cxform.Format(buff);
    LogParse(buff);
}
void    GFxStream::LogParseClass(const GRenderer::Matrix &matrix)
{
    char buff[GFX_STREAM_BUFFER_SIZE];
    matrix.Format(buff);
    LogParse(buff);
}


// Log the contents of the current tag, in hex.
void    GFxStream::LogTagBytes()
{
    static const int    ROW_BYTES = 16;

    char    rowBuf[ROW_BYTES];
    int     rowCount = 0;

    while(Tell() < GetTagEndPosition())
    {
        UInt    c = ReadU8();
        LogParse("%02X", c);

        if (c < 32) c = '.';
        if (c > 127) c = '.';
        rowBuf[rowCount] = (UByte)c;
        
        rowCount++;
        if (rowCount >= ROW_BYTES)
        {
            LogParse("    ");
            for (int i = 0; i < ROW_BYTES; i++)
            {
                LogParse("%c", rowBuf[i]);
            }

            LogParse("\n");
            rowCount = 0;
        }
        else
        {
            LogParse(" ");
        }
    }

    if (rowCount > 0)
    {
        LogParse("\n");
    }
}

#endif
