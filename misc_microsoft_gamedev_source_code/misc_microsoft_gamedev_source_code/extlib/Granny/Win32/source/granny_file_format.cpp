// ========================================================================
// $File: //jeffr/granny/rt/granny_file_format.cpp $
// $DateTime: 2007/07/23 15:42:28 $
// $Change: 15572 $
// $Revision: #17 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_FILE_FORMAT_H)
#include "granny_file_format.h"
#endif

#if !defined(GRANNY_ASSERT_H)
#include "granny_assert.h"
#endif

#if !defined(GRANNY_MEMORY_H)
#include "granny_memory.h"
#endif

#if !defined(GRANNY_CONVERSIONS_H)
#include "granny_conversions.h"
#endif

#if !defined(GRANNY_FILE_COMPRESSOR_H)
#include "granny_file_compressor.h"
#endif

#if !defined(GRANNY_DATA_TYPE_IO_H)
#include "granny_data_type_io.h"
#endif

#if !defined(GRANNY_PARAMETER_CHECKING_H)
#include "granny_parameter_checking.h"
#endif

#if !defined(GRANNY_CPP_SETTINGS_H)
// This should always be the last header included
#include "granny_cpp_settings.h"
#endif

#undef SubsystemCode
#define SubsystemCode FileReadingLogMessage

USING_GRANNY_NAMESPACE;

BEGIN_GRANNY_NAMESPACE;

grn_file_magic_value const GRNFileMV_Old = {{0xCAB067B8, 0x0FB16DF8, 0x7E8C7284, 0x1E00195E}};

// Use #defines here to prevent a compiler warning for the ThisPlatform variable
#define MAGIC_32LE {{0xC06CDE29, 0x2B53A4BA, 0xA5B7F525, 0xEEE266F6}}
#define MAGIC_32BE {{0xB595110E, 0x4BB5A56A, 0x502828EB, 0x04B37825}}
#define MAGIC_64LE {{0x5E499BE5, 0x141F636F, 0xA9EB131E, 0xC4EDBE90}}
#define MAGIC_64BE {{0xE3D49531, 0x624FDC20, 0x3AD036CC, 0x89FF82B1}}

grn_file_magic_value const GRNFileMV_32Bit_LittleEndian  = MAGIC_32LE;
grn_file_magic_value const GRNFileMV_32Bit_BigEndian     = MAGIC_32BE;
grn_file_magic_value const GRNFileMV_64Bit_LittleEndian  = MAGIC_64LE;
grn_file_magic_value const GRNFileMV_64Bit_BigEndian     = MAGIC_64BE;

// Set the magic value for this platform, and some handy constants
// TODO: should we move these out to platform/processor.h?
#if PROCESSOR_32BIT_POINTER
  #if PROCESSOR_LITTLE_ENDIAN
     grn_file_magic_value const GRNFileMV_ThisPlatform = MAGIC_32LE;
     int32x const PlatformPointerSize    = 32;
     bool   const PlatformIsLittleEndian = true;
  #else // PROCESSOR_BIG_ENDIAN
     grn_file_magic_value const GRNFileMV_ThisPlatform = MAGIC_32BE;
     int32x const PlatformPointerSize    = 32;
     bool   const PlatformIsLittleEndian = false;
  #endif
#else // PROCESSOR_64BIT_POINTER
  #if PROCESSOR_LITTLE_ENDIAN
     grn_file_magic_value const GRNFileMV_ThisPlatform = MAGIC_64LE;
     int32x const PlatformPointerSize    = 64;
     bool   const PlatformIsLittleEndian = true;
  #else // PROCESSOR_BIG_ENDIAN
     grn_file_magic_value const GRNFileMV_ThisPlatform = MAGIC_64BE;
     int32x const PlatformPointerSize    = 64;
     bool   const PlatformIsLittleEndian = false;
  #endif
#endif


struct _mvPropList {
    const grn_file_magic_value* MagicValuePtr;
    int  PointerSize;
    bool IsLittleEndian;
} MagicValuePropertyList[] = {
    { &GRNFileMV_32Bit_LittleEndian, 32, true  },
    { &GRNFileMV_32Bit_BigEndian,    32, false },
    { &GRNFileMV_64Bit_LittleEndian, 64, true  },
    { &GRNFileMV_64Bit_BigEndian,    64, false },
};

