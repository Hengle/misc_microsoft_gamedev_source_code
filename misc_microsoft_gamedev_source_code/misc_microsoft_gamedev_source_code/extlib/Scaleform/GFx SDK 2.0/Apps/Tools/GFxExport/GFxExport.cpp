/**********************************************************************

Filename    :   GFxExport.cpp
Content     :   SWF to GFX resource extraction and conversion tool
Created     :   August, 2006
Authors     :   Artyom Bolgar
Copyright   :   (c) 2006-2007 Scaleform Corp. All Rights Reserved.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#define GFX_EXPORT_MAJOR_VERSION    2
#define GFX_EXPORT_MINOR_VERSION    04
#define GFX_EXPORT_VERSION (((GFX_EXPORT_MAJOR_VERSION)<<8)|(GFX_EXPORT_MINOR_VERSION))

#include "GFxExport.h"
#include "GFxPlayerImpl.h"

// Standard includes
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <locale.h>
#include <zlib.h>
#include <winbase.h>
#include <direct.h>

#define GFX_MAX_TEXTURE_DIMENSION 4096

// ***** Default Font Lib texture generation constants

// The default rendered glyph size can be overridden to trade off
// memory vs. tessellation of large glyphs.
// To set these values use GFxFontPackParams::GetTextureGlyphConfig.

#ifndef GFX_FONT_NOMINAL_GLYPH_SIZE_DEFAULT
#define GFX_FONT_NOMINAL_GLYPH_SIZE_DEFAULT 48
#endif // GFX_FONT_NOMINAL_GLYPH_SIZE_DEFAULT

#ifndef GFX_FONT_PAD_PIXELS_DEFAULT
#define GFX_FONT_PAD_PIXELS_DEFAULT 3
#endif // GFX_FONT_PAD_PIXELS_DEFAULT

#ifndef GFX_FONT_CACHE_TEXTURE_SIZE_DEFAULT
#define GFX_FONT_CACHE_TEXTURE_SIZE_DEFAULT 1024
#endif // GFX_FONT_CACHE_TEXTURE_SIZE_DEFAULT

#if defined(GFC_CC_MSVC) && (GFC_CC_MSVC >= 1400)
#define gfc_getcwd  _getcwd
#define gfc_chdir   _chdir
#define gfc_mkdir   _mkdir
#else
#define gfc_getcwd  getcwd
#define gfc_chdir   chdir
#define gfc_mkdir   mkdir
#endif

inline char* gfc_strlwr(char* str, size_t numberOfElements)
{
#if defined(GFC_MSVC_SAFESTRING)
    _strlwr_s(str, numberOfElements);
    return str;
#else
    GUNUSED(numberOfElements);
    return strlwr(str);
#endif
}

inline char* gfc_strupr(char* str, size_t numberOfElements)
{
#if defined(GFC_MSVC_SAFESTRING)
    _strupr_s(str, numberOfElements);
    return str;
#else
    GUNUSED(numberOfElements);
    return strupr(str);
#endif
}

void GFxImageExporterParams::Set(UInt32 charId, const GFxDataExporter* pexp, GFxExportImageDescr* pimgDescr, GImage* pimage, GFile* pfile)
{
    CharacterId = charId;
    pExporter   = pexp;
    pImage      = pimage;
    pImageDescr = pimgDescr;
    pFile       = pfile;
    Rescale     = (pExporter) ? pExporter->Rescale : GDXTHelper::RescaleNone;
}

GFxDataExporter::GFxDataExporter(UByte verboseFlags)
{
    Quiet           = false;
    DoStrip         = true;
    DoCompress      = false;
    DoCreateSubDirs = false;
    UncompressedDDS = false;
    DTX1Allowed     = false;
    DTXn            = 3;
    GenMipMapLevels = true;
    Quality         = GDXTHelper::QualityNormal;
    Rescale         = GDXTHelper::RescaleNone;
    RescaleFilter   = GDXTHelper::FilterCubic;
    MipFilter       = GDXTHelper::FilterTriangle;
    ToUppercase     = false;
    ToLowercase     = false;
    ImagesFormat    = "tga";
    JustStripImages = false;
    ShareImages     = false;

    ExportDefaultEditText = false;
    DefaultDynamicTextOnly = false;

    ExportFsCommands = false;
    FsCommandsAsTree = false;
    FsCommandsAsList = false;
    FsCommandsParams = false;

    ExportFonts             = false;
    ExportFontList          = false;
    GlyphsStripped          = false;
    TextureGlyphNominalSize = GFX_FONT_NOMINAL_GLYPH_SIZE_DEFAULT;
    TextureGlyphPadPixels   = GFX_FONT_PAD_PIXELS_DEFAULT;
    FontTextureWidth        = GFX_FONT_CACHE_TEXTURE_SIZE_DEFAULT;
    FontTextureHeight       = GFX_FONT_CACHE_TEXTURE_SIZE_DEFAULT;
    FontImagesFormat        = GFxFileConstants::File_TGA;
    FontImagesBits          = 8;
    UseSeparateFontTextures = false;

    ExportGradients         = false;
    GradientSize            = 0;
    GradientImagesFormat    = GFxFileConstants::File_TGA;
    GradientImagesBits      = 32;

    GPtr<GFxImageCreator> pimageCreator = *new GFxImageCreator(1);
    Loader.SetImageCreator(pimageCreator);
    Loader.SetFontPackParams(GPtr<GFxFontPackParams>(*new GFxFontPackParams));

    if (verboseFlags)
    {
        GPtr<GFxParseControl> pparseControl = *new GFxParseControl(verboseFlags);
        Loader.SetParseControl(pparseControl);
    }

    class FileOpener : public GFxFileOpener
    {
    public:
        virtual GFile* OpenFile(const char *purl)
        {
            // Buffered file wrapper is faster to use because it optimizes seeks.
            return new GBufferedFile(GPtr<GSysFile>(*new GSysFile(purl)));
        }
    };
    GPtr<GFxFileOpener> pfileOpener = *new FileOpener;
    Loader.SetFileOpener(pfileOpener);
    //?Loader.SetLog(GPtr<GFxLog>(*new Log()));

    class ProgressHandler : public GFxProgressHandler
    {
        GFxDataExporter* pExporter;
    public:

        ProgressHandler(GFxDataExporter* pexporter) : pExporter(pexporter) { }

        virtual void    ProgressUpdate(const GFxString& fileURL, const GFxTagInfo& tagInfo, UInt bytesLoaded, UInt totalBytes)
        {
            pExporter->ProgressCallback(fileURL, tagInfo, bytesLoaded, totalBytes);
        }
    };
    GPtr<GFxProgressHandler> pprogressHandler = *new ProgressHandler(this);
    Loader.SetProgressHandler(pprogressHandler);
}

GFxDataExporter::~GFxDataExporter()
{
    ClearOriginalImageData();

    GTL::ghash<int, GFxImageExporter*>::const_iterator iter = ImageExporters.begin();
    for(; iter != ImageExporters.end(); ++iter)
    {
        delete iter->second;
    }
}

GFile*  GFxDataExporter::FileOpener(const char* url)
{
    return new GSysFile(url);
}

GFxString GFxDataExporter::CutPath(const GFxString& filename)
{
    for (int n = filename.GetSize() - 1; n >= 0; --n)
    {
        char c = filename[n];
        if (c == '\\' || c == '/')
        {
            GFxString nopath(filename.ToCStr() + n + 1);
            return nopath;
        }
    }
    return filename;
}

GFxString GFxDataExporter::CutExtension(const GFxString& filename)
{
    GFxString noext(filename);
    for (int n = filename.GetSize() - 1; n >= 0; --n)
    {
        if (filename[n] == '.')
        {
            noext.Resize(n);
            return noext;
        }
    }
    return noext;   
}

void GFxDataExporter::ImageExtractorVisitor::Visit(GFxMovieDef* pmovieDef, GFxResource* presource,
                                                   GFxResourceId rid, const char* pexportName)

{
    GUNUSED(pmovieDef);

    GPtr<GImageInfoBase> pimageInfoBase;
    if (presource->GetResourceUse() == GFxResource::Use_Bitmap)
    {
        pimageInfoBase = static_cast<GFxImageResource*>(presource)->GetImageInfo();
        GASSERT(pimageInfoBase);

        Exporter.ExtractImage(pimageInfoBase, rid, Name, DestPath, pexportName);
    }
}

void GFxDataExporter::ExtractImage(GImageInfoBase* pimageInfoBase,
                                   GFxResourceId rid, const char* pswfName, const char* pdestPath, 
                                   const char* pexportName)

{
    GASSERT(pimageInfoBase);

    GFxExportImageDescr   imgDescr;
    GImageInfo* pimageInfo = static_cast<GImageInfo*>(pimageInfoBase);
    GDXTHelper::RescaleTypes rescale = Rescale;

    if (!Quiet)
    {
        ++ImagesProcessedCount;
        printf("%3d%%\b\b\b\b", int(ImagesProcessedCount*100/TotalImageCount));
    }

    UInt32 characterId = rid.GetIdValue();

    GPtr<GImage> pimage = pimageInfo->GetImage();
    GASSERT(pimage);

    imgDescr.CharacterId  = characterId;
    imgDescr.pImageInfo   = pimageInfo;
    if (pexportName)
        imgDescr.ExportName = pexportName;
    imgDescr.SwfName      = pswfName;
    imgDescr.TargetWidth  = pimage->Width;
    imgDescr.TargetHeight = pimage->Height;

    int imgFormatId;
    GImage::ImageFormat destImageFormat = pimage->Format;
    if (rid.GetIdType() == GFxResourceId::IdType_FontImage) // DynFontImage (??)
    {
        // font textures
        imgFormatId     = FontImagesFormat;
        switch(FontImagesBits)
        {
        case 24:    destImageFormat = GImage::Image_RGB_888; break;
        case 32:    destImageFormat = GImage::Image_ARGB_8888; break;
        }
        rescale = GDXTHelper::RescaleNone; // turn of rescale for font textures.
    }
    else if (rid.GetIdType() == GFxResourceId::IdType_GradientImage)
    {
        // gradient textures
        imgFormatId     = GradientImagesFormat;
        destImageFormat = GImage::Image_ARGB_8888;
        rescale = GDXTHelper::RescaleNone; // turn of rescale for gradient textures.
    }
    else
        imgFormatId     = GetImageFormatId();

    imgDescr.Format = UInt16(imgFormatId);

    GFxImageExporter* pimageExporter = GetImageExporter(imgFormatId);
    GASSERT(pimageExporter);

    char buf[1024];
    if (pexportName != 0)
    {
        gfc_sprintf(buf, 1024, "%s", pexportName);
    }
    else
    {
        char idstr[20];
        rid.GenerateIdString(idstr);
        gfc_sprintf(buf, 1024, "%s_%s", ((Prefix.GetLength() == 0) ? pswfName : Prefix.ToCStr()), idstr);
    }
    GFxImageExporterParams& ieParams = pimageExporter->InitParams(characterId, this, &imgDescr, pimage);
    imgDescr.ImageName = pimageExporter->MakeFileName(buf);
    GFxString destPath = pimageExporter->MakePath(pdestPath, buf);

    ieParams.Rescale = rescale;

    ImageDescriptors.set(characterId, imgDescr);
    GFxString tempPath = destPath;
    if (ShareImages)
        tempPath += ".$$$";

    if(JustStripImages && (rid.GetIdType() == 0)) //?
    {
        // only strip images, do not write them on disk
        return;
    }

    GPtr<GFile> pfile = *new GSysFile(tempPath.ToCStr(),
        GFile::Open_Write | GFile::Open_Truncate | GFile::Open_Create);
    if (!pfile->IsValid() || !pfile->IsWritable())
    {
        fprintf(stderr, "Error: Can't open file '%s' for writing\n", tempPath.ToCStr());
        return;
    }
    ieParams.pFile = pfile;

    if (ieParams.Rescale != GDXTHelper::RescaleNone && pimageExporter->MightBeRescaled())
    {
        GPtr<GImage> pnewimage = *GDXTHelper::Rescale
            (ieParams.pImage, ieParams.Rescale, RescaleFilter, 
             imgDescr.TargetWidth, imgDescr.TargetHeight, destImageFormat);
        if (pnewimage)
            ieParams.pImage = pnewimage;
        else
        {
            fprintf(stderr, "Can't rescale image: '%s', error: '%s'\n", tempPath.ToCStr(), GDXTHelper::GetLastErrorString());
        }
    }

    if (pimageExporter->NeedToBeConverted(ieParams.pImage, destImageFormat))
    {
        GPtr<GImage> pnewimage = *ieParams.pImage->ConvertImage(destImageFormat);
        if (pnewimage)
            ieParams.pImage = pnewimage;
        else
        {
            fprintf(stderr, "Can't rescale image: '%s', error: '%s'\n", tempPath.ToCStr(), GDXTHelper::GetLastErrorString());
        }
    }

    pimageExporter->Write();
    pfile->Close();
    
    if (ShareImages)
    {
        GFxExportImageDescr* pdescr = ImageDescriptors.get(characterId);
        if (FindIdenticalImage(destPath, tempPath, pdescr, pimageExporter))
        {
            remove(tempPath);
            ++SharedImagesCount;
        }
        else
        {
            remove(destPath.ToCStr());
            rename(tempPath.ToCStr(), destPath.ToCStr());
            ++ImagesWrittenCount;
        }
    }
    else
        ++ImagesWrittenCount;
}

bool GFxDataExporter::FindIdenticalImage(const GFxString& imagePath, const GFxString& filePath, GFxExportImageDescr* pdescr, GFxImageExporter* pimageExporter)
{
    GASSERT(pimageExporter);

    GPtr<GImage> pimage = *pimageExporter->Read(filePath.ToCStr());
    if (pimage)
    {
        UPInt hashCode = pimage->ComputeHash();
        GTL::garray<GFxString>* pfileNamesArr = SharedImagesHash.get(hashCode);

        if (pfileNamesArr)
        {
            for (UInt i = 0, n = pfileNamesArr->size(); i < n; ++i)
            {
                GFxString& fname = (*pfileNamesArr)[i];
                GPtr<GImage> pcmpimage = *pimageExporter->Read(fname);
                if (pcmpimage)
                {
                    if (*pimage == *pcmpimage)
                    {
                        pdescr->ImageName = CutPath(fname);
                        return true;
                    }
                }
            }
        }
        const char* pext = strrchr(imagePath.ToCStr(), '.');
        if (pext != NULL)
        {
            ++pext;
        }
        else
            pext = pimageExporter->GetDefaultExtension();
        GASSERT(pext);

        GFxString wildCard = imagePath;
        const char* pslash1 = strrchr(imagePath, '\\');
        const char* pslash2 = strrchr(imagePath, '/');
        if (pslash1 > pslash2)
            wildCard.Resize(pslash1 - imagePath + 1);
        else if (pslash2)
            wildCard.Resize(pslash2 - imagePath + 1);
        else
            wildCard = "./"; // current path
        GFxString path = wildCard;
        wildCard += "*.";
        wildCard += pext;

        // search the path
        WIN32_FIND_DATAA ffd;
        HANDLE fh = ::FindFirstFile(wildCard.ToCStr(), &ffd);
        if (fh!=INVALID_HANDLE_VALUE)
        {
            do
            {
                // skip subdirectories
                if (!(ffd.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY))
                {
                    // add the file to the list
                    GFxString fname = path;
                    fname += ffd.cFileName;
                    if (fname == filePath || SharedImageNamesSet.get(fname) != NULL)
                        continue;

                    GPtr<GImage> pcmpimage = *pimageExporter->Read(fname);
                    if (pcmpimage)
                    {
                        UPInt hashCode = pcmpimage->ComputeHash();
                        GTL::garray<GFxString>* pfileNamesArr = SharedImagesHash.get(hashCode);

                        // cache hash-filename assoc.
                        if (pfileNamesArr)
                        {
                            pfileNamesArr->push_back(fname);
                        }
                        else
                        {
                            GTL::garray<GFxString> fileNamesArr;
                            fileNamesArr.push_back(fname);
                            SharedImagesHash.add(hashCode, fileNamesArr);
                        }
                        SharedImageNamesSet.set(fname, 0);

                        if (*pimage == *pcmpimage)
                        {
                            pdescr->ImageName = CutPath(fname);
                            return true;
                        }
                    }
                }
            } while (::FindNextFile(fh,&ffd));
            ::FindClose(fh);
        }
    }
    return false;
}

int GFxDataExporter::Load(const GFxString& fileName)
{
    TagsToBeRemoved.resize(0);
    ImageDescriptors.clear();
    SharedImagesHash.clear();
    SharedImageNamesSet.clear();
    ClearOriginalImageData();
    FirstTagOffset = 0;

    // Get info about the width & height of the movie.
    if (!Loader.GetMovieInfo(fileName, &MovieInfo))
    {
        fprintf(stderr, "\nError: Failed to get info about '%s'\n", fileName.ToCStr());
        return 0;
    }

    // Do not allow stripped files as input, since they would cause a crash.
    if (MovieInfo.IsStripped())
    {
        fprintf(stderr, "\nError: '%s' is already stripped, gfxexport requires SWF files\n",
                fileName.ToCStr());
        return 0;
    }

    GPtr<GFxFontPackParams> pfontPackParams = Loader.GetFontPackParams();
    GASSERT(pfontPackParams);
    GFxFontPackParams::TextureConfig textureGlyphCfg;
    textureGlyphCfg.TextureWidth     = FontTextureWidth;
    textureGlyphCfg.TextureHeight    = FontTextureHeight;
    textureGlyphCfg.NominalSize      = TextureGlyphNominalSize;
    textureGlyphCfg.PadPixels        = TextureGlyphPadPixels;

    pfontPackParams->SetTextureConfig(textureGlyphCfg);
    pfontPackParams->SetUseSeparateTextures(UseSeparateFontTextures);

    GFxTextureGlyphData::TextureConfig actualCfg;
    pfontPackParams->GetTextureConfig(&actualCfg);
    if (actualCfg.NominalSize != TextureGlyphNominalSize)
    {
        TextureGlyphNominalSize = actualCfg.NominalSize;
        fprintf(stderr, "Warning: Glyph size is corrected to %d\n", TextureGlyphNominalSize);
    }

    if (ExportGradients)
    {
        if (GradientSize > 0)
        {
            GPtr<GFxGradientParams> pgrParams = *new GFxGradientParams(GradientSize);
            Loader.SetGradientParams(pgrParams);
        }
    }

    if (!Quiet)
        printf("Loading SWF file: %s - ", fileName.ToCStr());

#if 0
    // Load the actual new movie and create an instance.
    // Don't use library: this will ensure that the memory is released.
    UInt loadFlags = GFxLoader::LoadImageData;
    if (ExportFonts)
        loadFlags |= GFxLoader::LoadFontShapes|GFxLoader::LoadRenderFonts;
    if (ExportGradients)
        loadFlags |= GFxLoader::LoadRenderGradients;
#else
    // Disable import loading; import files are processed individually.
    UInt loadFlags = GFxLoader::LoadDisableImports;
#endif

    pMovieDef = *Loader.CreateMovie(fileName, loadFlags);
    if (!pMovieDef)
    {
        fprintf(stderr, "\nError: Failed to create a movie from '%s'\n", fileName.ToCStr());
        return 0;
    }

    if (!Quiet)
    {
        printf("\n");
    }
    return 1;
}

void GFxDataExporter::ProgressCallback(const GFxString& fileURL, const GFxTagInfo& tagInfo,
                                       UInt bytesLoaded, UInt totalBytes)
{
    GUNUSED(fileURL);
    GFxDataExporter* pthis = this;

    if (!pthis->Quiet)
    {
        printf("%3d%%\b\b\b\b", int(bytesLoaded*100/totalBytes));
    }

    // save offset of the very first tag
    if (pthis->FirstTagOffset == 0)
    {
        pthis->FirstTagOffset = tagInfo.TagOffset;
    }

    if (tagInfo.TagType == 12 ||  // DoAction
        tagInfo.TagType == 59 ||  // DoInitAction
        tagInfo.TagType == 7  ||  // DefineButton
        tagInfo.TagType == 34  || // DefineButton2
        tagInfo.TagType == 39  || // DefineSprite
        tagInfo.TagType == 26  || // PlaceObject2
        tagInfo.TagType == 70)    // PlaceObject3
        pthis->TagsWithActions.push_back(tagInfo);

    if (tagInfo.TagType == 6 ||     // DefineBits
        tagInfo.TagType == 8 ||     // JPEGTables
        tagInfo.TagType == 21 ||    // DefineBitsJPEG2
        tagInfo.TagType == 35 ||    // DefineBitsJPEG3
        tagInfo.TagType == 20 ||    // DefineBitsLossless
        tagInfo.TagType == 36 ||    // DefineBitsLossless2
        tagInfo.TagType == 0)       // End
    {
        pthis->TagsToBeRemoved.push_back(tagInfo);
    }
    else if (ExportFonts && (
        tagInfo.TagType == 10 ||    // DefineFont
        tagInfo.TagType == 48 ||    // DefineFont2
        tagInfo.TagType == 75))     // DefineFont3
    {
        pthis->TagsToBeRemoved.push_back(tagInfo);
    }
    else
    {
        // schedule removal for all tags w/o loaders
        if (!pthis->Loader.DoesNeedTag(tagInfo.TagType))
        {
            //printf("Going to remove tag = %d\n", tagInfo.TagType);
            pthis->TagsToBeRemoved.push_back(tagInfo);
        }
    }
}


int GFxDataExporter::GetImageFormatId() const
{
    const GFxString format = ImagesFormat;
    GTL::ghash<int, GFxImageExporter*>::const_iterator iter = ImageExporters.begin();
    for(; iter != ImageExporters.end(); ++iter)
    {
        if (format.CompareNoCase(iter->second->GetName()) == 0)
            return iter->first;
    }
    return GFxFileConstants::File_Unknown;
}

static void WriteTagHeader(GFile* pout, UInt tag, UInt len)
{
    GASSERT(tag < 1024);
    if (len < 63)
    {
        // short form of tag
        pout->WriteUInt16(UInt16((tag << 6) | (len & 0x3F)));
    }
    else
    {
        // long form of tag
        pout->WriteUInt16(UInt16(tag << 6) | 0x3F);
        pout->WriteUInt32(UInt32(len));
    }
}

void GFxDataExporter::WriteExternalImageTag(GFile* pout, UInt32 characterId, const GFxExportImageDescr& imgDescr)
{
    // utilizes the tag 1001 (unused in normal SWF): the format is as follows:
    // Header           RECORDHEADER    1001
    // CharacterID      UI16
    // BitmapFormat     UI16            0 - Default, as in 1001 tag
    //                                  1 - TGA
    //                                  2 - DDS
    // TargetWidth      UI16
    // TargetHeight     UI16
    // ExportNameLen    UI8
    // ExportName       UI8[ExportNameLen]
    // FileNameLen      UI8             with extension
    // FileName         UI8[FileNameLen]

    // determine long or short form of tag
    UByte len = UByte(2 + 2 + 2 + 2 + 1 + imgDescr.ExportName.GetSize() + 1 + imgDescr.ImageName.GetSize());
    WriteTagHeader(pout, 1001, len);
    pout->WriteUInt16(UInt16(characterId));
    pout->WriteUInt16((JustStripImages)?0:imgDescr.Format);
    pout->WriteUInt16(UInt16(imgDescr.TargetWidth));
    pout->WriteUInt16(UInt16(imgDescr.TargetHeight));

    pout->WriteUByte(UByte(imgDescr.ExportName.GetSize()));
    pout->Write((const UByte*)imgDescr.ExportName.ToCStr(), imgDescr.ExportName.GetSize());

    pout->WriteUByte(UByte(imgDescr.ImageName.GetSize()));
    pout->Write((const UByte*)imgDescr.ImageName.ToCStr(), imgDescr.ImageName.GetSize());
}

void GFxDataExporter::WriteExternalGradientImageTag(GFile* pout, UInt32 characterId, const GFxExportImageDescr& imgDescr)
{
    // utilizes the tag 1003 (unused in normal SWF): the format is as follows:
    // Header           RECORDHEADER    1003
    // GradientID       UI16
    // BitmapFormat     UI16            0 - Default, as in 1001 tag
    //                                  1 - TGA
    //                                  2 - DDS
    // GradientSize     UI16
    // FileNameLen      UI8             without extension, only name
    // FileName         UI8[FileNameLen]
    // determine long or short form of tag
    UByte len = UByte(2 + 2 + 2 + 1 + imgDescr.ImageName.GetSize());
    WriteTagHeader(pout, 1003, len);
    pout->WriteUInt16(UInt16(characterId));
    pout->WriteUInt16(imgDescr.Format);
    pout->WriteUInt16(UInt16(imgDescr.TargetWidth));
    pout->WriteUByte(UByte(imgDescr.ImageName.GetSize()));
    pout->Write((const UByte*)imgDescr.ImageName.ToCStr(), imgDescr.ImageName.GetSize());
}

void GFxDataExporter::WriteGlyphTextureInfo(GFile* pout, UInt textureId, const GFxFontTextureDescr& textureDescr)
{
    GPtr<GFxFontPackParams> pfontPackParams = Loader.GetFontPackParams();
    GASSERT(pfontPackParams);
    const GFxExportImageDescr* descr = ImageDescriptors.get(textureId);

    // Glyphs' texture info tags
    // utilizes the tag 1002 (unused in normal SWF): the format is as follows:
    // Header           RECORDHEADER    1002
    // TextureID        UI32            Texture ID
    // TextureFormat    UI16            0 - Default, as in 1001 tag
    //                                  1 - TGA
    //                                  2 - DDS
    // FileNameLen      UI8             name of file with texture's image (without extension)
    // FileName         UI8[FileNameLen]
    // TextureWidth     UI16
    // TextureHeight    UI16
    // PadPixels        UI8             
    // NominalGlyphSz   UI16            Nominal height of glyphs
    // NumTexGlyphs     UI16            Number of texture glyphs
    // TexGlyphs        TEXGLYPH[NumTexGlyphs]
    // NumFonts         UI16            Number of fonts using this texture
    // Fonts            FONTINFO[NumFonts]  Font infos
    //
    // FONTINFO
    // FontId           UI16
    // NumGlyphs        UI16            Number of texture glyphs in the font from the current texture
    // GlyphIndices     GLYPHIDX[NumGlyphs] Mapping of font glyph's indices to textures' ones (TexGlyphs)
    //
    // GLYPHIDX
    // IndexInFont      UI16            Index in font
    // IndexInTexture   UI16            Index in texture
    //
    // TEXGLYPH:
    // UvBoundsLeft     FLOAT
    // UvBoundsTop      FLOAT
    // UvBoundsRight    FLOAT
    // UvBoundsBottom   FLOAT
    // UvOriginX        FLOAT
    // UvOriginY        FLOAT

    // count number of texture glyphs
    const UInt numTextureGlyphs = textureDescr.TextureGlyphs.size();
    UInt i, n;

    // short form of tag
    UInt32 len = UInt32(4 + 2 + 1 + descr->ImageName.GetSize() + 2 + 2 + 1 + 2 + 2 + (numTextureGlyphs * 6 * 4) + 2 + textureDescr.Fonts.size()*(2 + 2) + numTextureGlyphs*(2+2));
    WriteTagHeader(pout, 1002, len);

    pout->WriteUInt32(UInt32(textureId));           // UI32 - textureId

    pout->WriteUInt16(UInt16(FontImagesFormat));
    pout->WriteUByte(UByte(descr->ImageName.GetSize()));
    pout->Write((const UByte*)descr->ImageName.ToCStr(), descr->ImageName.GetSize());

    pout->WriteUInt16(UInt16(textureDescr.pTexture->GetWidth()));
    pout->WriteUInt16(UInt16(textureDescr.pTexture->GetHeight()));

    GFxFontPackParams::TextureConfig texGlyphCfg;
    pfontPackParams->GetTextureConfig(&texGlyphCfg);

    pout->WriteUInt8 (UInt8 (texGlyphCfg.PadPixels));
    pout->WriteUInt16(UInt16(texGlyphCfg.NominalSize));

    pout->WriteUInt16(UInt16(numTextureGlyphs));
    for (i = 0, n = numTextureGlyphs; i < n; ++i)
    {
        GPtr<GFxTextureGlyph> ptextureGlyph = textureDescr.TextureGlyphs[i];

        // save TEXGLYPH
        pout->WriteFloat(ptextureGlyph->UvBounds.Left);
        pout->WriteFloat(ptextureGlyph->UvBounds.Top);
        pout->WriteFloat(ptextureGlyph->UvBounds.Right);
        pout->WriteFloat(ptextureGlyph->UvBounds.Bottom);

        pout->WriteFloat(ptextureGlyph->UvOrigin.x);
        pout->WriteFloat(ptextureGlyph->UvOrigin.y);
    }

    pout->WriteUInt16(UInt16(textureDescr.Fonts.size()));   // UI16 - NumFonts
    for (i = 0, n = textureDescr.Fonts.size(); i < n; ++i)
    {
        // save FONTINFO
        const GFxFontTextureDescr::FontDescr& fntDescr = textureDescr.Fonts[i];
        pout->WriteUInt16(UInt16(fntDescr.FontId));
        pout->WriteUInt16(UInt16(fntDescr.TexGlyphsIndicesMap.size()));

        for(UInt i = 0, n = fntDescr.TexGlyphsIndicesMap.size(); i < n; ++i)
        {
            const GFxFontTextureDescr::FontDescr::Pair& assoc = fntDescr.TexGlyphsIndicesMap[i];
            pout->WriteUInt16(UInt16(assoc.IndexInFont));
            pout->WriteUInt16(UInt16(assoc.IndexInTexture));
        }
    }

}

void GFxDataExporter::WriteStrippedSwf(const char* srcFileName, const char* dstFileName, const GFxString& name)
{
    GPtr<GFile> pin = *new GSysFile(srcFileName, GFile::Open_Read);
    if (!pin || !pin->IsValid())
    {
        fprintf(stderr, "Error: Can't open source file '%s' to read from\n", srcFileName);
        return;
    }

    // load header
    UInt32 header          = pin->ReadUInt32();
    UInt32 fileLength      = pin->ReadUInt32();
    UByte  version         = UByte((header >> 24) & 255);
    bool   compressed      = (header & 255) == 'C';
    if (compressed)
    {
        pin = *new GZLibFile(pin);
    }

    GPtr<GFile> pout = *new GSysFile(dstFileName,
        GFile::Open_Write | GFile::Open_Truncate | GFile::Open_Create);
    if (!pout || !pout->IsWritable())
    {
        fprintf(stderr, "Error: Can't open destination file '%s' to write to\n", dstFileName);
        return;
    }

    // write new header
    static const UByte gfxHeader[] = {'G', 'F', 'X' };
    static const UByte cfxHeader[] = {'C', 'F', 'X' };
    if (!DoCompress)
        pout->Write(gfxHeader, 3);
    else
        pout->Write(cfxHeader, 3);
    pout->WriteUByte(version);
    pout->WriteUInt32(fileLength); // will need to be modified later

    // copy remaining part of header
    SInt hdrsize = FirstTagOffset - pin->Tell();
    if (pout->CopyFromStream(pin, hdrsize) == -1)
    {
        fprintf(stderr, "Error: Failed to copy %d bytes from %s to %s\n",
            hdrsize, pin->GetFilePath(), pout->GetFilePath());
        return;
    }

    // write extra-tag with helper info
    // Utilizes the tag 1000 (unused in normal SWF): the format is as follows:
    // Header           RECORDHEADER    1000
    // Tool ver         UI16            Version 1.00 will be encoded as 0x100 value
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
    UInt tlen = 2 + 4 + 2 + 1 + 1 + Prefix.GetSize() + name.GetSize();
    WriteTagHeader(pout, 1000, tlen);

    pout->WriteUInt16(GFX_EXPORT_VERSION);

    UInt32 flags = 0;
    if (ExportFonts)     flags |= GFxExporterInfo::EXF_GlyphTexturesExported;
    if (ExportGradients) flags |= GFxExporterInfo::EXF_GradientTexturesExported;
    if (GlyphsStripped)  flags |= GFxExporterInfo::EXF_GlyphsStripped;
    pout->WriteUInt32(flags);

    int format = GetImageFormatId();
    pout->WriteUInt16(UInt16(format));
    pout->WriteUByte(UByte(Prefix.GetSize()));
    pout->Write((const UByte*)Prefix.ToCStr(), Prefix.GetSize());
    pout->WriteUByte(UByte(name.GetSize()));
    pout->Write((const UByte*)name.ToCStr(), name.GetSize());
    /////////////////

    // Write gradients' info first. It should be written BEFORE all tags, which may use
    // gradients.
    if (ExportGradients)
    {
        // iterate through ImageDescriptors, find all gradients (charId & 0x20000 != 0)
        // and write image file tag info (1001).
        GradientTexturesHashType::const_iterator iter = GradientTextures.begin();
        for (; iter != GradientTextures.end(); ++iter)
        {
            const GFxGradientTextureDescr& gtDescr = iter->second;
            const GFxExportImageDescr* pimgDescr = ImageDescriptors.get(gtDescr.MasterId);
            GASSERT(pimgDescr);
            WriteExternalGradientImageTag(pout, gtDescr.MasterId, *pimgDescr);
            for (UInt i = 0, n = gtDescr.SlaveIds.size(); i < n; ++i)
            {
                WriteExternalGradientImageTag(pout, gtDescr.SlaveIds[i], *pimgDescr);
            }
        }
    }

    UInt32 prevOffset = pin->Tell();
    for (UInt i = 0, n = TagsToBeRemoved.size(); i < n; ++i)
    {
        const GFxTagInfo& tag = TagsToBeRemoved[i];

        SInt size = tag.TagOffset - prevOffset;
        if (pout->CopyFromStream(pin, size) == -1)
        {
            fprintf(stderr, "Error: Failed to copy %d bytes from %s to %s\n",
                    size, pin->GetFilePath(), pout->GetFilePath());
            return;
        }
        prevOffset = tag.TagOffset;

        // skip tag header
        pin->Seek(tag.TagDataOffset);

        UInt32 characterId = ~0u;
        // read character id, if exists
        if (tag.TagType == 6 ||     // DefineBits
            tag.TagType == 21 ||    // DefineBitsJPEG2
            tag.TagType == 35 ||    // DefineBitsJPEG3
            tag.TagType == 20 ||    // DefineBitsLossless
            tag.TagType == 36)      // DefineBitsLossless2
        {
            characterId = pin->ReadUInt16();
        }
        else if (tag.TagType == GFxTag_DefineFont ||
            tag.TagType == GFxTag_DefineFont2 ||
            tag.TagType == GFxTag_DefineFont3)
        {
            UInt16 fontId = pin->ReadUInt16();
            if (GlyphsStripped)
            {
                if (tag.TagType == GFxTag_DefineFont)    // DefineFont
                {
                    UInt16 offset0 = pin->ReadUInt16();
                    UInt len = 2 + 2 + ((offset0 > 0) ? 2 : 0);
                    WriteTagHeader(pout, tag.TagType, len);

                    pout->WriteUInt16(fontId);
                    pout->WriteUInt16(offset0); // first offset, indicates num of glyphs
                    if (offset0 > 0) pout->WriteUInt16(0); // end indicator
                }
                else
                {
                    UByte  flags   = pin->ReadUByte();
                    UByte  langCode = pin->ReadUByte(); // Language code
                    UByte  nameLen  = pin->ReadUByte();
                    UByte  name[256];
                    if (pin->Read(name, nameLen) != nameLen)
                    {
                        fprintf(stderr, "Error: Can't read from '%s'\n", pin->GetFilePath());
                        pin->Seek(tag.TagOffset); // revert back
                        continue;
                    }
                    UInt16 numGlyphs = pin->ReadUInt16();
                    if (numGlyphs == 0) // do not change font tags with no glyphs
                    {
                        pin->Seek(tag.TagOffset); // revert back
                        continue;
                    }

                    int tableBase    = pin->Tell();

                    // write null as the first entry of offset table - means there are no
                    // offsets in it. Skip offset table in source SWF
                    pin->Skip(((flags & 0x8) ? 4 : 2) * numGlyphs); // take into account wide or not wide offsets

                    UInt32 codeTableOffset;
                    if (flags & 0x8) // wide offsets
                    {
                        codeTableOffset = pin->ReadUInt32();
                    }
                    else
                    {
                        codeTableOffset = pin->ReadUInt16();
                    }
                    // skip shapes
                    pin->Seek(tableBase + codeTableOffset);

                    // now we need to calculate the size of the remaining part of DefineFont2/3 record:
                    // it will be written into the new tag without any modification
                    SInt remainingLen = (tag.TagDataOffset + tag.TagLength) - pin->Tell();
                    GASSERT(remainingLen >= 0);

                    // now we can start writing a new tag. Calculate its length first.
                    UInt len = 2 + 1 + 1 + 1 + nameLen + 2 /* numglyphs */ + 2 /* off */ + 2 /* code off */ + remainingLen;

                    WriteTagHeader(pout, tag.TagType, len);

                    // reset "wide offset" flag
                    flags &= (~0x8);

                    pout->WriteUInt16(fontId);
                    pout->WriteUByte(flags);
                    pout->WriteUByte(langCode);
                    pout->WriteUByte(nameLen);
                    if (pout->Write(name, nameLen) != nameLen)
                    {
                        fprintf(stderr, "Error: Can't write to '%s'\n", pout->GetFilePath());
                        return;
                    }
                    pout->WriteUInt16(numGlyphs);
                    pout->WriteUInt16(0); // offset table
                    pout->WriteUInt16(4); // code table offset is always 4 with stripped shapes
                    if (pout->CopyFromStream(pin, remainingLen) == -1)
                    {
                        fprintf(stderr, "Error: Failed to copy %d bytes from %s to %s\n",
                            remainingLen, pin->GetFilePath(), pout->GetFilePath());
                        return;
                    }
                }
            }
            else
            {
                // no stripping
                // just copy the tag unchanged
                pin->Seek(tag.TagOffset);
                if (pout->CopyFromStream(pin, tag.TagLength + (tag.TagDataOffset - tag.TagOffset)) == -1)
                {
                    fprintf(stderr, "Error: Failed to copy %d bytes from %s to %s\n",
                        size, pin->GetFilePath(), pout->GetFilePath());
                    return;
                }
            }
            // check, can we write glyph texture info or not yet
            if (ExportFonts)
            {
                FontTextureCounterHashType* ptexCounter = FontTextureUse.get(fontId);
                if (ptexCounter)
                {
                    FontTextureCounterHashType::iterator it = ptexCounter->begin();
                    for (; it != ptexCounter->end(); ++it)
                    {
                        GASSERT(it->second->Counter > 0);
                        --it->second->Counter;
                        if (it->second->Counter == 0)
                        {
                            const GFxFontTextureDescr* ptextureDescr = FontTextures.get(it->first);
                            if (ptextureDescr)
                            {
                                WriteGlyphTextureInfo(pout, it->first, *ptextureDescr);
                            }
                        }
                    }
                }
            }
        }

        // skip tag content
        pin->Seek(tag.TagDataOffset + tag.TagLength);
        prevOffset = tag.TagDataOffset + tag.TagLength;

        if (characterId != ~0u)
        {
            // write new tag
            const GFxExportImageDescr* pdescr = ImageDescriptors.get(characterId);
            GASSERT(pdescr);

            WriteExternalImageTag(pout, characterId, *pdescr);
        }
    }
    // we don't need to finish copying since the last tag in TagsToBeRemoved
    // should be an end-tag.

    pin->Close();

    // write end tag
    pout->WriteUInt16(UInt16((0 << 6) | (0 & 0x3F)));

    // correct FileLength field in SWF header
    UInt32 totalLen = pout->Tell();
    pout->Seek(4);
    pout->WriteUInt32(totalLen);

    // force out file to be closed
    pout->Close();

    if (DoCompress)
    {
        // compress just written file starting from offset 8
        GPtr<GFile> pin = *new GSysFile(dstFileName, GFile::Open_Read);
        if (!pin || !pin->IsValid())
        {
            fprintf(stderr, "Error: Can't open source file '%s' to read from\n", dstFileName);
            return;
        }

        GFxString tmpFileName = dstFileName;
        tmpFileName += "###";
        GPtr<GFile> pout = *new GSysFile(tmpFileName,
            GFile::Open_Write | GFile::Open_Truncate | GFile::Open_Create);
        if (!pout || !pout->IsWritable())
        {
            fprintf(stderr, "Error: Can't open destination temporary file '%s' to write to\n", tmpFileName.ToCStr());
            return;
        }

        // Copy the header bits
        if (pout->CopyFromStream(pin, 8) == -1)
        {
            fprintf(stderr, "Error: Failed to copy %d bytes from %s to %s\n",
                    8, pin->GetFilePath(), pout->GetFilePath());
            return;
        }

        // read file into a buffer
        SInt lenToCompress = SInt(totalLen - 8);

        z_stream zstream;
        memset(&zstream, 0, sizeof(zstream));
        deflateInit(&zstream, 9);

        while (lenToCompress > 0)
        {
            UByte buffer[4096];
            UByte outBuff[8192];

            SInt toRead = GTL::gmin(lenToCompress, SInt(sizeof(buffer)));
            if (pin->Read(buffer, toRead) != toRead)
            {
                fprintf(stderr, "Error: Can't read from file '%s'\n", dstFileName);
                break;
            }
            lenToCompress -= toRead;

            // compress the buffer
            zstream.next_in = buffer;
            zstream.avail_in = sizeof(buffer);
            int ret;
            do {
                zstream.next_out = outBuff;
                zstream.avail_out = sizeof(outBuff);
                ret = deflate(&zstream, ((lenToCompress == 0) ? Z_FINISH : Z_NO_FLUSH));

                SInt toWrite = SInt(sizeof(outBuff) - zstream.avail_out);
                if (pout->Write(outBuff, toWrite) != toWrite)
                {
                    fprintf(stderr, "Error: Can't write in file '%s'\n", tmpFileName.ToCStr());
                    lenToCompress = 0;
                    break;
                }
            } while(zstream.avail_out == 0 && ret != Z_STREAM_END);
        }
        deflateEnd(&zstream);
        pout->Close();
        pin->Close();

        // rename temp file into dstFileName
        remove(dstFileName);
        rename(tmpFileName, dstFileName);
    }
}

