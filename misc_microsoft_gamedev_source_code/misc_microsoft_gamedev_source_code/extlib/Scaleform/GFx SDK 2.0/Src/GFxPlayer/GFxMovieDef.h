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
class GFxImportData : public GNewOverrideBase
{
public:

    struct Symbol
    {
        GFxString   SymbolName;
        int         CharacterId;
    };

    GFxString           SourceUrl;
    UInt                Frame;
    GTL::garray<Symbol> Imports;

    
    GFxImportData() 
    {        
        Frame = 0;
    }

    GFxImportData(const char* source, UInt frame)
        :
        SourceUrl(source),
        Frame(frame)
    {
    }

    void    AddSymbol(const char* psymbolName, int characterId)
    {
        Symbol s;
        s.SymbolName  = psymbolName;
        s.CharacterId = characterId;
        Imports.push_back(s);
    }
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
		// Round to Platform Alignment
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



// ***** GFxMovieDataDef

// GFxMovieDataDef - stores file loaded data.

// Use a base class for GFxMovieDataDef to make sure that TagMemAllocator
// is destroyed last. That is necessary because GFxMovieDataDef contains
// multiple references to sprites and objects that use that allocator
class GFxMovieDataDefBase : public GFxTimelineDef
{
protected:
    // Tag/Frame Array Memory allocator.
    // Memory is allocated permanently until GFxMovieDataDef dies.
    GFxDataAllocator        TagMemAllocator;

    // Path allocator used for shape data. Use a pointer
    // to avoid including GFxShape.h.
    class GFxPathAllocator* pPathAllocator;
};


// Inherit from GFxTimelineDef (through GFxMovieDataDefBase).
class GFxMovieDataDef : public GFxMovieDataDefBase
{
public:

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

    // Store the resource key used to create us.
    // Note: There is a bit of redundancy here with respect to filename;
    // could resolve it with shared strings later on...
    GFxResourceKey          ResourceKey;



    // **** MovieDataDef Type

    // Describe the type of data that is stored in this DataDef.    
    enum MovieDataType
    {
        MT_Empty,   // No data, empty clip - useful for LoadVars into new level.
        MT_Flash,   // SWF or GFX file.
        MT_Image    // An image file.
    };

    MovieDataType            MovieType;



    // *** Current Loading State
    //   
    // GFxMovieDataDef starts out in LS_Unitialized and switches to
    // the LS_LoadingFrames state as the frames start loading. During this
    // process the LoadingFrames counter will be incremented, with playback
    // being possible after it is greater then 0. 
    // For image files, state can be set immediately to LS_LoadFinished
    // while LoadingFrame is set to 1.

    enum MovieLoadState
    {
        LS_Unitialized,
        LS_LoadingFrames,
        LS_LoadFinished,
        LS_LoadError
    };

    // These values are updated based the I/O progress.
    MovieLoadState          LoadState;
    // Current frame being loaded; playback/rendering is only possible
    // when this value is greater then 0.
    UInt                    LoadingFrame;

    // Tag Count, used for reporting movie info; also advanced during loading.
    // Serves as a general information 
    UInt                    TagCount;


    bool                    IsPlaybackPossible() const 
    {
        return (LoadState >= LS_LoadingFrames) && (LoadingFrame > 0);
    }


    
    // *** Resource Data

    // Resource data is progressively loaded as new tags become available; during
    // loading new entries are added to the data structures below.

    typedef GTL::ghash_uncached<GFxResourceId, GFxResourceHandle, GFxResourceId::HashOp>  ResourceHash;

    // {resourceCharId -> resourceHandle} 
    // A library of all resources defined in this SWF/GFX file.   
    ResourceHash                    Resources;
      
    // A list of ResourceData objects associated with each bindIndex in GFxResourceHandle.
    // Except for imports, which will have an invalid data object, appropriate resources can
    // be created by calling GFxResourceData::CreateResource
    GTL::garray<GFxResourceData>    ResourceBindData;    
 
    // Lists the number of resources for every playlist frame.
    // Resources are to be read from ResourceBindData based on
    // the counters provided in this array.
    GTL::garray<UInt>               ResourcesInFrame;

    // BytesLoaded state for each frame.
    GTL::garray<UInt32>             FrameBytesLoaded;

