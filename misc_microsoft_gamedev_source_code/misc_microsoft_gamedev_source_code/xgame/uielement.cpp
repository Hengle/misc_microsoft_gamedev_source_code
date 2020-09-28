//==============================================================================
// uielement.cpp
//
// Copyright (c) 2006 Ensemble Studios
//==============================================================================

// Includes
#include "common.h"
#include "uielement.h"
#include "debugprimitives.h"
#include "FontSystem2.h"
#include "ui.h"

//==============================================================================
// BUIElement::BUIElement
//==============================================================================
BUIElement::BUIElement() : 
   mFlags(), 
   mX(0), 
   mY(0), 
   mWidth(0), 
   mHeight(0), 
   mColor(cDWORDWhite), 
   mFontHandle(NULL), 
   mTextureHandle(cInvalidManagedTextureHandle),
   mTextJust(BFontManager2::cJustifyCenter),
   mText()
{
}

//==============================================================================
// BUIElement::operator=
//==============================================================================
BUIElement& BUIElement::operator=(const BUIElement& source)
{
   if(this==&source)
      return *this;

   mFlags=source.mFlags;
   mX=source.mX;
   mY=source.mY;
   mWidth=source.mWidth;
   mHeight=source.mHeight;
   mColor=source.mColor;
   mFontHandle=source.mFontHandle;
   mTextureHandle=source.mTextureHandle;
   mText=source.mText;
   mTextureName=source.mTextureName;

   return *this;
}

BUIElement::~BUIElement()
{
}

//==============================================================================
// BUIElement::setTexture
//==============================================================================
void BUIElement::setTexture(const char* pTextureName, bool useAlpha)
{
   mTextureName=pTextureName;
   setFlag(cFlagUseAlpha, useAlpha);

   if(getFlag(cFlagTextureLoaded))
   {
      gUI.unloadTexture(mTextureHandle);
      setFlag(cFlagTextureLoaded, false);
   }

   mTextureHandle=cInvalidManagedTextureHandle;

   if(!mTextureName.isEmpty())
   {
      mTextureHandle=gUI.loadTexture(mTextureName);
      setFlag(cFlagTextureLoaded, true);
   }
}

//==============================================================================
// BUIElement::setTexture
//==============================================================================
void BUIElement::setTexture(BManagedTextureHandle textureHandle, bool useAlpha)
{
   if(getFlag(cFlagTextureLoaded))
   {
      gUI.unloadTexture(mTextureHandle);
      setFlag(cFlagTextureLoaded, false);
   }

   mTextureHandle=textureHandle;

   setFlag(cFlagUseAlpha, useAlpha);
}

//==============================================================================
// BUIElement::refresh
//==============================================================================
void BUIElement::refresh()
{
}

//==============================================================================
// BUIElement::cleanup
//==============================================================================
void BUIElement::cleanup()
{
   if(getFlag(cFlagTextureLoaded))
   {
      gUI.unloadTexture(mTextureHandle);
      setFlag(cFlagTextureLoaded, false);
   }
   mTextureHandle=cInvalidManagedTextureHandle;
}

//==============================================================================
// BUIElement::render
//==============================================================================
void BUIElement::render(long parentX, long parentY)
{
   if(getFlag(cFlagHidden))
      return;
   float x1=(float)(parentX+mX);
   float y1=(float)(parentY+mY);
   float x2=(float)(x1+mWidth-1);
   float y2=(float)(y1+mHeight-1);
   if(mTextureHandle!=cInvalidManagedTextureHandle)
      gUI.renderTexture(mTextureHandle, (long)x1, (long)y1, (long)x2, (long)y2, getFlag(cFlagUseAlpha), mColor, getFlag(cFlagOverbright));
   if(!mText.isEmpty())
   {
      gFontManager.drawText(mFontHandle, x1, y1, mText, mColor, mTextJust);
   }
}
