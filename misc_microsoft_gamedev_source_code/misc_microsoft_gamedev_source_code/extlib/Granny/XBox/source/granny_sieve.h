#if !defined(GRANNY_SIEVE_H)
#include "header_preamble.h"
// ========================================================================
// $File: //jeffr/granny/rt/granny_sieve.h $
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

struct sieve;

sieve *NewSieve(int32x GroupCount, int32x UnitSize);
int32x GetUsedGroupCount(sieve &Sieve);
void ClearSieve(sieve &Sieve);
void FreeSieve(sieve *Sieve);
void *AddSieveUnit(sieve &Sieve, int32x GroupIndex);
int32x GetSieveGroupUnitCount(sieve &Sieve, int32x GroupIndex);
void SerializeSieveGroup(sieve &Sieve, int32x GroupIndex, void *Dest);

END_GRANNY_NAMESPACE;

#include "header_postfix.h"
#define GRANNY_SIEVE_H
#endif
