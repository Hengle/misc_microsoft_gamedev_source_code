#if !defined(GRANNY_FILE_OPERATIONS_H)
#include "header_preamble.h"
// ========================================================================
// $File: //jeffr/granny/rt/granny_file_operations.h $
// $DateTime: 2006/10/16 14:57:23 $
// $Change: 13583 $
// $Revision: #8 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

// This prevents windows.h from stealing the name
#undef DeleteFile

#if !defined(GRANNY_NAMESPACE_H)
#include "granny_namespace.h"
#endif

#if !defined(GRANNY_TYPES_H)
#include "granny_types.h"
#endif

BEGIN_GRANNY_NAMESPACE EXPGROUP(FileBuilderGroup);

struct file_writer;
struct file_reader;

EXPAPI GS_SAFE char const *GetTemporaryDirectory(void);
void DeleteFile(char const *FileName);
bool ConcatenateFile(file_writer &Writer, char const *FileName,
                     int32x CopyBufferSize, void *CopyBuffer,
                     uint32x &BytesCopied);
bool ConcatenateFileReader(file_writer &Writer,
                           file_reader &Reader,
                           int32x CopyBufferSize, void *CopyBuffer,
                           uint32x &BytesCopied);
int32x GetFileSize(char const *FileName);

END_GRANNY_NAMESPACE;

#include "header_postfix.h"
#define GRANNY_FILE_OPERATIONS_H
#endif
