#if !defined(GRANNY_STRING_TABLE_H)
#include "header_preamble.h"
// ========================================================================
// $File: //jeffr/granny/rt/granny_string_table.h $
// $DateTime: 2006/10/16 14:57:23 $
// $Change: 13583 $
// $Revision: #7 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_DATA_TYPE_DEFINITION_H)
#include "granny_data_type_definition.h"
#endif

BEGIN_GRANNY_NAMESPACE EXPGROUP(StringTableGroup);

EXPTYPE struct string_table;

EXPAPI GS_SAFE string_table *NewStringTable(void);
EXPAPI GS_PARAM void FreeStringTable(string_table *Table);

EXPAPI GS_PARAM char const *MapString(string_table &Table, char const *String);

END_GRANNY_NAMESPACE;

#include "header_postfix.h"
#define GRANNY_STRING_TABLE_H
#endif
