/**********************************************************************

Filename    :   GFxImageExporters.cpp
Content     :   SWF to GFX resource extraction and conversion tool
Created     :   October, 2006
Authors     :   Artyom Bolgar
Copyright   :   (c) 2006 Scaleform Corp. All Rights Reserved.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#define GFX_EXPORT_MAJOR_VERSION    1
#define GFX_EXPORT_MINOR_VERSION    11
#define GFX_EXPORT_VERSION (((GFX_EXPORT_MAJOR_VERSION)<<8)|(GFX_EXPORT_MINOR_VERSION))

#include "GFxExport.h"

class GFxTGAExporter : public GFxImageExporter
{
    GFxImageExporterParams Params;
public:
    const char* GetName() const         { return "TGA"; }
    const char* GetDescription() const  { return "Truevision (Targa or TGA)"; }
    int         GetFormatId() const     { return GFxFileConstants::File_TGA; }
    const char* GetDefaultExtension() const { return "tga"; }

    GFxImageExporterParams& InitParams(UInt32 charId, const GFxDataExporter* pexp, GFxExportImageDescr* pimgDescr, GImage* pimage, GFile* pfile = 0)
    {
        Params.Set(charId, pexp, pimgDescr, pimage, pfile);
        return Params;
    }

    bool        MightBeRescaled() const { return true; }
    bool        NeedToBeConverted(GImage* pimage, int destFormat) const
    {
        return (destFormat != pimage->Format);
    }

    bool        Write(GDXTHelper * pdxtHelper )
    {
        GUNUSED(pdxtHelper);
        if (!Params.pImage->WriteTga(Params.pFile))
        {
            fprintf(stderr, "Error: Can't write to '%s' file\n", Params.pFile->GetFilePath());
            return false;
        }
        return true;
    }

    // Read file and create GImage. This method used for sharing images. If this method
    // is not implemented (returns NULL) then images of this format cannot be shared
    GImage*     Read(const char* filePath)
    {
        GPtr<GFile> pin = *new GSysFile(filePath, GFile::Open_Read);
        if (!pin || !pin->IsValid())
        {
            fprintf(stderr, "Error: Can't open source file '%s' to read from\n", filePath);
            return NULL;
        }
        return GImage::ReadTga(pin);
    }
};
GFX_REGISTER_IMAGE_EXPORTER(GFxTGAExporter);

class GFxDDSExporter : public GFxImageExporter
{
    struct GFxDDSImageExporterParams : public GFxImageExporterParams
    {
        bool                        UncompressedDDS;
    } Params;
public:
    const char* GetName() const         { return "DDS"; }
    const char* GetDescription() const  { return "DirectDraw Surface (DDS)"; }
    int         GetFormatId() const     { return GFxFileConstants::File_DDS; }
    const char* GetDefaultExtension() const { return "dds"; }

    GFxImageExporterParams& InitParams(UInt32 charId, const GFxDataExporter* pexp, GFxExportImageDescr* pimgDescr, GImage* pimage, GFile* pfile = 0)
    {
        Params.Set(charId, pexp, pimgDescr, pimage, pfile);
        Params.UncompressedDDS = pexp->UncompressedDDS;

        if (GFxResourceId(Params.CharacterId).GetIdType() == GFxResourceId::IdType_GradientImage)
        {
            if (Params.pExporter->GradientImagesBits > 0)
                Params.UncompressedDDS = true;
            else
            {
                // check, is it for linear gradient (size 256x1) or not.
                // if linear - do not use compressed DDS, since this will 
                // resize gradient up to 256x4.
                if (Params.pImageDescr->TargetHeight == 1)
                    Params.UncompressedDDS = true;
                else
                    Params.UncompressedDDS = false;
            }
        }

        return Params;
    }

    bool        MightBeRescaled() const { return false; }
    bool        NeedToBeConverted(GImage*, int) const { return false; }

    bool        Write(GDXTHelper*  pdxtHelper)
    {
        if (!pdxtHelper->CompressToFile(Params.pImage, Params.pFile, Params.pExporter->Quality,
            Params.Rescale, Params.pExporter->RescaleFilter, Params.pExporter->MipFilter,Params.pExporter->DTXn,
            Params.UncompressedDDS, Params.pExporter->GenMipMapLevels, Params.pExporter->DTX1Allowed))
        {
            fprintf(stderr, "Error: Can't compress image '%s' to DDS, error: '%s'\n", Params.pFile->GetFilePath(), pdxtHelper->GetLastErrorString());
            return false;
        }
        return true;
    }

    // Read file and create GImage. This method used for sharing images. If this method
    // is not implemented (returns NULL) then images of this format cannot be shared
    GImage*     Read(const char* filePath)
    {
        GPtr<GFile> pin = *new GSysFile(filePath, GFile::Open_Read);
        if (!pin || !pin->IsValid())
        {
            fprintf(stderr, "Error: Can't open source file '%s' to read from\n", filePath);
            return NULL;
        }
        return GImage::ReadDDS(pin);
    }
};
GFX_REGISTER_IMAGE_EXPORTER(GFxDDSExporter);

class GFxORIGExporter : public GFxImageExporter
{
    struct GFxOrigImageExporterParams : public GFxImageExporterParams
    {
        GFxString                           PathNoExt;
        const GFxDataExporter::JpegDesc*    pJpegDesc;
    } Params;
public:
    const char* GetName() const         { return "ORIG"; }
    const char* GetDescription() const  { return "Original format: JPEG data will be saved as JPG files\n"
        "                         and lossless images will be saved as TGAs.\n"; }
    int         GetFormatId() const     { return GFxFileConstants::File_Original; }
    const char* GetDefaultExtension() const 
    { 
        if (Params.pImageDescr == NULL || Params.pImageDescr->Format == GFxFileConstants::File_JPEG)
        {
            return "jpg";
        }
        else
        {
            return "tga";
        }
        //return "jpg"; 
    }

    GFxImageExporterParams& InitParams(UInt32 charId, const GFxDataExporter* pexp, GFxExportImageDescr* pimgDescr, GImage* pimage, GFile* pfile = 0)
    {
        Params.Set(charId, pexp, pimgDescr, pimage, pfile);
        Params.pJpegDesc = Params.pExporter->JpegDescriptors.get(Params.CharacterId);
        if (Params.pJpegDesc)
        {
            if (Params.pJpegDesc->TagType != 35)
                Params.pImageDescr->Format = GFxFileConstants::File_JPEG;
            else
                Params.pImageDescr->Format = GFxFileConstants::File_TGA;
        }
        else
        {
            Params.pImageDescr->Format = GFxFileConstants::File_TGA;
            Params.pJpegDesc = 0;
        }
        return Params;
    }
    bool        MightBeRescaled() const { return false; }
    bool        NeedToBeConverted(GImage*, int) const { return false; }

    GFxString   MakeFileName(const GFxString& nameNoExt)
    {
        return nameNoExt + ((Params.pImageDescr->Format == GFxFileConstants::File_JPEG) ? ".jpg" : ".tga");
    }
    virtual GFxString   MakePath(const GFxString& path, const GFxString& nameWithExt)
    {
        GFxString p = path + nameWithExt;
        Params.PathNoExt = GFxDataExporter::CutExtension(p);
        return p;
    }

    bool        Write(GDXTHelper * pdxtHelper)
    {
        GUNUSED(pdxtHelper);
        if (Params.pJpegDesc)
        {
            if (Params.pJpegDesc->TagType == 35)
            {
                // if JPEG was used with alpha, we need to save the 32-bit TGA as main
                // image and only then JPEG.
                if (!Params.pImage->WriteTga(Params.pFile))
                {
                    fprintf(stderr, "Error: Can't write to '%s' file\n", Params.pFile->GetFilePath());
                    return false;
                }
                Params.pFile->Close();

                // now need to reopen file as jpg and write data to it
                GFxString destPath;
                destPath = Params.PathNoExt;
                destPath += ".jpg";
                Params.pFile = *new GSysFile(destPath.ToCStr(),
                    GFile::Open_Write | GFile::Open_Truncate | GFile::Open_Create);
                if (!Params.pFile->IsValid() || !Params.pFile->IsWritable())
                {
                    fprintf(stderr, "Error: Can't open file '%s' for writing\n", destPath.ToCStr());
                    return false;
                }
            }

            // check if tagType == 6 and we have parsed tagType == 8 (JPEGTables) then
            // write these tables first.
            if (Params.pJpegDesc->TagType == 6)
            {
                const GFxDataExporter::JpegDesc* pjpeg8Desc = Params.pExporter->JpegDescriptors.get(-1);
                if (pjpeg8Desc)
                {
                    if (Params.pFile->Write(pjpeg8Desc->pData, SInt(pjpeg8Desc->DataSize)) != SInt(pjpeg8Desc->DataSize))
                    {
                        fprintf(stderr, "Error: Can't write to '%s' file\n", Params.pFile->GetFilePath());
                        return false;
                    }
                }
            }
            if (Params.pFile->Write(Params.pJpegDesc->pData, SInt(Params.pJpegDesc->DataSize)) != SInt(Params.pJpegDesc->DataSize))
            {
                fprintf(stderr, "Error: Can't write to '%s' file\n", Params.pFile->GetFilePath());
                return false;
            }
            GFxString destPath = Params.pFile->GetFilePath();
            Params.pFile->Close(); // close file and re-open for reading
            Params.pFile = *new GSysFile(destPath.ToCStr(), GFile::Open_Read);
            if (!Params.pFile->IsValid())
            {
                fprintf(stderr, "Error: Can't open file '%s' for reading\n", destPath.ToCStr());
                return false;
            }

            // original format
            // Write original JPEG data without re-compression
            // First of all, re-read JPEG by jpeglib without decompression....
            GJPEGInput* pjin = GJPEGInput::CreateSwfJpeg2HeaderOnly(Params.pFile);
            if (!pjin) return false;

            pjin->StartRawImage();
            void* prawData;
            pjin->ReadRawData(&prawData);
            Params.pFile->Close(); // close file and re-open for writing

            Params.pFile = *new GSysFile(destPath.ToCStr(),
                GFile::Open_Write | GFile::Open_Truncate | GFile::Open_Create);
            if (!Params.pFile->IsValid() || !Params.pFile->IsWritable())
            {
                fprintf(stderr, "Error: Can't open file '%s' for writing\n", destPath.ToCStr());
                return false;
            }

            // write JPEG data back w/o re-compression
            GJPEGOutput*    pjout = GJPEGOutput::Create(Params.pFile);
            pjout->CopyCriticalParams(pjin->GetCInfo());
            pjout->WriteRawData(prawData);
            delete pjout;

            pjin->FinishImage();
            delete pjin;

            Params.pFile->Close();
        }
        else
        {
            if (!Params.pImage->WriteTga(Params.pFile))
            {
                fprintf(stderr, "Error: Can't write to '%s' file\n", Params.pFile->GetFilePath());
                return false;
            }
        }
        return true;
    }

    // Read file and create GImage. This method used for sharing images. If this method
    // is not implemented (returns NULL) then images of this format cannot be shared
    GImage*     Read(const char* filePath)
    {
        // check for extension first
        const char* pext = strrchr(filePath, '.');
        if (pext != NULL)
        {
            ++pext;
        }
        GPtr<GFile> pin = *new GSysFile(filePath, GFile::Open_Read);
        if (!pin || !pin->IsValid())
        {
            fprintf(stderr, "Error: Can't open source file '%s' to read from\n", filePath);
            return NULL;
        }
        if (gfc_stricmp(pext, "tga") == 0)
            return GImage::ReadTga(pin);
        else if (gfc_stricmp(pext, "jpg") == 0)
            return GImage::ReadJpeg(pin);
        return NULL;
    }
};
GFX_REGISTER_IMAGE_EXPORTER(GFxORIGExporter);


