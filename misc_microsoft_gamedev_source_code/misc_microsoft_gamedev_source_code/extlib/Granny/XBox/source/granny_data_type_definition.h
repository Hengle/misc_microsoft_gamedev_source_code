#if !defined(GRANNY_DATA_TYPE_DEFINITION_H)
#include "header_preamble.h"
// ========================================================================
// $File: //jeffr/granny/rt/granny_data_type_definition.h $
// $DateTime: 2007/04/23 12:52:22 $
// $Change: 14830 $
// $Revision: #24 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_NAMESPACE_H)
#include "granny_namespace.h"
#endif

#if !defined(GRANNY_TYPES_H)
#include "granny_types.h"
#endif

#if !defined(GRANNY_STRING_H)
#include "granny_string.h"
#endif


BEGIN_GRANNY_NAMESPACE EXPGROUP(DataTypeGroup);

EXPTYPE enum marshalling_type
{
    AnyMarshalling = 0x0,
    Int8Marshalling = 0x1,
    Int16Marshalling = 0x2,
    Int32Marshalling = 0x4,

    MarshallingMask = 0x7
};

EXPTYPE enum member_type
{
    EndMember,

    // Structured members
    InlineMember,
    ReferenceMember,
    ReferenceToArrayMember,
    ArrayOfReferencesMember,
    VariantReferenceMember,
    UnsupportedMemberType_Remove,
    ReferenceToVariantArrayMember,

    StringMember,
    TransformMember,

    // Floating-point members
    Real32Member,

    Int8Member,
    UInt8Member,
    BinormalInt8Member,
    NormalUInt8Member,

    Int16Member,
    UInt16Member,
    BinormalInt16Member,
    NormalUInt16Member,

    Int32Member,
    UInt32Member,

    Real16Member,

    EmptyReferenceMember,

    OnePastLastMemberType,

    // Convenient names
    Bool32Member = Int32Member,
};

// Note that the pointer size conversion routines are
//  dependant on the layout of this structure, touch
//  not!
EXPTYPE struct data_type_definition
{
    member_type Type;
    char const *Name;

    data_type_definition *ReferenceType;
    int32 ArrayWidth;

    int32  Extra[3];

    // yes, I know, we're writing this to disk.  It's value is ignored
    // todo: we don't use the traversal id anymore, remove if possible
    uintaddrx Ignored__Ignored;
};

EXPTYPE struct variant
{
    data_type_definition *Type;
    void *Object;
};


EXPAPI GS_SAFE int32x GetMemberUnitSize(data_type_definition const &MemberType);
EXPAPI GS_SAFE int32x GetMemberTypeSize(data_type_definition const &MemberType);
EXPAPI GS_SAFE int32x GetTotalObjectSize(data_type_definition const *TypeDefinition);
EXPAPI GS_SAFE int32x GetTotalTypeSize(data_type_definition const *TypeDefinition);
EXPAPI GS_SAFE char const *GetMemberTypeName(member_type Type);
EXPAPI GS_SAFE char const *GetMemberCTypeName(member_type Type);

int32x GetMemberUnitSizePlatform(member_type Type, int32x PointerSizeInBits);

// This function is not fully implemented yet, so not exposed in the API.
void GetTypeDefintionCName(data_type_definition const *TypeDef, char *Result, int32x MaxLength);

EXPAPI GS_SAFE bool MemberHasPointers(data_type_definition const &MemberType);
EXPAPI GS_SAFE bool TypeHasPointers(data_type_definition const *TypeDefinition);

EXPAPI GS_SAFE uint32 GetMemberMarshalling(data_type_definition const &MemberType);
EXPAPI GS_SAFE uint32 GetObjectMarshalling(data_type_definition const *TypeDefinition);
EXPAPI GS_SAFE bool IsMixedMarshalling(uint32x Marshalling);

EXPAPI GS_SAFE intaddrx MakeEmptyDataTypeMember(
    data_type_definition const &MemberType, void *Member);
EXPAPI GS_SAFE intaddrx MakeEmptyDataTypeObject(
    data_type_definition const *TypeDefinition, void *Object);

EXPAPI GS_SAFE int32x GetMemberArrayWidth(data_type_definition const &MemberType);
EXPAPI GS_SAFE int32x GetTypeTableCount(data_type_definition const **TypeTable);


EXPAPI GS_SAFE bool DataTypesAreEqual(data_type_definition const *A,
                                      data_type_definition const *B);
EXPAPI GS_SAFE bool DataTypesAreEqualWithNames(data_type_definition const *A,
                                               data_type_definition const *B);
EXPAPI GS_SAFE bool DataTypesAreEqualWithNameCallback(data_type_definition const *A,
                                                      data_type_definition const *B,
                                                      string_comparison_callback *Callback);
bool DataTypesAreEqualAndBDoesntInclude(data_type_definition const *A,
                                        data_type_definition const *B,
                                        data_type_definition const *UnlessBEquals);
EXPAPI GS_SAFE data_type_definition *DataTypeBeginsWith(data_type_definition const *Type,
                                                        data_type_definition const *Prefix);

EXPAPI GS_SAFE void ReverseTypeArray(data_type_definition const *Type,
                                     int32x Count, void *TypeArray);

//
// Basic types
//

EXPCONST extern data_type_definition EmptyType[];
EXPCONST extern data_type_definition StringType[];
EXPCONST EXPGROUP(int16) extern data_type_definition Int16Type[];
EXPCONST EXPGROUP(int32) extern data_type_definition Int32Type[];
EXPCONST EXPGROUP(uint8) extern data_type_definition UInt8Type[];
EXPCONST EXPGROUP(uint16) extern data_type_definition UInt16Type[];
EXPCONST EXPGROUP(uint32) extern data_type_definition UInt32Type[];
EXPCONST EXPGROUP(real32) extern data_type_definition Real32Type[];
EXPCONST EXPGROUP(triple) extern data_type_definition TripleType[];
EXPCONST EXPGROUP(quad) extern data_type_definition QuadType[];
EXPCONST EXPGROUP(transform) extern data_type_definition TransformType[];

END_GRANNY_NAMESPACE;

#include "header_postfix.h"
#define GRANNY_DATA_TYPE_DEFINITION_H
#endif
