// ========================================================================
// $File: //jeffr/granny/rt/granny_file.cpp $
// $DateTime: 2007/10/24 13:15:43 $
// $Change: 16361 $
// $Revision: #40 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#define DEBUG_FILES 0

#if !defined(GRANNY_FILE_H)
#include "granny_file.h"
#endif

#if !defined(GRANNY_MEMORY_H)
#include "granny_memory.h"
#endif

#if !defined(GRANNY_FILE_READER_H)
#include "granny_file_reader.h"
#endif

#if !defined(GRANNY_FILE_FORMAT_H)
#include "granny_file_format.h"
#endif

#if !defined(GRANNY_FILE_COMPRESSOR_H)
#include "granny_file_compressor.h"
#endif

#if !defined(GRANNY_LOG_H)
#include "granny_log.h"
#endif

#if !defined(GRANNY_ASSERT_H)
#include "granny_assert.h"
#endif

#if !defined(GRANNY_CRC_H)
#include "granny_crc.h"
#endif

#if !defined(GRANNY_LIMITS_H)
#include "granny_limits.h"
#endif

#if !defined(GRANNY_PARAMETER_CHECKING_H)
#include "granny_parameter_checking.h"
#endif

#if !defined(GRANNY_MEMORY_FILE_READER_H)
#include "granny_memory_file_reader.h"
#endif

#if !defined(GRANNY_MEMORY_FILE_WRITER_H)
#include "granny_memory_file_writer.h"
#endif

#if !defined(GRANNY_STATISTICS_H)
#include "granny_statistics.h"
#endif

#if !defined(GRANNY_FILE_BUILDER_H)
#include "granny_file_builder.h"
#endif

#if !defined(GRANNY_FILE_OPERATIONS_H)
#include "granny_file_operations.h"
#endif

#if !defined(GRANNY_FILE_WRITER_H)
#include "granny_file_writer.h"
#endif

#if !defined(GRANNY_DATA_TYPE_IO_H)
#include "granny_data_type_io.h"
#endif

#if !defined(GRANNY_FILE_INFO_H)
#include "granny_file_info.h"
#endif

#if !defined(GRANNY_PLATFORM_CONVERT_H)
#include "granny_platform_convert.h"
#endif

#if !defined(GRANNY_CPP_SETTINGS_H)
// This should always be the last header included
#include "granny_cpp_settings.h"
#endif


#undef SubsystemCode
#define SubsystemCode FileReadingLogMessage

USING_GRANNY_NAMESPACE;

#if DEBUG_FILES
#if !defined(GRANNY_STRING_FORMATTING_H)
#include "granny_string_formatting.h"
#endif

static file_writer *Writer = 0;
static void
Dump(char const *FileName)
{
    if(!Writer)
    {
        Writer = NewFileWriter("g:/debug/filedebug.txt", true);
    }
    char Buffer[64];
    int32x Length = ConvertToStringVar(SizeOf(Buffer), Buffer,
                                       "%s\n", FileName);
    Write(*Writer, Length, Buffer);
}
#else
#define Dump(FileName)
#endif

static bool
ReadAt(int32x Offset, file_reader &Reader,
       bool Reverse, int32x Bytes, void *Buffer)
{
    bool Result = false;
    Assert(Buffer);

    if(ReadExactly(Reader, Offset, Bytes, Buffer))
    {
        if(Reverse)
        {
            intaddrx RBytes = Align32(Bytes);
            Reverse32(RBytes, Buffer);
        }

        Result = true;
    }

    return(Result);
}

static void *
LoadFileSection(file_reader &Reader, grn_section const &Section,
                bool FileIsByteReversed, void *DestinationMemory)
{
    void *Result = 0;

    if(Section.ExpandedDataSize > 0)
    {
        intaddrx BlockSize = Align32(Section.ExpandedDataSize);
        if ( DestinationMemory == NULL )
        {
            // No destination specified - allocate your own.
            aggr_allocator Allocator;
            InitializeAggrAlloc(Allocator);

            SetAggrAlignment(Allocator, Section.InternalAlignment);
            AggrAllocSizePtr(Allocator, BlockSize, Result);
            EndAggrAlloc(Allocator);
        }
        else
        {
            Result = DestinationMemory;
            // Better hope that was aligned right.
            Assert ( ( uintaddrx(Result) & ( (uintaddrx)Section.InternalAlignment - 1 ) ) == 0 );
        }

        if ( Result != NULL )
        {
            if(Section.Format == NoCompression)
            {
                Assert(Section.DataSize <= Section.ExpandedDataSize);
                ReadAt(Section.DataOffset, Reader,
                       false, Section.DataSize, Result);
            }
            else
            {
                compression_type const Format =
                    (compression_type)Section.Format;
                void *TempBuffer = AllocateSize(
                    Section.DataSize +
                    GetCompressedBytesPaddingSize(Format));
                if(TempBuffer)
                {
                    ReadAt(Section.DataOffset, Reader,
                           false, Section.DataSize,
                           TempBuffer);

                    if (!DecompressData(Format, FileIsByteReversed,
                                        Section.DataSize, TempBuffer,
                                        Section.First16Bit, Section.First8Bit,
                                        Section.ExpandedDataSize, Result))
                    {
                        Deallocate(Result);
                        Result = 0;
                    }
                }

                Deallocate(TempBuffer);
            }
        }
    }

    return(Result);
}

bool GRANNY
ReadFileSectionInPlace(file_reader &Reader, file &File, int32x SectionIndex, void *DestinationMemory)
{
    COUNT_BLOCK("ReadFileSectionInPlace");

    CheckPointerNotNull(File.Header, return false);
    CheckCountedInt32(SectionIndex, File.SectionCount, return false);

    if(!File.Sections[SectionIndex])
    {
        grn_section const *SectionArray = GetGRNSectionArray(*File.Header);
        Assert(SectionArray);

        grn_section const &Section = SectionArray[SectionIndex];
        File.Sections[SectionIndex] =
            LoadFileSection(Reader, Section, File.IsByteReversed, DestinationMemory);
        File.IsUserMemory[SectionIndex] = ( DestinationMemory != NULL ) ? true : false;

        if(File.Sections[SectionIndex] && File.IsByteReversed)
        {
            ReverseSection(Section.First16Bit, Section.First8Bit,
                           Section.ExpandedDataSize,
                           File.Sections[SectionIndex]);
        }

        File.Marshalled[SectionIndex] = !File.IsByteReversed;

        return (Section.ExpandedDataSize == 0 ||
                File.Sections[SectionIndex] != NULL);
    }
    else
    {
        Log1(WarningLogMessage, FileReadingLogMessage,
             "Ignored request to read already populated file section %d",
             SectionIndex);

        // Note: Kind of a toss up what to do here, but we'll return true,
        // since the section is loaded.
        return true;
    }
}

