//==============================================================================
// ui.cpp
//
// Copyright (c) 2006 Ensemble Studios
//==============================================================================

// Includes
#include "common.h"
#include "ui.h"
#include "config.h"
#include "configsgame.h"
#include "database.h"
#include "debugprimitives.h"
#include "primDraw2D.h"
#include "render.h"
#include "renderDraw.h"
#include "renderViewParams.h"
#include "soundmanager.h"
#include "user.h"
#include "usermanager.h"
#include "xmlreader.h"
#include "camera.h"
#include "game.h"
#include "selectionmanager.h"
#include "flashminimaprenderer.h"
#include "uimanager.h"
#include "world.h"

// Global
BUI gUI;

//==============================================================================
// BUI::init
//==============================================================================
void BUI::init()
{
   // rg [1/8/08] - Moving these init()'s here because they async. load effects, which need the D3D device to be created. We can't load during BModeGame::init() 
   // because the D3D device is owned by the flash background player.
   gFlashMinimapRenderer.init();
            
   long lvw=gRender.getViewParams().getViewportWidth();
   long lvh=gRender.getViewParams().getViewportHeight();

   float fvw=(float)lvw;
   float fvh=(float)lvh;

   mfSafeX1=fvw*(1.0f-cTitleSafeArea)*0.5f;
   mfSafeY1=fvh*(1.0f-cTitleSafeArea)*0.5f;
   mfSafeX2=fvw-mfSafeX1;
   mfSafeY2=fvh-mfSafeY1;
   mfSafeWidth=fvw*cTitleSafeArea;
   mfSafeHeight=fvh*cTitleSafeArea;
   mfCenterX=fvw*0.5f;
   mfCenterY=fvh*0.5f;
   mfWidth=fvw;
   mfHeight=fvh;

   mlSafeX1=(long)mfSafeX1;
   mlSafeY1=(long)mfSafeY1;
   mlSafeX2=(long)mfSafeX2;
   mlSafeY2=(long)mfSafeY2;
   mlSafeWidth=(long)mfSafeWidth;
   mlSafeHeight=(long)mfSafeHeight;
   mlCenterX=(long)mfCenterX;
   mlCenterY=(long)mfCenterY;
   mlWidth=lvw;
   mlHeight=lvh;
};

//==============================================================================
// BUI::deinit
//==============================================================================
void BUI::deinit()
{
   gFlashMinimapRenderer.deInit();
   
   long count=mTextureList.getNumber();
   for(long i=0; i<count; i++)
      gD3DTextureManager.releaseManagedTextureByHandle(mTextureList[i].mTextureHandle);
   mTextureList.clear();
}

//==============================================================================
// BUI::update
//==============================================================================
void BUI::update()
{
   long count=mUnloadList.getNumber();
   for(long i=count-1; i>=0; i--)
   {
      // Unload the texture after two updates
      mUnloadList[i].mRefCount--;
      if(mUnloadList[i].mRefCount<-2)
      {
         gD3DTextureManager.releaseManagedTextureByHandle(mUnloadList[i].mTextureHandle);
         mUnloadList.removeIndex(i, false);
      }
   }
}

