// ========================================================================
// $File: //jeffr/granny/rt/granny_data_type_io.cpp $
// $DateTime: 2007/11/14 18:07:24 $
// $Change: 16557 $
// $Revision: #39 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================
#if !defined(GRANNY_DATA_TYPE_IO_H)
#include "granny_data_type_io.h"
#endif

#if !defined(GRANNY_CRC_H)
#include "granny_crc.h"
#endif

#if !defined(GRANNY_DATA_TYPE_DEFINITION_H)
#include "granny_data_type_definition.h"
#endif

#if !defined(GRANNY_FILE_BUILDER_H)
#include "granny_file_builder.h"
#endif

#if !defined(GRANNY_FILE_H)
#include "granny_file.h"
#endif

#if !defined(GRANNY_POINTER_HASH_H)
#include "granny_pointer_hash.h"
#endif

#if !defined(GRANNY_MEMORY_H)
#include "granny_memory.h"
#endif

#if !defined(GRANNY_LOG_H)
#include "granny_log.h"
#endif

#if !defined(GRANNY_STRING_TABLE_H)
#include "granny_string_table.h"
#endif

#if !defined(GRANNY_STRING_H)
#include "granny_string.h"
#endif

#if !defined(GRANNY_ASSERT_H)
#include "granny_assert.h"
#endif

#if !defined(GRANNY_FILE_FORMAT_H)
#include "granny_file_format.h"
#endif

#if !defined(GRANNY_ANIMATION_H)
#include "granny_animation.h"
#endif

#if !defined(GRANNY_TRACK_GROUP_H)
#include "granny_track_group.h"
#endif

#if !defined(GRANNY_PARAMETER_CHECKING_H)
#include "granny_parameter_checking.h"
#endif

#if !defined(GRANNY_CPP_SETTINGS_H)
// This should always be the last header included
#include "granny_cpp_settings.h"
#endif

#undef SubsystemCode
#define SubsystemCode DataTypeLogMessage

#define DEBUG_TYPE_IO 0

// 64bit note: Handy macro for pulling cnt + pointer out of a memory
//  location in the correct 64/32-bit fashion.
#define ExtractCountAndPtr(Count, Pointer, Type, Memory)        \
                do {                                            \
                    Count   = *((int32*)(Memory));              \
                    Pointer = *((Type*)((uint8*)Memory + 4));   \
                } while (false)

// 64bit note: Handy macro for setting cnt + pointer into a memory
// location in the correct 64/32-bit fashion.
#define SetCountAndPtr(Count, Pointer, Type, Memory)            \
    do {                                                        \
        *((int32*)(Memory)) = Count;                            \
        *((Type*)((uint8*)Memory + SizeOf(int32))) = Pointer;   \
    } while (false)


USING_GRANNY_NAMESPACE;

BEGIN_GRANNY_NAMESPACE;


// Type registry.  This allows us to detect and eliminate variant types that share the
// same type structure.  This is critical for really big characters and levels in the
// exporter
struct written_type
{
    uint32 Signature;
    data_type_definition const* Type;

    written_type* Left;
    written_type* Right;
    written_type* Next;
    written_type* Previous;
};

#define CONTAINER_NAME written_type_registry
#define CONTAINER_ITEM_TYPE written_type
#define CONTAINER_ADD_FIELDS uint32 Sig, data_type_definition const* Type
#define CONTAINER_ADD_ASSIGN(Item) (Item)->Signature = Sig; (Item)->Type = Type;
#define CONTAINER_COMPARE_ITEMS(Item1, Item2) ((int32x)(Item1)->Signature - (int32x)(Item2)->Signature)
#define CONTAINER_FIND_FIELDS uint32 Signature
#define CONTAINER_COMPARE_FIND_FIELDS(Item) ((int32x)Signature - (int32x)(Item)->Signature)
#define CONTAINER_SORTED 1
#define CONTAINER_SUPPORT_DUPES 1
#include "granny_contain.inl"


struct string_fixup
{
    file_fixup *Fixup;
    uint32 Index;
};

struct object_fixup
{
    file_fixup *Fixup;
};

struct hashed_file_block
{
    int32x TraversalNumber;
    file_location Location;
    bool SectionSetExplicitly;

    // Used only if this is a type
    int32x ObjectSection;
};

struct data_type_io_context
{
    file_data_tree_writer *TreeWriter;
    file_builder *Builder;
    string_table *Strings;
    pointer_hash *BlockHash;

    // Stores the types we've written, so we can find matching variant types...
    written_type_registry* WrittenTypeRegistry;


    int32x UsedStringFixups;
    string_fixup *StringFixups;
};

struct file_data_tree_writer
{
    data_type_definition const *RootObjectTypeDefinition;
    void* RootObject;

    int32x TraversalNumber;
    uint32x Flags;
    int32x DefaultTypeSectionIndex;
    int32x DefaultObjectSectionIndex;

    int32x MaximumBlockCount;
    int32x UsedBlockCount;
    hashed_file_block *Blocks;

    // These point into the Blocks array
    pointer_hash *BlockHash;

    int32x MaximumStringCount;
    string_fixup *StringFixups;

    int32x MaximumObjectCount;
    object_fixup *ObjectFixups;

