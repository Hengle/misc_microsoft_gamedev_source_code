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

class GDXTHelper
{
public:
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
    enum FilterTypes
    {
        FilterPoint,    
        FilterBox,      
        FilterTriangle, 
        FilterQuadratic,
        FilterCubic,    
        FilterCatrom,   
        FilterMitchell, 
        FilterGaussian, 
        FilterSinc,     
        FilterBessel,   
        FilterHanning,  
        FilterHamming,  
        FilterBlackman, 
        FilterKaiser,
    };

    static int LastError;

    static GImage* Rescale(const GImage* psrcimage, 
                           GDXTHelper::RescaleTypes rescale, 
                           GDXTHelper::FilterTypes rescaleFilter, 
                           unsigned targetWidth,
                           unsigned targetHeight,
                           GImage::ImageFormat destImageFormat);

    static bool CompressToFile(const GImage* psrcimage, 
                               GFile* pdstFile, 
                               GDXTHelper::QualitySetting quality, 
                               GDXTHelper::RescaleTypes rescale,
                               GDXTHelper::FilterTypes rescaleFilter, 
                               GDXTHelper::FilterTypes mipFilter, 
                               unsigned targetWidth,
                               unsigned targetHeight,
                               int dxtN,
                               bool isUncompressedDDS,
                               bool doGenMipMap,
                               bool isDxt1Allowed);
    
    static const char*  GetLastErrorString();
};

#endif //INC_GDXTHELPER_H