//==============================================================================
// BUI::loadTexture
//==============================================================================
BManagedTextureHandle BUI::loadTexture(const char* pFileName)
{
   if (gConfig.isDefined(cConfigDisableOldUITextures))
      return cInvalidManagedTextureHandle;
   
   //(pFileName, cTRTStatic, cInvalidEventReceiverHandle, cDefaultTextureTransparent, cThreadIndexRender, (eTextureFlags)(cTFDiscard | cTFDoNotAddUnusedRegionsToHeap));
   BManagedTextureHandle textureHandle = gD3DTextureManager.getOrCreateHandle(pFileName, BFILE_OPEN_DISCARD_ON_CLOSE, BD3DTextureManager::cUI, false, cDefaultTextureTransparent, false, false, "BUI", false);
   if(textureHandle!=cInvalidManagedTextureHandle)
   {
      //FIXME - Temp code for ref-counting ui textures
      long count=mTextureList.getNumber();
      for(long i=0; i<count; i++)
      {
         if(mTextureList[i].mTextureHandle==textureHandle)
         {
            mTextureList[i].mRefCount++;
            return textureHandle;
         }
      }
      count=mUnloadList.getNumber();
      for(long i=0; i<count; i++)
      {
         if(mUnloadList[i].mTextureHandle==textureHandle)
         {
            mUnloadList[i].mRefCount=1;
            mTextureList.add(mUnloadList[i]);
            mUnloadList.removeIndex(i, false);
            return textureHandle;
         }
      }
      BUITexture uiTexture;
      uiTexture.mTextureHandle=textureHandle;
      uiTexture.mRefCount=1;
      mTextureList.add(uiTexture);

      // Load the texture
      gD3DTextureManager.loadManagedTextureByHandle(textureHandle);
   }
   return textureHandle;
}

//==============================================================================
// BUI::unloadTexture
//==============================================================================
void BUI::unloadTexture(BManagedTextureHandle textureHandle)
{
   if(textureHandle!=cInvalidManagedTextureHandle)
   {
      //FIXME - Temp code for ref-counting ui textures
      long count=mTextureList.getNumber();
      for(long i=0; i<count; i++)
      {
         if(mTextureList[i].mTextureHandle==textureHandle)
         {
            mTextureList[i].mRefCount--;
            if(mTextureList[i].mRefCount>0)
               return;
            mUnloadList.add(mTextureList[i]);
            mTextureList.removeIndex(i, false);
            break;
         }
      }
   }
}

//==============================================================================
// BUI::forceUnloadAllTextures()
//==============================================================================
void BUI::forceUnloadAllTextures()
{
   long count=mUnloadList.getNumber();
   for(long i=0; i<count; i++)
   {
      if (mUnloadList[i].mTextureHandle != cInvalidManagedTextureHandle)
         gD3DTextureManager.releaseManagedTextureByHandle(mUnloadList[i].mTextureHandle);

      mUnloadList[i].mRefCount = 0;
      mUnloadList[i].mTextureHandle = cInvalidManagedTextureHandle;
   }
   mUnloadList.clear();

   for (long j=0; j<mTextureList.getNumber(); j++)
   {
      if (mTextureList[j].mTextureHandle != cInvalidManagedTextureHandle)
         gD3DTextureManager.releaseManagedTextureByHandle(mTextureList[j].mTextureHandle);
      mTextureList[j].mRefCount = 0;
      mTextureList[j].mTextureHandle = cInvalidManagedTextureHandle;
   }
   mTextureList.clear();
}

//==============================================================================
// class BRenderTextureRCO
//==============================================================================
namespace
{
   class BRenderTextureRCO : BRenderCommandObjectInterface
   {
   public:
      BRenderTextureRCO(bool useAlpha) :
         mUseAlpha(useAlpha)
      {
      }
      
      virtual void processCommand(DWORD data) const
      {
         if (data == 0)
         {
            // begin
            BD3D::mpDev->SetRenderState(D3DRS_ZENABLE, FALSE);

            BD3D::mpDev->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
            BD3D::mpDev->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
            BD3D::mpDev->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
            BD3D::mpDev->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
            BD3D::mpDev->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_POINT);

            if(mUseAlpha)
            {
               BD3D::mpDev->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
               BD3D::mpDev->SetRenderState(D3DRS_SRCBLEND,         D3DBLEND_SRCALPHA);
               BD3D::mpDev->SetRenderState(D3DRS_DESTBLEND,        D3DBLEND_INVSRCALPHA);
               BD3D::mpDev->SetRenderState(D3DRS_BLENDOP,          D3DBLENDOP_ADD);

               BD3D::mpDev->SetRenderState(D3DRS_ALPHATESTENABLE, TRUE);
               BD3D::mpDev->SetRenderState(D3DRS_ALPHAREF, 0);
               BD3D::mpDev->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_GREATER);
            }
            else
            {
               BD3D::mpDev->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
               BD3D::mpDev->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
            }
         }
         else
         {
            // end
            if(mUseAlpha)
            {
               BD3D::mpDev->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
               BD3D::mpDev->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
            }  
            
            BD3D::mpDev->SetTexture(0, NULL);
         }            
      }
   private:
      bool mUseAlpha;
   };
}

