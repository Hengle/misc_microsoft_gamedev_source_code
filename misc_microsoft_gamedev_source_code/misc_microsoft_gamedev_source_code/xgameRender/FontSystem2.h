//============================================================================
//
//  FontSystem2.h
//
//  Copyright (c) 2000 Ensemble Studios
//
//============================================================================


#ifndef __FONT_SYSTEM_2_H__
#define __FONT_SYSTEM_2_H__

#if (_MSC_VER > 1200)
//#include <Wingdi.h>
#endif
#include "color.h"

#ifdef XBOX
#include "AtgFont.h"
#endif

#include "D3DTextureManager.h"

//----------------------------------------------------------------------------
//  Externs
//----------------------------------------------------------------------------
class BFont2;
class BMaterial;


//----------------------------------------------------------------------------
//  Public Enums
//----------------------------------------------------------------------------
enum FONT_JUSTIFICATION
{
   JUSTIFY_LEFT = 0,
   JUSTIFY_RIGHT,
   JUSTIFY_CENTER
};


//----------------------------------------------------------------------------
//  Public Structs
//----------------------------------------------------------------------------
struct BFontInfo
{
#ifdef XBOX
   BString  mSourceFile;
#endif
   BString  mTypeFace;
   long     mPixelHeight;
   bool     mAntialiased;
   bool     mBold;
   bool     mItalic;
   bool     mBackgroundOutline;
   bool     mBackgroundShadow;
   long     mBackgroundOffset;
   float    mBackgroundAlpha;
   float    mSpacingAdjustment;
   long     mHorizontalOffset;
   long     mVerticalOffset;
};

struct BFontClipRect
{
   long x1;
   long y1;
   long x2;
   long y2;
};

struct BCharacterExtent2
{
   float mExtentX;
   float mExtentY;
   float mPreAdvanceX;
   float mPostAdvanceX;
};


//----------------------------------------------------------------------------
//  Private Structs
//----------------------------------------------------------------------------
struct BFontMaterial
{
   BFont2* mpFont;
   BManagedTextureHandle mTextureIndex;
};

struct BFontCharacter
{
   float   u1;
   float   v1;
   float   u2;
   float   v2;
   float   mCellBoxWidth;
   float   mCellBoxHeight;
   float   mPreAdvance;
   float   mPostAdvance;
   float   mYAdvance;
   BHandle mhFontMaterial;
   WCHAR   mCharacter;
   bool    mIsWhiteSpace;
};

#ifndef XBOX
struct BFontMask
{
   BString  mTypeFace;
   DWORD    mMask[2048];
};

struct BFontMaskFile
{
   BString mTypeFace;
   BString mMSKFileName;
};
#endif

#ifdef XBOX
struct BFontSource
{
   BString     mName;
   BString     mTypeFace;
   BString     mFileName;
   long        mPixelHeight;
   ATG::Font   mFont;
};
#endif

struct BFontSprite
{
   float x1;
   float y1;
   float x2;
   float y2;
   float u1;
   float v1;
   float u2;
   float v2;
};

struct BFontBatch
{
   BFont2*                mpFont;
   BHandle                mhFontMaterial;
   float                  mAlpha;
   BColor                 mColor;
   BCopyList<BFontSprite> mCharacters;
};

struct LineData
{
   const WCHAR*           mpChars;
   long                   mNumChars;
   float                  mLineHeight;
   float                  mX;
   float                  mY;
   float                  mAlpha;
   float                  mClipX1;
   float                  mClipY1;
   float                  mClipX2;
   float                  mClipY2;
   BColor                 mColor;
   const BColor*          mpBGColor;
   float                  mBGAlpha;
   const BColor*          mpOutColor;
   float                  mOutAlpha;
};


//----------------------------------------------------------------------------
//  Includes
//----------------------------------------------------------------------------
#ifdef XBOX
   #include "Font2_xbox.h"
   #include "FontManager2_xbox.h"
#else
   #include "FontCharHashTable.h"
   #include "Font2.h"
   #include "FontMaterialManager.h"
   #include "FontManager2.h"
#endif

#endif

extern BFontManager2 gFontManager;
