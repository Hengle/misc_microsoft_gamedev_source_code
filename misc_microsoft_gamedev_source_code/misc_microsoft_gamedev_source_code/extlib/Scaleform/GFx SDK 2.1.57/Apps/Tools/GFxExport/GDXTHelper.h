/**********************************************************************

Filename    :   GDXTHelper.h
Content     :   Interface to NVidia DXT library 
Created     :   Sep, 2006
Authors     :   Artyom Bolgar
Copyright   :   (c) 2006 Scaleform Corp. All Rights Reserved.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef INC_GDXTHELPER_H
#define INC_GDXTHELPER_H

// GFx Includes
#include "GTypes.h"
#include "GRefCount.h"
#include "GResizeImage.h"


// ***** GDXTHelper
// Does image rescaling and provides interface for nvtt library. 
// Used in GFxExport

class GDXTHelper
{
public:
    GDXTHelper()
    {
        pRescaleFilterLut=0;
    }
    virtual ~GDXTHelper()
    {
        if(pRescaleFilterLut)
            delete pRescaleFilterLut;
    }
    enum QualitySetting 
    {
        QualityFastest,
        QualityNormal,
        QualityProduction,
        QualityHighest,
    };
    enum RescaleTypes
    {
        RescaleNone,                // no rescale
        RescaleNearestPower2,       // rescale to nearest power of two
        RescaleBiggestPower2,       // rescale to next bigger power of 2
        RescaleSmallestPower2,      // rescale to smaller power of 2 
        RescaleNextSmallestPower2,  // rescale to next smaller power of 2
        RescalePreScale,            // rescale to this size
        RescaleRelScale,            // relative rescale
    };
    enum MipFilterTypes
    {
        nvFilterBox,      
        nvFilterTriangle, 
        nvFilterKaiser,
    };
    enum RescaleFilterTypes
    { 
        FilterBox,      
        FilterTriangle, 
        FilterQuadratic,
        FilterCatrom,   
        FilterMitchell, 
        FilterGaussian, 
        FilterSinc,     
        FilterBessel,   
        FilterHanning,  
        FilterHamming,  
        FilterBlackman, 
        FilterKaiser,
        FilterBicubic,
        FilterSpline16,
        FilterSpline36,
        FilterHermite,
        FilterQuadric,
        FilterLanczos,
        FilterBilinear,
    };
    static int LastError;
    GImageFilterLut * pRescaleFilterLut;

    //Calculates destination width and height, made separate function for Info mode 
    static void CalculateWH(UInt32 srcW, UInt32 srcH, UInt32 &destW, UInt32 &destH, GDXTHelper::RescaleTypes rescale);
    //
    static UInt32 CalculateDDSSize( UInt32 srcW, UInt32 srcH, GImage::ImageFormat format,                               int dxtN,
        bool isUncompressedDDS,
        bool doGenMipMap,
        bool isDxt1Allowed);
    
    GImage* Rescale(const GImage* psrcimage, 
                           GDXTHelper::RescaleTypes rescale, 
                           GDXTHelper::RescaleFilterTypes rescaleFilter, 
                         /*  unsigned targetWidth,
                           unsigned targetHeight,*/
                           GImage::ImageFormat destImageFormat);

    bool CompressToFile(const GImage* psrcimage, 
                               GFile* pdstFile, 
                               GDXTHelper::QualitySetting quality, 
                               GDXTHelper::RescaleTypes rescale,
                               GDXTHelper::RescaleFilterTypes rescaleFilter, 
                               GDXTHelper::MipFilterTypes mipFilter, 
                            /*   unsigned targetWidth,
                               unsigned targetHeight, */
                               int dxtN,
                               bool isUncompressedDDS,
                               bool doGenMipMap,
                               bool isDxt1Allowed);
    
    static const char*  GetLastErrorString();
    void CalculateResizeFilter(RescaleFilterTypes FilterType);
    GImage* CreateResizedImage(const GImage *psrc, GDXTHelper::RescaleTypes rescale, GDXTHelper::RescaleFilterTypes filterType /*, unsigned targetWidth, unsigned targetHeight*/);
};
#endif //INC_GDXTHELPER_H
