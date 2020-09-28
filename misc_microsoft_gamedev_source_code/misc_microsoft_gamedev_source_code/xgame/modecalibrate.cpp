//==============================================================================
// modecalibrate.cpp
//
// Copyright (c) 2005 Ensemble Studios
//==============================================================================

// Includes
#include "common.h"
#include "modecalibrate.h"
#include "modemenu.h"
#include "database.h"
#include "fontSystem2.h"
#include "game.h"
#include "gamedirectories.h"
#include "gamesettings.h"
#include "modemanager.h"
#include "render.h"
#include "ui.h"
#include "configsgame.h"
#include "workdirsetup.h"
#include "world.h"

// xgameRender
#include "gammaRamp.h"

//==============================================================================
// BModeCalibrate::BModeCalibrate
//==============================================================================
BModeCalibrate::BModeCalibrate(long modeType) :
   BMode(modeType),
   mState(-1),
   mNextState(cStateMain),
   mBackgroundHandle(cInvalidManagedTextureHandle)
{
}

//==============================================================================
// BModeCalibrate::~BModeCalibrate
//==============================================================================
BModeCalibrate::~BModeCalibrate()
{
}

//==============================================================================
// BModeCalibrate::setup
//==============================================================================
bool BModeCalibrate::setup()
{
   return BMode::setup();
}

//==============================================================================
// BModeCalibrate::shutdown
//==============================================================================
void BModeCalibrate::shutdown()
{
   BMode::shutdown();
}

//==============================================================================
// BModeCalibrate::preEnter
//==============================================================================
void BModeCalibrate::preEnter(BMode* lastMode)
{
   return BMode::preEnter(lastMode);
}

//==============================================================================
// BModeCalibrate::enter
//==============================================================================
void BModeCalibrate::enter(BMode* lastMode)
{
   mBackgroundHandle=gUI.loadTexture("ui\\menu\\background\\gammaTest");
   return BMode::enter(lastMode);
}

//==============================================================================
// BModeCalibrate::leave
//==============================================================================
void BModeCalibrate::leave(BMode* newMode)
{
   gUI.unloadTexture(mBackgroundHandle);
   mBackgroundHandle=cInvalidManagedTextureHandle;
   return BMode::leave(newMode);
}

//==============================================================================
// BModeCalibrate::update
//==============================================================================
void BModeCalibrate::update()
{
   BMode::update();

   if(mNextState!=-1)
   {
      mState=mNextState;
      mNextState=-1;
            
      BHandle fontHandle;
      fontHandle=gFontManager.getFontDenmark24();
      gFontManager.setFont(fontHandle);

      float yh=gFontManager.getLineHeight();

      mList.setFont(fontHandle);
      mList.setPosition(gUI.mlSafeX1, gUI.mlSafeY1);
      mList.setRowSpacing(yh);
      mList.setColors(cColorWhite, cColorCyan);
      mList.setColumnWidth(gUI.mfSafeWidth/3.0f);
      mList.setNumberItemsPerColumn(((long)(gUI.mlSafeHeight/yh))-3);
      mList.setMultiColumn(true);
      mList.setJustifyCenter(false);

      mList.clearItems();
      mList.addItem("Change Gamma");
      mList.addItem("Change Contrast");
      mList.addItem("Reset");
      mList.addItem("Save");
      mList.addItem("Exit");
   }
   else
   {
      switch(mState)
      {
         case cStateMain : 
         {
            mList.update();
            break;
         }
         case cStateExit: 
         {
            mNextState=cStateMain;
            //gModeManager.getModeMenu()->setNextState(BModeMenu::cStateOther);
            gModeManager.getModeMenu()->setNextState(BModeMenu::cStateMain);
            gModeManager.getModeMenu()->setMenuItemFocus(BModeMenu::cStateGotoCalibrate);
            gModeManager.setMode(BModeManager::cModeMenu);
            break;
         }
      }
   }
}

