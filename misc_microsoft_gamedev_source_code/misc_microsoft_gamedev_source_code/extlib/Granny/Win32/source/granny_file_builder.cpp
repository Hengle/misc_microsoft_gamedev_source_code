// ========================================================================
// $File: //jeffr/granny/rt/granny_file_builder.cpp $
// $DateTime: 2007/07/23 15:42:28 $
// $Change: 15572 $
// $Revision: #35 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_FILE_BUILDER_H)
#include "granny_file_builder.h"
#endif

#if !defined(GRANNY_FILE_FORMAT_H)
#include "granny_file_format.h"
#endif

#if !defined(GRANNY_FILE_H)
#include "granny_file.h"
#endif

#if !defined(GRANNY_MEMORY_H)
#include "granny_memory.h"
#endif

#if !defined(GRANNY_LOG_H)
#include "granny_log.h"
#endif

#if !defined(GRANNY_FILE_WRITER_H)
#include "granny_file_writer.h"
#endif

#if !defined(GRANNY_MEMORY_FILE_WRITER_H)
#include "granny_memory_file_writer.h"
#endif

#if !defined(GRANNY_MEMORY_FILE_READER_H)
#include "granny_memory_file_reader.h"
#endif

#if !defined(GRANNY_STRING_FORMATTING_H)
#include "granny_string_formatting.h"
#endif

#if !defined(GRANNY_STRING_H)
#include "granny_string.h"
#endif

#if !defined(GRANNY_ASSERT_H)
#include "granny_assert.h"
#endif

#if !defined(GRANNY_PARAMETER_CHECKING_H)
#include "granny_parameter_checking.h"
#endif

#if !defined(GRANNY_DATA_TYPE_DEFINITION_H)
#include "granny_data_type_definition.h"
#endif

#if !defined(GRANNY_FILE_READER_H)
#include "granny_file_reader.h"
#endif

#if !defined(GRANNY_FILE_OPERATIONS_H)
#include "granny_file_operations.h"
#endif

#if !defined(GRANNY_CPP_SETTINGS_H)
// This should always be the last header included
#include "granny_cpp_settings.h"
#endif

#undef SubsystemCode
#define SubsystemCode FileWritingLogMessage

USING_GRANNY_NAMESPACE;

BEGIN_GRANNY_NAMESPACE;

struct file_fixup
{
    file_location From;
    file_location To;

    file_fixup *Next;
};

struct marshalling_fixup
{
    int32x Count;
    file_location Type;
    file_location Object;

    marshalling_fixup *Next;
};


int32x GetSectionSize(section &Section)
{
    int32x Size = 0;

    {for(int32x BufferIndex = 0;
         BufferIndex < SectionBufferCount;
         ++BufferIndex)
    {
        Size += Section.BufferSize[BufferIndex];
    }}

    return(Size);
}

END_GRANNY_NAMESPACE;

int32x GRANNY
GetBufferIndexFor(uint32x Marshalling)
{
    int32x BufferIndex = 0;

    switch(Marshalling)
    {
        case AnyMarshalling:
        case Int32Marshalling:
        {
            BufferIndex = BufferIndex32;
        } break;

        case Int16Marshalling:
        {
            BufferIndex = BufferIndex16;
        } break;

        case Int8Marshalling:
        default:
        {
            BufferIndex = BufferIndex8;
        } break;
    }
    Assert(BufferIndex < SectionBufferCount);

    return(BufferIndex);
}

static void
InitializeSection(section &Section)
{
    ZeroStructure(Section);

    Section.Header.Format = NoCompression;
    Section.Header.DataOffset = 0;
    Section.Header.DataSize = 0;
    Section.Header.ExpandedDataSize = 0;
    Section.Header.InternalAlignment = 4;
    Section.Header.First16Bit = 0;
    Section.Header.First8Bit = 0;
    Section.Header.PointerFixupArrayOffset = 0;
    Section.Header.PointerFixupArrayCount = 0;
    Section.Header.MixedMarshallingFixupArrayOffset = 0;
    Section.Header.MixedMarshallingFixupArrayCount = 0;

    ZeroArray(SectionBufferCount, Section.BufferSize);
    ZeroArray(SectionBufferCount, Section.Buffers);
    ZeroArray(SectionBufferCount, Section.UncompressedBuffers);
    Section.CompressedBuffer = NULL;
    Section.CompressedBufferSize = 0;

    Section.FirstFixup = Section.LastFixup = 0;
    Section.FirstMarshalling = Section.LastMarshalling = 0;
}

static void
InitializeLocation(file_location &Location)
{
    Location.SectionIndex = 0;
    Location.BufferIndex = 0;
    Location.Offset = 0;
}

static void
GetUncompressedFilename(file_builder &Builder,
                        int32x SectionIndex, int32x BufferIndex,
                        int32x ResultSize, char *Result)
{
    Assert(Builder.TempFilesOnDisk);
    ConvertToStringVar(ResultSize, Result, "%s" PLATFORM_PATH_SEPARATOR "%s_%d_%d.utf",
                       Builder.TemporaryDirectory,
                       Builder.TemporaryFileNameRoot,
                       SectionIndex, BufferIndex);
}

static void
GetCompressedFilename(file_builder &Builder,
                      int32x SectionIndex,
                      int32x ResultSize, char *Result)
{
    ConvertToStringVar(ResultSize, Result, "%s" PLATFORM_PATH_SEPARATOR "%s_%d.ctf",
                       Builder.TemporaryDirectory,
                       Builder.TemporaryFileNameRoot,
                       SectionIndex);
}


