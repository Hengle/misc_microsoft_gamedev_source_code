/**********************************************************************

Filename    :   GFile_sys.cpp
Content     :   GFile wrapper class implementation (Win32)

Created     :   April 5, 1999
Authors     :   Michael Antonov

History     :   8/27/2001 MA    Reworked file and directory interface

Copyright   :   (c) 1999-2006 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#define  GFILE_CXX

// Standard C library
#include <stdio.h>
#include <sys/stat.h>

// GKernel
#include "GFile.h"
//#include "GTextutil.h"
//#include "GFunctions.h"

#ifndef GFC_OS_WINCE
#include <errno.h>
#endif

#ifdef GFC_OS_WIN32
// GKernel/Win32
#include "GSysFile_sys.h"
#endif

#ifdef GFC_OS_WII
#include <revolution/os.h>
#endif

// unistd provides stat() function
#ifdef GFC_OS_PS3
#include <unistd.h>
#endif

#ifdef GFC_OS_PSP
#include <iofilemgr_stat.h>
#include <rtcsvc.h>
#endif

// ***** GFile interface


// ***** GUnopenedFile

// This is - a dummy file that fails on all calls.

class GUnopenedFile : public GFile
{
public:
    GUnopenedFile()  { }
    ~GUnopenedFile() { }

    virtual const char* GetFilePath()               { return 0; }

    // ** File Information
    virtual bool        IsValid()                   { return 0; }
    virtual bool        IsWritable()                { return 0; }
    //virtual bool      IsRecoverable()             { return 0; }
    // Return position / file size
    virtual SInt        Tell()                      { return 0; }
    virtual SInt64      LTell()                     { return 0; }
    virtual SInt        GetLength()                 { return 0; }
    virtual SInt64      LGetLength()                { return 0; }

//  virtual bool        Stat(GFileStats *pfs)       { return 0; }
    virtual SInt        GetErrorCode()              { return Error_FileNotFound; }

    // ** GFxStream implementation & I/O
    virtual SInt        Write(const UByte *pbuffer, SInt numBytes)      { return -1; GUNUSED2(pbuffer, numBytes); }
    virtual SInt        Read(UByte *pbuffer, SInt numBytes)             { return -1; GUNUSED2(pbuffer, numBytes); }
    virtual SInt        SkipBytes(SInt numBytes)                        { return 0;  GUNUSED(numBytes); }
    virtual SInt        BytesAvailable()                                { return 0; }
    virtual bool        Flush()                                         { return 0; }
    virtual SInt        Seek(SInt offset, SInt origin)                  { return -1; GUNUSED2(offset, origin); }
    virtual SInt64      LSeek(SInt64 offset, SInt origin)               { return -1; GUNUSED2(offset, origin); }

    virtual bool        ChangeSize(SInt newSize)                        { return 0;  GUNUSED(newSize); }
    virtual SInt        CopyFromStream(GFile *pstream, SInt byteSize)   { return -1; GUNUSED2(pstream, byteSize); }
    virtual bool        Close()                                         { return 0; }
    virtual bool        CloseCancel()                                   { return 0; }
};



// ***** GFILEFile - C streams file

#ifdef GFC_USE_SYSFILE

static int GFC_error ()
{
#if !defined(GFC_OS_WINCE) && !defined(GFC_CC_RENESAS)
    if (errno == ENOENT)
        return GFileConstants::Error_FileNotFound;
    else if (errno == EACCES || errno == EPERM)
        return GFileConstants::Error_Access;
    else if (errno == ENOSPC)
        return GFileConstants::Error_DiskFull;
    else
#endif
        return GFileConstants::Error_IOError;
};

// This is the simplest possible file implementation, it wraps around the descriptor
// This file is delegated to by GSysFile

class GFILEFile : public GFile
{
protected:

    // Allocated filename
    char*       pFileName;

    // File handle & open mode
    bool        Opened;
    FILE*       fs;
    SInt        OpenFlags;
    // Error code for last request
    SInt        ErrorCode;

    SInt        LastOp;

public:

    GFILEFile() { Opened = 0; pFileName =0; }
    // Initialize file by opening it
    GFILEFile(const char* pfileName, SInt flags, SInt Mode);

    ~GFILEFile()
    {
        if (Opened)
            Close();
        if (pFileName)
            GFREE(pFileName);
    }

    virtual const char* GetFilePath();

    // ** File Information
    virtual bool        IsValid();
    virtual bool        IsWritable();
    //virtual bool      IsRecoverable();
    // Return position / file size
    virtual SInt        Tell();
    virtual SInt64      LTell();
    virtual SInt        GetLength();
    virtual SInt64      LGetLength();

//  virtual bool        Stat(GFileStats *pfs);
    virtual SInt        GetErrorCode();

    // ** GFxStream implementation & I/O
    virtual SInt        Write(const UByte *pbuffer, SInt numBytes);
    virtual SInt        Read(UByte *pbuffer, SInt numBytes);
    virtual SInt        SkipBytes(SInt numBytes);
    virtual SInt        BytesAvailable();
    virtual bool        Flush();
    virtual SInt        Seek(SInt offset, SInt origin);
    virtual SInt64      LSeek(SInt64 offset, SInt origin);

    virtual bool        ChangeSize(SInt newSize);
    virtual SInt        CopyFromStream(GFile *pStream, SInt byteSize);
    virtual bool        Close();
    //virtual bool      CloseCancel();
};


// Initialize file by opening it
GFILEFile::GFILEFile(const char *pfileName, SInt flags, SInt mode)
{
    GUNUSED(mode);
    // Save data
    int nameLength = (int) strlen(pfileName);
    pFileName = (char*)GALLOC(nameLength+1);
    memcpy(pFileName, pfileName, nameLength+1);

    OpenFlags   = flags;

    // Open mode for file's open
    const char *omode = "rb";

    if (flags&Open_Truncate)
    {
        if (flags&Open_Read)
            omode = "w+b";
        else
            omode = "wb";
    }
    else if (flags&Open_Create)
    {
        if (flags&Open_Read)
            omode = "a+b";
        else
            omode = "ab";
    }
    else if (flags&Open_Write)
        omode = "r+b";

#ifdef GFC_OS_WIN32
    GFileUtilWin32::SysErrorModeDisabler disabler(pfileName);
#endif

#if defined(GFC_CC_MSVC) && (GFC_CC_MSVC >= 1400) && !defined(GFC_OS_WINCE)
    fopen_s(&fs, pFileName, omode);
#else
    fs = fopen(pFileName, omode);
#endif
#ifdef GFC_OS_WINCE
    if (fs)
        fseek(fs, 0, SEEK_SET);
#else
    if (fs)
        rewind (fs);
#endif
    Opened = (fs != NULL);
    // Set error code
    if (!Opened)
        ErrorCode = GFC_error();
    else
        ErrorCode = 0;
    LastOp = 0;
}



const char* GFILEFile::GetFilePath()
{
    return pFileName;
}


// ** File Information
bool    GFILEFile::IsValid()
{
    return Opened;
}
bool    GFILEFile::IsWritable()
{
    return IsValid() && (OpenFlags&Open_Write);
}
/*
bool    GFILEFile::IsRecoverable()
{
    return IsValid() && ((OpenFlags&GFC_FO_SAFETRUNC) == GFC_FO_SAFETRUNC);
}
*/