bool GRANNY
ReadFileSection(file_reader &Reader, file &File, int32x SectionIndex)
{
    return ReadFileSectionInPlace(Reader, File, SectionIndex, NULL);
}


static bool
LoadMarshallingArray(file_reader &Reader, grn_section const &Section,
                     bool FileIsByteReversed,
                     grn_mixed_marshalling_fixup** Array)
{
    CheckPointerNotNull(Array, return false);
    if (Section.MixedMarshallingFixupArrayCount == 0)
    {
        *Array = 0;
        return true;
    }

    grn_mixed_marshalling_fixup *FixupArray =
        AllocateArray(Section.MixedMarshallingFixupArrayCount,
                      grn_mixed_marshalling_fixup);
    if (FixupArray)
    {
        ReadAt(Section.MixedMarshallingFixupArrayOffset, Reader,
               FileIsByteReversed,
               Section.MixedMarshallingFixupArrayCount *
               SizeOf(grn_mixed_marshalling_fixup),
               FixupArray);
    }

    *Array = FixupArray;
    return (FixupArray != 0);
}

bool GRANNY
LoadFixupArray(file_reader &Reader, grn_section const &Section,
               bool FileIsByteReversed,
               grn_pointer_fixup** Array)
{
    CheckPointerNotNull(Array, return false);
    if (Section.PointerFixupArrayCount == 0)
    {
        *Array = 0;
        return true;
    }

    grn_pointer_fixup *FixupArray =
        AllocateArray(Section.PointerFixupArrayCount, grn_pointer_fixup);
    if(FixupArray)
    {
        ReadAt(Section.PointerFixupArrayOffset, Reader,
               FileIsByteReversed,
               Section.PointerFixupArrayCount * SizeOf(grn_pointer_fixup),
               FixupArray);
    }

    *Array = FixupArray;
    return (FixupArray != 0);
}



// A word of explanation here.
// Why do we do fixup in two phases, with pointer fixup
// happening twice? Well, I'll tell you.
// If you have a data_type_definition that refers to another
// data_type_definition (such as an array or an inline member),
// and the structure it lives in is a mixed-marshalling type
// (because it contains int16s and int8s for example),
// then you need to do mixed marshalling on it. This needs to
// walk the type trees of those structures. But when walking
// the type trees, the pointers between the trees need to have
// been fixed up (or the pointers are bogus). So we first
// fix the pointers up, then we do mixed marshalling.
// However, mixed marshalling can then mangle pointers
// that actually live in the data, e.g. if you have a
// structure that is an int and then a pointer, that will
// be marked as int32 marshalling, and the whole 8 bytes
// will be marshalled, destroying the pointer that we already
// fixed up. So the solution is to fix up the pointers
// twice. Ideally, we wouldn't do this, instead we'd
// actually walk the structures and marshall the int32 but not
// the pointer. But that's not the way the code currently works.
// The other problem is that 90% of Granny's structures would
// then be mixed marshalling, which is far far slower than
// just zapping through them all with Reverse32 and
// then fixing the pointers up. So the brute-force
// approach may actually be the quickest one.
//
// Also, we need to do phase 1 (pointer fixup) of all the
// sections before we can do phase 2 (mixed marshalling, then
// pointer fixup), because we might have a granny_variant with a
// type descriptor stored in section 3 that refers to a data
// chunk in section 1.
//
// TODO: fix the marshalling so that it doesn't re-marshall
// pointers, meaning we can remove the second pointer fixup.
// Unless the alternative is slower (see above).

void GRANNY
FixupFileSectionPhase1(file &File, int32x SectionIndex,
                       grn_pointer_fixup const* PointerFixupArray)
{
    COUNT_BLOCK("FixupFileSectionPhase1");

    CheckPointerNotNull(File.Header, return);
    CheckCountedInt32(SectionIndex, File.SectionCount, return);

    grn_section const *SectionArray = GetGRNSectionArray(*File.Header);
    Assert(SectionArray);

    grn_section const &Section = SectionArray[SectionIndex];

    if(Section.PointerFixupArrayCount)
    {
        CheckPointerNotNull(PointerFixupArray, return);

        GRNFixUp(Section.PointerFixupArrayCount,
                 PointerFixupArray, (void const **)File.Sections,
                 File.Sections[SectionIndex]);
    }
}


bool GRANNY
FixupFileSectionPhase2(file_reader &Reader, file &File, int32x SectionIndex,
                       grn_pointer_fixup const* PointerFixupArray)
{
    COUNT_BLOCK("FixupFileSectionPhase2");

    CheckPointerNotNull(File.Header, return false);
    CheckCountedInt32(SectionIndex, File.SectionCount, return false);

    grn_section const *SectionArray = GetGRNSectionArray(*File.Header);
    Assert(SectionArray);

    grn_section const &Section = SectionArray[SectionIndex];

    if(File.IsByteReversed &&
       Section.MixedMarshallingFixupArrayCount &&
       !File.Marshalled[SectionIndex])
    {
        grn_mixed_marshalling_fixup *MixedMarshallingFixupArray;
        if (!LoadMarshallingArray(Reader, Section, File.IsByteReversed,
                                  &MixedMarshallingFixupArray))
        {
            Log1(ErrorLogMessage, FileReadingLogMessage,
                 "FixupFileSectionPhase2: unable to load marshalling array for section: %d",
                 SectionIndex);
            return false;
        }

        if (MixedMarshallingFixupArray)
        {
            GRNMarshall(Section.MixedMarshallingFixupArrayCount,
                        MixedMarshallingFixupArray, (void const **)File.Sections,
                        File.Sections[SectionIndex]);

            File.Marshalled[SectionIndex] = true;
            Deallocate(MixedMarshallingFixupArray);
        }

        // Marshalling may have destroyed the pointers. Do the fixup again.
        if(Section.PointerFixupArrayCount)
        {
            CheckPointerNotNull(PointerFixupArray, return false);
            GRNFixUp(Section.PointerFixupArrayCount,
                     PointerFixupArray, (void const **)File.Sections,
                     File.Sections[SectionIndex]);
        }
    }

    return true;
}

