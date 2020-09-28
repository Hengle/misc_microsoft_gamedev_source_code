// ========================================================================
// $File: //jeffr/granny/rt/granny_variant_builder.cpp $
// $DateTime: 2007/07/27 17:42:17 $
// $Change: 15625 $
// $Revision: #16 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_VARIANT_BUILDER_H)
#include "granny_variant_builder.h"
#endif

#if !defined(GRANNY_DATA_TYPE_DEFINITION_H)
#include "granny_data_type_definition.h"
#endif

#if !defined(GRANNY_MEMORY_H)
#include "granny_memory.h"
#endif

#if !defined(GRANNY_STRING_TABLE_H)
#include "granny_string_table.h"
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

struct variant_member_builder
{
    data_type_definition Type;
    void *Data;

    variant_member_builder *Next;
};

struct variant_builder
{
    string_table *Strings;

    int32 MemberCount;
    int32 TotalObjectSize;
    variant_member_builder *FirstMember;
    variant_member_builder *LastMember;
};

END_GRANNY_NAMESPACE;

variant_builder *GRANNY
BeginVariant(string_table &StringTableBuilder)
{
    variant_builder *Builder = Allocate(variant_builder);
    if(Builder)
    {
        Builder->Strings = &StringTableBuilder;

        // We initialize the member count to 1 because we always
        // have a trailing "EndMember" type.
        Builder->MemberCount = 1;
        Builder->TotalObjectSize = 0;
        Builder->FirstMember = 0;
        Builder->LastMember = 0;
    }

    return(Builder);
}

bool GRANNY
EndVariant(variant_builder *Builder, data_type_definition *&Type,
           void *&Object)
{
    Type = 0;
    Object = 0;

    if(Builder)
    {
        aggr_allocator Allocator;
        InitializeAggrAlloc(Allocator);

        void *TypeMemory = 0;
        void *ObjectMemory = 0;

        AggrAllocSizePtr(Allocator, GetResultingVariantTypeSize(*Builder), TypeMemory);
        AggrAllocSizePtr(Allocator, GetResultingVariantObjectSize(*Builder), ObjectMemory);
        EndAggrAlloc(Allocator);

        EndVariantInPlace(Builder, TypeMemory, Type, ObjectMemory, Object);
    }

    return(Type && Object);
}

bool GRANNY
VariantIsNonEmpty(variant_builder const& Builder)
{
    return Builder.TotalObjectSize != 0;
}


int32x GRANNY
GetResultingVariantTypeSize(variant_builder const &Builder)
{
    return(Builder.MemberCount * SizeOf(data_type_definition));
}

int32x GRANNY
GetResultingVariantObjectSize(variant_builder const &Builder)
{
    // Subtlety - we need to make sure we always allocate at least a little
    // space for the object, otherwise its pointer may coincide with the
    // type pointer, which would wreck havoc for the type writer.

    // TODO: Should the aggregate allocator always bump sizes up to 4
    // if they come in as zero??
    return(Builder.TotalObjectSize ? Builder.TotalObjectSize : 4);
}

bool GRANNY
EndVariantInPlace(variant_builder *Builder,
                  void *TypeMemory,
                  data_type_definition *&Type,
                  void *ObjectMemory,
                  void *&Object)
{
    Type = 0;
    Object = 0;

    if(Builder)
    {
        if(TypeMemory && ObjectMemory)
        {
            Type = (data_type_definition *)TypeMemory;
            Object = ObjectMemory;

            data_type_definition *TypePtr = Type;
            uint8 *ObjectPtr = (uint8 *)Object;
            {for(variant_member_builder const *Member = Builder->FirstMember;
                 Member;
                 Member = Member->Next)
            {
                *TypePtr++ = Member->Type;

                int32x MemberSize = GetMemberTypeSize(Member->Type);
                Copy(MemberSize, Member->Data, ObjectPtr);
                ObjectPtr += MemberSize;
            }}

            TypePtr->Type = EndMember;
        }

        variant_member_builder *MemberIterator = Builder->FirstMember;
        while(MemberIterator)
        {
            variant_member_builder *DeleteIterator = MemberIterator;
            MemberIterator = MemberIterator->Next;

            Deallocate(DeleteIterator);
        }

        Deallocate(Builder);
    }

    return(Type && Object);
}