// Return position / file size
SInt    GFILEFile::Tell()
{
    int pos = ftell (fs);
    if (pos < 0)
        ErrorCode = GFC_error();
    return pos;
}

SInt64  GFILEFile::LTell()
{
    int pos = ftell(fs);
    if (pos < 0)
        ErrorCode = GFC_error();
    return pos;
}

SInt    GFILEFile::GetLength()
{
    SInt pos = Tell();
    if (pos >= 0)
    {
        Seek (0, Seek_End);
        SInt size = Tell();
        Seek (pos, Seek_Set);
        return size;
    }
    return -1;
}
SInt64  GFILEFile::LGetLength()
{
    return GetLength();
}

SInt    GFILEFile::GetErrorCode()
{
    return ErrorCode;
}

// ** GFxStream implementation & I/O
SInt    GFILEFile::Write(const UByte *pbuffer, SInt numBytes)
{
    if (LastOp && LastOp != Open_Write)
        fflush(fs);
    LastOp = Open_Write;
    int written = (int) fwrite(pbuffer, 1, numBytes, fs);
    if (written < numBytes)
        ErrorCode = GFC_error();
    return written;
}

SInt    GFILEFile::Read(UByte *pbuffer, SInt numBytes)
{
    if (LastOp && LastOp != Open_Read)
        fflush(fs);
    LastOp = Open_Read;
    int read = (int) fread(pbuffer, 1, numBytes, fs);
    if (read < numBytes)
        ErrorCode = GFC_error();

    return read;
}

