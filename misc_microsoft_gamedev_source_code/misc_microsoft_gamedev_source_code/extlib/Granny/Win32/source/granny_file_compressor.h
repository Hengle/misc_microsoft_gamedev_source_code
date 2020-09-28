#if !defined(GRANNY_FILE_COMPRESSOR_H)
#include "header_preamble.h"
// ========================================================================
// $File: //jeffr/granny/rt/granny_file_compressor.h $
// $DateTime: 2006/12/18 15:41:05 $
// $Change: 13934 $
// $Revision: #10 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_NAMESPACE_H)
#include "granny_namespace.h"
#endif

#if !defined(GRANNY_TYPES_H)
#include "granny_types.h"
#endif

BEGIN_GRANNY_NAMESPACE EXPGROUP(FileCompressionGroup);

struct file_writer;
struct file_reader;

EXPTYPE enum compression_type
{
    NoCompression,
    Oodle0Compression,
    Oodle1Compression,
    OnePastLastCompressionType
};

EXPTYPE struct file_compressor;

EXPAPI GS_SAFE int32x GetCompressedBytesPaddingSize(compression_type Format);
EXPAPI GS_PARAM bool DecompressData(compression_type Format,
                                    bool FileIsByteReversed,
                                    int32x CompressedBytesSize,
                                    void *CompressedBytes,
                                    int32x Stop0, int32x Stop1, int32x Stop2,
                                    void *DecompressedBytes);
EXPAPI GS_PARAM bool DecompressDataChunk(compression_type Format,
                                         bool FileIsByteReversed,
                                         int32x CompressedBytesSize,
                                         void *CompressedBytes,
                                         int32x DecompressedBytesSize,
                                         void *DecompressedBytes);

EXPAPI GS_PARAM file_compressor *BeginFileCompression(uint32x ExpandedDataSize,
                                                      int32x ContentsCount,
                                                      compression_type Type,
                                                      bool WritingForReversedPlatform,
                                                      file_writer *Writer);

EXPAPI GS_MODIFY bool CompressContentsOfFile(file_compressor &Compressor,
                                             int32x FileSize,
                                             char const *FileName,
                                             int32x CopyBufferSize, void *CopyBuffer);
EXPAPI GS_PARAM bool CompressContentsOfReader(file_compressor &Compressor,
                                              int32x FileSize,
                                              file_reader &Reader,
                                              int32x CopyBufferSize, void *CopyBuffer);
EXPAPI GS_PARAM bool CompressContentsOfMemory(file_compressor &Compressor,
                                              int32x BufferSize,
                                              void const *Buffer);

EXPAPI GS_PARAM bool EndFileCompression(file_compressor *Compressor,
                                        uint32x &CompressedSize);

END_GRANNY_NAMESPACE;

#include "header_postfix.h"
#define GRANNY_FILE_COMPRESSOR_H
#endif
