#if !defined(GRANNY_FILE_FORMAT_H)
#include "header_preamble.h"
// ========================================================================
// $File: //jeffr/granny/rt/granny_file_format.h $
// $DateTime: 2007/08/08 14:25:23 $
// $Change: 15703 $
// $Revision: #34 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_NAMESPACE_H)
#include "granny_namespace.h"
#endif

#if !defined(GRANNY_TYPES_H)
#include "granny_types.h"
#endif

/*

TODO: There is a superior way to implement the data type stuff in the
file format.  This way would be to dispense with the special code for
handling mixed marshalling, and fixups, etc., and just go to a single
relocation table method.  Everything in the file has a record
indicating it's location, in just a single large table.  Then, places
where pointers are just store indices into that table.  The reason
this is superior, besides the additiional cleanliness, is that it
cleanly handles the conversion case, such that you do not have to do a
sweep through the fixups after conversion to determine if you have to
remap pointers - this is because you can do the fixup _after_ you have
done conversion, and just have conversion overwrite the fixup table
with new locations (if that makes sense).  Conversion would
essentially allocate the nth section of the file, and put all
converted stuff into that section, then overwrite all converted
objects' fixups to point to that section.

*/

BEGIN_GRANNY_NAMESPACE EXPGROUP(GRNFileFormatGroup);

struct pointer_hash;

// Each file starts with a 16-byte magic value and a 4-byte field
// containing the number of bytes in the header.
EXPTYPE struct grn_file_magic_value
{
    uint32 MagicValue[4];
    uint32 HeaderSize;
    uint32 HeaderFormat;
    uint32 Reserved[2];
};

EXPCONST extern grn_file_magic_value const GRNFileMV_Old;

EXPCONST extern grn_file_magic_value const GRNFileMV_32Bit_LittleEndian;
EXPCONST extern grn_file_magic_value const GRNFileMV_32Bit_BigEndian;
EXPCONST extern grn_file_magic_value const GRNFileMV_64Bit_LittleEndian;
EXPCONST extern grn_file_magic_value const GRNFileMV_64Bit_BigEndian;
EXPCONST extern grn_file_magic_value const GRNFileMV_ThisPlatform;

EXPTYPE struct grn_reference
{
    uint32 SectionIndex;
    uint32 Offset;
};

EXPTYPE struct grn_pointer_fixup
{
    uint32 FromOffset;
    grn_reference To;
};

EXPTYPE struct grn_mixed_marshalling_fixup
{
    uint32 Count;
    uint32 Offset;
    grn_reference Type;
};

EXPTYPE struct grn_section
{
    // This is the data for the section, in the format specified
    // (for the list of formats, see granny_file_compressor.h)
    uint32 Format;
    uint32 DataOffset;
    uint32 DataSize;
    uint32 ExpandedDataSize;
    uint32 InternalAlignment;

    // These are for endianness conversion - byte 0 through byte
    // First16Bit are assumed to be 32-bit values (so they are
    // endianness swapped that way), and byte First16Bit through
    // byte First8Bit are assumed to be 16-bit values (so they
    // are endianness swapped that way).  The remaining bytes are
    // left untouched.
    uint32 First16Bit;
    uint32 First8Bit;

    // This is an array of grn_pointer_fixup structures
    uint32 PointerFixupArrayOffset;
    uint32 PointerFixupArrayCount;

    // This is an array of grn_mixed_marshalling_fixup structures
    uint32 MixedMarshallingFixupArrayOffset;
    uint32 MixedMarshallingFixupArrayCount;
};

EXPTYPE enum standard_section_index
{
    StandardMainSection = 0,

    StandardRigidVertexSection = 1,
    StandardRigidIndexSection = 2,

    StandardDeformableVertexSection = 3,
    StandardDeformableIndexSection = 4,

    StandardTextureSection = 5,

    StandardDiscardableSection = 6,
    StandardUnloadedSection = 7,

    StandardSectionCount
};
EXPAPI GS_SAFE char const *GetStandardSectionName(int32x SectionIndex);

EXPTYPE enum grn_type_tag
{
    FirstGRNUserTag = 0,
    LastGRNUserTag = 0x7FFFFFFF,

    FirstGRNStandardTag = 0x80000000,
    LastGRNStandardTag = 0xFFFFFFFF
};
// Changing the GRN file format? Remember to increment this number.
#define CurrentGRNStandardTag (0x80000000 + 38) EXPMACRO

#define CurrentGRNFileVersion 7 EXPMACRO
#define GRNExtraTagCount 4 EXPMACRO
EXPTYPE struct grn_file_header
{
    // Version 0 fields:
    uint32 Version;

    uint32 TotalSize;
    uint32 CRC;

    // Files are just a collection of sections, where each section
    // can be compressed with a compressor and has a list of fixups
    // that point back into itself or into other sections.
    // Note that the offset is from the beginning of the header, not
    // absolute to the entire file, because the section array is
    // considered part of the header.
    uint32 SectionArrayOffset;
    uint32 SectionArrayCount;

    // Some object in some section in the file is the root object
    // that is to be used to traverse all other objects.  This
    // fixup tells where that object is.
    grn_reference RootObjectTypeDefinition;
    grn_reference RootObject;

    // This is the extra type information used when loading a Granny
    // file to determine whether or not any conversion will be necessary
    // to turn the file's data tree into a data tree compatible with the
    // application.
    uint32 TypeTag;
    uint32 ExtraTags[GRNExtraTagCount];

    // New in version 7: allows matching to a string_database, and prevents double
    // reindexing
    uint32 StringDatabaseCRC;

    // Also new in version 7, a couple of unused fields just to make sure we don't have to
    // changing the file version in the future.
    uint32 ReservedUnused[3];

    // If you need to add on to the end here, make sure you bump the
    // version number, and label it
};

EXPAPI GS_SAFE bool IsGrannyFile(grn_file_magic_value const &MagicValue,
                                 uint32x &TotalHeaderSize,
                                 bool &IsByteReversed,
                                 bool &IsPointerSizeDifferent);
EXPAPI GS_READ grn_section *GetGRNSectionArray(grn_file_header const &Header);

EXPAPI GS_PARAM void ReverseSection(int32x First16Bit, int32x First8Bit,
                                    int32x End, void *BufferInit);
EXPAPI GS_PARAM void GRNFixUp(uint32x FixupCount, grn_pointer_fixup const *FixupArray,
                              void const **ToSections, void *FromSection);

EXPAPI GS_PARAM void GRNMarshall(uint32x FixupCount,
                                 grn_mixed_marshalling_fixup const *FixupArray,
                                 void const **ToSections, void *FromSection);

EXPAPI GS_READ void *DecodeGRNReference(void const **Sections,
                                        grn_reference const &Reference);

EXPAPI GS_SAFE bool GetMagicValueForPlatform(int32x PointerSizeInBits,
                                             bool   LittleEndian,
                                             grn_file_magic_value &MagicValue);

bool DoesMagicValueMatch(grn_file_magic_value const& MagicValue,
                         grn_file_magic_value const& Reference,
                         bool& ByteReversed);

EXPAPI GS_SAFE void GetThisPlatformProperties(int32x* PointerSize, bool* LittleEndian);

EXPAPI GS_SAFE bool GetMagicValueProperties(grn_file_magic_value const& MagicValue,
                                            int32x* PointerSize,
                                            bool*   LittleEndian);

void NormalizeMagicValue(grn_file_magic_value& MagicValue);

END_GRANNY_NAMESPACE;

#include "header_postfix.h"
#define GRANNY_FILE_FORMAT_H
#endif
