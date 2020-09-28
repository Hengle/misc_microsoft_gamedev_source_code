// ========================================================================
// $File: //jeffr/granny/rt/granny_member_iterator.cpp $
// $DateTime: 2007/04/23 12:52:22 $
// $Change: 14830 $
// $Revision: #8 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_MEMBER_ITERATOR_H)
#include "granny_member_iterator.h"
#endif

#if !defined(GRANNY_ASSERT_H)
#include "granny_assert.h"
#endif

#if !defined(GRANNY_LOG_H)
#include "granny_log.h"
#endif

#if !defined(GRANNY_MEMORY_H)
#include "granny_memory.h"
#endif

#if !defined(GRANNY_CPP_SETTINGS_H)
// This should always be the last header included
#include "granny_cpp_settings.h"
#endif

USING_GRANNY_NAMESPACE;

static void
PrepareIterator(member_iterator &Iterator)
{
    Iterator.Count = &Iterator.CountTemp;
    Iterator.PointerType = &Iterator.TypeTemp;
    Iterator.Pointer = &Iterator.PointerTemp;
    Iterator.ArrayPointer = 0;
    Iterator.TypeIndex = 0;

    if(MemberIteratorIsValid(Iterator))
    {
        Iterator.Type = Iterator.AtType->Type;
        if(Iterator.IteratorType == MemberIteratorType)
        {
            Iterator.MemberSize = GetMemberTypeSize(*Iterator.AtType);
        }
        Iterator.TypeTemp = Iterator.AtType->ReferenceType;
        Iterator.CountTemp = 1;
        Iterator.PointerTemp = 0;

        uint8 *Member = Iterator.At;
        if(Iterator.IteratorType == PointerArrayIteratorType)
        {
            Iterator.Pointer = (void **)Member;
        }
        else
        {
            switch(Iterator.Type)
            {
                case ReferenceMember:
                case EmptyReferenceMember:
                {
                    Iterator.Pointer = (void **)Member;
                } break;

                case ReferenceToArrayMember:
                {
                    Iterator.Count = (int32 *)Member;
                    Iterator.ArrayPointer = (void **)(Member + SizeOf(int32));
                } break;

                case ArrayOfReferencesMember:
                {
                    Iterator.Count = (int32 *)Member;
                    Iterator.ArrayPointer = (void **)(Member + SizeOf(int32));
                } break;

                case VariantReferenceMember:
                {
                    Iterator.PointerType = (data_type_definition **)Member;
                    Iterator.Pointer = (void **)(Member + SizeOf(void*));
                } break;

                case ReferenceToVariantArrayMember:
                {
                    Iterator.PointerType = (data_type_definition **)Member;
                    Iterator.Count = (int32 *)(Member + SizeOf(void*));
                    Iterator.ArrayPointer = (void **)(Member + SizeOf(void*) + SizeOf(int32));
                } break;

                case StringMember:
                {
                    Iterator.Pointer = (void **)Member;
                } break;

                case UnsupportedMemberType_Remove:
                {
                    InvalidCodePath("Switchable types no longer supported in Granny 2.7+");
                } break;

                default:
                {
                    // Nothing to be done for most types
                } break;
            }
        }

        *Iterator.Count *= GetMemberArrayWidth(*Iterator.AtType);
    }
}

void GRANNY
IterateOverMembers(data_type_definition const *Type, void *Object,
                   member_iterator &Iterator)
{
    Iterator.IteratorType = MemberIteratorType;
    Iterator.AtType = Type;
    Iterator.At = (uint8 *)Object;
    Iterator.AtCount = 1;

    if(Iterator.AtType)
    {
        Iterator.Type = OnePastLastMemberType;
    }

    PrepareIterator(Iterator);
}