bool GFxDataExporter::CreateDestDirectory(const char* path)
{
    // get cur dir
    char buf[1024];
    if (gfc_getcwd(buf, sizeof(buf)) == 0)
        return false;

    // check path existence
    if (gfc_chdir(path) == 0)
    {
        // path exists
        gfc_chdir(buf);
        return true;
    }
    // create dir
    if (gfc_mkdir(path) == 0)
        return true;
    return false;
}

int GFxDataExporter::CollectOriginalImageData(const char* srcFileName)
{
    GPtr<GFile> pin = *new GSysFile(srcFileName, GFile::Open_Read);
    if (!pin || !pin->IsValid())
    {
        fprintf(stderr, "Error: Can't open source file '%s' to read from\n", srcFileName);
        return 0;
    }

    // load header
    UInt32 header          = pin->ReadUInt32();
    pin->ReadUInt32(); //fileLength
    bool   compressed      = (header & 255) == 'C';
    if (compressed)
    {
        pin = *new GZLibFile(pin);
    }

    for (UInt i = 0, n = TagsToBeRemoved.size(); i < n; ++i)
    {
        const GFxTagInfo& tag = TagsToBeRemoved[i];

        // skip tag header
        pin->Seek(tag.TagDataOffset);

        int characterId = -1;
        // read character id, if exists
        if (tag.TagType == 6 ||     // DefineBits
            tag.TagType == 21 ||    // DefineBitsJPEG2
            tag.TagType == 35 ||    // DefineBitsJPEG3
            tag.TagType == 20 ||    // DefineBitsLossless
            tag.TagType == 36)      // DefineBitsLossless2
        {
            characterId = pin->ReadUInt16();
        }

        if (characterId >= 0 && (tag.TagType == 6 || tag.TagType == 21 || tag.TagType == 35))
        {
            // save original JPEG data
            JpegDesc jpegDesc;
            jpegDesc.TagType = tag.TagType;
            if (tag.TagType == 35)
            {
                jpegDesc.DataSize = pin->ReadUInt32();
            }
            else
            {
                jpegDesc.DataSize = tag.TagDataOffset + tag.TagLength - pin->Tell();
            }
            jpegDesc.pData = (UByte*)GALLOC(jpegDesc.DataSize);
            if (pin->Read(jpegDesc.pData, SInt(jpegDesc.DataSize)) != SInt(jpegDesc.DataSize))
            {
                fprintf(stderr, "Error: Failed to read %d bytes from %s\n",
                    jpegDesc.DataSize, pin->GetFilePath());
                GFREE(jpegDesc.pData);
                return 0;
            }
            JpegDescriptors.set(characterId, jpegDesc);
        }
        else if (tag.TagType == 8)
        {
            // special case for JPEGTables
            JpegDesc jpegDesc;
            jpegDesc.TagType = tag.TagType;
            jpegDesc.DataSize = tag.TagDataOffset + tag.TagLength - pin->Tell();
            jpegDesc.pData = (UByte*)GALLOC(jpegDesc.DataSize);
            if (pin->Read(jpegDesc.pData, SInt(jpegDesc.DataSize)) != SInt(jpegDesc.DataSize))
            {
                fprintf(stderr, "Error: Failed to read %d bytes from %s\n",
                    jpegDesc.DataSize, pin->GetFilePath());
                GFREE(jpegDesc.pData);
                return 0;
            }
            JpegDescriptors.set(-1, jpegDesc);
        }
        pin->Seek(tag.TagDataOffset + tag.TagLength);
    }
    return 1;
}

