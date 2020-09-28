/**********************************************************************

Filename    :   GFxExport.h
Content     :   SWF to GFX resource extraction and conversion tool
Created     :   May, 2007
Authors     :   Artyom Bolgar
Copyright   :   (c) 2006-2007 Scaleform Corp. All Rights Reserved.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef INC_GFXEXPORT_H
#define INC_GFXEXPORT_H

// GFx Includes
#include "GStd.h"
#include "GTypes.h"
#include "GRefCount.h"

#include "GFile.h"
#include "GZlibFile.h"
#include "GImage.h"
#include "GJPEGUtil.h"
#include "GFxPlayer.h"
#include "GFxLoader.h"
#include "GFxLog.h"
#include "GFxString.h"
#include "GFxStream.h"
#include "GFxFontResource.h"
#include "GFxStyles.h"
#include "GFxText.h"

#include "GDXTHelper.h"

class GFxDataExporter;
struct GFxFsCommandOrigin;
struct GFxFsCommand;

struct GFxExportImageDescr
{
    GPtr<GImageInfo> pImageInfo;
    GFxString        ImageName;
    GFxString        ExportName;
    GFxString        SwfName;
    UInt32           CharacterId;
    UInt             TargetWidth;
    UInt             TargetHeight;
    UInt16           Format;
};

struct GFxImageExporterParams
{
    UInt32                      CharacterId;
    const GFxDataExporter*      pExporter;
    GPtr<GImage>                pImage;
    GFxExportImageDescr*        pImageDescr;
    GPtr<GFile>                 pFile;
    GDXTHelper::RescaleTypes    Rescale;

    inline GFxImageExporterParams() 
    { Set(0, 0, 0, 0, 0); }
    inline GFxImageExporterParams(UInt32 charId, const GFxDataExporter* pexp, GFxExportImageDescr* pimgDescr, GImage* pimage, GFile* pfile = 0) 
    { Set(charId, pexp, pimgDescr, pimage, pfile); }

    void Set(UInt32 charId, const GFxDataExporter* pexp, GFxExportImageDescr* pimgDescr, GImage* pimage, GFile* pfile = 0);
};

// Interface for plug-in image exporters.
// Each implementation of this interface should also create and store the instance
// of GFxImageExporterParams. This instance should be accessible at least by InitParams() 
// and Write() methods.
class GFxImageExporter
{
public:
    virtual ~GFxImageExporter() {}

    // Returns short name of format, like "TGA", "DDS", etc
    virtual const char* GetName() const         = 0;
    // Returns description that will be displayed on help screen
    virtual const char* GetDescription() const  = 0;
    // Returns format id. Should be one of the GFxFileConstants::Image_<> constant,
    // or beyond GFxFileConstants::Image_NextAvail
    virtual int         GetFormatId() const     = 0;

    // Initializes and returns GFxImageExporterParams structure
    virtual GFxImageExporterParams& InitParams(UInt32 charId, const GFxDataExporter* pexp, GFxExportImageDescr* pimgDescr, GImage* pimage, GFile* pfile = 0) = 0;

    // Makes and returns full name of the file with extension (w/o path)
    virtual GFxString   MakeFileName(const GFxString& nameNoExt)
    {
        return nameNoExt + "." + GetDefaultExtension();
    }
    // Makes and returns full path to the file
    virtual GFxString   MakePathAndAddExt(const GFxString& path, const GFxString& nameNoExt)
    {
        return path + nameNoExt + "." + GetDefaultExtension();
    }
    // Makes and returns full path to the file
    virtual GFxString   MakePath(const GFxString& path, const GFxString& nameWithExt)
    {
        return path + nameWithExt;
    }
    // Returns default extension of the file
    virtual const char* GetDefaultExtension() const = 0;

    // Returns true, if image could be rescaled, if -rescale option is used
    virtual bool        MightBeRescaled() const = 0;
    // Returns true, if image need to be converted from the pimage->Format to
    // destFormat.
    virtual bool        NeedToBeConverted(GImage* pimage, int destFormat) const = 0;
    
    // Write image to the disk.
    virtual bool        Write(GDXTHelper* pdxtHepler) = 0;

    // Read file and create GImage. This method used for sharing images. If this method
    // is not implemented (returns NULL) then images of this format cannot be shared
    virtual GImage*     Read(const char* filePath) { GUNUSED(filePath); return NULL; }
};

class GFxDataExporter
{
    class Loader : public GFxLoader
    {
    public:
        bool DoesNeedTag(int tagType) const
        {
            return (tagType <= 1 || GFxLoader::CheckTagLoader(tagType));
        }
    };

    void                DumpFsCommandsAsTree(FILE* fout, GTL::garray<GFxFsCommandOrigin>& fscommands, 
        GTL::garray<GFxString>& stringHolderArray, int indent);
    void                MakeFsCommandsAsList(GTL::garray<GFxFsCommandOrigin>& fscommands, 
        GTL::garray<GFxString>& stringHolderArray, GTL::garray<GFxString>& cmdList,
        GTL::garray<int>& cmdSortedIdx);
    void                LookForFsCommandsInTags
        (GFxMovieDef* pmovieDef, GFile* pin, UInt finalOffset, GTL::garray<GFxFsCommandOrigin>& fscommands, 
        GTL::garray<GFxString>& stringHolderArray);

public:
    GDXTHelper                                  DXTHelper;
    GPtr<GFxMovieDef>                           pMovieDef;
    GFxMovieInfo                                MovieInfo;
    Loader                                      Loader;
    GTL::ghash<int, GFxImageExporter*>          ImageExporters;
    GTL::ghash<UPInt, GTL::garray<GFxString> >  SharedImagesHash;
    GFxStringHash<int>                          SharedImageNamesSet;
    struct GFxFontTextureDescr
    {
        struct FontDescr
        {
            int             FontId;
            GPtr<GFxFontResource>   pFont;
            struct Pair
            {
                int IndexInFont;
                int IndexInTexture;

                inline Pair() : IndexInFont(0), IndexInTexture(0) {}
                inline Pair(int i1, int i2) : IndexInFont(i1), IndexInTexture(i2) {}
            };
            GTL::garray<Pair> TexGlyphsIndicesMap; // map index of tex glyphs (index_of_tex_glyph_in_font <=> (index in texture))
        };
        GPtr<GImageInfoBase>    pTexture;
        GTL::garray<FontDescr>  Fonts;
        GTL::garray<GPtr<GFxTextureGlyph> > TextureGlyphs;
    };

    struct GFxGradientTextureDescr
    {
        UInt32              MasterId;
        GTL::garray<UInt32> SlaveIds;
    };

    struct GFxFontTextureCounter : public GRefCountBase<GFxFontTextureCounter>
    {
        UInt Counter;

        GFxFontTextureCounter(UInt c):Counter(c) {}
    };
    
    typedef GTL::ghash_identity<UInt32, GFxFontTextureDescr>                FontTexturesHashType;
    typedef GTL::ghash_identity<GImageInfoBase*, GFxGradientTextureDescr>   GradientTexturesHashType;
    typedef GTL::ghash_identity<UInt32, GFxExportImageDescr>                ExportImagesHashType;

    typedef GTL::ghash_identity<UInt32, GPtr<GFxFontTextureCounter> >       FontTextureCounterHashType; // TexId -> Counter
    typedef GTL::ghash_identity<UInt16, FontTextureCounterHashType >        FontTextureUseHashType; // FontId -> Texture Ids + UseCnt

    FontTexturesHashType        FontTextures;
    FontTextureUseHashType      FontTextureUse;
    GradientTexturesHashType    GradientTextures;

    FILE*                       TextureListFile;

    // Options
    GTL::garray<GFxString>      SrcFileNames;
    GFxString                   OutputRootDir;
    GFxString                   Prefix;
    GFxString                   ImagesFormat;   // TGA, DDS....
    bool                        Info;           //Information only, files are not written 
    bool                        DoCompress;
    bool                        DoStrip;
    bool                        DoCreateSubDirs;
    bool                        UncompressedDDS;
    bool                        ToUppercase;
    bool                        ToLowercase;
    bool                        NoExportNames;
    bool                        DTX1Allowed;    // for non-alpha RGB data
    int                         DTXn;           // 3 or 5
    bool                        GenMipMapLevels;
    bool                        Quiet;
    bool                        QuietProgress;
    GDXTHelper::QualitySetting  Quality;
    GDXTHelper::RescaleTypes    Rescale;
    GDXTHelper::RescaleFilterTypes     RescaleFilter;
    GDXTHelper::MipFilterTypes     MipFilter;
    bool                        JustStripImages;
    bool                        ShareImages;
    // Edit Textfields 
    bool                        ExportDefaultEditText;
    bool                        DefaultDynamicTextOnly;
    // FS Commands
    bool                        ExportFsCommands;
    bool                        FsCommandsAsTree;
    bool                        FsCommandsAsList;
    bool                        FsCommandsParams;
    // font options
    bool                        ExportFonts;
    bool                        ExportFontList;
    bool                        ExportTextureList;
    bool                        GlyphsStripped;
    int                         TextureGlyphNominalSize;
    int                         TextureGlyphPadPixels;
    int                         FontTextureWidth;
    int                         FontTextureHeight;
    GFxLoader::FileFormatType   FontImagesFormat;   // TGA, DDS
    UByte                       FontImagesBits;     // 8, 24 or 32
    bool                        UseSeparateFontTextures;
    // gradient options
    bool                        ExportGradients;
    UInt                        GradientSize;
    GFxLoader::FileFormatType   GradientImagesFormat;   // TGA, DDS
    UByte                       GradientImagesBits;     // 32 or 0 (for compressed DDS)
    GTL::garray<GFxString>      SharedGradDirs;
    GFxString                   GradientsOutputDir;

    ExportImagesHashType        ImageDescriptors;
    GTL::garray<GFxTagInfo>     TagsToBeRemoved, TagsWithActions;
    struct JpegDesc
    {
        int             TagType;
        UByte*          pData;
        UInt            DataSize;

        inline JpegDesc() : pData(0) {}
    };
    GTL::ghash<int,JpegDesc>    JpegDescriptors;
    UInt32                      FirstTagOffset;
    int                         TotalImageCount;
    int                         ImagesWrittenCount;
    int                         ImagesProcessedCount;
    int                         SharedImagesCount;
    UInt32                      TotalMemoryCount; //used for Info option
    GTL::garray<int>            GradientShareMap; // used for sharing gradients. Index in this array is
                                                  // gradient id & 0xFFFF, value - id of first identical 
                                                  // gradient; or 0, means this gradient is unique one.

    class Log : public GFxLog
    {
    public:
        // We override this function in order to do custom logging.
        virtual void    LogMessageVarg(LogMessageType messageType, const char* pfmt, va_list argList)
        {
            GUNUSED(messageType);
            // Output log to console
            vprintf(pfmt, argList);
        }
    };

    static GFile*       FileOpener(const char* url);

    // Removes the leading directory path
    static GFxString    CutPath(const GFxString& filename);
    // Removes the trailing filename extension
    static GFxString    CutExtension(const GFxString& filename);
    // Removes the file path leaves the directory
    static GFxString    CutFilename(const GFxString& filename);

    void LoadTagProgressCallback(const GFxProgressHandler::TagInfo& info);
    void ProgressCallback(const GFxProgressHandler::Info& info);

    friend struct ImageExtractorVisitor;
    class ImageExtractorVisitor : public GFxMovieDef::ResourceVisitor
    {
    public:
        GFxDataExporter& Exporter;
        GFxString        Name;
        GFxString        DestPath;

        ImageExtractorVisitor(GFxDataExporter& exporter, const GFxString& name, const GFxString& destPath) :
        Exporter(exporter), Name(name), DestPath(destPath) { }

        virtual void    Visit(GFxMovieDef* pmovieDef, GFxResource* presource,
                              GFxResourceId rid, const char* pexportName);

        inline ImageExtractorVisitor& operator=(const ImageExtractorVisitor&) { return *this; } // dummy
    };
    void    ExtractImage(GImageInfoBase* pimageInfoBase, GFxResourceId rid, const char* pswfName, 
                         const char* pdestPath, const char* pexportName);

    int     Load(const GFxString& fileName);

    int     GetImageFormatId() const;

    void    WriteStrippedSwf(const char* srcFileName, const char* dstFileName, const GFxString& name);
    void    WriteExternalImageTag(GFile* pout, UInt32 characterId, const GFxExportImageDescr& imgDescr);
    void    WriteExternalGradientImageTag(GFile* pout, UInt32 characterId, const GFxExportImageDescr& imgDescr);
    void    WriteGlyphTextureInfo(GFile* pout, UInt textureId, const GFxFontTextureDescr& textureDescr);

    void    WriteDefaultEditTextFieldText(GFxMovieDef* pmovieDef, const char* swfFileName, const GFxString& path, const GFxString& name);

    void    WriteFontList(GFxMovieDef* pmovieDef, const char* swfFileName, const GFxString& path, const GFxString& name);

    // *Added by ES*
    void    WriteTextureListBegin(GFxMovieDef* pmovieDef, const char* swfFileName, const GFxString& path, const GFxString& name);
    void    WriteTextureListEnd();

    enum FsCommandsMasks
    {
        FSTree = 1,
        FSList = 2
    };
    void    WriteFsCommands(GFxMovieDef* pmovieDef, const char* swfFileName, const GFxString& path, const GFxString& name, UInt mask);

    bool    FindIdenticalImage(const GFxString& imagePath, const GFxString& filePath, GFxExportImageDescr* pdescr, GFxImageExporter* pimageExporter);
public:

    GFxDataExporter(UByte verboseFlags = 0);
    ~GFxDataExporter();

    bool    CreateDestDirectory(const char* path);

    int     Process();

    int     ParseCommandLine(int argc, const char** const argv);

    int     CollectOriginalImageData(const char* srcFileName);
    void    ClearOriginalImageData();

    void                AddImageExporter(int imgFormatId, GFxImageExporter* pimgExp);
    GFxImageExporter*   GetImageExporter(int imgFormatId);

    void                ShowHelpScreen();

    // static methods
    static void         ShowVersion();
};

struct GFxImageExporterFactoryBase
{
    virtual ~GFxImageExporterFactoryBase() { }
    virtual GFxImageExporter* Create() = 0;
};

#define GFX_MAX_NUM_OF_IMAGE_EXPORTERS  10
extern GFxImageExporterFactoryBase* GFxImageExporterFactoriesArray[];
extern unsigned                     GFxImageExporterFactoriesArrayCount; 

#define GFX_REGISTER_IMAGE_EXPORTER(exporterClass) \
struct exporterClass##_Factory : public GFxImageExporterFactoryBase \
{   \
    GFxImageExporter* Create() { return new exporterClass(); } \
    exporterClass##_Factory() { GFxImageExporterFactoriesArray[GFxImageExporterFactoriesArrayCount++] = this; } \
} __instance_of_##exporterClass##_factory

#endif //INC_GFXEXPORT_H
