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
        (int)flags);
    
    SetData(version, (GFxLoader::FileFormatType)bitmapFormat, pswf, pfx, flags);    

    GFREE(pfx);
    GFREE(pswf);

}
 

// Processes and reads in a SWF file header and opens the GFxStream
// If 0 is returned, there was an error and error message is already displayed
bool    GFxSWFProcessInfo::Initialize(GFile *pin,GFxLog *plog,
                                      GFxParseControl* pparseControl, bool parseMsg)
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
        if (plog)
            plog->LogError("Error: GFxLoader read failed - file does not start with a SWF header\n");
        return 0;
    }
    if (((header >> 16) & 0xFF) == 'X')
        Header.SWFFlags |= GFxMovieInfo::SWF_Stripped;
    if (compressed)
        Header.SWFFlags |= GFxMovieInfo::SWF_Compressed;

    // Parse messages will not be generated if they are disabled by GFxParseControl.
    if (!plog || !pparseControl || !pparseControl->IsVerboseParse())
        parseMsg = false;

    if (parseMsg)
        plog->LogMessageByType(GFxLog::Log_Parse,
            "SWF File version = %d, File length = %d\n",
            Header.Version, Header.FileLength);

    // AddRef to file
    GPtr<GFile> pfileIn = pin;  
    if (compressed)
    {
#ifndef GFC_USE_ZLIB
        if (plog)
            plog->LogError("Error: GFxLoader - unable to read compressed SWF data; GFC_USE_ZLIB not defined\n");
        return 0;
#else
        if (parseMsg)
            plog->LogMessageByType(GFxLog::Log_Parse, "SWF file is compressed.\n");

        // Uncompress the input as we read it.
        pfileIn = *new GZLibFile(pfileIn);

        // Subtract the size of the 8-byte header, since
        // it's not included pin the compressed
        // GFxStream length.
        FileEndPos = Header.FileLength - 8;
#endif
    }

    // Initialize stream, this AddRefs to file
    Stream.Initialize(pfileIn, plog, pparseControl);

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
                    if (plog)
                        plog->LogError(
                            "Error: GFxLoader read failed - incompatible GFX file, version 2.x expected\n");
                    return 0;
                }
            }
            else
            {
                if (plog)
                    plog->LogError(
                        "Error: GFxLoader read failed - no ExporterInfo tag in GFX file header\n");
                return 0;
            }
            Stream.CloseTag();
        }
        // Do not seek back; we advance the tags by one, as appropriate.
    }

    return 1;
}

// ***** GFxLoaderTask - implementation

GFxLoaderTask::GFxLoaderTask(GFxLoadStates* pls, TaskId id)
: GFxTask(id), pLoadStates(pls)
{
    //printf("GFxLoaderTask::GFxLoaderTask : %x, thread : %d\n", this, GetCurrentThreadId());
    pLoadStates->pLoaderImpl->RegisterLoadProcess(this);
}
GFxLoaderTask::~GFxLoaderTask()
{
    //printf("GFxLoaderTask::~GFxLoaderTask : %x, thread : %d\n", this, GetCurrentThreadId());
    pLoadStates->pLoaderImpl->UnRegisterLoadProcess(this);
}

// ***** GFxLoaderImpl - loader implementation


GFxLoaderImpl::GFxLoaderImpl(GFxResourceLib* plib)
{
    SetRefCountMode(GFC_REFCOUNT_THREADSAFE);
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

        pSharedState->SetTextClipboard(GPtr<GFxTextClipboard>(*new GFxTextClipboard));
        pSharedState->SetTextKeyMap(GPtr<GFxTextKeyMap>(*(new GFxTextKeyMap)->InitWindowsKeyMap()));
    }
}

GFxLoaderImpl::GFxLoaderImpl(GFxLoaderImpl* psource)
    : pWeakResourceLib(psource->pWeakResourceLib)
{  
    SetRefCountMode(GFC_REFCOUNT_THREADSAFE);
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
    CancelLoading();
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
                                               pfileOpener, pkeyImageCreator,
                                               pls->GetPreprocessParams());
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
        
        // Open and real file header, failing if it doesn't match.
        GFxSWFProcessInfo pi;
        if (!pi.Initialize(pin, pls->GetLog(), pls->pParseControl))
            return 0;
        
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
    // If CreateMovie is started on a thread (not main thread) we need to set
    // threaded loading flag regardless of whether task manage is set or not
    if (loadConstants & GFxLoader::LoadOnThread)
        pls->ThreadedLoading = true;
    
    // UNLOCK

    if (!pls->GetLib())
    {
        GFC_DEBUG_WARNING(1, "GFxLoader::CreateMovie failed, ResourceLibrary does not exist");
        return 0;
    }

    GFxURLBuilder::LocationInfo loc(GFxURLBuilder::File_Regular, pfilename);
    return CreateMovie_LoadState(pls, loc, loadConstants);
}


void GFxLoaderImpl::RegisterLoadProcess(GFxLoaderTask* ptask)
{
    
    //GASSERT(pSharedState->GetTaskManager());
    GLock::Locker guard(&LoadProcessesLock);
    LoadProcesses.PushBack(new GFxLoadProcessNode(ptask));
}