static grn_file_header *
ReadFileHeader(file_reader &Reader,
               int32x FilePosition,
               bool &IsByteReversed,
               bool &IsPointerSizeDifferent,
               grn_file_magic_value* FileMagicValue)
{
    grn_file_magic_value DiscardMagic;
    if (FileMagicValue == NULL)
    {
        FileMagicValue = &DiscardMagic;
    }

    if(ReadExactly(Reader, FilePosition, sizeof(grn_file_magic_value), FileMagicValue))
    {
        FilePosition += sizeof(grn_file_magic_value);

        uint32 HeaderSize;
        if(IsGrannyFile(*FileMagicValue, HeaderSize, IsByteReversed, IsPointerSizeDifferent))
        {
            NormalizeMagicValue(*FileMagicValue);

            // TODO: Compressed headers
            if(FileMagicValue->HeaderFormat == 0)
            {
                int32x HeaderAllocation = HeaderSize;
                if (SizeOf(grn_file_header) > HeaderSize)
                    HeaderAllocation = SizeOf(grn_file_header);

                grn_file_header *Header = (grn_file_header *)
                    AllocateSize(HeaderAllocation);
                if(Header)
                {
                    ZeroStructure(*Header);
                    if(ReadAt(FilePosition, Reader, IsByteReversed,
                              HeaderSize, Header))
                    {
                        FilePosition += HeaderSize;
                        return(Header);
                    }
                    else
                    {
                        Log0(ErrorLogMessage, FileReadingLogMessage,
                             "Unable to read file header");
                    }

                    Deallocate(Header);
                }
            }
            else
            {
                Log1(ErrorLogMessage, FileReadingLogMessage,
                     "File has an unrecognized header format (%d)",
                     FileMagicValue->HeaderFormat);
            }
        }
        else
        {
            Log0(ErrorLogMessage, FileReadingLogMessage,
                 "File is not a Granny file");
        }
    }
    else
    {
        Log0(ErrorLogMessage, FileReadingLogMessage,
             "Unable to read magic value");
    }

    return(0);
}

bool GRANNY
FileCRCIsValid(char const *FileName)
{
    bool Result = false;
    CheckPointerNotNull(FileName, return(Result););

    file_reader *Reader = OpenFileReader(FileName);
    if(Reader)
    {
        bool IsByteReversed = false;
        bool PointerSizeDifferent = false;

        grn_file_header *Header = ReadFileHeader(*Reader, 0, IsByteReversed, PointerSizeDifferent, NULL);
        if(Header)
        {
            int32x FilePosition =
                SizeOf(grn_file_magic_value) + Header->SectionArrayOffset;
            uint32 CRC;
            BeginCRC32(CRC);

            uint8 *Buffer = (uint8 *)AllocateSize(CRCCheckBufferSize);
            if(Buffer)
            {
                while(true)
                {
                    int32x ReadSize = ReadAtMost(*Reader, FilePosition, CRCCheckBufferSize, Buffer);
                    if ( ReadSize == 0 )
                    {
                        break;
                    }
                    AddToCRC32(CRC, ReadSize, Buffer);
                    FilePosition += ReadSize;
                }
                EndCRC32(CRC);
                Result = (Header->CRC == CRC);
            }

            Deallocate(Buffer);
        }

        Deallocate(Header);
        CloseFileReader(Reader);
    }
    else
    {
        Log1(ErrorLogMessage, FileReadingLogMessage,
             "Unable to open %s for reading", FileName);
    }

    return(Result);
}

file *GRANNY
ReadEntireFile(char const *FileName)
{
    COUNT_BLOCK("ReadEntireFile");

    Dump(FileName);

    file *Result = 0;
    CheckPointerNotNull(FileName, return(Result));

    file_reader *Reader = OpenFileReader(FileName);
    if(Reader)
    {
        Result = ReadEntireFileFromReader(*Reader);
        CloseFileReader(Reader);
    }
    else
    {
        Log1(ErrorLogMessage, FileReadingLogMessage,
             "Unable to open %s for reading", FileName);
    }

    return(Result);
}

file *GRANNY
ReadEntireFileFromMemory(int32x MemorySize, void *Memory)
{
    file *Result = 0;

    file_reader *Reader = CreateMemoryFileReader(__FILE__, __LINE__,
                                                 0, MemorySize, Memory);
    if(Reader)
    {
        Result = ReadEntireFileFromReader(*Reader);
        CloseFileReader(Reader);
    }

    return(Result);
}

static file *
ReadRestOfFileFromReaderPointerSize(file_reader &Reader,
                                    file* Result,
                                    grn_file_magic_value &FileMV)
{
    grn_section const *SectionArray = GetGRNSectionArray(*Result->Header);
    Assert(SectionArray);

    // Read the sections
    {for(int32x SectionIndex = 0;
         SectionIndex < Result->SectionCount;
         ++SectionIndex)
    {
        Result->Sections[SectionIndex] = 0;
        if (!ReadFileSection(Reader, *Result, SectionIndex))
        {
            FreeFile(Result);
            return 0;
        }

        // Ok, I admit, this is f'ed up.  Since we're going to allow the
        //  pointer conversion to do all of the endian correct, we don't
        //  actually /want/ the sections to be reversed.  Since it's now
        //  in the "correct" format, reverse it again.
        // Todo: this is pretty wacky, can fix?
        if (Result->Sections[SectionIndex] && Result->IsByteReversed)
        {
            grn_section const& Section = SectionArray[SectionIndex];
            ReverseSection(Section.First16Bit, Section.First8Bit,
                           Section.ExpandedDataSize,
                           Result->Sections[SectionIndex]);
        }
    }}

    // Load the fixup arrays and the marshalling
    grn_pointer_fixup **Fixups =
        (grn_pointer_fixup**)AllocateArray(Result->SectionCount, grn_pointer_fixup*);
    grn_mixed_marshalling_fixup **Marshalls =
        (grn_mixed_marshalling_fixup**)AllocateArray(Result->SectionCount, grn_mixed_marshalling_fixup*);
    if (!Fixups || !Marshalls)
    {
        Log0(ErrorLogMessage, FileReadingLogMessage,
             "ReadRestOfFileFromReaderPointerSize: unable to allocate fixup arrays");
        Deallocate(Fixups);
        Deallocate(Marshalls);
        FreeFile(Result);
        return 0;
    }
    SetPtrNULL(Result->SectionCount, Fixups);
    SetPtrNULL(Result->SectionCount, Marshalls);

    bool FixupLoadsSucceeded = true;
    {for(int32x SectionIndex = 0;
         SectionIndex < Result->SectionCount;
         ++SectionIndex)
    {
        grn_section const &Section = SectionArray[SectionIndex];

        if (!LoadFixupArray(Reader, Section,
                           Result->IsByteReversed, &Fixups[SectionIndex]))
        {
            FixupLoadsSucceeded = false;
            break;
        }

        if (!LoadMarshallingArray(Reader, Section,
                                  Result->IsByteReversed, &Marshalls[SectionIndex]))
        {
            FixupLoadsSucceeded = false;
            break;
        }
    }}
    if (!FixupLoadsSucceeded)
    {
        {for(int32x SectionIndex = 0;
             SectionIndex < Result->SectionCount;
             ++SectionIndex)
        {
            Deallocate(Fixups[SectionIndex]);
            Deallocate(Marshalls[SectionIndex]);
        }}

        Deallocate(Fixups);
        Deallocate(Marshalls);
        FreeFile(Result);

        return 0;
    }

    // Convert the file
    int32x SourcePointerSize = 0, DestPointerSize = 0;
    bool   SourceLittleEndian = false, DestLittleEndian = false;
    bool ValidMagic = GetMagicValueProperties(FileMV,
                                              &SourcePointerSize,
                                              &SourceLittleEndian);
    Assert(ValidMagic); // this should never, ever happen
    GetThisPlatformProperties(&DestPointerSize, &DestLittleEndian);

    if (PlatformConversion(SourcePointerSize, SourceLittleEndian,
                           DestPointerSize, DestLittleEndian, false,
                           Result, Fixups, Marshalls))
    {
        // Fix up the section pointers, phase 1
        {for(int32x SectionIndex = 0;
            SectionIndex < Result->SectionCount;
                ++SectionIndex)
        {
            FixupFileSectionPhase1(*Result, SectionIndex, Fixups[SectionIndex]);
        }}

        // Fix up the section pointers, phase 2
        FixupLoadsSucceeded = true;
        {for(int32x SectionIndex = 0;
            SectionIndex < Result->SectionCount;
            ++SectionIndex)
        {
            if (!FixupFileSectionPhase2(Reader, *Result, SectionIndex, Fixups[SectionIndex]))
            {
                // Tested below
                FixupLoadsSucceeded = false;
                break;
            }
        }}
    }
    else
    {
        Log0(ErrorLogMessage, FileReadingLogMessage,
             "ReadRestOfFileFromReaderPointerSize: PlatformConversion failed");
        // Tested below
        FixupLoadsSucceeded = false;
    }


    // Lose the fixups and the marshalls.
    {for(int32x SectionIndex = 0;
         SectionIndex < Result->SectionCount;
         ++SectionIndex)
    {
        Deallocate(Fixups[SectionIndex]);
        Deallocate(Marshalls[SectionIndex]);
    }}
    Deallocate(Fixups);
    Deallocate(Marshalls);

    if (!FixupLoadsSucceeded)
    {
        FreeFile(Result);
        Result = 0;
    }
    return Result;
}


