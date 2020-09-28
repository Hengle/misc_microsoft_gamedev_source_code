//============================================================================
// flashcirclemnuradial1.cpp
// Ensemble Studios (c) 2006
//============================================================================

#include "common.h"
#include "flashcirclemenuradial1.h"

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
BFlashCircleMenuRadial1::BFlashCircleMenuRadial1():
   mCivID(-1),
   mpMovie(NULL)
{
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BFlashCircleMenuRadial1::~BFlashCircleMenuRadial1()
{
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashCircleMenuRadial1::init(const char* filename)
{
   gFlashGateway.getOrCreateData(filename, mDataHandle);
   mpMovie = gFlashGateway.createInstance(mDataHandle, true);
   gFlashGateway.registerEventHandler(mpMovie, mSimEventHandle);
   
   setFlag(cFlagInitialized, true);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashCircleMenuRadial1::deinit()
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
void BFlashCircleMenuRadial1::enter()
{
   if (!getFlag(cFlagInitialized))
      return;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashCircleMenuRadial1::leave()
{
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashCircleMenuRadial1::update()
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
void BFlashCircleMenuRadial1::renderBegin()
{   
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashCircleMenuRadial1::render()
{   
   BScopedPIXNamedEvent pixEvent("FlashUIGame", 0xFFFF0000);

   mpMovie->render();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashCircleMenuRadial1::renderEnd()
{
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BFlashCircleMenuRadial1::handleInput(int port, BInputEventType event, BInputControlType controlType, BInputEventDetail& detail)
{
   return false;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BFlashCircleMenuRadial1::receiveEvent(const BEvent& event, BThreadIndex threadIndex)
{
   return false;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BTextureHandle BFlashCircleMenuRadial1::getRenderTargetTexture()
{
   if (mpMovie)
      return mpMovie->mRenderTargetHandle;

   return cInvalidTextureHandle;
}