//==============================================================================
// BModeCalibrate::renderBegin
//==============================================================================
void BModeCalibrate::renderBegin()
{
   gRender.beginFrame(1.0f/30.0f);
   gRender.beginViewport(-1);
   
   gRenderDraw.beginScene();
   gRenderDraw.clear(0, NULL, D3DCLEAR_TARGET|D3DCLEAR_STENCIL|D3DCLEAR_ZBUFFER, D3DCOLOR_ARGB(0,0,0,0), 1.0f, 0);
}

//==============================================================================
// BModeCalibrate::render
//==============================================================================
void BModeCalibrate::render()
{
   // renderBegin called before this

   BHandle fontHandle;
   fontHandle=gFontManager.getFontDenmark24();
   gFontManager.setFont(fontHandle);

   int width = 1024;
   int height = 1024;
   int x = (gUI.mlWidth - width) / 2;
   int y = (gUI.mlHeight - height) / 2;
   gUI.renderTexture(mBackgroundHandle, x, y, x + width - 1, y + height - 1, false);
   
   float sx=gUI.mfSafeX1;
   float yh=gFontManager.getLineHeight();
   float by=gUI.mfSafeY2-yh;

   BHandle fontHandle2=gFontManager.getFontDenmark24();
   gFontManager.setFont(fontHandle2);

   mList.render(0, 0);
   
   BSimString buf;
   buf.format("Gamma: %2.3f Contrast: %2.3f", gGammaRamp.getGamma(), gGammaRamp.getContrast());
   gFontManager.drawText(fontHandle, sx, by-yh, buf, cDWORDGreen);
   gFontManager.drawText(fontHandle, sx, by, "A - Select, DPad Left - Decrease, DPad Right - Increase", cDWORDGreen);

   gFontManager.render2D();

   // renderEnd called after this
}

//==============================================================================
// BModeCalibrate::renderEnd
//==============================================================================
void BModeCalibrate::renderEnd(void)
{
   gRenderDraw.endScene();
   gRenderDraw.present(NULL, NULL);
   
   gRender.endViewport();
   gRender.endFrame();
   gEventDispatcher.sleep(16);
}


//==============================================================================
// BModeCalibrate::saveGamma
//==============================================================================
void BModeCalibrate::saveGamma(void)
{
   gGammaRamp.save(cDirStartup, "gammaRamp.xml");
}

//==============================================================================
// BModeCalibrate::handleInput
//==============================================================================
bool BModeCalibrate::handleInput(long port, long event, long controlType, BInputEventDetail& detail)
{
   port; detail;
   
   if(event==cInputEventControlRepeat)
   {
      switch(controlType)
      {
         case cDpadLeft:
         case cDpadRight:
         {
            if (mList.getCurrentItem() < 2)
            {
               double dir = (controlType == cDpadLeft) ? -1.0f : 1.0f;

               double gamma = gGammaRamp.getGamma();
               double contrast = gGammaRamp.getContrast();

               if (0 == mList.getCurrentItem())
                  gamma += dir * 1.0f/250.0f;
               else
                  contrast += dir * 1.0f/250.0f;

               gamma = Math::Clamp<double>(gamma, .01f, 5.0f);
               contrast = Math::Clamp<double>(contrast, .01f, 5.0f);

               gGammaRamp.set((float)gamma, (float)contrast);
               
               return true;
            }               

            break;
         }
      }         
   }      
   else if(event==cInputEventControlStart)
   {
      switch(controlType)
      {
         case cButtonA:
         {
            gUI.playClickSound();
            
            if(mState==cStateMain)
            {
               switch(mList.getCurrentItem())
               {
                  //case 0 : mNextState=cStateScenario; return true;
                  //case 1 : mNextState=cStateGotoMultiplayer; return true;
                  case 2 : gGammaRamp.set(1.0f, 1.0f); return true;
                  case 3 : saveGamma(); return true;
                  case 4 : mNextState=cStateExit; return true;
               }
            }
            
            return true;
         }
         case cButtonB:
         {
            gUI.playClickSound();
            mNextState=cStateExit;
            return true;
         }
      }
   }
      
   if(mList.handleInput(port, event, controlType, detail))
      return true;

   return false;
}