void GRANNY
AbortVariant(variant_builder* Builder)
{
    variant Dummy;
    EndVariantInPlace(Builder,
                      NULL, Dummy.Type,
                      NULL, Dummy.Object);
}

static void
AddMember(variant_builder &Builder, data_type_definition const &Type,
          void const *Value)
{
    aggr_allocator Allocator;
    InitializeAggrAlloc(Allocator);

    int32x MemberSize = GetMemberTypeSize(Type);
    variant_member_builder *NewMember;
    AggrAllocPtr(Allocator, NewMember);
    AggrAllocOffsetSizePtr(Allocator, NewMember, MemberSize, Data);
    if(EndAggrAlloc(Allocator))
    {
        NewMember->Type = Type;
        NewMember->Next = 0;
        Copy(MemberSize, Value, NewMember->Data);

        Builder.TotalObjectSize += MemberSize;
        ++Builder.MemberCount;
        if(Builder.FirstMember)
        {
            Builder.LastMember = Builder.LastMember->Next = NewMember;
        }
        else
        {
            Builder.FirstMember = Builder.LastMember = NewMember;
        }
    }
}

void GRANNY
AddBoolMember(variant_builder &Builder, char const *Name, bool32 Value)
{
    Name = MapString(*Builder.Strings, Name);
    data_type_definition Type = {Bool32Member, Name};
    AddMember(Builder, Type, &Value);
}

void GRANNY
AddIntegerMember(variant_builder &Builder, char const *Name, int32 Value)
{
    Name = MapString(*Builder.Strings, Name);
    data_type_definition Type = {Int32Member, Name};
    AddMember(Builder, Type, &Value);
}

void GRANNY
AddIntegerArrayMember(variant_builder &Builder, char const *Name, int32x Width, int32* Value)
{
    Name = MapString(*Builder.Strings, Name);
    data_type_definition Type = {Int32Member, Name, 0, Width};
    AddMember(Builder, Type, Value);
}

void GRANNY
AddUnsignedIntegerMember(variant_builder &Builder, char const *Name, uint32 Value)
{
    Name = MapString(*Builder.Strings, Name);
    data_type_definition Type = {UInt32Member, Name};
    AddMember(Builder, Type, &Value);
}

void GRANNY
AddScalarMember(variant_builder &Builder, char const *Name, real32 Value)
{
    Name = MapString(*Builder.Strings, Name);
    data_type_definition Type = {Real32Member, Name};
    AddMember(Builder, Type, &Value);
}

void GRANNY
AddScalarArrayMember(variant_builder &Builder, char const *Name,
                     int32x Width, real32 const *Value)
{
    Name = MapString(*Builder.Strings, Name);
    data_type_definition Type = {Real32Member, Name, 0, Width};
    AddMember(Builder, Type, Value);
}

void GRANNY
AddStringMember(variant_builder &Builder, char const *Name, char const *Value)
{
    Name = MapString(*Builder.Strings, Name);
    data_type_definition Type = {StringMember, Name};
    char const *String = MapString(*Builder.Strings, Value);
    AddMember(Builder, Type, &String);
}

void GRANNY
AddReferenceMember(variant_builder &Builder, char const *Name,
                   data_type_definition *ReferenceType, void *Value)
{
    Name = MapString(*Builder.Strings, Name);
    data_type_definition Type = {ReferenceMember, Name, ReferenceType};
    AddMember(Builder, Type, &Value);
}

struct array_stub
{
    int32 Count;
    void *Array;
};
CompileAssert(SizeOf(array_stub) == (SizeOf(int32) + SizeOf(void*)));

void GRANNY
AddDynamicArrayMember(variant_builder &Builder, char const *Name,
                      int32x Count, data_type_definition *EntryType,
                      void *Array)
{
    Name = MapString(*Builder.Strings, Name);
    data_type_definition Type = {ReferenceToArrayMember, Name, EntryType};
    array_stub Stub = {Count, Array};

    AddMember(Builder, Type, &Stub);
}