static bool
OpenFiles(file_builder &Builder)
{
    Assert(Builder.TempFilesOnDisk);
    bool Result = true;

    {for(int32x SectionIndex = 0;
         (SectionIndex < Builder.SectionCount) && Result;
         ++SectionIndex)
    {
        section &Section = Builder.Sections[SectionIndex];

        {for(int32x BufferIndex = 0;
             (BufferIndex < SectionBufferCount) && Result;
             ++BufferIndex)
        {
            char FileName[MaximumTempFileNameSize];
            GetUncompressedFilename(Builder, SectionIndex, BufferIndex,
                                    SizeOf(FileName), FileName);
            Result = ((Section.Buffers[BufferIndex] =
                       NewFileWriter(FileName, true)) != 0);
        }}
    }}

    return(Result);
}

static void
CloseFiles(file_builder &Builder)
{
    Assert(Builder.TempFilesOnDisk);

    {for(int32x SectionIndex = 0;
         SectionIndex < Builder.SectionCount;
         ++SectionIndex)
    {
        section &Section = Builder.Sections[SectionIndex];

        {for(int32x BufferIndex = 0;
             BufferIndex < SectionBufferCount;
             ++BufferIndex)
        {
            DeleteFileWriter(Section.Buffers[BufferIndex]);
            Section.Buffers[BufferIndex] = 0;
        }}
    }}
}

static void
DeleteFiles(file_builder &Builder)
{
    Assert(Builder.TempFilesOnDisk);

    {for(int32x SectionIndex = 0;
         SectionIndex < Builder.SectionCount;
         ++SectionIndex)
    {
        char FileName[MaximumTempFileNameSize];

        GetCompressedFilename(Builder, SectionIndex,
                              SizeOf(FileName), FileName);
        DeleteFile(FileName);

        {for(int32x BufferIndex = 0;
             BufferIndex < SectionBufferCount;
             ++BufferIndex)
        {
            GetUncompressedFilename(Builder, SectionIndex, BufferIndex,
                                    SizeOf(FileName), FileName);
            DeleteFile(FileName);
        }}
    }}
}

static bool
OpenMemoryBuffers(file_builder &Builder,
                  int32x MemoryBlockSize)
{
    Assert(!Builder.TempFilesOnDisk);

    bool Result = true;
    {for(int32x SectionIndex = 0;
         (SectionIndex < Builder.SectionCount) && Result;
         ++SectionIndex)
    {
        section &Section = Builder.Sections[SectionIndex];

        {for(int32x BufferIndex = 0;
             (BufferIndex < SectionBufferCount) && Result;
             ++BufferIndex)
        {
            Result = (Section.Buffers[BufferIndex] =
                      CreateMemoryFileWriter(__FILE__, __LINE__, MemoryBlockSize)) != 0;
        }}
    }}

    return(Result);
}

static void
CloseMemoryBuffers(file_builder &Builder)
{
    Assert(!Builder.TempFilesOnDisk);

    {for(int32x SectionIndex = 0;
         SectionIndex < Builder.SectionCount;
         ++SectionIndex)
    {
        section &Section = Builder.Sections[SectionIndex];

        {for(int32x BufferIndex = 0;
             BufferIndex < SectionBufferCount;
             ++BufferIndex)
        {
            int32x CheckSize;
            if (StealMemoryWriterBuffer(*Section.Buffers[BufferIndex],
                                        Section.UncompressedBuffers[BufferIndex],
                                        CheckSize))
            {
                Assert(CheckSize == Section.BufferSize[BufferIndex]);
                Assert(Section.UncompressedBuffers[BufferIndex] || CheckSize == 0);
            }
            else
            {
                InvalidCodePath("Unable to steal the memory buffer?  That should never get caught here");
            }

            DeleteFileWriter(Section.Buffers[BufferIndex]);
            Section.Buffers[BufferIndex] = 0;
        }}
    }}
}

static void
DeleteSectionBuffers(file_builder &Builder)
{
    {for(int32x SectionIndex = 0;
         SectionIndex < Builder.SectionCount;
         ++SectionIndex)
    {
        section &Section = Builder.Sections[SectionIndex];

        if (Section.CompressedBuffer)
        {
            FreeMemoryWriterBuffer(Section.CompressedBuffer);
            Section.CompressedBuffer = 0;
        }

        {for(int32x BufferIndex = 0;
             BufferIndex < SectionBufferCount;
             ++BufferIndex)
        {
            if (Section.UncompressedBuffers[BufferIndex])
            {
                FreeMemoryWriterBuffer(Section.UncompressedBuffers[BufferIndex]);
                Section.UncompressedBuffers[BufferIndex] = 0;
            }
        }}
    }}
}

static grn_reference
ReferenceFrom(file_builder &Builder, file_location &Location)
{
    Assert(Location.SectionIndex < (uint32)Builder.SectionCount);
    Assert(Location.BufferIndex < (uint32)SectionBufferCount);

    section &Section = Builder.Sections[Location.SectionIndex];

    grn_reference Reference;
    Reference.SectionIndex = Location.SectionIndex;
    Reference.Offset = Location.Offset;
    {for(uint32 BufferIndex = 0;
         BufferIndex < Location.BufferIndex;
         ++BufferIndex)
    {
        Reference.Offset += Section.BufferSize[BufferIndex];
    }}

    return(Reference);
}

