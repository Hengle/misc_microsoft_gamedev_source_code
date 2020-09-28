#if !defined(GRANNY_DATA_TYPE_IO_H)
#include "header_preamble.h"
// ========================================================================
// $File: //jeffr/granny/rt/granny_data_type_io.h $
// $DateTime: 2007/04/10 12:07:41 $
// $Change: 14746 $
// $Revision: #16 $
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

BEGIN_GRANNY_NAMESPACE EXPGROUP(DataTypeIOGroup);

struct string_table;
struct data_type_definition;
struct file_builder;
struct file;

EXPTYPE struct file_data_tree_writer;
struct grn_file_magic_value;

EXPAPI GS_MODIFY file_data_tree_writer *BeginFileDataTreeWriting(
    data_type_definition const *RootObjectTypeDefinition,
    void* RootObject,
    int32x DefaultTypeSectionIndex,
    int32x DefaultObjectSectionIndex);

EXPAPI GS_MODIFY void PreserveObjectFileSections(file_data_tree_writer& Writer,
                                                 file const *SourceFile);

EXPAPI GS_MODIFY void EndFileDataTreeWriting(file_data_tree_writer *Writer);

EXPTYPE enum file_data_tree_flags
{
    ExcludeTypeTree = 0x1
};
EXPAPI GS_MODIFY void SetFileDataTreeFlags(file_data_tree_writer &Writer,
                                           uint32x Flags);
EXPAPI GS_MODIFY void SetFileSectionForObjectsOfType(file_data_tree_writer &Writer,
                                                     data_type_definition const *Type,
                                                     int32x SectionIndex);
EXPAPI GS_MODIFY void SetFileSectionForObject(file_data_tree_writer &Writer,
                                              void const *Object,
                                              int32x SectionIndex);

EXPAPI GS_MODIFY bool WriteDataTreeToFileBuilder(file_data_tree_writer &Writer,
                                                 file_builder &Builder);
EXPAPI GS_MODIFY bool WriteDataTreeToFile(file_data_tree_writer &Writer,
                                          uint32 FileTypeTag,
                                          grn_file_magic_value const &PlatformMagicValue,
                                          char const *FileName,
                                          int32x FileSectionCount);

EXPAPI typedef uint32 file_writer_string_callback(void *Data, char const *String);
EXPAPI GS_MODIFY void SetFileWriterStringCallback(
    file_data_tree_writer &Writer,
    file_writer_string_callback *Callback, void *Data);

END_GRANNY_NAMESPACE;

#include "header_postfix.h"
#define GRANNY_DATA_TYPE_IO_H
#endif