void GFxDataExporter::ClearOriginalImageData()
{
    GTL::ghash<int,JpegDesc>::const_iterator i = JpegDescriptors.begin();

    for(; i != JpegDescriptors.end(); ++i)
    {
        const JpegDesc& desc = i->second;
        GFREE(desc.pData);
    }
    JpegDescriptors.clear();
}

// Computes a hash of a string-like object (something that has ::GetLength() and ::[int]).
template<class T>
class GImageHashFunctor
{
public:
    size_t  operator()(const T& data) const
    {
        GASSERT(data.pImage);
        return data.pImage->ComputeHash();
    }
};

struct GImageKey
{
    GPtr<GImage> pImage;

    GImageKey(GImage* p) : pImage(p) {}

    bool operator==(const GImageKey& s) const
    {
        return *pImage == *s.pImage;
    }
};

typedef GTL::ghash<GImageKey,int,GImageHashFunctor<GImageKey> > GImageHashSet;

int GFxDataExporter::Process()
{
    if (!Quiet) ShowVersion();

    GFxString destPath;
    if (OutputRootDir.GetSize() != 0)
    {
        destPath = OutputRootDir;
        if (OutputRootDir[OutputRootDir.GetSize() - 1] != '\\' &&
            OutputRootDir[OutputRootDir.GetSize() - 1] != '/')
            destPath += "/";
    }

    for (UInt i = 0, n = SrcFileNames.size(); i < n; ++i)
    {
        if (!Load(SrcFileNames[i]))
            continue;

        int format = GetImageFormatId();
        if (format == GFxFileConstants::File_Original)
        {
            // original format is requested. Need to traverse through original source SWF,
            // scratch out image data. JPEG data will be saved as JPG. JPEG with alpha
            // will be saved as JPEG + TGA, lossless images - TGA
            CollectOriginalImageData(SrcFileNames[i]);
        }

        // Filename with extension
        GFxString fname = GFxDataExporter::CutPath(SrcFileNames[i]);
        // Filename without extension
        GFxString name  = GFxDataExporter::CutExtension(fname);

        GFxString path;

        // If the destination path was not set and the SWF is in a
        // subdirectory, use the SWFs subdirectory as the destination path
        if (destPath.GetSize() != 0)
        {
            path = destPath;
        }
        else if (fname.GetLength() != SrcFileNames[i].GetLength())
        {
            path = SrcFileNames[i];
            path.Remove(path.GetLength()-fname.GetLength(), fname.GetLength());
        }

        // Put the extracted files into a sub
        // directory with the same name as the SWF
        if (DoCreateSubDirs)
        {
            path += name;
            path += "/";
        }

        if (path.GetLength() && !CreateDestDirectory(path))
        {
            fprintf(stderr, "Failed to create or open destination path '%s'\n", path.ToCStr());
            continue;
        }

        TotalImageCount     = 0;
        ImagesWrittenCount  = 0;
        ImagesProcessedCount= 0;
        SharedImagesCount   = 0;

        // calculate total number of images
        struct ImageCounterVisitor : public GFxMovieDef::ResourceVisitor
        {
            int Count;
            ImageCounterVisitor() : Count(0) {}

            //virtual void    Visit(GFxMovieDef*, GImageInfoBase*, int, const char*) { ++Count; }
            virtual void    Visit(GFxMovieDef*, GFxResource* presource, GFxResourceId, const char*)
            {
                if (presource->GetResourceType() == GFxResource::RT_Image && 
                    presource->GetResourceUse()  == GFxResource::Use_Bitmap)
                    ++Count;
            }
        } cntVisitor;

        UInt visitMask = GFxMovieDef::ResVisit_AllLocalImages;
        //if (ExportFonts)
        //    visitMask |= GFxMovieDef::ResVisit_Fonts; //?
        //if (!ExportGradients)
        //    visitMask &= ~GFxMovieDef::ResVisit_GradientImages;

        pMovieDef->VisitResources(&cntVisitor, visitMask);
        TotalImageCount = cntVisitor.Count;

        FontTextures.clear();
        FontTextureUse.clear();
        GradientTextures.clear();

        if (ExportFonts)
        {
            // collect font textures.
            GFxMovieDefImpl* pmovieDefImpl = static_cast<GFxMovieDefImpl*>(pMovieDef.GetPtr());

            struct FontsVisitor : public GFxMovieDef::ResourceVisitor
            {
                GFxDataExporter* pExporter;
                GFxResourceBinding* pResBinding;
                
                FontsVisitor(GFxDataExporter* pexporter, GFxResourceBinding* presbinding) : 
                    pExporter(pexporter), pResBinding(presbinding) {}

                virtual void    Visit(GFxMovieDef*, GFxResource* presource,
                    GFxResourceId rid, const char*)
                {
                    if (presource->GetResourceType() == GFxResource::RT_Font)
                    {
                        GFxFontResource* pfontResource = static_cast<GFxFontResource*>(presource);
                        int fontId = rid.GetIdValue();

                        struct FontTexturesVisitor : public GFxTextureGlyphData::TexturesVisitor
                        {
                            FontTexturesHashType*               pTextures;
                            int                                 FontId;
                            GPtr<GFxFontResource>               pFont;

                            inline FontTexturesVisitor(FontTexturesHashType& textures, 
                                                       int fontId, GFxFontResource* pfont) : 
                                pTextures(&textures), FontId(fontId), pFont(pfont) {}

                            void Visit(GFxResourceId textureResId, GFxImageResource* pimageRes)
                            {
                                UInt32 textureId = (textureResId.GetIdValue() & (~GFxResourceId::IdType_Bit_TypeMask)) | GFxResourceId::IdType_FontImage;
                                FontTexturesHashType::iterator iter = pTextures->find(textureId);
                                GFxFontTextureDescr::FontDescr fontDescr;
                                fontDescr.FontId = FontId;
                                fontDescr.pFont  = pFont;
                                if (iter != pTextures->end())
                                {
                                    // just add font to already existing GFxFontTextureDescr
                                    iter->second.Fonts.push_back(fontDescr);
                                }
                                else
                                {
                                    GFxFontTextureDescr ftDescr;
                                    ftDescr.Fonts.push_back(fontDescr);
                                    ftDescr.pTexture = pimageRes->GetImageInfo();
                                    pTextures->add(textureId, ftDescr);
                                }
                            }
                        } texturesVisitor(pExporter->FontTextures, fontId, pfontResource);
                        GFxTextureGlyphData* ptextGlyphData = pfontResource->GetTextureGlyphData();
                        if (ptextGlyphData == NULL)
                            ptextGlyphData = pfontResource->GetFont()->GetTextureGlyphData();
                        if (ptextGlyphData)
                            ptextGlyphData->VisitTextures(&texturesVisitor, pResBinding);

                    }
                }
            } fontVisitor(this, &pmovieDefImpl->ResourceBinding);
            pMovieDef->VisitResources(&fontVisitor, GFxMovieDef::ResVisit_Fonts);

            TotalImageCount += FontTextures.size();

            GTL::ghash_identity<UInt, GPtr<GFxFontTextureCounter> > TexId2Counter; // TexId -> Cnt
            { // collect number of fonts per each texture
                FontTexturesHashType::iterator iter = FontTextures.begin();
                for(; iter != FontTextures.end(); ++iter)
                {
                    GFxFontTextureDescr& textureDescr = iter->second;
                    GPtr<GFxFontTextureCounter> pcnt = *new GFxFontTextureCounter(textureDescr.Fonts.size());
                    TexId2Counter.set(iter->first, pcnt);
                }
            }

            FontTexturesHashType::iterator iter = FontTextures.begin();
            for(; iter != FontTextures.end(); ++iter)
            {
                GFxFontTextureDescr& textureDescr = iter->second;

                UInt i, n;
                for (i = 0, n = textureDescr.Fonts.size(); i < n; ++i)
                {
                    GFxFontTextureDescr::FontDescr& fntDescr = textureDescr.Fonts[i];
                    GPtr<GFxFontResource> pfont = fntDescr.pFont;

                    // so, build the table of used textures, by fonts
                    FontTextureCounterHashType* ptexCounter = FontTextureUse.get((UInt16)fntDescr.FontId);
                    if (!ptexCounter)
                    {
                        FontTextureUse.set((UInt16)fntDescr.FontId, FontTextureCounterHashType());
                        ptexCounter = FontTextureUse.get((UInt16)fntDescr.FontId);
                    }
                    GASSERT(ptexCounter);
                    GPtr<GFxFontTextureCounter>* ppcnt = TexId2Counter.get(iter->first);
                    GASSERT(ppcnt);
                    ptexCounter->set(iter->first, *ppcnt);

                    // visit each texture glyph to gather all texture glyphs
                    // also creates map for each font to translate font's texture glyph
                    // indices into the texture's ones.
                    struct TextureGlyphVisitor : public GFxTextureGlyphData::TextureGlyphVisitor
                    {
                        GFxResourceBinding*             pResBinding;
                        GFxFontTextureDescr&            TextureDescr;
                        GFxFontTextureDescr::FontDescr& FntDescr;
                        UInt                            NumGlyphsInFont;

                        TextureGlyphVisitor(GFxFontTextureDescr& textureDescr, 
                                            GFxFontTextureDescr::FontDescr& fntDescr, 
                                            GFxResourceBinding* presbinding) : 
                            pResBinding(presbinding), TextureDescr(textureDescr), FntDescr(fntDescr), NumGlyphsInFont(0) {}

                        void Visit(UInt index, GFxTextureGlyph* ptextureGlyph)
                        {
                            if (ptextureGlyph->GetImageInfo(pResBinding) == TextureDescr.pTexture)
                            {
                                UInt indexInTexture = TextureDescr.TextureGlyphs.size();
                                TextureDescr.TextureGlyphs.push_back(ptextureGlyph);

                                FntDescr.TexGlyphsIndicesMap.push_back(GFxFontTextureDescr::FontDescr::Pair(index, indexInTexture));
                                ++NumGlyphsInFont;
                            }
                        }
                        TextureGlyphVisitor& operator=(const TextureGlyphVisitor&) { return *this; } // warning suppression
                    } texGlyphVisitor(textureDescr, fntDescr, &pmovieDefImpl->ResourceBinding);
                    GFxTextureGlyphData* ptextGlyphData = pfont->GetTextureGlyphData();
                    if (ptextGlyphData == NULL)
                        ptextGlyphData = pfont->GetFont()->GetTextureGlyphData();
                    if (ptextGlyphData)
                        ptextGlyphData->VisitTextureGlyphs(&texGlyphVisitor);
                }
            }
        }
        if (ExportGradients)
        {
            // collect gradients' textures
            struct GradientsVisitor : public GFxMovieDef::ResourceVisitor
            {
                GFxDataExporter* pExporter;

                GradientsVisitor(GFxDataExporter* pexporter) : pExporter(pexporter) {}

                virtual void    Visit(GFxMovieDef*, GFxResource* presource,
                    GFxResourceId rid, const char*)
                {
                    if (presource->GetResourceType() == GFxResource::RT_Image &&
                        presource->GetResourceUse()  == GFxResource::Use_Gradient)
                    {
                        GFxImageResource* pgradientRes = static_cast<GFxImageResource*>(presource);

                        UInt32 textureId = (rid.GetIdValue() & (~GFxResourceId::IdType_Bit_TypeMask)) | GFxResourceId::IdType_GradientImage;
                        GImageInfoBase* pimageInfo = pgradientRes->GetImageInfo();
                        GradientTexturesHashType::iterator iter = pExporter->GradientTextures.find(pimageInfo);
                        if (iter != pExporter->GradientTextures.end())
                        {
                            // just add font to already existing GFxFontTextureDescr
                            iter->second.SlaveIds.push_back(textureId);
                        }
                        else
                        {
                            GFxGradientTextureDescr gtDescr;
                            gtDescr.MasterId = textureId;
                            pExporter->GradientTextures.add(pimageInfo, gtDescr);
                        }
                    }
                }
            } gradientsVisitor(this);
            pMovieDef->VisitResources(&gradientsVisitor, GFxMovieDef::ResVisit_GradientImages);

            TotalImageCount += GradientTextures.size();
        }

        if (!Quiet) printf("Processing %d images - ", TotalImageCount);

        // Extract all images to the path
        if (TotalImageCount > 0)
        {
            ImageExtractorVisitor visitor(*this, name, path);
            pMovieDef->VisitResources(&visitor, visitMask);

            if (FontTextures.size() > 0)
            {
                // write font images
                FontTexturesHashType::const_iterator it = FontTextures.begin();
                for(; it != FontTextures.end(); ++it)
                {
                    UInt32 id = it->first;
                    const GFxFontTextureDescr& ftDescr = it->second;
                    ExtractImage(ftDescr.pTexture, GFxResourceId(id), name, path, NULL);
                }
            }

            if (GradientTextures.size() > 0)
            {
                // write gradient images
                GradientTexturesHashType::const_iterator it = GradientTextures.begin();
                for(; it != GradientTextures.end(); ++it)
                {
                    const GFxGradientTextureDescr& gtDescr = it->second;
                    ExtractImage(it->first, GFxResourceId(gtDescr.MasterId), name, path, NULL);
                }
            }
        }
        else
        {
            if (!Quiet) printf("100%%");
        }

        if (!Quiet) 
        {
            printf("\n");
            printf("Total images written: %d\n", ImagesWrittenCount);
            if (SharedImagesCount)
                printf("Total images shared: %d\n", SharedImagesCount);
        }

        // Create the stripped SWF file and save it as a GFX file
        if (DoStrip)
        {
            // save .gfx file
            char buf[1024];
            gfc_sprintf(buf, 1024, "%s.gfx", name.ToCStr());

            // Change the exported filename case if specified
            if (ToLowercase)
                gfc_strlwr(buf, sizeof(buf));
            else if (ToUppercase)
                gfc_strupr(buf, sizeof(buf));

            GFxString fullPath = path;
            fullPath += buf;

            if (!Quiet) printf("Saving stripped SWF file as '%s'", fullPath.ToCStr());
            WriteStrippedSwf(SrcFileNames[i], fullPath, name);
            if (!Quiet) printf("\n");
        }

        if (ExportFsCommands)
        {
            UInt mask = ((FsCommandsAsTree) ? FSTree : 0) | ((FsCommandsAsList) ? FSList : 0);
            WriteFsCommands(pMovieDef, SrcFileNames[i], path, name, mask);
        }

        if (ExportDefaultEditText)
        {
            WriteDefaultEditTextFieldText(pMovieDef, SrcFileNames[i], path, name);
        }

        if (ExportFontList)
        {
            WriteFontList(pMovieDef, SrcFileNames[i], path, name);
        }
    }
    //if (!Quiet) printf("\n");
    return 1;
}

