/**********************************************************************

Filename    :   GFxLoadProcess.ccp
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

#include "GFxLoadProcess.h"

#include "GJPEGUtil.h"

// For GFxMovieRoot::IsPathAbsolute
//#include "GFxPlayerImpl.h"



// ***** GFxLoadStates


GFxLoadStates::GFxLoadStates(GFxLoaderImpl* pimpl, GFxSharedState* pstates,
                             GFxMovieDefBindStates *pbindStates)
{
    pLoaderImpl         = pimpl;
    pWeakResourceLib    = pimpl->GetWeakLib();

    // If pstates is null, states comde from loader.
    if (!pstates)
        pstates = pimpl;

    // Capture states.
    // We may override bind states when loading fonts from FontLib.
    pBindStates         = pbindStates ? *new GFxMovieDefBindStates(pbindStates) :
                                        *new GFxMovieDefBindStates(pstates);
    pLog                = pstates->GetLog();
    pParseControl       = pstates->GetParseControl();
    pProgressHandler    = pstates->GetProgressHandler();
    pFontCacheManager   = pstates->GetFontCacheManager();
    pTaskManager        = pstates->GetTaskManager();
    // Record render config so that it can be used for image creation during loading.
    pRenderConfig       = pstates->GetRenderConfig();
}


// Obtains an image creator, but only if image data is not supposed to
// be preserved; considers GFxLoader::LoadKeepBindData flag from arguments.
GFxImageCreator* GFxLoadStates::GetLoadTimeImageCreator(UInt loadConstants) const
{
    // Image creator is only used as a key if it is bound to, as specified
    // by flags and IsKeepingImageData member.
    GFxImageCreator* pkeyImageCreator = 0;

    if (!(loadConstants & GFxLoader::LoadKeepBindData) &&
        pBindStates->pImageCreator &&
        !pBindStates->pImageCreator->IsKeepingImageData())
    {
        pkeyImageCreator = pBindStates->pImageCreator.GetPtr();
    }

    return pkeyImageCreator;
}



// Implementation that allows us to override the log.
GFile*      GFxLoadStates::OpenFile(const char *pfilename)
{    
    if (!pBindStates->pFileOpener)
    {             
        // Don't even have a way to open the file.
        if (pLog)
            pLog->LogError(
            "Error: GFxLoader failed to open '%s', GFxFileOpener not installed\n",
            pfilename);
        return 0;
    }

    return pBindStates->pFileOpener->OpenFileEx(pfilename, pLog);
}

// Perform filename translation and/or copy by relying on translator.
void    GFxLoadStates::BuildURL(GFxString *pdest, const GFxURLBuilder::LocationInfo &loc) const
{
    GASSERT(pdest);
    GFxURLBuilder* pbuilder = pBindStates->pURLBulider;

    if (!pbuilder)
        GFxURLBuilder::DefaultBuildURL(pdest, loc);
    else
        pbuilder->BuildURL(pdest, loc);
}

// Helper that clones load states, pBindSates.
// The only thing left un-copied is GFxMovieDefBindStates::pDataDef
GFxLoadStates*  GFxLoadStates::CloneForImport() const
{    
    GPtr<GFxMovieDefBindStates> pnewBindStates = *new GFxMovieDefBindStates(pBindStates);
    GFxLoadStates*              pnewStates = new GFxLoadStates;

    if (pnewStates)
    {
        pnewStates->pBindStates     = pnewBindStates;
        pnewStates->pLoaderImpl     = pLoaderImpl;
        pnewStates->pLog            = pLog;
        pnewStates->pProgressHandler= pProgressHandler;
        pnewStates->pTaskManager    = pTaskManager;
        pnewStates->pFontCacheManager = pFontCacheManager;
        pnewStates->pParseControl   = pParseControl;
        pnewStates->pWeakResourceLib= pWeakResourceLib;
        // Must copy RenderConfig, even though it's not a binding state.
        pnewStates->pRenderConfig   = pRenderConfig;            
    }
    return pnewStates;
}




// ***** GFxLoadProcess


GFxLoadProcess::GFxLoadProcess(GFxLoadStates *pstates, UInt loadFlags)
{
    // Save states.
    pStates             = pstates;
    // Cache parse flags for quick accesss.
    ParseFlags          = pstates->pParseControl.GetPtr() ?
                          pstates->pParseControl->GetParseFlags() : 0;

    // Initialize other data used during loading.    
    pTimelineDef        = 0;
    pDataDef            = 0;

    pJpegIn             = 0;
    LoadFlags           = loadFlags;
    LoadState           = LS_LoadingRoot;

    // Avoid resizing frame arrays to 0 (extra allocations).
    FrameTags[0].set_size_policy(GTL::garray<GASExecuteTag*>::Buffer_NoClear);
    FrameTags[1].set_size_policy(GTL::garray<GASExecuteTag*>::Buffer_NoClear);
    InitActionTags.set_size_policy(GTL::garray<GASExecuteTag*>::Buffer_NoClear);
}


GFxLoadProcess::~GFxLoadProcess()
{
    // Clear JPEG loader if it was used.
    if (pJpegIn)
        delete pJpegIn;
}

void    GFxLoadProcess::ReadRgbaTag(GColor *pc, GFxTagType tagType)
{
    if (tagType <= GFxTag_DefineShape2)
        GetStream()->ReadRgb(pc);
    else
        GetStream()->ReadRgba(pc);
}


GFxTimelineDef::Frame GFxLoadProcess::TagArrayToFrame(GTL::garray<GASExecuteTag*> &tagArray)
{
    GFxTimelineDef::Frame frame;

    if (tagArray.size())
    {
        size_t memSize = sizeof(GASExecuteTag*) * tagArray.size();

        if ((frame.pTagPtrList = (GASExecuteTag**)AllocTagMemory(memSize)) != 0)
        {
            memcpy(frame.pTagPtrList, &tagArray[0], memSize);
            frame.TagCount = (UInt)tagArray.size();
        }
        tagArray.clear();
    }

    return frame;
}




// Add a dynamicaly-loaded image resource, with unique key.
// This is normally used for SWF embedded images.
void    GFxLoadProcess::AddImageResource(GFxResourceId rid, GImage *pimage)
{    
    GFxImageCreator* pkeyImageCreator = pStates->GetLoadTimeImageCreator(LoadFlags);

    // Image creator is only used as a key if it is bound to, based on flags.
    if (pkeyImageCreator)
    {
        GFxImageCreateInfo   icreateInfo;
        GPtr<GImageInfoBase> pimageInfo;

        icreateInfo.SetStates(0, pStates->GetRenderConfig());

        if (pimage)
            icreateInfo.SetImage(pimage);
        if (GetBindStates()->pImageCreator)
            pimageInfo= *GetBindStates()->pImageCreator->CreateImage(icreateInfo);
        
        GPtr<GFxImageResource> pch = *new GFxImageResource(pimageInfo.GetPtr());
        AddResource(rid, pch);        
    }
    else
    {
        // Creation of image resource is delayed due to late binding.
        // Create image resource data instead. This data will be used during binding.
        GFxResourceData rdata = GFxImageResourceCreator::CreateImageResourceData(pimage);
        AddDataResource(rid, rdata);
    }
}




// ***** GFxImageResourceCreator - Image creator from files

// Creates/Loads resource based on data and loading process
bool    GFxImageFileResourceCreator::CreateResource(DataHandle hdata, GFxResourceBindData *pbindData,
                                                    GFxLoadStates *pls) const
{
    GFxImageFileInfo *prfi = (GFxImageFileInfo*) hdata;
    GASSERT(prfi);


    // Make a new fileInfo so that it can have a modified filename.   
    GPtr<GFxImageFileInfo> pimageFileInfo = *new GFxImageFileInfo(*prfi);

    // Ensure format
    // TBD: If we always save the format for texture tags in gfxexport we
    // would not need to do this. Verify.
    if ((pimageFileInfo->Format == GFxLoader::File_Unknown) && pimageFileInfo->pExporterInfo)
        pimageFileInfo->Format =
            (GFxFileConstants::FileFormatType) pimageFileInfo->pExporterInfo->Format;
   
    // Translate filename.
    GFxURLBuilder::LocationInfo loc(GFxURLBuilder::File_ImageImport,
                                    prfi->FileName, pls->GetRelativePath());
    pls->BuildURL(&pimageFileInfo->FileName, loc);
  

     // Now that we have a correct file object we need to match it against
     // the library so as to check if is is already loaded
     GFxResourceKey imageKey =
         GFxImageResource::CreateImageFileKey(pimageFileInfo, 
                                              pls->GetFileOpener(),
                                              pls->GetBindStates()->pImageCreator);

    GFxString                   errorMessage;
    GFxResourceLib::BindHandle	bh;
    GPtr<GFxImageResource>      pimageRes = 0;
    
    if (pls->GetLib()->BindResourceKey(&bh, imageKey) == GFxResourceLib::RS_NeedsResolve)
    {
        // If hot found, create an object of the right type.
        GFxImageCreateInfo ico;
        ico.SetType(GFxImageCreateInfo::Input_File);
        ico.Use         = prfi->Use;
        ico.pFileInfo   = pimageFileInfo.GetPtr();
        ico.SetStates(pls->GetFileOpener(), pls->GetRenderConfig());

        GPtr<GImageInfoBase> pimage;
        GFxImageCreator*     pcreator = pls->GetBindStates()->pImageCreator;
        
        if (pcreator)
            pimage = *pcreator->CreateImage(ico);
        else
            GFC_DEBUG_WARNING(1, "Image resource creation failed - GFxImageCreator not installed");        
       
        if (pimage)
            pimageRes = *new GFxImageResource(pimage.GetPtr(), imageKey, ico.Use);
        // Need to read header first.
        if (pimageRes)
            bh.ResolveResource(pimageRes);
        else
        {
            errorMessage = "Failed to load image '";
            errorMessage += pimageFileInfo->FileName;
            errorMessage += "'";

            bh.CancelResolve(errorMessage.ToCStr());
        }
    }
    else
    {
        // If Available and Waiting resources will be resolved here.
        if ((pimageRes = *(GFxImageResource*)bh.WaitForResolve()).GetPtr() == 0)
        {
            errorMessage = bh.GetResolveError();
        }
    }
    
    // If there was an error, display it
    if (!pimageRes)
    {
        pls->pLog->LogError("Error: %s\n", errorMessage.ToCStr());
        return 0;
    }

    // Pass resource ownership to BindData.
    pbindData->pResource = pimageRes;
    return 1;    
}


GFxResourceData GFxImageFileResourceCreator::CreateImageFileResourceData(GFxImageFileInfo * prfi)
{
    static GFxImageFileResourceCreator inst;
    return GFxResourceData(&inst, prfi);
}



// Creates/Loads resource based on GImage during binding process
bool    GFxImageResourceCreator::CreateResource(DataHandle hdata, GFxResourceBindData *pbindData,
                                                GFxLoadStates *pls) const
{
    GImage *pimage = (GImage*) hdata;
    GASSERT(pimage);

    // Create resource from image.
    GFxImageCreateInfo   icreateInfo(pimage, GFxResource::Use_None, pls->GetRenderConfig());
    GPtr<GImageInfoBase> pimageInfo;
    GFxImageCreator*     pcreator = pls->GetBindStates()->pImageCreator;
    if (!pcreator)
    {
        GFC_DEBUG_WARNING(1, "Image resource creation failed - GFxImageCreator not installed");
        return 0;
    }
    if (pcreator)    
        pimageInfo= *pcreator->CreateImage(icreateInfo);
    if (!pimageInfo)
        return 0;
    
    GPtr<GFxImageResource> pimageRes = *new GFxImageResource(pimageInfo.GetPtr());
    if (!pimageRes)
        return 0;

    // Pass resource ownership to BindData.
    pbindData->pResource = pimageRes;
    return 1;    
}

GFxResourceData GFxImageResourceCreator::CreateImageResourceData(GImage* pimage)
{
    static GFxImageResourceCreator inst;
    //!AB: pimage might be NULL if ZLIB is not linked and zlib-compresed image is found.
    if (pimage)
        return GFxResourceData(&inst, pimage);
    else
        return GFxResourceData();
}
