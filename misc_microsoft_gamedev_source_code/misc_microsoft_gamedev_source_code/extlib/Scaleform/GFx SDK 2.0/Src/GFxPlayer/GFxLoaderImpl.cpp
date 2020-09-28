/**********************************************************************

Filename    :   GFxLoaderImpl.cpp
Content     :   GFxPlayer loader implementation
Created     :   June 30, 2005
Authors     :   

Copyright   :   (c) 2001-2006 Scaleform Corp. All Rights Reserved.

Notes       :   

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#include "GFile.h"
#include "GZlibFile.h"
#include "GImage.h"

#include "GFxLoaderImpl.h"

#include "GFxLoadProcess.h"
#include "GFxImageResource.h"

#include "GFxLog.h"
#include "GFxTaskManager.h"

#include <string.h> // for memset
#include <float.h>

#ifdef GFC_USE_ZLIB
#include <zlib.h>
#endif

int StateAccessCount = 0;


// ***** GFxExporterInfoImpl Loading

// Assigns data
void    GFxExporterInfoImpl::SetData(UInt16 version, GFxLoader::FileFormatType format,
                                     const char* pname, const char* pprefix, UInt flags)
{
    SI.Version  = version;
    SI.Format   = format;
    Prefix      = (pprefix) ? pprefix : "";
    SWFName     = (pname) ? pname : "";
    SI.pSWFName = SWFName.ToCStr(); // Update SI pointers.
    SI.pPrefix  = Prefix.ToCStr();
    SI.ExportFlags = flags;
}

void    GFxExporterInfoImpl::ReadExporterInfoTag(GFxStream *pin, GFxTagType tagType)
{
    GUNUSED(tagType);
    GASSERT(tagType == GFxTag_ExporterInfo);

    // Utilizes the tag 1000 (unused in normal SWF): the format is as follows:
    // Header           RECORDHEADER    1000
    // Version          UI16            Version (1.10 will be encoded as 0x10A)
    // Flags            UI32            Version 1.10 (0x10A) and above - flags
    //      Bit0 - Contains glyphs' textures info (tags 1002)
    //      Bit1 - Glyphs are stripped from DefineFont tags
    //      Bit2 - Indicates gradients' images were exported
    // BitmapsFormat    UI16            1 - TGA
    //                                  2 - DDS
    // PrefixLen        UI8
    // Prefix           UI8[PrefixLen]
    // SwfNameLen       UI8
    // SwfName          UI8[SwfNameLen]

    UInt32  flags = 0;
    UInt16  version  = pin->ReadU16(); // read version
    if (version >= 0x10A)
        flags = pin->ReadU32();
    UInt16  bitmapFormat = pin->ReadU16();
    char*   pfx   = pin->ReadStringWithLength();
    char*   pswf  = pin->ReadStringWithLength();

    pin->LogParse("  ExportInfo: tagType = %d, tool ver = %d.%d, imgfmt = %d, prefix = '%s', swfname = '%s', flags = 0x%X\n",
        tagType,
        (version>>8), (version&0xFF),
        bitmapFormat,
        pfx,
        pswf,
        flags);
    
    SetData(version, (GFxLoader::FileFormatType)bitmapFormat, pswf, pfx, flags);    

    GFREE(pfx);
    GFREE(pswf);

}
 



// ***** GFxLoaderImpl - loader implementation


GFxLoaderImpl::GFxLoaderImpl(GFxResourceLib* plib)
{
    if (plib)
        pWeakResourceLib = plib->GetWeakLib();

    if (pSharedState = *new GFxSharedStateImpl(0))
    {
        pSharedState->SetLog(GPtr<GFxLog>(*new GFxLog));
        pSharedState->SetImageCreator(GPtr<GFxImageCreator>(*new GFxImageCreator));

        // By default there should be no glyph packer
        pSharedState->SetFontPackParams(0);
        //pSharedState->SetFontPackParams(GPtr<GFxFontPackParams>(*new GFxFontPackParams));

        // It's mandatory to have the cache manager for text rendering to work,
        // even if the dynamic cache isn't used. 
        pSharedState->SetFontCacheManager(GPtr<GFxFontCacheManager>(*new GFxFontCacheManager(true)));
    }
}

GFxLoaderImpl::GFxLoaderImpl(GFxLoaderImpl* psource)
    : pWeakResourceLib(psource->pWeakResourceLib)
{  
    if (pSharedState = *new GFxSharedStateImpl(0))
    {
        if (psource->pSharedState)
            pSharedState->CopyStatesFrom(psource->pSharedState);
        else
        {
            pSharedState->SetLog(GPtr<GFxLog>(*new GFxLog));

            // By default there should be no glyph packer
            pSharedState->SetFontPackParams(0);
            //pSharedState->SetFontPackParams(GPtr<GFxFontPackParams>(*new GFxFontPackParams));

            // It's mandatory to have the cache manager for text rendering to work,
            // even if the dynamic cache isn't used. 
            pSharedState->SetFontCacheManager(GPtr<GFxFontCacheManager>(*new GFxFontCacheManager(true)));
        }      
    }
}

GFxLoaderImpl::~GFxLoaderImpl()
{
}



// Obtains information about SWF file and checks for its availability.
// Return 1 if the info was obtained successfully (or was null, but SWF file existed),
// or 0 if it did not exist. Pass LoadCheckLibrary if the library should be checked before loading the file.
// Specifying LoadFromLibraryOnly can be used to check for presence of the file in the library.
bool    GFxLoaderImpl::GetMovieInfo(const char *pfilename, GFxMovieInfo *pinfo,
                                    bool getTagCount, UInt loadConstants)
{    
    if (!pinfo)
    {
        GFC_DEBUG_WARNING(1, "GFxLoader::GetMovieInfo failed, pinfo argument is null");
        return 0;
    }
    pinfo->Clear();
    
    // LOCK
    // Capture loading states/variables used during loading.
    GPtr<GFxLoadStates>  pls = *new GFxLoadStates(this);
    // UNLOCK

    if (!pls->GetLib())
    {
        GFC_DEBUG_WARNING(1, "GFxLoader::GetMovieInfo failed, ResourceLibrary does not exist");
        return 0;
    }

    
    // Translate the filename.
    GFxURLBuilder::LocationInfo loc(GFxURLBuilder::File_Regular, pfilename);
    GFxString                   fileName;
    pls->BuildURL(&fileName, loc);
   
    // Use the MovieDataDef version already in the library if necessary.
    GPtr<GFxResource>    pmovieDataResource;

//    if (loadConstants & GFxLoader::LoadCheckLibrary)
    {
        // Image creator is only used as a key if it is bound to, based on flags.    
        GFxImageCreator* pkeyImageCreator = pls->GetLoadTimeImageCreator(loadConstants);

        GFxFileOpener *pfileOpener = pls->GetFileOpener();
        SInt64         modifyTime  = pfileOpener ?
            pfileOpener->GetFileModifyTime(fileName.ToCStr()) : 0;

        GFxResourceKey fileDataKey = 
           GFxMovieDataDef::CreateMovieFileKey(fileName.ToCStr(), modifyTime,
                                               pfileOpener, pkeyImageCreator);
        pmovieDataResource = *pls->GetLib()->GetResource(fileDataKey);
    }

    if (pmovieDataResource)
    {
        // Fetch the data from GFxMovieDataDef.
        GFxMovieDataDef* pmd = (GFxMovieDataDef*)pmovieDataResource.GetPtr();
        pmd->GetMovieInfo(pinfo);

        if (getTagCount)
        {
            // TBD: This may have to block for MovieDef to load.
            pinfo->TagCount = pmd->GetTagCount();
        }
    }
    else
    {
        // Open the file; this will automatically do the logging on error.
        GPtr<GFile> pin = *pls->OpenFile(fileName.ToCStr());
        if (!pin)        
            return 0;        

        GPtr<GFxLoadProcess> plp = *new GFxLoadProcess(pls, loadConstants);
        if (!plp)
            return 0;
        
        // Open and real file header.
        if (!plp->Initialize(pin))
            return 0;        
        GFxLoaderImpl::SWFProcessInfo &pi = plp->GetProcessInfo();
        
        // Store header data.
        pi.Header.GetMovieInfo(pinfo);

        if (getTagCount)
        {
            // Count tags.
            // pinfo->TagCount starts out at 0 after Clear
            while ((UInt32) pi.Stream.Tell() < pi.FileEndPos)
            {
                pi.Stream.OpenTag();
                pi.Stream.CloseTag();
                pinfo->TagCount++;
            }
        }

        // Done; file will be closed by destructor.
    }   
       
    return 1;
}



GFxMovieDef* GFxLoaderImpl::CreateMovie(const char* pfilename, UInt loadConstants)
{
    // LOCK

    // Capture loading states/variables used during loading.
    GPtr<GFxLoadStates>  pls = *new GFxLoadStates(this);
    
    // UNLOCK

    if (!pls->GetLib())
    {
        GFC_DEBUG_WARNING(1, "GFxLoader::CreateMovie failed, ResourceLibrary does not exist");
        return 0;
    }

    GFxURLBuilder::LocationInfo loc(GFxURLBuilder::File_Regular, pfilename);
    return CreateMovie_LoadState(pls, loc, loadConstants);
}




// Loading tasks.
class GFxMovieDataLoadTask : public GFxTask
{
    // TODO: Replace this with equivalent of a weak pointer,
    // so that if GFxMovieDataDef is released, loading does not
    // need to complete and can instead stop in the middle.
    // We may need to add some threaded hooks to GFxMovieDataDef for this.
    GPtr<GFxMovieDataDef>       pDef;

    GPtr<GFxLoadProcess>        pLoadProcess;
    // BindProcess is optional - specified only if we are doing
    // simultaneous binding and loading.
    GPtr<GFxMovieBindProcess>   pBindProcess;
    
public:
    GFxMovieDataLoadTask(GFxMovieDataDef *pdef,
                         GFxLoadProcess *plp,
                         GFxMovieBindProcess* pbp)        
        : GFxTask(Id_MovieDataLoad),
          pDef(pdef), pLoadProcess(plp), pBindProcess(pbp)
    { }

    ~GFxMovieDataLoadTask()
    {
    }

    
    virtual void    Execute()
    {
        // Do the loading.
        pDef->Read(pLoadProcess, pBindProcess);
    }

    virtual void    Abandon()
    {        
        // TODO: Mark movie as canceled, so that it knows that
        // it will not proceed here.
    }
};



// Loading of image into GFxMovieDataDef.
class GFxMovieImageLoadTask : public GFxTask
{
    // TODO: Replace this with equivalent of a weak pointer,
    // same as in GFxMovieDataLoadTask above.
    GPtr<GFxMovieDataDef>       pDef;

    // Data required for image loading.
    GFxString                   FileName;
    GPtr<GFile>                 pImageFile;
    GFxLoader::FileFormatType   ImageFormat;
    GPtr<GFxImageCreator>       pImageCreator;
    GPtr<GFxRenderConfig>       pRenderConfig;

    // This stores the result of loading: Image Resource.
    GPtr<GFxImageResource>      pImageRes;

public:
    GFxMovieImageLoadTask(
                GFxMovieDataDef *pdef,
                const GFxString& fileName, GFile *pin, GFxLoader::FileFormatType format,
                GFxImageCreator *pimageCreator, GFxRenderConfig *prconfig)
        : GFxTask(Id_MovieImageLoad),
          pDef(pdef), FileName(fileName), pImageFile(pin), ImageFormat(format),
          pImageCreator(pimageCreator), pRenderConfig(prconfig)
    { }


    virtual void    Execute()
    {
        // Image data loading: could be separated into a different task later on.
        // For now, read the data.
        GPtr<GImage>           pimage;
        GPtr<GImageInfoBase>   pimageBase;        

        pimage = *GFxImageCreator::LoadBuiltinImage(pImageFile, ImageFormat,
                                                    GFxResource::Use_Bitmap);

        if (pimage)
        {
            // Use creator for image
            GFxImageCreateInfo ico(pimage, GFxResource::Use_None, pRenderConfig);
            pimageBase = *pImageCreator->CreateImage(ico);

            if (pimageBase)
            {
                pImageRes = *new GFxImageResource(pimageBase, GFxResource::Use_Bitmap);
            }
        }

        if (pImageRes)
            pDef->InitImageFileMovieDef(FileName, pImageFile->GetLength(), pImageRes);
        else // Error
        {
            // This used to release DefImpl in case of failure.
            //if (pm) pm->Release();
            //return 0;
        }        
    }

    virtual void    Abandon()
    {        
        // TODO: Mark movie as canceled, so that it knows that
        // it will not proceed here.
    }

    bool    LoadingSucceeded() const   { return pImageRes.GetPtr() != 0; }
};


class GFxMovieBindTask : public GFxTask
{
    // Movie bind process is the only thing of interest here.
    // TBD: We might still want to detect MovieDefs / MovieImpls 
    // being released here...
    GPtr<GFxMovieBindProcess>   pBindProcess;

public:
    GFxMovieBindTask(GFxMovieBindProcess* pbp)        
        : GFxTask(Id_MovieBind),
          pBindProcess(pbp)
    { }

    ~GFxMovieBindTask()
    {
    }

    virtual void    Execute()
    {
        // Do the binding.
        while(pBindProcess->BindNextFrame() == GFxMovieBindProcess::Bind_InProgress)
            ;
        pBindProcess->FinishBinding();
    }

    virtual void    Abandon()
    {        
        // TODO: 
    }
};



// Static: The actual creation function; called from CreateMovie.
GFxMovieDefImpl* GFxLoaderImpl::CreateMovie_LoadState(GFxLoadStates* pls,
                                                      const GFxURLBuilder::LocationInfo& loc,
                                                      UInt loadConstants)
{
    // Need to check for:
    //  if (loadConstants & GFxLoader::LoadCheckLibrary)
    //  if (loadConstants & GFxLoader::LoadFromLibraryOnly)
    //  if (loadConstants & GFxLoader::LoadUpdateLibrary) 

    // Translate the filename.
    GFxString fileName;
    pls->BuildURL(&fileName, loc);
   
    
    // *** Check Library and Initiate Loading

    GFxResourceLib::BindHandle	    bh;    
    GPtr<GFxMovieDataDef>	        pmd;
    GFxMovieDefImpl*			    pm = 0;
    GFxLog*                         plog  = pls->pLog;

    bool                            movieNeedsLoading = 0;    
    GFxMovieDataDef::MovieDataType  mtype  = GFxMovieDataDef::MT_Empty;
    GPtr<GFxMovieBindProcess>       pbp;
    GPtr<GFxLoadProcess>            plp;
    GPtr<GFile>                     pin;
    FileFormatType                  format = GFxLoader::File_Unopened;

    // We integrate optional ImageCreator for loading, with hash matching
    // dependent on GFxImageLoader::IsKeepingImageData and LoadKeepBindData flag.
    // Image creator is only used as a key if it is bound to, based on flags.    
    GFxImageCreator*                 pkeyImageCreator = pls->GetLoadTimeImageCreator(loadConstants);
    GFxFileOpener*                   pfileOpener = pls->GetFileOpener();
    UInt64                           modifyTime  = pfileOpener ?
                                        pfileOpener->GetFileModifyTime(fileName.ToCStr()) : 0;
    
    GFxResourceKey fileDataKey = 
        GFxMovieDataDef::CreateMovieFileKey(fileName.ToCStr(), modifyTime,
                                            pfileOpener, pkeyImageCreator);


    if (pls->GetLib()->BindResourceKey(&bh, fileDataKey) ==
        GFxResourceLib::RS_NeedsResolve)
    {        
        // Open the file; this will automatically do the logging on error.
        pin = *pls->OpenFile(fileName.ToCStr());
        if (!pin) 
        {
            // TBD: Should'nt we transfer this OpenFile's error message to our 
            // waiters, if any? That way both threads would report the same message.
            // For now, just create a string.
            GFxString s = GFxString("GFxLoader failed to open \"") + fileName + "\"\n";
            bh.CancelResolve(s.ToCStr());
            return 0;
        }

        
        // Detect file format so that we can determine whether we can
        // and/or allowed to support it. Images can be loaded directly
        // into GFxMovieDef files, but their loading logic is custom.        
        format = DetectFileFormat(pin);

        switch(format)
        {      
        case GFxLoader::File_SWF:
            if (loadConstants & GFxLoader::LoadDisableSWF)
            {                
                GFxString s =  GFxString("Error loading SWF file \"") + fileName + 
                               "\" - GFX file format expected\n";
                plog->LogError(s.ToCStr());
                bh.CancelResolve(s.ToCStr());
                return 0;
            }
            // Fall through to Flash file loading
        case GFxLoader::File_GFX:
            mtype = GFxMovieDataDef::MT_Flash;
            break;

        // Image file formats support.
        case GFxLoader::File_JPEG:
        case GFxLoader::File_DDS:
        case GFxLoader::File_TGA:
        case GFxLoader::File_PNG:
            // If image file format loading is enabled proceed to do so.
            if (loadConstants & GFxLoader::LoadImageFiles)
            {
                mtype = GFxMovieDataDef::MT_Image;
                break;
            }

        case GFxLoader::File_Unopened:
            // Unopened should not occur due to the check above.
        case GFxLoader::File_Unknown:
        default:
            {            
                GFxString s =  GFxString("Unknown file format at URL \"") +
                               fileName + "\"\n";
                plog->LogError(s.ToCStr());
                bh.CancelResolve(s.ToCStr());
            }            
            return 0;
        }
            

        if (mtype == GFxMovieDataDef::MT_Flash)
        {            
            plp = *new GFxLoadProcess(pls, loadConstants);

            // Read in header.
            // Not that this also reads the export tags,
            // so no extra pre-loading will be necessary in GFxMovieDef.
            if (!plp->Initialize(pin, 1))
            {
                GFxString s =  GFxString("Failed to load SWF file \"") +
                               fileName + "\"\n";
                bh.CancelResolve(s.ToCStr());
                return 0;
            }
            pmd = *new GFxMovieDataDef(plp->GetProcessInfo(), fileDataKey);

            // SetDataDef to load states so that Read/Bind can proceed.            
            if (pmd)
                plp->SetDataDef(pmd);
        }
        else
        {
            // Image movie def
            pmd = *new GFxMovieDataDef(fileDataKey);

            // For images we also need to set DataDef because it is stored
            // as a part of GFxMovieDefBindStates, which is used as a library key.
            if (pmd)
                pls->SetDataDef(pmd);
        }        

        // Need to read header first.
        if (pmd)
        {            
            if (!(loadConstants & GFxLoader::LoadOrdered))
            {
                // If we are doing ordered loading, create the bound movie entry immediately,
                // to ensure that we don't have another thread start binding before us.
                pm = CreateMovieDefImpl(pls, pmd, loadConstants,
                        (mtype == GFxMovieDataDef::MT_Flash) ? &pbp.GetRawRef() : 0);
            }

            bh.ResolveResource(pmd.GetPtr());
        }
        else
        {
            GFxString s =  GFxString("Failed to load SWF file \"") + fileName + "\"\n";
            bh.CancelResolve(s.ToCStr());
            return 0;
        }

        movieNeedsLoading = 1;
    }
    else
    {
        // If Available and Waiting resources will be resolved here.
        if ((pmd = *(GFxMovieDataDef*)bh.WaitForResolve()).GetPtr() == 0)
        {
            // Error occurred during loading.
            pls->pLog->LogError("Error: %s", bh.GetResolveError());
            return 0;
        }

        mtype = pmd->MovieType;

        // SetDataDef to load states so that GFxMovieDefImpl::Bind can proceed.
        pls->SetDataDef(pmd);
        // May need to wait for movieDefData to become available.
    }


    // *** Check the library for MovieDefImpl and Initiate Binding    
    
    // Do a check because for Ordered loading this might have been
    // done above to avoid data race.
    if (!movieNeedsLoading || (loadConstants & GFxLoader::LoadOrdered))
        pm = CreateMovieDefImpl(pls, pmd, loadConstants,
                (mtype == GFxMovieDataDef::MT_Flash) ? &pbp.GetRawRef() : 0);
    if (!pm)
        return 0;


    // *** Do Loading

    if (movieNeedsLoading)
    {
        if (mtype == GFxMovieDataDef::MT_Flash)
        { 
            // Set 'ploadBind' if we are going to do interleaved binding
            // simultaneously with loading. LoadOrdered means that binding
            // will be done separately - in that case Read is don with no binding.
            GFxMovieBindProcess* ploadBind =
                (loadConstants & GFxLoader::LoadOrdered) ? 0 : pbp.GetPtr();

            GPtr<GFxMovieDataLoadTask> ptask = *new GFxMovieDataLoadTask(pmd, plp, ploadBind);

            // If we have task manager, queue up loading task for execution,
            // otherwise just run it immediately.
            if (pls->GetTaskManager())
                pls->GetTaskManager()->AddTask(ptask);
            else
                ptask->Execute();

            if (ploadBind)
            {
                // If bind process was performed as part of the load task,
                // we no longer need it.
                pbp = 0;
            }

            plp = 0;
            pin = 0;
        }
        else
        {
            GPtr<GFxMovieImageLoadTask> ptask = 
                *new GFxMovieImageLoadTask(pmd, fileName, pin, format,
                                           pls->GetBindStates()->pImageCreator,
                                           pls->GetRenderConfig());

            if (pls->GetTaskManager())
                pls->GetTaskManager()->AddTask(ptask);
            else
            {
                ptask->Execute();

                if (!ptask->LoadingSucceeded())
                {
                    if (pm) pm->Release();
                    return 0;
                }

                // TODO: Will need to do a similar check for threaded tasks in
                // the future. Potentially, we'll have to only do this later if
                // wait for frame flag is specified.

                // Must set Binding Frame so that ASSERT is not hit during Advance.
                pm->BindingFrame = pmd->GetLoadingFrame();
                pm->BytesLoaded  = pmd->GetFileBytes();
            }
        }
    }


    // It we still need binding, perform it.
    if (pbp)
    {   
        GPtr<GFxMovieBindTask> ptask = *new GFxMovieBindTask(pbp);

        if (pls->GetTaskManager())
            pls->GetTaskManager()->AddTask(ptask);
        else        
            ptask->Execute();        
    }


    // Note that if loading failed in the middle we may return a partially loaded object.
    // This is normal because (a) loading can technically take place in a different thread
    // so it is not yet known if it will finish successfully and (2) Flash can actually
    // play unfinished files, even if the error-ed in a middle.

    // The exception to above are wait flags, however.
    if (loadConstants & (GFxLoader::LoadWaitCompletion|GFxLoader::LoadWaitFrame1))
    {
        // Nothing to do now, but will do waits here if threaded.
        // Under threaded situation the semantic of LoadWaitCompletion might
        // actually be to do loading on 'this' thread without actually queuing
        // a task.

        // We might also want to have a flag that would control whether WaitCompletion
        // fails the load on partial load, or returns an object partially loaded
        // similar to the standard behavior above.
    }
    
    return pm;
}


// Looks up or registers GFxMovieDefImpl, separated so that both versions 
// of loading can share implementation. Fills is pbindProcess pointer if
// the later is provided (not necessary for image file MovieDefImpl objects).
GFxMovieDefImpl* GFxLoaderImpl::CreateMovieDefImpl(GFxLoadStates* pls,
                                           GFxMovieDataDef* pmd,
                                           UInt loadConstants,
                                           GFxMovieBindProcess** ppbindProcess)
{        
    GFxResourceLib::BindHandle	    bh;    
    GFxMovieDefImpl*			    pm = 0;

    // Create an Impl key and see if it can be resolved.
    GFxMovieDefBindStates*      pbindStates   = pls->GetBindStates();
    GFxResourceKey              movieImplKey  = GFxMovieDefImpl::CreateMovieKey(pbindStates);
    GPtr<GFxMovieBindProcess>   pbp;

    if (pls->GetLib()->BindResourceKey(&bh, movieImplKey) ==
        GFxResourceLib::RS_NeedsResolve)
    {
        // Create a new MovieDefImpl
        // We pass GetSharedImpl() from loader so that it is used for delegation 
        // when accessing non-binding states such as log and renderer.
        pm = new GFxMovieDefImpl(pbindStates, pls->pLoaderImpl, loadConstants,
                                 pls->pLoaderImpl->pSharedState);

        if (ppbindProcess)
        {
            // Only create bind process for Flash movies, not images.
            *ppbindProcess = new GFxMovieBindProcess(pls, pm);
            if (!*ppbindProcess && pm)
            {
                pm->Release();
                pm = 0;
            }
        }

        // Need to read header first.
        if (pm)
            bh.ResolveResource(pm);
        else
        {
            GFxString s = GFxString("Failed to bind SWF file \"") + pmd->GetFileURL() + "\"\n";
            bh.CancelResolve(s.ToCStr());
            return 0;
        }

    }
    else
    {
        // If Available and Waiting resources will be resolved here.
        // Note: Returned value is AddRefed for us, so we don't need to do so.
        if ((pm = (GFxMovieDefImpl*)bh.WaitForResolve()) == 0)
        {
            // Error occurred during loading.
            pls->pLog->LogError("Error: %s", bh.GetResolveError());
            return 0;
        }
    }

    return pm;
}


// Loading version used for look up / bind GFxMovieDataDef based on provided states.
// Used to look up movies serving fonts from GFxFontProviderSWF.
GFxMovieDefImpl*  GFxLoaderImpl::CreateMovie_LoadState(GFxLoadStates* pls,
                                                       GFxMovieDataDef* pmd,
                                                       UInt loadConstants)
{
    if (pmd)
        pls->SetDataDef(pmd);

    GFxResourceLib::BindHandle	    bh;
    GPtr<GFxMovieBindProcess>       pbp;
    GFxMovieDefImpl*			    pm = CreateMovieDefImpl(pls, pmd, loadConstants,
                                                            &pbp.GetRawRef());
    
    // It we need binding, perform it.
    if (pbp)
    {   
        GPtr<GFxMovieBindTask> ptask = *new GFxMovieBindTask(pbp);

        if (pls->GetTaskManager())
            pls->GetTaskManager()->AddTask(ptask);
        else
            ptask->Execute();
    }

    // TBD: Not sure that this wait will be necessary for bound loading.
    if (loadConstants & (GFxLoader::LoadWaitCompletion|GFxLoader::LoadWaitFrame1))
    {       
    }

    return pm;
}




// *** File format detection logic.

GFxLoader::FileFormatType GFxLoaderImpl::DetectFileFormat(GFile *pfile)
{   
    if (!pfile)
        return GFxLoader::File_Unopened;

    SInt            pos     = pfile->Tell();
    FileFormatType  format  = GFxLoader::File_Unknown;
    UByte           buffer[4] = {0,0,0,0};

    if (pfile->Read(buffer, 4) <= 0)
        return GFxLoader::File_Unknown;

    switch(buffer[0])
    {
    case 0x43:
    case 0x46:    
        if ((buffer[1] == 0x57) && (buffer[2] == 0x53))
            format = GFxLoader::File_SWF;
        else if ((buffer[1] == 0x46) && (buffer[2] == 0x58))
            format = GFxLoader::File_GFX;
        break;    

    case 0xFF:
        if (buffer[1] == 0xD8)
            format = GFxLoader::File_JPEG;
        break;

    case 0x89:
        if ((buffer[1] == 'P') && (buffer[2] == 'N') && (buffer[3] == 'G'))
            format = GFxLoader::File_PNG;
        break;

    case 'G':
        if ((buffer[1] == 'I') && (buffer[2] == 'F') && (buffer[3] == '8'))
            format = GFxLoader::File_GIF;
        // 'GFX' also starts with a G.
        if ((buffer[1] == 0x46) && (buffer[2] == 0x58))
            format = GFxLoader::File_GFX;
        break;
    case 'D': // check is it DDS
        if ((buffer[1] == 'D') && (buffer[2] == 'S'))
            format = GFxLoader::File_DDS;
        break;
    }

    pfile->Seek(pos);
    if (format == GFxLoader::File_Unknown)
    {
        // check for extension. TGA format is hard to detect, that is why
        // we use extension test.
        const char* ppath = pfile->GetFilePath();
        if (ppath)
        {
            // look for the last '.'
            const char* pstr = strrchr(ppath, '.');
            if (GFxString::CompareNoCase(pstr, ".tga") == 0)
                format = GFxLoader::File_TGA;
        }
    }
    return format;
}



// Processes and reads in a SWF file header and opens the GFxStream
// If 0 is returned, there was an error and error message is already displayed
bool    GFxLoaderImpl::SWFProcessInfo::Initialize(GFile *pin, GFxLoadProcess *plp, bool readingMsg)
{
    UInt32  header;
    bool    compressed;
    
    FileStartPos        = pin->Tell();
    header              = pin->ReadUInt32();
    Header.FileLength   = pin->ReadUInt32();
    FileEndPos          = FileStartPos + Header.FileLength;
    Header.Version      = (header >> 24) & 255;
    Header.SWFFlags     = 0;
    compressed          = (header & 255) == 'C';

    // Verify header
    if ( ((header & 0x0FFFFFF) != 0x00535746) && // FWS
         ((header & 0x0FFFFFF) != 0x00535743) && // CWS
         ((header & 0x0FFFFFF) != 0x00584647) && // GFX
         ((header & 0x0FFFFFF) != 0x00584643) )  // CFX
    {
        // ERROR
        plp->LogError("Error: GFxLoader read failed - file does not start with a SWF header\n");
        return 0;
    }
    if (((header >> 16) & 0xFF) == 'X')
        Header.SWFFlags |= GFxMovieInfo::SWF_Stripped;
    if (compressed)
        Header.SWFFlags |= GFxMovieInfo::SWF_Compressed;

    if (readingMsg)
        plp->LogParse("SWF File version = %d, File length = %d\n", Header.Version, Header.FileLength);

    // AddRef to file
    GPtr<GFile> pfileIn = pin;  
    if (compressed)
    {
#ifndef GFC_USE_ZLIB
        plp->LogError("Error: GFxLoader - unable to read compressed SWF data; GFC_USE_ZLIB not defined\n");
        return 0;
#else
        if (readingMsg)
            plp->LogParse("SWF file is compressed.\n");

        // Uncompress the input as we read it.
        pfileIn = *new GZLibFile(pfileIn);

        // Subtract the size of the 8-byte header, since
        // it's not included pin the compressed
        // GFxStream length.
        FileEndPos = Header.FileLength - 8;
#endif
    }

    // Initialize stream, this AddRefs to file
	Stream.Initialize(pfileIn, plp->GetLog(), plp->GetLoadStates()->pParseControl);

    // Read final data
    Stream.ReadRect(& Header.FrameRect);
    Header.FPS         = Stream.ReadU16() / 256.0f;
    Header.FrameCount  = Stream.ReadU16();
    
    // Read the exporter tag, which must be the first tag in the GFX file.
    // We require this tag to be included in the very beginning of file because:
    //  1. Some of its content is reported by GFxMovieInfo.
    //  2. Reporting it from cached MovieDataDef would require a
    //     'wait-for-load' in cases when most of the loading is done
    //     on another thread.

    if (Header.SWFFlags & GFxMovieInfo::SWF_Stripped)
    {
        if ((UInt32) Stream.Tell() < FileEndPos)
        {
            if (Stream.OpenTag() == GFxTag_ExporterInfo)
            {
                Header.ExporterInfo.ReadExporterInfoTag(&Stream, GFxTag_ExporterInfo);
                if ((Header.ExporterInfo.GetExporterInfo()->Version & (~0xFF)) != 0x200)
                {
                    // Only gfx from exporter 2.0 are supported
                    plp->LogError(
                        "Error: GFxLoader read failed - incompatible GFX file, version 2.x expected\n");
                    return 0;
                }
            }
            else
            {
                plp->LogError(
                    "Error: GFxLoader read failed - no ExporterInfo tag in GFX file header\n");
                return 0;
            }
            Stream.CloseTag();
        }
        // Do not seek back; we advance the tags by one, as appropriate.
     }

    return 1;
}
    



GFxImageResource* GFxLoaderImpl::LoadMovieImage(const char *purl,
                                                GFxImageLoader *ploader, GFxLog *plog)
{
    // LoadMovieImage function is only used to load images with 'img://' prefix.
    // This means that we don't cache it in the resourceLib and instead just
    // pass it along to the user (allowing them to refresh the data if necessary).

    GPtr<GImageInfoBase> pimage;
    if (ploader)
        pimage = *ploader->LoadImage(purl);
    
    if (!pimage)
    {
        plog->LogScriptWarning(
            "Could not load user image \"%s\" - GFxImageLoader failed or not specified\n", purl);
        pimage = *CreateStaticUserImage();
    }

    // With respect to image keys, we just use a unique key here.
    return pimage? new GFxImageResource(pimage) : 0;
}

// Create a filler image that will be displayed in place of loadMovie() user images.
GImageInfoBase* GFxLoaderImpl::CreateStaticUserImage()
{
    enum {
        StaticImgWidth      = 19,
        StaticImgHeight     = 12,
        StaticImageScale    = 3,
    };

    // Encodes 'img:' picture with color palette int the back.  
    static char pstaticImage[StaticImgWidth * StaticImgHeight + 1] =
        "aaaaaaaaaaaaaaaaaaa"
        "aaaaaaaaaaaaaaaaaaa"
        "rr#rrrrrrrrrrrrrrrr"
        "rrrrrrrrrrrrrrrr##r" 
        "g##gg##g#ggg###g##g"
        "gg#gg#g#g#g#gg#gggg"
        "bb#bb#b#b#b#bb#b##b"
        "bb#bb#b#b#bb###b##b"
        "y###y#y#y#yyyy#yyyy"
        "yyyyyyyyyyy###yyyyy"
        "ccccccccccccccccccc"
        "ccccccccccccccccccc"
        ;

    GPtr<GImage> pimage = *new GImage(GImage::Image_ARGB_8888,
        StaticImgWidth * StaticImageScale,
        StaticImgHeight * StaticImageScale);
    if (pimage)
    {
        for (int y=0; y<StaticImgHeight; y++)
            for (int x=0; x<StaticImgWidth; x++)
            {
                UInt32 color = 0;

                switch(pstaticImage[y * StaticImgWidth + x])
                {
                case '#': color = 0xFF000000; break;
                case 'r': color = 0x80FF2020; break;
                case 'g': color = 0x8020FF20; break;
                case 'b': color = 0x802020FF; break;
                case 'y': color = 0x80FFFF00; break;
                case 'a': color = 0x80FF00FF; break;
                case 'c': color = 0x8000FFFF; break;
                }

                for (int iy = 0; iy < StaticImageScale; iy++)
                    for (int ix = 0; ix < StaticImageScale; ix++)
                    {
                        pimage->SetPixelRGBA(x * StaticImageScale + ix,
                            y * StaticImageScale + iy, color);
                    }
            }
    }

    // Use GImageInfo directly without callback, since this is an alternative
    // to pImageLoadFunc, which is not the same as pImageCreateFunc.    
    return new GImageInfo(pimage);
}