    // This allows users to override string handling
    file_writer_string_callback *StringCallback;
    void *StringCallbackData;

    // Stores the types we've written, so we can find matching variant types...
    written_type_registry WrittenTypeRegistry;

    // This allows the writer to preserve the sectioning found in the previous version of
    // the file.  (Mainly useful in the preprocessor.)
    file const* SourceFileForSectioning;
    file const* SourceFileForFormats;
};

END_GRANNY_NAMESPACE;

static hashed_file_block &WriteObject(data_type_io_context &Context,
                                      data_type_definition const *TypeDefinition,
                                      void* Object);
static void WriteObjectInPlace(data_type_io_context &Context,
                               data_type_definition const *TypeDefinition,
                               file_location const &ObjectLocation,
                               void* Object);

file_data_tree_writer *GRANNY
BeginFileDataTreeWriting(data_type_definition const *RootObjectTypeDefinition,
                         void* RootObject,
                         int32x DefaultTypeSectionIndex,
                         int32x DefaultObjectSectionIndex)
{
    // TODO: Make a damn variable length allocator!
    int32x const MaximumObjectCount = 1 << 17;
    int32x const MaximumStringCount = 1 << 18;
    int32x const MaximumSpecialObjectReferenceCount = 1 << 10;

    pointer_hash *BlockHash = NewPointerHash();

    aggr_allocator Allocator;
    InitializeAggrAlloc(Allocator);

    file_data_tree_writer *TreeWriter;
    AggrAllocPtr(Allocator, TreeWriter);
    AggrAllocOffsetArrayPtr(Allocator, TreeWriter, MaximumObjectCount,
                            MaximumBlockCount, Blocks);
    AggrAllocOffsetArrayPtr(Allocator, TreeWriter, MaximumStringCount,
                            MaximumStringCount, StringFixups);
    AggrAllocOffsetArrayPtr(Allocator, TreeWriter, MaximumSpecialObjectReferenceCount,
                            MaximumObjectCount, ObjectFixups);
    if(EndAggrAlloc(Allocator) && BlockHash)
    {
        TreeWriter->RootObjectTypeDefinition = RootObjectTypeDefinition;
        TreeWriter->RootObject = RootObject;

        TreeWriter->TraversalNumber = 0;
        TreeWriter->Flags = 0;
        TreeWriter->DefaultTypeSectionIndex = DefaultTypeSectionIndex;
        TreeWriter->DefaultObjectSectionIndex = DefaultObjectSectionIndex;

        TreeWriter->UsedBlockCount = 0;
        TreeWriter->BlockHash = BlockHash;

        // Add the trivial 0, 0 block
        hashed_file_block &Block =
            TreeWriter->Blocks[TreeWriter->UsedBlockCount++];
        Block.TraversalNumber = 0;
        Block.Location.SectionIndex = TreeWriter->DefaultTypeSectionIndex;
        Block.Location.BufferIndex = 0;
        Block.Location.Offset = 0;
        Block.ObjectSection = TreeWriter->DefaultTypeSectionIndex;
        AddPointerToHash(*TreeWriter->BlockHash, 0, &Block);

        TreeWriter->StringCallback = 0;
        TreeWriter->StringCallbackData = 0;

        TreeWriter->SourceFileForSectioning = 0;
        TreeWriter->SourceFileForFormats = 0;

        // Init the type registry...
        Initialize(&TreeWriter->WrittenTypeRegistry, 0);
    }
    else
    {
        DeletePointerHash(BlockHash);
        Deallocate(TreeWriter);
    }

    return(TreeWriter);
}

void GRANNY
PreserveObjectFileSections(file_data_tree_writer& Writer,
                           file const *SourceFile)
{
    CheckPointerNotNull(SourceFile, return);
    Writer.SourceFileForSectioning = SourceFile;
}

void PreserveFileSectionFormats(file_data_tree_writer& Writer,
                                file const *SourceFile)
{
    CheckPointerNotNull(SourceFile, return);
    Writer.SourceFileForFormats = SourceFile;
}


void GRANNY
EndFileDataTreeWriting(file_data_tree_writer *Writer)
{
    if(Writer)
    {
        DeletePointerHash(Writer->BlockHash);
        FreeMemory(&Writer->WrittenTypeRegistry);
        Deallocate(Writer);
    }
}

static hashed_file_block &
GetHashedBlock(file_data_tree_writer &Writer,
               data_type_definition const *Type,
               void const *Object)
{
    hashed_file_block *Block = 0;

    void *Temp = 0;
    if(GetHashedPointerData(*Writer.BlockHash, Object, Temp))
    {
        Block = (hashed_file_block *)Temp;
    }
    else
    {
        Block = &Writer.Blocks[Writer.UsedBlockCount++];

        Block->SectionSetExplicitly = false;
        Block->TraversalNumber = 0;
        //Block->Location.SectionIndex = ... set below
        Block->Location.Offset = 0;
        Block->Location.BufferIndex = AnyMarshalling;
        Block->ObjectSection = Writer.DefaultObjectSectionIndex;

        Block->Location.SectionIndex =
            GetHashedBlock(Writer, 0, Type).ObjectSection;
        if (Writer.SourceFileForSectioning != NULL)
        {
            // Look up the object in the source file.  If it exists, and the returned
            // section is in range, use that instead.  (The user can always override with
            // SetFileSectionForObject)
            int32x SourceSection = GetFileSectionOfLoadedObject(*Writer.SourceFileForSectioning,
                                                                Object);
            if (SourceSection != -1)
            {
                Block->Location.SectionIndex = SourceSection;
                Block->SectionSetExplicitly = true;
            }
       }

        AddPointerToHash(*Writer.BlockHash, Object, Block);
    }

    Assert(Block);
    return(*Block);
}