    // Counter used to assign resource handle indices during loading.
    UInt                            ResIndexCounter;

    // Font descriptors. We need to keep these in case we
    // need to substitute fonts.
    // We can scan through this list to figure out which fonts to substitute.
    struct FontDataUse
    {
        GFxResourceId       Id;
        // pointer is technically redundant
        GPtr<GFxFontData>   pFontData;          
        // This font data is used at specified font index.
        UInt                ResourceBindIndex;
    };
    GTL::garray<FontDataUse>        Fonts;


    // Imports
    GTL::garray<GFxImportData*>     ImportData;


    // TBD: It would be good to consolidate 'Exports' and 'InvExports',
    // so that shared strings can be used for all lookups.

    // A resource can also be exported
    // (it is possible to both import AND export a resource in the same file).
    // {export SymbolName -> Resource}   
    GFxStringHash< GFxResourceHandle >    Exports;

    // Inverse exports map from GFxResourceId, since mapping from GFxResourceHandle
    // would not be useful (we don't know how to create a handle from a GFxResource*).
    // This is used primarily in GetNameOfExportedResource.
    GTL::ghash<GFxResourceId, GFxString>   InvExports;

    

    // *** Playlist Frames
   
    GTL::garray<Frame>          Playlist;          // A list of movie control events for each frame.
    GTL::garray<Frame>          InitActionList;    // Init actions for each frame.
    int                         InitActionsCnt;    // Total number of init action buffers in InitActionList

    GFxStringHash<UInt>			NamedFrames;        // 0-based frame #'s
    
    // Incremented progressively as gradients are loaded to assign ids
    // to their data, and match them with loaded files.
    GFxResourceId               GradientIdGenerator;



    // *** Constructor/Destructor

    GFxMovieDataDef(const GFxResourceKey &creationKey);
    // Initialize GFxMovieDataDef() with a header loaded into processInfo.
    GFxMovieDataDef(GFxLoaderImpl::SWFProcessInfo& processInfo, const GFxResourceKey &creationKey);

    ~GFxMovieDataDef();


    // Create an empty usable definition with no content.
    // Used for LoadVars into a new level.
    void    InitEmptyMovieDef(const char *purl);

    // Create a definition describing an image file.
    void    InitImageFileMovieDef(const char *purl, UInt fileLength,
                                  GFxImageResource *pimageResource, bool bilinear = 1);

    
    // *** Information query methods.
    
    Float               GetFrameRate() const        { return Header.FPS; }
    const GRectF&       GetFrameRectInTwips() const { return Header.FrameRect; }
    GRectF              GetFrameRect() const        { return TwipsToPixels(Header.FrameRect); }
    Float               GetWidth() const            { return ceilf(TwipsToPixels(Header.FrameRect.Width())); }
    Float               GetHeight() const           { return ceilf(TwipsToPixels(Header.FrameRect.Height())); }
    virtual UInt        GetVersion() const          { return Header.Version; }
    virtual UInt        GetLoadingFrame() const     { return LoadingFrame; }
    virtual UInt        GetSWFFlags() const         { return Header.SWFFlags; }

    UInt32              GetFileBytes() const        { return Header.FileLength; }
    UInt                GetTagCount() const         { return TagCount;  }
    const char*         GetFileURL() const          { return FileURL.ToCStr(); }

	UInt                GetFrameCount() const       { return Header.FrameCount; }
	// Obtain MovieDataDef to which this resource belongs (will belong).
	virtual const GFxMovieDataDef*    GetMovieDataDef() const  { return this; }


    // Fills in GfxMovieInfo
    void                GetMovieInfo(GFxMovieInfo *pinfo) const { Header.GetMovieInfo(pinfo); }

 
    // Sets MetaData of desired size.
    UInt                GetMetadata(char *pbuff, UInt buffSize) const;
    void                SetMetadata(UByte *pdata, UInt size);    
    void                SetFileAttributes(UInt attrs)   { FileAttributes = attrs; }    
    UInt                GetFileAttributes() const       { return FileAttributes; }

    
    // Exporter tool info management.
    const GFxExporterInfo*  GetExporterInfo() const                         { return Header.ExporterInfo.GetExporterInfo(); }
    void                    SetExporterInfo(const GFxExporterInfoImpl& src) { Header.ExporterInfo = src; }


