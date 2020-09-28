//============================================================================
// font2_xbox.cpp
//
// Copyright (c) 2005 Ensemble Studios
//============================================================================

//============================================================================
// INCLUDES
//============================================================================
#include "xgameRender.h"
#include "FontSystem2.h"
#include "atgfont.h"

//============================================================================
// BFont2::BFont2
//============================================================================
BFont2::BFont2() : 
   mpManager(NULL)
{
   reset();
}

//============================================================================
// BFont2::~BFont2
//============================================================================
BFont2::~BFont2()
{
   reset();
}

//============================================================================
// BFont2::reset
//============================================================================
void BFont2::reset()
{
   //-- Reset data.
   mpManager               = NULL;
   BFontManager2::clearFontInfo(mFontInfo);
   mScale=1.0f;
   mNames.reset();
   mFontSource=NULL;
}

//============================================================================
// BFont2::create
//============================================================================
bool BFont2::create(const BFontInfo& fontInfo, BFontManager2* pManager)
{
   //-- Start fresh.
   reset();

   //-- Make sure we got a manager.
   if (!pManager)
      return false;
   mpManager = pManager;

   //-- Copy the font info.
   mFontInfo = fontInfo;
   if (mFontInfo.mPixelHeight      < 0)    mFontInfo.mPixelHeight      = 0;
   if (mFontInfo.mBackgroundOffset < 0)    mFontInfo.mBackgroundOffset = 0;
   if (mFontInfo.mBackgroundAlpha  < 0.0f) mFontInfo.mBackgroundAlpha  = 0.0f;
   if (mFontInfo.mBackgroundAlpha  > 1.0f) mFontInfo.mBackgroundAlpha  = 1.0f;

   //-- Get a link to the font source.
   mFontSource = pManager->getFontSource(mFontInfo.mSourceFile);
   if(!mFontSource)
      return false;

   long h1=mFontSource->mPixelHeight;
   long h2=mFontInfo.mPixelHeight;
   if(h1!=h2 && h1!=0)
   {
      mScale=(float)h2/(float)h1;
      mFontInfo.mPixelHeight=(long)((float)h2*mScale);
   }

   //-- Done.
   return true;
}

//============================================================================
// BFont2::getFontInfo
//============================================================================
bool BFont2::getFontInfo(BFontInfo& fontInfo)
{
   if (!mpManager)
   {
      BFontManager2::clearFontInfo(fontInfo);
      return false;
   }

   //-- Copy it and return.
   fontInfo = mFontInfo;
   return true;
}

//============================================================================
// BFont2::begin
//============================================================================
void BFont2::begin()
{
   if(mFontSource)
   {
      if(mScale!=1.0f)
         mFontSource->mFont.SetScaleFactors(mScale, mScale);
      mFontSource->mFont.Begin();
   }
}

//============================================================================
// BFont2::drawText
//============================================================================
void BFont2::drawText(float x, float y, DWORD color, const char* text, long justification, float scale )
{
   BUString temp(text);
   drawText(x, y, color, temp.getPtr(), justification, scale );
}

//============================================================================
// BFont2::drawText
//============================================================================
void BFont2::drawText(float x, float y, DWORD color, const WCHAR* text, long justification, float scale )
{
   if(mFontSource)
   {
      DWORD dwFlags=0;
      switch(justification)
      {
         case JUSTIFY_LEFT:   dwFlags=ATGFONT_LEFT; break;
         case JUSTIFY_RIGHT:  dwFlags=ATGFONT_RIGHT; break;
         case JUSTIFY_CENTER: dwFlags=ATGFONT_CENTER_X; break;

      }

      mFontSource->mFont.SetScaleFactors( scale, scale );

      mFontSource->mFont.DrawText(x, y, color, text, dwFlags);

   }
}

//============================================================================
// BFont2::end
//============================================================================
void BFont2::end()
{
   if(mFontSource)
   {
      mFontSource->mFont.End();
      if(mScale!=1.0f)
         mFontSource->mFont.SetScaleFactors(1.0f, 1.0f);
   }
}

//============================================================================
// BFont2::getHeightFloat
//============================================================================
float BFont2::getHeightFloat()
{
   if(mFontSource)
      return mFontSource->mFont.GetFontHeight();
   return 0.0f;
}

//============================================================================
// BFont2::getHeight
//============================================================================
long BFont2::getHeight()
{
   if(mFontSource)
      return (long)(mFontSource->mFont.GetFontHeight()+0.5f);
   return 0;
}

//============================================================================
// BFont2::getTextWidth
//============================================================================
float BFont2::getTextWidth(const WCHAR* pChars) const
{
   //FIXME XBOX
   if(mFontSource)
      return mFontSource->mFont.GetTextWidth(pChars);
   return 0.0f;
}

//============================================================================
// BFont2::getTextWidth
//============================================================================
float BFont2::getTextWidth(const char* pChars) const
{
   BUString temp(pChars);
   return getTextWidth(temp);
}

//============================================================================
// BFont2::destroy
//============================================================================
void BFont2::destroy()
{
}

//============================================================================
// BFont2::addName
//============================================================================
void BFont2::addName(const BString& name)
{
   mNames.addToTail(name);
}

//============================================================================
// BFont2::removeName
//============================================================================
void BFont2::removeName(const BString& name)
{
   BHandle hName = mNames.findItemForward(name);
   if (hName)
      mNames.remove(hName);
}

//============================================================================
// BFont2::hasName
//============================================================================
bool BFont2::hasName(const BString& name)
{
   return (mNames.findItemForward(name) != NULL);
}

//============================================================================
// BFont2::getNumNames
//============================================================================
long BFont2::getNumNames()
{
   return mNames.getSize();
}

//============================================================================
// BFont2::getName
//============================================================================
bool BFont2::getName(long index, BString& name)
{
//-- FIXING PREFIX BUG ID 6601
   const BString* pName = mNames.getItem(index);
//--
   if (pName)
   {
      name = *pName;
      return true;
   }

   name.empty();
   return false;
}

//============================================================================
// BFont2::operator ==
//============================================================================
bool BFont2::operator == (const BFont2& font) const
{
   return (*this == font.mFontInfo);
}

//============================================================================
// BFont2::operator ==
//============================================================================
bool BFont2::operator == (const BFontInfo& fontInfo) const
{
   if ((mFontInfo.mTypeFace          == fontInfo.mTypeFace         ) &&
       (mFontInfo.mPixelHeight       == fontInfo.mPixelHeight      ) &&
       (mFontInfo.mAntialiased       == fontInfo.mAntialiased      ) &&
       (mFontInfo.mBold              == fontInfo.mBold             ) &&
       (mFontInfo.mItalic            == fontInfo.mItalic           ) &&
       (mFontInfo.mBackgroundOutline == fontInfo.mBackgroundOutline) &&
       (mFontInfo.mBackgroundShadow  == fontInfo.mBackgroundShadow ) &&
       (mFontInfo.mBackgroundOffset  == fontInfo.mBackgroundOffset ) &&
       (mFontInfo.mBackgroundAlpha   == fontInfo.mBackgroundAlpha  ))
       return true;

   return false;
}