// Seeks ahead to skip bytes
SInt    GFILEFile::SkipBytes(SInt numBytes)
{
    SInt64 pos    = LTell();
    SInt64 newPos = LSeek(numBytes, Seek_Cur);

    // Return -1 for major error
    if ((pos==-1) || (newPos==-1))
    {
        return -1;
    }
    //ErrorCode = ((NewPos-Pos)<numBytes) ? errno : 0;

    return SInt(newPos-(SInt)pos);
}

// Return # of bytes till EOF
SInt    GFILEFile::BytesAvailable()
{
    SInt64 pos    = LTell();
    SInt64 endPos = LGetLength();

    // Return -1 for major error
    if ((pos==-1) || (endPos==-1))
    {
        ErrorCode = GFC_error();
        return 0;
    }
    else
        ErrorCode = 0;

    return SInt(endPos-(SInt)pos);
}

// Flush file contents
bool    GFILEFile::Flush()
{
    return !fflush(fs);
}

SInt    GFILEFile::Seek(SInt offset, SInt origin)
{
    int newOrigin = 0;
    switch(origin)
    {
    case Seek_Set: newOrigin = SEEK_SET; break;
    case Seek_Cur: newOrigin = SEEK_CUR; break;
    case Seek_End: newOrigin = SEEK_END; break;
    }

    if (newOrigin == SEEK_SET && offset == Tell())
    return Tell();

    if (fseek (fs, offset, newOrigin))
        return -1;
    return (SInt)Tell();
}

SInt64  GFILEFile::LSeek(SInt64 offset, SInt origin)
{
    return Seek((SInt)offset,origin);
}

bool    GFILEFile::ChangeSize(SInt newSize)
{
    GUNUSED(newSize);
    GFC_DEBUG_WARNING(1, "GFile::ChangeSize not supported");
    ErrorCode = Error_IOError;
    return 0;
}

SInt    GFILEFile::CopyFromStream(GFile *pstream, SInt byteSize)
{
    UByte   buff[0x4000];
    SInt    count = 0;
    SInt    szRequest, szRead, szWritten;

    while (byteSize)
    {
        szRequest = (byteSize>0x4000) ? (byteSize>0x4000) : byteSize;

        szRead    = pstream->Read(buff, szRequest);
        szWritten = 0;
        if (szRead > 0)
            szWritten = Write(buff, szRead);

        count    += szWritten;
        byteSize -= szWritten;
        if (szWritten < szRequest)
            break;
    }
    return count;
}


bool    GFILEFile::Close()
{
    bool closeRet = !fclose(fs);

    if (!closeRet)
    {
        ErrorCode = GFC_error();
        return 0;
    }
    else
    {
        Opened    = 0;
        fs        = 0;
        ErrorCode = 0;
    }

    // Handle safe truncate
    /*
    if ((OpenFlags & GFC_FO_SAFETRUNC) == GFC_FO_SAFETRUNC)
    {
        // Delete original file (if it existed)
        DWORD oldAttributes = GFileUtilWin32::GetFileAttributes(FileName);
        if (oldAttributes!=0xFFFFFFFF)
            if (!GFileUtilWin32::DeleteFile(FileName))
            {
                // Try to remove the readonly attribute
                GFileUtilWin32::SetFileAttributes(FileName, oldAttributes & (~FILE_ATTRIBUTE_READONLY) );
                // And delete the file again
                if (!GFileUtilWin32::DeleteFile(FileName))
                    return 0;
            }

        // Rename temp file to real filename
        if (!GFileUtilWin32::MoveFile(TempName, FileName))
        {
            //ErrorCode = errno;
            return 0;
        }
    }
    */
    return 1;
}