file *GRANNY
ReadEntireFileFromReader(file_reader &Reader)
{
    COUNT_BLOCK("ReadEntireFileFromReader");

    file *Result = ReadPartialFileFromReader(Reader);
    if(Result)
    {
        // If the file is a different pointer size, then we need to do
        // some compilicated shizzle.  Shunt off to a different
        // function.  Note that since we can now write out files that
        // have the correct byte-order, regardless of source platform,
        // we now issue warnings for files that require marshalling.
        int32x SourcePointerSize = 0;
        bool   SourceLittleEndian = false;
        int32x DestPointerSize  = 0;
        bool   DestLittleEndian = false;
        bool ValidMagic = GetMagicValueProperties(*Result->SourceMagicValue,
                                                  &SourcePointerSize,
                                                  &SourceLittleEndian);
        Assert(ValidMagic);  // this should never, ever happen
        GetThisPlatformProperties(&DestPointerSize, &DestLittleEndian);

        if (SourcePointerSize != DestPointerSize)
        {
            Log2(WarningLogMessage, FileReadingLogMessage,
                 "Converting pointer size from %d to %d.  This is EXTREMELY SLOW!",
                 SourcePointerSize, DestPointerSize);

            return ReadRestOfFileFromReaderPointerSize(Reader, Result, *Result->SourceMagicValue);
        }

        if (SourceLittleEndian != DestLittleEndian)
        {
            Log0(WarningLogMessage, FileReadingLogMessage,
                 "Endian mismatch between current platform and source.  File will be endian marshalled.");
        }


        // Read the sections
        int32x const SectionCount = Result->SectionCount;
        {for(int32x SectionIndex = 0; SectionIndex < SectionCount; ++SectionIndex)
        {
            Result->Sections[SectionIndex] = 0;
            if (!ReadFileSection(Reader, *Result, SectionIndex))
            {
                Log1(ErrorLogMessage, FileReadingLogMessage,
                     "ReadEntireFileFromReader: Failed to load section: %d", SectionIndex);
                FreeFile(Result);
                return 0;
            }
        }}

        // Load the fixup arrays
        grn_pointer_fixup **Fixups =
            (grn_pointer_fixup**)AllocateArray(SectionCount, grn_pointer_fixup*);
        if (Fixups == NULL)
        {
            FreeFile(Result);
            return 0;
        }
        SetPtrNULL(SectionCount, Fixups);

        grn_section const *SectionArray = GetGRNSectionArray(*Result->Header);
        Assert(SectionArray);

        bool FixupsLoaded = true;
        {for(int32x SectionIndex = 0; SectionIndex < SectionCount; ++SectionIndex)
        {
            grn_section const &Section = SectionArray[SectionIndex];
            if (!LoadFixupArray(Reader, Section, Result->IsByteReversed,
                                &Fixups[SectionIndex]))
            {
                FixupsLoaded = false;
                break;
            }
        }}

        if (FixupsLoaded == true)
        {
            // Fix up the section pointers, phase 1
            {for(int32x SectionIndex = 0; SectionIndex < SectionCount; ++SectionIndex)
            {
                FixupFileSectionPhase1(*Result, SectionIndex, Fixups[SectionIndex]);
            }}

            //--- TODO64bit: not actually necessary.  Phase2 is a nop
            // Fix up the section pointers, phase 2
            {for(int32x SectionIndex = 0; SectionIndex < SectionCount; ++SectionIndex)
            {
                if (!FixupFileSectionPhase2(Reader, *Result, SectionIndex, Fixups[SectionIndex]))
                {
                    Log0(ErrorLogMessage, FileReadingLogMessage,
                         "ReadEntireFileFromReader: unable to run phase 2 fixup");
                    FreeFile(Result);
                    Result = 0;
                    break;
                }
            }}
        }
        else
        {
            // We failed to properly load a set of pointer fixups.
            // It's necessary to bail at this point.  Note that the fixup freeing below /must/
            // happen though, so just null the result
            FreeFile(Result);
            Result = 0;
        }

        // Lose the fixups
        {for(int32x SectionIndex = 0; SectionIndex < SectionCount; ++SectionIndex)
        {
            Deallocate(Fixups[SectionIndex]);
        }}
        Deallocate(Fixups);
    }

    return Result;
}