// Helper function which strips tags from HTML.
void StripHtmlTags(GFxString *presult, const char *ptext)
{
    UInt32 code;
    presult->Clear();

    while ( (code = GUTF8Util::DecodeNextChar(&ptext))!=0 )
    {
        // Skip HTML markup

        // Ampersand
        if (code == '&')
        {
            // &lt;
            // &gt;
            // &amp;
            // &quot;
            code = GUTF8Util::DecodeNextChar(&ptext);
            UInt32 decoded = 0;
            switch(code)
            {
            case 'l': decoded = '<'; break;
            case 'g': decoded = '>'; break;
            case 'a': decoded = '&'; break;
            case 'q': decoded = '\"'; break;
            }
            // Skip past ';'
            while (code && ((code = GUTF8Util::DecodeNextChar(&ptext)) != ';'))
                ;
            // convert code and proceeded
            code = decoded;                 
        }

        else if (code == '<')
        {
            while ((code = GUTF8Util::DecodeNextChar(&ptext))!=0)
            {
                if (code == '>')
                    break;
            }
            continue;
        }

        // Append to string.
        presult->AppendString((const wchar_t*)&code, 1);
    }           
}


void    GFxDataExporter::WriteFontList(GFxMovieDef* pmovieDef, const char* swfFileName, const GFxString& path, const GFxString& name)
{
    if (!Quiet) printf("Looking for fonts in '%s'\n", swfFileName);

    GTL::garray<GFxString> fonts;
    GTL::garray<GFxString> text;

    // resource visitor
    struct FontVisitor : public GFxMovieDef::ResourceVisitor
    {        
        GTL::garray<GFxString>& Fonts;

        FontVisitor(GTL::garray<GFxString>& farr) : Fonts(farr) {}

        virtual void Visit(GFxMovieDef*, GFxResource* presource, GFxResourceId rid, const char*)
        {
            GFxFontResource* pfontResource = static_cast<GFxFontResource*>(presource);

            char buf[9];
            GFxString font;
            font = pfontResource->GetName();
            if (pfontResource->IsBold())
            {
                font += " - Bold";
            }
            else if (pfontResource->IsItalic())
            {
                font += " - Italic";
            }
            font += " (";
            rid.GenerateIdString(buf);
            font += buf;
            font += ")";

            Fonts.push_back(font);
        }

        FontVisitor& operator=(const FontVisitor&) { return *this; } // warning suppression
    } fontVisitor(fonts);

    // resource visitor
    struct TextFieldVisitor : public GFxMovieDef::ResourceVisitor
    {        
        GTL::garray<GFxString>& Text;

        TextFieldVisitor(GTL::garray<GFxString>& tarr) : Text(tarr) {}

        virtual void Visit(GFxMovieDef*, GFxResource* presource, GFxResourceId, const char*)
        {
            GFxEditTextCharacterDef* dyntxtresource = (GFxEditTextCharacterDef*)presource;
           
            GFxString tmp;
            tmp = "\"";
            if (dyntxtresource->IsHtml())
            {
                StripHtmlTags(&tmp, dyntxtresource->DefaultText.ToCStr());
            }
            else
            {
                tmp = dyntxtresource->DefaultText.ToCStr();
            }
            tmp += "\" - ";
            
            char buf[9];
            dyntxtresource->FontId.GenerateIdString(buf);
            tmp += buf;
            AddUniqueTextToArray(tmp);
        }

        void AddUniqueTextToArray(const GFxString& txt)
        {
            // find the string in the array
            for (UInt i=0; i < Text.size(); i++)
            {
                if (txt.CompareNoCase(Text[i]) == 0)
                    return;
            }
            Text.push_back(txt);
        }

        TextFieldVisitor& operator=(const TextFieldVisitor&) { return *this; } // warning suppression
    } tfVisitor(text);

    // visit the resources
    pmovieDef->VisitResources(&fontVisitor, GFxMovieDef::ResVisit_Fonts);
    pmovieDef->VisitResources(&tfVisitor, GFxMovieDef::ResVisit_EditTextFields);

    // setup output file
    char buf[1024];
    gfc_sprintf(buf, 1024, "%s.fnt", name.ToCStr());
    if (ToLowercase)
        gfc_strlwr(buf, sizeof(buf));
    else if (ToUppercase)
        gfc_strupr(buf, sizeof(buf));
    GFxString fullPath = path;
    fullPath += buf;    
    if (!Quiet) printf("Saving list of fonts as '%s'\n", fullPath.ToCStr());

    // write out data in UTF8
    FILE* fout;
#if defined(GFC_CC_MSVC) && (GFC_CC_MSVC >= 1400)
    fout = NULL;
    fopen_s(&fout, fullPath, "w");
#else
    fout = fopen(fullPath, "w");
#endif // defined(GFC_CC_MSVC) && (GFC_CC_MSVC >= 1400)
    if (!fout)
    {
        fprintf(stderr, "\nError: Can't open destination file '%s' to write to\n", fullPath.ToCStr());
        return;
    }

    // write-out UTF8 BOM
    fprintf(fout, "%c%c%c", 0xEF, 0xBB, 0xBF);
    for (UInt i=0; i < fonts.size(); i++)
    {
        GFxString& t = fonts[i];

        if (t.GetLength() > 0)
            fprintf(fout, "%s\n", fonts[i].ToCStr());
    }
    fprintf(fout, "\n\nTextfield default text (font id)\n\n");
    for (UInt i=0; i < text.size(); i++)
    {
        GFxString& t = text[i];

        if (t.GetLength() > 0)
            fprintf(fout, "%s\n", text[i].ToCStr());
    }

    fclose(fout);
    if (!Quiet) printf("\n");

}


