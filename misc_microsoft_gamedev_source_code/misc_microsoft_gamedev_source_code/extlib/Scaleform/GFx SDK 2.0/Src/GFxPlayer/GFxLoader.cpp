/**********************************************************************

Filename    :   GFxLoader.cpp
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
#include "GDebug.h"
#include "GUTF8Util.h"

#include "GFxLoaderImpl.h"

#include "GImageInfo.h"
#include "GFxImageResource.h"
#include "GImage.h"

// Include GRenderConfig to support GFxImageCreator creating textures.
#include "GFxRenderConfig.h"


// ***** GFxFileOpener implementation

// Default implementation - use GSysFile.
GFile* GFxFileOpener::OpenFile(const char *purl)
{
#ifdef GFC_USE_SYSFILE
    // Buffered file wrapper is faster to use because it optimizes seeks.
    return new GBufferedFile(GPtr<GSysFile>(*new GSysFile(purl)));
#else
    GUNUSED(purl);
    GFC_DEBUG_WARNING(1, "GFxFileOpener::OpenFile failed - GFC_USE_SYSTFILE not defined");
    return 0;
#endif    
}

SInt64  GFxFileOpener::GetFileModifyTime(const char *purl)
{
#ifdef GFC_USE_SYSFILE
    GFileStat fileStat;
    if (GSysFile::GetFileStat(&fileStat, purl))
        return fileStat.ModifyTime; 
    return -1;
#else
    GUNUSED(purl);
    return 0;
#endif    
}


// Implementation that allows us to override the log.
GFile*      GFxFileOpener::OpenFileEx(const char *pfilename, GFxLog *plog)
{   
    GFile*  pin = OpenFile(pfilename);
    SInt errCode = 16;
    if (pin)
        errCode = pin->GetErrorCode();

    if (!pin || errCode)
    {
        if (plog)
            plog->LogError("Error: GFxLoader failed to open '%s'\n", pfilename);
        if (pin)
        {
            //plog->LogError("Error: Code '%d'\n", errCode);
            pin->Release();
        }
        return 0;
    }
    return pin;
}


// ***** GFxURLBuilder implementation

// Default TranslateFilename implementation.
void    GFxURLBuilder::BuildURL(GFxString *ppath, const LocationInfo& loc)
{
    DefaultBuildURL(ppath, loc);  
}  

void    GFxURLBuilder::DefaultBuildURL(GFxString *ppath, const LocationInfo& loc)
{
    // Determine the file name we should use base on a relative path.
    if (IsPathAbsolute(loc.FileName))
        *ppath = loc.FileName;
    else
    {  
        // If not absolute, concatenate image path to the relative parent path.

        UPInt length = loc.ParentPath.GetSize();
        if (length > 0)
        {
            *ppath = loc.ParentPath;
            UInt32 cend = loc.ParentPath[length - 1];

            if ((cend != '\\') && (cend != '/'))
            {
                *ppath += "/";
            }
            *ppath += loc.FileName;
        }
        else
        {
            *ppath = loc.FileName;
        }
    }
}

// Static helper function used to determine if the path is absolute.
bool    GFxURLBuilder::IsPathAbsolute(const char *putf8str)
{

    // Absolute paths can star with:
    //  - protocols:        'file://', 'http://'
    //  - windows drive:    'c:\'
    //  - UNC share name:   '\\share'
    //  - unix root         '/'

    // On the other hand, relative paths are:
    //  - directory:        'directory/file'
    //  - this directory:   './file'
    //  - parent directory: '../file'
    // 
    // For now, we don't parse '.' or '..' out, but instead let it be concatenated
    // to string and let the OS figure it out. This, however, is not good for file
    // name matching in library/etc, so it should be improved.

    if (!putf8str || !*putf8str)
        return true; // Treat empty strings as absolute.    

    UInt32 charVal = GUTF8Util::DecodeNextChar(&putf8str);

    // Fist character of '/' or '\\' means absolute path.
    if ((charVal == '/') || (charVal == '\\'))
        return true;

    while (charVal != 0)
    {
        // Treat a colon followed by a slash as absolute.
        if (charVal == ':')
        {
            charVal = GUTF8Util::DecodeNextChar(&putf8str);
            // Protocol or windows drive. Absolute.
            if ((charVal == '/') || (charVal == '\\'))
                return true;
        }
        else if ((charVal == '/') || (charVal == '\\'))
        {
            // Not a first character (else 'if' above the loop would have caught it).
            // Must be a relative path.
            break;
        }

        charVal = GUTF8Util::DecodeNextChar(&putf8str);
    }

    // We get here for relative paths.
    return false;
}

// Modifies path to not include the filename, leaves trailing '/'.
bool    GFxURLBuilder::ExtractFilePath(GFxString *ppath)
{
    // And strip off the actual file name.
    SPInt length  = (SPInt)ppath->GetLength();
    SPInt i       = length-1;
    for (; i>=0; i--)
    {
        UInt32 charVal = ppath->GetCharAt(i);

        // The trailing name will end in either '/' or '\\',
        // so just clip it off at that point.
        if ((charVal == '/') || (charVal == '\\'))
        {
            *ppath = ppath->Substring(0, i+1);
            break;
        }
    }

    // Technically we can have extra logic somewhere for paths,
    // such as enforcing protocol and '/' only based on flags,
    // but we keep it simple for now.
    return (i >= 0);
}




// ***** Default Image Creator implementation

// Default implementation reads images from DDS/TGA files
// creating GImageInfo objects.
GImageInfoBase* GFxImageCreator::CreateImage(const GFxImageCreateInfo &info)
{   
    GPtr<GImage> pimage;
    UInt         twidth = 0, theight = 0;

    switch(info.Type)
    {
    case GFxImageCreateInfo::Input_Image:
        pimage  = info.pImage;
        // We have to pass correct size; it is required at least
        // when we are initializing image info with a texture.
        twidth  = pimage->Width;
        theight = pimage->Height;
        break;

    case GFxImageCreateInfo::Input_File:
        {
            // If we got here, we are responsible for loading an image file.
            GPtr<GFile> pfile  = *info.pFileOpener->OpenFile(info.pFileInfo->FileName.ToCStr());
            if (!pfile)
                return 0; // Log ??

            // Detecting the file format may be better!?! But then, how do we handle extensions?
            // GFxLoader::FileFormatType format = GFxMovieRoot::DetectFileFormat(pfile);

            // Load an image into GImage object.
            pimage = *LoadBuiltinImage(pfile, info.pFileInfo->Format, info.Use);
            if (!pimage)
                return 0;
            twidth  = info.pFileInfo->TargetWidth;
            theight = info.pFileInfo->TargetHeight;
        }        
        break;

    // No input - empty image info.
    case GFxImageCreateInfo::Input_None:
    default:
          return new GImageInfo();
    }


    // Make a distinction whether to keep the data or not based on
    // the KeepBindingData flag in GFxImageInfo.
    if (IsKeepingImageData())    
        return new GImageInfo(pimage, twidth, theight);

    // Else, create a texture.
    if (!info.pRenderConfig || !info.pRenderConfig->GetRenderer())
    {
        // We need to either provide a renderer, or use KeepImageBindData flag.
        GFC_DEBUG_ERROR(1, "GFxImageCreator failed to create texture; no renderer installed");
        return 0;
    }

    // If renderer can generate Event_DataLost for textures (D3D9), store GImage in GFxImageInfo
    // anyway. This is helpful because otherwise images can be lost and wiped out. Users can
    // override this behavior with custom creator if desired, handling loss in alternative manner.
    if (info.pRenderConfig->GetRendererCapBits() & GRenderer::Cap_CanLoseData)    
         return new GImageInfo(pimage, twidth, theight);    

    // Renderer ok, create a texture-based GFxImageInfo instead. This means that
    // the source image memory will be released by our caller.
    GPtr<GTexture> ptexture = *info.pRenderConfig->GetRenderer()->CreateTexture();
    if (!ptexture || !ptexture->InitTexture(pimage, twidth, theight))
        return 0;

    return new GImageInfo(ptexture, twidth, theight);
}


// Loads a GImage from an open file, assuming the specified file format.
GImage* GFxImageCreator::LoadBuiltinImage(GFile* pfile,
                                         GFxFileConstants::FileFormatType format,
                                         GFxResource::ResourceUse use)
{
    // Open the file and create image.
    GImage    *pimage = 0;

    switch(format)
    {
    case GFxLoader::File_TGA:
        pimage = GImage::ReadTga(pfile, 
            (use == GFxResource::Use_FontTexture) ?
            GImage::Image_A_8 : GImage::Image_None);
        break;

    case GFxLoader::File_DDS:
        pimage = GImage::ReadDDS(pfile);
        break;
    case GFxLoader::File_JPEG:

#ifndef GFC_USE_LIBJPEG
        // This should be a log message!
        GFC_DEBUG_WARNING1(1, "Unable to load JPEG at URL \"%s\" - libjpeg not included\n",
                              pfile->GetFilePath());  
#else
        pimage = GImage::ReadJpeg(pfile);
#endif
        break;

    case GFxLoader::File_PNG:

#ifndef GFC_USE_LIBPNG
        // This should be a log message!
        GFC_DEBUG_WARNING1(1, "Unable to load PNG at URL \"%s\" - libpng not included\n",
            pfile->GetFilePath());  
#else
        pimage = GImage::ReadPng(pfile);
#endif
        break;

    default:
        // Unknown format!
        GFC_DEBUG_WARNING1(1, "Default image loader failed to load '%s'",
                             pfile->GetFilePath());
        break;
    }

    return pimage;
}




// ***** GFxSharedState implementation


// Implementation that allows us to override the log.
GFile*      GFxSharedState::OpenFileEx(const char *pfilename, GFxLog *plog)
{
    GPtr<GFxFileOpener> popener = GetFileOpener();
    if (!popener)
    {             
        // Don't even have a way to open the file.
        if (plog)
            plog->LogError(
            "Error: GFxLoader failed to open '%s', GFxFileOpener not installed\n",
            pfilename);
        return 0;
    }

    return popener->OpenFileEx(pfilename, plog);
}

// Opens a file using the specified callback.
GFile*      GFxSharedState::OpenFile(const char *pfilename)
{
    return OpenFileEx(pfilename, GetLog());
}



GImageInfoBase* GFxSharedState::CreateImageInfo(const GFxImageCreateInfo& info)
{
    GPtr<GFxImageCreator> pcreator = GetImageCreator();
    if (pcreator)
        return pcreator->CreateImage(info);

    return new GImageInfo(
        (info.Type == GFxImageCreateInfo::Input_Image) ? info.pImage : 0,
        (info.Type == GFxImageCreateInfo::Input_File) ? info.pFileInfo->TargetWidth : 0,
        (info.Type == GFxImageCreateInfo::Input_File) ? info.pFileInfo->TargetHeight : 0);     
}





// ***** GFxLoader implementation

// Internal GFC Evaluation License Reader
#ifdef GFC_BUILD_EVAL
#include "GFCValidateEval.cpp"
#else
void    GFx_ValidateEvaluation() { }
#endif


GFxLoader::GFxLoader(GFxFileOpener* pfileOpener, UByte verboseParseFlags)
{
    GFx_ValidateEvaluation();

    pStrongResourceLib = new GFxResourceLib();   
 
    if ((pImpl = new GFxLoaderImpl(pStrongResourceLib))!=0)
    {
        SetFileOpener(pfileOpener);
        SetParseControl(GPtr<GFxParseControl>(*new GFxParseControl(verboseParseFlags)));
    }    

    GFC_DEBUG_ERROR(!pImpl || !pStrongResourceLib,
                    "GFxLoader::GFxLoader failed to initialize properly, low memory");
}

// Create a new loader, copying it's library and states.
GFxLoader::GFxLoader(const GFxLoader& src)
{
    GFx_ValidateEvaluation();

    // Create new LoaderImpl with copied states.
    pImpl = new GFxLoaderImpl(*src.pImpl);
    // Copy strong resource lib reference.
    pStrongResourceLib = src.pStrongResourceLib;
    if (pStrongResourceLib)
        pStrongResourceLib->AddRef();

    GFC_DEBUG_ERROR(!pImpl, "GFxLoader::GFxLoader failed to initialize properly, low memory");
}


GFxLoader::~GFxLoader()
{
    if (pImpl)
        pImpl->Release();
    if (pStrongResourceLib)
        pStrongResourceLib->Release();

}

GFxSharedState* GFxLoader::GetSharedImpl() const
{
    return pImpl;
}

bool GFxLoader::CheckTagLoader(int tagType) const 
{ 
    return (pImpl) ? pImpl->CheckTagLoader(tagType) : 0;        
};

// Resource library management.
void               GFxLoader::SetResourceLib(GFxResourceLib *plib)
{
    if (plib)
        plib->AddRef();
    if (pStrongResourceLib)
        pStrongResourceLib->Release();
    pStrongResourceLib = plib;
}
GFxResourceLib*    GFxLoader::GetResourceLib() const
{
    return pStrongResourceLib;
}


// *** Movie Loading

// Movie Loading APIs just delegate to GFxLoaderImpl after some error checking.

bool    GFxLoader::GetMovieInfo(const char *pfilename, GFxMovieInfo *pinfo,
                                bool getTagCount, UInt loadConstants)
{
    if (!pfilename || pfilename[0]==0)
    {
        GFC_DEBUG_WARNING(1, "GFxLoader::GetMovieInfo - passed filename is empty, no file to load");
        return 0;
    }
    if (!pinfo)
    {
        GFC_DEBUG_WARNING(1, "GFxLoader::GetMovieInfo - passed GFxMovieInfo pointer is NULL");
        return 0;
    }

    return pImpl ? pImpl->GetMovieInfo(pfilename, pinfo, getTagCount, loadConstants) : 0;
}


GFxMovieDef*    GFxLoader::CreateMovie(const char *pfilename, UInt loadConstants)
{
    if (!pfilename || pfilename[0]==0)
    {
        GFC_DEBUG_WARNING(1, "GFxLoader::CreateMovie - passed filename is empty, no file to load");
        return 0;
    }
    return pImpl ? pImpl->CreateMovie(pfilename, loadConstants) : 0;
}

/*
GFxMovieDef*    GFxLoader::CreateMovie(GFile *pfile, UInt loadConstants)
{
    if (!pfile || !pfile->IsValid())
    {
        GFC_DEBUG_WARNING(1, "GFxLoader::CreateMovie - passed file is not valid, no file to load");
        return 0;
    }
    return pImpl ? pImpl->CreateMovie(pfile, 0, loadConstants) : 0;
}
*/


// Unpins all resources held in the library
void GFxLoader::UnpinAll()
{
    if (pStrongResourceLib)    
        pStrongResourceLib->UnpinAll();    
}