file *GRANNY
ReadPartialFileFromReader(file_reader &Reader)
{
    file *Result = 0;

    bool IsByteReversed, IsPointerSizeDifferent;
    grn_file_magic_value FileMagicValue;
    grn_file_header *Header =
        ReadFileHeader(Reader, 0, IsByteReversed,
                       IsPointerSizeDifferent, &FileMagicValue);
    if(Header)
    {
        if(Header->Version != CurrentGRNFileVersion)
        {
            Log2(WarningLogMessage, FileReadingLogMessage,
                 "File is file format revision %d "
                 "(current version is %d)",
                 Header->Version, CurrentGRNFileVersion);
        }

        aggr_allocator Allocator;
        InitializeAggrAlloc(Allocator);

        AggrAllocPtr(Allocator, Result);
        AggrAllocOffsetPtr(Allocator, Result, SourceMagicValue);
        AggrAllocOffsetArrayPtr(Allocator, Result, Header->SectionArrayCount,
                                SectionCount, Sections);
        AggrAllocOffsetArrayPtr(Allocator, Result, Header->SectionArrayCount,
                                SectionCount, Marshalled);
        AggrAllocOffsetArrayPtr(Allocator, Result, Header->SectionArrayCount,
                                SectionCount, IsUserMemory);
        if(EndAggrAlloc(Allocator))
        {
            Result->IsByteReversed = IsByteReversed;
            Result->Header = Header;
            *Result->SourceMagicValue = FileMagicValue;
            Result->ConversionBuffer = 0;

            ZeroArray(Result->SectionCount, Result->Sections);
            ZeroArray(Result->SectionCount, Result->Marshalled);
            ZeroArray(Result->SectionCount, Result->IsUserMemory);
        }
        else
        {
            Deallocate(Header);
            Header = 0;
        }
    }

    return(Result);
}

void GRANNY
FreeFileSection(file &File, int32x SectionIndex)
{
    CheckCountedInt32(SectionIndex, File.SectionCount, return);

    if(File.Sections[SectionIndex])
    {
        if ( !File.IsUserMemory[SectionIndex] )
        {
            Deallocate(File.Sections[SectionIndex]);
        }
        File.IsUserMemory[SectionIndex] = false;
        File.Marshalled[SectionIndex] = false;
        File.Sections[SectionIndex] = 0;
    }
}



void GRANNY
FreeAllFileSections(file &File)
{
    {for(int32x SectionIndex = 0;
         SectionIndex < File.SectionCount;
         ++SectionIndex)
    {
        FreeFileSection(File, SectionIndex);
    }}
}

void GRANNY
FreeFile(file *File)
{
    if(File)
    {
        FreeAllFileSections(*File);
        Deallocate(File->Header);
        Deallocate(File->ConversionBuffer);
        Deallocate(File);
    }
}


int32x GRANNY
GetFileSectionOfLoadedObject(file const &File,
                             void const *Object)
{
    grn_section *SectionArray = GetGRNSectionArray(*File.Header);
    {for ( uint32x SectionNum = 0; SectionNum < File.Header->SectionArrayCount; SectionNum++ )
    {
        char *SectionStart = (char*)File.Sections[SectionNum];
        char *SectionEnd = SectionStart + SectionArray[SectionNum].ExpandedDataSize;
        if ( ( (char*)Object >= SectionStart ) && ( (char*)Object < SectionEnd ) )
        {
            return SectionNum;
        }
    }}
    // Unknown!
    return -1;
}


void GRANNY
GetDataTreeFromFile(file const &File,
                    variant* Result)
{
    CheckPointerNotNull(Result, return);

    Result->Type = (data_type_definition *)DecodeGRNReference(
        (void const **)File.Sections, File.Header->RootObjectTypeDefinition);
    Result->Object = DecodeGRNReference(
        (void const **)File.Sections, File.Header->RootObject);
}

uint32 GRANNY
GetFileTypeTag(file const &File)
{
    CheckPointerNotNull(File.Header, return(0));
    return(File.Header->TypeTag);
}

static file_location
FileLocationFrom(grn_section const *SectionArray, int32x SectionIndex,
                 uint32 Offset)
{
    grn_section const &Section = SectionArray[SectionIndex];

    marshalling_type Marshalling;
    if(Offset < Section.First16Bit)
    {
        Marshalling = Int32Marshalling;
    }
    else if(Offset < Section.First8Bit)
    {
        Marshalling = Int16Marshalling;
        Offset -= Section.First16Bit;
    }
    else
    {
        Marshalling = Int8Marshalling;
        Offset -= Section.First8Bit;
    }

    file_location Location;
    Location.SectionIndex = SectionIndex;
    Location.BufferIndex = GetBufferIndexFor(Marshalling);
    Location.Offset = Offset;

    return(Location);
}