//==============================================================================
// BUI::renderTexture
//==============================================================================
void BUI::renderTexture(BManagedTextureHandle textureHandle, long x1, long y1, long x2, long y2, bool useAlpha, DWORD color, bool bUseOverBright)
{
   renderTexture(textureHandle, x1, y1, x2, y2, 0.0f, 0.0f, 1.0f, 1.0f, useAlpha, color, bUseOverBright);
}

//==============================================================================
// BUI::renderTexture
//==============================================================================
void BUI::renderTexture(BManagedTextureHandle textureHandle, long x1, long y1, long x2, long y2, float u0, float v0, float u1, float v1, bool useAlpha, DWORD color, bool bUseOverBright)
{
   if(textureHandle!=cInvalidManagedTextureHandle)
   {
      //gRenderDraw.setTextureByHandle(0, textureHandle, cDefaultTextureTransparent);
      gD3DTextureManager.setManagedTextureByHandle(textureHandle, 0);

      BRenderTextureRCO rco(useAlpha);
      gRenderThread.submitCopyOfCommandObject(rco, FALSE);

      eFixedFuncShaderIndex psIndex = cDiffuseTex1PS;
      if (bUseOverBright)
         psIndex = cDiffuseTex1PSOverBright;

      // rg [6/16/06] - Adjust lower right coords to match what the hardware expects for proper screenspace quad rendering.
      BPrimDraw2D::drawSolidRect2D(x1, y1, x2 + 1, y2 + 1, u0, v0, u1, v1, color, 0xFFFFFFFF, cPosDiffuseTex1VS, psIndex); //cPosTex1VS, cTex1PS);

      gRenderThread.submitCopyOfCommandObject(rco, TRUE);
   }
}

//==============================================================================
// BUI::loadSize
//==============================================================================
bool BUI::loadSize(BXMLNode node, long*  pX, long*  pY)
{
   BSimString tempStr;
   if(sscanf_s(node.getTextPtr(tempStr), "%d %d", pX, pY) < 2)
      return false;
   return true;
}

//==============================================================================
// BUI::loadPosition
//==============================================================================
bool BUI::loadPosition(BXMLNode node, BUIRect*  pRect)
{
   BSimString tempStr;
   if(sscanf_s(node.getTextPtr(tempStr), "%d %d %d %d", &pRect->mX1, &pRect->mY1, &pRect->mX2, &pRect->mY2) < 4)
      return false;

   pRect->mX1+=(pRect->mX1<0.0f ? mlSafeX2 : mlSafeX1);
   pRect->mY1+=(pRect->mY1<0.0f ? mlSafeY2 : mlSafeY1);
   pRect->mX2+=(pRect->mX2<0.0f ? mlSafeX2 : mlSafeX1);
   pRect->mY2+=(pRect->mY2<0.0f ? mlSafeY2 : mlSafeY1);

   return true;
}

//==============================================================================
// BUI::playRolloverSound
//==============================================================================
void BUI::playRolloverSound()
{
   gSoundManager.playCue("play_ui_game_menu_scroll");
   playRumbleEvent(BRumbleEvent::cTypeUIRollover);
}

//==============================================================================
// BUI::playClickSound
//==============================================================================
void BUI::playClickSound()
{
   gSoundManager.playCue("play_ui_game_menu_select");
   playRumbleEvent(BRumbleEvent::cTypeConfirmCommand);
}

//==============================================================================
// BUI::playCantDoSound
//==============================================================================
void BUI::playCantDoSound()
{
   gSoundManager.playCue("play_ui_cant_do_that");
   playRumbleEvent(BRumbleEvent::cTypeCantDoCommand);
}

