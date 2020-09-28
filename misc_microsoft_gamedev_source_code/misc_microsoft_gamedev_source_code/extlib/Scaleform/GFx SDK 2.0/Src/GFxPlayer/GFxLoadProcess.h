/**********************************************************************

Filename    :   GFxLoadProcess.h
Content     :   GFxLoadProcess - tracks loading and binding state.
Created     :   
Authors     :   

Copyright   :   (c) 2001-2006 Scaleform Corp. All Rights Reserved.

Notes       :   This file contains class declarations used in
                GFxPlayerImpl.cpp only. Declarations that need to be
                visible by other player files should be placed
                in GFxCharacter.h.


Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef INC_GFXLOADPROCESS_H
#define INC_GFXLOADPROCESS_H

#include "GTLTypes.h"

#include "GFxStream.h"
#include "GFxLoaderImpl.h"

#include "GFxTaskManager.h"

// LoadProcess relies no many MovieDef function and APIs.
#include "GFxMovieDef.h"


// ***** Declared Classes

class GFxLoadProcess;

// ***** External Classes

class GFxLoader;
class GFxFontPackParams;
class GFxMovieRoot;
class GFxLoadQueueEntry;

class GJPEGInput;



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


    // *** Substitute fonts

    // These are 'substitute' fonts provider files that need to be considered when
    // binding creating a GFxFontResource from ResourceData. Substitute fonts come from
    // files that have '_glyphs' in filename and replace fonts with matching names
    // in the file being bound.
    // Technically, this array should be a part of 'GFxBindingProcess'. If more
    // of such types are created, we may need to institute one.
    GTL::garray<GPtr<GFxMovieDefImpl> > SubstituteFontMovieDefs;


    
    GFxLoadStates() { }
    // Creates GFxLoadStates by capturing states from pstates. If pstates
    // is null, all states come from loader. If binds states are null,
    // bind states come from pstates.
    GFxLoadStates(GFxLoaderImpl* ploader, GFxSharedState* pstates = 0,
                  GFxMovieDefBindStates *pbindStates = 0);

    // Helper that clones load states, pBindSates.
    // The only thing left un-copied is GFxMovieDefBindStates::pDataDef
    GFxLoadStates*          CloneForImport() const;


    GFxResourceWeakLib*     GetLib() const            { return pWeakResourceLib.GetPtr();  }
    GFxMovieDefBindStates*  GetBindStates() const     { return pBindStates.GetPtr(); }
    GFxLog*                 GetLog() const            { return pLog; }
    GFxTaskManager*         GetTaskManager() const    { return pTaskManager; }
    GFxRenderConfig*        GetRenderConfig() const   { return pRenderConfig; }
    GFxFontCacheManager*    GetFontCacheManager() const { return pFontCacheManager; }

    GFxFileOpener*          GetFileOpener() const     { return pBindStates->pFileOpener;  }
    GFxFontPackParams*      GetFontPackParams() const { return pBindStates->pFontPackParams; }

    void                    SetDataDef(GFxMovieDataDef* pdef)   { pBindStates->SetDataDef(pdef); }
    GFxMovieDataDef*        GetDataDef() const                  { return pBindStates->GetDataDef(); }

    const GFxString&        GetRelativePath() const             { return pBindStates->GetRelativePath();  }


    // Obtains an image creator, but only if image data is not supposed to
    // be preserved; considers GFxLoader::LoadKeepBindData flag from arguments.
    GFxImageCreator*        GetLoadTimeImageCreator(UInt loadConstants) const; 


    // Delegated state access helpers
    GFile*      OpenFile(const char *pfilename);

    // Perform filename translation and/or copy by relying on translator.
    void        BuildURL(GFxString *pdest, const GFxURLBuilder::LocationInfo& loc) const;
    
};



// ***** Loading Process

class GFxLoadProcess : public GRefCountBase<GFxLoadProcess>, public GFxLogBase<GFxLoadProcess>
{
public:

    // Load states used during load processing.
    GPtr<GFxLoadStates>         pStates;
    // Cached GFxParseControl flags from the states for more efficient checks.
    UInt                        ParseFlags;


    // *** Temporary load process state

    GFxLoaderImpl::SWFProcessInfo   ProcessInfo;

    // Temporary: copy of DataDef from pStates.
    GFxMovieDataDef*            pDataDef;

    GJPEGInput*                 pJpegIn;

    // Load flags, from GfxLoader::LoadConstants
    UInt                        LoadFlags;


    // *** Sprite/Root time-line loading stack

    enum LoadStateType
    {
        LS_LoadingRoot      = 0,
        LS_LoadingSprite    = 1,
        LS_LoadState_Count 
    };

    LoadStateType               LoadState;

    // Definition into which data is being loaded. This will either be
    // a GFxMovieDataDef for root or GFxSpriteDef for loading a sprite.
    GFxTimelineDef*             pTimelineDef;

    // Array of loaded frame pointers. Once the frame is done, this 
    // array is re-allocated as tag data and saved in GFxMovieDataDef.
    // Note that tags themselves also come from Def's allocator.    
    GTL::garray<GASExecuteTag*> FrameTags[LS_LoadState_Count];
    GTL::garray<GASExecuteTag*> InitActionTags;




    GFxLoadProcess(GFxLoadStates *pstates, UInt loadFlags);    
    ~GFxLoadProcess();
    

    // Initializes the stream.
    bool Initialize (GFile *pfile, bool readingMsg = 0)
    {
        return ProcessInfo.Initialize(pfile, this, readingMsg);
    }   

    GFxLoaderImpl::SWFProcessInfo&  GetProcessInfo() { return ProcessInfo; }


    // Save/access an input object used for later loading DefineBits
    // Images (JPEG images without the table info).
    void        SetJpegLoader(GJPEGInput* pjin) { GASSERT(pJpegIn == NULL); pJpegIn = pjin; }
    GJPEGInput* GetJpegLoader() const           { return pJpegIn; }


    // Cache dataDef locally for more efficient access.
    void                    SetDataDef(GFxMovieDataDef* pdef)
    {
        GASSERT(pDataDef == 0 && pTimelineDef == 0);
        pStates->SetDataDef(pdef);
        pDataDef     = pdef;
        // Set TimelineDef so that it can proceed correctly.
        pTimelineDef = pDataDef;
    }

    GFxMovieDataDef*        GetDataDef() const                  { return pDataDef; }
    
    GFxResourceWeakLib*     GetWeakLib() const                   { return pStates->pWeakResourceLib; }

    GFxLoadStates*          GetLoadStates() const       { return pStates; }
    GFxMovieDefBindStates*  GetBindStates() const       { return pStates->GetBindStates(); }

    const char*             GetFileURL() const          { return GetDataDef()->GetFileURL(); }
    UInt                    GetVersion() const          { return GetDataDef()->GetVersion(); }

    // Stripper info query.
    const GFxExporterInfo*  GetExporterInfo() const     { return GetDataDef()->GetExporterInfo(); }


    // *** GFxStream Support

    inline  GFxStream* GetStream()          { return &ProcessInfo.Stream; }

    // Stream inlines
    inline UInt8    ReadU8()                { return GetStream()->ReadU8(); }
    inline UInt16   ReadU16()               { return GetStream()->ReadU16(); }
    inline UInt32   ReadU32()               { return GetStream()->ReadU32(); }
    inline SInt8    ReadS8()                { return GetStream()->ReadS8(); }
    inline SInt16   ReadS16()               { return GetStream()->ReadS16(); }
    inline SInt32   ReadS32()               { return GetStream()->ReadS32(); }

    inline void     AlignStream()           { return GetStream()->Align(); }

    inline GFile*   GetUnderlyingFile ()    { return GetStream()->GetUnderlyingFile();  }
    inline int      GetTagEndPosition()     { return GetStream()->GetTagEndPosition(); }
    inline int      Tell()                  { return GetStream()->Tell();  }
    inline void     SetPosition(int pos)    { GetStream()->SetPosition(pos);  }

    void            ReadRgbaTag(GColor *pc, GFxTagType tagType);


    // *** Delegated Logging Support 

    // GFxLogBase will output log messages to the appropriate logging stream,
    // but only if the enable option in context is set. 
    GFxLog*         GetLog() const              { return pStates->pLog; }

    // These work of captured parse control flags for efficiency. This is ok because
    // user updates that take place in the middle of loading are not guaranteed to take effect.
    inline bool     IsVerboseParse() const              { return (ParseFlags & GFxParseControl::VerboseParse) != 0; }
    inline bool     IsVerboseParseShape() const         { return (ParseFlags & GFxParseControl::VerboseParseShape) != 0; }
    inline bool     IsVerboseParseMorphShape() const    { return (ParseFlags & GFxParseControl::VerboseParseMorphShape) != 0; }
    inline bool     IsVerboseParseAction() const        { return (ParseFlags & GFxParseControl::VerboseParseAction) != 0; }


    inline bool     IsLoadingImageData() const
    { return 1; }
    inline bool     IsLoadingFontShapes() const
    { return 1; }


    inline GFxResourceId  GetNextGradientId() { return pDataDef->GetNextGradientId(); }
    



    // *** Resource Loading / Access

    
    // Add a resource during loading.
    inline void    AddResource(GFxResourceId rid, GFxResource* pres);
    
    inline void    AddCharacter(GFxResourceId rid, GFxCharacterDef* pdef)
    {
        return GetDataDef()->AddCharacter(rid, pdef);
    }

//    inline void    AddResource(GFxResourceId rid, GFxResourceFileInfo* pfileInfo);

    inline bool    GetResourceHandle(GFxResourceHandle* phandle, GFxResourceId rid) const;
    
    // Adds a new resource and generates a handle for it.
    GFxResourceHandle   AddDataResource(GFxResourceId rid, const GFxResourceData &resData)
    {
        return GetDataDef()->AddDataResource(rid, resData);
    }

    GFxResourceHandle   AddFontDataResource(GFxResourceId rid, GFxFontData *pfontData)
    {
        return GetDataDef()->AddFontDataResource(rid, pfontData);
    }


    // Add a dynamicaly-loaded image resource, with unique key.
    // This is normally used for SWF embedded images.
    // Based on image creator flags, either the resource itself or
    // its resource data will be added.
    void            AddImageResource(GFxResourceId rid, GImage *pimage);



    // *** Frame Loading

    // Enter and leave Sprite. Only one level of nesting
    // is possible in SWF files.
    void                EnterSpriteDef(GFxTimelineDef *psprite)
    {
        GASSERT(LoadState == LS_LoadingRoot);
        GASSERT(psprite != 0);
        LoadState    = LS_LoadingSprite;
        pTimelineDef = psprite;
    }

    void                LeaveSpriteDef()
    {
        GASSERT(LoadState == LS_LoadingSprite);
        LoadState    = LS_LoadingRoot;
        pTimelineDef = pDataDef;
    }


    // Get allocator used for path shape storage.
    GFxPathAllocator*   GetPathAllocator() const        { return GetDataDef()->GetPathAllocator(); }

    // Allocate MovieData local memory.
    inline void*        AllocTagMemory(size_t bytes)    { return GetDataDef()->AllocTagMemory(bytes);  }
    // Allocate a tag directly through method above.
    template<class T>
    inline T*           AllocTag()                      { return GTL::gconstruct<T>(AllocTagMemory(sizeof(T))); }

    // Labels the frame currently being loaded with the given name.
    // A copy of the name string is made and kept in this object.    
    inline void    AddFrameName(const char* pname, GFxLog *plog)
    {
        pTimelineDef->AddFrameName(pname, plog);
    }

    inline void    AddExecuteTag(GASExecuteTag* ptag)
    {
        FrameTags[LoadState].push_back(ptag);
    }
    // Need to execute the given tag before entering the
    // currently-loading frame for the first time.    
    inline void    AddInitAction(GFxResourceId spriteId, GASExecuteTag* ptag)
    {
        GUNUSED(spriteId);
        GASSERT(LoadState == LS_LoadingRoot);
        InitActionTags.push_back(ptag);
    }
    
    // Helper method used to convert accumulated tags into a frame.
    GFxTimelineDef::Frame TagArrayToFrame(GTL::garray<GASExecuteTag*> &tagArray);

    // Apply frame tags that have been accumulated to the MovieDef;
    inline void    CommitFrameTags()
    {
        // Add frame to time-line. Timeline can be either MovieDataDef or SpriteDef.
        GASSERT(pTimelineDef);
        pTimelineDef->SetLoadingPlaylistFrame(TagArrayToFrame(FrameTags[LoadState]));
        if (LoadState == LS_LoadingRoot)
            pTimelineDef->SetLoadingInitActionFrame(TagArrayToFrame(InitActionTags));
    }

    inline bool     FrameTagsAvailable() const
    {
        return (FrameTags[LoadState].size() > 0) ||
               ((LoadState == LS_LoadingRoot) && (InitActionTags.size() > 0));
    }

    void ReportProgress(const GFxString& fileURL, const GFxTagInfo& tagInfo, UInt bytesLoaded, UInt totalBytes)
    { 
        if (pStates->pProgressHandler)
            pStates->pProgressHandler->ProgressUpdate(fileURL, tagInfo, bytesLoaded, totalBytes); 
    }

};




// ***** Image creator from files

class GFxImageFileResourceCreator : public GFxResourceData::DataInterface
{
    typedef GFxResourceData::DataHandle DataHandle;

    // Creates/Loads resource based on data and loading process
    virtual bool    CreateResource(DataHandle hdata, GFxResourceBindData *pbindData,
                                   GFxLoadStates *plp) const;
public:

    static  GFxResourceData CreateImageFileResourceData(GFxImageFileInfo * prfi);    
};

class GFxImageResourceCreator : public GFxResourceData::DataInterface
{
    typedef GFxResourceData::DataHandle DataHandle;

    // Creates/Loads resource based on data and loading process
    virtual bool    CreateResource(DataHandle hdata, GFxResourceBindData *pbindData,
                                   GFxLoadStates *plp) const;
public:

    static  GFxResourceData CreateImageResourceData(GImage* pimage);    
};




// GFxLoadProcess Inlines (delegation)

inline void    GFxLoadProcess::AddResource(GFxResourceId rid, GFxResource* pres)
{
    GASSERT(LoadState == LS_LoadingRoot);
    if (LoadState == LS_LoadingRoot)
        pDataDef->AddResource(rid, pres);
}

/*
inline void    GFxLoadProcess::AddResource(GFxResourceId rid, GFxResourceFileInfo* pfileInfo)
{
    GASSERT(LoadState == LS_LoadingRoot);
    if (LoadState == LS_LoadingRoot)
        pDataDef->AddResource(rid, pfileInfo);
}
*/

inline bool    GFxLoadProcess::GetResourceHandle(GFxResourceHandle* phandle, GFxResourceId rid) const
{
    return pDataDef->GetResourceHandle(phandle, rid);
}



#endif // INC_GFXLOADPROCESS_H
