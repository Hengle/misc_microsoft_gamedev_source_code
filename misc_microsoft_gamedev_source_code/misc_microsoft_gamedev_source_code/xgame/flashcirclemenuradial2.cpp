//============================================================================
// flashcirclemnuradial2.cpp
// Ensemble Studios (c) 2006
//============================================================================

#include "common.h"
#include "flashcirclemenuradial2.h"

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
BFlashCircleMenuRadial2::BFlashCircleMenuRadial2():
   mCivID(-1),
   mpMovie(NULL)
{
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BFlashCircleMenuRadial2::~BFlashCircleMenuRadial2()
{
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashCircleMenuRadial2::init(const char* filename)
{
   gFlashGateway.getOrCreateData(filename, mDataHandle);
   mpMovie = gFlashGateway.createInstance(mDataHandle, true);
   gFlashGateway.registerEventHandler(mpMovie, mSimEventHandle);
   
   setFlag(cFlagInitialized, true);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashCircleMenuRadial2::deinit()
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
void BFlashCircleMenuRadial2::enter()
{
   if (!getFlag(cFlagInitialized))
      return;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashCircleMenuRadial2::leave()
{
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashCircleMenuRadial2::update()
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
void BFlashCircleMenuRadial2::renderBegin()
{   
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashCircleMenuRadial2::render()
{   
   BScopedPIXNamedEvent pixEvent("FlashUIGame", 0xFFFF0000);

   mpMovie->render();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashCircleMenuRadial2::renderEnd()
{
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BFlashCircleMenuRadial2::handleInput(int port, BInputEventType event, BInputControlType controlType, BInputEventDetail& detail)
{
   return false;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BFlashCircleMenuRadial2::receiveEvent(const BEvent& event, BThreadIndex threadIndex)
{
   return false;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BTextureHandle BFlashCircleMenuRadial2::getRenderTargetTexture()
{
   if (mpMovie)
      return mpMovie->mRenderTargetHandle;

   return cInvalidTextureHandle;
}