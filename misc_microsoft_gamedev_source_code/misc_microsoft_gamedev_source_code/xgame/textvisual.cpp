//==============================================================================
// textvisual.cpp
//
// Copyright (c) 2005, Ensemble Studios
//==============================================================================
#include "common.h"
#include "textvisual.h"
#include "textvisualeffect.h"
#include "render.h"
#include "FontSystem2.h"
#include "viewportManager.h"

//==============================================================================
// BTextVisual::BTextVisual
//==============================================================================
BTextVisual::BTextVisual() :
   mWorldAnchor(cOriginVector),
   mOffsetX(0),
   mOffsetY(0),
   mColor(cColorWhite),
   mAlpha(1.0f),
   mElapsed(0),
   mLifespan(0xFFFFFFFF),
   mFontHandle(0),
   mIcon(NULL),
   mPlayerID(-1),
   mFadeOutDuration(2000)
{
}

//==============================================================================
// BTextVisual::~BTextVisual
//==============================================================================
BTextVisual::~BTextVisual()
{
   // Destroy any effects we own.
   for(long i=0; i<mEffects.getNumber(); i++)
   {
      // Get effect.
      BTextVisualEffect *effect = mEffects[i];
      if(!effect)
         continue;
         
      // If def is managing this, don't delete.
      if(effect->getManagedByDef())
         continue;
         
      // Nuke.
      delete effect;
   }
   mEffects.setNumber(0);

   //Cleanup Icon
   if(mIcon)
   {
      delete mIcon;
      mIcon = NULL;
   }
}

//==============================================================================
// BTextVisual::addEffect
//==============================================================================
void BTextVisual::addEffect(BTextVisualEffect *effect)
{
   // Sanity.
   if(!effect)
      return;
      
   // Add it.
   mEffects.add(effect);
   
   // Add corresponding parameter (some day may be a more complex instance class)
   mEffectParams.add(0.0f);
}


//==============================================================================
// BTextVisual::update
//==============================================================================
bool BTextVisual::update(DWORD elapsed)
{
   // Update elapsed.
   mElapsed += elapsed;
   
   // If we have a non-infinite lifespan, see if it has expired
   if(mLifespan != 0xFFFFFFFF)
   {
      if (mFadeOutDuration > 0)
      {
         DWORD totalLifeTime = mLifespan+mFadeOutDuration;
         //-- done fading kill it.         
         if (mElapsed > totalLifeTime)
            return false;

         if (mElapsed > mLifespan)
         {
            DWORD deltaT = Math::ClampLow((DWORD) (mElapsed - mLifespan), (DWORD) 0);
            float alpha = 1.0f - ((float) deltaT / (float) mFadeOutDuration);

            mAlpha = alpha;
         }         
      }
      else 
      {
         if (mElapsed > mLifespan)
            return false;
      }      
   }
   
   // Let the effects party on our info.
   for(long i=0; i<mEffects.getNumber(); i++)
   {
      // Get the effect.
      BTextVisualEffect *effect = mEffects[i];
      if(!effect)
         continue;
         
      // Update.
      effect->update(elapsed, this, mEffectParams[i]);
   }
   
   // Let us live.
   return(true);
}


//==============================================================================
// BTextVisual::render
//==============================================================================
void BTextVisual::render()
{
   // Convert anchor to screen point.
   long renderX = 0;
   long renderY = 0;

   int viewportIndex = gViewportManager.getUserViewportIndex(gViewportManager.getCurrentUser());
   if (viewportIndex < 0)
      return;
   
   const BRenderDraw::BViewportDesc& vpDesc = gRenderDraw.getViewportDesc(viewportIndex);
   XMMATRIX worldToScreen = vpDesc.mMatrixTracker.getMatrix(eMTMatrix::cMTWorldToScreen, false);
   XMVECTOR screenPosition = XMVector3TransformCoord(mWorldAnchor, worldToScreen);
   
   const D3DVIEWPORT9& viewport = gViewportManager.getSceneViewport(viewportIndex);
   if (screenPosition.z < viewport.MinZ || screenPosition.z > viewport.MaxZ)
      return;   

   if (screenPosition.x < 0 || screenPosition.x > viewport.Width)
      return;

   if (screenPosition.y < 0 || screenPosition.y > viewport.Height)
      return;

   renderX = (int) screenPosition.x;
   renderY = (int) screenPosition.y;
            
   // Get the extents of this string.
   BVector extents = gFontManager.stringExtents(mFontHandle, mText);   
   
   // Offset render position by half our X extents.
   renderX -= long(0.5f*extents.x);

   // Draw.
   gFontManager.drawText(mFontHandle, float(renderX+mOffsetX), float(renderY+mOffsetY), mText, mColor.asDWORD());
   
   //Draw Icon.   
   if(mIcon)
   {
      long iconOffset = mIcon->getWidth() + BTextVisualEffectIcon::cIconSpacing;
      XMCOLOR color;
      XMStoreColor(&color, XMVectorSet(1.0f, 1.0f, 1.0f, mAlpha));
      mIcon->setColor(color.c);
      mIcon->setPosition(renderX+mOffsetX - iconOffset, renderY+mOffsetY);      
      mIcon->setFlag(BUIElement::cFlagHidden, false);
      mIcon->render(0,0);  
      mIcon->setFlag(BUIElement::cFlagHidden, true);

   }   
}

//==============================================================================
// BTextVisual::createIcon
//==============================================================================
BUIElement* BTextVisual::createIcon(void)
{
   if(mIcon == NULL)
      mIcon = new BUIElement();

   return mIcon;
}

//==============================================================================
// eof: textvisual.cpp
//==============================================================================