void GFxLoaderImpl::UnRegisterLoadProcess(GFxLoaderTask* ptask)
{
    //GASSERT(pSharedState->GetTaskManager());
    GLock::Locker guard(&LoadProcessesLock);
    GFxLoadProcessNode* pnode = LoadProcesses.GetFirst();
    while (!LoadProcesses.IsNull(pnode))
    {
        if (pnode->pTask == ptask) 
        {
            LoadProcesses.Remove(pnode);
            //pSharedState->GetTaskManager()->AbandonTask(ptask);
            delete pnode;
            break;
        }
        pnode = pnode->pNext;
    }
}
void GFxLoaderImpl::CancelLoading()
{
    //printf("GFxLoaderTask::~CancelLoading ---  : %x, thread : %d\n", this, GetCurrentThreadId());
    GPtr<GFxTaskManager> ptm = pSharedState->GetTaskManager();
    if (!ptm)
        return;
    GLock::Locker guard(&LoadProcessesLock);
    GFxLoadProcessNode* pnode = LoadProcesses.GetFirst();
    while (!LoadProcesses.IsNull(pnode))
    {
        LoadProcesses.Remove(pnode);
        ptm->AbandonTask(pnode->pTask);
        delete pnode;
        pnode = LoadProcesses.GetFirst();
    }
}

// *** Loading tasks.

// Loading of image into GFxMovieDataDef.
class GFxMovieImageLoadTask : public GFxLoaderTask
{
    // TODO: Replace this with equivalent of a weak pointer,
    // same as in GFxMovieDataLoadTask above.
    GPtr<GFxMovieDataDef>       pDef;
    GPtr<GFxMovieDefImpl>       pDefImpl;

    // Data required for image loading.    
    GPtr<GFile>                 pImageFile;
    GFxLoader::FileFormatType   ImageFormat;

    //GPtr<GFxLoadStates>         pLoadStates;
    //GPtr<GFxImageCreator>       pImageCreator;
    //GPtr<GFxRenderConfig>       pRenderConfig;

    // This stores the result of loading: Image Resource.
    GPtr<GFxImageResource>      pImageRes;

public:
    GFxMovieImageLoadTask(
                GFxMovieDataDef *pdef, GFxMovieDefImpl *pdefImpl,
                GFile *pin, GFxLoader::FileFormatType format, GFxLoadStates* pls)
        : GFxLoaderTask(pls, Id_MovieImageLoad),
          pDef(pdef), pDefImpl(pdefImpl), pImageFile(pin), ImageFormat(format)//, pLoadStates(pls)
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
            GFxImageCreateInfo ico(pimage, GFxResource::Use_None, pLoadStates->GetRenderConfig());
            ico.ThreadedLoading = pLoadStates->IsThreadedLoading();
            pimageBase = *pLoadStates->GetBindStates()->pImageCreator->CreateImage(ico);

            if (pimageBase)            
                pImageRes = *new GFxImageResource(pimageBase, GFxResource::Use_Bitmap);
        }

        if (pImageRes)
        {
            pDef->InitImageFileMovieDef(pImageFile->GetLength(), pImageRes);

            // Notify GFxMovieDefImpl that binding is finished,
            // so that any threaded waiters can be released.
            UInt fileBytes = pDef->GetFileBytes();
            pDefImpl->pBindData->UpdateBindingFrame(pDef->GetLoadingFrame(), fileBytes);
            pDefImpl->pBindData->SetBindState(
                GFxMovieDefImpl::BS_Finished |
                GFxMovieDefImpl::BSF_Frame1Loaded | GFxMovieDefImpl::BSF_LastFrameLoaded);
        }
        else
        {   // Error            
            pDefImpl->pBindData->SetBindState(GFxMovieDefImpl::BS_Error);
        }
    }

    virtual void    OnAbandon(bool)
    {        
        // TODO: Mark movie as canceled, so that it knows that
        // it will not proceed here.
    }

    bool    LoadingSucceeded() const   { return pImageRes.GetPtr() != 0; }
};  





