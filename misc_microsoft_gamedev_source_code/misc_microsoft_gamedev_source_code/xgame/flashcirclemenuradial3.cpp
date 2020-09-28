//============================================================================
// flashcirclemnuradial3.cpp
// Ensemble Studios (c) 2006
//============================================================================

#include "common.h"
#include "flashcirclemenuradial3.h"

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
BFlashCircleMenuRadial3::BFlashCircleMenuRadial3():
   mCivID(-1),
   mpMovie(NULL)
{
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BFlashCircleMenuRadial3::~BFlashCircleMenuRadial3()
{
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashCircleMenuRadial3::init(const char* filename)
{
   gFlashGateway.getOrCreateData(filename, mDataHandle);
   mpMovie = gFlashGateway.createInstance(mDataHandle, true);
   gFlashGateway.registerEventHandler(mpMovie, mSimEventHandle);
   
   setFlag(cFlagInitialized, true);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashCircleMenuRadial3::deinit()
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
void BFlashCircleMenuRadial3::enter()
{
   if (!getFlag(cFlagInitialized))
      return;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashCircleMenuRadial3::leave()
{
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashCircleMenuRadial3::update()
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
void BFlashCircleMenuRadial3::renderBegin()
{   
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashCircleMenuRadial3::render()
{   
   BScopedPIXNamedEvent pixEvent("FlashUIGame", 0xFFFF0000);

   mpMovie->render();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashCircleMenuRadial3::renderEnd()
{
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BFlashCircleMenuRadial3::handleInput(int port, BInputEventType event, BInputControlType controlType, BInputEventDetail& detail)
{
   return false;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BFlashCircleMenuRadial3::receiveEvent(const BEvent& event, BThreadIndex threadIndex)
{
   return false;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BTextureHandle BFlashCircleMenuRadial3::getRenderTargetTexture()
{
   if (mpMovie)
      return mpMovie->mRenderTargetHandle;

   return cInvalidTextureHandle;
}