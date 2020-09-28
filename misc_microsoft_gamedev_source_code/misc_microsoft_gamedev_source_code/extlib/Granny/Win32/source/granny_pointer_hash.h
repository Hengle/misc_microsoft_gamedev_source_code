#if !defined(GRANNY_POINTER_HASH_H)
#include "header_preamble.h"
// ========================================================================
// $File: //jeffr/granny/rt/granny_pointer_hash.h $
// $DateTime: 2007/03/01 14:09:19 $
// $Change: 14467 $
// $Revision: #6 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_NAMESPACE_H)
#include "granny_namespace.h"
#endif

#if !defined(GRANNY_TYPES_H)
#include "granny_types.h"
#endif

BEGIN_GRANNY_NAMESPACE EXPGROUP(MemoryAllocatorGroup);

EXPTYPE struct pointer_hash;

EXPAPI pointer_hash *NewPointerHash(void);
EXPAPI void DeletePointerHash(pointer_hash *Hash);

EXPAPI void AddPointerToHash(pointer_hash &Hash, void const *Key, void *Data);
EXPAPI bool SetHashedPointerData(pointer_hash &Hash, void const *Key, void *Data);
EXPAPI bool GetHashedPointerData(pointer_hash &Hash, void const *Key, void *&Data);
EXPAPI bool HashedPointerKeyExists(pointer_hash &Hash, void const *Key);

END_GRANNY_NAMESPACE;

#include "header_postfix.h"
#define GRANNY_POINTER_HASH_H
#endif
