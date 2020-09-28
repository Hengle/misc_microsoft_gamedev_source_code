/**********************************************************************

Filename    :   GZLibFile.h
Content     :   Header for z-lib wrapped file input 
Created     :   June 24, 2005
Authors     :   Michael Antonov

Notes       :   GZLibFile is currently Read Only

Copyright   :   (c) 1998-2006 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef INC_GZLIBFILE_H
#define INC_GZLIBFILE_H

#include "GFile.h"

// ZLib functionality is only available if GFC_USE_ZLIB is defined
#ifdef GFC_USE_ZLIB

// ***** Declared Classes
class   GZLibFile;



class GZLibFile : public GFile
{
    class GZLibFileImpl   *pImpl;
public:

    // GZLibFile must be constructed with a source file 
    GEXPORT GZLibFile(GFile *psourceFile = 0);
    GEXPORT ~GZLibFile();

    // ** File Information
    GEXPORT virtual const char* GetFilePath();

    // Return 1 if file's usable (open)
    GEXPORT virtual bool        IsValid();
    // Return 0; ZLib files are not writable for now
    GINLINE virtual bool        IsWritable()
        { return 0; }

    // Return position
    // Position position is reported in relation to the compressed stream, NOT the source file
    GEXPORT virtual SInt        Tell ();
    GEXPORT virtual SInt64      LTell ();
    GEXPORT virtual SInt        GetLength ();
    GEXPORT virtual SInt64      LGetLength ();
    // Return errno-based error code
    GEXPORT virtual SInt        GetErrorCode();

    // ** GFxStream implementation & I/O
    
    GEXPORT virtual SInt        Write(const UByte *pbufer, SInt numBytes);  
    GEXPORT virtual SInt        Read(UByte *pbufer, SInt numBytes);
    GEXPORT virtual SInt        SkipBytes(SInt numBytes);           
    GEXPORT virtual SInt        BytesAvailable();
    GEXPORT virtual bool        Flush();
    // Returns new position, -1 for error
    // Position seeking works in relation to the compressed stream, NOT the source file
    GEXPORT virtual SInt        Seek(SInt offset, SInt origin=SEEK_SET);
    GEXPORT virtual SInt64      LSeek(SInt64 offset, SInt origin=SEEK_SET);
    // Writing not supported..
    GEXPORT virtual bool        ChangeSize(SInt newSize);       
    GEXPORT virtual SInt        CopyFromStream(GFile *pstream, SInt byteSize);
    // Closes the file  
    GEXPORT virtual bool        Close();    
};

#endif // GFC_USE_ZLIB


#endif
