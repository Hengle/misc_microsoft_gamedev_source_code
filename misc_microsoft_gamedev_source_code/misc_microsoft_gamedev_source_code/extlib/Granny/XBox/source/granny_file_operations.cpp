// ========================================================================
// $File: //jeffr/granny/rt/granny_file_operations.cpp $
// $DateTime: 2006/10/16 14:57:23 $
// $Change: 13583 $
// $Revision: #5 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_FILE_OPERATIONS_H)
#include "granny_file_operations.h"
#endif

#if !defined(GRANNY_FILE_WRITER_H)
#include "granny_file_writer.h"
#endif

#if !defined(GRANNY_FILE_READER_H)
#include "granny_file_reader.h"
#endif

#if !defined(GRANNY_CPP_SETTINGS_H)
// This should always be the last header included
#include "granny_cpp_settings.h"
#endif

USING_GRANNY_NAMESPACE;

bool GRANNY
ConcatenateFile(file_writer &Writer, char const *FileName,
                int32x CopyBufferSize, void *CopyBuffer,
                uint32x &BytesCopied)
{
    bool Result = false;
    BytesCopied = 0;

    file_reader *Reader = OpenFileReader(FileName);
    if(Reader)
    {
        Result = ConcatenateFileReader(Writer, *Reader,
                                       CopyBufferSize, CopyBuffer,
                                       BytesCopied);
        CloseFileReader(Reader);
    }

    return(Result);
}


bool GRANNY
ConcatenateFileReader(file_writer &Writer,
                      file_reader &Reader,
                      int32x CopyBufferSize, void *CopyBuffer,
                      uint32x &BytesCopied)
{
    bool Result = true;
    BytesCopied = 0;

    while(Result)
    {
        int32x ReadSize = ReadAtMost(Reader, BytesCopied,
                                     CopyBufferSize, CopyBuffer);
        if ( ReadSize == 0 )
        {
            break;
        }
        Result = Result && Write(Writer, ReadSize, CopyBuffer);
        BytesCopied += ReadSize;
    }

    return(Result);
}