void GRANNY
SetFileDataTreeFlags(file_data_tree_writer &Writer, uint32x Flags)
{
    Writer.Flags = Flags;
}

void GRANNY
SetFileSectionForObjectsOfType(file_data_tree_writer &Writer,
                               data_type_definition const *Type,
                               int32x SectionIndex)
{
    hashed_file_block &Block = GetHashedBlock(Writer, 0, Type);
    Block.ObjectSection = SectionIndex;
}

void GRANNY
SetFileSectionForObject(file_data_tree_writer &Writer,
                        void const *Object,
                        int32x SectionIndex)
{
    hashed_file_block &Block = GetHashedBlock(Writer, 0, Object);
    Block.Location.SectionIndex = SectionIndex;
    Block.SectionSetExplicitly = true;
}

static bool
ShouldTraverseBlock(data_type_io_context &Context,
              hashed_file_block &Block)
{
    if(Block.TraversalNumber != Context.TreeWriter->TraversalNumber)
    {
        Block.TraversalNumber = Context.TreeWriter->TraversalNumber;
        return(true);
    }

    return(false);
}

static hashed_file_block &
WriteType(data_type_io_context &Context,
          data_type_definition const *TypeDefinition);

static hashed_file_block &
WriteString(data_type_io_context &Context, char const *String)
{
    hashed_file_block &Block = GetHashedBlock(*Context.TreeWriter, StringType, String);
    if(ShouldTraverseBlock(Context, Block))
    {
        WriteFileChunk(*Context.Builder,
                       Block.Location.SectionIndex,
                       Int8Marshalling,
                       StringLength(String) + 1,
                       String,
                       &Block.Location);
    }

    return(Block);
}



struct string_hash_data
{
    uint32 HashResult;
    char const *StringPointer;
};

struct string_hash_block_data
{
    int NumStringHashes;
    string_hash_data *StringHash;
};

struct signature_call_chain
{
    data_type_definition const* Type;
    signature_call_chain* Prev;
};

static uint32
TypeSignature(data_type_io_context& Context,
              data_type_definition const* Type,
              signature_call_chain* CallChain)
{
    if (Type == 0)
        return 0;

    uint32 CRC;
    BeginCRC32(CRC);

    data_type_definition const* Walk = Type;
    while (Walk->Type != EndMember)
    {
        AddToCRC32(CRC, sizeof(Walk->Type), &Walk->Type);

        if (!Context.TreeWriter->StringCallback)
        {
            AddToCRC32(CRC, StringLength(Walk->Name), Walk->Name);
        }
        else
        {
            AddToCRC32(CRC, SizeOf(Walk->Name), &Walk->Name);
        }

        if (Walk->Type == ReferenceMember ||
            Walk->Type == ReferenceToArrayMember ||
            Walk->Type == ArrayOfReferencesMember)
        {
            if (Walk->ReferenceType != NULL)
            {
                signature_call_chain ChainEntry;
                ChainEntry.Type = Type;
                ChainEntry.Prev = CallChain;

                // Make sure that we haven't already computed the signature for this type.
                signature_call_chain* CallWalk = &ChainEntry;
                while (CallWalk)
                {
                    if (CallWalk->Type == Walk->ReferenceType)
                        break;
                    CallWalk = CallWalk->Prev;
                }

                if (CallWalk == NULL)
                {
                    // Not found, go ahead and recurse
                    uint32 SubTypeSignature =
                        TypeSignature(Context, Walk->ReferenceType, &ChainEntry);
                    AddToCRC32(CRC, sizeof(SubTypeSignature), &SubTypeSignature);
                }
            }
        }

        AddToCRC32(CRC, sizeof(Walk->ArrayWidth), &Walk->ArrayWidth);
        ++Walk;
    }

    EndCRC32(CRC);
    return CRC;
}

