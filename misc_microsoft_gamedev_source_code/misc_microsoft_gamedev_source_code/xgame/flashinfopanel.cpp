//============================================================================
// flashinfopanel.cpp
// Ensemble Studios (c) 2006
//============================================================================

#include "common.h"
#include "flashinfopanel.h"

//-- render
#include "render.h"
#include "renderDraw.h"
#include "renderThread.h"
#include "rendertargetTexture.h"
//-- game classes
#include "user.h"
#include "usermanager.h"
#include "player.h"
#include "uigame.h"
#include "visualmanager.h"


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BFlashInfoPanel::BFlashInfoPanel():
   mCivID(-1),
   mpMovie(NULL)
{
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BFlashInfoPanel::~BFlashInfoPanel()
{
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashInfoPanel::init(const char* filename)
{
   gFlashGateway.getOrCreateData(filename, mDataHandle);
   mpMovie = gFlashGateway.createInstance(mDataHandle, true);
   gFlashGateway.registerEventHandler(mpMovie, mSimEventHandle);
   
   setFlag(cFlagInitialized, true);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashInfoPanel::deinit()
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
void BFlashInfoPanel::enter()
{
   if (!getFlag(cFlagInitialized))
      return;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashInfoPanel::leave()
{
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashInfoPanel::update()
{
   BUser* pUser = gUserManager.getUser(0);
   BDEBUG_ASSERT(pUser);

   BPlayer* pPlayer = pUser->getPlayer();
   long civID=pPlayer->getCivID();
   if (civID != mCivID)
   {
      mCivID = civID;
   }
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashInfoPanel::renderBegin()
{   
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashInfoPanel::render()
{   
   BScopedPIXNamedEvent pixEvent("FlashUIGame", 0xFFFF0000);

   mpMovie->render();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashInfoPanel::renderEnd()
{
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BFlashInfoPanel::handleInput(int port, BInputEventType event, BInputControlType controlType, BInputEventDetail& detail)
{
   return false;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BFlashInfoPanel::receiveEvent(const BEvent& event, BThreadIndex threadIndex)
{
   return false;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BTextureHandle BFlashInfoPanel::getRenderTargetTexture()
{
   if (mpMovie)
      return mpMovie->mRenderTargetHandle;

   return cInvalidTextureHandle;
}