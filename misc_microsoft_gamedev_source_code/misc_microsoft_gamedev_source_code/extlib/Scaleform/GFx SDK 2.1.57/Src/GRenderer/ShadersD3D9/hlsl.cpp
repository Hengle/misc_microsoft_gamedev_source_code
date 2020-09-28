/**********************************************************************

Filename    :   hlsl.cpp
Content     :   Direct3D9 hlsl shaders
Created     :   
Authors     :   Michael Antonov & Andrew Reisse

Copyright   :   (c) 2001-2006 Scaleform Corp. All Rights Reserved.

Notes       :   

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#include "VShaderStrip.hlsl.h"
#include "VShaderGlyph.hlsl.h"
#include "VShaderStripXY16iC32.hlsl.h"
#include "VShaderStripXY16iCF32.hlsl.h"
#include "VShaderStripXY16iCF32_T2.hlsl.h"
#include "VShaderGlyphSzc.hlsl.h"
#include "VShaderStripXY16iC32Szc.hlsl.h"
#include "VShaderStripXY16iCF32Szc.hlsl.h"
#include "VShaderStripXY16iCF32Szc_T2.hlsl.h"
#include "VShaderStripXY32fCF32.hlsl.h"
#include "VShaderStripXYUV32fCF32.hlsl.h"
#include "VShaderStripXYUVUV32fCF32.hlsl.h"
#include "PS_SolidColor.hlsl.h"
#include "PS_CxformTexture.hlsl.h"
#include "PS_CxformTextureMultiply.hlsl.h"
#include "PS_TextTextureColor.hlsl.h"
#include "PS_CxformGauraud.hlsl.h"
#include "PS_CxformGauraudMultiply.hlsl.h"
#include "PS_CxformGauraudNoAddAlpha.hlsl.h"
#include "PS_CxformGauraudMultiplyNoAddAlpha.hlsl.h"
#include "PS_CxformGauraudTexture.hlsl.h"
#include "PS_CxformGauraudMultiplyTexture.hlsl.h"
#include "PS_Cxform2Texture.hlsl.h"
#include "PS_CxformMultiply2Texture.hlsl.h"
#include "PS_AcSolidColor.hlsl.h"
#include "PS_AcCxformTexture.hlsl.h"
#include "PS_AcCxformTextureMultiply.hlsl.h"
#include "PS_AcTextTexture.hlsl.h"
#include "PS_AcTextTextureColor.hlsl.h"
#include "PS_AcCxformGauraud.hlsl.h"
#include "PS_AcCxformGauraudMultiply.hlsl.h"
#include "PS_AcCxformGauraudNoAddAlpha.hlsl.h"
#include "PS_AcCxformGauraudMultiplyNoAddAlpha.hlsl.h"
#include "PS_AcCxformGauraudTexture.hlsl.h"
#include "PS_AcCxformGauraudMultiplyTexture.hlsl.h"
#include "PS_AcCxform2Texture.hlsl.h"
#include "PS_AcCxformMultiply2Texture.hlsl.h"