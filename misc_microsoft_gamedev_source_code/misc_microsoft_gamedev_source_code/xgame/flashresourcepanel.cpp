//============================================================================
// flashuiGame.cpp
// Ensemble Studios (c) 2006
//============================================================================

#include "common.h"
#include "flashresourcepanel.h"

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
BFlashResourcePanel::BFlashResourcePanel():
   mpMovie(NULL),
   mCivID(-1)
{
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BFlashResourcePanel::~BFlashResourcePanel()
{
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashResourcePanel::init(const char* filename)
{
   gFlashGateway.getOrCreateData(filename, mDataHandle);
   mpMovie = gFlashGateway.createInstance(mDataHandle, true);
   gFlashGateway.registerEventHandler(mpMovie, mSimEventHandle);

   setFlag(cFlagInitialized, true);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashResourcePanel::deinit()
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
void BFlashResourcePanel::enter()
{
   if (!getFlag(cFlagInitialized))
      return;

   BUser* pUser = gUserManager.getUser(0);
   BDEBUG_ASSERT(pUser);

   BPlayer* pPlayer = pUser->getPlayer();
   mCivID=pPlayer->getCivID();
   
   initResourcePanel();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashResourcePanel::leave()
{
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashResourcePanel::update()
{
   if (!getFlag(cFlagInitialized))
      return;   
      
   updateResourcePanel();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashResourcePanel::renderBegin()
{   
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashResourcePanel::render()
{   
   BScopedPIXNamedEvent pixEvent("FlashUIGame", 0xFFFF0000);

   mpMovie->render();

   /*
   mpMovie->renderToTexture(mRenderTargetHandle);
   int x = 300;
   int y = 300;
   int width = 128;
   int height = 128;
   gUI.renderTexture(mRenderTargetHandle, x, y, x+width, y+height);
   */
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashResourcePanel::renderEnd()
{
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BFlashResourcePanel::handleInput(int port, BInputEventType event, BInputControlType controlType, BInputEventDetail& detail)
{
   return false;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BFlashResourcePanel::receiveEvent(const BEvent& event, BThreadIndex threadIndex)
{
   return false;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashResourcePanel::initResourcePanel()
{
   if (mCivID == 1)
      mpMovie->invokeActionScript("_root.gotoandplay", "%s", "UNSC_res");
   else if (mCivID == 2)
      mpMovie->invokeActionScript("_root.gotoandplay", "%s", "COV_res");
   else
      BASSERT(0); // unsupported civ!!!!!
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashResourcePanel::updateResourcePanel()
{
   BUser* pUser = gUserManager.getUser(0);
   BDEBUG_ASSERT(pUser);

   BPlayer* pPlayer = pUser->getPlayer();
   long civID=pPlayer->getCivID();
   if (civID != mCivID)
   {
      mCivID = civID;
      initResourcePanel();
   }

   //-- civ ID == UNSC
   if (civID == 1)
   {      
      BFixedString32 str;
      BUIGamePlayerStat* pStat=gUIGame.getPlayerStat(civID, 0);
      str.format("%d/%d", pPlayer->getPopCount(pStat->mID)+pPlayer->getPopFuture(pStat->mID), min(pPlayer->getPopCap(pStat->mID), pPlayer->getPopMax(pStat->mID)));
      mpMovie->setVariable("_root.UNSC_MC.UNSC_res_pop.value", BStrConv::toA(str), GFxMovie::SV_Normal);

      pStat=gUIGame.getPlayerStat(civID, 1);
      str.format("%d", (long) pPlayer->getResource(pStat->mID));
      mpMovie->setVariable("UNSC_MC.UNSC_res_power.value", BStrConv::toA(str), GFxMovie::SV_Normal);

      pStat=gUIGame.getPlayerStat(civID, 2);
      str.format("%d", (long) pPlayer->getResource(pStat->mID));
      mpMovie->setVariable("UNSC_MC.UNSC_res_supplies.value", BStrConv::toA(str), GFxMovie::SV_Normal);
   }
   else if (civID == 2)
   {
      BFixedString32 str;
      BUIGamePlayerStat* pStat=gUIGame.getPlayerStat(civID, 0);
      str.format("%d/%d", pPlayer->getPopCount(pStat->mID)+pPlayer->getPopFuture(pStat->mID), min(pPlayer->getPopCap(pStat->mID), pPlayer->getPopMax(pStat->mID)));
      mpMovie->setVariable("COV_MC.COV_res_pop.value", BStrConv::toA(str), GFxMovie::SV_Normal);

      /* ajl 11/14/06 - organics have been removed from the game
      pStat=gUIGame.getPlayerStat(civID, 1);
      str.format("%d", (long) pPlayer->getResource(pStat->mID));
      mpMovie->setVariable("COV_MC.COV_res_organics.value", BStrConv::toA(str), GFxMovie::SV_Normal);
      */

      pStat=gUIGame.getPlayerStat(civID, 1);
      str.format("%d", (long) pPlayer->getResource(pStat->mID));
      mpMovie->setVariable("COV_MC.COV_res_relics.value", BStrConv::toA(str), GFxMovie::SV_Normal);

      pStat=gUIGame.getPlayerStat(civID, 2);
      str.format("%d", (long) pPlayer->getResource(pStat->mID));
      mpMovie->setVariable("COV_MC.COV_res_favor.value", BStrConv::toA(str), GFxMovie::SV_Normal);
   }
   else
   {
      BASSERT(0);
   }
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BTextureHandle BFlashResourcePanel::getRenderTargetTexture()
{
   if (mpMovie)
      return mpMovie->mRenderTargetHandle;
   
   return cInvalidTextureHandle;
}