END_GRANNY_NAMESPACE;


bool GRANNY
GetMagicValueForPlatform(int32x PointerSizeInBits,
                         bool   LittleEndian,
                         grn_file_magic_value &MagicValue)
{
    {for(int32x Index = 0; Index < ArrayLength(MagicValuePropertyList); ++Index)
    {
        if (MagicValuePropertyList[Index].PointerSize == PointerSizeInBits &&
            MagicValuePropertyList[Index].IsLittleEndian == LittleEndian)
        {
            MagicValue = *MagicValuePropertyList[Index].MagicValuePtr;
            return true;
        }
    }}

    InvalidCodePath("Should have asked for 32 or 64bit pointers!");
    return false;
}


void GRANNY
GetThisPlatformProperties(int32x* PointerSize, bool* LittleEndian)
{
    // Lame, but I don't want extra #defines
    bool Result = GetMagicValueProperties(GRNFileMV_ThisPlatform,
                                          PointerSize, LittleEndian);
    Assert(Result);
}


// Not too fast, but this isn't really a speed critical function.
// Note that we need the ByteReversed field to handle old format
// files properly.  Sigh.
bool GRANNY
DoesMagicValueMatch(grn_file_magic_value const& MagicValue,
                    grn_file_magic_value const& Reference,
                    bool& ByteReversed)
{
    // Make sure the unroll is still valid...
    CompileAssert(ArrayLength(MagicValue.MagicValue) == 4);

    // Proper ordering first...
    if(Compare(SizeOf(MagicValue.MagicValue),
               MagicValue.MagicValue,
               Reference.MagicValue))
    {
        ByteReversed = false;
        return true;
    }

    // Now test the byte-swapped ordering...
    bool MatchedByteReversed = true;
    {for(int32x Index = 0; Index < ArrayLength(MagicValue.MagicValue); ++Index)
    {
        if (MagicValue.MagicValue[Index] != Reverse32(Reference.MagicValue[Index]))
            MatchedByteReversed = false;
    }}

    // This is kind of cheating, but if MatchedByteReversed is true, then we set the
    // appropriate value.  If it's not, we're indicating no match anyways...
    ByteReversed = MatchedByteReversed;
    return MatchedByteReversed;
}


bool GRANNY
GetMagicValueProperties(grn_file_magic_value const& MagicValue,
                        int32x*                     InputPointerSize,
                        bool*                       InputLittleEndian)
{
    bool DummyLE; int32x DummyPointerSize;
    bool*   LittleEndian = InputLittleEndian ? InputLittleEndian : &DummyLE;
    int32x* PointerSize  = InputPointerSize ? InputPointerSize : &DummyPointerSize;

    bool ByteReversed;
    if (DoesMagicValueMatch(MagicValue, GRNFileMV_Old, ByteReversed))
    {
        // Old format file.  Infer the correct endianness from
        //  the platform endian and the swap value...
        *PointerSize = 32;
        if (ByteReversed)
            *LittleEndian = !PlatformIsLittleEndian;
        else
            *LittleEndian = PlatformIsLittleEndian;
        return true;
    }

    {for(int32x Index = 0; Index < ArrayLength(MagicValuePropertyList); ++Index)
    {
        if (DoesMagicValueMatch(MagicValue, *MagicValuePropertyList[Index].MagicValuePtr, ByteReversed))
        {
            *PointerSize  = MagicValuePropertyList[Index].PointerSize;
            *LittleEndian = MagicValuePropertyList[Index].IsLittleEndian;
            return true;
        }
    }}

    // Not a valid granny magic value
    Log4(WarningLogMessage, FileReadingLogMessage,
         "Magic value (0x%x, 0x%x, 0x%x, 0x%x) doesn't match any known file format",
         MagicValue.MagicValue[0],
         MagicValue.MagicValue[1],
         MagicValue.MagicValue[2],
         MagicValue.MagicValue[3]);
    return false;
}