    // Helper used to look up labeled frames and/or translate frame numbers from a string.
    static bool             TranslateFrameString(
                    const  GFxStringHash<UInt>  &namedFrames,
                    const char* label, UInt* frameNumber, bool translateNumbers);

    // Returns 0-based frame #
    bool                    GetLabeledFrame(const char* label, UInt* frameNumber, bool translateNumbers = 1)
    {
        return TranslateFrameString(NamedFrames, label, frameNumber, translateNumbers);     
    }

    // Get allocator used for path shape storage.
    GFxPathAllocator*   GetPathAllocator() { return pPathAllocator; }



    // ** Loading

    // Read a .SWF pMovie.
    // Binding will take place interleaved with loading if passed; no binding is
    // done otherwise.
    void                Read(GFxLoadProcess *plp, GFxMovieBindProcess* pbp = 0);

    // Frame access during loading.

    // Increments LoadingFrame. 
    // It is expected that it will also release any threads waiting on it in the future.
    void                IncrementLoadingFrame() { LoadingFrame++;  }
    // Wait for the specified loading frame, returning 1 if succeeded and 0 if loading failed.
    bool                WaitForLoadingFrame(UInt frame);


    
    // Allocate MovieData local memory.
    inline void*        AllocTagMemory(size_t bytes)    { return TagMemAllocator.Alloc(bytes);  }

   
    // Add a resource during loading.
    void                AddResource(GFxResourceId rid, GFxResource* pres);   
  
    // A character is different from other resources because it tracks its creation Id.
    void                AddCharacter(GFxResourceId rid, GFxCharacterDef* c)
    {
        c->SetId(rid);
        AddResource(rid, c);
    }

    // Adds a new resource and generates a handle for it.
    GFxResourceHandle   AddDataResource(GFxResourceId rid, const GFxResourceData &resData);
    // Add import resource and generate a handle for it.
    GFxResourceHandle   AddImportResource(GFxResourceId rid, GFxImportData *pimportData);

    // Adds a font data resource. Fonts are special because we need to be able to
    // look them up by frames and do substitution even when they are not imported.
    GFxResourceHandle   AddFontDataResource(GFxResourceId rid, GFxFontData *pfontData);
    
    // Get font data bi ResourceId.
    GFxFontData*        GetFontData(GFxResourceId rid);
    
    bool                GetResourceHandle(GFxResourceHandle* phandle, GFxResourceId rid) const;

    
    // *** Frame loading        
    
    inline bool             HasInitActions() const   { return InitActionsCnt > 0; }
    GFxResourceId           GetNextGradientId()     { return GradientIdGenerator.GenerateNextId(); }
  
    // Expose one of our resources under the given symbol, for export.  Other movies can import it.        
    void                    ExportResource(const GFxString& symbol, GFxResourceId rid, GFxResourceHandle hres);
    // Adds ImportData, ownership passed to the Imports array.
    void                    AddImport(GFxImportData* pimportData);

    
    // GFxTimelineDef implementation.

    // Labels the frame currently being loaded with the given name.
    // A copy of the name string is made and kept in this object.    
    virtual void            AddFrameName(const char* name, GFxLog *plog);

    virtual void            SetLoadingPlaylistFrame(const Frame& frame);
    virtual void            SetLoadingInitActionFrame(const Frame& frame);  

	// Frame access from GFxTimelineDef.
    virtual const Frame&    GetPlaylist(int frameNumber) const;
    virtual const Frame*    GetInitActions(int frameNumber) const;


    // *** Creating MovieDefData file keys

    // Create a key for an SWF file corresponding to GFxMovieDef.
    // Note that GFxImageCreator is only used as a key if is not null.
    static  GFxResourceKey  CreateMovieFileKey(const char* pfilename,
                                               SInt64 modifyTime,
                                               GFxFileOpener* pfileOpener,
                                               GFxImageCreator* pimageCreator);


    // *** GFxResource implementation
    
