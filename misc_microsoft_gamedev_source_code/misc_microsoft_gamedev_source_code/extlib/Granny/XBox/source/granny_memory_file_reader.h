#if !defined(GRANNY_MEMORY_FILE_READER_H)
#include "header_preamble.h"
// ========================================================================
// $File: //jeffr/granny/rt/granny_memory_file_reader.h $
// $DateTime: 2006/10/16 14:57:23 $
// $Change: 13583 $
// $Revision: #4 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_TYPES_H)
#include "granny_types.h"
#endif

BEGIN_GRANNY_NAMESPACE EXPGROUP(FileReaderGroup);

struct file_reader;
EXPAPI GS_SAFE file_reader *CreateMemoryFileReader(char const *SourceFileName,
                                                   int32x SourceLineNumber,
                                                   int32x Offset, int32x MemorySize,
                                                   void *Memory);

END_GRANNY_NAMESPACE;

#include "header_postfix.h"
#define GRANNY_MEMORY_FILE_READER_H
#endif