static hashed_file_block &
WriteType(data_type_io_context &Context,
          data_type_definition const *TypeDefinition)
{
    uint32 Signature = TypeSignature(Context, TypeDefinition, NULL);
    written_type* Written = Find(Context.WrittenTypeRegistry, Signature);
    while (Written && Written->Signature == Signature)
    {
        if (DataTypesAreEqualWithNames(TypeDefinition, Written->Type))
        {
            if (TypeDefinition != Written->Type)
                return WriteType(Context, Written->Type);
            else
                break;
        }

        Written = Next(Context.WrittenTypeRegistry, Written);
    }

    // Make sure the type is in the registry...
    if (!Written)
    {
        Add(Context.WrittenTypeRegistry, Signature, TypeDefinition);
    }

    hashed_file_block &Block = GetHashedBlock(*Context.TreeWriter, 0, TypeDefinition);
    if(ShouldTraverseBlock(Context, Block))
    {
        int32x const TypeSize = GetTotalTypeSize(TypeDefinition);
        int32x NumStringHashes = 0;
        string_hash_data *StringHash = NULL;

        // todo: we're cheating here and casting away const.  fix that, will you?
        if(Context.TreeWriter->StringCallback)
        {
            {for(data_type_definition const *MemberIterator = TypeDefinition;
                 MemberIterator && (MemberIterator->Type != EndMember);
                 ++MemberIterator)
            {
                NumStringHashes++;
            }}

            StringHash = AllocateArray ( NumStringHashes, string_hash_data );
            NumStringHashes = 0;

            {for(data_type_definition const *MemberIterator = TypeDefinition;
                 MemberIterator && (MemberIterator->Type != EndMember);
                 ++MemberIterator)
            {
                StringHash[NumStringHashes].StringPointer = MemberIterator->Name;
                StringHash[NumStringHashes].HashResult = Context.TreeWriter->StringCallback(
                    Context.TreeWriter->StringCallbackData,
                    MemberIterator->Name);
                const_cast<const char*&>(MemberIterator->Name) = (char*)(uintaddrx)StringHash[NumStringHashes].HashResult;
                NumStringHashes++;
            }}
        }

        WriteFileChunk(*Context.Builder,
                       Block.Location.SectionIndex,
                       Int32Marshalling,
                       TypeSize, TypeDefinition,
                       &Block.Location);


        // NB: Very important that this is done BEFORE we recurse.  That way the name
        // strings are put back in place if a sub type refers to us later in the tree.
        if(Context.TreeWriter->StringCallback)
        {
            int32x HashNum = 0;
            {for(data_type_definition const *MemberIterator = TypeDefinition;
                 MemberIterator && (MemberIterator->Type != EndMember);
                 ++MemberIterator)
            {
                Assert ( MemberIterator->Name == (char*)(uintaddrx)StringHash[HashNum].HashResult );
                const_cast<const char*&>(MemberIterator->Name) = StringHash[HashNum].StringPointer;
                HashNum++;
            }}
        }
        DeallocateSafe ( StringHash );
        StringHash = NULL;

        uint32x MemberOffset = 0;
        {for(data_type_definition const *MemberIterator = TypeDefinition;
             MemberIterator && (MemberIterator->Type != EndMember);
             ++MemberIterator)
        {
            if(!Context.TreeWriter->StringCallback)
            {
                char const *String = MapString(*Context.Strings, MemberIterator->Name);
                hashed_file_block &StringBlock = WriteString(Context, String);

                int32x PointerOffset;
                CheckConvertToInt32(PointerOffset, OffsetFromPtr(MemberIterator, Name),
                                    PointerOffset = (int32x)OffsetFromPtr(MemberIterator, Name); Assert(false));

                MarkFileFixup(*Context.Builder,
                              Block.Location,
                              MemberOffset + PointerOffset,
                              StringBlock.Location);
            }
            else
            {
                //--- todo: store string fixup points for optimization in file converter?
            }

            switch(MemberIterator->Type)
            {
                // Note that we don't need to do anything for variant
                // types, because they get hashed out during object
                // traversal.
                case ReferenceMember:
                case ReferenceToArrayMember:
                case ArrayOfReferencesMember:
                case InlineMember:
                {
                    hashed_file_block &TypeBlock =
                        WriteType(Context, MemberIterator->ReferenceType);

                    int32x PointerOffset;
                    CheckConvertToInt32(PointerOffset, OffsetFromPtr(MemberIterator, ReferenceType),
                                        PointerOffset = (int32x)OffsetFromPtr(MemberIterator, ReferenceType); Assert(false));

                    MarkFileFixup(*Context.Builder,
                                  Block.Location,
                                  MemberOffset + PointerOffset,
                                  TypeBlock.Location);
                } break;

                case UnsupportedMemberType_Remove:
                {
                    InvalidCodePath(!"Switchable types no longer supported in Granny 2.7+");
                } break;

                default:
                {
                    // Nothing to do
                } break;
            }

            MemberOffset += SizeOf(data_type_definition);
        }}
    }

    return(Block);
}


