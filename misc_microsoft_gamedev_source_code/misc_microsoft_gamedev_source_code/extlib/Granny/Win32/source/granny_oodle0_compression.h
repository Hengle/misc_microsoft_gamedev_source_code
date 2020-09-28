#if !defined(GRANNY_OODLE0_COMPRESSION_H)
#include "header_preamble.h"
// ========================================================================
// $File: //jeffr/granny/rt/granny_oodle0_compression.h $
// $DateTime: 2006/10/16 14:57:23 $
// $Change: 13583 $
// $Revision: #5 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_NAMESPACE_H)
#include "granny_namespace.h"
#endif

#if !defined(GRANNY_TYPES_H)
#include "granny_types.h"
#endif

BEGIN_GRANNY_NAMESPACE;

// CompressedBytes must be padded at the end by at least the number of
// bytes returned by GetOodle0DecompressBufferPaddingSize(void) or
// the compressor will read-fault off the end.
int32x GetOodle0DecompressBufferPaddingSize(void);
void Oodle0Decompress(bool FileIsByteReversed,
                      int32x CompressedBytesSize,
                      void *CompressedBytes,
                      int32x Stop0, int32x Stop1, int32x Stop2,
                      void *DecompressedBytes);
void Oodle0Decompress(bool FileIsByteReversed,
                      int32x CompressedBytesSize,
                      void *CompressedBytes,
                      int32x DecompressedSize,
                      void *DecompressedBytes);

END_GRANNY_NAMESPACE;

#include "header_postfix.h"
#define GRANNY_OODLE0_COMPRESSION_H
#endif
