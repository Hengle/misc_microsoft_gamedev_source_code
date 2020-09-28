//============================================================================
// FontManager2_xbox.h
//
// Copyright (c) 2005 Ensemble Studios
//============================================================================
#pragma once
#ifndef _FONTMANAGER2_XBOX_H_
#define _FONTMANAGER2_XBOX_H_

#include "xmlreader.h"

//----------------------------------------------------------------------------
//  TypeDefs
//----------------------------------------------------------------------------
typedef long BJustification;

//----------------------------------------------------------------------------
//  Externs
//----------------------------------------------------------------------------


//----------------------------------------------------------------------------
// class BTextData
//----------------------------------------------------------------------------
class BTextData
{
public:
   BTextData();
   
   void setPos2D(float x, float y) { mPos.set(x, y, 0.0f); mFlag3DPos = false; mFlagRenderIn3DViewport = false; }
   void setPos3D(BVector pos) { mPos = pos; mFlag3DPos = true; mFlagRenderIn3DViewport = true; }
   void setText(const char* pText) { mText.set(pText); }
   void setText(const char* pText, long maxLength) { mText.set(pText, maxLength); }
   void setText(const WCHAR* pText) { mText.set(pText); }
   void setText(const WCHAR* pText, long maxLength) { mText.set(pText, maxLength); }
   void setFont(BHandle hFont) { mhFont = hFont; }
   void setColor(DWORD c) { mColor = c; }
   void setColor(const BColor& c) { mColor = c.asDWORD(); }
   void setScale(float scale) { mScale = scale; }
   void setJustification(BJustification j) { mJustification = j; }
   void setFlagDeleteOnSimUpdate() { mFlagDeleteOnSimUpdate = true; }
   void setFlagRenderIn3DViewport(bool flag) { mFlagRenderIn3DViewport = false; }

   BVector getPos() const { return (mPos); }
   const BUString& getText() const { return (mText); }
   BHandle getFont() const { return (mhFont); }
   DWORD getColor() const { return (mColor); }
   float getScale() const { return (mScale); }
   BJustification getJustification() const { return (mJustification); }
   bool getFlag3DPos() const { return (mFlag3DPos); }
   bool getFlagDeleteOnSimUpdate() const { return (mFlagDeleteOnSimUpdate); }
   bool getFlagRenderIn3DViewport() const { return (mFlagRenderIn3DViewport); }

   void copy(const BTextData& src);
   void reset();

protected:

   BVector mPos;
   BUString mText;
   BHandle mhFont;
   DWORD mColor;
   float mScale;
   BJustification mJustification;
   bool mFlag3DPos              : 1;
   bool mFlagRenderIn3DViewport : 1;
   bool mFlagDeleteOnSimUpdate  : 1;
};

//----------------------------------------------------------------------------
//  Class BFontManager2
//----------------------------------------------------------------------------
class BFontManager2
{
   public:
      //-- Construction/Destruction
      BFontManager2();
      ~BFontManager2();

      void                    setBaseDirectoryID(long ID) { mBaseDirectoryID = ID; }
      long                    getBaseDirectoryID(void) const { return mBaseDirectoryID; }

      //-- Setup Interface
      void     setDefaultFont     (const BString &name)   { mDefaultFont=name; }
      BHandle  getDefaultFont     ( void )                { return findFont(mDefaultFont); }

      //-- Font Control
      BHandle createFont         (const BString& name, const BFontInfo& fontInfo);
      BHandle findFont           (const BString& name);
      BHandle findFont           (const BString& name, long pixelHeight);
      BHandle findFont           (const BFontInfo& fontInfo);
      void    destroyFont        (const BString& name);
      void    setFont            (BHandle hFont);
      BFont2* getFont            (BHandle hFont);
      BFontSource* getFontSource (const BString& sourceName);
      float   getLineHeight      ();
      float   getLineLength      (const WCHAR* pChars, long numChars);
      float   getLineLength      (const char* pChars, long numChars);