bool GRANNY
RecompressFile(char const *SourceFile, char const *DestFile,
               int32x CompressorMappingCount, int32x const *CompressorMapping)
{
    bool Result = false;

    file_reader *Reader = OpenFileReader(SourceFile);
    if(Reader)
    {
        bool IsByteReversed;
        bool PointerSizeDifferent;
        grn_file_magic_value MagicValue;
        grn_file_header *Header =
            ReadFileHeader(*Reader, 0, IsByteReversed, PointerSizeDifferent, &MagicValue);
        if(Header)
        {
            if(Header->Version != CurrentGRNFileVersion)
            {
                Log2(WarningLogMessage, FileReadingLogMessage,
                     "File is file format revision %d (current version is %d)",
                     Header->Version, CurrentGRNFileVersion);
            }

            if (!IsByteReversed && !PointerSizeDifferent)
            {
                int32x SectionCount = Header->SectionArrayCount;

                file_builder *Builder = BeginFile(SectionCount, Header->TypeTag,
                                                  MagicValue,
                                                  GetTemporaryDirectory(),
                                                  "granny_recompress");
                Assert(Builder);

                {for(int32x TagIndex = 0; TagIndex < GRNExtraTagCount; ++TagIndex)
                {
                    SetFileExtraTag(*Builder, TagIndex,
                                    Header->ExtraTags[TagIndex]);
                }}

                SetFileStringDatabaseCRC(*Builder, Header->StringDatabaseCRC);

                grn_section const *SectionArray = GetGRNSectionArray(*Header);
                Assert(SectionArray);

                {for(int32x SectionIndex = 0; SectionIndex < SectionCount; ++SectionIndex)
                {
                    grn_section const &Section = SectionArray[SectionIndex];

                    int32x CompressionType = Section.Format;
                    if(CompressionType < CompressorMappingCount)
                    {
                        CompressionType = CompressorMapping[Section.Format];
                    }
                    SetFileSectionFormat(*Builder, SectionIndex,
                                         (compression_type)CompressionType,
                                         Section.InternalAlignment);

                    uint8 *SectionData =
                        (uint8 *)LoadFileSection(*Reader, Section, IsByteReversed, NULL);

                    WriteFileChunk(*Builder, SectionIndex,
                                   Int32Marshalling, Section.First16Bit,
                                   SectionData, NULL);
                    WriteFileChunk(*Builder, SectionIndex,
                                   Int16Marshalling, Section.First8Bit - Section.First16Bit,
                                   SectionData + Section.First16Bit, NULL);
                    WriteFileChunk(*Builder, SectionIndex,
                                   Int8Marshalling, Section.ExpandedDataSize - Section.First8Bit,
                                   SectionData + Section.First8Bit, NULL);

                    grn_pointer_fixup *FixupArray;
                    if (!LoadFixupArray(*Reader, Section, IsByteReversed, &FixupArray))
                    {
                        // TODO: handle correctly
                        InvalidCodePath("Out of memory condition in RecompressFile?");
                        return false;
                    }

                    Assert(FixupArray || Section.PointerFixupArrayCount == 0);
                    {for(uint32x FixupIndex = 0;
                         FixupIndex < Section.PointerFixupArrayCount;
                         ++FixupIndex)
                    {
                        grn_pointer_fixup &Fixup = FixupArray[FixupIndex];
                        MarkFileFixup(*Builder,
                                      FileLocationFrom(SectionArray,
                                                       SectionIndex,
                                                       Fixup.FromOffset), 0,
                                      FileLocationFrom(SectionArray,
                                                       Fixup.To.SectionIndex,
                                                       Fixup.To.Offset));
                    }}
                    Deallocate(FixupArray);

                    grn_mixed_marshalling_fixup *MarshallingArray;
                    if (!LoadMarshallingArray(*Reader, Section, IsByteReversed, &MarshallingArray))
                    {
                        // TODO: handle correctly
                        InvalidCodePath("Out of memory condition in RecompressFile?");
                        return false;
                    }

                    Assert(MarshallingArray || Section.MixedMarshallingFixupArrayCount == 0);
                    {for(uint32x FixupIndex = 0;
                         FixupIndex < Section.MixedMarshallingFixupArrayCount;
                         ++FixupIndex)
                    {
                        grn_mixed_marshalling_fixup &Fixup = MarshallingArray[FixupIndex];

                        MarkMarshallingFixup(
                            *Builder,
                            FileLocationFrom(SectionArray, Fixup.Type.SectionIndex, Fixup.Type.Offset),
                            FileLocationFrom(SectionArray, SectionIndex, Fixup.Offset),
                            Fixup.Count);
                    }}
                    Deallocate(MarshallingArray);
                    Deallocate(SectionData);
                }}

                // Fixup the root object and type definitions.  This is somewhat wierd, since
                // we have to backport the flattened reference into the separated buffers, but
                // that's ok.
                {
                    file_location RootObject = {
                        Header->RootObject.SectionIndex, 0,
                        Header->RootObject.Offset
                    };
                    file_location RootType = {
                        Header->RootObjectTypeDefinition.SectionIndex, 0,
                        Header->RootObjectTypeDefinition.Offset
                    };
                    Assert(int32x(RootObject.SectionIndex) < SectionCount);
                    Assert(int32x(RootType.SectionIndex) < SectionCount);

                    section* Sections = Builder->Sections;
                    while (int32x(RootObject.Offset) >=
                           Sections[RootObject.SectionIndex].BufferSize[RootObject.BufferIndex])
                    {
                        RootObject.Offset -=
                            Sections[RootObject.SectionIndex].BufferSize[RootObject.BufferIndex];
                        ++RootObject.BufferIndex;
                        Assert(RootObject.BufferIndex < SectionBufferCount);
                    }

                    while (int32x(RootType.Offset) >=
                           Sections[RootType.SectionIndex].BufferSize[RootType.BufferIndex])
                    {
                        RootType.Offset -=
                            Sections[RootType.SectionIndex].BufferSize[RootType.BufferIndex];
                        ++RootType.BufferIndex;
                        Assert(RootType.BufferIndex < SectionBufferCount);
                    }

                    Builder->RootObject = RootObject;
                    Builder->RootObjectTypeDefinition = RootType;
                }

                // Just in case the filenames for the input and the output are the same, close
                // the file reader before we execute the EndFile.
                CloseFileReader(Reader);
                Result = EndFile(Builder, DestFile);
            }
            else
            {
                Log0(ErrorLogMessage, FileReadingLogMessage,
                     "RecompressFile: only supports recompressing native file types.  "
                     "Must rewrite file using the normal file_builder interface.");
            }

            Deallocate(Header);
        }
        else
        {
            CloseFileReader(Reader);
        }
    }

    return(Result);
}

bool GRANNY
ConvertFileInfoToRaw(file_info* SourceFileInfo,
                     char const *DestFileName)
{
    bool Result = false;

    if(SourceFileInfo)
    {
        int32x FileSectionCount = 1;
        int32x DefaultSectionIndex = 0;

        file_writer *Writer = NewFileWriter(DestFileName, true);
        if(Writer)
        {
            file_builder *Builder =
                BeginFile(FileSectionCount, 0,
                          GRNFileMV_ThisPlatform,
                          GetTemporaryDirectory(),
                          "raw_writing_buffer");

            file_data_tree_writer *DataTreeWriter =
                BeginFileDataTreeWriting(FileInfoType,
                                         SourceFileInfo,
                                         DefaultSectionIndex,
                                         DefaultSectionIndex);
            if(DataTreeWriter)
            {
                SetFileDataTreeFlags(*DataTreeWriter, ExcludeTypeTree);

                Result = true;
                if(!WriteDataTreeToFileBuilder(*DataTreeWriter, *Builder))
                {
                    Result = false;
                }

                if(!EndFileRawToWriter(Builder, *Writer))
                {
                    Result = false;
                }

                EndFileDataTreeWriting(DataTreeWriter);
            }
            else
            {
                AbortFile(Builder);
            }

            DeleteFileWriter(Writer);
        }
    }

    return(Result);
}

bool GRANNY
ConvertFileToRaw(char const *SourceFileName, char const *DestFileName)
{
    bool Result = false;

    file *SourceFile = ReadEntireFile(SourceFileName);
    if(SourceFile)
    {
        // Ask Granny to parse the source file
        file_info *SourceFileInfo = GetFileInfo(*SourceFile);
        Result = ConvertFileInfoToRaw(SourceFileInfo, DestFileName);
        FreeFile(SourceFile);
    }
    else
    {
        Log1(WarningLogMessage, FileReadingLogMessage,
             "Unable to read %s for raw conversion.", SourceFileName);
    }

    return(Result);
}


static void
FreeCompressedSections(file *File,
                       uint8 ***CompressedSections)
{
    {for(int32x SectionIndex = 0;
         SectionIndex < File->SectionCount;
         ++SectionIndex)
    {
        if ((*CompressedSections)[SectionIndex] != 0)
        {
            FreeMemoryWriterBuffer((*CompressedSections)[SectionIndex]);
            (*CompressedSections)[SectionIndex] = 0;
        }
    }}
}