static void
ClearUnreferencedPointers(data_type_definition const *TypeDefinition,
                          void* Object)
{
    data_type_definition const *MemberIterator = TypeDefinition;
    uint8 *MemberData = (uint8 *)Object;
    while(MemberIterator && (MemberIterator->Type != EndMember))
    {
        switch (MemberIterator->Type)
        {
            case InlineMember:
            {
                ClearUnreferencedPointers(MemberIterator->ReferenceType, MemberData);
            } break;


            case ReferenceMember:
            {
                // void*
                Assert(MemberIterator->ReferenceType);
                if (MemberIterator->ReferenceType->Type == EndMember)
                {
                    // If the type pointed to is empty, i.e, the type definition is /only/
                    // EndMember, don't write out the object.
                    *((void**)MemberData) = NULL;
                }
            } break;

            case EmptyReferenceMember:
            {
                // void*
                Assert(!MemberIterator->ReferenceType);
                *((void**)MemberData) = NULL;
            } break;

            case ReferenceToArrayMember:
            case ArrayOfReferencesMember:
            {
                int32 Count;
                void* Pointer;
                ExtractCountAndPtr(Count, Pointer, void*, MemberData);
                if (Count == 0 || Pointer == NULL)
                {
                    SetCountAndPtr(0, NULL, void*, MemberData);
                }
            } break;

            case VariantReferenceMember:
            {
                // void* + void*
                data_type_definition* TypePointer = *((data_type_definition**)(MemberData));
                if (!TypePointer)
                {
                    // If the type pointer is null, we never write the object.  The
                    // converse is NOT true.  We write the type if the object pointer is
                    // null.
                    *((void**)(MemberData + SizeOf(void*))) = NULL;
                }
                else if (TypePointer->Type == EndMember)
                {
                    // If the type pointed to is empty, i.e, the type definition is /only/
                    // EndMember, don't write out the object.
                    *((void**)(MemberData + SizeOf(void*))) = NULL;
                }
            } break;

            case ReferenceToVariantArrayMember:
            {
                // void* + int32 + void*
                data_type_definition** TypePointer = ((data_type_definition**)(MemberData));
                int32* Count = (int32*)(MemberData + SizeOf(void*));
                void** ObjectPointer = (void**)(MemberData + SizeOf(void*) + SizeOf(int32));

                // Any of these conditions will cause the array not to be written out
                if (*TypePointer == NULL ||
                    *Count == 0 ||
                    *ObjectPointer == NULL)
                {
                    *TypePointer = 0;
                    *Count = 0;
                    *ObjectPointer = 0;
                }
                else if (*TypePointer != NULL && (*TypePointer)->Type == EndMember)
                {
                    // If the type pointed to is empty, i.e, the type definition is /only/
                    // EndMember, don't write out the object.
                    *TypePointer = 0;
                    *Count = 0;
                    *ObjectPointer = 0;
                }
            } break;

            default:
                break;
        }

        int32x TypeSize = GetMemberTypeSize(*MemberIterator);
        MemberData += TypeSize;
        ++MemberIterator;
    }
}



static string_hash_block_data
StringHashingFind(data_type_io_context &Context,
                  data_type_definition const *TypeDefinition,
                  void const *Object)
{
    string_hash_block_data Result;
    Result.NumStringHashes = 0;
    Result.StringHash = NULL;

    if(Context.TreeWriter->StringCallback)
    {
        data_type_definition const *MemberIterator = TypeDefinition;
        uint8 const *MemberData = (uint8 const *)Object;
        while(MemberIterator && (MemberIterator->Type != EndMember))
        {
            int32x TypeSize = GetMemberTypeSize(*MemberIterator);
            if(MemberIterator->Type == StringMember)
            {
                Result.NumStringHashes++;
            }
            MemberData += TypeSize;
            ++MemberIterator;
        }

        Result.StringHash = AllocateArray ( Result.NumStringHashes, string_hash_data );
        Result.NumStringHashes = 0;

        MemberIterator = TypeDefinition;
        MemberData = (uint8 const *)Object;
        while(MemberIterator && (MemberIterator->Type != EndMember))
        {
            int32x TypeSize = GetMemberTypeSize(*MemberIterator);

            if(MemberIterator->Type == StringMember)
            {
                Result.StringHash[Result.NumStringHashes].StringPointer = *(char **)MemberData;
                Result.StringHash[Result.NumStringHashes].HashResult = Context.TreeWriter->StringCallback(
                                Context.TreeWriter->StringCallbackData, *(char **)MemberData);
                Result.NumStringHashes++;
            }

            MemberData += TypeSize;
            ++MemberIterator;
        }
    }

    return Result;
}

static void
StringHashingApply ( string_hash_block_data &StringHashBlock,
                     data_type_io_context &Context,
                     data_type_definition const *TypeDefinition,
                     void const *Object)
{
    if ( StringHashBlock.NumStringHashes > 0 )
    {
        int32x CurStringHash = 0;

        data_type_definition const *MemberIterator = TypeDefinition;
        uint8 const *MemberData = (uint8 const *)Object;
        while(MemberIterator && (MemberIterator->Type != EndMember))
        {
            int32x TypeSize = GetMemberTypeSize(*MemberIterator);

            if(MemberIterator->Type == StringMember)
            {
                Assert ( *(char **)MemberData == StringHashBlock.StringHash[CurStringHash].StringPointer );
                *(uint32 *)MemberData = StringHashBlock.StringHash[CurStringHash].HashResult;
                CurStringHash++;
                Assert ( CurStringHash <= StringHashBlock.NumStringHashes );
            }

            MemberData += TypeSize;
            ++MemberIterator;
        }

        Assert ( CurStringHash == StringHashBlock.NumStringHashes );
    }
}

static void
StringHashingUndo ( string_hash_block_data &StringHashBlock,
                    data_type_io_context &Context,
                    data_type_definition const *TypeDefinition,
                    void const *Object)
{
    if ( StringHashBlock.NumStringHashes > 0 )
    {
        int32x CurStringHash = 0;

        data_type_definition const *MemberIterator = TypeDefinition;
        uint8 const *MemberData = (uint8 const *)Object;
        while(MemberIterator && (MemberIterator->Type != EndMember))
        {
            int32x TypeSize = GetMemberTypeSize(*MemberIterator);

            if(MemberIterator->Type == StringMember)
            {
                Assert ( *(uint32 *)MemberData == StringHashBlock.StringHash[CurStringHash].HashResult );
                *(char const **)MemberData = StringHashBlock.StringHash[CurStringHash].StringPointer;
                CurStringHash++;
                Assert ( CurStringHash <= StringHashBlock.NumStringHashes );
            }

            MemberData += TypeSize;
            ++MemberIterator;
        }

        Assert ( CurStringHash == StringHashBlock.NumStringHashes );
    }
}


