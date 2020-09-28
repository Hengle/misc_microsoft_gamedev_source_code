#if !defined(GRANNY_VARIANT_BUILDER_H)
#include "header_preamble.h"
// ========================================================================
// $File: //jeffr/granny/rt/granny_variant_builder.h $
// $DateTime: 2007/07/27 17:42:17 $
// $Change: 15625 $
// $Revision: #14 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_NAMESPACE_H)
#include "granny_namespace.h"
#endif

#if !defined(GRANNY_TYPES_H)
#include "granny_types.h"
#endif

BEGIN_GRANNY_NAMESPACE EXPGROUP(VariantBuilderGroup);

struct string_table;
struct data_type_definition;

EXPTYPE struct variant_builder;

EXPAPI GS_SAFE variant_builder *BeginVariant(string_table &StringTableBuilder);
EXPAPI GS_PARAM bool EndVariant(variant_builder *Builder,
                                data_type_definition *&Type,
                                void *&Object);
EXPAPI GS_PARAM void AbortVariant(variant_builder *Builder);

bool VariantIsNonEmpty(variant_builder const& Builder);
EXPAPI GS_READ int32x GetResultingVariantTypeSize(variant_builder const &Builder);
EXPAPI GS_READ int32x GetResultingVariantObjectSize(variant_builder const &Builder);
EXPAPI GS_PARAM bool EndVariantInPlace(variant_builder *Builder,
                                       void *TypeMemory,
                                       data_type_definition *&Type,
                                       void *ObjectMemory,
                                       void *&Object);

EXPAPI GS_PARAM void AddBoolMember(variant_builder &Builder, char const *Name,
                                   bool32 Value);

EXPAPI GS_PARAM void AddIntegerMember(variant_builder &Builder, char const *Name,
                                      int32 Value);
EXPAPI GS_PARAM void AddIntegerArrayMember(variant_builder &Builder, char const *Name,
                                           int32x Width, int32* Value);
EXPAPI GS_PARAM void AddUnsignedIntegerMember(variant_builder &Builder, char const *Name,
                                              uint32 Value);

EXPAPI GS_PARAM void AddScalarMember(variant_builder &Builder, char const *Name,
                                     real32 Value);
EXPAPI GS_PARAM void AddScalarArrayMember(variant_builder &Builder, char const *Name,
                                          int32x Width, real32 const *Value);

EXPAPI GS_PARAM void AddStringMember(variant_builder &Builder, char const *Name,
                                     char const *Value);
EXPAPI GS_PARAM void AddReferenceMember(variant_builder &Builder,
                                        char const *Name,
                                        data_type_definition *Type,
                                        void *Value);
EXPAPI GS_PARAM void AddDynamicArrayMember(variant_builder &Builder,
                                           char const *Name,
                                           int32x Count,
                                           data_type_definition *EntryType,
                                           void *ArrayEntries);

END_GRANNY_NAMESPACE;

#include "header_postfix.h"
#define GRANNY_VARIANT_BUILDER_H
#endif