    virtual GFxResourceKey  GetKey()                        { return ResourceKey; }
    virtual UInt            GetResourceTypeCode() const     { return MakeTypeCode(RT_MovieDataDef); }

};




// State values that cause generation of a different binding
class  GFxMovieDefBindStates : public GRefCountBase<GFxMovieDefBindStates>
{
public:

    
    // Pointer to MovieDefData. This pointer starts out NULL,
    // however, it is always initialized through 
    GPtr<GFxMovieDataDef>     pDataDef;

    // Cached relativePath taken from DataDef URL, stored
    // so that it can be easily passed for imports.
    GFxString                 RelativePath;


    // GFxStates that would cause generation of a new DefImpl binding.
    // These are required for the following reasons:
    //
    //  Class               Binding Change Cause
    //  ------------------  ----------------------------------------------------
    //  GFxFileOpener       Substitute GFile can produce different data.
    //  GFxURLBuilder   Can cause different import file substitutions.
    //  GFxImageCreator     Different image representaion = different binding.
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

    GFxMovieDefBindStates(GFxSharedState* psharedState)
    {
        // Get multiple states at once to avoid extra locking.
        GFxState*                        pstates[9]    = {0,0,0,0,0,0,0,0,0};
        const static GFxState::StateType stateQuery[9] =
          { GFxState::State_FileOpener,     GFxState::State_URLBuilder,
            GFxState::State_ImageCreator,   GFxState::State_ImportVisitor,
            GFxState::State_GradientParams, GFxState::State_FontPackParams,
            GFxState::State_FontProvider,   GFxState::State_FontLib,
            GFxState::State_FontMap };

        // Get states and assign them locally.
        psharedState->GetStatesAddRef(pstates, stateQuery, 9);
        pFileOpener     = *(GFxFileOpener*)     pstates[0];
        pURLBulider     = *(GFxURLBuilder*)     pstates[1];
        pImageCreator   = *(GFxImageCreator*)   pstates[2];
        pImportVisitor  = *(GFxImportVisitor*)  pstates[3];
        pGradientParams = *(GFxGradientParams*) pstates[4];
        pFontPackParams = *(GFxFontPackParams*) pstates[5];
        pFontProvider   = *(GFxFontProvider*)   pstates[6];
        pFontLib        = *(GFxFontLib*)        pstates[7];
        pFontMap        = *(GFxFontMap*)        pstates[8];
    }

    GFxMovieDefBindStates(GFxMovieDefBindStates *pother)
    {
        pFileOpener     = pother->pFileOpener;
        pURLBulider     = pother->pURLBulider;
        pImageCreator   = pother->pImageCreator;
        pImportVisitor  = pother->pImportVisitor;
      //  pImageVisitor   = pother->pImageVisitor;
        pFontPackParams = pother->pFontPackParams;
        pGradientParams = pother->pGradientParams;
        pFontProvider   = pother->pFontProvider;
        pFontLib        = pother->pFontLib;
        pFontMap        = pother->pFontMap;
        // Leave pDataDef uninitialized since this is used
        // from GFxLoadStates::CloneForImport, and imports
        // have their own DataDefs.
    }

    void                SetDataDef(GFxMovieDataDef* pdef);
    
    GFxMovieDataDef*    GetDataDef() const { return pDataDef; }
    const GFxString&    GetRelativePath() const { return RelativePath;  }

    // Functionality necessary for this GFxMovieDefBindStates to be used
    // as a key object for GFxMovieDefImpl.
    size_t  GetHashCode() const;
    bool operator == (GFxMovieDefBindStates& other) const;  
    bool operator != (GFxMovieDefBindStates& other) const { return !operator == (other); }    
};



// *** GFxMovieDefBindProcess

// GFxMovieDefBindProcess stores the states necessary for binding. The actual
// binding is implemented by calling BindFrame for every frame that needs to be bound.
//
// Binding is separated into a separate object so that it can be reused independent
// of whether it takes place in a separate thread (which just calls BindFrame until
// its done) or is interleaved together with the loading process, which can call
// it after each frame.


class GFxMovieBindProcess : public GRefCountBase<GFxMovieBindProcess>
{
    UInt                BindIndex;
    UInt                ImportIndex;
    UInt                FontUseIndex; 

    // Perform binding of resources.
    GFxResourceId       GlyphTextureIdGen;

    bool                Stripped;
    GFxMovieDataDef*    pDataDef;    
    GFxLoadStates *     pLoadStates;