void GRANNY
NormalizeMagicValue(grn_file_magic_value& MagicValue)
{
    // This looks a little weird, but what we're doing is stripping out the "old" magic
    // value, and replacing it with the appropriate "new" value.
    int32x PointerSizeInBits;
    bool LittleEndian;
    if (GetMagicValueProperties(MagicValue, &PointerSizeInBits, &LittleEndian))
    {
        grn_file_magic_value NewMagicValue;
        GetMagicValueForPlatform(PointerSizeInBits,
                                 LittleEndian,
                                 NewMagicValue);

        // Preserve the header size, header format, and reserved members...
        CompileAssert(sizeof(NewMagicValue.MagicValue) == 4 * sizeof(uint32));
        Copy32(4, NewMagicValue.MagicValue, MagicValue.MagicValue);
    }
}


static char const *SectionNames[] =
{
    "0 - StandardMainSection",

    "1 - StandardRigidVertexSection",
    "2 - StandardRigidIndexSection",

    "3 - StandardDeformableVertexSection",
    "4 - StandardDeformableIndexSection",

    "5 - StandardTextureSection",

    "6 - StandardDiscardableSection",
    "7 - StandardUnloadedSection",
};

char const *GRANNY
GetStandardSectionName(int32x SectionIndex)
{
    CheckCountedInt32(SectionIndex, StandardSectionCount,
                      return("(non-standard section)"));

    return(SectionNames[SectionIndex]);
}

bool GRANNY
IsGrannyFile(grn_file_magic_value const &MagicValue,
             uint32 &TotalHeaderSize,
             bool &IsByteReversed,
             bool &IsPointerSizeDifferent)
{
    bool IsGrannyFile = false;
    TotalHeaderSize = 0;
    IsByteReversed = false;
    IsPointerSizeDifferent = false;

    int32x FilePointerSize;
    bool   FileIsLittleEndian;
    if (GetMagicValueProperties(MagicValue,
                                &FilePointerSize,
                                &FileIsLittleEndian))
    {
        IsGrannyFile = true;

        IsByteReversed         = (FileIsLittleEndian != PlatformIsLittleEndian);
        IsPointerSizeDifferent = (FilePointerSize    != PlatformPointerSize);
        if (IsByteReversed)
        {
            TotalHeaderSize = Reverse32(MagicValue.HeaderSize);
        }
        else
        {
            TotalHeaderSize = MagicValue.HeaderSize;
        }
    }

    return(IsGrannyFile);
}



grn_section *GRANNY
GetGRNSectionArray(grn_file_header const &Header)
{
    return((grn_section *)
           ((uint8 *)&Header + Header.SectionArrayOffset));
}

void GRANNY
ReverseSection(int32x First16Bit, int32x First8Bit,
               int32x End, void *BufferInit)
{
    uint8 *Buffer = (uint8 *)BufferInit;
    Reverse32(First16Bit, Buffer);
    Reverse16(First8Bit - First16Bit, Buffer + First16Bit);
}

void GRANNY
GRNFixUp(uint32x FixupCount, grn_pointer_fixup const *FixupArray,
         void const **ToSections, void *FromSectionInit)
{
    uint8 *FromSection = (uint8 *)FromSectionInit;
    while(FixupCount--)
    {
        grn_pointer_fixup const &Fixup = *FixupArray++;

        CompileAssert(sizeof(uintaddrx) == sizeof(void*));

        uintaddrx &Location = *(uintaddrx*)&FromSection[Fixup.FromOffset];
        Location = (uintaddrx)ToSections[Fixup.To.SectionIndex];
        if(Location)
        {
            Location += Fixup.To.Offset;
        }
    }
}

void GRANNY
GRNMarshall(uint32x FixupCount,
            grn_mixed_marshalling_fixup const *FixupArray,
            void const **ToSections, void *FromSectionInit)
{
    uint8 *FromSection = (uint8 *)FromSectionInit;
    while(FixupCount--)
    {
        grn_mixed_marshalling_fixup const &Fixup = *FixupArray++;

        data_type_definition *Type = (data_type_definition *)
            ((uint8 *)ToSections[Fixup.Type.SectionIndex] +
             Fixup.Type.Offset);
        void *Object = &FromSection[Fixup.Offset];

        ReverseTypeArray(Type, Fixup.Count, Object);
    }
}

void *GRANNY
DecodeGRNReference(void const **Sections, grn_reference const &Reference)
{
    uint8 const *Section = (uint8 const *)Sections[Reference.SectionIndex];
    return((void *)(Section + Reference.Offset));
}
