#if !defined(GRANNY_FILE_BUILDER_H)
#include "header_preamble.h"
// ========================================================================
// $File: //jeffr/granny/rt/granny_file_builder.h $
// $DateTime: 2007/04/10 12:07:41 $
// $Change: 14746 $
// $Revision: #15 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_NAMESPACE_H)
#include "granny_namespace.h"
#endif

#if !defined(GRANNY_FILE_COMPRESSOR_H)
#include "granny_file_compressor.h"
#endif

#if !defined(GRANNY_FILE_FORMAT_H)
#include "granny_file_format.h"
#endif

#if !defined(GRANNY_STACK_ALLOCATOR_H)
#include "granny_stack_allocator.h"
#endif

#if !defined(GRANNY_LIMITS_H)
#include "granny_limits.h"
#endif


BEGIN_GRANNY_NAMESPACE EXPGROUP(FileBuilderGroup);

struct file_writer;
struct file;

EXPTYPE struct file_location
{
    uint32 SectionIndex;
    uint32 BufferIndex;
    uint32 Offset;
};


EXPTYPE struct file_fixup;
EXPTYPE struct file_builder;
struct section;
struct file_builder
{
    grn_file_magic_value PlatformMagicValue;
    bool ExcludeTypeTreeOnConversion;

    stack_allocator FixupAllocator;
    stack_allocator MarshallingAllocator;

    uint32 FileTypeTag;
    uint32 StringDatabaseCRC;
    uint32 ExtraTags[GRNExtraTagCount];

    char TemporaryDirectory[MaximumTempFileNameSize];
    char TemporaryFileNameRoot[MaximumTempFileNameSize];
    bool TempFilesOnDisk;


    file_location RootObject;
    file_location RootObjectTypeDefinition;

    int32x SectionCount;
    section *Sections;

    bool FilesAreValid;
};

enum SectionConstants
{
    SectionBufferCount = 3,
    BufferIndex32 = 0,
    BufferIndex16 = 1,
    BufferIndex8 = 2,
};

struct file_fixup;
struct marshalling_fixup;
struct section
{
    grn_section Header;

    int32x BufferSize[SectionBufferCount];
    file_writer *Buffers[SectionBufferCount];

    // Valid if we're using memory buffers rather than temporary
    // files.  We steal the buffers from the memory_file_writers in
    // Buffers[] in that case.  Note that BufferSize is still correct.
    uint8* UncompressedBuffers[SectionBufferCount];

    // Stolen from a MemoryWriter on CompressSection
    uint8* CompressedBuffer;
    int32x CompressedBufferSize;

    file_fixup *FirstFixup;
    file_fixup *LastFixup;

    marshalling_fixup *FirstMarshalling;
    marshalling_fixup *LastMarshalling;

    // This is only used during raw writing
    int32x RawOffset[SectionBufferCount];
};



EXPAPI GS_MODIFY file_builder *BeginFile(int32x SectionCount, uint32 FileTypeTag,
                                         grn_file_magic_value const &PlatformMagicValue,
                                         char const *TemporaryDirectory,
                                         char const *TemporaryFileNameRoot);
EXPAPI GS_SAFE file_builder *BeginFileInMemory(int32x SectionCount, uint32 FileTypeTag,
                                               grn_file_magic_value const &PlatformMagicValue,
                                               int32x MemoryBlockSize);
EXPAPI GS_PARAM bool EndFile(file_builder *Builder, char const *FileName);
EXPAPI GS_PARAM bool EndFileToWriter(file_builder *Builder, file_writer &ToFile);
EXPAPI GS_PARAM bool EndFileRaw(file_builder *Builder, char const *FileName);
EXPAPI GS_PARAM bool EndFileRawToWriter(file_builder *Builder, file_writer &ToFile);
EXPAPI GS_PARAM void AbortFile(file_builder *Builder);

// Each buffer can have its own compression type (or no compression)
EXPAPI GS_PARAM void SetFileSectionFormat(file_builder &Builder, int32x SectionIndex,
                                          compression_type Compression,
                                          int32x Alignment);

EXPAPI GS_MODIFY void PreserveFileSectionFormats(file_builder &Builder,
                                                 file const &SourceFile);


// ReserveBufferSpace returns the space to write into
EXPAPI GS_PARAM void WriteFileChunk(file_builder &Builder, int32x InSectionIndex,
                                    uint32 Marshalling, uint32x Size, void const *Data,
                                    file_location* Result);
EXPAPI GS_SAFE void OffsetFileLocation(file_builder &Builder,
                                       file_location const &Location,
                                       uint32 AdditionalOffset,
                                       file_location* Result);

// Fixups are pointer fixups that patch files after loading to have
// valid pointers into themselves
EXPAPI GS_PARAM file_fixup *MarkFileFixup(file_builder &Builder,
                                          file_location const &From,
                                          int32x FromOffset,
                                          file_location const &To);
EXPAPI GS_PARAM void AdjustFileFixup(file_builder &Builder, file_fixup &Fixup,
                                     file_location const &NewTo);

EXPAPI GS_PARAM void MarkMarshallingFixup(file_builder &Builder,
                                          file_location const &Type,
                                          file_location const &Object,
                                          int32x ArrayCount);

// Every file has a "root object" which is used as the initial parse
// point for the file.
EXPAPI GS_PARAM void MarkFileRootObject(file_builder &Builder,
                                        file_location const &TypeLocation,
                                        file_location const &ObjectLocation);

EXPAPI GS_PARAM void SetFileStringDatabaseCRC(file_builder &Builder,
                                              uint32 DatabaseCRC);

// If you wish to use the file's extra tags, you need to call this
// at some point before writing.
EXPAPI GS_PARAM void SetFileExtraTag(file_builder &Builder,
                                     int32x Index, uint32 Value);

// These are for internal use, so don't call them unless you really
// know what you're doing.
int32x GetBufferIndexFor(uint32x Marshalling);

END_GRANNY_NAMESPACE;

#include "header_postfix.h"
#define GRANNY_FILE_BUILDER_H
#endif