void    GFxDataExporter::WriteDefaultEditTextFieldText(GFxMovieDef* pmovieDef, const char* swfFileName, const GFxString& path, const GFxString& name)
{
    if (!Quiet) printf("Looking for dynamic textfields in '%s'\n", swfFileName);

    GTL::garray<GFxString> text;

    // resource visitor
    struct EditTextFieldVisitor : public GFxMovieDef::ResourceVisitor
    {        
        GTL::garray<GFxString>& Text;
        bool DynOnly;

        EditTextFieldVisitor(GTL::garray<GFxString>& tarr, bool dynOnly = false) : Text(tarr), DynOnly(dynOnly) {}

        virtual void Visit(GFxMovieDef*, GFxResource* presource, GFxResourceId, const char*)
        {
            GFxEditTextCharacterDef* dyntxtresource = (GFxEditTextCharacterDef*)presource;

            if (!DynOnly || dyntxtresource->IsReadOnly())
            {

                // TODO: Only add unique text

                if (dyntxtresource->IsHtml())
                {
                    GFxString tmp;
                    StripHtmlTags(&tmp, dyntxtresource->DefaultText.ToCStr());
                    AddUniqueTextToArray(tmp);
                }
                else
                {
                    AddUniqueTextToArray(dyntxtresource->DefaultText);
                }
            }
        }

        void AddUniqueTextToArray(const GFxString& txt)
        {
            // find the string in the array
            for (UInt i=0; i < Text.size(); i++)
            {
                if (txt.CompareNoCase(Text[i]) == 0)
                    return;
            }
            Text.push_back(txt);
        }

        EditTextFieldVisitor& operator=(const EditTextFieldVisitor&) { return *this; } // warning suppression
    } dtfVisitor(text, DefaultDynamicTextOnly);

    // visit the resources
    pmovieDef->VisitResources(&dtfVisitor, GFxMovieDef::ResVisit_EditTextFields);

    // setup output file
    char buf[1024];
    if (DefaultDynamicTextOnly)
        gfc_sprintf(buf, 1024, "%s.ddt", name.ToCStr());
    else
        gfc_sprintf(buf, 1024, "%s.det", name.ToCStr());
    if (ToLowercase)
        gfc_strlwr(buf, sizeof(buf));
    else if (ToUppercase)
        gfc_strupr(buf, sizeof(buf));
    GFxString fullPath = path;
    fullPath += buf;    
    if (!Quiet) printf("Saving list of unique default dynamic textfield text as '%s'\n", fullPath.ToCStr());

    // write out data in UTF8
    FILE* fout;
#if defined(GFC_CC_MSVC) && (GFC_CC_MSVC >= 1400)
    fout = NULL;
    fopen_s(&fout, fullPath, "w");
#else
    fout = fopen(fullPath, "w");
#endif // defined(GFC_CC_MSVC) && (GFC_CC_MSVC >= 1400)
    if (!fout)
    {
        fprintf(stderr, "\nError: Can't open destination file '%s' to write to\n", fullPath.ToCStr());
        return;
    }

    // write-out UTF8 BOM
    fprintf(fout, "%c%c%c", 0xEF, 0xBB, 0xBF);
    for (UInt i=0; i < text.size(); i++)
    {
        GFxString& t = text[i];
        
        if (t.GetLength() > 0)
            fprintf(fout, "\"%s\"\n", text[i].ToCStr());
    }

    fclose(fout);
    if (!Quiet) printf("\n");

}