/*
bool    GFILEFile::CloseCancel()
{
    bool closeRet = (bool)::CloseHandle(fd);

    if (!closeRet)
    {
        //ErrorCode = errno;
        return 0;
    }
    else
    {
        Opened    = 0;
        fd        = INVALID_HANDLE_VALUE;
        ErrorCode = 0;
    }

    // Handle safe truncate (delete tmp file, leave original unchanged)
    if ((OpenFlags&GFC_FO_SAFETRUNC) == GFC_FO_SAFETRUNC)
        if (!GFileUtilWin32::DeleteFile(TempName))
        {
            //ErrorCode = errno;
            return 0;
        }
    return 1;
}
*/

#endif // GFC_USE_SYSFILE


// ***** GBufferedFile

// Buffered file adds buffering to an existing file
// FILEBUFFER_SIZE defines the size of internal buffer, while
// FILEBUFFER_TOLERANCE controls the amount of data we'll effectively try to buffer
#ifdef GFC_OS_WII
#define FILEBUFFER_SIZE         8192
#else
#define FILEBUFFER_SIZE         (8192-8)
#endif
#define FILEBUFFER_TOLERANCE    4096

// ** Constructor/Destructor

// Hidden constructor
// Not supposed to be used
GBufferedFile::GBufferedFile() : GDelegatedFile(0)
{
    // WII OS requires file buffer to be 32-byte aligned.
    pBuffer     = (UByte*)GALLOC_ALIGN(FILEBUFFER_SIZE, 32);
    BufferMode  = NoBuffer;
    FilePos     = 0;
}

// Takes another file as source
GBufferedFile::GBufferedFile(GFile *pfile) : GDelegatedFile(pfile)
{
    // WII OS requires file buffer to be 32-byte aligned.
    pBuffer     = (UByte*)GALLOC_ALIGN(FILEBUFFER_SIZE, 32);
    BufferMode  = NoBuffer;
    FilePos     = pfile->LTell();
}


// Destructor
GBufferedFile::~GBufferedFile()
{
    // Flush in case there's data
    if (pFile)
        FlushBuffer();
    // Get rid of buffer
    if (pBuffer)
        GFREE_ALIGN(pBuffer);
}

/*
bool    GBufferedFile::VCopy(const GObject &source)
{
    if (!GDelegatedFile::VCopy(source))
        return 0;

    // Data members
    GBufferedFile *psource = (GBufferedFile*)&source;

    // Buffer & the mode it's in
    pBuffer         = psource->pBuffer;
    BufferMode      = psource->BufferMode;
    Pos             = psource->Pos;
    DataSize        = psource->DataSize;
    return 1;
}
*/

// Initializes buffering to a certain mode
bool    GBufferedFile::SetBufferMode(BufferModeType mode)
{
    if (!pBuffer)
        return 0;

    // Flush buffer
    if (mode!=BufferMode)
        FlushBuffer();

    // Can't set write mode if we can't write
    if ((mode==WriteBuffer) && (!pFile || !pFile->IsWritable()) )
        return 0;

    // And SetMode
    BufferMode = mode;
    Pos        = 0;
    DataSize   = 0;
    return 1;
}

// Flushes buffer
void    GBufferedFile::FlushBuffer()
{
    switch(BufferMode)
    {
        case WriteBuffer:
            // Write data in buffer
            FilePos += pFile->Write(pBuffer,Pos);
            Pos = 0;
            break;

        case ReadBuffer:
            // Seek back & reset buffer data
            if (((SInt)DataSize-(SInt)Pos)>0)
                FilePos = pFile->LSeek(-((SInt)DataSize-(SInt)Pos),SEEK_CUR);
            DataSize = 0;
            Pos      = 0;
            break;
        default:
            // not handled!
            break;
    }
}

// Reloads data for ReadBuffer
void    GBufferedFile::LoadBuffer()
{
    if (BufferMode==ReadBuffer)
    {
        // WARNING: Right now LoadBuffer() assumes the buffer's empty
        SInt sz  = pFile->Read(pBuffer,FILEBUFFER_SIZE);
        DataSize = sz<0 ? 0 : (UInt)sz;
        Pos      = 0;
        FilePos  += DataSize;
    }
}