static void
StringHashingFree ( string_hash_block_data &StringHashBlock )
{
    DeallocateSafe ( StringHashBlock.StringHash );
    StringHashBlock.NumStringHashes = 0;
}



static hashed_file_block &
HandleArray(data_type_io_context &Context,
            data_type_definition const &MemberType,
            data_type_definition const *ReferenceType,
            uint32x MemberSectionIndex,
            int32x Count, uint8* Array)
{
    hashed_file_block &Block =
        GetHashedBlock(*Context.TreeWriter, 0, Array);
    if(ShouldTraverseBlock(Context, Block))
    {
        if(!Block.SectionSetExplicitly)
        {
            Block.Location.SectionIndex = MemberSectionIndex;
        }

        uint32x const TypeSize = GetTotalObjectSize(ReferenceType);
        uint32x const Marshalling = GetObjectMarshalling(ReferenceType);

        string_hash_block_data *StringHashBlockData = NULL;

        if(TypeHasPointers(ReferenceType))
        {
            StringHashBlockData = AllocateArray ( Count, string_hash_block_data );

            uint8 const *ArrayPtr = Array;
            {for(int32x Index = 0;
                 Index < Count;
                 ++Index)
            {
                //HandleStringHashing(Context, ReferenceType, ArrayPtr);
                StringHashBlockData[Index] = StringHashingFind ( Context, ReferenceType, ArrayPtr );
                StringHashingApply ( StringHashBlockData[Index], Context, ReferenceType, ArrayPtr );
                ArrayPtr += TypeSize;
            }}
        }

        // Clear out the zero count array pointers...
        {for (int32x Idx = 0; Idx < Count; ++Idx)
        {
            ClearUnreferencedPointers(ReferenceType, (void*)(Array + (TypeSize * Idx)));
        }}

        WriteFileChunk(*Context.Builder,
                       Block.Location.SectionIndex,
                       Marshalling,
                       Count * TypeSize, Array,
                       &Block.Location);
        if(IsMixedMarshalling(Marshalling))
        {
            hashed_file_block &TypeBlock = WriteType(Context, ReferenceType);
            MarkMarshallingFixup(*Context.Builder,
                                 TypeBlock.Location,
                                 Block.Location,
                                 Count);
        }

        if(TypeHasPointers(ReferenceType))
        {
            uint8* ArrayPtr = Array;
            uint32 ArrayOffset = 0;
            {for(int32x Index = 0;
                 Index < Count;
                 ++Index)
            {
                file_location Offset;
                OffsetFileLocation(*Context.Builder,
                                   Block.Location,
                                   ArrayOffset,
                                   &Offset);

                WriteObjectInPlace(Context, ReferenceType,
                                   Offset,
                                   ArrayPtr);
                StringHashingUndo ( StringHashBlockData[Index], Context, ReferenceType, ArrayPtr );
                StringHashingFree ( StringHashBlockData[Index] );
                ArrayOffset += TypeSize;
                ArrayPtr += TypeSize;

            }}
        }
        else
        {
#if DEBUG_TYPE_IO
            Log(NoteLogMessage, DataTypeLogMessage,
                "Early-outting %s", MemberType.Name);
#endif
        }

        if ( StringHashBlockData != NULL )
        {
            Deallocate ( StringHashBlockData );
        }
    }

    return(Block);
}

