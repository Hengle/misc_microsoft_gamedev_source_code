// ========================================================================
// $File: //jeffr/granny/rt/granny_string_database.cpp $
// $DateTime: 2007/11/14 18:07:24 $
// $Change: 16557 $
// $Revision: #5 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================
#if !defined(GRANNY_STRING_DATABASE_H)
#include "granny_string_database.h"
#endif

#if !defined(GRANNY_ASSERT_H)
#include "granny_assert.h"
#endif

#if !defined(GRANNY_DATA_TYPE_CONVERSION_H)
#include "granny_data_type_conversion.h"
#endif

#if !defined(GRANNY_FILE_H)
#include "granny_file.h"
#endif

#if !defined(GRANNY_FILE_FORMAT_H)
#include "granny_file_format.h"
#endif

#if !defined(GRANNY_LOG_H)
#include "granny_log.h"
#endif

#if !defined(GRANNY_POINTER_HASH_H)
#include "granny_pointer_hash.h"
#endif

#if !defined(GRANNY_PARAMETER_CHECKING_H)
#include "granny_parameter_checking.h"
#endif

#if !defined(GRANNY_STATISTICS_H)
#include "granny_statistics.h"
#endif

#if !defined(GRANNY_CPP_SETTINGS_H)
// This should always be the last header included
#include "granny_cpp_settings.h"
#endif

#undef SubsystemCode
#define SubsystemCode StringTableLogMessage

USING_GRANNY_NAMESPACE;

BEGIN_GRANNY_NAMESPACE;

data_type_definition StringDatabaseType[] =
{
    {ReferenceToArrayMember, "Strings", StringType},
    {UInt32Member, "DatabaseCRC"},
    {VariantReferenceMember, "ExtendedData"},

    {EndMember}
};

struct remapping_context
{
    file* File;
    string_database* StringDB;

    pointer_hash* Hash;
};

END_GRANNY_NAMESPACE;

string_database *GRANNY
GetStringDatabase(file &File)
{
    variant Root;
    GetDataTreeFromFile(File, &Root);

    uint32 TypeTag = File.Header->TypeTag;
    if(TypeTag == CurrentGRNStandardTag)
    {
        return((string_database *)Root.Object);
    }
    else
    {
        if(!File.ConversionBuffer)
        {
            Log2(WarningLogMessage, FileReadingLogMessage,
                 "File has run-time type tag of 0x%x, which doesn't match this "
                 "version of Granny (0x%x).  Automatic conversion will "
                 "be attempted.", TypeTag, CurrentGRNStandardTag);

            File.ConversionBuffer =
                ConvertTree(Root.Type, Root.Object, StringDatabaseType);
        }

        return (string_database*)File.ConversionBuffer;
    }
}

static bool
RemapTypeStringPointers(remapping_context& Context, data_type_definition* TypePtr)
{
    if (!HashedPointerKeyExists(*Context.Hash, TypePtr))
    {
        AddPointerToHash(*Context.Hash, TypePtr, 0);

        data_type_definition* Walk = TypePtr;
        while (Walk->Type != EndMember)
        {
            intaddrx Idx = (intaddrx)Walk->Name;
            if (Idx < 0 || Idx >= Context.StringDB->StringCount)
            {
                Log0(ErrorLogMessage, StringLogMessage,
                     "Out of range string index found in type walk, has this file been remapped already?");
                return false;
            }

            Walk->Name = Context.StringDB->Strings[Idx];
            ++Walk;
        }

        // Rewalk, but now descend into the referenced types...
        Walk = TypePtr;
        while (Walk->Type != EndMember)
        {
            if (Walk->ReferenceType != NULL)
                RemapTypeStringPointers(Context, Walk->ReferenceType);

            ++Walk;
        }
    }

    return true;
}