// Static: The actual creation function; called from CreateMovie.
GFxMovieDefImpl* GFxLoaderImpl::CreateMovie_LoadState(GFxLoadStates* pls,
                                                      const GFxURLBuilder::LocationInfo& loc,
                                                      UInt loadConstants, LoadStackItem* ploadStack)
{
    // Translate the filename.
    GFxString fileName;
    pls->BuildURL(&fileName, loc);
   
    
    // *** Check Library and Initiate Loading

    GFxResourceLib::BindHandle        bh;    
    GPtr<GFxMovieDataDef>            pmd;
    GFxMovieDefImpl*                pm = 0;
    GFxLog*                         plog  = pls->pLog;

    bool                            movieNeedsLoading = 0;    
    GFxMovieDataDef::MovieDataType  mtype  = GFxMovieDataDef::MT_Empty;
    GPtr<GFxMovieBindProcess>       pbp;
    GPtr<GFxLoadProcess>            plp;
    GPtr<GFile>                     pin;
    FileFormatType                  format = GFxLoader::File_Unopened;

    // Ordered loading means that all binding-dependent files (imports and images) will be
    // loaded after the main file. Technically, this disagrees with progressive loading,
    // although we could devise a better scheme in the future.
    // If 'Ordered' is not specified, loading is interleaved, meaning that imports
    // and dependencies get resolved while parent file hasn't yet finished loading.
    // ThreadedBinding implies interleaved loading, since the binding thread can
    // issue a dependency load request at any time.    
    bool    interleavedLoading = (loadConstants & GFxLoader::LoadThreadedBinding) ||
                                 !(loadConstants & GFxLoader::LoadOrdered);

    // Since Ordered loading prevents threaded binding from starting on time, warn.
    GFC_DEBUG_WARNING((loadConstants & (GFxLoader::LoadOrdered|GFxLoader::LoadThreadedBinding)) ==
                           (GFxLoader::LoadOrdered|GFxLoader::LoadThreadedBinding),
                      "GFxLoader::CreateMovie - LoadOrdered flag conflicts with GFxLoader::LoadThreadedBinding");

    // We integrate optional ImageCreator for loading, with hash matching
    // dependent on GFxImageLoader::IsKeepingImageData and LoadKeepBindData flag.
    // Image creator is only used as a key if it is bound to, based on flags.    
    GFxImageCreator*                 pkeyImageCreator = pls->GetLoadTimeImageCreator(loadConstants);
    GFxFileOpener*                   pfileOpener = pls->GetFileOpener();
    UInt64                           modifyTime  = pfileOpener ?
                                        pfileOpener->GetFileModifyTime(fileName.ToCStr()) : 0;
    
    GFxResourceKey fileDataKey = 
        GFxMovieDataDef::CreateMovieFileKey(fileName.ToCStr(), modifyTime,
                                            pfileOpener, pkeyImageCreator, pls->GetPreprocessParams());

    GFxResourceLib::ResolveState rs;

    if ((rs = pls->GetLib()->BindResourceKey(&bh, fileDataKey)) ==
        GFxResourceLib::RS_NeedsResolve)
    {        
        // Open the file; this will automatically do the logging on error.
        pin = *pls->OpenFile(fileName.ToCStr(), loadConstants);
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
                if (plog)
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
                if (plog)
                    plog->LogError(s.ToCStr());
                bh.CancelResolve(s.ToCStr());
            }            
            return 0;
        }


        // Create GFxMovieDataDef of appropriate type (Image or Flash)
        pmd = *new GFxMovieDataDef(fileDataKey, mtype, fileName.ToCStr());        

        //printf("Thr %4d, %8x : CreateMovie - constructed GFxMovieDataDef for '%s'\n", 
        //       GetCurrentThreadId(), pmd.GetPtr(), fileName.ToCStr());
                
        if (pmd)
        {
            // Assign movieDef's file path to LoadStates.
            pls->SetRelativePathForDataDef(pmd);

            // Create a loading process for Flash files and verify header.
            // For images, this is done later on.
            if (mtype == GFxMovieDataDef::MT_Flash)
            {  
                plp = *new GFxLoadProcess(pmd, pls, loadConstants);

                // Read in and verify header, initializing loading.
                // Note that this also reads the export tags,
                // so no extra pre-loading will be necessary in GFxMovieDef.
                if (!plp || !plp->BeginSWFLoading(pin))
                {
                    // Clear pmd, causing an error message and CancelResolve below.
                    plp = 0;
                    pmd = 0;
                }
            }
        }
              
        if (pmd)
        {    
            // For images we always create DefImpl before ResolveResource, so that
            // other threads don't try to bind us (no separate binding from images now).

            if  ((mtype != GFxMovieDataDef::MT_Flash) || interleavedLoading )
            {
                // If we are doing interleaved loading, create the bound movie entry immediately,
                // to ensure that we don't have another thread start binding before us.     
                pm = CreateMovieDefImpl(pls, pmd, loadConstants,
                        (mtype == GFxMovieDataDef::MT_Flash) ? &pbp.GetRawRef() : 0, true, ploadStack);
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
        /*
        if (rs == GFxResourceLib::RS_Available)        
            printf("Thr %4d, ________ : CreateMovie - '%s' is in library\n", GetCurrentThreadId(), fileName.ToCStr());        
        else        
            printf("Thr %4d, ________ : CreateMovie - waiting on '%s'\n", GetCurrentThreadId(), fileName.ToCStr());        
        */
        GUNUSED(rs);
        
        if ((pmd = *(GFxMovieDataDef*)bh.WaitForResolve()).GetPtr() == 0)
        {
            // Error occurred during loading.
            if (plog)
                plog->LogError("Error: %s", bh.GetResolveError());
            return 0;
        }

        mtype = pmd->MovieType;

        // SetDataDef to load states so that GFxMovieDefImpl::Bind can proceed.
        pls->SetRelativePathForDataDef(pmd);
        // May need to wait for movieDefData to become available.
    }


    // *** Check the library for MovieDefImpl and Initiate Binding    
    
    // Do a check because for Ordered loading this might have been
    // done above to avoid data race.
    if (!movieNeedsLoading || !interleavedLoading)
    {
        if (!pm)
        {            
            // For images this can grab an existing MovieDefImpl, but it will never
            // create one since it's taken care of before DataDef ResolveResource.

            pm = CreateMovieDefImpl(pls, pmd, loadConstants,
                    (mtype == GFxMovieDataDef::MT_Flash) ? &pbp.GetRawRef() : 0, false, ploadStack);
        }
    }
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
                (loadConstants & (GFxLoader::LoadOrdered|GFxLoader::LoadThreadedBinding)) ? 0 : pbp.GetPtr();
            if (ploadBind)
                plp->SetBindProcess(ploadBind);
          
            // If we have task manager, queue up loading task for execution,
            // otherwise just run it immediately.
            if (loadConstants & GFxLoader::LoadWaitCompletion || !pls->SubmitBackgroundTask(plp))
                plp->Execute();

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
                *new GFxMovieImageLoadTask(pmd, pm, pin, format, pls);

            if ((loadConstants & (GFxLoader::LoadWaitCompletion|GFxLoader::LoadOrdered)) 
                 || !pls->SubmitBackgroundTask(ptask) )
            {
                ptask->Execute();

                if (!ptask->LoadingSucceeded())
                {
                    if (pm) pm->Release();
                    return 0;
                }

                // NOTE: A similar check is done by the use of 'waitSuceeded'
                // flag below for threaded tasks.
            }
        }
    }


    // Run bind task on a MovieDefImpl and waits for completion, based on flags. 
    return BindMovieAndWait(pm, pbp, pls, loadConstants, ploadStack);
}