void GRANNY
IterateOverArray(member_iterator_type IteratorType,
                 data_type_definition const *Type,
                 int32x Count, void *Array,
                 member_iterator &Iterator)
{
    Assert((IteratorType == PointerArrayIteratorType) ||
           (IteratorType == ArrayIteratorType));

    Iterator.IteratorType = IteratorType;
    Iterator.AtType = Type;
    Iterator.At = (uint8 *)Array;
    Iterator.AtCount = Count;

    if(Count && Iterator.AtType)
    {
        Iterator.MemberSize = GetTotalObjectSize(Iterator.AtType);
        Iterator.Type = Iterator.AtType->Type;
    }

    PrepareIterator(Iterator);
}

bool GRANNY
MemberIteratorIsValid(member_iterator &MemberIterator)
{
    return(MemberIterator.AtType && (MemberIterator.Type != EndMember) &&
           MemberIterator.At && MemberIterator.AtCount);
}

void GRANNY
AdvanceMemberIterator(member_iterator &MemberIterator)
{
    Assert(MemberIteratorIsValid(MemberIterator));

    if(MemberIterator.IteratorType == PointerArrayIteratorType)
    {
        MemberIterator.At += SizeOf(void*);
        --MemberIterator.AtCount;
    }
    else
    {
        MemberIterator.At += MemberIterator.MemberSize;

        if(MemberIterator.IteratorType == MemberIteratorType)
        {
            ++MemberIterator.AtType;
        }
        else
        {
            Assert(MemberIterator.IteratorType == ArrayIteratorType);
            --MemberIterator.AtCount;
        }
    }

    PrepareIterator(MemberIterator);
}

void GRANNY
IterateOverSubMembers(member_iterator &MemberIterator,
                      member_iterator &Result)
{
    Assert(MemberIteratorIsValid(MemberIterator));

    switch(MemberIterator.IteratorType)
    {
        case MemberIteratorType:
        {
            if(MemberIterator.AtType->Type == InlineMember)
            {
                IterateOverMembers(*MemberIterator.PointerType,
                                   MemberIterator.At, Result);
            }
            else
            {
                IterateOverMembers(*MemberIterator.PointerType,
                                   *MemberIterator.Pointer, Result);
            }
        } break;

        default:
        {
            Log1(ErrorLogMessage, DataTypeLogMessage,
                 "Illegal iterator type (%d) in IterateOverSubMembers",
                 MemberIterator.IteratorType);
            IterateOverArray(PointerArrayIteratorType,
                             0, 0, 0, Result);
        } break;
    }
}

void GRANNY
IterateOverSubArray(member_iterator &MemberIterator,
                    member_iterator &Result)
{
    switch(MemberIterator.Type)
    {
        case ReferenceToArrayMember:
        case ReferenceToVariantArrayMember:
        {
            IterateOverArray(ArrayIteratorType,
                             *MemberIterator.PointerType,
                             *MemberIterator.Count,
                             *MemberIterator.ArrayPointer,
                             Result);
        } break;

        case ArrayOfReferencesMember:
        {
            IterateOverArray(PointerArrayIteratorType,
                             *MemberIterator.PointerType,
                             *MemberIterator.Count,
                             *MemberIterator.ArrayPointer,
                             Result);
        } break;

        default:
        {
            Log1(ErrorLogMessage, DataTypeLogMessage,
                 "Illegal iterator type (%d) in IterateOverSubMembers",
                 MemberIterator.IteratorType);

            IterateOverArray(PointerArrayIteratorType,
                             0, 0, 0, Result);
        } break;
    }
}

void GRANNY
IterateOverElement(member_iterator &MemberIterator,
                   member_iterator &Result)
{
    Assert(MemberIteratorIsValid(MemberIterator));

    switch(MemberIterator.IteratorType)
    {
        case ArrayIteratorType:
        {
            IterateOverMembers(MemberIterator.AtType, MemberIterator.At,
                               Result);
        } break;

        case PointerArrayIteratorType:
        {
            IterateOverMembers(MemberIterator.AtType, *(void **)MemberIterator.At,
                               Result);
        } break;

        default:
        {
            IterateOverArray(PointerArrayIteratorType,
                             0, 0, 0, Result);
        } break;
    }
}