static int ParseMipFilterType(const char* pfilterName)
{
    if (gfc_stricmp(pfilterName, "point") == 0)
        return GDXTHelper::FilterPoint;
    else if (gfc_stricmp(pfilterName, "box") == 0)
        return GDXTHelper::FilterBox;
    else if (gfc_stricmp(pfilterName, "triangle") == 0)
        return GDXTHelper::FilterTriangle;
    else if (gfc_stricmp(pfilterName, "quadratic") == 0)
        return GDXTHelper::FilterQuadratic;
    else if (gfc_stricmp(pfilterName, "cubic") == 0)
        return GDXTHelper::FilterCubic;
    else if (gfc_stricmp(pfilterName, "catrom") == 0)
        return GDXTHelper::FilterCatrom;
    else if (gfc_stricmp(pfilterName, "mitchell") == 0)
        return GDXTHelper::FilterMitchell;
    else if (gfc_stricmp(pfilterName, "gaussian") == 0)
        return GDXTHelper::FilterGaussian;
    else if (gfc_stricmp(pfilterName, "sinc") == 0)
        return GDXTHelper::FilterSinc;
    else if (gfc_stricmp(pfilterName, "bessel") == 0)
        return GDXTHelper::FilterBessel;
    else if (gfc_stricmp(pfilterName, "hanning") == 0)
        return GDXTHelper::FilterHanning;
    else if (gfc_stricmp(pfilterName, "hamming") == 0)
        return GDXTHelper::FilterHamming;
    else if (gfc_stricmp(pfilterName, "blackman") == 0)
        return GDXTHelper::FilterBlackman;
    else if (gfc_stricmp(pfilterName, "kaiser") == 0)
        return GDXTHelper::FilterKaiser;
    return -1;
}

static UInt GetIntLog2(UInt32 value)
{ 
    // Binary search - decision tree (5 tests, rarely 6)
    return
        ((value <= 1U<<15) ?
        ((value <= 1U<<7) ?
        ((value <= 1U<<3) ?
        ((value <= 1U<<1) ? ((value <= 1U<<0) ? 0 : 1) : ((value <= 1U<<2) ? 2 : 3)) :
        ((value <= 1U<<5) ? ((value <= 1U<<4) ? 4 : 5) : ((value <= 1U<<6) ? 6 : 7))) :
        ((value <= 1U<<11) ?
        ((value <= 1U<<9) ? ((value <= 1U<<8) ? 8 : 9) : ((value <= 1U<<10) ? 10 : 11)) :
        ((value <= 1U<<13) ? ((value <= 1U<<12) ? 12 : 13) : ((value <= 1U<<14) ? 14 : 15)))) :
        ((value <= 1U<<23) ?
        ((value <= 1U<<19) ?
        ((value <= 1U<<17) ? ((value <= 1U<<16) ? 16 : 17) : ((value <= 1U<<18) ? 18 : 19)) :
        ((value <= 1U<<21) ? ((value <= 1U<<20) ? 20 : 21) : ((value <= 1U<<22) ? 22 : 23))) :
        ((value <= 1U<<27) ?
        ((value <= 1U<<25) ? ((value <= 1U<<24) ? 24 : 25) : ((value <= 1U<<26) ? 26 : 27)) :
        ((value <= 1U<<29) ? ((value <= 1U<<28) ? 28 : 29) : ((value <= 1U<<30) ? 30 : 
        ((value <= 1U<<31) ? 31 : 32))))));
}

