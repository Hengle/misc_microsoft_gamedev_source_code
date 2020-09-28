#if !defined(GRANNY_CRC_H)
#include "header_preamble.h"
// ========================================================================
// $File: //jeffr/granny/rt/granny_crc.h $
// $DateTime: 2006/10/16 14:57:23 $
// $Change: 13583 $
// $Revision: #4 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_NAMESPACE_H)
#include "granny_namespace.h"
#endif

#if !defined(GRANNY_TYPES_H)
#include "granny_types.h"
#endif

BEGIN_GRANNY_NAMESPACE EXPGROUP(CRCGroup);

EXPAPI GS_SAFE void BeginCRC32(uint32 &CRC);
EXPAPI GS_SAFE void AddToCRC32(uint32 &CRC, uint32 Count, void const *UInt8s);
EXPAPI GS_SAFE void EndCRC32(uint32 &CRC);

END_GRANNY_NAMESPACE;

#include "header_postfix.h"
#define GRANNY_CRC_H
#endif
