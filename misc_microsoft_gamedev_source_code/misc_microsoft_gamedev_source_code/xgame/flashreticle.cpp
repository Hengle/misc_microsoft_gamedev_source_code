//============================================================================
// flashreticle.cpp
// Ensemble Studios (c) 2006
//============================================================================

#include "common.h"
#include "flashreticle.h"

//-- render
#include "render.h"
#include "renderDraw.h"
#include "renderThread.h"

//-- game classes
#include "user.h"
#include "usermanager.h"
#include "player.h"
#include "uigame.h"
#include "visualmanager.h"


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BFlashReticle::BFlashReticle():
mpMovie(NULL),
mMode(-1),
mGoodAgainstRating(0),
mbVisible(true)
{
   for (int i = 0; i < cButtonCount; i++)
      mHelpButtonState[i].clear();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BFlashReticle::~BFlashReticle()
{
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BFlashReticle::init(const char* filename, const char* datafile)
{
   if (!filename)
      return false;

   if (!datafile)
      return false;

   if (!loadData(datafile))
      return false;

   gFlashGateway.getOrCreateData(filename, cFlashAssetCategoryInGame, mDataHandle);
   mpMovie = gFlashGateway.createInstance(mDataHandle, false);
   gFlashGateway.registerEventHandler(mpMovie, mSimEventHandle);

   setFlag(cFlagInitialized, true);
   return true;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashReticle::deinit()
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
void BFlashReticle::enter()
{
   if (!getFlag(cFlagInitialized))
      return;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashReticle::leave()
{
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashReticle::update(float elapsedTime)
{   
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashReticle::renderBegin()
{   
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashReticle::render()
{   
   SCOPEDSAMPLEID(FlashUIGame, 0xFFFF0000);
   if (!mpMovie)
      return;

   if (mbVisible)
      mpMovie->render();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashReticle::renderEnd()
{
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BFlashReticle::handleInput(int port, BInputEventType event, BInputControlType controlType, BInputEventDetail& detail)
{
   return false;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BFlashReticle::receiveEvent(const BEvent& event, BThreadIndex threadIndex)
{
   return false;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BManagedTextureHandle BFlashReticle::getRenderTargetTexture()
{
   if (mpMovie)
      return mpMovie->mRenderTargetHandle;

   return cInvalidManagedTextureHandle;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashReticle::setReticleHelp(int button, int frameID)
{
   if (!mpMovie)
      return;

   if (button < 0 || button >= cButtonCount)
      return;

   debugRangeCheck(button, cButtonCount);
   
   if (mHelpButtonState[button].mFrameID == frameID)
      return;

   mHelpButtonState[button].mFrameID = frameID;
   mHelpButtonState[button].mVisible = (frameID == cReticleKeyframeOff) ? false : true;
   mHelpButtonState[button].mDirty = true;   
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const BReticleHelpButtonState* BFlashReticle::getButtonState(int buttonID)
{
   debugRangeCheck(buttonID, cButtonCount);
   if (buttonID < 0 || buttonID >= cButtonCount)
      return NULL;
   
   return &mHelpButtonState[buttonID];
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashReticle::updateButtons()
{
   if (!mpMovie)
      return;

   GFxValue buttonID[cButtonCount];
   GFxValue frameID[cButtonCount];
   GFxValue dirty  [cButtonCount];
   GFxValue visible[cButtonCount];
   bool bIsAnyDirty = false;      
   for (int i = 0; i < cButtonCount; ++i)
   {  
      buttonID[i].SetString(getProtoControlPath(i));
      frameID[i].SetString(getProtoKeyFrameName(mHelpButtonState[i].mFrameID));
      visible[i].SetBoolean(mHelpButtonState[i].mVisible);

      if (mHelpButtonState[i].mDirty)
         bIsAnyDirty = true;

      dirty[i].SetBoolean(mHelpButtonState[i].mDirty);      
      mHelpButtonState[i].mDirty = false;
   }

   if (bIsAnyDirty)
   {
      mpMovie->setVariableArray("_global.myScene.mButtonStateDirty", dirty, (int) cButtonCount);
      mpMovie->setVariableArray("_global.myScene.mButtonStateVisible", visible, (int) cButtonCount);
      mpMovie->setVariableArray("_global.myScene.mButtonStateControlID", buttonID, (int) cButtonCount);
      mpMovie->setVariableArray("_global.myScene.mButtonStateFrameID", frameID, (int) cButtonCount);
      mpMovie->invokeActionScript(getProtoASFunctionName(cReticleASFunctionUpdateButtonStates), NULL, 0);
   }
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashReticle::setMode(int mode, uint goodAgainstRating, const BCost* pCost)
{
   if (!mpMovie)
      return;

   if ( (mode == mMode) && (mGoodAgainstRating == goodAgainstRating))
      return;

   const char* pKeyFrameName = gUIGame.getReticleFlashKeyframe(mode);

   mGoodAgainstRating = goodAgainstRating;
   mMode=mode;

   GFxValue values[2];
   values[0].SetString(pKeyFrameName);
   values[1].SetNumber((double)goodAgainstRating);

   mpMovie->invokeActionScript("setMode", values, 2);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashReticle::setDimension(int x, int y, int width, int height)
{
   if (!mpMovie)
      return;

   mpMovie->setDimension(x, y, width, height);
}