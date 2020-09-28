/**********************************************************************

Filename    :   GFxTagLoaders.h
Content     :   GFxPlayer tag loader implementation
Created     :   June 30, 2005
Authors     :   

Copyright   :   (c) 2001-2006 Scaleform Corp. All Rights Reserved.

Notes       :   

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef INC_GFXTAGLOADERS_H
#define INC_GFXTAGLOADERS_H

#include "GTypes.h"
#include "GFxTags.h"

// *** Forward Decralations
class GFxStream;
class GFxLoadProcess;
class GFxMovieDataDef;
struct GFxTagInfo;


//
// ***** File Loader callback functions.
//

// Tag loader functions.
void    GSTDCALL GFx_NullLoader(GFxLoadProcess* p, const GFxTagInfo& tagInfo);

void    GSTDCALL GFx_SetBackgroundColorLoader(GFxLoadProcess* p, const GFxTagInfo& tagInfo);
void    GSTDCALL GFx_JpegTablesLoader(GFxLoadProcess* p, const GFxTagInfo& tagInfo);
void    GSTDCALL GFx_DefineBitsJpegLoader(GFxLoadProcess* p, const GFxTagInfo& tagInfo);
void    GSTDCALL GFx_DefineBitsJpeg2Loader(GFxLoadProcess* p, const GFxTagInfo& tagInfo);
void    GSTDCALL GFx_DefineBitsJpeg3Loader(GFxLoadProcess* p, const GFxTagInfo& tagInfo);
void    GSTDCALL GFx_DefineShapeLoader(GFxLoadProcess* p, const GFxTagInfo& tagInfo);
void    GSTDCALL GFx_DefineShapeMorphLoader(GFxLoadProcess* p, const GFxTagInfo& tagInfo);
void    GSTDCALL GFx_DefineFontLoader(GFxLoadProcess* p, const GFxTagInfo& tagInfo);
void    GSTDCALL GFx_DefineFontInfoLoader(GFxLoadProcess* p, const GFxTagInfo& tagInfo);
void    GSTDCALL GFx_DefineTextLoader(GFxLoadProcess* p, const GFxTagInfo& tagInfo);
void    GSTDCALL GFx_DefineEditTextLoader(GFxLoadProcess* p, const GFxTagInfo& tagInfo);
void    GSTDCALL GFx_PlaceObject2Loader(GFxLoadProcess* p, const GFxTagInfo& tagInfo);
void    GSTDCALL GFx_DefineBitsLossless2Loader(GFxLoadProcess* p, const GFxTagInfo& tagInfo);
void    GSTDCALL GFx_SpriteLoader(GFxLoadProcess* p, const GFxTagInfo& tagInfo);
void    GSTDCALL GFx_EndLoader(GFxLoadProcess* p, const GFxTagInfo& tagInfo);
void    GSTDCALL GFx_RemoveObject2Loader(GFxLoadProcess* p, const GFxTagInfo& tagInfo);
void    GSTDCALL GFx_DoActionLoader(GFxLoadProcess* p, const GFxTagInfo& tagInfo);
void    GSTDCALL GFx_ButtonCharacterLoader(GFxLoadProcess* p, const GFxTagInfo& tagInfo);
void    GSTDCALL GFx_FrameLabelLoader(GFxLoadProcess* p, const GFxTagInfo& tagInfo);
void    GSTDCALL GFx_ExportLoader(GFxLoadProcess* p, const GFxTagInfo& tagInfo);
void    GSTDCALL GFx_ImportLoader(GFxLoadProcess* p, const GFxTagInfo& tagInfo);
void    GSTDCALL GFx_DefineSoundLoader(GFxLoadProcess* p, const GFxTagInfo& tagInfo);
void    GSTDCALL GFx_StartSoundLoader(GFxLoadProcess* p, const GFxTagInfo& tagInfo);
void    GSTDCALL GFx_ButtonSoundLoader(GFxLoadProcess* p, const GFxTagInfo& tagInfo);
void    GSTDCALL GFx_DoInitActionLoader(GFxLoadProcess* p, const GFxTagInfo& tagInfo);
// SWF 8
void    GSTDCALL GFx_MetadataLoader(GFxLoadProcess* p, const GFxTagInfo& tagInfo);
void    GSTDCALL GFx_FileAttributesLoader(GFxLoadProcess* p, const GFxTagInfo& tagInfo);
void    GSTDCALL GFx_CSMTextSettings(GFxLoadProcess* p, const GFxTagInfo& tagInfo);
void    GSTDCALL GFx_Scale9GridLoader(GFxLoadProcess* p, const GFxTagInfo& tagInfo);
// Stripper custom tags loaders
void    GSTDCALL GFx_ExporterInfoLoader(GFxLoadProcess* p, const GFxTagInfo& tagInfo);
void    GSTDCALL GFx_DefineExternalImageLoader(GFxLoadProcess* p, const GFxTagInfo& tagInfo);
void    GSTDCALL GFx_FontTextureInfoLoader(GFxLoadProcess* p, const GFxTagInfo& tagInfo);
void    GSTDCALL GFx_DefineExternalGradientImageLoader(GFxLoadProcess* p, const GFxTagInfo& tagInfo);
void    GSTDCALL GFx_DefineGradientMapLoader(GFxLoadProcess* p, const GFxTagInfo& tagInfo);
// SoundStreamLoader(); // head, head2, block


// ***** Tag Lookup Tables

typedef void (GSTDCALL *GFx_TagLoaderFunction)(GFxLoadProcess*, const GFxTagInfo&);

extern GFx_TagLoaderFunction GFx_SWF_TagLoaderTable[GFxTag_SWF_TagTableEnd];
extern GFx_TagLoaderFunction GFx_GFX_TagLoaderTable[GFxTag_GFX_TagTableEnd - GFxTag_GFX_TagTableBegin];


#endif
