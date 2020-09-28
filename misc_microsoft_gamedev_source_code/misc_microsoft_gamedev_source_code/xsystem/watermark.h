
//==============================================================================
// watermark.h
//
// Copyright (c) 1999 - 2002 Ensemble Studios
//==============================================================================

#ifndef __WATERMARK_H__
#define __WATERMARK_H__

#define WATERMARK_SENTINEL 0x44332211
#define WATERMARK_SIZE (4+4+4+256)

extern BYTE watermark[WATERMARK_SIZE];

#endif // __WATERMARK_H__

//==============================================================================
// eof: watermark.h
//==============================================================================