static UInt32 GetNextPow2(UInt32 value)
{ 
    return (1U<<GetIntLog2(value));
}

int GFxDataExporter::ParseCommandLine(int argc, const char** const argv)
{
    // get current directory
    char cwd[1024];
    if (gfc_getcwd(cwd, sizeof(cwd)) == 0)
        cwd[0] = '0';

    bool fontImagesFmtSpecified = false;
    bool gradImagesFmtSpecified = false;
    bool imagesFmtSpecified = false;

    for (int i = 1; i < argc; ++i)
    {
        // Handle options - they always begin with '-'
        if (argv[i][0] == '-')
        {
            const char* curArg = argv[i] + 1;

            // destination directory
            if (gfc_stricmp(curArg, "d") == 0)
            {
                // directory name is following
                if (i + 1 < argc)
                {
                    OutputRootDir = argv[++i];
                }
                else
                {
                    fprintf(stderr, "Directory name is missing for option '-d'\n");
                }
            }
            else if (gfc_stricmp(curArg, "sd") == 0)
            {
                DoCreateSubDirs = true;
            }
            else if (gfc_stricmp(curArg, "p") == 0) // prefix
            {
                // prefix name is following
                if (i + 1 < argc)
                {
                    Prefix = argv[++i];
                }
                else
                {
                    fprintf(stderr, "Prefix is missing for option '-p'\n");
                }
            }
            else if (gfc_stricmp(curArg, "i") == 0) // image format
            {
                // prefix name is following
                if (i + 1 < argc)
                {
                    ImagesFormat = GFxString(argv[++i]).ToLower();
                    if (!GetImageFormatId())
                        fprintf(stderr, "Unknown image format: %s. Using default TGA format instead...\n",
                                        ImagesFormat.ToCStr());
                    else
                        imagesFmtSpecified = true;
                }
                else
                {
                    fprintf(stderr, "Format is missing for option '-%s'\n", curArg);
                }
            }
            else if (gfc_stricmp(curArg, "c") == 0)
            {
                DoCompress = true;
                DoStrip = true;
            }
            else if (gfc_stricmp(curArg, "u") == 0)
            {
                DoCompress = false;
                DoStrip = true;
            }
            else if (gfc_strcmp(curArg, "lwr") == 0)
            {
                ToLowercase = true;
            }
            else if (gfc_strcmp(curArg, "upr") == 0)
            {
                ToUppercase = true;
            }
            else if (gfc_stricmp(curArg, "q") == 0)
            {
                Quiet = true;
            }

            else if (gfc_stricmp(curArg, "d0") == 0)
            {
                UncompressedDDS = true;
            }
            else if (gfc_stricmp(curArg, "d1") == 0)
            {
                DTX1Allowed = true;
            }
            else if (gfc_stricmp(curArg, "d3") == 0)
            {
                DTXn = 3;
            }
            else if (gfc_stricmp(curArg, "d5") == 0)
            {
                DTXn = 5;
            }
            else if (gfc_stricmp(curArg, "dm") == 0)
            {
                GenMipMapLevels = true;
            }
            else if (gfc_stricmp(curArg, "nomipmap") == 0)
            {
                GenMipMapLevels = false;
            }
            else if (gfc_stricmp(curArg, "qf") == 0 || gfc_stricmp(curArg, "quick") == 0)
            {
                Quality = GDXTHelper::QualityFastest;
            }
            else if (gfc_stricmp(curArg, "qn") == 0 || gfc_stricmp(curArg, "quality_normal") == 0)
            {
                Quality = GDXTHelper::QualityNormal;
            }
            else if (gfc_stricmp(curArg, "qp") == 0 || gfc_stricmp(curArg, "quality_production") == 0)
            {
                Quality = GDXTHelper::QualityProduction;
            }
            else if (gfc_stricmp(curArg, "qh") == 0 || gfc_stricmp(curArg, "quality_normal") == 0)
            {
                Quality = GDXTHelper::QualityHighest;
            }
            else if (gfc_stricmp(curArg, "rescale") == 0)
            {
                const char* curArg = argv[++i];
                if (gfc_stricmp(curArg, "nearest") == 0)
                    Rescale = GDXTHelper::RescaleNearestPower2;
                else if (gfc_stricmp(curArg, "hi") == 0)
                    Rescale = GDXTHelper::RescaleBiggestPower2;
                else if (gfc_stricmp(curArg, "low") == 0)
                    Rescale = GDXTHelper::RescaleSmallestPower2;
                else if (gfc_stricmp(curArg, "nextlow") == 0)
                    Rescale = GDXTHelper::RescaleNextSmallestPower2;
                else if (gfc_stricmp(curArg, "mult4") == 0)
                    Rescale = GDXTHelper::RescalePreScale;
                else
                {
                    fprintf(stderr, "Error: Invalid rescale mode is specified: '%s'\n", curArg);
                }
            }
            else if (gfc_strnicmp(curArg, "rescale", 7) == 0)
            {
                curArg += 7;
                int f = ParseMipFilterType(curArg);
                if (f > 0)
                {
                    RescaleFilter = (GDXTHelper::FilterTypes)f;
                }
                else
                {
                    fprintf(stderr, "Error: Invalid rescale filtering is specified: '%s'\n", curArg);
                }
            }
            else if (gfc_strnicmp(curArg, "mipmap", 6) == 0)
            {
                curArg += 6;
                int f = ParseMipFilterType(curArg);
                if (f > 0)
                {
                    // mipmap filtering
                    MipFilter = (GDXTHelper::FilterTypes)f;
                }
                else
                {
                    fprintf(stderr, "Error: Invalid mipmap filtering is specified: '%s'\n", curArg);
                }
            }
            else if (gfc_stricmp(curArg, "fonts") == 0)
            {
                ExportFonts = true;
            }
            else if (gfc_stricmp(curArg, "fns") == 0)
            {
                if (i + 1 < argc)
                {
                    TextureGlyphNominalSize = atoi(argv[++i]);
                }
                else
                {
                    fprintf(stderr, "Size is missing for option '-%s'\n", curArg);
                }
            }
            else if (gfc_stricmp(curArg, "fpp") == 0)
            {
                if (i + 1 < argc)
                {
                    TextureGlyphPadPixels = atoi(argv[++i]);
                }
                else
                {
                    fprintf(stderr, "Number of pixels is missing for option '-%s'\n", curArg);
                }
            }
            else if (gfc_stricmp(curArg, "fts") == 0)
            {
                if (i + 1 < argc)
                {
                    char* endp = 0;
                    unsigned long n = strtoul(argv[++i], &endp, 10);
                    if (n < GFX_FONT_CACHE_TEXTURE_SIZE_MINIMUM) n = GFX_FONT_CACHE_TEXTURE_SIZE_MINIMUM;
                    FontTextureWidth = int(GetNextPow2(UInt32(n)));
                    if (FontTextureWidth != int(n))
                    {
                        fprintf(stderr, "Warning: Texture width should be a power of 2. Setting to %d\n", FontTextureWidth);
                    }
                    if (*endp == 0)
                        FontTextureHeight = FontTextureWidth;
                    else if (*endp == 'x' || *endp == 'X' || *endp == '*' || *endp == '-' || *endp == ',')
                    {
                        n = strtoul(++endp, &endp, 10);
                        if (n < GFX_FONT_CACHE_TEXTURE_SIZE_MINIMUM) n = GFX_FONT_CACHE_TEXTURE_SIZE_MINIMUM;
                        if (*endp == 0)
                        {
                            FontTextureHeight = int(GetNextPow2(UInt32(n)));
                            if (FontTextureHeight != int(n))
                            {
                                fprintf(stderr, "Warning: Texture height should be a power of 2. Setting to %d\n", FontTextureHeight);
                            }
                        }
                        else
                            FontTextureHeight = FontTextureWidth;
                    }
                    if (FontTextureWidth > GFX_MAX_TEXTURE_DIMENSION)
                    {
                        FontTextureWidth = GFX_MAX_TEXTURE_DIMENSION;
                        fprintf(stderr, "Warning: Font texture width is too big. Resetting to %d\n", FontTextureWidth);
                    }
                    if (FontTextureHeight > GFX_MAX_TEXTURE_DIMENSION)
                    {
                        FontTextureHeight = GFX_MAX_TEXTURE_DIMENSION;
                        fprintf(stderr, "Warning: Font texture height is too big. Resetting to %d\n", FontTextureHeight);
                    }
                }
                else
                {
                    fprintf(stderr, "Dimensions of texture are missing for option '-%s'\n", curArg);
                }
            }
            else if (gfc_stricmp(curArg, "fs") == 0)
            {
                UseSeparateFontTextures = true;
            }
            else if (gfc_stricmp(curArg, "fi") == 0) // font image format
            {
                // prefix name is following
                if (i + 1 < argc)
                {
                    GFxString fontImagesFormatStr = GFxString(argv[++i]).ToLower();
                    if (fontImagesFormatStr == "tga" || fontImagesFormatStr == "tga8")
                    {
                        FontImagesFormat = GFxFileConstants::File_TGA;
                        FontImagesBits = 8;
                    }
                    else if (fontImagesFormatStr == "tga24")
                    {
                        FontImagesFormat = GFxFileConstants::File_TGA;
                        FontImagesBits = 24;
                    }
                    else if (fontImagesFormatStr == "tga32")
                    {
                        FontImagesFormat = GFxFileConstants::File_TGA;
                        FontImagesBits = 32;
                    }
                    else if (fontImagesFormatStr == "dds" || fontImagesFormatStr == "dds8")
                    {
                        FontImagesFormat = GFxFileConstants::File_DDS;
                        FontImagesBits = 8;
                    }
                    else
                    {
                        fprintf(stderr, "Unknown font image format: %s. Using default TGA8 format instead...\n",
                            fontImagesFormatStr.ToCStr());
                    }
                    fontImagesFmtSpecified = true;
                }
                else
                {
                    fprintf(stderr, "Format is missing for option '-%s'\n", curArg);
                }
            }
            else if (gfc_stricmp(curArg, "strip_font_shapes") == 0)
            {
                GlyphsStripped = true;
            }
            else if (gfc_stricmp(curArg, "gradients") == 0)
            {
                ExportGradients = true;
            }
            else if (gfc_stricmp(curArg, "grs") == 0)
            {
                if (i + 1 < argc)
                {
                    UInt sz = UInt(atoi(argv[++i]));
                    GradientSize = UInt(GetNextPow2(UInt32(sz)));
                    if (GradientSize != sz)
                    {
                        fprintf(stderr, "Warning: Gradients' size should be a power of 2. Setting to %d\n", GradientSize);
                    }
                    if (GradientSize > GFX_MAX_TEXTURE_DIMENSION)
                    {
                        GradientSize = GFX_MAX_TEXTURE_DIMENSION;
                        fprintf(stderr, "Warning: Gradient texture dimensions are too big. Resetting to %d\n", GradientSize);
                    }
                }
                else
                {
                    fprintf(stderr, "Size is missing for option '-%s'\n", curArg);
                }
            }
            else if (gfc_stricmp(curArg, "gi") == 0) // gradient image format
            {
                // prefix name is following
                if (i + 1 < argc)
                {
                    GFxString gradImagesFormatStr = GFxString(argv[++i]).ToLower();
                    if (gradImagesFormatStr == "tga" || gradImagesFormatStr == "tga32")
                    {
                        GradientImagesFormat = GFxFileConstants::File_TGA;
                        GradientImagesBits = 32;
                    }
                    else if (gradImagesFormatStr == "dds32")
                    {
                        GradientImagesFormat = GFxFileConstants::File_DDS;
                        GradientImagesBits = 32;
                    }
                    else if (gradImagesFormatStr == "dds")
                    {
                        GradientImagesFormat = GFxFileConstants::File_DDS;
                        GradientImagesBits = 0;
                    }
                    else
                    {
                        fprintf(stderr, "Unknown gradient image format: %s. Using default TGA format instead...\n",
                            gradImagesFormatStr.ToCStr());
                    }
                    gradImagesFmtSpecified = true;
                }
                else
                {
                    fprintf(stderr, "Format is missing for option '-%s'\n", curArg);
                }
            }
            else if (gfc_stricmp(curArg, "gsid") == 0) // Directories (comma-separated list) with gradient images
            {
                if (i + 1 < argc)
                {
                    const char* dirs = argv[++i];
                    while(dirs)
                    {
                        char buf[1024];
                        const char* p = strchr(dirs, ',');
                        if (p)
                        {
                            gfc_strncpy(buf, 1024, dirs, p-dirs);
                            buf[p-dirs] = '\0';
                        }
                        else
                            gfc_strcpy(buf, 1024, dirs);
                        SharedGradDirs.push_back(GFxString(buf));
                        dirs = p;
                    }
                }
            }
            else if (gfc_stricmp(curArg, "gd") == 0) // Output directory for gradient images
            {
                if (i + 1 < argc)
                {
                    GradientsOutputDir = argv[++i];
                }
            }
            else if (gfc_stricmp(curArg, "strip_images") == 0)
            {
                JustStripImages = true;
            }
            else if (gfc_stricmp(curArg, "det") == 0)
            {
                ExportDefaultEditText = true;
                DefaultDynamicTextOnly = false;
            }
            else if (gfc_stricmp(curArg, "ddt") == 0)
            {
                ExportDefaultEditText = true;
                DefaultDynamicTextOnly = true;
            }
            else if (gfc_stricmp(curArg, "fstree") == 0)
            {
                ExportFsCommands = true;
                FsCommandsAsTree = true;
            }
            else if (gfc_stricmp(curArg, "fslist") == 0)
            {
                ExportFsCommands = true;
                FsCommandsAsList = true;
            }
            else if (gfc_stricmp(curArg, "fsparams") == 0)
            {
                FsCommandsParams = true;
            }
            else if (gfc_stricmp(curArg, "share_images") == 0)
            {
                ShareImages = true;
            }
            else if (gfc_stricmp(curArg, "fntlst") == 0)
            {
                ExportFontList = true;
            }
            else
            {
                fprintf(stderr, "Error: Incorrect option: %s\n", argv[i]);
            }
        }
        // Otherwise it's a filename
        else
        {
            // check for wildcards
            const char* pch = argv[i];
            bool    hasWildcards = 0;
            int     lastDirPos = -1;
            while (*pch && !hasWildcards)
            {
                switch (*pch)
                {
                case '{':
                case '[':
                case '?':
                case '*':
                    hasWildcards=1;
                    break;
                case '/':
                case '\\':
                    lastDirPos=pch-argv[i];
                    break;
                }
                pch++;
            }

            if (!hasWildcards)
            {
                // add the single filename
                SrcFileNames.push_back(argv[i]);
            }
            else
            {
                // search the path
                WIN32_FIND_DATAA ffd;
                HANDLE fh = ::FindFirstFile(argv[i], &ffd);
                if (fh!=INVALID_HANDLE_VALUE)
                {
                    do
                    {
                        // skip subdirectories
                        if (!(ffd.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY))
                        {
                            // add the file to the list
                            GFxString fname(argv[i],lastDirPos+1);
                            fname += ffd.cFileName;
                            SrcFileNames.push_back(fname);
                        }
                    } while (::FindNextFile(fh,&ffd));
                    ::FindClose(fh);
                }
            }
        }
    }
    if (imagesFmtSpecified && !fontImagesFmtSpecified)
    {
        if (GetImageFormatId() == GFxFileConstants::File_TGA)
        {
            FontImagesFormat = GFxFileConstants::File_TGA;
            FontImagesBits = 8;
        }
        else if (GetImageFormatId() == GFxFileConstants::File_DDS)
        {
            FontImagesFormat = GFxFileConstants::File_DDS;
            FontImagesBits = 8;
        }
    }
    if (imagesFmtSpecified && !gradImagesFmtSpecified)
    {
        if (GetImageFormatId() == GFxFileConstants::File_TGA)
        {
            GradientImagesFormat = GFxFileConstants::File_TGA;
            GradientImagesBits = 32;
        }
        else if (GetImageFormatId() == GFxFileConstants::File_DDS)
        {
            GradientImagesFormat = GFxFileConstants::File_DDS;
            GradientImagesBits = 32;
        }
    }
    // if file format is DDS, rescale mode - mult4 then mipmap levels cannot be generated by DXTLib. 
    // The DXTLib crashes in this case.
    if (GetImageFormatId() == GFxFileConstants::File_DDS && Rescale == GDXTHelper::RescalePreScale && GenMipMapLevels)
    {
        fprintf(stderr, "Warning: Deminesions should be power of 2 for generation of mipmaps.\n");
        GenMipMapLevels = false;
    }

    return 1;
}