// ** Overridden functions

// We override all the functions that can possibly
// require buffer mode switch, flush, or extra calculations

// Tell() requires buffer adjustment
SInt    GBufferedFile::Tell()
{
    if (BufferMode == ReadBuffer)
        return SInt(FilePos - DataSize + Pos);

    SInt pos = pFile->Tell();
    // Adjust position based on buffer mode & data
    if (pos!=-1)
    {
        switch(BufferMode)
        {
            case ReadBuffer:
                pos-=(DataSize-Pos);
                break;
            case WriteBuffer:
                pos+=Pos;
                break;
            default:
                break;
        }
    }
    return pos;
}
SInt64  GBufferedFile::LTell()
{
    if (BufferMode == ReadBuffer)
        return FilePos - DataSize + Pos;

    SInt64 pos = pFile->LTell();

    if (pos!=-1)
    {
        switch(BufferMode)
        {
            case ReadBuffer:
                pos-=(DataSize-Pos);
                break;
            case WriteBuffer:
                pos+=Pos;
                break;
            default:
                break;
        }
    }
    return pos;
}

SInt    GBufferedFile::GetLength()
{
    SInt len = pFile->GetLength();
    // If writing through buffer, file length may actually be bigger
    if ((len!=-1) && (BufferMode==WriteBuffer))
    {
        SInt currPos = pFile->Tell() + Pos;
        if (currPos>len)
            len = currPos;
    }
    return len;
}
SInt64  GBufferedFile::LGetLength()
{
    SInt64 len = pFile->LGetLength();
    // If writing through buffer, file length may actually be bigger
    if ((len!=-1) && (BufferMode==WriteBuffer))
    {
        SInt64 currPos = pFile->LTell() + Pos;
        if (currPos>len)
            len = currPos;
    }
    return len;
}

/*
bool    GBufferedFile::Stat(GFileStats *pfs)
{
    // Have to fix up length is stat
    if (pFile->Stat(pfs))
    {
        if (BufferMode==WriteBuffer)
        {
            SInt64 currPos = pFile->LTell() + Pos;
            if (currPos > pfs->Size)
            {
                pfs->Size   = currPos;
                // ??
                pfs->Blocks = (pfs->Size+511) >> 9;
            }
        }
        return 1;
    }
    return 0;
}
*/

SInt    GBufferedFile::Write(const UByte *psourceBuffer, SInt numBytes)
{
    if ( (BufferMode==WriteBuffer) || SetBufferMode(WriteBuffer))
    {
        // If not data space in buffer, flush
        if ((FILEBUFFER_SIZE-(SInt)Pos)<numBytes)
        {
            FlushBuffer();
            // If bigger then tolerance, just write directly
            if (numBytes>FILEBUFFER_TOLERANCE)
            {
                SInt sz = pFile->Write(psourceBuffer,numBytes);
                if (sz > 0)
                    FilePos += sz;
                return sz;
            }
        }

        // Enough space in buffer.. so copy to it
        memcpy(pBuffer+Pos, psourceBuffer, numBytes);
        Pos += numBytes;
        return numBytes;
    }
    SInt sz = pFile->Write(psourceBuffer,numBytes);
    if (sz > 0)
        FilePos += sz;
    return sz;
}

