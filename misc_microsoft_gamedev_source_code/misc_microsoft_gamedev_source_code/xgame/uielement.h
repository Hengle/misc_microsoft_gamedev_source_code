//==============================================================================
// uielement.h
//
// Copyright (c) 2006-2007 Ensemble Studios
//==============================================================================
#pragma once

// Includes
#include "bitvector.h"
#include "ui.h"

//==============================================================================
// BUIElement
//==============================================================================
class BUIElement
{
   public:
      enum
      {
         cFlagUseAlpha,
         cFlagTextureLoaded,
         cFlagHidden,
         cFlagOverbright,
         cElementFlagCount
      };

                              BUIElement();
                              BUIElement(const BUIElement& source) { *this=source; }
                              BUIElement& operator=(const BUIElement& source);
                              virtual ~BUIElement();

      void                    setPosition(long x, long y) { mX=x; mY=y; }
      void                    setSize(long width, long height) { mWidth=width; mHeight=height; }
      void                    setColor(DWORD color) { mColor=color; }
      void                    setFont(BHandle hFont) { mFontHandle=hFont; }
      void                    setTexture(const char* pTextureName, bool useAlpha);
      void                    setTexture(BManagedTextureHandle textureHandle, bool useAlpha);
      void                    setText(const WCHAR* pText) { mText=pText; }
      void                    setTextJust(long just) { mTextJust=just; }
      void                    setHidden(bool val) { setFlag(cFlagHidden, val); }

      long                    getX() const { return mX; }
      long                    getY() const { return mY; }
      long                    getWidth() const { return mWidth; }
      long                    getHeight() const { return mHeight; }
      BHandle                 getFont() const { return mFontHandle; }
      bool                    getHidden() const { return getFlag(cFlagHidden); }

      virtual void            refresh();
      virtual void            update() { }
      virtual void            render(long parentX, long parentY);
      virtual bool            handleInput(long port, long event, long controlType, BInputEventDetail& detail) { port; event; controlType; detail; return false; }
      virtual void            cleanup();

      bool                    getFlag(long n) const { return(mFlags.isSet(n)!=0); }
      void                    setFlag(long n, bool v) { if(v) mFlags.set(n); else mFlags.unset(n); }

   protected:
      UTBitVector<8>          mFlags;
      long                    mX;
      long                    mY;
      long                    mWidth;
      long                    mHeight;
      DWORD                   mColor;
      BHandle                 mFontHandle;
      BManagedTextureHandle          mTextureHandle;
      BUString                mText;
      long                    mTextJust;
      BSimString              mTextureName;

};