static bool
ZeroSectionPointers(file_builder &Builder, int32x SectionIndex)
{
    Assert(SectionIndex >= 0 && SectionIndex < Builder.SectionCount);
    Assert(SectionIndex < Builder.SectionCount);
    section &Section = Builder.Sections[SectionIndex];

	// At this point the file is still in native format
	int32x const PointerSizeInBits = sizeof(void*) * 8;
    int32x const PointerSizeInBytes = PointerSizeInBits / 8;

    // The files are closed at this point, so reopen them
    file_writer* ReopenedBuffers[SectionBufferCount] = { 0 };
    if (Builder.TempFilesOnDisk)
    {
        {for (int32x i = 0; i < SectionBufferCount; ++i)
        {
            Assert(Section.Buffers[i] == NULL);
            char FileNameBuf[MaximumTempFileNameSize];
            GetUncompressedFilename(Builder, SectionIndex, i,
                                    SizeOf(FileNameBuf), FileNameBuf);
            ReopenedBuffers[i] = NewFileWriter(FileNameBuf, false);
        }}
    }

    bool FilesAreValid = true;
    if(Section.Header.ExpandedDataSize)
    {
        {for(file_fixup *Fixup = Section.FirstFixup;
             Fixup && FilesAreValid;
             Fixup = Fixup->Next)
        {
            Assert(Fixup->From.SectionIndex == (uint32)SectionIndex);
            uint32 const BufferIndex = Fixup->From.BufferIndex;
            uint32 const Offset      = Fixup->From.Offset;

            Assert(BufferIndex < (uint32)SectionBufferCount);
            Assert(Offset < (uint32)Section.BufferSize[BufferIndex]);

            if (Builder.TempFilesOnDisk)
            {
                static const uint8 ZeroArray[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
                Assert(ReopenedBuffers[BufferIndex] != NULL);
                Assert(PointerSizeInBytes <= 8);

                file_writer* Buffer = ReopenedBuffers[BufferIndex];
                SeekWriterFromStart(*Buffer, Offset);
                FilesAreValid = FilesAreValid && Write(*Buffer, PointerSizeInBytes, ZeroArray);
            }
            else
            {
                Assert(PointerSizeInBytes <= 8);
                SetUInt8(PointerSizeInBytes, 0,
                         Section.UncompressedBuffers[BufferIndex] + Offset);
            }
        }}
    }

    if (Builder.TempFilesOnDisk)
    {
        {for (int32x i = 0; i < SectionBufferCount; ++i)
        {
            DeleteFileWriter(ReopenedBuffers[i]);
        }}
    }

    return FilesAreValid;
}

static bool
CompressSection(file_builder &Builder, int32x SectionIndex,
                int32x CopyBufferSize, void *CopyBuffer)
{
    bool Result = false;

    Assert(SectionIndex < Builder.SectionCount);
    section &Section = Builder.Sections[SectionIndex];

    if(Section.Header.ExpandedDataSize)
    {
        file_writer* Writer = NULL;
        if (Builder.TempFilesOnDisk)
        {
            char FileName[MaximumTempFileNameSize];
            GetCompressedFilename(Builder, SectionIndex,
                                  SizeOf(FileName), FileName);
            Writer = NewFileWriter(FileName, true);
        }
        else
        {
            Writer = CreateMemoryFileWriter(__FILE__, __LINE__, 64 << 10);
        }

        if(Writer)
        {
            file_compressor *Compressor = BeginFileCompression(
                Section.Header.ExpandedDataSize, SectionBufferCount,
                (compression_type)Section.Header.Format,
                false, Writer);
            if(Compressor)
            {
                Result = true;
                {for(uint32 BufferIndex = 0;
                     (BufferIndex < (uint32)SectionBufferCount) && Result;
                     ++BufferIndex)
                {
                    int32x const BufferSize = Section.BufferSize[BufferIndex];

                    if (Builder.TempFilesOnDisk)
                    {
                        char FileNameBuf[MaximumTempFileNameSize];
                        GetUncompressedFilename(Builder, SectionIndex, BufferIndex,
                                                SizeOf(FileNameBuf), FileNameBuf);
                        Result = CompressContentsOfFile(*Compressor,
                                                        BufferSize,
                                                        FileNameBuf,
                                                        CopyBufferSize,
                                                        CopyBuffer);
                    }
                    else
                    {
                        //--- just compress from the buffer, jeez.

                        // Note that the buffer can be null if buffersize == 0
                        file_reader *Reader = CreateMemoryFileReader(__FILE__, __LINE__,
                                                                     0, BufferSize,
                                                                     Section.UncompressedBuffers[BufferIndex]);
                        if (Reader)
                        {
                            Result = CompressContentsOfReader(*Compressor,
                                                              BufferSize,
                                                              *Reader,
                                                              CopyBufferSize,
                                                              CopyBuffer);
                            CloseFileReader(Reader);
                        }
                        else
                        {
                            Log0(ErrorLogMessage, FileWritingLogMessage, "Unable to allocate a reader");
                            Result = false;
                        }
                    }
                }}

                Result =
                    EndFileCompression(Compressor, Section.Header.DataSize) &&
                    Result;
            }

            if (!Builder.TempFilesOnDisk)
            {
                Assert(Section.CompressedBuffer == 0);
                // Steal the compressed buffer from the writer
                if (!StealMemoryWriterBuffer(*Writer, Section.CompressedBuffer,
                                             Section.CompressedBufferSize))
                {
                    Log0(ErrorLogMessage, FileWritingLogMessage, "Failed to steal memorywriter buffer");
                    Result = false;
                }
                Assert(Section.CompressedBuffer != 0);
                Assert(Section.CompressedBufferSize == (int32x)Section.Header.DataSize);
            }

            DeleteFileWriter(Writer);
        }
    }
    else
    {
        Result = true;
        Section.Header.DataSize = 0;

        // Leave the compressed buffer pointer nulled
    }

    return(Result);
}

static bool
WriteFile(file_builder &Builder, file_writer &FinalWriter)
{
    // Check the platform.  If it's for a non-native platform, we'll
    // have to dance around a bit...

    file_writer *TargetWriter = NULL;
    bool MV_ByteReversed;
    if (DoesMagicValueMatch(Builder.PlatformMagicValue,
                            GRNFileMV_ThisPlatform,
                            MV_ByteReversed))
    {
        Assert(MV_ByteReversed == false);
        TargetWriter = &FinalWriter;
    }
    else
    {
        const int TempMemWriterBlockSize = FileCopyBufferSize;

        // Create a memory writer to dump this to...
        TargetWriter = CreateMemoryFileWriter(__FILE__, __LINE__, TempMemWriterBlockSize);
        CheckPointerNotNull(TargetWriter, return false);
    }

    bool FilesAreValid = Builder.FilesAreValid;

    int32x const CopyBufferSize = FileCopyBufferSize;
    void *CopyBuffer = AllocateSize(CopyBufferSize);
    if(FilesAreValid && CopyBuffer)
    {
        // Zero the pointers for all fixup locations.  This ensures that each write of an
        // identical structure will have an identical representation on disk.
        {for(int32x SectionIndex = 0;
             SectionIndex < Builder.SectionCount;
             ++SectionIndex)
        {
            ZeroSectionPointers(Builder, SectionIndex);
        }}

        // Compress all the file data we've buffered
        {for(int32x SectionIndex = 0;
             SectionIndex < Builder.SectionCount;
             ++SectionIndex)
        {
            FilesAreValid = FilesAreValid &&
                CompressSection(Builder, SectionIndex,
                                CopyBufferSize, CopyBuffer);
        }}

        // See where we start
        uint32 const TopOfFile = (uint32)GetWriterPosition(*TargetWriter);

        // Skip over where the file header would be, since we write it later.
        grn_file_header FileHeader;
        ZeroStructure(FileHeader);

        uint32x FixedHeaderSize = (SizeOf(grn_file_magic_value) + SizeOf(FileHeader));
        SeekWriterFromCurrentPosition(*TargetWriter, FixedHeaderSize);

        uint32x SectionArraySize = Builder.SectionCount * SizeOf(grn_section);
        uint32x FullHeaderSize = FixedHeaderSize + SectionArraySize;

        BeginWriterCRC(*TargetWriter);

        // Write the section headers.  Note that we're going to do a
        // little bit of fanciness here in order to write out the
        // section data, fixups, and marshalling data in the order
        // that they are read in on the backend.
        uint32x CurrentOffset = FullHeaderSize + PredictWriterAlignment(FullHeaderSize);
        {
            // Read order is [section...] [fixup marshall fixup marshall ... ]

            // Place sections
            {for(int32x SectionIndex = 0; SectionIndex < Builder.SectionCount; ++SectionIndex)
            {
                grn_section &SectionHeader = Builder.Sections[SectionIndex].Header;

                SectionHeader.DataOffset = CurrentOffset;
                CurrentOffset += SectionHeader.DataSize;
                CurrentOffset += PredictWriterAlignment(CurrentOffset);
            }}

            // Place fixups
            {for(int32x SectionIndex = 0;
                 (SectionIndex < Builder.SectionCount) && FilesAreValid;
                 ++SectionIndex)
            {
                grn_section &SectionHeader = Builder.Sections[SectionIndex].Header;

                SectionHeader.PointerFixupArrayOffset = CurrentOffset;
                CurrentOffset += (SectionHeader.PointerFixupArrayCount *
                                  SizeOf(grn_pointer_fixup));
                CurrentOffset += PredictWriterAlignment(CurrentOffset);
            }}

            // Place marshalls and write the headers
            {for(int32x SectionIndex = 0;
                 (SectionIndex < Builder.SectionCount) && FilesAreValid;
                 ++SectionIndex)
            {
                grn_section &SectionHeader = Builder.Sections[SectionIndex].Header;

                SectionHeader.MixedMarshallingFixupArrayOffset = CurrentOffset;
                CurrentOffset += (SectionHeader.MixedMarshallingFixupArrayCount *
                                  SizeOf(grn_mixed_marshalling_fixup));
                CurrentOffset += PredictWriterAlignment(CurrentOffset);

                FilesAreValid = FilesAreValid &&
                    Write(*TargetWriter, SizeOf(SectionHeader), &SectionHeader);
            }}
        }

        // Write the section data
        {for(int32x SectionIndex = 0;
             (SectionIndex < Builder.SectionCount) && FilesAreValid;
             ++SectionIndex)
        {
            section &Section = Builder.Sections[SectionIndex];

            AlignWriter(*TargetWriter);
            // Write section data
            if(Section.Header.DataSize)
            {
                if (Builder.TempFilesOnDisk)
                {
                    char FileName[MaximumTempFileNameSize];
                    GetCompressedFilename(Builder, SectionIndex,
                                          SizeOf(FileName), FileName);
                    uint32x Ignored;
                    FilesAreValid = FilesAreValid &&
                        ConcatenateFile(*TargetWriter, FileName,
                                        CopyBufferSize, CopyBuffer,
                                        Ignored);
                }
                else
                {
                    FilesAreValid = FilesAreValid &&
                        Write(*TargetWriter, Section.CompressedBufferSize, Section.CompressedBuffer);
                }
            }
            else
            {
                Assert(Section.CompressedBuffer == 0);
            }
        }}

        // Write the fixups
        {for(int32x SectionIndex = 0;
             (SectionIndex < Builder.SectionCount) && FilesAreValid;
             ++SectionIndex)
        {
            section &Section = Builder.Sections[SectionIndex];

            AlignWriter(*TargetWriter);
            // Write the pointer fixups
            {for(file_fixup *Fixup = Section.FirstFixup;
                 Fixup && FilesAreValid;
                 Fixup = Fixup->Next)
            {
                grn_pointer_fixup GRNFixup;
                GRNFixup.FromOffset = ReferenceFrom(Builder, Fixup->From).Offset;
                GRNFixup.To = ReferenceFrom(Builder, Fixup->To);

                FilesAreValid = FilesAreValid &&
                    Write(*TargetWriter, SizeOf(GRNFixup), &GRNFixup);
            }}
        }}

        // Write the marshalls
        {for(int32x SectionIndex = 0;
             (SectionIndex < Builder.SectionCount) && FilesAreValid;
             ++SectionIndex)
        {
            section &Section = Builder.Sections[SectionIndex];

            AlignWriter(*TargetWriter);
            // Write the marshalling fixups
            {for(marshalling_fixup *Fixup = Section.FirstMarshalling;
                 Fixup && FilesAreValid;
                 Fixup = Fixup->Next)
            {
                grn_mixed_marshalling_fixup GRNFixup;
                GRNFixup.Count = Fixup->Count;
                GRNFixup.Offset = ReferenceFrom(Builder, Fixup->Object).Offset;
                GRNFixup.Type = ReferenceFrom(Builder, Fixup->Type);

                FilesAreValid = FilesAreValid &&
                    Write(*TargetWriter, SizeOf(GRNFixup), &GRNFixup);
            }}
        }}

        // Final alignment operation not required...
        //AlignWriter(*TargetWriter);


        uint32 CRC = EndWriterCRC(*TargetWriter);

        // Seek back to the start and write the magic value & header
        // NOTE!  We're writing this platforms mv, not the Builder
        // magic value.  If these are mismatched, we'll deal with it
        // below...
        grn_file_magic_value MagicValue = GRNFileMV_ThisPlatform;

        uint32 const TotalFileSize =
            (uint32)(GetWriterPosition(*TargetWriter) - TopOfFile);
        SeekWriterFromStart(*TargetWriter, TopOfFile);
        MagicValue.HeaderSize = FullHeaderSize;
        MagicValue.HeaderFormat = 0;
        MagicValue.Reserved[0] = MagicValue.Reserved[1] = 0;
        FilesAreValid = FilesAreValid &&
            Write(*TargetWriter, SizeOf(MagicValue), &MagicValue);

        FileHeader.Version = CurrentGRNFileVersion;
        FileHeader.TotalSize = TotalFileSize;
        FileHeader.CRC = CRC;
        FileHeader.SectionArrayOffset = SizeOf(FileHeader);
        FileHeader.SectionArrayCount = Builder.SectionCount;
        FileHeader.RootObject = ReferenceFrom(Builder, Builder.RootObject);
        FileHeader.RootObjectTypeDefinition =
            ReferenceFrom(Builder, Builder.RootObjectTypeDefinition);
        FileHeader.TypeTag = Builder.FileTypeTag;
        Copy32(GRNExtraTagCount, Builder.ExtraTags, FileHeader.ExtraTags);
        FileHeader.StringDatabaseCRC = Builder.StringDatabaseCRC;

        FilesAreValid = FilesAreValid &&
            Write(*TargetWriter, SizeOf(FileHeader), &FileHeader);
    }

    Deallocate(CopyBuffer);


    if (DoesMagicValueMatch(Builder.PlatformMagicValue,
                            GRNFileMV_ThisPlatform,
                            MV_ByteReversed))
    {
        // Nothing to do, we wrote this to the final reader, and it's
        // in the correct format
        Assert(MV_ByteReversed == false);
    }
    else
    {
        // TargetWriter contains the file to convert

        // Get the buffer from the writer
        uint8* BufferPointer = NULL;
        int32x BufferSize = 0;
        FilesAreValid = FilesAreValid &&
            StealMemoryWriterBuffer(*TargetWriter, BufferPointer, BufferSize);
        if (FilesAreValid)
        {
            Assert(BufferPointer && BufferSize > 0);

            // Create a memory reader for the buffer
            file_reader *NativeFile = CreateMemoryFileReader(__FILE__, __LINE__,
                                                             0, BufferSize, BufferPointer);
            Assert(NativeFile);

            // convert it to the correct format
            // write it to the final writer.
            FilesAreValid = FilesAreValid &&
                PlatformConvertReaderToWriter(*NativeFile, FinalWriter,
                                              Builder.PlatformMagicValue,
                                              Builder.ExcludeTypeTreeOnConversion);

            CloseFileReader(NativeFile);
            FreeMemoryWriterBuffer(BufferPointer);
            BufferPointer = 0;
            BufferSize = 0;
        }

        DeleteFileWriter(TargetWriter);
        TargetWriter = NULL;
    }

    return(FilesAreValid);
}

static intaddrx
RawOffsetFrom(file_builder &Builder, file_location &Location)
{
    return(Builder.Sections[Location.SectionIndex].
           RawOffset[Location.BufferIndex] + Location.Offset);
}

static bool
WriteRaw(file_builder &Builder, file_writer &Writer)
{
    bool FilesAreValid = Builder.FilesAreValid;

    int32x const CopyBufferSize = FileCopyBufferSize;
    void *CopyBuffer = AllocateSize(CopyBufferSize);
    if(FilesAreValid && CopyBuffer)
    {
        uint32 const TopOfFile = (uint32)GetWriterPosition(Writer);

        // Write all section data out into one continguous blob,
        // but record the offsets of each into the RawOffset member
        {for(int32x SectionIndex = 0;
             (SectionIndex < Builder.SectionCount) && FilesAreValid;
             ++SectionIndex)
        {
            section &Section = Builder.Sections[SectionIndex];

            {for(int32x BufferIndex = 0;
                 BufferIndex < SectionBufferCount;
                 ++BufferIndex)
            {
                Section.RawOffset[BufferIndex] =
                    (uint32)GetWriterPosition(Writer) - TopOfFile;

                if (Builder.TempFilesOnDisk)
                {
                    char FileName[MaximumTempFileNameSize];
                    GetUncompressedFilename(Builder, SectionIndex, BufferIndex,
                                            SizeOf(FileName), FileName);
                    uint32x Ignored;
                    FilesAreValid = FilesAreValid &&
                        ConcatenateFile(Writer, FileName, CopyBufferSize, CopyBuffer,
                                        Ignored);
                }
                else
                {
                    if (Section.BufferSize[BufferIndex] != 0)
                    {
                        Assert(Section.UncompressedBuffers[BufferIndex]);

                        file_reader *Reader = CreateMemoryFileReader(__FILE__, __LINE__,
                                                                     0, Section.BufferSize[BufferIndex],
                                                                     Section.UncompressedBuffers[BufferIndex]);
                        if (Reader)
                        {
                            uint32x Ignored;
                            FilesAreValid = FilesAreValid &&
                                ConcatenateFileReader(Writer, *Reader, CopyBufferSize, CopyBuffer,
                                                      Ignored);
                            CloseFileReader(Reader);
                        }
                        else
                        {
                            Log0(ErrorLogMessage, FileWritingLogMessage, "Unable to allocate a reader");
                            FilesAreValid = false;
                        }
                    }
                }
                AlignWriter(Writer);
            }}
        }}

        // Write back the pointer fixups
        {for(int32x SectionIndex = 0;
             (SectionIndex < Builder.SectionCount) && FilesAreValid;
             ++SectionIndex)
        {
            section &Section = Builder.Sections[SectionIndex];

            {for(file_fixup *Fixup = Section.FirstFixup;
                 Fixup && FilesAreValid;
                 Fixup = Fixup->Next)
            {
                intaddrx FromOffset = RawOffsetFrom(Builder, Fixup->From);
                intaddrx ToOffset = RawOffsetFrom(Builder, Fixup->To);

                int32x FileOffset = 0;
                CheckConvertToInt32(FileOffset, TopOfFile + FromOffset, FilesAreValid = false);
                SeekWriterFromStart(Writer, FileOffset);
                FilesAreValid = FilesAreValid && Write(Writer, SizeOf(void*), &ToOffset);
            }}
        }}
    }

    Deallocate(CopyBuffer);
    return(FilesAreValid);
}


file_builder *GRANNY
BeginFile(int32x SectionCount, uint32 FileTypeTag,
          grn_file_magic_value const& PlatformMagicValue,
          char const *TemporaryDirectory,
          char const *TemporaryFileNameRoot)
{
    aggr_allocator Allocator;
    InitializeAggrAlloc(Allocator);

    file_builder *Builder;
    AggrAllocPtr(Allocator, Builder);
    AggrAllocOffsetArrayPtr(Allocator, Builder, SectionCount,
                            SectionCount, Sections);
    if(EndAggrAlloc(Allocator))
    {
        StackInitialize(Builder->FixupAllocator, SizeOf(file_fixup),
                        BlockFileFixupCount);
        StackInitialize(Builder->MarshallingAllocator, SizeOf(marshalling_fixup),
                        BlockFileFixupCount);

        Builder->FileTypeTag = FileTypeTag;
        Builder->StringDatabaseCRC = 0;
        Builder->PlatformMagicValue = PlatformMagicValue;

        // see note in granny_data_type_io relating to this variable
        Builder->ExcludeTypeTreeOnConversion = false;

        ZeroArray(GRNExtraTagCount, Builder->ExtraTags);

        StringEquals(Builder->TemporaryDirectory,
                     SizeOf(Builder->TemporaryDirectory),
                     TemporaryDirectory);
        StringEquals(Builder->TemporaryFileNameRoot,
                     SizeOf(Builder->TemporaryFileNameRoot),
                     TemporaryFileNameRoot);
        Builder->TempFilesOnDisk = true;

        InitializeLocation(Builder->RootObject);
        InitializeLocation(Builder->RootObjectTypeDefinition);

        {for(int32x SectionIndex = 0;
             SectionIndex < SectionCount;
             ++SectionIndex)
        {
            InitializeSection(Builder->Sections[SectionIndex]);
        }}

        if(OpenFiles(*Builder))
        {
            Builder->FilesAreValid = true;
        }
        else
        {
            CloseFiles(*Builder);
            DeleteFiles(*Builder);
            Deallocate(Builder);
            Builder = 0;
        }
    }

    return(Builder);
}

file_builder *GRANNY
BeginFileInMemory(int32x SectionCount, uint32 FileTypeTag,
                  grn_file_magic_value const& PlatformMagicValue,
                  int32x MemoryBlockSize)
{
    aggr_allocator Allocator;
    InitializeAggrAlloc(Allocator);

    file_builder *Builder;
    AggrAllocPtr(Allocator, Builder);
    AggrAllocOffsetArrayPtr(Allocator, Builder, SectionCount,
                            SectionCount, Sections);
    if(EndAggrAlloc(Allocator))
    {
        StackInitialize(Builder->FixupAllocator, SizeOf(file_fixup),
                        BlockFileFixupCount);
        StackInitialize(Builder->MarshallingAllocator, SizeOf(marshalling_fixup),
                        BlockFileFixupCount);

        Builder->FileTypeTag = FileTypeTag;
        Builder->PlatformMagicValue = PlatformMagicValue;

        // see note in granny_data_type_io relating to this variable
        Builder->ExcludeTypeTreeOnConversion = false;

        ZeroArray(GRNExtraTagCount, Builder->ExtraTags);

        StringEquals(Builder->TemporaryDirectory,
                     SizeOf(Builder->TemporaryDirectory),
                     "invalid");
        StringEquals(Builder->TemporaryFileNameRoot,
                     SizeOf(Builder->TemporaryFileNameRoot),
                     "invalid");
        Builder->TempFilesOnDisk = false;

        InitializeLocation(Builder->RootObject);
        InitializeLocation(Builder->RootObjectTypeDefinition);

        {for(int32x SectionIndex = 0;
             SectionIndex < SectionCount;
             ++SectionIndex)
        {
            InitializeSection(Builder->Sections[SectionIndex]);
        }}

        if (OpenMemoryBuffers(*Builder, MemoryBlockSize))
        {
            Builder->FilesAreValid = true;
        }
        else
        {
            CloseMemoryBuffers(*Builder);
            Deallocate(Builder);
            Builder = 0;
        }
    }

    return(Builder);
}

bool GRANNY
EndFile(file_builder *Builder, char const *FileName)
{
    bool Result = false;

    file_writer *Writer = NewFileWriter(FileName, true);
    if(Writer)
    {
        Result = EndFileToWriter(Builder, *Writer);
        DeleteFileWriter(Writer);
    }
    else
    {
        AbortFile(Builder);
    }

    return(Result);
}

static void
PreEnd(file_builder &Builder)
{
    {for(int32x SectionIndex = 0;
         SectionIndex < Builder.SectionCount;
         ++SectionIndex)
    {
        section &Section = Builder.Sections[SectionIndex];
        Section.Header.First16Bit = Section.BufferSize[BufferIndex32];
        Section.Header.First8Bit = (Section.Header.First16Bit +
                                    Section.BufferSize[BufferIndex16]);

        {for(int32x BufferIndex = 0;
             BufferIndex < SectionBufferCount;
             ++BufferIndex)
        {
            Section.Header.ExpandedDataSize += Section.BufferSize[BufferIndex];
        }}

        Assert((Section.Header.First8Bit + Section.BufferSize[BufferIndex8]) ==
               Section.Header.ExpandedDataSize);
    }}

    if (Builder.TempFilesOnDisk)
    {
        CloseFiles(Builder);
    }
    else
    {
        CloseMemoryBuffers(Builder);
    }
}

static void
PostEnd(file_builder &Builder)
{
    if (Builder.TempFilesOnDisk)
    {
        DeleteFiles(Builder);
    }

    // Always do this, since the compression buffer is always in memory
    DeleteSectionBuffers(Builder);

    StackCleanUp(Builder.FixupAllocator);
    StackCleanUp(Builder.MarshallingAllocator);
}

bool GRANNY
EndFileToWriter(file_builder *Builder, file_writer &ToFile)
{
    bool Result = false;

    if(Builder)
    {
        PreEnd(*Builder);
        Result = WriteFile(*Builder, ToFile);
        PostEnd(*Builder);
        Deallocate(Builder);
        Builder = NULL;
    }

    return(Result);
}

bool GRANNY
EndFileRawToWriter(file_builder *Builder, file_writer &ToFile)
{
    bool Result = false;

    if(Builder)
    {
        PreEnd(*Builder);

        Result = WriteRaw(*Builder, ToFile);

        PostEnd(*Builder);
        Deallocate(Builder);
        Builder = NULL;
    }

    return(Result);
}


bool GRANNY
EndFileRaw(file_builder *Builder, char const *FileName)
{
    bool Result = false;

    file_writer *Writer = NewFileWriter(FileName, true);
    if(Writer)
    {
        Result = EndFileRawToWriter(Builder, *Writer);
        DeleteFileWriter(Writer);
    }
    else
    {
        AbortFile(Builder);
    }

    return(Result);
}

void GRANNY
AbortFile(file_builder *Builder)
{
    if(Builder)
    {
        PreEnd(*Builder);
        PostEnd(*Builder);
        Deallocate(Builder);
        Builder = NULL;
    }
}

void GRANNY
SetFileSectionFormat(file_builder &Builder, int32x SectionIndex,
                     compression_type Compression, int32x Alignment)
{
    CheckCountedInt32(SectionIndex, Builder.SectionCount, return);

    if (Compression == Oodle0Compression)
    {
        Log0(WarningLogMessage, CompressorLogMessage,
             "Substituting Oodle1 compression for obsolete Oodle0 compression\n");
        Compression = Oodle1Compression;
    }

    section &Section = Builder.Sections[SectionIndex];
    Section.Header.Format = Compression;
    Section.Header.InternalAlignment = Alignment;
}


void GRANNY
PreserveFileSectionFormats(file_builder &Builder,
                           file const &SourceFile)
{
    int32x SectionsToPreserve = SourceFile.Header->SectionArrayCount;
    if (SectionsToPreserve > Builder.SectionCount)
        SectionsToPreserve = Builder.SectionCount;

    grn_section const *SectionArray = GetGRNSectionArray(*SourceFile.Header);
    {for(int32x SectionIndex = 0;
         SectionIndex < SectionsToPreserve;
         ++SectionIndex)
    {
        grn_section const &Section = SectionArray[SectionIndex];
        SetFileSectionFormat(Builder, SectionIndex,
                             (compression_type)Section.Format,
                             Section.InternalAlignment);
    }}
}


static void
BuilderWrite(file_builder &Builder, section &Section, int32x BufferIndex,
             int32x Size, void const *Buffer)
{
    Assert(BufferIndex < SectionBufferCount);
    Builder.FilesAreValid = (Builder.FilesAreValid &&
                             Write(*Section.Buffers[BufferIndex],
                                   Size, Buffer));
    Section.BufferSize[BufferIndex] += Size;
}

static void
Align(file_builder &Builder, section &Section, int32x BufferIndex)
{
    uint32 Zero = 0;
    int32x Pad = Section.Header.InternalAlignment -
        (Section.BufferSize[BufferIndex] %
         Section.Header.InternalAlignment);
    if(Pad != (int32x)Section.Header.InternalAlignment)
    {
        while((Pad > 4) && Builder.FilesAreValid)
        {
            BuilderWrite(Builder, Section, BufferIndex, 4, &Zero);
            Pad -= 4;
        }

        while((Pad > 0) && Builder.FilesAreValid)
        {
            BuilderWrite(Builder, Section, BufferIndex, 1, &Zero);
            --Pad;
        }
    }
}

void GRANNY
WriteFileChunk(file_builder &Builder, int32x InSectionIndex,
               uint32 Marshalling, uint32x Size, void const *Data,
               file_location* Result)
{
    // Work with a copy, just in case.  (No obvious aliasing problems
    //  here, but let's be safe in the file io routines...)  Note
    //  also that we allow Result to be NULL
    file_location result_loc;
    InitializeLocation(result_loc);

    CheckCountedInt32(InSectionIndex, Builder.SectionCount, return);

    section &Section = Builder.Sections[InSectionIndex];

    int32x BufferIndex = GetBufferIndexFor(Marshalling);

    result_loc.SectionIndex = InSectionIndex;
    result_loc.BufferIndex  = BufferIndex;
    result_loc.Offset       = Section.BufferSize[BufferIndex];

    if(Builder.FilesAreValid)
    {
        Assert(Section.Buffers[BufferIndex]);
        BuilderWrite(Builder, Section, BufferIndex, Size, Data);
        Align(Builder, Section, BufferIndex);
    }

    // Copy to result var
    if (Result != NULL)
        *Result = result_loc;
}

void GRANNY
OffsetFileLocation(file_builder &Builder,
                   file_location const &Location,
                   uint32 AdditionalOffset,
                   file_location* Result)
{
    CheckPointerNotNull(Result, return);

    // There's a danger that Location and Result refer to the
    //  same var, so handle this carefully...
    file_location offset = Location;
    offset.Offset += AdditionalOffset;

    *Result = offset;
}

file_fixup *GRANNY
MarkFileFixup(file_builder &Builder,
              file_location const &From, int32x FromOffset,
              file_location const &To)
{
    static file_fixup BogusFixup;
    CheckCountedUInt32(From.SectionIndex, (uint32)Builder.SectionCount,
                       return(&BogusFixup));
    CheckCountedUInt32(To.SectionIndex, (uint32)Builder.SectionCount,
                       return(&BogusFixup));
    Assert(FromOffset >= 0);

    int32x FixupIndex;
    if(NewStackUnit(Builder.FixupAllocator, FixupIndex))
    {
        file_fixup &Fixup = *(file_fixup *)
            GetStackUnit(Builder.FixupAllocator, FixupIndex);
        OffsetFileLocation(Builder, From, FromOffset, &Fixup.From);
        Fixup.To = To;
        Fixup.Next = 0;

        section &Section = Builder.Sections[From.SectionIndex];
        if(Section.LastFixup)
        {
            Section.LastFixup = Section.LastFixup->Next = &Fixup;
        }
        else
        {
            Section.FirstFixup = Section.LastFixup = &Fixup;
        }

        ++Section.Header.PointerFixupArrayCount;

        return(&Fixup);
    }
    else
    {
        return(&BogusFixup);
    }
}

void GRANNY
AdjustFileFixup(file_builder &Builder, file_fixup &Fixup,
                file_location const &NewTo)
{
    Fixup.To = NewTo;
}

void GRANNY
MarkMarshallingFixup(file_builder &Builder,
                     file_location const &Type,
                     file_location const &Object,
                     int32x ArrayCount)
{
    CheckCountedUInt32(Type.SectionIndex, (uint32)Builder.SectionCount, return);
    CheckCountedUInt32(Object.SectionIndex, (uint32)Builder.SectionCount, return);

    int32x FixupIndex;
    if(NewStackUnit(Builder.MarshallingAllocator, FixupIndex))
    {
        marshalling_fixup &Fixup = *(marshalling_fixup *)
            GetStackUnit(Builder.MarshallingAllocator, FixupIndex);
        Fixup.Count = ArrayCount;
        Fixup.Type = Type;
        Fixup.Object = Object;
        Fixup.Next = 0;

        section &Section = Builder.Sections[Object.SectionIndex];
        if(Section.LastMarshalling)
        {
            Section.LastMarshalling = Section.LastMarshalling->Next = &Fixup;
        }
        else
        {
            Section.FirstMarshalling = Section.LastMarshalling = &Fixup;
        }

        ++Section.Header.MixedMarshallingFixupArrayCount;
    }
}

void GRANNY
MarkFileRootObject(file_builder &Builder,
                   file_location const &TypeLocation,
                   file_location const &ObjectLocation)
{
    Builder.RootObjectTypeDefinition = TypeLocation;
    Builder.RootObject = ObjectLocation;
}

void GRANNY
SetFileExtraTag(file_builder &Builder, int32x Index, uint32 Value)
{
    CheckCountedInt32(Index, GRNExtraTagCount, return);
    Builder.ExtraTags[Index] = Value;
}

void GRANNY
SetFileStringDatabaseCRC(file_builder &Builder, uint32 DatabaseCRC)
{
    Builder.StringDatabaseCRC = DatabaseCRC;
}

