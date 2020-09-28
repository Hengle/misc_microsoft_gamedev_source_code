#if !defined(GRANNY_DATA_TYPE_CONVERSION_H)
#include "header_preamble.h"
// ========================================================================
// $File: //jeffr/granny/rt/granny_data_type_conversion.h $
// $DateTime: 2006/10/19 16:35:09 $
// $Change: 13632 $
// $Revision: #12 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_NAMESPACE_H)
#include "granny_namespace.h"
#endif

#if !defined(GRANNY_TYPES_H)
#include "granny_types.h"
#endif

#if !defined(GRANNY_DATA_TYPE_DEFINITION_H)
#include "granny_data_type_definition.h"
#endif

BEGIN_GRANNY_NAMESPACE EXPGROUP(DataTypeConversionGroup);

struct data_type_definition;

EXPAPI GS_READ bool FindMatchingMember(data_type_definition const *SourceType,
                                       void const *SourceObject,
                                       char const *DestMemberName,
                                       variant* Result);

EXPAPI GS_PARAM void ConvertSingleObject(data_type_definition const *SourceType,
                                         void const *SourceObject,
                                         data_type_definition const *DestType,
                                         void *DestObject);
EXPAPI GS_PARAM void MergeSingleObject(data_type_definition const *SourceType,
                                       void const *SourceObject,
                                       data_type_definition const *DestType,
                                       void *DestObject);

EXPAPI GS_SAFE void *ConvertTree(data_type_definition const *SourceType,
                                 void const *SourceTree,
                                 data_type_definition const *DestType);

EXPAPI GS_READ int32x GetConvertedTreeSize(data_type_definition const *SourceType,
                                           void const *SourceTree,
                                           data_type_definition const *DestType);
EXPAPI GS_PARAM void *ConvertTreeInPlace(data_type_definition const *SourceType,
                                         void const *SourceTree,
                                         data_type_definition const *DestType,
                                         void *Memory);

EXPAPI typedef char *rebase_pointers_string_callback(void *Data, uint32 Identifier);

EXPAPI GS_PARAM bool RebasePointers(data_type_definition const *Type,
                                    void *Data,
                                    intaddrx Offset,
                                    bool RebaseStrings);
EXPAPI GS_READ bool RebasePointersStringCallback(data_type_definition const *Type,
                                                 void *Data,
                                                 intaddrx Offset,
                                                 rebase_pointers_string_callback *Callback,
                                                 void *CallbackData);

END_GRANNY_NAMESPACE;

#include "header_postfix.h"
#define GRANNY_DATA_TYPE_CONVERSION_H
#endif