SInt    GBufferedFile::Read(UByte *pdestBuffer, SInt numBytes)
{
    if ( (BufferMode==ReadBuffer) || SetBufferMode(ReadBuffer))
    {
#ifdef GFC_OS_WII
        SInt totalRead = 0;
        while (numBytes > 0)
        {
            // Data in buffer... copy it
            if (((SInt)DataSize-(SInt)Pos)>=numBytes)
            {
                memcpy(pdestBuffer, pBuffer+Pos, numBytes);
                Pos += numBytes;
                return totalRead + numBytes;
            }

            // Not enough data in buffer, copy buffer
            SInt    readBytes = DataSize-Pos;
            memcpy(pdestBuffer, pBuffer+Pos, readBytes);
            numBytes    -= readBytes;
            pdestBuffer += readBytes;
            totalRead   += readBytes;
            Pos = DataSize;

            LoadBuffer();
            if (FilePos >= GetLength() && Pos == DataSize)
                return totalRead;
        }
        return totalRead;
#else
        // Data in buffer... copy it
        if (((SInt)DataSize-(SInt)Pos)>=numBytes)
        {
            memcpy(pdestBuffer, pBuffer+Pos, numBytes);
            Pos += numBytes;
            return numBytes;
        }

        // Not enough data in buffer, copy buffer
        SInt    readBytes = DataSize-Pos;
        memcpy(pdestBuffer, pBuffer+Pos, readBytes);
        numBytes    -= readBytes;
        pdestBuffer += readBytes;
        Pos = DataSize;

        // Don't reload buffer if more then tolerance
        // (No major advantage, and we don't want to write a loop)
        if (numBytes>FILEBUFFER_TOLERANCE)
        {
            numBytes = pFile->Read(pdestBuffer,numBytes);
            if (numBytes > 0)
                FilePos += numBytes;
            return readBytes + ((numBytes==-1) ? 0 : numBytes);
        }

        // Reload the buffer
        // WARNING: Right now LoadBuffer() assumes the buffer's empty
        LoadBuffer();
        if (((SInt)DataSize-(SInt)Pos) < numBytes)
            numBytes = DataSize-Pos;

        memcpy(pdestBuffer, pBuffer+Pos, numBytes);
        Pos += numBytes;
        return numBytes + readBytes;
#endif
    }
    SInt sz = pFile->Read(pdestBuffer,numBytes);
    if (sz > 0)
        FilePos += sz;
    return sz;
}


SInt    GBufferedFile::SkipBytes(SInt numBytes)
{
    SInt skippedBytes = 0;

    // Special case for skipping a little data in read buffer
    if (BufferMode==ReadBuffer)
    {
        skippedBytes = (((SInt)DataSize-(SInt)Pos) >= numBytes) ? numBytes : (DataSize-Pos);
        Pos          += skippedBytes;
        numBytes     -= skippedBytes;
    }

    if (numBytes)
    {
        numBytes = pFile->SkipBytes(numBytes);
        // Make sure we return the actual number skipped, or error
        if (numBytes!=-1)
        {
            skippedBytes += numBytes;
            FilePos += numBytes;
        }
        else if (skippedBytes <= 0)
            skippedBytes = -1;
    }
    return skippedBytes;
}

SInt    GBufferedFile::BytesAvailable()
{
    SInt available = pFile->BytesAvailable();
    // Adjust available size based on buffers
    switch(BufferMode)
    {
        case ReadBuffer:
            available += DataSize-Pos;
            break;
        case WriteBuffer:
            available -= Pos;
            if (available<0)
                available= 0;
            break;
        default:
            break;
    }
    return available;
}

bool    GBufferedFile::Flush()
{
    FlushBuffer();
    return pFile->Flush();
}

// Seeking could be optimized better..
SInt    GBufferedFile::Seek(SInt offset, SInt origin)
{
    if (origin == Seek_Cur && offset + Pos < DataSize)
    {
        Pos += offset;
        return SInt(FilePos - DataSize + Pos);
    }
    else if (origin == Seek_Set && UInt(offset) >= FilePos - DataSize && UInt(offset) < FilePos)
    {
        Pos = UInt(offset - FilePos + DataSize);
        return SInt(FilePos - DataSize + Pos);
    }

    FlushBuffer();
    FilePos = pFile->Seek(offset,origin);
    return SInt(FilePos);
}

SInt64  GBufferedFile::LSeek(SInt64 offset, SInt origin)
{
    if (origin == Seek_Cur && offset + Pos < DataSize)
    {
        Pos += SInt(offset);
        return FilePos - DataSize + Pos;
    }
    else if (origin == Seek_Set && offset >= SInt64(FilePos - DataSize) && offset < SInt64(FilePos))
    {
        Pos = UInt(offset - FilePos + DataSize);
        return FilePos - DataSize + Pos;
    }

    FlushBuffer();
    FilePos = pFile->LSeek(offset,origin);
    return FilePos;
}

bool    GBufferedFile::ChangeSize(SInt newSize)
{
    FlushBuffer();
    return pFile->ChangeSize(newSize);
}