    // Keep a smart pointer to DefImpl to guard against the possibility of DefImpl
    // being unloaded while it's still being bound in a different thread.
    // TBD: Need more intelligent system with 'Abandon loading' feature.
    GPtr<GFxMovieDefImpl> pDefImpl;

public:

    enum BindState
    {
        Bind_NotStarted,
        Bind_InProgress,
        Bind_Finished,
        Bind_Error
    };
    BindState           State;


    
    GFxMovieBindProcess(GFxLoadStates *pls, GFxMovieDefImpl* pdefImpl);

    // Bind a next frame.
    // If binding failed, then Bind_Error will be returned.
    BindState           BindNextFrame();

    void                FinishBinding();

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
      
    // Binding table for handles in pMovieDataDef.
    GFxResourceBinding          ResourceBinding;

    // Movies we import from; hold a ref on these, to keep them alive.
    // This array directly corresponds to GFxMovieDataDef::ImportData.
    GTL::garray<GPtr<GFxMovieDefImpl> >    ImportSourceMovies;

    // Other imports such as through FontLib.
    GTL::garray<GPtr<GFxMovieDefImpl> >    ResourceImports;

    
    // Bound Frame - the frame we are binding now. This frame corresponds
    // to the Loading Frame, as only bound frames (with index < BindingFrame)
    // can actually be used.
    volatile UInt               BindingFrame;

    // Bound amount of bytes loaded for the Binding frame,
    // must be assigned before BindingFrame.
    volatile UInt32             BytesLoaded;

    // Save load flags so that they can be propagated
    // into the nested LoadMovie calls.
    UInt                        LoadFlags;


    
    // *** Constructor / Destructor

    GFxMovieDefImpl(GFxMovieDefBindStates* pstates,
                    GFxLoaderImpl* ploaderImpl,
                    UInt loadConstantFlags,
                    GFxSharedStateImpl *pdelegateState = 0,
                    bool fullyLoaded = 0);
    ~GFxMovieDefImpl();

   
    // After GFxMovieDefImpl constructor, the most important part of GFxMovieDefImpl 
    // is initialization binding, i.e. resolving all of dependencies based on the binding states.
    // This is a step where imports and images are loaded, gradient images are
    // generated and fonts are pre-processed.
    // Binding is done by calls to GFxMovieBindProcess::BindNextFrame.

    // Resolves and import during binding.
    void                    ResolveImport(UInt importIndex, GFxMovieDefImpl* pdefImpl,
                                          GFxLoadStates *pls, bool recursive);

    // Resolves an import of 'gfxfontlib.swf' through the GFxFontLib object.
    // Returns 1 if ALL mappings succeeded, otherwise 0.
    bool                    ResolveImportThroughFontLib(UInt importIndex);

    // Internal helper for import updates.
    bool                    SetResourceBindData(GFxResourceId rid, GFxResourceBindData& bindData,
                                                const char* pimportSymbolName);



    // Create a movie instance.
    GFxMovieView*           CreateInstance(bool initFirstFrame = 0);


    // *** Creating MovieDefImpl keys

    // GFxMovieDefImpl key depends (1) pMovieDefData, plus (2) all of the states
    // used for its resource bindings, such as file opener, file translator, image creator,
    // visitors, etc. A snapshot of these states is stored in GFxMovieDefBindStates.
    // Movies that share the same bind states are shared through GFxResourceLib.

    // Create a key for an SWF file corresponding to GFxMovieDef.
    static  GFxResourceKey  CreateMovieKey(GFxMovieDefBindStates* pbindStates);
    
    

    // *** Property access

    GFxMovieDataDef*    GetDataDef() const      { return pBindStates->GetDataDef(); }

    // ...
    UInt                GetFrameCount() const       { return GetDataDef()->GetFrameCount(); }
    Float               GetFrameRate() const        { return GetDataDef()->GetFrameRate(); }
    GRectF              GetFrameRect() const        { return GetDataDef()->GetFrameRect(); }
    Float               GetWidth() const            { return GetDataDef()->GetWidth(); }
    Float               GetHeight() const           { return GetDataDef()->GetHeight(); }
    virtual UInt        GetVersion() const          { return GetDataDef()->GetVersion(); }
    virtual UInt        GetLoadingFrame() const     { return BindingFrame; } // TBD
    virtual UInt        GetSWFFlags() const         { return GetDataDef()->GetSWFFlags(); }
    