//==============================================================================
// BUI::playPowerNotEnoughResourcesSound
//==============================================================================
void BUI::playPowerNotEnoughResourcesSound()
{
   gSoundManager.playCue("play_vog_leaderpower_resource");
   playRumbleEvent(BRumbleEvent::cTypeCantDoCommand);
}

//============================================================================
//============================================================================
void BUI::playConfirmSound()
{
   gSoundManager.playCue("play_ui_game_menu_select");
}

//============================================================================
//============================================================================
void BUI::playCancelSound()
{
   gSoundManager.playCue("play_ui_menu_back_button");
}

//==============================================================================
//==============================================================================
uint BUI::wordWrapText(const BUString& text, uint maxLines, BSmallDynamicSimArray<BUString>& lines)
{
   // Break the text into multiple lines based on \n characters
   long len=text.length();
   long startIndex=0;
   long searchIndex=0;
   BUString s;
   for(;;)
   {
      long endIndex=text.findLeft(L'\\', searchIndex);
      if(endIndex==-1)
         break;
      if(endIndex>=len-1)
         break;
      if(text.getPtr()[endIndex+1]!=L'n')
      {
         searchIndex+=2;
         if(searchIndex>=len-1)
            break;
         continue;
      }
      if(endIndex==startIndex)
         s.empty();
      else
      {
         s=text;
         s.crop(startIndex, endIndex-1);
      }
      lines.add(s);
      startIndex=endIndex+2;
      if(startIndex>=len-1)
         break;
      searchIndex=startIndex;
      if(lines.size()==maxLines)
         break;
   }
   if(lines.size()<maxLines)
   {
      if(startIndex==0)
         lines.add(text);
      else
      {
         long endIndex=len-1;
         if(startIndex<endIndex)
         {
            s=text;
            s.crop(startIndex, endIndex);
            lines.add(s);
         }
      }
   }
   return lines.size();
}

//==============================================================================
//==============================================================================
int BUI::playRumbleEvent(int eventType, bool loop)
{
   const BRumbleEvent* pRumbleEvent = gDatabase.getRumbleEvent(eventType);
   if (pRumbleEvent)
   {
      BGamepad& gamepad = gInputSystem.getGamepad(gUserManager.getPrimaryUser()->getPort());
      if (!pRumbleEvent->mMultiple)
      {
         int rumbleID = gDatabase.getRumbleEventID(eventType);
         if (rumbleID != -1)
            gamepad.stopRumble(rumbleID);
      }
      int rumbleID = -1;
      if (pRumbleEvent->mPattern != -1)
         rumbleID = gamepad.playRumblePattern(pRumbleEvent->mPattern, loop);
      else
         rumbleID = gamepad.playRumble(pRumbleEvent->mLeftRumbleType, pRumbleEvent->mLeftStrength, pRumbleEvent->mRightRumbleType, pRumbleEvent->mRightStrength, pRumbleEvent->mDuration, loop);
      gDatabase.setRumbleEventID(eventType, rumbleID);
      return rumbleID;
   }
   else
      return -1;
}

//==============================================================================
//==============================================================================
int BUI::playRumbleEvent(int eventType, int leftRumbleType, float leftStrength, int rightRumbleType, float rightStrength, float duration, bool loop)
{
   const BRumbleEvent* pRumbleEvent = gDatabase.getRumbleEvent(eventType);
   if (pRumbleEvent)
   {
      BGamepad& gamepad = gInputSystem.getGamepad(gUserManager.getPrimaryUser()->getPort());
      if (!pRumbleEvent->mMultiple)
      {
         int rumbleID = gDatabase.getRumbleEventID(eventType);
         if (rumbleID != -1)
            gamepad.stopRumble(rumbleID);
      }
      int rumbleID = gamepad.playRumble(leftRumbleType, leftStrength, rightRumbleType, rightStrength, duration, loop);
      gDatabase.setRumbleEventID(eventType, rumbleID);
      return rumbleID;
   }
   else
      return -1;
}

