#if !defined(GRANNY_FILE_WRITER_H)
#include "header_preamble.h"
// ========================================================================
// $File: //jeffr/granny/rt/granny_file_writer.h $
// $DateTime: 2006/10/16 14:57:23 $
// $Change: 13583 $
// $Revision: #7 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

/* ========================================================================
   Explicit Dependencies
   ======================================================================== */
#if !defined(GRANNY_NAMESPACE_H)
#include "granny_namespace.h"
#endif

#if !defined(GRANNY_TYPES_H)
#include "granny_types.h"
#endif

BEGIN_GRANNY_NAMESPACE EXPGROUP(FileWriterGroup);

struct file_writer;

EXPTYPE enum file_writer_seek_type
{
    SeekStart,
    SeekEnd,
    SeekCurrent
};

// On creation, you must specify a file to write and a flag to specify
// whether or not to check for overwriting an existing file.  If the
// file cannot be opened for any reason, a warning is written to the
// error log and 0 is returned.
EXPAPI typedef file_writer *open_file_writer_callback(
    char const *SourceFileName,
    int32x SourceLineNumber,
    char const *FileName,
    bool EraseExisting);
EXPAPI typedef void delete_file_writer_callback(
    char const *SourceFileName,
    int32x SourceLineNumber,
    file_writer *Writer);
EXPAPI typedef int32x seek_file_writer_callback(
    char const *SourceFileName,
    int32x SourceLineNumber,
    file_writer &Writer,
    int32x OffsetInUInt8s,
    file_writer_seek_type SeekType);
EXPAPI typedef bool write_file_writer_callback(
    char const *SourceFileName,
    int32x SourceLineNumber,
    file_writer &Writer,
    int32x UInt8Count,
    void const *WritePointer);
EXPAPI typedef void begincrc_file_writer_callback(
    char const *SourceFileName,
    int32x SourceLineNumber,
    file_writer &Writer);
EXPAPI typedef uint32 endcrc_file_writer_callback(
    char const *SourceFileName,
    int32x SourceLineNumber,
    file_writer &Writer);


EXPTYPE_EPHEMERAL struct file_writer
{
    char const *SourceFileName;
    int32x SourceLineNumber;

    delete_file_writer_callback *DeleteFileWriterCallback;
    seek_file_writer_callback   *SeekWriterCallback;
    write_file_writer_callback  *WriteCallback;
    begincrc_file_writer_callback *BeginCRCCallback;
    endcrc_file_writer_callback   *EndCRCCallback;

    bool   CRCing;
    uint32 CRC;
};


#define NewFileWriter(FileName, EraseExisting) (CreatePlatformFileWriter(__FILE__, __LINE__, FileName, EraseExisting))
#define DeleteFileWriter(Writer)               (*(Writer)->DeleteFileWriterCallback)(__FILE__, __LINE__, Writer) EXPMACRO

#define GetWriterPosition(Writer) (*(Writer).SeekWriterCallback)(__FILE__, __LINE__, Writer, 0, SeekCurrent) EXPMACRO

// Forward these to stubs so we can get the enum right in granny.h
#define SeekWriterFromStart(Writer, OffsetInUInt8s)           (SeekWriterFromStartStub(__FILE__, __LINE__, Writer, OffsetInUInt8s)) EXPMACRO
#define SeekWriterFromEnd(Writer, OffsetInUInt8s)             (SeekWriterFromEndStub(__FILE__, __LINE__, Writer, OffsetInUInt8s)) EXPMACRO
#define SeekWriterFromCurrentPosition(Writer, OffsetInUInt8s) (SeekWriterFromCurrentPositionStub(__FILE__, __LINE__, Writer, OffsetInUInt8s)) EXPMACRO

// Generic writing of bytes.  If a write fails, false is returned
// and a warning is written to the error log.
#define Write(Writer, UInt8Count, WritePointer) (*(Writer).WriteCallback)(__FILE__, __LINE__, Writer, UInt8Count, WritePointer) EXPMACRO

#define BeginWriterCRC(Writer) (*(Writer).BeginCRCCallback)(__FILE__, __LINE__, Writer) EXPMACRO
#define EndWriterCRC(Writer)   (*(Writer).EndCRCCallback)(__FILE__, __LINE__, Writer) EXPMACRO
#define WriterIsCRCing(Writer) ((Writer).CRCing) EXPMACRO

// AlignWriter() will move the writing pointer by as many bytes as
// necessary to ensure optimal alignment for whatever filesystem
// is being written (this is usually 32-bit alignment, but may
// be 64-bit on some systems).  It returns the number of bytes it
// moved forward.  Align is CRC aware and will write 0's instead of
// seeking if CRCing is active, to ensure that the CRC is valid.
EXPAPI GS_PARAM int32x AlignWriter(file_writer &Writer);

// PredictWriterAlignment() must return what Align() would return
// if the current position was the given parameter
EXPAPI GS_SAFE int32x PredictWriterAlignment(int32x Position);


EXPAPI GS_PARAM void InitializeFileWriter(
    char const *SourceFileName, int32x SourceLineNumber,
    delete_file_writer_callback *DeleteFileWriterCallback,
    seek_file_writer_callback   *SeekWriterCallback,
    write_file_writer_callback  *WriteCallback,
    begincrc_file_writer_callback *BeginCRCCallback,
    endcrc_file_writer_callback   *EndCRCCallback,
    file_writer &Writer);

EXPAPI GS_PARAM file_writer *CreatePlatformFileWriter(
    char const *SourceFileName, int32x SourceLineNumber,
    char const *FileNameToOpen, bool EraseExisting);



// TODO: Access to some sort of list of open file handles?

extern open_file_writer_callback *OpenFileWriterCallback;

EXPAPI GS_READ open_file_writer_callback *GetDefaultFileWriterOpenCallback(void);
EXPAPI GS_MODIFY void SetDefaultFileWriterOpenCallback(
    open_file_writer_callback *OpenFileWriterCallback);



EXPAPI GS_PARAM int32x SeekWriterFromStartStub(char const *SourceFileName,
                                               int32x SourceLineNumber,
                                               file_writer& Writer,
                                               int32x OffsetInUInt8s);
EXPAPI GS_PARAM int32x SeekWriterFromEndStub(char const *SourceFileName,
                                             int32x SourceLineNumber,
                                             file_writer& Writer,
                                             int32x OffsetInUInt8s);
EXPAPI GS_PARAM int32x SeekWriterFromCurrentPositionStub(char const *SourceFileName,
                                                         int32x SourceLineNumber,
                                                         file_writer& Writer,
                                                         int32x OffsetInUInt8s);

END_GRANNY_NAMESPACE;

#include "header_postfix.h"
#define GRANNY_FILE_WRITER_H
#endif
