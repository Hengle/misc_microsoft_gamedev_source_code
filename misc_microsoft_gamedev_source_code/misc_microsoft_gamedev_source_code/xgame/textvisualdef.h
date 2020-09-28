//==============================================================================
// textvisualdef.h
//
// Copyright (c) 2005-2007, Ensemble Studios
//==============================================================================
#pragma once

//==============================================================================
// Includes
#include "xmlreader.h"

//==============================================================================
// Forward declarations
class BTextVisual;
class BTextVisualEffect;


//==============================================================================
// Defines.


//==============================================================================
// class BTextVisualDefElement
//==============================================================================
class BTextVisualDefElement
{
   public:
                              BTextVisualDefElement();
                              ~BTextVisualDefElement();

      bool                    load(BXMLNode root);
      void                    initVisual(BTextVisual *vis);
   
   protected:
      BHandle                 mFont;
      DWORD                   mLifespan;      
      DWORD                   mFadeOutDuration;
      BDynamicSimArray<BTextVisualEffect*> mEffects;
};


//==============================================================================
// class BTextVisualDef
//==============================================================================
class BTextVisualDef
{
   public:
                              BTextVisualDef();
                              ~BTextVisualDef();
                              
      void                    setFilename(const BCHAR_T *filename) {mFilename = filename;}
      const BString           &getFilename(void) const {return(mFilename);}
      
      bool                    load();
      
      void                    create(long playerID, const BUString &text, const BVector &anchorPos, BDynamicSimLongArray *resultIDs=NULL);
                              
   protected:
      BString                 mFilename;
      BDynamicSimArray<BTextVisualDefElement *> mElements;      
};