//==============================================================================
//==============================================================================
int BUI::playRumbleEvent(int eventType, const BRumbleEvent* pRumbleData, bool loop)
{
   const BRumbleEvent* pRumbleEvent = gDatabase.getRumbleEvent(eventType);
   if (pRumbleEvent)
   {
      BGamepad& gamepad = gInputSystem.getGamepad(gUserManager.getPrimaryUser()->getPort());
      if (!pRumbleEvent->mMultiple)
      {
         int rumbleID = gDatabase.getRumbleEventID(eventType);
         if (rumbleID != -1)
            gamepad.stopRumble(rumbleID);
      }
      int rumbleID = -1;
      if (pRumbleData->mPattern != -1)
         rumbleID = gamepad.playRumblePattern(pRumbleData->mPattern, loop);
      else
         rumbleID = gamepad.playRumble(pRumbleData->mLeftRumbleType, pRumbleData->mLeftStrength, pRumbleData->mRightRumbleType, pRumbleData->mRightStrength, pRumbleData->mDuration, loop);
      gDatabase.setRumbleEventID(eventType, rumbleID);
      return rumbleID;
   }
   else
      return -1;
}

//==============================================================================
//==============================================================================
void BUI::stopRumble(int rumbleID)
{
   BGamepad& gamepad = gInputSystem.getGamepad(gUserManager.getPrimaryUser()->getPort());
   gamepad.stopRumble(rumbleID);
}

//==============================================================================
//==============================================================================
void BUI::doCameraShake(BVector location, float strength, float duration, bool onlyIfSelected, BEntityID unitID)
{
   if (duration > 0.0f && strength > 0.0f)
   {
      if (!onlyIfSelected || gUserManager.getPrimaryUser()->getSelectionManager()->isUnitSelected(unitID))
      {
         if (gUserManager.getPrimaryUser()->isSphereVisible(location, 1.0f))
         {
            BCamera* pCamera = gUserManager.getPrimaryUser()->getCamera();
            pCamera->clearShake();
            pCamera->beginShake(duration, strength);
         }
      }
      if (gGame.isSplitScreen())
      {
         if (!onlyIfSelected || gUserManager.getSecondaryUser()->getSelectionManager()->isUnitSelected(unitID))
         {
            if (gUserManager.getSecondaryUser()->isSphereVisible(location, 1.0f))
            {
               BCamera* pCamera = gUserManager.getSecondaryUser()->getCamera();
               pCamera->clearShake();
               pCamera->beginShake(duration, strength);
            }
         }
      }
   }
}

//==============================================================================
//==============================================================================
void BUI::doCameraEffect(BVector location, BCameraEffectData* pCamEffect)
{
   if (!pCamEffect)
      return;

   /*
   // Don't add any effects if the circle menu effect is up
   // MPB TODO - We may want a stack of cam effects so that we can add effects
   // that would be playing once the circle menu goes away
   if (gUIManager->getCircleMenuVisible())
      return;
   */

//-- FIXING PREFIX BUG ID 2120
   const BUIContext* pPrimaryUserUIContext = gUserManager.getPrimaryUser()->getUIContext();
//--
   if (gUserManager.getPrimaryUser()->isSphereVisible(location, 1.0f) && !pPrimaryUserUIContext->getCircleMenuVisible())
   {
      BCamera* pCamera = gUserManager.getPrimaryUser()->getCamera();
      
      pCamera->beginCameraEffect(gWorld->getSubGametime(), pCamEffect, &location, 0);
   }

   if (gGame.isSplitScreen())
   {
//-- FIXING PREFIX BUG ID 2119
      const BUIContext* pSecondaryUserUIContext = gUserManager.getSecondaryUser()->getUIContext();
//--
      if (gUserManager.getSecondaryUser()->isSphereVisible(location, 1.0f) && !pSecondaryUserUIContext->getCircleMenuVisible())
      {
         BCamera* pCamera = gUserManager.getSecondaryUser()->getCamera();
         pCamera->beginCameraEffect(gWorld->getSubGametime(), pCamEffect, &location, 1);
      }
   }
}
