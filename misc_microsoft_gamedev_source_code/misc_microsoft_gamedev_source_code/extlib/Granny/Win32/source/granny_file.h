#if !defined(GRANNY_FILE_H)
#include "header_preamble.h"
// ========================================================================
// $File: //jeffr/granny/rt/granny_file.h $
// $DateTime: 2007/10/24 13:15:43 $
// $Change: 16361 $
// $Revision: #22 $
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


BEGIN_GRANNY_NAMESPACE EXPGROUP(GRNFileReadingGroup);

struct grn_file_header;
struct grn_file_magic_value;
struct grn_pointer_fixup;
struct grn_section;
struct file_reader;
struct file_writer;
struct file_info;

EXPTYPE_EPHEMERAL struct file
{
    bool IsByteReversed;
    grn_file_header *Header;

    // Note that this magic value represents the platform of the
    // /source file/.  In memory granny_file is always targeted to the
    // current platform.
    grn_file_magic_value* SourceMagicValue;

    // Direct access to the sections
    int32x SectionCount;
    void **Sections;
    bool *Marshalled;
    bool *IsUserMemory;

    // Buffer space for tree conversion
    void *ConversionBuffer;
};

EXPAPI GS_SAFE bool FileCRCIsValid(char const *FileName);

EXPAPI GS_READ file *ReadEntireFile(char const *FileName);
EXPAPI GS_SAFE file *ReadEntireFileFromMemory(int32x MemorySize, void *Memory);
EXPAPI GS_PARAM file *ReadEntireFileFromReader(file_reader &Reader);
EXPAPI GS_PARAM file *ReadPartialFileFromReader(file_reader &Reader);
EXPAPI GS_PARAM void FreeFileSection(file &File, int32x SectionIndex);
EXPAPI GS_PARAM void FreeAllFileSections(file &File);
EXPAPI GS_PARAM bool ReadFileSection(file_reader &Reader,
                                     file &File, int32x SectionIndex);
EXPAPI GS_MODIFY bool ReadFileSectionInPlace(file_reader &Reader,
                                             file &File, int32x SectionIndex, void *Memory);
EXPAPI GS_PARAM void FixupFileSectionPhase1(file &File, int32x SectionIndex,
                                            grn_pointer_fixup const* PointerFixupArray);
EXPAPI GS_PARAM bool FixupFileSectionPhase2(file_reader &Reader,
                                            file &File, int32x SectionIndex,
                                            grn_pointer_fixup const* PointerFixupArray);
EXPAPI GS_PARAM bool LoadFixupArray(file_reader &Reader,
                                    grn_section const &Section,
                                    bool FileIsByteReversed,
                                    grn_pointer_fixup** Array);

EXPAPI GS_PARAM void FreeFile(file *File);

EXPAPI GS_READ int32x GetFileSectionOfLoadedObject(file const &File,
                                                   void const *Object);


EXPAPI GS_READ void GetDataTreeFromFile(file const &File, variant* Result);
EXPAPI GS_READ uint32 GetFileTypeTag(file const &File);

EXPAPI GS_PARAM bool RecompressFile(char const *SourceFile,
                                    char const *DestFile,
                                    int32x CompressorMappingCount,
                                    int32x const *CompressorMapping);
EXPAPI GS_PARAM bool ConvertFileInfoToRaw(file_info* SourceFileInfo,
                                          char const *DestFileName);
EXPAPI GS_PARAM bool ConvertFileToRaw(char const *SourceFileName,
                                      char const *DestFileName);

EXPAPI GS_PARAM bool PlatformConvertReaderToWriter(file_reader &SourceReader,
                                                   file_writer &DestWriter,
                                                   grn_file_magic_value const &DestMagicValue,
                                                   bool ExcludeTypeTree);

EXPAPI GS_READ uint32 GetInMemoryFileCRC(file *File);

END_GRANNY_NAMESPACE;

#include "header_postfix.h"
#define GRANNY_FILE_H
#endif
