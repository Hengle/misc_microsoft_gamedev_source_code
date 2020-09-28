//==============================================================================
// textvisual.h
//
// Copyright (c) 2005-2007, Ensemble Studios
//==============================================================================
#pragma once

//==============================================================================
// Includes
#include "uielement.h"

//==============================================================================
// Forward declarations
class BTextVisualEffect;

//==============================================================================
// Defines.


//==============================================================================
// class BTextVisual
//==============================================================================
class BTextVisual
{
   public:      

                              BTextVisual();
                              ~BTextVisual();
                              
      bool                    update(DWORD elapsed);
      void                    render();
      
      DWORD                   getElapsed(void) const {return(mElapsed);}
      void                    setElapsed(DWORD elapsed) {mElapsed = elapsed;}
      
      const BVector           &getWorldAnchor(void) const {return(mWorldAnchor);}
      void                    setWorldAnchor(const BVector &v) {mWorldAnchor = v;}

      const BUString          &getText(void) const {return(mText);}
      void                    setText(const WCHAR *text) {mText = text;}
      
      void                    addEffect(BTextVisualEffect *effect);
      
      void                    setFont(BHandle font) {mFontHandle = font;}
      BHandle                 getFont(void) const {return(mFontHandle);}
      
      DWORD                   getLifespan(void) const {return(mLifespan);}
      void                    setLifespan(DWORD lifespan) {mLifespan = lifespan;}

      DWORD                   getFadeoutDuration(void) const {return(mFadeOutDuration);}
      void                    setFadeoutDuration(DWORD value) {mFadeOutDuration = value;}

      long                    getOffsetX(void) const {return(mOffsetX);}
      void                    setOffsetX(long x) {mOffsetX = x;}
      long                    getOffsetY(void) const {return(mOffsetY);}
      void                    setOffsetY(long y) {mOffsetY = y;}

      const BColor            &getColor(void) const {return(mColor);}
      void                    setColor(const BColor &color) {mColor = color;}
      float                   getAlpha(void) const {return(mAlpha);}
      void                    setAlpha(float alpha) {mAlpha = alpha;}

      void                    setPlayerID(long playerID) { mPlayerID = playerID; }
      long                    getPlayerID() const { return mPlayerID; }
      
      BUIElement*             getIcon(void) {return mIcon;}
      BUIElement*             createIcon(void);                    

   protected:
      BUString                mText;
      BVector                 mWorldAnchor;
      DWORD                   mElapsed;
      DWORD                   mLifespan;
      DWORD                   mFadeOutDuration;
      BHandle                 mFontHandle;
      BUIElement*             mIcon;
      
      // Info for rendering.
      long                    mOffsetX;
      long                    mOffsetY;
      BColor                  mColor;
      float                   mAlpha;
      long                    mPlayerID;
      
      // Effects.
      BDynamicSimArray<BTextVisualEffect*> mEffects;
      BDynamicSimArray<float>     mEffectParams;
};