GFxMovieDefImpl* GFxLoaderImpl::BindMovieAndWait(GFxMovieDefImpl* pm, GFxMovieBindProcess* pbp,
                                                 GFxLoadStates* pls, UInt loadConstants, LoadStackItem* ploadStack)
{
    // It we still need binding, perform it.
    if (pbp)
    {   
        if (loadConstants & GFxLoader::LoadWaitCompletion || !pls->SubmitBackgroundTask(pbp))
            pbp->Execute();
    }

    // Note that if loading failed in the middle we may return a partially loaded object.
    // This is normal because (a) loading can technically take place in a different thread
    // so it is not yet known if it will finish successfully and (2) Flash can actually
    // play unfinished files, even if the error-ed in a middle.

    // The exception to above are wait flags, however.
    bool waitSuceeded = true;
    bool needWait = true;
    
    // Checking for recursion in the loading process.
    LoadStackItem* pstack = ploadStack;
    while(pstack)
    {
        if (pstack->pDefImpl == pm)
        {
            // Recursion is detected. 
            // Check if this is a self recursion 
            if(pstack->pNext)
            {
                // This is not a self recursion. We don't support this recursion type yet.
                // Stop loading and return error.
                waitSuceeded = false;
                if (pls->GetLog())
                {
                    char buffer[512] = "";
                    char buf[128] = "";
                    while(ploadStack)
                    {
                        gfc_sprintf(buf, sizeof (buf), "%s\n", ploadStack->pDefImpl->GetFileURL());
                        gfc_strcat(buffer,sizeof(buffer), buf);
                        ploadStack = ploadStack->pNext;
                    }
                    gfc_sprintf(buf, sizeof (buf), "%s\n", pm->GetFileURL());
                    gfc_strcat(buffer,sizeof(buffer), buf);
                    pls->GetLog()->LogError("Error: Recursive import detected. Import stack:\n%s", buffer);
                }
            }
            // We must not wait on a waitcondition which will never be set.
            needWait = false;
            break;
        }
        pstack = pstack->pNext;
    }
    if (needWait && (loadConstants & GFxLoader::LoadWaitCompletion))
    {
        // TBD: Under threaded situation the semantic of LoadWaitCompletion might
        // actually be to do loading on 'this' thread without actually queuing
        // a task.

        // We might also want to have a flag that would control whether WaitCompletion
        // fails the load on partial load, or returns an object partially loaded
        // similar to the standard behavior above.
        waitSuceeded = pm->WaitForBindStateFlags(GFxMovieDefImpl::BSF_LastFrameLoaded);
    }
    else if (needWait && (loadConstants & GFxLoader::LoadWaitFrame1))
    {
        waitSuceeded = pm->WaitForBindStateFlags(GFxMovieDefImpl::BSF_Frame1Loaded);
    }

    // waitSuceeded would only be 'false' in case of error.
    if (!waitSuceeded)
    {
        pm->Release();
        pm = 0;
    }
    return pm;
}