static bool
RemapStringPointers(remapping_context& Context,
                    data_type_definition* TypePtr,
                    uint8* ObjectPtr)
{
    Assert(Context.File && Context.StringDB);
    Assert(TypePtr && ObjectPtr);

    // The first thing to do is to walk the type, and remap the "Name" strings contained
    // therein.  This has to be recursive on the type, because we don't recurse into
    // sub-objects if the Object pointer of a ReferenceMember, say, is NULL.  Variant
    // members will be handled in the course of the tree traversal.
    if (!RemapTypeStringPointers(Context, TypePtr))
        return false;

    // Have we already walked this object?
    if (HashedPointerKeyExists(*Context.Hash, ObjectPtr))
        return true;
    AddPointerToHash(*Context.Hash, ObjectPtr, 0);

    // Ok, so we have the names remapped properly, let's start walking the object data.
    bool Result = true;

    uint8* WalkObj = ObjectPtr;
    data_type_definition* WalkType = TypePtr;
    while (Result && WalkType->Type != EndMember)
    {
        switch (WalkType->Type)
        {
            case InlineMember:
            {
                Result = (Result && RemapStringPointers(Context,
                                                        WalkType->ReferenceType,
                                                        WalkObj));
            } break;

            case ReferenceMember:
            {
                uint8* ReferencedObject = *((uint8**)WalkObj);

                if (ReferencedObject != NULL)
                {
                    Result = (Result && RemapStringPointers(Context,
                                                            WalkType->ReferenceType,
                                                            ReferencedObject));
                }
            } break;

            case ReferenceToArrayMember:
            {
                int32 ArrayCount       = *((int32*)WalkObj);
                uint8* ReferencedArray = *((uint8**)(WalkObj + sizeof(int32)));

                if (ArrayCount != 0 && ReferencedArray != NULL)
                {
                    {for(int32 Idx = 0; Idx < ArrayCount && Result; ++Idx)
                    {
                        Result = (Result && RemapStringPointers(Context,
                                                                WalkType->ReferenceType,
                                                                ReferencedArray));
                        ReferencedArray += GetTotalObjectSize(WalkType->ReferenceType);
                    }}
                }
            } break;

            case ArrayOfReferencesMember:
            {
                int32 ArrayCount        = *((int32*)WalkObj);
                uint8** ReferencedArray = *((uint8***)(WalkObj + sizeof(int32)));

                if (ArrayCount != 0 && ReferencedArray != NULL)
                {
                    {for(int32 Idx = 0; Idx < ArrayCount && Result; ++Idx)
                    {
                        if (ReferencedArray[Idx])
                        {
                            Result = (Result && RemapStringPointers(Context,
                                                                    WalkType->ReferenceType,
                                                                    ReferencedArray[Idx]));
                        }
                    }}
                }
            } break;

            case VariantReferenceMember:
            {
                data_type_definition* VariantType = *((data_type_definition**)WalkObj);
                uint8*                VariantObj  = *((uint8**)(WalkObj + sizeof(void*)));

                // Ensure that this always happens
                if (VariantType)
                {
                    if (!RemapTypeStringPointers(Context, VariantType))
                        return false;
                }

                if (VariantType && VariantObj)
                {
                    Result = (Result && RemapStringPointers(Context, VariantType,
                                                            VariantObj));
                }
            } break;

            case ReferenceToVariantArrayMember:
            {
                data_type_definition* VariantType = *((data_type_definition**)WalkObj);
                int32 ArrayCount = *((int32*)(WalkObj + sizeof(void*)));
                uint8** ReferencedArray = *((uint8***)(WalkObj + sizeof(int32) + sizeof(void*)));

                // Ensure that this always happens
                if (VariantType)
                {
                    if (!RemapTypeStringPointers(Context, VariantType))
                        return false;
                }
                
                if (ArrayCount != 0 && ReferencedArray && VariantType)
                {
                    {for(int32 Idx = 0; Idx < ArrayCount && Result; ++Idx)
                    {
                        if (ReferencedArray[Idx])
                        {
                            Result = (Result && RemapStringPointers(Context,
                                                                    VariantType,
                                                                    ReferencedArray[Idx]));
                        }
                    }}
                }
            } break;

            case StringMember:
            {
                // This is the easy one.
                char** StringPtr = (char**)WalkObj;

                intaddrx Idx = (intaddrx)*StringPtr;
                if (Idx < 0 || Idx >= Context.StringDB->StringCount)
                {
                    Log0(ErrorLogMessage, StringLogMessage,
                         "Out of range string index found in type walk, has this file been remapped already?");
                    Result = false;
                }
                *StringPtr = Context.StringDB->Strings[Idx];
            } break;

            default:
                // nothing to do
                break;
        }

        WalkObj += GetMemberTypeSize(*WalkType);
        ++WalkType;
    }

    return true;
}


bool GRANNY
RemapFileStrings(file& File, string_database& StringDatabase)
{
    COUNT_BLOCK("RemapFileStrings");

    grn_file_header* Header = File.Header;
    CheckCondition(Header->StringDatabaseCRC != 0, return false);
    CheckCondition(Header->StringDatabaseCRC == StringDatabase.DatabaseCRC, return false);

    remapping_context Context;
    Context.File = &File;
    Context.StringDB = &StringDatabase;
    Context.Hash = NewPointerHash();

    bool Result = false;
    if (Context.Hash)
    {
        variant Root;
        GetDataTreeFromFile(*Context.File, &Root);

        Result = RemapStringPointers(Context, Root.Type, (uint8*)Root.Object);
        DeletePointerHash(Context.Hash);

        // Clear the database CRC member, to ensure that this isn't accidentally done
        // twice
        Header->StringDatabaseCRC = 0;
    }

    return Result;
}

char* GRANNY
RebaseToStringDatabase(void *Data, uint32 Identifier)
{
    CheckPointerNotNull(Data, return NULL);
    string_database* StringDB = (string_database*)Data;

    CheckCountedUInt32(Identifier, (uint32)StringDB->StringCount, return NULL);

    return StringDB->Strings[Identifier];
}
