//============================================================================
// UIHints.cpp
// Ensemble Studios (c) 2007
//============================================================================

#include "common.h"
#include "UIHints.h"

//-- render
#include "render.h"
#include "renderDraw.h"
#include "renderThread.h"

#include "camera.h"
#include "uigame.h"
#include "visualmanager.h"
#include "configsgame.h"
#include "soundmanager.h"

#include "hintmanager.h"

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BUIHints::BUIHints():
mpMovie(NULL),
mHintPanelVisible(true)
{
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BUIHints::~BUIHints()
{
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BUIHints::init(const char* filename, const char* datafile)
{
   gFlashGateway.getOrCreateData(filename, cFlashAssetCategoryInGame, mDataHandle);
   mpMovie = gFlashGateway.createInstance(mDataHandle, false);
   gFlashGateway.registerEventHandler(mpMovie, mSimEventHandle);

   setFlag(cFlagInitialized, true);
   return true;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BUIHints::deinit()
{
   if (mpMovie)
   {
      gFlashGateway.unregisterEventHandler(mpMovie, mSimEventHandle);
      gFlashGateway.releaseInstance(mpMovie);
   }
   mpMovie = NULL;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BUIHints::enter()
{
   if (!getFlag(cFlagInitialized))
      return;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BUIHints::leave()
{
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BUIHints::update(float elapsedTime)
{   
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BUIHints::renderBegin()
{   
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BUIHints::render()
{   
   SCOPEDSAMPLEID(FlashUIGame, 0xFFFF0000);
   if (mHintPanelVisible && mpMovie)
      mpMovie->render();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BUIHints::renderEnd()
{
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BUIHints::handleInput(int port, BInputEventType event, BInputControlType controlType, BInputEventDetail& detail)
{
   return false;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BUIHints::receiveEvent(const BEvent& event, BThreadIndex threadIndex)
{
   return false;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BManagedTextureHandle BUIHints::getRenderTargetTexture()
{
   if (mpMovie)
      return mpMovie->mRenderTargetHandle;

   return cInvalidManagedTextureHandle;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BUIHints::setDimension(int x, int y, int width, int height)
{
   if (!mpMovie)
      return;

   mpMovie->setDimension(x, y, width, height);
}

//==============================================================================
// BUIHints::displayHint
//==============================================================================
void BUIHints::displayHint(BHintMessage* hintMessage)
{
   // display the hint
   GFxValue values[1];
   values[0].SetStringW(hintMessage->getHintString());
   mpMovie->invokeActionScript("displayHint", values, 1);

   gSoundManager.playCue("ui_hint_channel");
}


//==============================================================================
// BUIHints::hideHint
//==============================================================================
void BUIHints::hideHint()
{
   GFxValue values[1];
   values[0].SetBoolean(false);
   mpMovie->invokeActionScript("setHintVisible", values, 1);
}
