#if !defined(GRANNY_MEMBER_ITERATOR_H)
#include "header_preamble.h"
// ========================================================================
// $File: //jeffr/granny/rt/granny_member_iterator.h $
// $DateTime: 2006/10/16 14:57:23 $
// $Change: 13583 $
// $Revision: #5 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_TYPES_H)
#include "granny_types.h"
#endif

#if !defined(GRANNY_DATA_TYPE_DEFINITION_H)
#include "granny_data_type_definition.h"
#endif

BEGIN_GRANNY_NAMESPACE;

enum member_iterator_type
{
    PointerArrayIteratorType,
    MemberIteratorType,
    ArrayIteratorType,
};

struct member_iterator
{
    // These are the primary iterator members
    member_iterator_type IteratorType;
    data_type_definition const *AtType;
    uint8 *At;
    int32x AtCount;

    // These members are generated from the current state of the
    // primary members
    member_type Type;
    int32x MemberSize;

    void *PointerTemp;
    data_type_definition *TypeTemp;
    int32 CountTemp;

    int32 *Count;
    data_type_definition **PointerType;
    void **Pointer;
    void **ArrayPointer;
    int32 *TypeIndex;

    // some version of gcc will complain about all member functions being
    //  private.  There is no useful way to selectively remove this warning
    //  without shutting everything off, so just swap the compiletime error
    //  for a link error in this case
#if !COMPILER_GCC
private:
#endif
    member_iterator &operator=(member_iterator const &);
};

void IterateOverMembers(data_type_definition const *Type, void *Object,
                        member_iterator &Result);
void IterateOverArray(member_iterator_type IteratorType,
                      data_type_definition const *Type,
                      int32x Count, void *Array,
                      member_iterator &Result);

bool MemberIteratorIsValid(member_iterator &MemberIterator);
void AdvanceMemberIterator(member_iterator &MemberIterator);
void IterateOverSubMembers(member_iterator &MemberIterator,
                           member_iterator &Result);
void IterateOverSubArray(member_iterator &MemberIterator,
                         member_iterator &Result);
void IterateOverElement(member_iterator &MemberIterator,
                        member_iterator &Result);

END_GRANNY_NAMESPACE;

#include "header_postfix.h"
#define GRANNY_MEMBER_ITERATOR_H
#endif