static bool
CreateCompressedSections(file *File,
                         uint8 ***CompressedSections)
{
    grn_section *SectionArray = GetGRNSectionArray(*File->Header);
    Assert(SectionArray);

    bool Result = true;
    {for(int32x SectionIndex = 0;
         SectionIndex < File->SectionCount;
         ++SectionIndex)
    {
        grn_section &Section = SectionArray[SectionIndex];
        if (Section.ExpandedDataSize == 0)
        {
            (*CompressedSections)[SectionIndex] = NULL;
            Section.DataSize = 0;
            continue;
        }

        file_writer *Writer = CreateMemoryFileWriter(__FILE__, __LINE__, FileCopyBufferSize);
        if(Writer)
        {
            file_compressor *Compressor =
                BeginFileCompression(Section.ExpandedDataSize, SectionBufferCount,
                                     (compression_type)Section.Format, File->IsByteReversed,
                                     Writer);
            if(Compressor)
            {
                // Simulate having broken out 32/16/8 buffers
                uint8* SubSectionBuffer[SectionBufferCount];
                int32x SubSectionBufferSize[SectionBufferCount];
                {
                    SubSectionBuffer[BufferIndex32] = (uint8*)File->Sections[SectionIndex];
                    SubSectionBufferSize[BufferIndex32] = Section.First16Bit;

                    SubSectionBuffer[BufferIndex16] =
                        ((uint8*)File->Sections[SectionIndex] +
                         Section.First16Bit);
                    SubSectionBufferSize[BufferIndex16] = (Section.First8Bit -
                                                           Section.First16Bit);

                    SubSectionBuffer[BufferIndex8] =
                        ((uint8*)File->Sections[SectionIndex] +
                         Section.First8Bit);
                    SubSectionBufferSize[BufferIndex8] = (Section.ExpandedDataSize -
                                                          Section.First8Bit);
                }

                {for(uint32 BufferIndex = 0;
                     (BufferIndex < (uint32)SectionBufferCount) && Result;
                     ++BufferIndex)
                {
                    uint8 const* Buffer     = SubSectionBuffer[BufferIndex];
                    int32x const BufferSize = SubSectionBufferSize[BufferIndex];

                    Result = Result && CompressContentsOfMemory(*Compressor, BufferSize, Buffer);
                }}

                // Not a typo that Result is after.  We need to
                // delete the compressor even if we've failed.
                // EndFileCompression does it...
                Result =
                    EndFileCompression(Compressor, Section.DataSize) &&
                    Result;
            }
        }

        // Steal the compressed buffer from the writer.  Note that we snatch
        //  the size at the same time...
        int32x CheckSize = -1;
        Result = Result &&
            StealMemoryWriterBuffer(*Writer, (*CompressedSections)[SectionIndex],
                                    CheckSize);
        Assert((*CompressedSections)[SectionIndex] != 0);
        Assert(CheckSize == (int32x)Section.DataSize);

        DeleteFileWriter(Writer);
    }}

    return Result;
}


