//============================================================================
// font2_xbox.h
//
// Copyright (c) 2005 Ensemble Studios
//
// This is a modified version of font2.h. The public interface is basically
// the same to keep compatibility. 
//============================================================================

#ifndef _FONT2_XBOX_H_
#define _FONT2_XBOX_H_

// Includes
#include "atgfont.h"

//----------------------------------------------------------------------------
//  Externs
//----------------------------------------------------------------------------
class BFontManager2;

//----------------------------------------------------------------------------
//  Class BFont2
//----------------------------------------------------------------------------
class BFont2
{
public:
   //-- Construction/Destruction
   BFont2();
   ~BFont2();

   //-- Interface
   void            reset               ();
   bool            create              (const BFontInfo& fontInfo, BFontManager2* pManager);
   bool            getFontInfo         (BFontInfo& fontInfo);
   void            begin               ();
   void            drawText            (float x, float y, DWORD color, const WCHAR* text, long justification=0, float scale = 1.0f );
   void            drawText            (float x, float y, DWORD color, const char* text, long justification=0, float scale = 1.0f );
   void            end                 ();
   void            destroy             ();

   //-- Name Interface
   void            addName    (const BString& name);
   void            removeName (const BString& name);
   bool            hasName    (const BString& name);
   long            getNumNames();
   bool            getName    (long index, BString& name);

   //-- Inlines
   inline long     getAscentHeight          ()                { return mFontInfo.mPixelHeight;       }
   inline float    getSpacingAdjustment     ()                { return mFontInfo.mSpacingAdjustment; }
   inline long     getHorizontalOffset      ()                { return mFontInfo.mHorizontalOffset;  }
   inline long     getVerticalOffset        ()                { return mFontInfo.mVerticalOffset;    }
   inline float    getScale                 ()                { return mScale; }

   float           getHeightFloat           ();
   long            getHeight                ();
   float           getTextWidth             (const WCHAR* pChars) const;
   float           getTextWidth             (const char* pChars) const;

   //-- Operators
   bool operator == (const BFont2&    font) const;
   bool operator == (const BFontInfo& fontInfo) const;

private:
   //-- Private Data
   BFontSource*        mFontSource;
   BCopyList<BString>  mNames;
   BFontManager2*      mpManager;
   BFontInfo           mFontInfo;
   float               mScale;

};

#endif
