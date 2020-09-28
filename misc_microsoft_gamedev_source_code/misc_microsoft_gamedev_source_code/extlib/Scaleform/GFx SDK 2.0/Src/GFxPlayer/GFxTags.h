/**********************************************************************

Filename    :   GFxTags.h
Content     :   Tags constants used by the SWF/GFX Files
Created     :   June 30, 2005
Authors     :   

Copyright   :   (c) 2001-2006 Scaleform Corp. All Rights Reserved.

Notes       :   

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef INC_GFXTAGS_H
#define INC_GFXTAGS_H


// ***** SWF/GFX File Tag Constants

enum GFxTagType
{
    // Add the standard loaders.
    GFxTag_End                      = 0,
    GFxTag_EndFrame                 = 1,
    GFxTag_DefineShape              = 2,
    GFxTag_PlaceObject              = 4,
    GFxTag_RemoveObject             = 5,
    GFxTag_DefineBitsJpeg           = 6,
    GFxTag_ButtonCharacter          = 7,
    GFxTag_JpegTables               = 8,
    GFxTag_SetBackgroundColor       = 9,
    GFxTag_DefineFont               = 10,
    GFxTag_DefineText               = 11,
    GFxTag_DoAction                 = 12,
    GFxTag_DefineFontInfo           = 13,
    GFxTag_DefineSound              = 14,
    GFxTag_StartSound               = 15,
    GFxTag_ButtonSound              = 17,
    GFxTag_DefineBitsLossless       = 20,
    GFxTag_DefineBitsJpeg2          = 21,
    GFxTag_DefineShape2             = 22,
    GFxTag_Protect                  = 24,
    GFxTag_PlaceObject2             = 26,
    GFxTag_RemoveObject2            = 28,
    GFxTag_DefineShape3             = 32,
    GFxTag_DefineText2              = 33,
    GFxTag_DefineEditText           = 37,
    GFxTag_ButtonCharacter2         = 34,
    GFxTag_DefineBitsJpeg3          = 35,
    GFxTag_DefineBitsLossless2      = 36,
    GFxTag_Sprite                   = 39,
    GFxTag_FrameLabel               = 43,
    GFxTag_DefineShapeMorph         = 46,
    GFxTag_DefineFont2              = 48,
    GFxTag_Export                   = 56,
    GFxTag_Import                   = 57,
    GFxTag_DoInitAction             = 59,
    GFxTag_DefineFontInfo2          = 62,
    // SWF8 starts (may need to add more tags)
    GFxTag_FileAttributes           = 69,
    GFxTag_PlaceObject3             = 70,
    GFxTag_Import2                  = 71,
    GFxTag_CSMTextSettings          = 74,
    GFxTag_DefineFont3              = 75,
    GFxTag_Metadata                 = 77,
    GFxTag_DefineShape4             = 83,
    GFxTag_DefineShapeMorph2        = 84,

    // Size of table for lookup arrays.
    GFxTag_SWF_TagTableEnd,

    // GFx Extension tags.
    GFxTag_ExporterInfo             = 1000,
    GFxTag_DefineExternalImage      = 1001,
    GFxTag_FontTextureInfo          = 1002,
    GFxTag_DefineExternalGradient   = 1003,
    GFxTag_DefineGradientMap        = 1004,
    
    // Size of table for lookup arrays.
    GFxTag_GFX_TagTableEnd,
    GFxTag_GFX_TagTableBegin        = 1000,
};


#endif
