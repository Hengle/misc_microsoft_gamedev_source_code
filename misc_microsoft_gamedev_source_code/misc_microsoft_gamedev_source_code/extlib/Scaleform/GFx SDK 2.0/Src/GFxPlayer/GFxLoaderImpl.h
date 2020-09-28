/**********************************************************************

Filename    :   GFxLoaderImpl.h
Content     :   SWF loading interface implementation for GFxPlayer
Created     :   
Authors     :   Michael Antonov

Copyright   :   (c) 2001-2006 Scaleform Corp. All Rights Reserved.

Notes       :   

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef INC_GFXLOADERIMPL_H
#define INC_GFXLOADERIMPL_H

#include "GTLTypes.h"
#include "GRefCount.h"
#include "GFxLoader.h"

//#include "GFxCharacter.h"
#include "GFxResourceHandle.h"

#include "GFxLog.h"
#include "GFxStream.h"
#include "GFxString.h"

#include "GFxTagLoaders.h"

// ***** Declared Classes
class GFxLoaderImpl;
class GFxSharedStateImpl;

class GFxExporterInfoImpl;

class GFxImageResource;

class GFxMovieDefImpl;

class GFxMovieBindProcess;


// ***** Helper classes for extractor tool data

class GFxExporterInfoImpl
{
    GFxExporterInfo SI;    
    // String buffers that actually hold data SI elements point to.
    GFxString Prefix;
    GFxString SWFName;

public:

    GFxExporterInfoImpl()
    {
        SI.Format       = GFxLoader::File_Unopened;
        SI.pSWFName     = 0;
        SI.pPrefix      = 0;
        SI.ExportFlags  = 0;
        SI.Version      = 0;

    }
    GFxExporterInfoImpl(const GFxExporterInfoImpl &src)
    { SetData(src.SI.Version, src.SI.Format, src.SI.pSWFName, src.SI.pPrefix, src.SI.ExportFlags); }
    const GFxExporterInfoImpl& operator = (const GFxExporterInfoImpl &src)
    { SetData(src.SI.Version, src.SI.Format, src.SI.pSWFName, src.SI.pPrefix, src.SI.ExportFlags); return *this; }

    // Assigns data
    void    SetData(UInt16 version, GFxLoader::FileFormatType format,
                    const char* pname, const char* pprefix, UInt flags = 0);
    
    const GFxExporterInfo* GetExporterInfo() const
    { return (SI.Format == GFxLoader::File_Unopened) ? 0 : &SI; }

    
    // Read in the data from tag GFxTag_ExporterInfo. It is assumed the tag has been opened.    
    void    ReadExporterInfoTag(GFxStream *pstream, GFxTagType tagType);
};



// ***** Loader Implementation class


// Shared state - allocated in loader, movieDef, and movie root instance.
class GFxSharedStateImpl : public GRefCountBase<GFxSharedStateImpl>,
                           public GFxSharedState,                          
                           public GFxLogBase<GFxSharedStateImpl>
{
protected:

    // Pointer to the delegate implementation which we fall
    // back onto if our fields do not provide a certain value.
    GPtr<GFxSharedStateImpl> pDelegate;


    struct StatePtr
    {
        GPtr<GFxState> pState;

        StatePtr() { }
        StatePtr(GFxState* pstate)
            : pState(pstate) { }
        StatePtr(const GPtr<GFxState> &pstate)
            : pState(pstate) { }
        StatePtr(const StatePtr& other)
            : pState(other.pState) { }

        bool operator == (GFxState::StateType stype) const
        { return pState->GetStateType() == stype; }
        bool operator != (GFxState::StateType stype) const
        { return !operator == (stype); }

        bool operator == (const StatePtr& other) const
        { return pState->GetStateType() == other.pState->GetStateType(); }
        bool operator != (const StatePtr& other) const
        { return !operator == (other); }
    };


    // Hash function used for nodes.
    struct StatePtrHashOp
    {                
        // Hash code is computed based on a state key.
        size_t  operator() (const StatePtr& pstate) const
        {
            GASSERT(pstate.pState.GetPtr() != 0);
            return (size_t) pstate.pState->GetStateType();
        }
        size_t  operator() (const GPtr<GFxState>& pstate) const
        {
            GASSERT(pstate.GetPtr() != 0);
            return (size_t) pstate->GetStateType();
        }
        size_t  operator() (const GFxState* pstate) const
        {
            GASSERT(pstate != 0);
            return (size_t) pstate->GetStateType();
        }
        size_t  operator() (GFxState::StateType stype) const
        {
            return (size_t) stype;
        }
    };


    // State hash
    typedef GTL::ghash_set<StatePtr, StatePtrHashOp> StateHash;

    StateHash       States;
    // Lock to keep state updates thread-safe.
    mutable GLock   StateLock;

public:
    
    GFxSharedStateImpl(GFxSharedStateImpl *pdelegate)   
    {
        pDelegate = pdelegate;
    }
    ~GFxSharedStateImpl()
    {
    }

    // Change delegate
    void    SetDelegate(GFxSharedStateImpl* pdelegate)
    {
        pDelegate = pdelegate;
    }
    GFxSharedStateImpl* GetDelegate() const
    {
        return pDelegate;
    }

    // Copy states, as necessary when creating a new loader from an old one.    
    void    CopyStatesFrom(GFxSharedStateImpl *pother)
    {  
        GLock::Locker lock(&StateLock);
        GLock::Locker lockOther(&pother->StateLock);
        States = pother->States;
    }
    
    // *** GFxSharedState member implementation.

    // General state access.
    virtual void        SetState(GFxState::StateType state, GFxState* pstate)
    {
        GLock::Locker lock(&StateLock);
        if (pstate)
            States.set(StatePtr(pstate));
        else
            States.remove(state);
    }

    virtual GFxState*   GetStateAddRef(GFxState::StateType state) const
    {
        const StatePtr* pstate;       
        {   // Lock scope for 'get'.
            GLock::Locker lock(&StateLock);
            pstate = States.get(state);

            if (pstate)
            {
                pstate->pState->AddRef();
                return pstate->pState;
            }            
        }

        if (pDelegate) 
            return pDelegate->GetStateAddRef(state);
        return 0;
    }

    // Fills in a set of states with one call.
    virtual void        GetStatesAddRef(GFxState** pstateList,
                                        const GFxState::StateType *pstates, UInt count) const
    {
        const StatePtr* pstate;
        // Not Found flag is set to call a delegate if at least one
        // of the requested states isn't yet filled in.
        bool            notFound = 0;        

        // Lock scope for 'get'.
        {
            GLock::Locker lock(&StateLock);

            for (UInt i=0; i<count; i++)
            {
                if (pstateList[i] == 0)
                {
                    pstate = States.get(pstates[i]);
                    if (pstate)
                    {
                        pstate->pState->AddRef();
                        pstateList[i] = pstate->pState;
                    }
                    else
                    {
                        notFound = true;
                    }
                }
            }
        }

        if (notFound && pDelegate) 
            pDelegate->GetStatesAddRef(pstateList, pstates, count);
    }

};




struct GFxMovieHeaderData
{
    UInt32              FileLength;
    SInt                Version;
    GRectF              FrameRect;
    Float               FPS;
    UInt                FrameCount;
    UInt                SWFFlags;
    // Exporter info, read through tag 'GFxTag_ExporterInfo' during header loading.
    GFxExporterInfoImpl ExporterInfo;

    // Applies our content to GFxMovieInfo.
    void GetMovieInfo(GFxMovieInfo *pinfo) const
    {        
        // Store header data.
        pinfo->Version          = Version;
        pinfo->Flags            = SWFFlags;
        pinfo->FPS              = FPS;
        pinfo->FrameCount       = FrameCount;
        pinfo->Width            = int(FrameRect.Width() / 20.0f + 0.5f);
        pinfo->Height           = int(FrameRect.Height() / 20.0f + 0.5f);
     
        const GFxExporterInfo* pexi = ExporterInfo.GetExporterInfo();
        if (pexi)
        {
            pinfo->ExporterVersion  = pexi->Version;
            pinfo->ExporterFlags    = pexi->ExportFlags;
        }
        else
        {
            pinfo->ExporterVersion  = 0;
            pinfo->ExporterFlags    = 0;
        }       
    }
};




class GFxLoaderImpl : public GRefCountBase<GFxLoaderImpl>, public GFxSharedState, public GFxLogBase<GFxLoaderImpl>
{
    // Shared loader state overidable in movies.
    GPtr<GFxSharedStateImpl>    pSharedState;

    // Weak library of resources created so far. This needs to be a weak library 
    // because GFxMovieRoot instances keep strong pointers to GFxLoaderImpl.
    // A strong version of this library exists in GFxLoaderImpl.
    GPtr<GFxResourceWeakLib>    pWeakResourceLib;
   
    // Implement GFxSharedState through delegation.
    virtual GFxSharedState* GetSharedImpl() const { return pSharedState; }


    // Helper used from CreateMovie_LoadState; looks up or registers GFxMovieDefImpl.
    static GFxMovieDefImpl* CreateMovieDefImpl(GFxLoadStates* pls,
                                               GFxMovieDataDef* pmd, UInt loadConstants,
                                               GFxMovieBindProcess** ppbindProcess);

public:

    // Creates a loader with new states.
    GFxLoaderImpl(GFxResourceLib *pownerLib = 0);
    // Creates a loader impl, copying state pointers from another loader.
    GFxLoaderImpl(GFxLoaderImpl* psource);

    virtual ~GFxLoaderImpl();    


    // *** Movie Loading

    bool            GetMovieInfo(const char *pfilename, GFxMovieInfo *pinfo,
                                 bool getTagCount, UInt loadLibConstants);    
    GFxMovieDef*    CreateMovie(const char* filename, UInt loadConstants);    

    // The actual creation function; called from CreateMovie.
    // This function uses an externally captured specified load state.   
    static GFxMovieDefImpl*  CreateMovie_LoadState(GFxLoadStates* pls,
                                                   const GFxURLBuilder::LocationInfo& loc,
                                                   UInt loadConstants);

    // Loading version used for look up / bind GFxMovieDataDef based on provided states.
    // Used to look up movies serving fonts from GFxFontProviderSWF.
    static GFxMovieDefImpl*  CreateMovie_LoadState(GFxLoadStates* pls,
                                                   GFxMovieDataDef* pmd,
                                                   UInt loadConstants);


    struct SWFProcessInfo
    {
        GFxStream           Stream;
        UInt32              FileStartPos;
        UInt32              FileEndPos;
        GFxMovieHeaderData  Header;      

        SWFProcessInfo()
            : Stream(0, 0, 0)
        { }

        // Processes and reads in a SWF file header and opens the GFxStream
        // If 0 is returned, there was an error and error message is already displayed
        bool    Initialize(GFile *pfile, GFxLoadProcess *pimpl, bool readingMsg =0);
    };


    typedef GFxLoader::FileFormatType FileFormatType;

    static FileFormatType   DetectFileFormat(GFile *pfile);

    // GFxLogBase support
    /*
    bool    IsVerboseParse() const              { return (VerboseOptions & VerboseParse) != 0; }
    bool    IsVerboseParseShape() const         { return (VerboseOptions & VerboseParseShape) != 0; }
    bool    IsVerboseParseMorphShape() const    { return (VerboseOptions & VerboseParseMorphShape) != 0; }
    bool    IsVerboseParseAction() const        { return (VerboseOptions & VerboseParseAction) != 0; }
    */


    // *** Tag Processing support

    typedef GFx_TagLoaderFunction LoaderFunction;

    // Looks up a tag loader.
    static bool    GetTagLoader(UInt tagType, LoaderFunction *plf)
    { 
        if (tagType < GFxTag_SWF_TagTableEnd)
            *plf = GFx_SWF_TagLoaderTable[tagType];
        else if ((tagType >= GFxTag_GFX_TagTableBegin) && (tagType < GFxTag_GFX_TagTableEnd))
            *plf = GFx_GFX_TagLoaderTable[tagType - GFxTag_GFX_TagTableBegin];                    
        else
            *plf = 0;
        return (*plf) ? 1 : 0;
    }

    bool    CheckTagLoader(UInt tagType) const 
    {
        LoaderFunction lf;
        return GetTagLoader(tagType, &lf);
    }


    GFxResourceWeakLib* GetWeakLib() const  { return pWeakResourceLib; }


    // Override - Image loading function used by loadMovie().
    static GFxImageResource*   LoadMovieImage(const char *purl, GFxImageLoader *ploder, GFxLog *plog);
    // Create a filler image that will be displayed in place of loadMovie() user images.
    static GImageInfoBase*     CreateStaticUserImage();

    /*
    void            ReportProgress(GFxMovieDef *pmovieDef, const GFxTagInfo& tagInfo, UInt bytesLoaded, UInt totalBytes)
    { if (pProgressFunc) pProgressFunc(pmovieDef, tagInfo, bytesLoaded, totalBytes, pProgressContextData); }
    */
};


#endif // INC_GFXIMPL_H