      // High-level string rendering functions that used to be in materialmanager.
      enum
      {
         cJustifyLeft=0,
         cJustifyCenter,
         cJustifyXCenter,
         cJustifyYCenter,
         cJustifyRight,
         cJustifyLeftRight,
         cJustifyRightYCenter,
      };

      // drawText() queues up requests to draw debug text on the screen.
      // Interfaces include:
      // 1.) A 2d screen position.
      // 2.) A 3d world position (that gets converted to screen space at render time.)
      // 3.) A filled out BTextData class.
      BTextData* drawText(BHandle hFont, float x, float y, const char* pStr, DWORD dwColor = cDWORDWhite, BJustification j = BFontManager2::cJustifyLeft, float scale = 1.0f, bool renderIn3DViewport = false);
      BTextData* drawText(BHandle hFont, float x, float y, const WCHAR* pStr, DWORD dwColor = cDWORDWhite, BJustification j = BFontManager2::cJustifyLeft, float scale = 1.0f, bool renderIn3DViewport = false);
      BTextData* drawText(BHandle hFont, BVector worldPos, const char* pStr, DWORD dwColor = cDWORDWhite, BJustification j = BFontManager2::cJustifyLeft, float scale = 1.0f);
      BTextData* drawText(BHandle hFont, BVector worldPos, const WCHAR* pStr, DWORD dwColor = cDWORDWhite, BJustification j = BFontManager2::cJustifyLeft, float scale = 1.0f);
      BTextData* drawText(const BTextData& src);

      // Renders all the drawText() calls in one place.
      void render3D();
      void render2D();
      void onUpdateSimStart();
      
      //-- System Interface
      void destroyAllMaterials();
      void destroyAllFonts();
      void createFontsFromXML();
      void initFontHandles();

      //-- Static Functions
      static void clearFontInfo(BFontInfo& fontInfo);

      //-- Legacy Compatability Interface
      BVector           stringExtents             (BHandle hFont, const BString& str);
      BCharacterExtent2 getCharacterExtent        (BHandle hFont, WCHAR character);
      float             getMaximumCharacterHeight (BHandle hFont);
      float             getMaximumCharacterAscent (BHandle hFont);
      float             getMaximumCharacterDescent(BHandle hFont);
      float             getCharPosInString        (BHandle hFont, WCHAR character, const BString& text);

      BHandle getFontArial10() const { return (mFontArial10); }
      BHandle getFontCourier10() const { return (mFontCourier10); }
      BHandle getFontDenmark14() const { return (mFontDenmark14); }
      BHandle getFontDenmark16() const { return (mFontDenmark16); }
      BHandle getFontDenmark18() const { return (mFontDenmark18); }
      BHandle getFontDenmark24() const { return (mFontDenmark24); }

   private:

      void _renderText(BHandle hFont, float x, float y, const BUString& str, DWORD dwColor = cDWORDWhite, BJustification j = BFontManager2::cJustifyLeft, float scale = 1.0f);
      void renderString(float x, float y, DWORD dwColor, const WCHAR* pStr, BJustification just, float scale);

      //-- Private Data
      BString                    mDefaultFont;
      BFont2*                    mpCurrentFont;   
      BPointerList<BFont2>       mFonts;
      BPointerList<BFontSource>  mFontSources;
      BDynamicArray<BHandle>     mSafeHandles;
      long                       mBaseDirectoryID;

      BHandle                    mFontArial10;
      BHandle                    mFontCourier10;
      BHandle                    mFontDenmark14;
      BHandle                    mFontDenmark16;
      BHandle                    mFontDenmark18;
      BHandle                    mFontDenmark24;

      BFreeList<BTextData>       mQueuedText;

      //-- Helpers
      void            parseParameter  (BXMLNode node, BFontInfo& fontInfo);
      void            parseOverride   (BXMLNode node, BFontInfo& fontInfo);

};

#endif