void              GFxDataExporter::AddImageExporter(int imgFormatId, GFxImageExporter* pimgExp)
{
    ImageExporters.set(imgFormatId, pimgExp);
}

GFxImageExporter* GFxDataExporter::GetImageExporter(int imgFormatId)
{
    GFxImageExporter* ptr = 0;
    ImageExporters.get(imgFormatId, &ptr);
    return ptr;
}

void GFxDataExporter::ShowVersion()
{
    printf("GFxExport v%d.%2.2d for SDK v%d.%d.%d, (c) 2006-2007 Scaleform Corporation\n",
            (GFX_EXPORT_VERSION>>8), (GFX_EXPORT_VERSION&0xFF),
            GFC_FX_MAJOR_VERSION, GFC_FX_MINOR_VERSION, GFC_FX_BUILD_VERSION);
    printf("This tool uses nvDXTlib, (c) 2006 NVIDIA Corporation\n");
}

void GFxDataExporter::ShowHelpScreen()
{
    ShowVersion();
    printf("\nUsage: gfxexport [options] [file.swf] [file(s)...] [-d dirname]\n");
    printf(" Options:\n");
    printf("  -i <format>   Specifies output format for exporting image data\n"
           "                where <format> is one of the following:\n");
    GTL::ghash<int, GFxImageExporter*>::const_iterator iter = ImageExporters.begin();
    for(; iter != ImageExporters.end(); ++iter)
    {
        printf("                  %s - %s\n", iter->second->GetName(), iter->second->GetDescription());
    }
           
    printf("  -strip_images Just strip images, do not write them in files\n"
           "  -d <dirname>  Destination directory for exported data. If not specified,\n"
           "                files are stored in the directory containing the SWF.\n"
           "  -sd           Create subdirectories for each SWF file using the SWF filename.\n"
           "                Extracted files are placed in the corresponding subdirectories.\n"
           "  -c            Write compressed stripped .gfx file(s).\n"
           "  -u            Write uncompressed stripped .gfx file(s) (default).\n"
           "  -p <prefix>   Specifies prefix to add to the name of each exported resource.\n"
           "                By default, the original SWF filename is used as prefix.\n"
           "  -lwr          Force all exported files to have lowercase names.\n"
           "  -upr          Force all exported files to have uppercase names.\n"
           "  -q            Quiet mode (suppress output).\n"
           "  -share_images Try to reuse images in the destination directory rather than\n"
           "                write new ones every time.\n"
           "  -rescale <nearest | hi | low | nextlow | mult4> \n"
           "                Rescale image to nearest, next highest or next lowest power\n"
           "                of two or multiple of 4. 'hi' is default for compressed DDS.\n"
           "  Optional filtering for rescaling: (\n"
           "  -rescalePoint\n"
           "  -rescaleBox\n"
           "  -rescaleTriangle\n"
           "  -rescaleQuadratic\n"
           "  -rescaleCubic   (default)\n"
           "  -rescaleCatrom\n"
           "  -rescaleMitchell\n"
           "  -rescaleGaussian\n"
           "  -rescaleSinc\n"
           "  -rescaleBessel\n"
           "  -rescaleHanning\n"
           "  -rescaleHamming\n"
           "  -rescaleBlackman\n"
           "  -rescaleKaiser\n"
           "\n"
           "DDS options ('-i DDS' is specified):\n"
           "  -d0      Write uncompressed DDS\n"
           "  -d1      Use DXT1 for RGB data without alpha channel (default: off)\n"
           "  -d3/-d5  Use DXT3 (default) or DXT5 for RGB data with alpha channel\n"
           "  -nomipmap                Do not generate mipmaps in DDS file\n"
           "  -qf, -quick              Fast compression method\n"
           "  -qn, -quality_normal     Normal quality compression (default)\n"
           "  -qp, -quality_production Production quality compression\n"
           "  -qh, -quality_highest    Highest quality compression (this can be very slow)\n"
           "\n"
           "Mipmap filtering options:\n"
           "  -mipmapPoint\n"
           "  -mipmapBox\n"
           "  -mipmapTriangle (default)\n"
           "  -mipmapQuadratic\n"
           "  -mipmapCubic\n"
           "  -mipmapCatrom\n"
           "  -mipmapMitchell\n"
           "  -mipmapGaussian\n"
           "  -mipmapSinc\n"
           "  -mipmapBessel\n"
           "  -mipmapHanning\n"
           "  -mipmapHamming\n"
           "  -mipmapBlackman\n"
           "  -mipmapKaiser\n"
           "\n"
           "Font options:\n");
    printf("  -fonts        Export font textures.\n"
        "  -fntlst       Export font list and textfield/font map (.fnt-file)\n"
           "  -fns <size>   Nominal size of texture glyph (in pixels). Default - %d\n", GFX_FONT_NOMINAL_GLYPH_SIZE_DEFAULT);
    printf("  -fpp <n>      Space, in pixels, to leave around the individual glyph image\n"
           "                Default - %d\n", GFX_FONT_PAD_PIXELS_DEFAULT); 
    printf("  -fts <WxH>    The dimensions of the textures that the glyphs get packed\n"
           "                into. Default size is %dx%d. To specify square texture only\n"
           "                one dimension can be specified, e.g.: '-fts 128' is 128x128.\n"
           "                '-fts 512x128' specifies rectangle texture.\n", GFX_FONT_CACHE_TEXTURE_SIZE_DEFAULT, GFX_FONT_CACHE_TEXTURE_SIZE_DEFAULT);
    printf("  -fs           Force separate textures for each font. By default, fonts\n"
           "                share textures.\n"
           "  -strip_font_shapes\n"
           "                Do not write font shapes in resulting GFX file.\n"
           "  -fi <format>  Specifies output format for font textures\n"
           "                where <format> is one of the following:\n"
           "                  TGA8  - 8-bit Targa TrueVision (grayscaled)\n"
           "                  TGA24 - 24-bit Targa TrueVision (grayscaled)\n"
           "                  TGA32 - 32-bit Targa TrueVision\n"
           "                  DDS8  - 8-bit DDS A8\n"
           "                By default, if image format (-i option) is TGA then TGA8\n"
           "                is used for font textures; otherwise DDS A8.\n"
           "\n"
           "Gradient options:\n"
           "  -gradients    Export gradient images.\n"
           "  -grs <size>   Sets size of radial gradient image as <size> by <size> pixels.\n"
           "                Default size is %dx%d\n", GFX_GRADIENT_SIZE_DEFAULT, GFX_GRADIENT_SIZE_DEFAULT);
    printf(
           "  -gi <format>  Specifies output format for gradient textures\n"
           "                where <format> is one of the following:\n"
           "                  TGA   - 32-bit Targa TrueVision\n"
           "                  DDS32 - 32-bit uncompressed DDS\n"
           "                  DDS   - Use same DDS settings as for images (see \n"
           "                          \"DDS Options\")\n"
           "                By default, if image format (-i option) is TGA then TGA\n"
           "                is used for gradient textures; if image format is \n"
           "                DDS then UNCOMPRESSED DDS is used (use -gi DDS option, if \n"
           "                compressed DDS is necessary).\n"
           "\n"
           "Default edit-textfield (dynamic/input) text report options:\n"
           "  -det          Export list of unique default dynamic/input textfield\n"
           "                text as a list (.det-file; UTF-8)\n"
           "  -ddt          Export list of unique default dynamic textfield\n"
           "                text as a list (.ddt-file; UTF-8)\n"
           "\n"
           "FSCommand use report options:\n"
           "  -fstree       Export list of FSCommands as a tree (.fst-file)\n"
           "  -fslist       Export list of FSCommands as a sorted list (.fsl-file)\n"
           "  -fsparams     Try to find parameters for FSCommands\n"
           );
}


