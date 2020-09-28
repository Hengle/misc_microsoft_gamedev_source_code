// ========================================================================
// $File: //jeffr/granny/rt/granny_pointer_hash.cpp $
// $DateTime: 2006/12/21 17:45:34 $
// $Change: 14013 $
// $Revision: #11 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_POINTER_HASH_H)
#include "granny_pointer_hash.h"
#endif

#if !defined(GRANNY_MEMORY_H)
#include "granny_memory.h"
#endif

#if !defined(GRANNY_ASSERT_H)
#include "granny_assert.h"
#endif

#if !defined(GRANNY_LOG_H)
#include "granny_log.h"
#endif

#if !defined(GRANNY_CPP_SETTINGS_H)
// This should always be the last header included
#include "granny_cpp_settings.h"
#endif

USING_GRANNY_NAMESPACE;

BEGIN_GRANNY_NAMESPACE;

struct hash_entry
{
    void const *Key;
    void *Data;

    hash_entry *Left;
    hash_entry *Right;
};

inline int32x HashKeyDifference(const void* a, const void* b)
{
    intaddrx Diff = ((const uint8 *)a - (const uint8 *)b);
    if (Diff < 0)
        return -1;
    else if (Diff > 0)
        return 1;
    return 0;
}

#define CONTAINER_NAME pointer_hash
#define CONTAINER_ITEM_TYPE hash_entry
#define CONTAINER_ADD_FIELDS void const *Key, void *Data
#define CONTAINER_ADD_ASSIGN(Item) (Item)->Key = Key; (Item)->Data = Data;
#define CONTAINER_COMPARE_ITEMS(Item1, Item2) HashKeyDifference((Item1)->Key, (Item2)->Key)
#define CONTAINER_FIND_FIELDS void const *Key
#define CONTAINER_COMPARE_FIND_FIELDS(Item) HashKeyDifference(Key, (Item)->Key)
#define CONTAINER_SORTED 1
#define CONTAINER_KEEP_LINKED_LIST 0
#define CONTAINER_SUPPORT_DUPES 0
#include "granny_contain.inl"

END_GRANNY_NAMESPACE;

pointer_hash *GRANNY
NewPointerHash(void)
{
    pointer_hash *Hash = Allocate(pointer_hash);
    if(Hash)
    {
        Initialize(Hash, 0);
    }

    return(Hash);
}

void GRANNY
DeletePointerHash(pointer_hash *Hash)
{
    if(Hash)
    {
        FreeMemory(Hash);
        Deallocate(Hash);
    }
}

void GRANNY
AddPointerToHash(pointer_hash &Hash, void const *Key, void *Data)
{
    Assert(Find(&Hash, Key) == 0);

    hash_entry *Entry = Add(&Hash, Key, Data);
    if(!Entry)
    {
        Log0(ErrorLogMessage, HashLogMessage,
             "Out of space in pointer hash table");
    }
}

bool GRANNY
SetHashedPointerData(pointer_hash &Hash, void const *Key, void *Data)
{
    bool Result = true;

    hash_entry *Entry = Find(&Hash, Key);
    if(Entry)
    {
        Entry->Data = Data;
    }
    else
    {
        Result = false;
        Entry = Add(&Hash, Key, Data);
        if(!Entry)
        {
            Log0(ErrorLogMessage, HashLogMessage,
                 "Out of space in pointer hash table");
        }
    }

    return(Result);
}

bool GRANNY
GetHashedPointerData(pointer_hash &Hash, void const *Key, void *&Data)
{
    hash_entry *Entry = Find(&Hash, Key);
    if(Entry)
    {
        Data = Entry->Data;
        return(true);
    }

    Data = 0;

    return(false);
}

bool GRANNY
HashedPointerKeyExists(pointer_hash &Hash, void const *Key)
{
    return (Find(&Hash, Key) != NULL);
}