static void
HandleMember(data_type_io_context &Context,
             data_type_definition const &MemberType,
             file_location const &MemberLocation,
             uint8* MemberData)
{
    // TODO: Section index here?  Everywhere?
    switch(MemberType.Type)
    {
        case ReferenceMember:
        {
            void *Pointer = *(void **)MemberData;
            if(Pointer)
            {
                hashed_file_block &ReferenceBlock = WriteObject(
                    Context, MemberType.ReferenceType, Pointer);

                MarkFileFixup(*Context.Builder,
                              MemberLocation, 0,
                              ReferenceBlock.Location);
            }
        } break;

        case VariantReferenceMember:
        {
            data_type_definition *TypePointer =
                *(data_type_definition **)MemberData;
            if(TypePointer)
            {
                hashed_file_block &TypeBlock = WriteType(Context, TypePointer);

                MarkFileFixup(*Context.Builder,
                              MemberLocation, 0, TypeBlock.Location);

                MemberData += SizeOf(data_type_definition *);

                void *Pointer = *(void **)MemberData;
                if(Pointer)
                {
                    hashed_file_block &VariantBlock =
                        WriteObject(Context, TypePointer, Pointer);

                    MarkFileFixup(*Context.Builder,
                                  MemberLocation,
                                  SizeOf(data_type_definition *),
                                  VariantBlock.Location);
                }
            }
        } break;

        case ReferenceToVariantArrayMember:
        {
            void** Pointer = (void**)MemberData;
            data_type_definition *TypePointer =
                *(data_type_definition **)Pointer++;
            if(TypePointer)
            {
                hashed_file_block &TypeBlock = WriteType(Context, TypePointer);

                MarkFileFixup(*Context.Builder,
                              MemberLocation, 0, TypeBlock.Location);

                int32x Count;
                uint8* Array;
                ExtractCountAndPtr(Count, Array, uint8*, Pointer);
                Count *= GetMemberArrayWidth(MemberType);

                if(Count && Array)
                {
                    hashed_file_block &ArrayBlock =
                        HandleArray(Context, MemberType, TypePointer,
                                    MemberLocation.SectionIndex,
                                    Count, Array);
                    MarkFileFixup(*Context.Builder,
                                  MemberLocation, SizeOf(uint32) + SizeOf(void*),
                                  ArrayBlock.Location);
                }
                else
                {
                    // We don't allow these to be disconnected anymore
                    Assert(Count == 0 && Array == NULL);
                }
            }
        } break;

        case ReferenceToArrayMember:
        {
            uint32 *Pointer = (uint32 *)MemberData;

            int32x Count;
            uint8* Array;
            ExtractCountAndPtr(Count, Array, uint8*, Pointer);
            Count *= GetMemberArrayWidth(MemberType);

            if(Count && Array)
            {
                hashed_file_block &ArrayBlock =
                    HandleArray(Context, MemberType, MemberType.ReferenceType,
                                MemberLocation.SectionIndex,
                                Count, Array);
                MarkFileFixup(*Context.Builder,
                              MemberLocation, SizeOf(uint32),
                              ArrayBlock.Location);
            }
            else
            {
                // We don't allow these to be disconnected anymore
                Assert(Count == 0 && Array == NULL);
            }
        } break;

        case ArrayOfReferencesMember:
        {
            uint32 *Pointer = (uint32 *)MemberData;

            int32x Count;
            void** Array;
            ExtractCountAndPtr(Count, Array, void**, Pointer);
            Count *= GetMemberArrayWidth(MemberType);

            if(Count && Array)
            {
                file_location ArrayLocation;
                WriteFileChunk(*Context.Builder,
                               MemberLocation.SectionIndex,
                               AnyMarshalling,
                               Count * SizeOf(void *),
                               Array,
                               &ArrayLocation);

                MarkFileFixup(*Context.Builder,
                              MemberLocation, SizeOf(uint32),
                              ArrayLocation);

                int32x ArrayOffset = 0;
                while(Count--)
                {
                    if(*Array)
                    {
                        hashed_file_block &ReferenceBlock = WriteObject(
                            Context, MemberType.ReferenceType,
                            *Array);
                        MarkFileFixup(*Context.Builder,
                                      ArrayLocation, ArrayOffset,
                                      ReferenceBlock.Location);
                    }

                    ++Array;
                    ArrayOffset += SizeOf(void *);
                }
            }
            else
            {
                // We don't allow these to be disconnected anymore
                Assert(Count == 0 && Array == NULL);
            }
        } break;

        case StringMember:
        {
            if(!Context.TreeWriter->StringCallback)
            {
                char *StringPointer = *(char **)MemberData;

                char const *String = MapString(*Context.Strings, StringPointer);
                hashed_file_block &StringBlock = WriteString(Context, String);

                MarkFileFixup(*Context.Builder, MemberLocation, 0, StringBlock.Location);
            }
        } break;

        case EndMember:
        case TransformMember:
        case Real32Member:
        case Real16Member:
        case Int8Member:
        case UInt8Member:
        case BinormalInt8Member:
        case NormalUInt8Member:
        case Int16Member:
        case UInt16Member:
        case BinormalInt16Member:
        case NormalUInt16Member:
        case Int32Member:
        case UInt32Member:
        case EmptyReferenceMember:
        {
            // Nothing special to do for this type
        } break;

        case InlineMember:
        {
            WriteObjectInPlace(Context, MemberType.ReferenceType,
                               MemberLocation, MemberData);
        } break;

        case UnsupportedMemberType_Remove:
        {
            InvalidCodePath("Switchable types no longer supported in Granny 2.7+");
        } break;

        default:
        {
            // Illegal type
            InvalidCodePath("Unrecognized type");
        } break;
    }
}

static void
WriteObjectInPlace(data_type_io_context &Context,
                   data_type_definition const *TypeDefinition,
                   file_location const &ObjectLocation,
                   void* Object)
{
    data_type_definition const *MemberIterator = TypeDefinition;
    file_location MemberLocation = ObjectLocation;
    uint8* MemberData = (uint8*)Object;
    while(MemberIterator && (MemberIterator->Type != EndMember))
    {
#if DEBUG_TYPE_IO
        Log(NoteLogMessage, DataTypeLogMessage,
            "%s", MemberIterator->Name);
#endif

        int32x TypeSize = GetMemberTypeSize(*MemberIterator);
        HandleMember(Context, *MemberIterator, MemberLocation, MemberData);
        MemberData += TypeSize;
        OffsetFileLocation(*Context.Builder,
                           MemberLocation, TypeSize,
                           &MemberLocation);
        ++MemberIterator;
    }
}