bool GRANNY
PlatformConvertReaderToWriter(file_reader                &SourceReader,
                              file_writer                &DestWriter,
                              grn_file_magic_value const &DestMagicValue,
                              bool                       ExcludeTypeTree)
{
    file *File = ReadPartialFileFromReader(SourceReader);
    if (!File)
    {
        Log0(ErrorLogMessage, FileReadingLogMessage,
             "PlatformConvertReaderToWriter: ReadPartialFileFromReader failed");
        return false;
    }

    // Log a warning if this is a useless conversion
    bool Ignored;
    if (DoesMagicValueMatch(*File->SourceMagicValue, DestMagicValue, Ignored))
    {
        Log0(WarningLogMessage, FileReadingLogMessage,
             "PlatformConvertReaderToWriter called when Source and Dest Magic values match.");
    }

    int32x SourcePointerSize, DestPointerSize;
    bool   SourceLittleEndian, DestLittleEndian;
    if (!GetMagicValueProperties(*File->SourceMagicValue,
                                 &SourcePointerSize,
                                 &SourceLittleEndian) ||
        !GetMagicValueProperties(DestMagicValue,
                                 &DestPointerSize,
                                 &DestLittleEndian))
    {
        Log0(ErrorLogMessage, FileReadingLogMessage,
             "PlatformConvertReaderToWriter: failed to obtain good platform information");
        return false;
    }

    // Do we need to reverse values on persistence?
    bool const NeedsReversalOnPersist = (DestLittleEndian != (PROCESSOR_LITTLE_ENDIAN != 0));

    // Read the sections
    SetPtrNULL(File->SectionCount, File->Sections);
    {for(int32x SectionIndex = 0;
         SectionIndex < File->SectionCount;
         ++SectionIndex)
    {
        if (!ReadFileSection(SourceReader, *File, SectionIndex))
        {
            FreeFile(File);
            return false;
        }
    }}

    grn_section *SectionArray = GetGRNSectionArray(*File->Header);
    Assert(SectionArray);

    // Load the fixup and marshalling arrays
    grn_pointer_fixup **Fixups =
        (grn_pointer_fixup**)AllocateArray(File->SectionCount, grn_pointer_fixup*);
    grn_mixed_marshalling_fixup **Marshalls =
        (grn_mixed_marshalling_fixup**)AllocateArray(File->SectionCount, grn_mixed_marshalling_fixup*);
    if (!Fixups || !Marshalls)
    {
        Deallocate(Fixups);
        Deallocate(Marshalls);
        FreeFile(File);
        return 0;
    }
    SetPtrNULL(File->SectionCount, Fixups);
    SetPtrNULL(File->SectionCount, Marshalls);

    bool FixupsLoaded = true;
    {for(int32x SectionIndex = 0;
         SectionIndex < File->SectionCount;
         ++SectionIndex)
    {
        grn_section const &Section = SectionArray[SectionIndex];

        if (!LoadFixupArray(SourceReader, Section, File->IsByteReversed,
                            &Fixups[SectionIndex]))
        {
            FixupsLoaded = false;
            break;
        }

        if (!LoadMarshallingArray(SourceReader, Section, File->IsByteReversed,
                                  &Marshalls[SectionIndex]))
        {
            FixupsLoaded = false;
            break;
        }
    }}


    bool Result = false;
    if (FixupsLoaded)
    {
        // Perform the conversion
        Result = PlatformConversion(SourcePointerSize, SourceLittleEndian,
                                    DestPointerSize, DestLittleEndian, ExcludeTypeTree,
                                    File, Fixups, Marshalls);
        if (Result)
        {
            // This is going to look pretty familiar.  We can't use the
            // WriteFile function of the granny_file_builder, since we
            // actually /have/ a file here, rather than a file_builder,
            // but it's the same drill.  Note that we have to do this
            // crappy thing of precompressing the sections rather than
            // compressing them in place since we need to write the bits
            // of the file from the start of the sectionarray to the end
            // of the file linearly for the CRC check.

            // Note that this also fills in the datasize member of the section
            uint8 **CompressedData = AllocateArray(File->SectionCount, uint8*);
            Result = CreateCompressedSections(File, &CompressedData);

            // Compute some basic offsets...
            uint32 const TopOfFile = (uint32)GetWriterPosition(DestWriter);

            uint32x FixedHeaderSize  = (SizeOf(grn_file_magic_value) + SizeOf(grn_file_header));
            uint32x SectionArraySize = File->SectionCount * SizeOf(grn_section);
            uint32x FullHeaderSize   = FixedHeaderSize + SectionArraySize;

            // Position the writer to the section header and start CRCing
            SeekWriterFromCurrentPosition(DestWriter, FixedHeaderSize);
            BeginWriterCRC(DestWriter);

            // Ok, compute the rest of the section fields and write them
            // out.  The only fields that need work are:
            //  DataOffset
            //  PointerFixupArrayOffset;
            //  MixedMarshallingFixupArrayOffset;
            // The rest are valid from the previous read
            int32x CurrentOffset = FullHeaderSize;
            {for(int32x SectionIndex = 0;
                 SectionIndex < File->SectionCount;
                 SectionIndex++)
            {
                grn_section &Section = SectionArray[SectionIndex];

                //--- TODO: reorder so the pointer and marshalls are all contiguous
                Section.PointerFixupArrayOffset = CurrentOffset;
                CurrentOffset += (Section.PointerFixupArrayCount *
                                  SizeOf(grn_pointer_fixup));

                Section.MixedMarshallingFixupArrayOffset = CurrentOffset;
                CurrentOffset += (Section.MixedMarshallingFixupArrayCount *
                                  SizeOf(grn_mixed_marshalling_fixup));

                Section.DataOffset = CurrentOffset;
                CurrentOffset += Section.DataSize;

                if (NeedsReversalOnPersist)
                {
                    Reverse32(SizeOf(Section), &Section);
                }

                Result = Result &&
                    Write(DestWriter, SizeOf(Section), &Section);

                if (NeedsReversalOnPersist)
                {
                    Reverse32(SizeOf(Section), &Section);
                }
            }}

            // Now dump the section data
            {for(int32x SectionIndex = 0;
                 SectionIndex < File->SectionCount;
                 SectionIndex++)
            {
                grn_section &Section = SectionArray[SectionIndex];

                // Write the pointer fixups
                if (Section.PointerFixupArrayCount != 0)
                {
                    Assert(Fixups[SectionIndex]);

                    if (NeedsReversalOnPersist)
                    {
                        Reverse32(SizeOf(grn_pointer_fixup) * Section.PointerFixupArrayCount,
                                  Fixups[SectionIndex]);
                    }

                    Result = Result &&
                        Write(DestWriter,
                              SizeOf(grn_pointer_fixup) * Section.PointerFixupArrayCount,
                              Fixups[SectionIndex]);

                    // Technically, we could ignore this, we don't use
                    // these after they're written, but let's be safe...
                    if (NeedsReversalOnPersist)
                    {
                        Reverse32(SizeOf(grn_pointer_fixup) * Section.PointerFixupArrayCount,
                                  Fixups[SectionIndex]);
                    }
                }

                // Write the marshalls
                if (Section.MixedMarshallingFixupArrayCount != 0)
                {
                    Assert(Marshalls[SectionIndex]);

                    if (NeedsReversalOnPersist)
                    {
                        Reverse32(SizeOf(grn_mixed_marshalling_fixup) * Section.MixedMarshallingFixupArrayCount,
                                  Marshalls[SectionIndex]);
                    }

                    Result = Result &&
                        Write(DestWriter,
                              (SizeOf(grn_mixed_marshalling_fixup) *
                               Section.MixedMarshallingFixupArrayCount),
                              Marshalls[SectionIndex]);

                    // Technically, we could ignore this, we don't use
                    // these after they're written, but let's be safe...
                    if (NeedsReversalOnPersist)
                    {
                        Reverse32(SizeOf(grn_mixed_marshalling_fixup) * Section.MixedMarshallingFixupArrayCount,
                                  Marshalls[SectionIndex]);
                    }
                }

                // Write section data
                if(Section.DataSize != 0)
                {
                    Assert(CompressedData[SectionIndex] != 0);

                    Result = Result &&
                        Write(DestWriter, Section.DataSize, CompressedData[SectionIndex]);
                }
            }}

            // Get rid of these as soon as possible
            FreeCompressedSections(File, &CompressedData);
            Deallocate(CompressedData);
            CompressedData = NULL;

            // All of the data that needs to be CRC'd is.
            uint32 CRC = EndWriterCRC(DestWriter);

            // Seek back to the start and write the magic value & header
            uint32 const TotalFileSize =
                (uint32)(GetWriterPosition(DestWriter) - TopOfFile);
            SeekWriterFromStart(DestWriter, TopOfFile);

            grn_file_magic_value MagicValue = DestMagicValue;
            MagicValue.HeaderSize = FullHeaderSize;
            MagicValue.HeaderFormat = 0;
            MagicValue.Reserved[0] = MagicValue.Reserved[1] = 0;

            grn_file_header FileHeader = *File->Header;
            FileHeader.TotalSize = TotalFileSize;
            FileHeader.CRC = CRC;
            FileHeader.SectionArrayOffset = SizeOf(FileHeader);

            // Here's a tricky bit.  If the platform that we're writing is
            // of different endianness than this platform, then we have to
            // Reverse32 the magic value and the FileHeader
            if (NeedsReversalOnPersist)
            {
                Reverse32(SizeOf(MagicValue), &MagicValue);
                Reverse32(SizeOf(FileHeader), &FileHeader);
            }

            Result = Result &&
                Write(DestWriter, SizeOf(MagicValue), &MagicValue);
            Result = Result &&
                Write(DestWriter, SizeOf(FileHeader), &FileHeader);
        }
        else
        {
            // PlatformConversion failed.  That function will log appropriately
        }
    }
    else
    {
        // Fixup loads failed: Result is off the screen, so reset here... :)
        Result = false;
    }

    // Lose the fixups and marshalls
    {for(int32x SectionIndex = 0; SectionIndex < File->SectionCount; ++SectionIndex)
    {
        Deallocate(Fixups[SectionIndex]);
        Deallocate(Marshalls[SectionIndex]);
    }}
    Deallocate(Fixups);
    Deallocate(Marshalls);
    FreeFile(File);

    return Result;
}


uint32 GRANNY
GetInMemoryFileCRC(file *File)
{
    CheckPointerNotNull(File, return 0);
    CheckPointerNotNull(File->Header, return 0);

    grn_section *Sections = GetGRNSectionArray(*File->Header);

    uint32 CRC;
    BeginCRC32(CRC);
    {for(int32x Section = 0; Section < File->SectionCount; Section++)
    {
        AddToCRC32(CRC, Sections[Section].ExpandedDataSize, File->Sections[Section]);
    }}
    EndCRC32(CRC);

    return CRC;
}