    virtual const char* GetFileURL() const          { return GetDataDef()->GetFileURL(); }

    UInt32              GetFileBytes() const        { return GetDataDef()->GetFileBytes(); }
    UInt32              GetBytesLoaded() const      { return BytesLoaded; }
    UInt                GetTagCount() const         { return GetDataDef()->GetTagCount();  }

    // Stripper info query.
    virtual const GFxExporterInfo*  GetExporterInfo() const  { return GetDataDef()->GetExporterInfo(); }

    // Faster non-virtual version (internal use only).
    //const GRectF&              GetFrameRectRef() const     { return GetDataDef()->GetFrameRect(); }
    GRectF                     GetFrameRectInTwips() const { return GetDataDef()->GetFrameRectInTwips(); }
    GFxResourceWeakLib*        GetWeakLib() const          { return pLoaderImpl->GetWeakLib(); }

    // Shared state implementation.
    virtual GFxSharedState* GetSharedImpl() const       { return pSharedState.GetPtr(); }    

    // Overrides for users
    virtual UInt                GetMetadata(char *pbuff, UInt buffSize) const
        { return GetDataDef()->GetMetadata(pbuff, buffSize); }
    virtual UInt                GetFileAttributes() const { return GetDataDef()->GetFileAttributes(); }


    // Obtain flags used to load shapes.
    inline UInt                GetLoadFlags() const     { return LoadFlags; }
   
    // GFxLog Error delegation
    void    LogError(const char* pfmt, ...)
    { 
        va_list argList; va_start(argList, pfmt);
        GFxLog *plog = GetLog();
        if (plog) plog->LogMessageVarg(GFxLog::Log_Error, pfmt, argList);
        va_end(argList); 
    }

       

    // *** Resource Lookup

    // Obtains a resource based on its id. If resource is not yet resolved,
    // NULL is returned. Should be used only before creating an instance.
    // Type checks the resource based on specified type.
    GFxResource*                GetResource(GFxResourceId rid, GFxResource::ResourceType rtype);
    // Obtains full character creation information, including GFxCharacterDef.
    GFxCharacterCreateInfo      GetCharacterCreateInfo(GFxResourceId rid);

  
   
    // *** GFxMovieDef implementation

    virtual void                VisitImportedMovies(ImportVisitor* visitor);       

    // Enumerates all resources.    
    virtual void                VisitResources(ResourceVisitor* pvisitor, UInt visitMask = ResVisit_AllImages);

    virtual GFxResource*        GetResource(const char *pexportName) const;


    // Locate a font resource by name and style.
    // It's ok to return GFxFontResource* without the binding because pBinding
    // is embedded into font resource allowing imports to be handled properly.
    virtual GFxFontResource*    GetFontResource(const char* pfontName, UInt styleFlags);


    //virtual GImageInfoBase*     GetImageInfo(const char *pexportName) const;


    // May Come in handy
    void                    PrecomputeCachedData();
   

    // Fill in the binding resource information together with its binding.
    // Return 0 if Get failed and no bind data was returned.
    bool                    GetExportedResource(GFxResourceBindData *pdata, const GFxString& symbol);
    
    const GFxString*       GetNameOfExportedResource(GFxResourceId rid) const;
    

    // Debug helper; returns true if the given
    // CharacterId is listed in the import table.
 //   bool                    InImportTable(int CharacterId);     


    // *** GFxResource implementation

    virtual GFxResourceKey  GetKey()                        { return CreateMovieKey(pBindStates); }
    virtual UInt            GetResourceTypeCode() const     { return MakeTypeCode(RT_MovieDef); }

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
    GFxTextFilter*      TextFilter;

    GFxCharPosInfo()
    {
        Ratio       = 0.0f;
        HasMatrix   = 0;
        HasCxform   = 0;
        Depth       = 0;        
        ClipDepth   = 0;
        BlendMode   = GRenderer::Blend_None;
        TextFilter  = 0;
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

    ~GFxCharPosInfo()
    {
        delete TextFilter;
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
