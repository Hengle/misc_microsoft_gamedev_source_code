/**********************************************************************

Filename    :   GFxMovieDef.h
Content     :   GFxMovieDataDef and GFxMovieDefImpl classes used to
                represent loaded and bound movie data, respectively.
Created     :   
Authors     :   Michael Antonov

Copyright   :   (c) 2001-2006 Scaleform Corp. All Rights Reserved.

Notes       :   Starting with GFx 2.0, loaded and bound data is stored
                separately in GFxMovieDataDef and GFxMovieDefImpl
                classes. GFxMovieDataDef represents data loaded from
                a file, shared by all bindings. GFxMovieDefImpl binds
                resources referenced by the loaded data, by providing
                a custom ResourceBinding table. Bindings can vary
                depending on GFxState objects specified during loading.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef INC_GFXMOVIEDEF_H
#define INC_GFXMOVIEDEF_H

#include "GTLTypes.h"

#include "GFxCharacterDef.h"
#include "GFxResourceHandle.h"
#include "GFxLoaderImpl.h"
#include "GFxFontResource.h"
#include "GFxFontLib.h"
#include "GFxStringHash.h"
#include "GFxActionTypes.h"
#include "GFxFilterDesc.h"

// For GFxTask base.
#include "GFxTaskManager.h"


// ***** Declared Classes

class GFxMovieDataDef;
class GFxMovieDefImpl;
class GFxSpriteDef;
class GFxSprite;
// Helpers
class GFxImportInfo;
class GFxSwfEvent;

// Tag classes
class GFxPlaceObject2;
class GFxRemoveObject2;
class GFxSetBackgroundColor;
// class GASDoAction;           - in GFxAction.cpp
// class GFxStartSoundTag;      - in GFxSound.cpp


class GFxMovieBindProcess;

// ***** External Classes

class GFxLoadStates;

class GFxFontPackParams;

class GFxMovieRoot;
class GFxLoadQueueEntry;

class GJPEGInput;




// ***** Movie classes

    
// Helper for GFxMovieDefImpl
class GFxImportData
{  
public:

    struct Symbol
    {
        GFxString   SymbolName;
        int         CharacterId;
        UInt        BindIndex;
    };

    GTL::garray<Symbol> Imports;
    GFxString           SourceUrl;
    UInt                Frame;
    
    // Index of this import in the file, used to index
    // us within GFxMovieDefImpl::ImportSourceMovies array.
    UInt            ImportIndex;

    // Pointer to next import node.
    GAtomicPtr<GFxImportData>  pNext;

    GFxImportData()
        : Frame(0), ImportIndex(0)
    { }

    GFxImportData(const char* source, UInt frame)
        : SourceUrl(source), Frame(frame), ImportIndex(0)
    { }

    void    AddSymbol(const char* psymbolName, int characterId, UInt bindIndex)
    {
        Symbol s;
        s.SymbolName  = psymbolName;
        s.CharacterId = characterId;
        s.BindIndex   = bindIndex;
        Imports.push_back(s);
    }

};

struct GFxResourceDataNode
{
    // Resource binding data used here.
    GFxResourceData      Data;
    UInt                 BindIndex;
    GAtomicPtr<GFxResourceDataNode> pNext;    

    GFxResourceDataNode()        
    {  }
};

// Font data reference. We need to keep these in case we need to substitute fonts.
// We can scan through this list to figure out which fonts to substitute.
struct GFxFontDataUseNode
{
    GFxResourceId       Id;
    // pointer is technically redundant
    GPtr<GFxFontData>   pFontData;          
    // This font data is used at specified font index.
    UInt                BindIndex;

    GAtomicPtr<GFxFontDataUseNode> pNext;

    GFxFontDataUseNode()
        : BindIndex(0)
    { }
};


// Frame bind data consists of 
// - imports, fonts, resource data for creation

struct GFxFrameBindData
{
    // Frame that this structure represents. Stroring this field
    // allows us to skip frames that don't require binding.
    UInt                Frame;
    // Number of bytes loaded by this frame.
    UInt                BytesLoaded;

    // Number of Imports, Resources, and Fonts for this frame.
    // We use these cumbers instead of traversing the entire linked
    // lists, which can include elements from other frames.
    UInt                FontCount;
    UInt                ImportCount;
    UInt                ResourceCount;
   
    // Singly linked lists of imports for this frame.   
    GFxImportData*      pImportData;
    // Fonts referenced in this frame.
    GFxFontDataUseNode* pFontData;

    // A list of ResourceData objects associated with each bindIndex in GFxResourceHandle.
    // Except for imports, which will have an invalid data object, appropriate resources can
    // be created by calling GFxResourceData::CreateResource
    GFxResourceDataNode* pResourceData;
       
    // Pointer to next binding frame. It will be 0 if there are
    // either no more frames of they haven't been loaded yet (this
    // pointer is updated asynchronously while FrameUpdateMutex
    // is locked).    
    GAtomicPtr<GFxFrameBindData> pNextFrame;

    GFxFrameBindData()
        : Frame(0), BytesLoaded(0),
          FontCount(0), ImportCount(0), ResourceCount(0),
          pImportData(0), pFontData(0), pResourceData(0)
    { }
};



// ***** GFxDataAllocator

// GFxDataAllocator is a simple allocator used for tag data in GFxMovieDef.
// Memory is allocated consecutively and can only be freed all at once in a
// destructor. This scheme is used to save on allocation overhead, to reduce
// the number of small allocated chunks and to improve loading performance.

class GFxDataAllocator
{   
    struct Block
    {
        Block*  pNext;
    };

    enum AllocConstants
    {
        // Leave 4 byte space to be efficient with buddy allocator.
        BlockSize = 8192 - sizeof(Block*) - 8
    };

    // Current pointer and number of bytes left there.
    UByte*   pCurrent;
    size_t   BytesLeft;
    // All allocated blocks, including current.
    Block*  pAllocations;

    void*   OverflowAlloc(size_t bytes);

public:

    GFxDataAllocator();
    ~GFxDataAllocator();

    inline void*   Alloc(size_t bytes)
    {
        // Round up to pointer size.
		bytes = (bytes + (SYSTEMALIGNMENT - 1)) & ~(SYSTEMALIGNMENT - 1);

        if (bytes <= BytesLeft)
        {
            void* pmem = pCurrent;
            pCurrent += bytes;
            BytesLeft -= bytes;
            return pmem;
        }
        return OverflowAlloc(bytes);
    }

    // Allocates an individual chunk of memory, without using the pool.
    // The chunk will be freed later together with the pool.
    void*    AllocIndividual(size_t bytes);

};



// ***** GFxMovieDataDef - Stores file loaded data.

class GFxMovieDataDef : public GFxTimelineDef
{
public:

    // Store the resource key used to create us.
    // Note: There is a bit of redundancy here with respect to filename;
    // could resolve it with shared strings later on...
    GFxResourceKey          ResourceKey;    

    // MovieDataDefType - Describe the type of data that is stored in this DataDef.    
    enum MovieDataType
    {
        MT_Empty,   // No data, empty clip - useful for LoadVars into new level.
        MT_Flash,   // SWF or GFX file.
        MT_Image    // An image file.
    };

    MovieDataType            MovieType;


    // *** Binding and enumeration lists

    // DefBindingData contains a number of linked lists which are updated during
    // by the LoadProcess and used later on by BindProcess. Linked lists are used 
    // here because they can be updated without disturbing the reader thread; all
    // list data nodes are allocated from tag allocator. Containing DefBindingData
    // to a single object allows it to be easily passed to LoadProcess, which can
    // add items to appropriate lists. If load process fails, these entries are
    // correctly cleaned up by the virtue of being here.
    // 
    // The following points are significant:
    //  - pFrameData / pFrameDataLast writes are protected by the FrameUpdateMutex
    //    and can be waited on through FrameUpdated WaitCondition.
    //  - pImports and pFonts lists are maintained to allow independent traversal
    //    from other non-bind process functions. Parts of these lists are also
    //    referenced from GFxFrameBindData, limited by a count.
    //  - pResourceNodes is kept to allow destructors to be called.
    //  - GAtomicPtr<> is used to have proper Acquire/Release semantics on
    //    variables updated across multiple threads. 'pLast' pointers do not need
    //    sync because they are for loading thread housekeeping only.

    struct DefBindingData
    {
        // A linked list of binding data for each frame that requires it.
        // This may be empty if no frames required binding data.
        GAtomicPtr<GFxFrameBindData>    pFrameData;
        GFxFrameBindData*               pFrameDataLast;

        // A linked list of all import nodes.
        GAtomicPtr<GFxImportData>       pImports;
        GFxImportData*                  pImportsLast;
        // A linked list of all fonts; we keep these in case we need to substitute
        // fonts. Scan through this list to figure out which fonts to substitute.
        GAtomicPtr<GFxFontDataUseNode>  pFonts;
        GFxFontDataUseNode*             pFontsLast;

        // A list of resource data nodes; used for cleanup.
        GAtomicPtr<GFxResourceDataNode> pResourceNodes;
        GFxResourceDataNode*            pResourceNodesLast;

        DefBindingData()
            : pFrameDataLast(0),
            pImportsLast(0), pFontsLast(0), pResourceNodesLast(0)
        { }

        // Clean up data instanced created from tag allocator.
        ~DefBindingData();
    };


    // GFxMovieDataDef starts out in LS_Uninitialized and switches to
    // the LS_LoadingFrames state as the frames start loading. During this
    // process the LoadingFrames counter will be incremented, with playback
    // being possible after it is greater then 0. 
    // For image files, state can be set immediately to LS_LoadFinished
    // while LoadingFrame is set to 1.
    enum MovieLoadState
    {
        LS_Uninitialized,
        LS_LoadingFrames,
        LS_LoadFinished,
        LS_LoadCanceled, // Canceled by user.
        LS_LoadError
    };


    // *** LoadTaskData

    // Most of the GFxMovieDataDef data is contained in LoadTaskData class,
    // which is AddRef-ed separately. Such setup is necessary to allow
    // the loading thread to be canceled when all user reference count
    // for GFxMovieDataDef are released. The loading thread only references
    // the LoadData object and thus can be notified when GFxMovieDataDef dies.

    // Use a base class for LoadTaskData to make sure that TagMemAllocator
    // is destroyed last. That is necessary because LoadTaskData contains
    // multiple references to sprites and objects that use that allocator
    class LoadTaskDataBase : public GRefCountBase<LoadTaskDataBase>
    {
    protected:
        // Tag/Frame Array Memory allocator.
        // Memory is allocated permanently until GFxMovieDataDef/LoadTaskData dies.
        GFxDataAllocator        TagMemAllocator;

        // Path allocator used for shape data. Use a pointer
        // to avoid including GFxShape.h.
        class GFxPathAllocator* pPathAllocator;
    };

    // Resource hash table used in LoadTaskData.
    typedef GTL::ghash_uncached<
        GFxResourceId, GFxResourceHandle, GFxResourceId::HashOp>  ResourceHash;


    class LoadTaskData : public LoadTaskDataBase
    {
        friend class GFxMovieDataDef;
        friend class GFxLoadProcess;
        friend class GFxMovieDefImpl;
        friend class GFxMovieBindProcess;


        // *** Header / Info Tag data.

        // General info about an SWF/GFX file

        // File path passed to FileOpenCallback for loading this object, un-escaped.
        // For Flash, this should always include the protocol; however, we do not require that.
        // For convenience of users we also support this being relative to the cwd.
        GFxString               FileURL;
        // Export header.
        GFxMovieHeaderData      Header;
        // These attributes should become available any time after loading has started.
        // File Attributes, described by FileAttrType.
        UInt                    FileAttributes;
        // String storing meta-data. May have embedded 0 chars ?
        UByte*                  pMetadata;
        UInt                    MetadataSize;


        // *** Current Loading State        

        // These values are updated based the I/O progress.
        MovieLoadState          LoadState;
        // Current frame being loaded; playback/rendering is only possible
        // when this value is greater then 0.
        UInt                    LoadingFrame;
        // This flag is set to 'true' if loading is being canceled;
        // this flag will be set if the loading process should terminate.
        // This value is different from LoadState == BS_Canceled
        volatile bool           LoadingCanceled;

        // Tag Count, used for reporting movie info; also advanced during loading.
        // Serves as a general information 
        UInt                    TagCount;


#ifndef GFC_NO_THREADSUPPORT
        // Mutex obtained when incrementing a frame and
        // thus adding data to pFrameBindData.
        GMutex                  FrameUpdateMutex;
        GWaitCondition          FrameUpdated;
#endif

        // Read-time updating linked lists of binding data, as 
        // described by comments above for DefBindingData.
        DefBindingData          BindData;


        // *** Resource Data

        // Counter used to assign resource handle indices during loading.
        UInt                    ResIndexCounter;

        // Resource data is progressively loaded as new tags become available; during
        // loading new entries are added to the data structures below.
       
        // Lock applies to all resource data; only used during loading,
        // i.e. when (pdataDef->LoadState < LS_LoadFinished).
        // Locks: Resources, Exports and InvExports.
        mutable GLock            ResourceLock;

        // {resourceCharId -> resourceHandle} 
        // A library of all resources defined in this SWF/GFX file.
        ResourceHash            Resources;

        // A resource can also be exported
        // (it is possible to both import AND export a resource in the same file).
        // {export SymbolName -> Resource}   
        GFxStringHash< GFxResourceHandle >     Exports;
        // Inverse exports map from GFxResourceId, since mapping from GFxResourceHandle
        // would not be useful (we don't know how to create a handle from a GFxResource*).
        // This is used primarily in GetNameOfExportedResource.
        GTL::ghash<GFxResourceId, GFxString>   InvExports;


        // *** Playlist Frames

        // Playlist lock, only applied when LoadState < LoadingFrames.
        // Locks: Playlist, InitActionList, NamedFrames.
        // TBD: Can become a bottleneck in FindPreviousPlaceObject2 during loading
        //      if a movie has a lot of frames and seek-back occurs.
        mutable GLock           PlaylistLock;

        GTL::garray<Frame>      Playlist;          // A list of movie control events for each frame.
        GTL::garray<Frame>      InitActionList;    // Init actions for each frame.
        int                     InitActionsCnt;    // Total number of init action buffers in InitActionList

        GFxStringHash<UInt>        NamedFrames;        // 0-based frame #'s

        // Incremented progressively as gradients are loaded to assign ids
        // to their data, and match them with loaded files.
        GFxResourceId           GradientIdGenerator;

    public:

        // ***** Initialization / Cleanup

        LoadTaskData(GFxMovieDataDef* pdataDef, const char *purl);
        ~LoadTaskData();

        // Notifies LoadTaskData that MovieDataDef is being destroyed. This may be a
        // premature destruction if we are in the process of loading (in that case it
        // will lead to loading being canceled).
        void                OnMovieDataDefRelease();

        // Create an empty usable definition with no content.        
        void                InitEmptyMovieDef();
        // Create a definition describing an image file.
        void                InitImageFileMovieDef(UInt fileLength,
                                                  GFxImageResource *pimageResource, bool bilinear = 1);


        // *** Accessors

        // Get allocator used for path shape storage.
        GFxPathAllocator*   GetPathAllocator()          { return pPathAllocator; }

        const char*         GetFileURL() const          { return FileURL.ToCStr(); }
        UInt                GetVersion() const          { return Header.Version; }
        const GFxExporterInfo*  GetExporterInfo() const { return Header.ExporterInfo.GetExporterInfo(); }

        
        // ** Loading

        // Initializes SWF/GFX header for loading.
        void                BeginSWFLoading(const GFxMovieHeaderData &header);

        // Read a .SWF/GFX pMovie. Binding will take place interleaved with
        // loading if passed; no binding is done otherwise.
        void                Read(GFxLoadProcess *plp, GFxMovieBindProcess* pbp = 0);
        
        // Updates bind data and increments LoadingFrame. 
        // It is expected that it will also release any threads waiting on it in the future.
        // Returns false if there was an error during loading,
        // in which case LoadState is set to LS_LoadError can loading finished.
        bool                FinishLoadingFrame(GFxLoadProcess* plp, bool finished = 0);

        // Update frame and loading state with proper notification, for use image loading, etc.
        void                UpdateLoadState(UInt loadingFrame, MovieLoadState loadState);
        // Signals frame updated; can be used to wake up threads waiting on it
        // if another condition checked externally was met (such as cancel binding).
        void                NotifyFrameUpdated();

        // Waits until loading is completed, used by GFxFontLib.
        void                WaitForLoadFinish();
        
        // Waits until a specified frame is loaded
        void                WaitForFrame(UInt frame);

        // *** Init / Access IO related data

        UInt                GetMetadata(char *pbuff, UInt buffSize) const;
        void                SetMetadata(UByte *pdata, UInt size); 
        void                SetFileAttributes(UInt attrs)   { FileAttributes = attrs; }

        // Allocate MovieData local memory.
        inline void*        AllocTagMemory(size_t bytes)    { return TagMemAllocator.Alloc(bytes);  }
        // Allocate a tag directly through method above.
        template<class T>
        inline T*           AllocMovieDefClass()            { return GTL::gconstruct<T>(AllocTagMemory(sizeof(T))); }

        GFxResourceId       GetNextGradientId()             { return GradientIdGenerator.GenerateNextId(); }


        // Creates a new resource handle with a binding index for the resourceId; used 
        // to register ResourceData objects that need to be bound later.
        GFxResourceHandle   AddNewResourceHandle(GFxResourceId rid);
        // Add a resource during loading.
        void                AddResource(GFxResourceId rid, GFxResource* pres);   
        // A character is different from other resources because it tracks its creation Id.
        void                AddCharacter(GFxResourceId rid, GFxCharacterDef* c)
        {
            c->SetId(rid);
            AddResource(rid, c);
        }

        // Expose one of our resources under the given symbol, for export.  Other movies can import it.        
        void                ExportResource(const GFxString& symbol, GFxResourceId rid, const GFxResourceHandle &hres);

        bool                GetResourceHandle(GFxResourceHandle* phandle, GFxResourceId rid) const;

        GFxFontData*        GetFontData(GFxResourceId rid);

        // Gets binding data for the first frame, if any.
        GFxFrameBindData*    GetFirstFrameBindData() const   { return BindData.pFrameData; }
        GFxFontDataUseNode* GetFirstFont() const            { return BindData.pFonts; }
        GFxImportData*      GetFirstImport() const          { return BindData.pImports; }
        
        
        // Labels the frame currently being loaded with the given name.
        // A copy of the name string is made and kept in this object.    
        virtual void        AddFrameName(const char* name, GFxLog *plog);
        virtual void        SetLoadingPlaylistFrame(const Frame& frame);
        virtual void        SetLoadingInitActionFrame(const Frame& frame);  

        bool                GetLabeledFrame(const char* label, UInt* frameNumber, bool translateNumbers = 1);

        // *** Playlist access
        const Frame         GetPlaylist(int frameNumber) const;
        bool                GetInitActions(Frame* pframe, int frameNumber) const;
        UInt                GetInitActionListSize() const;
        inline bool         HasInitActions() const   { return InitActionsCnt > 0; }

      
        // Locker class used for LoadTaskData::Resources, Exports and InvExports.
        struct ResourceLocker
        {
            const LoadTaskData *pLoadData;

            ResourceLocker(const LoadTaskData* ploadData)
            {
                pLoadData = 0;
                if (ploadData->LoadState < LS_LoadFinished)
                {
                    pLoadData = ploadData;
                    pLoadData->ResourceLock.Lock();
                }            
            }
            ~ResourceLocker()
            {
                if (pLoadData)
                    pLoadData->ResourceLock.Unlock();
            }
        };
    };
   


    // Data for all of our content.
    GPtr<LoadTaskData>  pData;



    // *** Initialiation / Cleanup

    GFxMovieDataDef(const GFxResourceKey &creationKey,
                    MovieDataType mtype, const char *purl);
    ~GFxMovieDataDef();


    // Create an empty MovieDef with no content; Used for LoadVars into a new level.
    void    InitEmptyMovieDef()
    {
        pData->InitEmptyMovieDef();
    }

    // Create a MovieDef describing an image file.
    void    InitImageFileMovieDef(UInt fileLength,
                                  GFxImageResource *pimageResource, bool bilinear = 1)
    {
        pData->InitImageFileMovieDef(fileLength, pimageResource, bilinear);
    }

    // Waits until loading is completed, used by GFxFontLib.
    void                WaitForLoadFinish() const   { pData->WaitForLoadFinish(); }

    // Waits until a specified frame is loaded
    void                WaitForFrame(UInt frame) const { pData->WaitForFrame(frame); }

    
    // *** Information query methods.

    // All of the accessor methods just delegate to LoadTaskData; there
    // are no loading methods exposed here since loading happens only through GFxLoadProcess.    
    Float               GetFrameRate() const        { return pData->Header.FPS; }
    const GRectF&       GetFrameRectInTwips() const { return pData->Header.FrameRect; }
    GRectF              GetFrameRect() const        { return TwipsToPixels(pData->Header.FrameRect); }
    Float               GetWidth() const            { return ceilf(TwipsToPixels(pData->Header.FrameRect.Width())); }
    Float               GetHeight() const           { return ceilf(TwipsToPixels(pData->Header.FrameRect.Height())); }
    virtual UInt        GetVersion() const          { return pData->GetVersion(); }
    virtual UInt        GetLoadingFrame() const     { return pData->LoadingFrame; }
    virtual UInt        GetSWFFlags() const         { return pData->Header.SWFFlags; }

    MovieLoadState      GetLoadState() const        { return pData->LoadState; }

    UInt32              GetFileBytes() const        { return pData->Header.FileLength; }
    UInt                GetTagCount() const         { return pData->TagCount;  }
    const char*         GetFileURL() const          { return pData->GetFileURL(); }
    UInt                GetFrameCount() const       { return pData->Header.FrameCount; }
    
    const GFxExporterInfo*  GetExporterInfo() const { return pData->GetExporterInfo(); }    
    void                GetMovieInfo(GFxMovieInfo *pinfo) const { pData->Header.GetMovieInfo(pinfo); }
    
    // Sets MetaData of desired size.
    UInt                GetFileAttributes() const   { return pData->FileAttributes; }
    UInt                GetMetadata(char *pbuff, UInt buffSize) const { return pData->GetMetadata(pbuff, buffSize); }
           

    // Fills in 0-based frame number.
    bool                GetLabeledFrame(const char* label, UInt* frameNumber, bool translateNumbers = 1)
    {
        return pData->GetLabeledFrame(label, frameNumber, translateNumbers);
    }
    
    // Get font data by ResourceId.   
    bool                GetResourceHandle(GFxResourceHandle* phandle, GFxResourceId rid) const
    {
        return pData->GetResourceHandle(phandle, rid);
    }

    GFxFontDataUseNode* GetFirstFont() const     { return pData->GetFirstFont(); }
    GFxImportData*      GetFirstImport() const   { return pData->GetFirstImport(); }


    // Helper used to look up labeled frames and/or translate frame numbers from a string.
    static bool         TranslateFrameString(
                            const  GFxStringHash<UInt>  &namedFrames,
                            const char* label, UInt* frameNumber, bool translateNumbers);
    
         
    // *** GFxTimelineDef implementation.

    // Frame access from GFxTimelineDef.
    virtual const Frame GetPlaylist(int frame) const        { return pData->GetPlaylist(frame); }
    UInt                GetInitActionListSize() const       { return pData->GetInitActionListSize(); }
    inline bool         HasInitActions() const              { return pData->HasInitActions(); }
    virtual bool        GetInitActions(Frame* pframe, int frame) const { return pData->GetInitActions(pframe, frame); }


    // *** Creating MovieDefData file keys

    // Create a key for an SWF file corresponding to GFxMovieDef.
    // Note that GFxImageCreator is only used as a key if is not null.
    static  GFxResourceKey  CreateMovieFileKey(const char* pfilename,
                                               SInt64 modifyTime,
                                               GFxFileOpener* pfileOpener,
                                               GFxImageCreator* pimageCreator,
                                               GFxPreprocessParams* ppreprocessParams);

    // *** GFxResource implementation
    
    virtual GFxResourceKey  GetKey()                        { return ResourceKey; }
    virtual UInt            GetResourceTypeCode() const     { return MakeTypeCode(RT_MovieDataDef); }

};


// State values that cause generation of a different binding
class  GFxMovieDefBindStates : public GRefCountBase<GFxMovieDefBindStates>
{
public:
    // NOTE: We no longer store pDataDef here, instead it is now a part of
    // a separate GFxMovieDefImplKey object. Such separation is necessary
    // to allow release of GFxMovieDefImpl to cancel the loading process, which
    // is thus not allowed to AddRef to pDataDef. 

    // GFxStates that would cause generation of a new DefImpl binding.
    // These are required for the following reasons:
    //
    //  Class               Binding Change Cause
    //  ------------------  ----------------------------------------------------
    //  GFxFileOpener       Substitute GFile can produce different data.
    //  GFxURLBuilder   Can cause different import file substitutions.
    //  GFxImageCreator     Different image representation = different binding.
    //  GFxImportVisitor     (?) Import visitors can substitute import files/data.
    //  GFxImageVisitor      (?) Image visitors  can substitute image files/data.
    //  GFxGradientParams   Different size of gradient = different binding.
    //  GFxFontParams       Different font texture sizes = different binding.


    GPtr<GFxFileOpener>         pFileOpener;
    GPtr<GFxURLBuilder>         pURLBulider;
    GPtr<GFxImageCreator>       pImageCreator;       
    GPtr<GFxImportVisitor>      pImportVisitor;
  //  GPtr<GFxImageVisitor>       pImageVisitor;
    GPtr<GFxGradientParams>     pGradientParams;
    GPtr<GFxFontPackParams>     pFontPackParams;
    GPtr<GFxFontProvider>       pFontProvider;
    GPtr<GFxFontLib>            pFontLib;
    GPtr<GFxFontMap>            pFontMap;
    GPtr<GFxPreprocessParams>   pPreprocessParams;

    GFxMovieDefBindStates(GFxSharedState* psharedState)
    {
        SetRefCountMode(GFC_REFCOUNT_THREADSAFE);
        // Get multiple states at once to avoid extra locking.
        GFxState*                        pstates[10]    = {0,0,0,0,0,0,0,0,0,0};
        const static GFxState::StateType stateQuery[10] =
          { GFxState::State_FileOpener,     GFxState::State_URLBuilder,
            GFxState::State_ImageCreator,   GFxState::State_ImportVisitor,
            GFxState::State_GradientParams, GFxState::State_FontPackParams,
            GFxState::State_FontProvider,   GFxState::State_FontLib,
            GFxState::State_FontMap,        GFxState::State_PreprocessParams };

        // Get states and assign them locally.
        psharedState->GetStatesAddRef(pstates, stateQuery, 10);
        pFileOpener         = *(GFxFileOpener*)         pstates[0];
        pURLBulider         = *(GFxURLBuilder*)         pstates[1];
        pImageCreator       = *(GFxImageCreator*)       pstates[2];
        pImportVisitor      = *(GFxImportVisitor*)      pstates[3];
        pGradientParams     = *(GFxGradientParams*)     pstates[4];
        pFontPackParams     = *(GFxFontPackParams*)     pstates[5];
        pFontProvider       = *(GFxFontProvider*)       pstates[6];
        pFontLib            = *(GFxFontLib*)            pstates[7];
        pFontMap            = *(GFxFontMap*)            pstates[8];
        pPreprocessParams   = *(GFxPreprocessParams*)   pstates[9];
    }

    GFxMovieDefBindStates(GFxMovieDefBindStates *pother)
    {
        SetRefCountMode(GFC_REFCOUNT_THREADSAFE);
        pFileOpener         = pother->pFileOpener;
        pURLBulider         = pother->pURLBulider;
        pImageCreator       = pother->pImageCreator;
        pImportVisitor      = pother->pImportVisitor;
      //  pImageVisitor       = pother->pImageVisitor;
        pFontPackParams     = pother->pFontPackParams;
        pGradientParams     = pother->pGradientParams;
        pFontProvider       = pother->pFontProvider;
        pFontLib            = pother->pFontLib;
        pFontMap            = pother->pFontMap;
        pPreprocessParams   = pother->pPreprocessParams;
        // Leave pDataDef uninitialized since this is used
        // from GFxLoadStates::CloneForImport, and imports
        // have their own DataDefs.
    }

    // Functionality necessary for this GFxMovieDefBindStates to be used
    // as a key object for GFxMovieDefImpl.
    size_t  GetHashCode() const;
    bool operator == (GFxMovieDefBindStates& other) const;  
    bool operator != (GFxMovieDefBindStates& other) const { return !operator == (other); }    
};


// ***** Loading state collection


// Load states are used for both loading and binding, they 
// are collected atomically at the beginning of CreateMovie call,
// and apply for the duration of load/bind time.

class GFxLoadStates : public GRefCountBase<GFxLoadStates>
{
public:

    // *** These states are captured from Loader in constructor.

    // States used for binding of GFxMovieDefImpl.
    GPtr<GFxMovieDefBindStates> pBindStates;

    // Other states we might need
    // Load-affecting GFxState(s).
    GPtr<GFxLog>                pLog;     
    GPtr<GFxParseControl>       pParseControl;
    GPtr<GFxProgressHandler>    pProgressHandler;
    GPtr<GFxTaskManager>        pTaskManager;
    // Store cache manager so that we can issue font params warning if
    // it is not available.
    GPtr<GFxFontCacheManager>   pFontCacheManager;
    // Store render config so that it can be passed for image creation.
    // Note that this state is non-binding.
    GPtr<GFxRenderConfig>       pRenderConfig;

    // Weak States
    GPtr<GFxResourceWeakLib>    pWeakResourceLib;    


    // *** Loader

    // Loader back-pointer (contents may change).
    // We do NOT use this to access states because that may result 
    // in inconsistent state use when loading is threaded.    
    GPtr<GFxLoaderImpl>         pLoaderImpl;

    // Cached relativePath taken from DataDef URL, stored
    // so that it can be easily passed for imports.
    GFxString                 RelativePath;

    // set to true if multi-threaded loading is using
    bool ThreadedLoading; 

    // *** Substitute fonts

    // These are 'substitute' fonts provider files that need to be considered when
    // binding creating a GFxFontResource from ResourceData. Substitute fonts come from
    // files that have '_glyphs' in filename and replace fonts with matching names
    // in the file being bound.
    // Technically, this array should be a part of 'GFxBindingProcess'. If more
    // of such types are created, we may need to institute one.
    GTL::garray<GPtr<GFxMovieDefImpl> > SubstituteFontMovieDefs;


    GFxLoadStates();
    ~GFxLoadStates();

    // Creates GFxLoadStates by capturing states from pstates. If pstates
    // is null, all states come from loader. If binds states are null,
    // bind states come from pstates.
    GFxLoadStates(GFxLoaderImpl* ploader, GFxSharedState* pstates = 0,
        GFxMovieDefBindStates *pbindStates = 0);

    // Helper that clones load states, pBindSates.
    // The only thing left un-copied is GFxMovieDefBindStates::pDataDef
    GFxLoadStates*          CloneForImport() const;


    GFxResourceWeakLib*     GetLib() const              { return pWeakResourceLib.GetPtr();  }
    GFxMovieDefBindStates*  GetBindStates() const       { return pBindStates.GetPtr(); }
    GFxLog*                 GetLog() const              { return pLog; }
    GFxTaskManager*         GetTaskManager() const      { return pTaskManager; }
    GFxRenderConfig*        GetRenderConfig() const     { return pRenderConfig; }
    GFxFontCacheManager*    GetFontCacheManager() const { return pFontCacheManager; }
    GFxProgressHandler*     GetProgressHandler() const  { return pProgressHandler; }

    GFxFileOpener*          GetFileOpener() const       { return pBindStates->pFileOpener;  }
    GFxFontPackParams*      GetFontPackParams() const   { return pBindStates->pFontPackParams; }
    GFxPreprocessParams*    GetPreprocessParams() const { return pBindStates->pPreprocessParams; }

    // Initializes the relative path.
    void                    SetRelativePathForDataDef(GFxMovieDataDef* pdef);
    const GFxString&        GetRelativePath() const   { return RelativePath;  }


    // Obtains an image creator, but only if image data is not supposed to
    // be preserved; considers GFxLoader::LoadKeepBindData flag from arguments.
    GFxImageCreator*        GetLoadTimeImageCreator(UInt loadConstants) const; 


    // Delegated state access helpers. If loader flag LoadQuietOpen is set in loadConstants
    // and requested file can not be opened all error messages will be suppressed.
    GFile*      OpenFile(const char *pfilename, UInt loadConstants = GFxLoader::LoadAll);

    // Perform filename translation and/or copy by relying on translator.
    void        BuildURL(GFxString *pdest, const GFxURLBuilder::LocationInfo& loc) const;

    // Submit a background task. Returns false if TaskManager is not set or it couldn't 
    // add the task
    bool        SubmitBackgroundTask(GFxLoaderTask* ptask);

    bool IsThreadedLoading() const { return ThreadedLoading || pTaskManager; }
};



// ***  GFxCharacterCreateInfo

// Simple structure containing information necessary to create a character.
// This is returned bu GFxMovieDefImpl::GetCharacterCreateInfo().

// Since characters can come from an imported file, we also contain a 
// GFxMovieDefImpl that should be used for a context of any characters
// created from this character def.
struct GFxCharacterCreateInfo
{
    GFxCharacterDef* pCharDef;
    GFxMovieDefImpl* pBindDefImpl;
};



// ***** GFxMovieDefImpl

// This class holds the immutable definition of a GFxMovieSub's
// contents.  It cannot be played directly, and does not hold
// current state; for that you need to call CreateInstance()
// to get a MovieInstance.


class GFxMovieDefImpl : public GFxMovieDef
{
public:

    // Shared state, for loading, images, logs, etc.
    // Note that we actually own our own copy here.
    GPtr<GFxSharedStateImpl>    pSharedState;

    // We store the loader here; however, we do not take our
    // states from it -> instead, the states come from pSharedState.
    // The loader is here to give access to pWeakLib.
    // TBD: Do we actually need Loader then? Or just Weak Lib?
    // May need task manger too...
    // NOTE: This instead could be stored in MovieRoot. What's a more logical place?    
    GPtr<GFxLoaderImpl>         pLoaderImpl;


    // States form OUR Binding Key.

    // Bind States contain the DataDef.    
    // It could be 'const GFxMovieDefBindStates*' but we can't do that with GPtr.
    GPtr<GFxMovieDefBindStates> pBindStates;



    // *** BindTaskData

    // Most of the GFxMovieDefImpl data is contained in BindTaskData class,
    // which is AddRef-ed separately. Such setup is necessary to allow
    // the loading thread to be canceled when all user reference count
    // for GFxMovieDefImpl are released. The loading thread only references
    // the LoadData object and thus can be notified when GFxMovieDefImpl dies.

    // BindStateType is used by BindTaskData::BindState.
    enum BindStateType
    {
        BS_NotStarted       = 0,
        BS_InProgress       = 1,
        BS_Finished         = 2,
        BS_Canceled         = 3, // Canceled due to a user request.
        BS_Error            = 4,
        // Mask for above states.
        BS_StateMask        = 0xF,

        // These bits store the status of what was
        // actually loaded; we can wait based on them.
        BSF_Frame1Loaded    = 0x100,
        BSF_LastFrameLoaded = 0x200,
    };


    class BindTaskData : public GRefCountBase<BindTaskData>
    {
        friend class GFxMovieDefImpl;
        friend class GFxMovieBindProcess;

        // AddRef to DataDef because as long as binding process
        // is alive we must exist.
        GPtr<GFxMovieDataDef>       pDataDef;

        // A shadow pointer to pDefImpl that can 'turn' bad unless
        // accessed through a lock in GetDefImplAddRef(). Can be null
        // if GFxMovieDefImpl has been deleted by another thread.
        GFxMovieDefImpl*            pDefImpl_Unsafe;

        // Save load flags so that they can be propagated
        // into the nested LoadMovie calls.
        UInt                        LoadFlags;
              

        // Binding table for handles in pMovieDataDef.
        GFxResourceBinding          ResourceBinding;

        // Movies we import from; hold a ref on these, to keep them alive.
        // This array directly corresponds to GFxMovieDataDef::ImportData.
        GTL::garray<GPtr<GFxMovieDefImpl> >    ImportSourceMovies;
        // Lock for accessing above.
        //  - We also use this lock to protect pDefImpl
        GLock                                  ImportSourceLock;

        // Other imports such as through FontLib. These do not need
        // to be locked because it is never accessed outside binding
        // thread and destructor.
        GTL::garray<GPtr<GFxMovieDefImpl> >    ResourceImports;


        // *** Binding Progress State

        // Binding state variables are modified as binding progresses; there is only
        // one writer - the binding thread. The binding variables modified progressively
        // are BindState, BindingFrame and BytesLoaded. BindingFrame and BytesLoaded
        // are polled in the beginning of the next frame Advance, thus no extra sync
        // is necessary for them. BindState; however, can be waited on with BindStateMutex.

        // Current binding state modified after locking BindStateMutex.
        volatile UInt       BindState;

#ifndef GFC_NO_THREADSUPPORT
        // This mutex/WC pair is used to wait on the BindState, modified when a
        // critical event such as binding completion, frame 1 bound or error
        // take place. We have to wait on these when/if GFxLoader::LoadWaitFrame1
        // or LoadWaitCompletion load flags are passed.
        GMutex              BindStateMutex;
        GWaitCondition      BindStateUpdated;
#endif

        // Bound Frame - the frame we are binding now. This frame corresponds
        // to the Loading Frame, as only bound frames (with index < BindingFrame)
        // can actually be used.
        volatile UInt       BindingFrame;

        // Bound amount of bytes loaded for the Binding frame,
        // must be assigned before BindingFrame.
        volatile UInt32     BytesLoaded;

        // This flag is set to 'true' if binding is being canceled;
        // this flag will be set if the binding process should terminate.
        // This value is different from BinsState == BS_Canceled
        volatile bool       BindingCanceled;


    public:
        
        // Technically BindTaskData should not store a pointer to GFxMovieDefImpl,
        // however, we just pass it to initialize regular ptr in GFxResourceBinding.
        BindTaskData(GFxMovieDataDef *pdataDef,
                     GFxMovieDefImpl *pdefImpl,
                     UInt loadFlags, bool fullyLoaded);
        ~BindTaskData();

        // Notifies BindData that MovieDefImpl is being destroyed. This may be a premature
        // destruction if we are in the process of loading (in that case it will lead to
        // loading being canceled).
        void                OnMovieDefRelease();


        GFxMovieDataDef*    GetDataDef() const { return pDataDef; }

        // Grabs OUR GFxMovieDefImpl through a lock; can return null.
        GFxMovieDefImpl*    GetMovieDefImplAddRef();

        
        // Bind state accessors; calling SetBindState notifies BindStateUpdated.
        void                SetBindState(UInt newState);
        UInt                GetBindState() const { return BindState; }    
        BindStateType       GetBindStateType() const { return (BindStateType) (BindState & BS_StateMask); }
        UInt                GetBindStateFlags() const { return BindState & ~BS_StateMask; }

        // Wait for for bind state flag or error. Return true for success,
        // false if bind state was changed to error without setting the flags.
        bool                WaitForBindStateFlags(UInt flags);

        // Query progress.
        UInt                GetLoadingFrame() const     { return GAtomicOps<UInt>::Load_Acquire(&BindingFrame); }
        UInt32              GetBytesLoaded() const      { return BytesLoaded; }
        
        // Updates binding state Frame and BytesLoaded (called from image loading task).
        void                UpdateBindingFrame(UInt frame, UInt32 bytesLoaded);


        // Access import source movie based on import index (uses a lock).
        GFxMovieDefImpl*    GetImportSourceMovie(UInt importIndex);
        // Adds a movie reference to ResourceImports array.
        void                AddResourceImportMovie(GFxMovieDefImpl *pdefImpl);


        // *** Import binding support.

        // After GFxMovieDefImpl constructor, the most important part of GFxMovieDefImpl 
        // is initialization binding, i.e. resolving all of dependencies based on the binding states.
        // This is a step where imports and images are loaded, gradient images are
        // generated and fonts are pre-processed.
        // Binding is done by calls to GFxMovieBindProcess::BindNextFrame.

        // Resolves and import during binding.
        void                    ResolveImport(GFxImportData* pimport, GFxMovieDefImpl* pdefImpl,
                                              GFxLoadStates* pls, bool recursive);

        // Resolves an import of 'gfxfontlib.swf' through the GFxFontLib object.
        // Returns 1 if ALL mappings succeeded, otherwise 0.
        bool                    ResolveImportThroughFontLib(GFxImportData* pimport,
                                                            GFxLoadStates* pls,
                                                            GFxMovieDefImpl* pourDefImpl);

        // Internal helper for import updates.
        bool                    SetResourceBindData(GFxResourceId rid, GFxResourceBindData& bindData,
                                                    const char* pimportSymbolName);
    };

    GPtr<BindTaskData>  pBindData;


    
    // *** Constructor / Destructor

    GFxMovieDefImpl(GFxMovieDataDef* pdataDef,
                    GFxMovieDefBindStates* pstates,
                    GFxLoaderImpl* ploaderImpl,
                    UInt loadConstantFlags,
                    GFxSharedStateImpl *pdelegateState = 0,
                    bool fullyLoaded = 0);
    ~GFxMovieDefImpl();


    // Create a movie instance.
    GFxMovieView*           CreateInstance(bool initFirstFrame = 0);


    // *** Creating MovieDefImpl keys

    // GFxMovieDefImpl key depends (1) pMovieDefData, plus (2) all of the states
    // used for its resource bindings, such as file opener, file translator, image creator,
    // visitors, etc. A snapshot of these states is stored in GFxMovieDefBindStates.
    // Movies that share the same bind states are shared through GFxResourceLib.

    // Create a key for an SWF file corresponding to GFxMovieDef.
    static  GFxResourceKey  CreateMovieKey(GFxMovieDataDef *pdataDef,
                                           GFxMovieDefBindStates* pbindStates);
    
    
    // *** Property access

    GFxMovieDataDef*    GetDataDef() const          { return pBindData->GetDataDef(); }

    // ...
    UInt                GetFrameCount() const       { return GetDataDef()->GetFrameCount(); }
    Float               GetFrameRate() const        { return GetDataDef()->GetFrameRate(); }
    GRectF              GetFrameRect() const        { return GetDataDef()->GetFrameRect(); }
    Float               GetWidth() const            { return GetDataDef()->GetWidth(); }
    Float               GetHeight() const           { return GetDataDef()->GetHeight(); }
    virtual UInt        GetVersion() const          { return GetDataDef()->GetVersion(); }
    virtual UInt        GetSWFFlags() const         { return GetDataDef()->GetSWFFlags(); }    
    virtual const char* GetFileURL() const          { return GetDataDef()->GetFileURL(); }

    UInt32              GetFileBytes() const        { return GetDataDef()->GetFileBytes(); }
    virtual UInt        GetLoadingFrame() const     { return pBindData->GetLoadingFrame(); }
    UInt32              GetBytesLoaded() const      { return pBindData->GetBytesLoaded(); }
    GFxMovieDataDef::MovieLoadState      GetLoadState() const      { return GetDataDef()->GetLoadState(); }
    UInt                GetTagCount() const         { return GetDataDef()->GetTagCount();  }
    
    inline UInt         GetLoadFlags() const        { return pBindData->LoadFlags; }

    void                WaitForLoadFinish() const   { GetDataDef()->WaitForLoadFinish(); }
    void                WaitForFrame(UInt frame) const { GetDataDef()->WaitForFrame(frame); }

    // Stripper info query.
    virtual const GFxExporterInfo*  GetExporterInfo() const  { return GetDataDef()->GetExporterInfo(); }

    GRectF                      GetFrameRectInTwips() const { return GetDataDef()->GetFrameRectInTwips(); }
    GFxResourceWeakLib*         GetWeakLib() const          { return pLoaderImpl->GetWeakLib(); }

    // Shared state implementation.
    virtual GFxSharedState*     GetSharedImpl() const       { return pSharedState.GetPtr(); }    

    // Overrides for users
    virtual UInt                GetMetadata(char *pbuff, UInt buffSize) const
        { return GetDataDef()->GetMetadata(pbuff, buffSize); }
    virtual UInt                GetFileAttributes() const { return GetDataDef()->GetFileAttributes(); }


    // GFxLog Error delegation
    void    LogError(const char* pfmt, ...)
    { 
        va_list argList; va_start(argList, pfmt);
        GFxLog *plog = GetLog();
        if (plog) plog->LogMessageVarg(GFxLog::Log_Error, pfmt, argList);
        va_end(argList); 
    }


    // Wait for for bind state flag or error. Return true for success,
    // false if bind state was changed to error without setting the flags.
    bool                        WaitForBindStateFlags(UInt flags) { return pBindData->WaitForBindStateFlags(flags); }
       

    // *** Resource Lookup

    // Obtains a resource based on its id. If resource is not yet resolved,
    // NULL is returned. Should be used only before creating an instance.
    // Type checks the resource based on specified type.
   // GFxResource*                GetResource(GFxResourceId rid, GFxResource::ResourceType rtype);
    // Obtains full character creation information, including GFxCharacterDef.
    GFxCharacterCreateInfo      GetCharacterCreateInfo(GFxResourceId rid);

    // Get a binding table reference.
    const GFxResourceBinding&   GetResourceBinding() const { return pBindData->ResourceBinding; }
    GFxResourceBinding&         GetResourceBinding()       { return pBindData->ResourceBinding; }
 
   
    // *** GFxMovieDef implementation

    virtual void                VisitImportedMovies(ImportVisitor* visitor);           
    virtual void                VisitResources(ResourceVisitor* pvisitor, UInt visitMask = ResVisit_AllImages);
    virtual GFxResource*        GetResource(const char *pexportName) const;

    // Locate a font resource by name and style.
    // It's ok to return GFxFontResource* without the binding because pBinding
    // is embedded into font resource allowing imports to be handled properly.
    virtual GFxFontResource*    GetFontResource(const char* pfontName, UInt styleFlags);

    // Fill in the binding resource information together with its binding.
    // Return 0 if Get failed and no bind data was returned.
    bool                        GetExportedResource(GFxResourceBindData *pdata, const GFxString& symbol);   
    const GFxString*            GetNameOfExportedResource(GFxResourceId rid) const;


    // *** GFxResource implementation

    virtual GFxResourceKey  GetKey()                        { return CreateMovieKey(GetDataDef(), pBindStates); }
    virtual UInt            GetResourceTypeCode() const     { return MakeTypeCode(RT_MovieDef); }
};




// *** GFxMovieDefBindProcess

// GFxMovieDefBindProcess stores the states necessary for binding. The actual
// binding is implemented by calling BindFrame for every frame that needs to be bound.
//
// Binding is separated into a separate object so that it can be reused independent
// of whether it takes place in a separate thread (which just calls BindFrame until
// its done) or is interleaved together with the loading process, which can call
// it after each frame.

class GFxMovieBindProcess : public GFxLoaderTask
{
    typedef GFxMovieDefImpl::BindTaskData BindTaskData;


    // This is either current (if in BindNextFrame) or previous frame bind data.
    GFxFrameBindData*    pFrameBindData;

    // Perform binding of resources.
    GFxResourceId       GlyphTextureIdGen;
  
    // We keep a smart pointer to GFxMovieDefImpl::BindTaskData and not
    // the GFxMovieDefImpl itself; this allows us to cancel loading
    // gracefully if user's GFxMovieDef is released.
    GPtr<BindTaskData>  pBindData;

    // Ok to store weak ptr since BindTaskData AddRefs to DataDef. 
    GFxMovieDataDef*    pDataDef;   

    bool                Stripped;

    // We need to keep load import stack so we can detect recursion.
    GFxLoaderImpl::LoadStackItem* pLoadStack;

public:
   
    GFxMovieBindProcess(GFxLoadStates *pls,
                        GFxMovieDefImpl* pdefImpl,
                        GFxLoaderImpl::LoadStackItem* ploadStack = NULL);

    ~GFxMovieBindProcess();


    typedef GFxMovieDefImpl::BindStateType BindStateType;

    // Bind a next frame.
    // If binding failed, then BS_Error will be returned.
    BindStateType       BindNextFrame();


    // BindState delegates to GFxMovieDefImpl.
    void                SetBindState(UInt newState) { if (pBindData) pBindData->SetBindState(newState); }
    UInt                GetBindState() const        { return pBindData->GetBindState(); }
    BindStateType       GetBindStateType() const    { return pBindData->GetBindStateType(); }
    UInt                GetBindStateFlags() const   { return pBindData->GetBindStateFlags(); }

    // Gets binding data for the next frame, if any.
    GFxFrameBindData*    GetNextFrameBindData()
    {
        if (pFrameBindData)
            return pFrameBindData->pNextFrame;
        return pDataDef->pData->GetFirstFrameBindData();
    }

    void                FinishBinding();


    // *** GFxTask implementation
    
    virtual void    Execute()
    {
        // Do the binding.
        while(BindNextFrame() == GFxMovieDefImpl::BS_InProgress)
        { }     
    }

    virtual void    OnAbandon(bool started) 
    { 
        if (pBindData)
        {
            if (started)
                pBindData->BindingCanceled = true;
            else
                SetBindState(GFxMovieDefImpl::BS_Canceled); 
        }
    }
};





//
// GFxSwfEvent
//
// For embedding event handlers in GFxPlaceObject2

class GFxSwfEvent : public GNewOverrideBase
{
public:
    // NOTE: DO NOT USE THESE AS VALUE TYPES IN AN
    // GTL::garray<>!  They cannot be moved!  The private
    // operator=(const GFxSwfEvent&) should help guard
    // against that.

    GFxEventId                  Event;
    GPtr<GASActionBufferData>   pActionOpData;    

    GFxSwfEvent()
    { }
        
    void    Read(GFxStream* in, UInt32 flags);

    void    AttachTo(GFxASCharacter* ch) const;
    
private:
    // DON'T USE THESE
    GFxSwfEvent(const GFxSwfEvent& ) { GASSERT(0); }
    void    operator=(const GFxSwfEvent& ) { GASSERT(0); }
};


// ***** Execute Tags

class GFxActionPriority
{
public:
    enum Priority
    {
        AP_Highest      = 0, // initclips for imported source movies
        AP_Initialize      , // onClipEvent(initialize)
        AP_InitClip        , // local initclips
        AP_Construct       , // onClipEvent(construct)/ctor
        //AP_EnterFrame     , // onClipEvent(enterFrame)/onEnterFrame
        //AP_Unload          , // onClipEvent(unload)/onUnload
        AP_Frame           , // frame code
        AP_Normal = AP_Frame,
        AP_ClipEventLoad   , // onClipEvent(load)/onLoad pair
        AP_Load            , // onLoad-only

        AP_Count
    };
};



// Execute tags include things that control the operation of
// the GFxSprite.  Essentially, these are the events associated with a frame.
class GASExecuteTag //: public GNewOverrideBase
{
public:

#ifdef GFC_BUILD_DEBUG    
    GASExecuteTag();
    virtual ~GASExecuteTag();
#else
    virtual ~GASExecuteTag() {}
#endif    
    
    virtual void    Execute(GFxSprite* m)                           { GUNUSED(m); }
    virtual void    ExecuteWithPriority(GFxSprite* m, GFxActionPriority::Priority prio) { GUNUSED(prio);  Execute(m); }
    virtual void    ExecuteState(GFxSprite* m)                      { GUNUSED(m); }
    virtual void    ExecuteStateReverse(GFxSprite* m, int frame)    { GUNUSED(frame); ExecuteState(m); }

    virtual bool    IsRemoveTag() const { return false; }
    virtual bool    IsActionTag() const { return false; }
    virtual bool    IsInitImportActionsTag() const { return false;  }


    // A combination of ResourceId and depth - used to identify a tag created
    // character in a display list.
    struct DepthResId
    {
        SInt                Depth;
        GFxResourceId       Id;

        DepthResId() { Depth = 0; }
        DepthResId(GFxResourceId rid, SInt depth) : Depth(depth), Id(rid) { }
        DepthResId(const DepthResId& src) : Depth(src.Depth), Id(src.Id) { }
        inline DepthResId& operator = (const DepthResId& src) { Id = src.Id; Depth = src.Depth; return *this; }
        // Comparison - used during searches.
        inline bool operator == (const DepthResId& other) const { return (Id == other.Id) && (Depth == other.Depth); }
        inline bool operator != (const DepthResId& other) const { return !operator == (other); }
    };

    // Default DepthResId corresponds to an invalid character.
    // Returns depth id for PlaceObject2 tag only. If return value != DepthResId(GFxResourceId(), -1),
    // this can be cast into GFxPlaceObject2 (PlaceObject2 always has valid depth).
    virtual DepthResId  GetPlaceObject2_DepthId() const { return DepthResId(GFxResourceId(), -1); }
};



// A data structure that describes common positioning state.
class GFxCharPosInfo
{
public:
    GRenderer::Cxform   ColorTransform;
    GRenderer::Matrix   Matrix_1;
    GRenderer::BlendType BlendMode;
    Float               Ratio;  
    SInt                Depth;
    GFxResourceId       CharacterId;
    UInt16              ClipDepth;
    bool                HasMatrix;
    bool                HasCxform;
    GPtr<GFxTextFilter> TextFilter;

    GFxCharPosInfo()
    {
        Ratio       = 0.0f;
        HasMatrix   = 0;
        HasCxform   = 0;
        Depth       = 0;        
        ClipDepth   = 0;
        BlendMode   = GRenderer::Blend_None;
    }

    GFxCharPosInfo(GFxResourceId chId, SInt depth,
        bool hasCxform, const GRenderer::Cxform &cxform,
        bool hasMatrix, const GRenderer::Matrix &matrix,
        Float ratio = 0.0f, UInt16 clipDepth = 0,
        GRenderer::BlendType blend = GRenderer::Blend_None)
        : ColorTransform(cxform), Matrix_1(matrix), CharacterId(chId)
    {
        Ratio       = ratio;
        HasMatrix   = hasMatrix;
        HasCxform   = hasCxform;
        Depth       = depth;
        CharacterId = chId;
        ClipDepth   = clipDepth;
        BlendMode   = blend;
        TextFilter  = 0;
    }
};


// GFxPlaceObject2
class GFxPlaceObject2 : public GASExecuteTag
{
public: 
    enum PlaceActionType
    {
        Place_Add,
        Place_Move,
        Place_Replace,
    };

    GFxTagType          TagType;
    char*               Name;
    // Matrices, depth and character index.
    GFxCharPosInfo      Pos;
    PlaceActionType     PlaceType;

    GTL::garray<GFxSwfEvent*>   EventHandlers;

    // Constructors
    GFxPlaceObject2();
    ~GFxPlaceObject2();


    // *** GASExecuteTag implementation

    void    Read(GFxLoadProcess* p, GFxTagType tagType);
    void    Read(GFxStream* pin, GFxTagType tagType, UInt movieVersion);

    // Initialize to a specified value (used when generating MovieDataDef for images).
    void    InitializeToAdd(const GFxCharPosInfo& posInfo);

    // Execute with custom Pos structure.
    void    ExecuteWithPos(const GFxCharPosInfo& pos, GFxSprite* m, UInt createFrame = GFC_MAX_UINT);

    // Place/move/whatever our object in the given pMovie.
    void    Execute(GFxSprite* m)           { ExecuteWithPos(Pos, m);}
    void    ExecuteState(GFxSprite* m)      { Execute(m); }
    void    ExecuteStateReverse(GFxSprite* m, int frame);       

    // DepthResId is packed id and depth together (it identifies
    // a character operated on by this PlaceObject2 tag).
    virtual DepthResId  GetPlaceObject2_DepthId() const;
};

        
class GFxRemoveObject2 : public GASExecuteTag
{
public:
    int           Depth;
    GFxResourceId Id;

    GFxRemoveObject2()
    {
        Depth = -1;        
        // By default Id is set to IdInvalid.
        // Old search logic treats IdInvalid as a special value.
    }

    // *** GASExecuteTag implementation

    void    Read(GFxLoadProcess* p, GFxTagType tagType);

    virtual void    Execute(GFxSprite* m);
    virtual void    ExecuteState(GFxSprite* m)  { Execute(m); }
    virtual void    ExecuteStateReverse(GFxSprite* m, int frame);   

    virtual bool    IsRemoveTag() const
        { return true; }
};


class GFxSetBackgroundColor : public GASExecuteTag
{
public:
    GColor  Color;

    void    Execute(GFxSprite* m);

    void    ExecuteState(GFxSprite* m)
        { Execute(m); }

    void    Read(GFxLoadProcess* p);    
};



//
// GFxInitImportActions
//

// GFxInitImportActions - created for import tags.
class GFxInitImportActions : public GASExecuteTag
{
    UInt        ImportIndex;
public:
   
    GFxInitImportActions()
    {
        ImportIndex = 0;
    }
    void            SetImportIndex(UInt importIndex)
    {
        ImportIndex = importIndex;
    }

    // Queues up logic to execute InitClip actions from the import,
    // using Highest priority by default.
    virtual void    Execute(GFxSprite* m);  

    // InitImportActions that come from imported files need to be executed
    // in the MovieDefImpl binding context (otherwise we would index parent's
    // source movies incorrectly, resulting in recursive loop).
    void            ExecuteInContext(GFxSprite* m, GFxMovieDefImpl *pbindDef, bool recursiveCheck = 1);  

    virtual bool    IsInitImportActionsTag() const { return true; }
};



// ** Inline Implementation


// ** End Inline Implementation


#endif // INC_GFXIMPL_H