SInt    GBufferedFile::CopyFromStream(GFile *pstream, SInt byteSize)
{
    // We can't rely on overridden Write()
    // because delegation doesn't override virtual pointers
    // So, just re-implement
    UByte   buff[0x4000];
    SInt    count = 0;
    SInt    szRequest, szRead, szWritten;

    while(byteSize)
    {
        szRequest = (byteSize>0x4000) ? (byteSize>0x4000) : byteSize;

        szRead    = pstream->Read(buff,szRequest);
        szWritten = 0;
        if (szRead > 0)
            szWritten = Write(buff,szRead);

        count   +=szWritten;
        byteSize-=szWritten;
        if (szWritten < szRequest)
            break;
    }
    return count;
}

// Closing files
bool    GBufferedFile::Close()
{
    switch(BufferMode)
    {
        case WriteBuffer:
            FlushBuffer();
            break;
        case ReadBuffer:
            // No need to seek back on close
            BufferMode = NoBuffer;
            break;
        default:
            break;
    }
    return pFile->Close();
}

/*
bool    GBufferedFile::CloseCancel()
{
    // If operation's being canceled
    // There's not need to flush buffers at all
    BufferMode = NoBuffer;
    return pFile->CloseCancel();
}
*/


#ifdef GFC_USE_SYSFILE

// ***** System File

// System file is created to access objects on file system directly
// This file can refer directly to path

// ** Constructor
GSysFile::GSysFile() : GDelegatedFile(0)
{
    pFile = *new GUnopenedFile;
}

// Opens a file
GSysFile::GSysFile(const char* ppath, SInt flags, SInt mode) : GDelegatedFile(0)
{
    pFile = *new GFILEFile(ppath,flags,mode);
    if (pFile && !pFile->IsValid())
    {
        pFile = *new GUnopenedFile;
    }
}


// ** Open & management
// Will fail if file's already open
bool    GSysFile::Open(const char *ppath, SInt flags, SInt mode)
{
    pFile = *new GFILEFile(ppath,flags,mode);
    if ((!pFile) || (!pFile->IsValid()))
    {
        pFile = *new GUnopenedFile;
        return 0;
    }
    return 1;
}

// Helper function: obtain file information time.
bool    GSysFile::GetFileStat(GFileStat* pfileStat, const char* ppath)
{
#if defined(GFC_OS_WII)
    GUNUSED2(pfileStat, ppath);
    return false;

#elif defined(GFC_OS_PSP)
    SceIoStat fileStat;
    // Stat returns 0 for success.
    if (sceIoGetstat(ppath, &fileStat) != 0)
        return false;

    pfileStat->FileSize = fileStat.st_size;
    time_t t;
    sceRtcGetTime_t(&fileStat.st_mtime, &t);
    pfileStat->ModifyTime = t;
    sceRtcGetTime_t(&fileStat.st_atime, &t);
    pfileStat->AccessTime = t;
    return true;

#else

#if defined(GFC_OS_WIN32)
    // 64-bit implementation on Windows.
    struct __stat64 fileStat;
    // Stat returns 0 for success.
    if (_stat64(ppath, &fileStat) != 0)
        return false;

#else
    struct stat fileStat;
    // Stat returns 0 for success.
    if (stat(ppath, &fileStat) != 0)
        return false;
#endif

    pfileStat->AccessTime = fileStat.st_atime;
    pfileStat->ModifyTime = fileStat.st_mtime;
    pfileStat->FileSize   = fileStat.st_size;
    return true;

#endif // !defined(GFC_OS_WII)
}



// ** Overrides

SInt    GSysFile::GetErrorCode()
{
    return pFile ? pFile->GetErrorCode() : Error_FileNotFound;
}


// Overrides to provide re-open support
bool    GSysFile::IsValid()
{
    return pFile && pFile->IsValid();
}
bool    GSysFile::Close()
{
    if (IsValid())
    {
        GDelegatedFile::Close();
        pFile = *new GUnopenedFile;
        return 1;
    }
    return 0;
}

/*
bool    GSysFile::CloseCancel()
{
    if (IsValid())
    {
        GBufferedFile::CloseCancel();
        pFile = *new GUnopenedFile;
        return 1;
    }
    return 0;
}
*/

#endif // GFC_USE_SYSFILE