static hashed_file_block &
WriteObject(data_type_io_context &Context,
            data_type_definition const *TypeDefinition,
            void* Object)
{
    hashed_file_block &Block = GetHashedBlock(*Context.TreeWriter, TypeDefinition, Object);
    if(ShouldTraverseBlock(Context, Block))
    {
        // Need to ensure that the array pointers with zero count are cleared.
        ClearUnreferencedPointers(TypeDefinition, Object);

        //HandleStringHashing(Context, TypeDefinition, Object);
        string_hash_block_data StringHashBlockData = StringHashingFind ( Context, TypeDefinition, Object );
        StringHashingApply ( StringHashBlockData, Context, TypeDefinition, Object );

        if ( Context.TreeWriter->StringCallback )
        {
            // If this type is a track_group, make sure we mark it as unsorted before writing it out,
            // because the CRCs will not be sorted even if the original strings were.
            // They will want to resort them on loading for more speed, but at least it now always works.
            if ( TypeDefinition == TrackGroupType )
            {
                track_group *TrackGroup = (track_group *)Object;
                TrackGroup->Flags &= ~TrackGroupIsSorted;
            }
            // I don't think it matters if we don't set the flag again afterwards.
        }

        int32x const ObjectSize = GetTotalObjectSize(TypeDefinition);
        uint32x Marshalling = GetObjectMarshalling(TypeDefinition);
        WriteFileChunk(*Context.Builder,
                       Block.Location.SectionIndex,
                       Marshalling,
                       ObjectSize, Object,
                       &Block.Location);

        if(IsMixedMarshalling(Marshalling))
        {
            hashed_file_block &TypeBlock = WriteType(Context, TypeDefinition);
            MarkMarshallingFixup(*Context.Builder,
                                 TypeBlock.Location,
                                 Block.Location,
                                 1);
        }

        WriteObjectInPlace(Context, TypeDefinition, Block.Location, Object);

        StringHashingUndo ( StringHashBlockData, Context, TypeDefinition, Object );
        StringHashingFree ( StringHashBlockData );
    }

    return(Block);
}

bool GRANNY
WriteDataTreeToFileBuilder(file_data_tree_writer &Writer,
                           file_builder &Builder)
{
    bool Result = false;

    ++Writer.TraversalNumber;

    // Sublety: if the platform tag for the file_builder isn't the native platform tag, we
    // ignore the ExcludeTypeTree flag for this function.  In the EndFile call, the
    // Builder will write out the file to a temp buffer, read it in and convert it using
    // the PlatformConvert api.  That API /requires/ the presence of type information in
    // the file to carry out it's operation.  We'll set a flag in the Builder so that it
    // knows to tell the converter to exclude the type info on conversion.
    bool Ignored;
    if (!DoesMagicValueMatch(Builder.PlatformMagicValue, GRNFileMV_ThisPlatform, Ignored))
    {
        Builder.ExcludeTypeTreeOnConversion = (Writer.Flags & ExcludeTypeTree) != 0;
        Writer.Flags &= ~ExcludeTypeTree;
    }

    string_table *Strings = NewStringTable();
    if(Strings)
    {
        data_type_io_context Context;
        Context.TreeWriter = &Writer;
        Context.Builder = &Builder;
        Context.Strings = Strings;
        Context.BlockHash = Writer.BlockHash;
        Context.WrittenTypeRegistry = &Writer.WrittenTypeRegistry;
        Context.UsedStringFixups = 0;
        Context.StringFixups = Writer.StringFixups;

        hashed_file_block const &RootObjectBlock =
            WriteObject(Context, Writer.RootObjectTypeDefinition, Writer.RootObject);
        hashed_file_block const &RootObjectTypeBlock =
            WriteType(Context,
                      (Writer.Flags & ExcludeTypeTree) ?
                      EmptyType : Writer.RootObjectTypeDefinition);

        MarkFileRootObject(*Context.Builder,
                           RootObjectTypeBlock.Location,
                           RootObjectBlock.Location);

        Result = true;

        FreeStringTable(Strings);
    }
    else
    {
        Log0(ErrorLogMessage, FileWritingLogMessage,
             "Unable to create file string table builder");
    }

    return(Result);
}

bool GRANNY
WriteDataTreeToFile(file_data_tree_writer &Writer,
                    uint32 FileTypeTag,
                    grn_file_magic_value const& PlatformMagicValue,
                    char const *FileName,
                    int32x FileSectionCount)
{
    bool Result = false;

    file_builder *Builder = BeginFile(FileSectionCount, FileTypeTag,
                                      PlatformMagicValue,
                                      ".", "WriteDataTreeToFile");
    if(Builder)
    {
        Result = true;

        if(!WriteDataTreeToFileBuilder(Writer, *Builder))
        {
            Result = false;
            Log0(ErrorLogMessage, FileWritingLogMessage,
                 "Unable to write tree contents to file buffer");
        }

        if(!EndFile(Builder, FileName))
        {
            Result = false;
            Log0(ErrorLogMessage, FileWritingLogMessage,
                 "Unable to write buffer to file");
        }
    }
    else
    {
        Log0(ErrorLogMessage, FileWritingLogMessage,
             "Unable to create file writing buffer");
    }

    return(Result);
}

void GRANNY
SetFileWriterStringCallback(file_data_tree_writer &Writer,
                            file_writer_string_callback *Callback,
                            void *Data)
{
    Writer.StringCallback = Callback;
    Writer.StringCallbackData = Data;
}