// Looks up or registers GFxMovieDefImpl, separated so that both versions 
// of loading can share implementation. Fills is pbindProcess pointer if
// the later is provided (not necessary for image file MovieDefImpl objects).
GFxMovieDefImpl* GFxLoaderImpl::CreateMovieDefImpl(GFxLoadStates* pls,
                                           GFxMovieDataDef* pmd,
                                           UInt loadConstants,
                                           GFxMovieBindProcess** ppbindProcess,
                                           bool checkCreate, LoadStackItem* ploadStack = NULL)
{        
    GFxResourceLib::BindHandle        bh;    
    GFxMovieDefImpl*                pm = 0;

    // Create an Impl key and see if it can be resolved.
    GFxMovieDefBindStates*      pbindStates   = pls->GetBindStates();
    GFxResourceKey              movieImplKey  = GFxMovieDefImpl::CreateMovieKey(pmd, pbindStates);
    GPtr<GFxMovieBindProcess>   pbp;
    GFxResourceLib::ResolveState rs;

    if ((rs = pls->GetLib()->BindResourceKey(&bh, movieImplKey)) ==
        GFxResourceLib::RS_NeedsResolve)
    {
        // Create a new MovieDefImpl
        // We pass GetSharedImpl() from loader so that it is used for delegation 
        // when accessing non-binding states such as log and renderer.
        pm = new GFxMovieDefImpl(pmd, pbindStates, pls->pLoaderImpl, loadConstants,
                                 pls->pLoaderImpl->pSharedState);

         //printf("Thr %4d, %8x :  CreateMovieDefImpl - GFxMovieDefImpl constructed for %8x\n",
         //       GetCurrentThreadId(), pm, pmd);

        if (ppbindProcess)
        {
            // Only create bind process for Flash movies, not images.
            *ppbindProcess = new GFxMovieBindProcess(pls, pm, ploadStack);
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
        GASSERT(!checkCreate);
        GUNUSED(checkCreate);

        /*
        if (rs == GFxResourceLib::RS_Available)
        {
            printf("Thr %4d, ________ :  CreateMovieDefImpl - Impl for %8x is in library\n",
                GetCurrentThreadId(), pmd);
        }
        else
        {
            printf("Thr %4d, ________ :  CreateMovieDefImpl - waiting GFxMovieDefImpl for %8x\n",
                GetCurrentThreadId(), pmd);
        }
        */
        GUNUSED(rs);

        
        // If Available and Waiting resources will be resolved here.
        // Note: Returned value is AddRefed for us, so we don't need to do so.
        if ((pm = (GFxMovieDefImpl*)bh.WaitForResolve()) == 0)
        {
            // Error occurred during loading.
            if (pls->pLog)
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
        pls->SetRelativePathForDataDef(pmd);

    GFxResourceLib::BindHandle        bh;
    GPtr<GFxMovieBindProcess>       pbp;
    GFxMovieDefImpl*                pm = CreateMovieDefImpl(pls, pmd, loadConstants,
                                                            &pbp.GetRawRef(),false);

    if (!pm) return 0;
    
    // It we need binding, perform it.
    return BindMovieAndWait(pm, pbp, pls, loadConstants);
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
        if (plog)
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




// This table is a compilation from Unicode 5.1.0 standard 
// (http://www.unicode.org/reports/tr14/tr14-22.html)
GFxWWHelper::CharBreakInfo GFxWWHelper::CharBreakInfoArray[] = 
{    
    {0x0021, CBI_NonStartingChar},                          // !    EX (XB)
    {0x0022, CBI_NonStartingChar | CBI_NonTerminatingChar}, // "    QU (XB/XA)
    {0x0024, CBI_NonTerminatingChar},                       // $    
    {0x0025, CBI_NonStartingChar},                          // %    PO (XB)
    {0x0027, CBI_NonStartingChar | CBI_NonTerminatingChar}, // '    QU (XB/XA)
    {0x0028, CBI_NonTerminatingChar},                       // (    
    {0x0029, CBI_NonStartingChar},                          // )           
    {0x00B1, CBI_NonTerminatingChar},                       // Plus Sign,       PR (XA)                           
    {0x002E, CBI_NonStartingChar | CBI_NonTerminatingChar}, // .    IS (XB)
    {0x002F, CBI_NonStartingChar | CBI_NonTerminatingChar}, // /    SY (A)
    {0x003A, CBI_NonStartingChar},                          // :    IS (XB)
    {0x003B, CBI_NonStartingChar},                          // ;    IS (XB)
    {0x003F, CBI_NonStartingChar},                          // ?    EX (XB)
    {0x005B, CBI_NonTerminatingChar},                       // [
    {0x005C, CBI_NonTerminatingChar},                       // Reverse Solidus, PR (XA)                           
    {0x005D, CBI_NonStartingChar},                          // ]
    {0x007B, CBI_NonTerminatingChar},                       // {
    {0x007D, CBI_NonStartingChar},                          // }
    {0x00A2, CBI_NonStartingChar},                          // Cent Sign,       PO (XB)
    {0x00A3, CBI_NonTerminatingChar},                       // Pound Sign
    {0x00A5, CBI_NonTerminatingChar},                       // Yen Sign
    {0x00A7, CBI_NonTerminatingChar},                       // Section Sign
    {0x00B0, CBI_NonStartingChar},                          // Degree Sign,     PO (XB)
    {0x00B1, CBI_NonTerminatingChar},                       // Plus-Minus,      PR (XA)                           
    {0x00B7, CBI_NonStartingChar},                          // Middle Dot
    {0x058A, CBI_NonStartingChar},                          // Armenian Hyphen, BA (A)
    {0x2010, CBI_NonStartingChar},                          // Hyphen           BA (A)
    {0x2012, CBI_NonStartingChar},                          // Figure Dash      BA (A)
    {0x2013, CBI_NonStartingChar},                          // En Dash          BA (A)
    {0x2014, CBI_NonStartingChar},                          // Em Dash          B2 (B/A/XP)
    {0x2018, CBI_NonTerminatingChar},                       // Left Single Quotation Mark
    {0x2019, CBI_NonStartingChar},                          // Right Single Quotation Mark               
    {0x201C, CBI_NonTerminatingChar},                       // Left Double Quotation Mark
    {0x201D, CBI_NonStartingChar},                          // Right Double Quotation Mark               
    {0x2024, CBI_NonStartingChar},                          // One Dot Leader   IN (XP)
    {0x2025, CBI_NonStartingChar},                          // Two Dot Leader   IN (XP)
    {0x2026, CBI_NonStartingChar},                          // Horizontal Ellipsis, IN (XP)
    {0x2027, CBI_NonStartingChar},                          // Hyphenation Point,   BA (A)
    {0x2032, CBI_NonStartingChar},                          // Prime                PO (XB)
    {0x2033, CBI_NonStartingChar},                          // Double Prime         PO (XB)
    {0x2035, CBI_NonTerminatingChar},                       // Reversed Prime       PO (XB)
    {0x20A7, CBI_NonTerminatingChar},                       // Peseta Sign          PO (XB)
    {0x2103, CBI_NonStartingChar},                          // Degree Celsius Sign, PO (XB)
    {0x2109, CBI_NonStartingChar},                          // Degree Fahrenheit,   PO (XB)
    {0x2116, CBI_NonTerminatingChar},                       // Numero Sign,         PR (XA)                           
    {0x2212, CBI_NonTerminatingChar},                       // Minus Sign,          PR (XA)                           
    {0x2213, CBI_NonTerminatingChar},                       // Minus-or-plus Sign,  PR (XA)                           
    {0x2122, CBI_NonStartingChar},                          // Trade mark sign
    {0x2574, CBI_NonStartingChar},                          // Box Drawings Light Left
    {0x266F, CBI_NonTerminatingChar},                       // Music Sharp Sign (#)
    {0x3001, CBI_NonStartingChar},                          // Ideographic comma    CL (XB)
    {0x3002, CBI_NonStartingChar},                          // Full stop            CL (XB)
    {0x3005, CBI_NonStartingChar},                          // Ideographic Iteration Mark   NS (XB)
    {0x3008, CBI_NonTerminatingChar},                       // Left Angle Bracket
    {0x3009, CBI_NonStartingChar},                          // Right Angle Bracket
    {0x300A, CBI_NonTerminatingChar},                       // Left Double Angle Bracket
    {0x300B, CBI_NonStartingChar},                          // Right Double Angle Bracket 
    {0x300C, CBI_NonTerminatingChar},                       // Left Corner Bracket  
    {0x300D, CBI_NonStartingChar},                          // Right Corner Bracket 
    {0x300E, CBI_NonTerminatingChar},                       // Left White Corner Bracket
    {0x300F, CBI_NonStartingChar},                          // Right White Corner Bracket
    {0x3010, CBI_NonTerminatingChar},                       // Left Black Lenticular Bracket 
    {0x3011, CBI_NonStartingChar},                          // Right black Lenticular Bracket
    {0x3012, CBI_NonTerminatingChar},                       // Postal Mark
    {0x3014, CBI_NonTerminatingChar},                       // Left Tortoise Shell Bracket
    {0x3015, CBI_NonStartingChar},                          // Right Tortoise Shell Bracket
    {0x301C, CBI_NonStartingChar},                          // Wave Dash                    NS (XB)
    {0x301D, CBI_NonTerminatingChar},                       // Reversed Double Prime Quotation Mark     
    {0x301E, CBI_NonStartingChar},                          // Double Prime Quotation Mark     
    {0x301F, CBI_NonStartingChar},                          // Low Double Prime Quotation Mark
    // Japanese, Hiragana
    {0x3041, CBI_NonStartingChar},                          // Hiragana Letter Small A
    {0x3043, CBI_NonStartingChar},                          // Hiragana Letter Small I 
    {0x3045, CBI_NonStartingChar},                          // Hiragana Letter Small U
    {0x3047, CBI_NonStartingChar},                          // Hiragana Letter Small E
    {0x3049, CBI_NonStartingChar},                          // Hiragana Letter Small O 
    {0x3063, CBI_NonStartingChar},                          // Hiragana Letter Small Tu
    {0x3083, CBI_NonStartingChar},                          // Hiragana Letter Small Ya
    {0x3085, CBI_NonStartingChar},                          // Hiragana Letter Small Yu 
    {0x3087, CBI_NonStartingChar},                          // Hiragana Letter Small Yo   
    {0x308E, CBI_NonStartingChar},                          // Hiragana Letter Small Wa 
    // Japanese, Katakana/Hiragana
    {0x309B, CBI_NonStartingChar},                          // Katakana-Hiragana Voiced Sound Mark      NS (XB)
    {0x309C, CBI_NonStartingChar},                          // Katakana-Hiragana Semi-Voiced Sound Mark NS (XB)
    {0x309D, CBI_NonStartingChar},                          // Hiragana Iteration Mark                  NS (XB) 
    {0x309E, CBI_NonStartingChar},                          // Hiragana Voiced Iteration Mark           NS (XB) 
    // Japanese, Katakana
    {0x30A1, CBI_NonStartingChar},                          // Katakana Letter Small A                   
    {0x30A3, CBI_NonStartingChar},                          // Katakana Letter Small I
    {0x30A5, CBI_NonStartingChar},                          // Katakana Letter Small U
    {0x30A7, CBI_NonStartingChar},                          // Katakana Letter Small E 
    {0x30A9, CBI_NonStartingChar},                          // Katakana Letter Small O 
    {0x30C3, CBI_NonStartingChar},                          // Katakana Letter Small Tu
    {0x30E3, CBI_NonStartingChar},                          // Katakana Letter Small Ya                   
    {0x30E5, CBI_NonStartingChar},                          // Katakana Letter Small Yu                   
    {0x30E7, CBI_NonStartingChar},                          // Katakana Letter Small Yo                   
    {0x30EE, CBI_NonStartingChar},                          // Katakana Letter Small Wa                   
    {0x30F5, CBI_NonStartingChar},                          // Katakana Letter Small Ka                   
    {0x30F6, CBI_NonStartingChar},                          // Katakana Letter Small Ke                   
    {0x30FB, CBI_NonStartingChar},                          // katakana middle dot                        
    {0x30FC, CBI_NonStartingChar},                          // Katakana-Hiragana Prolonged Sound Mark     
    {0x30FD, CBI_NonStartingChar},                          // Katakana Iteration Mark                    
    {0x30FE, CBI_NonStartingChar},                          // Katakana Voiced Iteration Mark             
    {0xFDFC, CBI_NonStartingChar},                          // Rial Sign        PO (XB)
    {0xFE50, CBI_NonStartingChar},                          // Small Comma      CL (XB)
    {0xFE51, CBI_NonStartingChar},                          // Small Ideographic Comma
    {0xFE52, CBI_NonStartingChar},                          // Small Full Stop  CL (XB)
    {0xFE54, CBI_NonStartingChar},                          // Small Semicolon  NS (XB)                   
    {0xFE55, CBI_NonStartingChar},                          // Small Colon      NS (XB)                  
    {0xFE56, CBI_NonStartingChar},                          // Small Question Mark      EX (XB)                
    {0xFE57, CBI_NonStartingChar},                          // Small Exclamation Mark   EX (XB)          
    {0xFE59, CBI_NonTerminatingChar},                       // Small Left Parenthesis             
    {0xFE5A, CBI_NonStartingChar},                          // Small Right Parenthesis            
    {0xFE5B, CBI_NonTerminatingChar},                       // Small Left Curly Bracket           
    {0xFE5C, CBI_NonStartingChar},                          // Small Right Curly Bracket          
    {0xFE5D, CBI_NonTerminatingChar},                       // Small Left Tortoise Shell Bracket  
    {0xFE5E, CBI_NonStartingChar},                          // Small Right Tortoise Shell Bracket 
    {0xFE6A, CBI_NonStartingChar},                          // Small Percent Sign, PO (XB)
    {0xFF01, CBI_NonStartingChar},                          // Fullwidth Exclamation Mark       EX (XB)             
    {0xFF04, CBI_NonTerminatingChar},                       // Fullwidth Dollar Sign            PO (XB)      
    {0xFF05, CBI_NonStartingChar},                          // Fullwidth Percent Sign                 
    {0xFF08, CBI_NonTerminatingChar},                       // Fullwidth Left Parenthesis              
    {0xFF09, CBI_NonStartingChar},                          // Fullwidth Right Parenthesis             
    {0xFF0C, CBI_NonStartingChar},                          // Fullwidth Comma          CL (XB)
    {0xFF0E, CBI_NonStartingChar},                          // Fullwidth Full Stop      CL (XB)
    {0xFF1A, CBI_NonStartingChar},                          // Fullwidth Colon          NS (XB)       
    {0xFF1B, CBI_NonStartingChar},                          // Fullwidth Semicolon      NS (XB)       
    {0xFF1F, CBI_NonStartingChar},                          // Fullwidth Question Mark  EX (XB)
    {0xFF20, CBI_NonTerminatingChar},                       // Fullwidth Commercial At        
    {0xFF3B, CBI_NonTerminatingChar},                       // Fullwidth Left Square Bracket  
    {0xFF3D, CBI_NonStartingChar},                          // Fullwidth Right Square Bracket 
    {0xFF5B, CBI_NonTerminatingChar},                       // Fullwidth Left Curly Bracket   
    {0xFF5D, CBI_NonStartingChar},                          // Fullwidth Right Curly Bracket  
    {0xFF61, CBI_NonStartingChar},                          // Halfwidth Comma          CL (XB)
    {0xFF64, CBI_NonStartingChar},                          // Halfwidth Full Stop      CL (XB)
    {0xFFE0, CBI_NonStartingChar | CBI_NonTerminatingChar}, // Fullwidth Cent Sign      PO (XB)
    {0xFFE1, CBI_NonTerminatingChar},                       // Fullwidth Pound Sign 
    {0xFFE5, CBI_NonTerminatingChar},                       // Fullwidth Yen Sign  
    {0xFFE6, CBI_NonTerminatingChar}                        // Fullwidth Won Sign  
};   

bool GFxWWHelper::FindCharWithFlags(UInt wwMode, wchar_t c, UInt charBreakFlags)
{
    if (!(wwMode & GFxTranslator::WWT_Prohibition)) 
        return false;

    SPInt left   = 0;
    SPInt right  = sizeof(CharBreakInfoArray)/sizeof(CharBreakInfoArray[0]);
    SPInt middle = 0;

    while (left <= right)
    {
        middle = ((right-left)/2) + left;
        if (c == CharBreakInfoArray[middle].Char)
            return (CharBreakInfoArray[middle].Flags & charBreakFlags) != 0;

        if (c < CharBreakInfoArray[middle].Char)
            right = middle - 1;
        else
            left  = middle + 1;
    }
    return false;
}

bool GFxTranslator::OnWordWrapping(LineFormatDesc* pdesc)
{
    if (WWMode == WWT_Default)
        return false;
    if ((WWMode & (WWT_Asian | WWT_NoHangulWrap | WWT_Prohibition)) && pdesc->NumCharsInLine > 0)
    {
        const wchar_t* pstr = pdesc->pParaText + pdesc->LineStartPos;
        UPInt remainingLen  = pdesc->ParaTextLen - pdesc->LineStartPos;
        UPInt pos = pdesc->NumCharsInLine - 1;
        UPInt wordWrapPos = pdesc->ProposedWordWrapPoint;
        for (; pos > 0; --pos)
        {
            if (GFxWWHelper::IsWhiteSpaceChar(pstr[pos]))
            {
                UPInt eolPos = GFxWWHelper::FindPrevNonWhiteSpace(pstr, pos);
                if (eolPos == GFC_MAX_UPINT)
                {
                    pos = GFxWWHelper::FindNextNonWhiteSpace (pstr, pos + 1, remainingLen - 1);
                    break;
                }
                pos = eolPos + 1;
            }
            if (GFxWWHelper::IsLineBreakOpportunityAt(WWMode, pstr, pos))
                break;
        }            
        if (pos == 0)
        {
            // couldn't find any character to break the line
            wordWrapPos = pdesc->NumCharsInLine - 1;
        }
        else
        {
            wordWrapPos = GFxWWHelper::FindNextNonWhiteSpace (pstr, pos, remainingLen - 1);
            if (wordWrapPos == GFC_MAX_UPINT)
                wordWrapPos = pdesc->NumCharsInLine - 1; //?
        }
        pdesc->ProposedWordWrapPoint = wordWrapPos;
        return true;
    }
    else if ((WWMode & WWT_Hyphenation))
    {
        if (pdesc->ProposedWordWrapPoint == 0)
            return false;
        const wchar_t* pstr = pdesc->pParaText + pdesc->LineStartPos;
        // determine if we need hyphenation or not. For simplicity,
        // we just will put dash only after vowels.
        UPInt hyphenPos = pdesc->NumCharsInLine;
        // check if the proposed word wrapping position is at the space.
        // if so, this will be the ending point in hyphenation position search.
        // Otherwise, will look for the position till the beginning of the line.
        // If we couldn't find appropriate position - just leave the proposed word
        // wrap point unmodified.
        UPInt endingHyphenPos = (gfc_iswspace(pstr[pdesc->ProposedWordWrapPoint - 1])) ? 
            pdesc->ProposedWordWrapPoint : 0;
        for (; hyphenPos > endingHyphenPos; --hyphenPos)
        {
            if (GFxWWHelper::IsVowel(pstr[hyphenPos - 1]))
            {
                // check if we have enough space for putting dash symbol
                // we need to summarize all widths up to hyphenPos + pdesc->DashSymbolWidth
                // and this should be less than view rect width
                Float lineW = pdesc->pWidths[hyphenPos - 1];
                lineW += pdesc->DashSymbolWidth;
                if (lineW < pdesc->VisibleRectWidth)
                {
                    // ok, looks like we can do hyphenation
                    pdesc->ProposedWordWrapPoint = hyphenPos;
                    pdesc->UseHyphenation = true;
                    return true;
                }
                else
                {
                    // oops, we have no space for hyphenation mark
                    continue;
                }
                break;
            }
        }
    }
    return false;
}
