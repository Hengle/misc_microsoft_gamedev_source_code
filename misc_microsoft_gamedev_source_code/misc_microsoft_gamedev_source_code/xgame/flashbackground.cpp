//============================================================================
// flashinfopanel.cpp
// Ensemble Studios (c) 2006
//============================================================================

#include "common.h"
#include "flashbackground.h"

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
BFlashBackground::BFlashBackground():
   mCivID(-1),
   mpMovie(NULL)
{
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BFlashBackground::~BFlashBackground()
{
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashBackground::init(const char* filename)
{
   gFlashGateway.getOrCreateData(filename, mDataHandle);
   mpMovie = gFlashGateway.createInstance(mDataHandle, true);
   gFlashGateway.registerEventHandler(mpMovie, mSimEventHandle);
   
   setFlag(cFlagInitialized, true);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashBackground::deinit()
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
void BFlashBackground::enter()
{
   if (!getFlag(cFlagInitialized))
      return;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashBackground::leave()
{
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashBackground::update()
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
void BFlashBackground::renderBegin()
{   
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashBackground::render()
{   
   BScopedPIXNamedEvent pixEvent("FlashUIGame", 0xFFFF0000);

   mpMovie->render();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashBackground::renderEnd()
{
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BFlashBackground::handleInput(int port, BInputEventType event, BInputControlType controlType, BInputEventDetail& detail)
{
   return false;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BFlashBackground::receiveEvent(const BEvent& event, BThreadIndex threadIndex)
{
   return false;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BTextureHandle BFlashBackground::getRenderTargetTexture()
{
   if (mpMovie)
      return mpMovie->mRenderTargetHandle;

   return cInvalidTextureHandle;
}