#if !defined(GRANNY_BINK0_COMPRESSION_H)
#include "header_preamble.h"
// ========================================================================
// $File: //jeffr/granny/rt/granny_bink0_compression.h $
// $DateTime: 2006/10/16 14:57:23 $
// $Change: 13583 $
// $Revision: #3 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_TYPES_H)
#include "granny_types.h"
#endif

BEGIN_GRANNY_NAMESPACE;

uint32 ToBinkTC0(void *output,
                 uint32 compress_to,
                 int16 **input,
                 real32 const *plane_weights,
                 uint32 planes,
                 uint32 width,
                 uint32 height,
                 void *temp, uint32 temp_size );
uint32 ToBinkTCTempMem0(uint32 width, uint32 height);
uint32 ToBinkTCOutputMem0(uint32 width, uint32 height,
                          uint32 planes, uint32 compress_to);
void BinkTCCheckSizes0(uint32 *width, uint32 *height);

void FromBinkTC0(int16 **output,
                 uint32 planes,
                 void const * bink_buf,
                 uint32 width,
                 uint32 height,
                 void *temp,
                 uint32 temp_size);
uint32 FromBinkTCTempMem0(void const *binkbuf);

END_GRANNY_NAMESPACE;

#include "header_postfix.h"
#define GRANNY_BINK0_COMPRESSION_H
#endif
