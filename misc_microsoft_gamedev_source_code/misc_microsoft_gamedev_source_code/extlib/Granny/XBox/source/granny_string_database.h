#if !defined(GRANNY_STRING_DATABASE_H)
#include "header_preamble.h"
// ========================================================================
// $File: //jeffr/granny/rt/granny_string_database.h $
// $DateTime: 2007/03/15 13:47:54 $
// $Change: 14546 $
// $Revision: #2 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_DATA_TYPE_DEFINITION_H)
#include "granny_data_type_definition.h"
#endif

BEGIN_GRANNY_NAMESPACE EXPGROUP(StringGroup);

struct file;

EXPTYPE struct string_database
{
    int32 StringCount;
    char **Strings;

    uint32 DatabaseCRC;

    // Extended data
    variant ExtendedData;
};
EXPCONST EXPGROUP(string_database) extern data_type_definition StringDatabaseType[];

EXPAPI GS_READ string_database *GetStringDatabase(file &File);

EXPAPI GS_PARAM bool RemapFileStrings(file& File, string_database& StringDatabase);

EXPAPI char *RebaseToStringDatabase(void *Data, uint32 Identifier);

END_GRANNY_NAMESPACE;

#include "header_postfix.h"
#define GRANNY_STRING_DATABASE_H
#endif
