//============================================================================
// flashhud.cpp
// Ensemble Studios (c) 2006
//============================================================================

#include "common.h"
#include "flashhud.h"

//-- render
#include "render.h"
#include "renderDraw.h"
#include "renderThread.h"

#include "camera.h"
//-- game classes
#include "user.h"
#include "usermanager.h"
#include "player.h"
#include "uigame.h"
#include "visualmanager.h"
#include "database.h"
#include "protoobject.h"
#include "protosquad.h"
#include "techtree.h"
#include "prototech.h"
#include "protopower.h"
#include "world.h"
#include "ability.h"
#include "FontSystem2.h"
#include "configsgame.h"
#include "ChatManager.h"
#include "game.h"
#include "inputinterface.h"
#include "squad.h"

BUString BFlashHUD::sLocalTempUString;

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BFlashHUD::BFlashHUD():
mpMovie(NULL),
mOverlayVisible(false),
mPowerOverlayVisible(false),
mUnitSelectionVisible(false),
mUnitCardVisible(false),
mGameTimeVisible(false),
mResourcePanelVisible(false),
mCircleMenuVisible(false),
mCircleCount(8),
mCurrentItem(-1),
mCurrentOrder(-1),
mPointingAtX(0.0f),
mPointingAtY(0.0f),
mCurInfoPanelSlot(-1),
mScoresVisible(false),
mUnitStatsVisible(false),
mChatDisplayVisible(false),
mButtonPanelVisible(false),
mPowerPanelVisible(false),
mDPadVisible(false),
mTechNotificationEndTimer(0.0f),
mTechNotificationID(-1),
mCurrentItemTimer(0.0f),
mCircleMenuExtraInfoVisible(false),
mpUser(NULL)
{
   mEnableStates.setAll(1);
   memset(&mUnitSlotID, 0, sizeof(mUnitSlotID));
   memset(&mUnitCard, 0, sizeof(mUnitCard));

   // init the stats data
   for(DWORD i=0; i<cMaxUnitStatsData; i++)
      mUnitStatsData[i] = 0;
   mUnitStatName.set(L"");

   for(DWORD i=0; i<cMaxPlayerSlots; i++)
      mPlayerColors[i] = -1;

   initAttackAngles();

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BFlashHUD::~BFlashHUD()
{
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashHUD::initAttackAngles()
{
   // I'd like to load this up from the XML file, but currently I don't have access to the nodes
   // The way to make it so is to have the parent call a method on a node it doesn't understand
   //  and have the child override that so it can read the data it needs to read.
   // If we have to change this data (and I suspect we will), then let's make it data driven
   //  the first time we have to change it.

   mAngleNormalizer=23;                   // our first angle is 23 degrees before 0.
   for (int i=0; i<cMaxAttackSlots; i++)
   {
      mAttackSlotsStates[i]=false;
      mAttackSlotsStatesCache[i]=false;

      int minAngle = 0;
      int maxAngle = 0;
      switch (i)
      {
         case 0:
            minAngle=0;
            maxAngle=45;
            break;
         case 1:
            minAngle=46;
            maxAngle=90;
            break;
         case 2:
            minAngle=91;
            maxAngle=135;
            break;
         case 3:
            minAngle=136;
            maxAngle=180;
            break;
         case 4:
            minAngle=181;
            maxAngle=225;
            break;
         case 5:
            minAngle=226;
            maxAngle=270;
            break;
         case 6:
            minAngle=271;
            maxAngle=315;
            break;
         case 7:
            minAngle=216;
            maxAngle=360;
            break;
      }

      mAttackSlotsAngleMin[i]=minAngle;
      mAttackSlotsAngleMax[i]=maxAngle;


   }

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BFlashHUD::init(const char* filename, const char* dataFile)
{
   if (!filename)
      return false;

   if (!dataFile)
      return false;

   if (!loadData(dataFile))
      return false;

   gFlashGateway.getOrCreateData(filename, cFlashAssetCategoryInGame, mDataHandle);
   mpMovie = gFlashGateway.createInstance(mDataHandle);
   gFlashGateway.registerEventHandler(mpMovie, mSimEventHandle);

   mText.resize(eControlLabelTotal);
   
   initLookups();   

   initStringIDs();

   setFlag(cFlagInitialized, true);

   initResolution();

   // get properties from the xml file that defined the minimap
   const BFlashProperties* pProps = getProperties();   
   if (pProps)
   {
      BFlashPropertyHandle handle = pProps->findStringHandle("PlaceHolderIcon");
      bool result = pProps->getString(handle, mPlaceholderIcon);
      BDEBUG_ASSERT(result);
      if (!result)
         return false;
   }     
   
   return true;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashHUD::initLookups()
{
   for(int i = 0; i < getProtoIconCount(); i++)
   {
      BFlashProtoIcon* pIcon = getProtoIcon(i);
      BDEBUG_ASSERT(pIcon !=NULL);
            
      BFlashIconLookup icon;
      icon.mIndex = i;
      

      
      //-- is this a unit selection keyframe?      
      if (pIcon->mType.compare("unitselection") == 0)
      {         
         int protoSquadID = gDatabase.getProtoSquad(pIcon->mOwnerName);
         int protoObjID   = gDatabase.getProtoObject(pIcon->mOwnerName);

         icon.mProtoID = -1;
         if (protoSquadID != -1)
         {
            icon.mProtoID = protoSquadID;
            BProtoSquad* pProtoSquad = gDatabase.getGenericProtoSquad(protoSquadID); 
            pProtoSquad->setUnitSelectionIconID(i);
         }
         else if (protoObjID != -1)
         {
            icon.mProtoID = protoObjID;
//-- FIXING PREFIX BUG ID 3255
            const BProtoObject* pProtoObject = gDatabase.getGenericProtoObject(protoObjID);
//--
            pProtoObject->setUnitSelectionIconID(i);
         }

         if (icon.mProtoID != -1)
         {            
            mUnitSelectionIconLookup.add(icon);
         }
      }
      else if (pIcon->mType.compare("powericon") == 0)
      {                  
         int protoID = gDatabase.getProtoPowerIDByName(pIcon->mOwnerName.c_str());
         if (protoID != -1)
         {
            icon.mProtoID = protoID;
            mPowerIconLookup.add(icon);
            BProtoPower* pProtoPower = gDatabase.getProtoPowerByID(protoID);
            if(pProtoPower)
            {
               pProtoPower->setCircleMenuIconID(i);
               pProtoPower->setUIPowerOverlayID(i);
            }
         }
      }
      else if (pIcon->mType.compare("abilityicon") == 0)
      {                  
         int protoID = gDatabase.getAbilityIDFromName(pIcon->mOwnerName.c_str());
         if (protoID != -1)
         {
            icon.mProtoID = protoID;
            mAbilityIconLookup.add(icon);
            BAbility* pAbility = gDatabase.getAbilityFromID(protoID);
            if(pAbility)
            {
               pAbility->setCircleMenuIconID(i);
            }
         }
      }
      else if (pIcon->mType.compare("techicon") == 0)
      {
         int protoID = gDatabase.getProtoTech(pIcon->mOwnerName.c_str());
         if (protoID != -1)
         {
            icon.mProtoID = protoID;
            mTechIconLookup.add(icon);

//-- FIXING PREFIX BUG ID 3256
            const BProtoTech* pProtoTech = gDatabase.getProtoTech(protoID);
//--
            pProtoTech->setCircleMenuIconID(i);            
         }
      }
      else if (pIcon->mType.compare("uniticon") == 0)
      {
         int protoSquadID = gDatabase.getProtoSquad(pIcon->mOwnerName);
         int protoObjID   = gDatabase.getProtoObject(pIcon->mOwnerName);

         icon.mProtoID = -1;
         if (protoSquadID != -1)
         {
            icon.mProtoID = protoSquadID;
            BProtoSquad* pProtoSquad = gDatabase.getGenericProtoSquad(protoSquadID); 
            if(pProtoSquad)
            {
               pProtoSquad->setCircleMenuIconID(i);
            }
            for (int j=0; j<gWorld->getNumberPlayers(); j++)
            {
               pProtoSquad = gWorld->getPlayer(j)->getProtoSquad(protoSquadID);
               if(pProtoSquad)
               {
                  pProtoSquad->setCircleMenuIconID(i);
               }
            }
         }
         else if (protoObjID != -1)
         {
            icon.mProtoID = protoObjID;
            BProtoObject* pProtoObject = gDatabase.getGenericProtoObject(protoObjID);
            int protoSquadID = pProtoObject->getProtoSquadID();
            pProtoObject->setCircleMenuIconID(i);
            for (int j=0; j<gWorld->getNumberPlayers(); j++)
            {
               pProtoObject = gWorld->getPlayer(j)->getProtoObject(protoObjID);
               pProtoObject->setCircleMenuIconID(i);
            }

            if (protoSquadID != -1)
            {
               BProtoSquad* pProtoSquad = gDatabase.getGenericProtoSquad(protoSquadID);
               if (pProtoSquad)
               {
                  pProtoSquad->setCircleMenuIconID(i);            
                  for (int j=0; j<gWorld->getNumberPlayers(); j++)
                  {
                     pProtoSquad = gWorld->getPlayer(j)->getProtoSquad(protoSquadID);
                     pProtoSquad->setCircleMenuIconID(i);
                  }
               }
            }
         }

         if (icon.mProtoID != -1)
         {            
            mUnitIconLookup.add(icon);
         }
      }
      else if (pIcon->mType.compare("buildingicon") == 0)
      {
         int protoID = gDatabase.getProtoObject(pIcon->mOwnerName.c_str());
         if (protoID != -1)
         {
            icon.mProtoID = protoID;
            mBuildingIconLookup.add(icon);

            BProtoObject* pProtoObject = gDatabase.getGenericProtoObject(protoID);
            int protoSquadID = pProtoObject->getProtoSquadID();
            pProtoObject->setCircleMenuIconID(i);            
            for (int j=0; j<gWorld->getNumberPlayers(); j++)
            {
               pProtoObject = gWorld->getPlayer(j)->getProtoObject(protoID);
               pProtoObject->setCircleMenuIconID(i);
            }

            if (protoSquadID != -1)
            {
               BProtoSquad* pProtoSquad = gDatabase.getGenericProtoSquad(protoSquadID);
               if (pProtoSquad)
               {
                  pProtoSquad->setCircleMenuIconID(i);            
                  for (int j=0; j<gWorld->getNumberPlayers(); j++)
                  {
                     pProtoSquad = gWorld->getPlayer(j)->getProtoSquad(protoSquadID);
                     pProtoSquad->setCircleMenuIconID(i);
                  }
               }
            }
         }
      }
      else if (pIcon->mType.compare("miscicon") == 0)
      {
         int id = gUIGame.getGameCommandID(pIcon->mOwnerName.c_str());
         if (id == -1)
            id = gDatabase.getProtoObjectCommandType(pIcon->mOwnerName.c_str());
         if (id == -1)
            id = gDatabase.getProtoObject(pIcon->mOwnerName.c_str());
         if (id != -1)
         {
            icon.mProtoID = id;
            mMiscIconLookup.add(icon);
         }
      }
      else if (pIcon->mType.compare("staticon") == 0)
      {         
         int id = gUIGame.getUnitStatIndex(pIcon->mOwnerName.c_str());
         if (id != -1)
         {
            icon.mProtoID = id;
            mStatIconLookup.add(icon);
         }         
      }
      else if (pIcon->mType.compare("notification") == 0)
      {
         int id = gDatabase.getProtoTech(pIcon->mOwnerName.c_str());
         if (id != -1)
         {
            icon.mProtoID = id;
            mNotificationIconLookup.add(icon);
            BProtoTech* pProtoTech = gDatabase.getProtoTech(id); 
            pProtoTech->setUINotificationID(i);
         }
      }
      else if (pIcon->mType.compare("InputFunction") == 0)
      {
         int id = BInputInterface::lookupFunction(pIcon->mOwnerName.c_str());
         if (id != -1)
         {
            icon.mProtoID = id;
            mInputFunctionIconLookup.add(icon);
         }         
      }
   }

   int numProtoSquads = gDatabase.getNumberProtoSquads();
   for (int i=0; i<numProtoSquads; i++)
   {
      BProtoSquad* pProtoSquad = gDatabase.getGenericProtoSquad(i); 
      if (!pProtoSquad->getAltIcon().isEmpty())
      {
         int id = lookupIconID("uniticon", pProtoSquad->getAltIcon());
         if (id != -1)
         {
            pProtoSquad->setAltCircleMenuIconID(id);
            for (int j=0; j<gWorld->getNumberPlayers(); j++)
            {
               pProtoSquad = gWorld->getPlayer(j)->getProtoSquad(i);
               pProtoSquad->setAltCircleMenuIconID(id);
            }
         }
      }
   }
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashHUD::initStringIDs()
{
   const BFlashProperties* pProps = getProperties();
   BDEBUG_ASSERT(pProps);

   int stringID = -1;   
   pProps->getLocStringID(pProps->findHandle("CombatGradeString"), stringID);
   mHUDStringID[eHUDStringCombatGrade] = gDatabase.getLocStringIndex(stringID);      
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashHUD::deinit()
{
   if (mpMovie)
   {
      gFlashGateway.unregisterEventHandler(mpMovie, mSimEventHandle);
      gFlashGateway.releaseInstance(mpMovie);
   }
   mpMovie = NULL;

   mpUser = NULL;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashHUD::enter()
{
   if (!getFlag(cFlagInitialized))
      return;
   
   initResourcePanel();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashHUD::leave()
{
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashHUD::update(float elapsedTime)
{
   if (!getFlag(cFlagInitialized))
      return;   

   if (!mpMovie)
      return;

   /* TEST CODE -- NEVER ENABLE THIS UNLESS DEBUGGING
   static bool bTestInit = false;
   if (bTestInit)
   {
      initResolution();
      bTestInit = false;
   }
   */
  
   //updatePopupHelp(elapsedTime);
   
   updateResourcePanel();
   updateLabels();
   updateCostTextColors();
   updateAlerts();
   updateTechNotification();   
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashHUD::updatePopupHelp(float elapsedTime)
{
   static float popupDelay = 2.0f;
   if (mCurrentItem != -1)
      mCurrentItemTimer+=elapsedTime;

   if (mCurrentItemTimer > popupDelay)
   {
      setCircleMenuExtraInfoVisible(true);
   }
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashHUD::renderBegin()
{   
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashHUD::render()
{   
   SCOPEDSAMPLEID(FlashUIGame, 0xFFFF0000);

   if (mpMovie)
      mpMovie->render();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashHUD::renderEnd()
{
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BFlashHUD::handleInput(int port, BInputEventType event, BInputControlType controlType, BInputEventDetail& detail)
{
   if (!mCircleMenuVisible)
      return false;
   
   bool start=(event==cInputEventControlStart);
   bool repeat=(event==cInputEventControlRepeat);
   bool stop=(event==cInputEventControlStop);

   if(start || repeat || stop)
   {
      switch(controlType)
      {
         case cStickLeft:
            {
               updatePointingAt(detail.mX, detail.mY, true);
               return true;
               break;
            }
         case cStickRightUp:
            {
               if (start)
                  startInfoPanelScroll(false);
               else if (stop)
                  stopInfoPanelScroll();
               break;
            }
         case cStickRightDown:
            {
               if (start)
                  startInfoPanelScroll(true);
               else if (stop)
                  stopInfoPanelScroll();
               break;
            }
      }
   }

   return false;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashHUD::startInfoPanelScroll(bool scrollUp)
{   
   if (!mpMovie)
      return;

   if (scrollUp)
   {         
      mpMovie->invokeActionScript(getProtoASFunctionName(eHUDASFunctionStartInfoPanelScrollUp), NULL, 0);         
   }
   else
   {
      mpMovie->invokeActionScript(getProtoASFunctionName(eHUDASFunctionStartInfoPanelScrollDown), NULL, 0);
   }
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashHUD::stopInfoPanelScroll()
{
   if (!mpMovie)
      return;

   mpMovie->invokeActionScript(getProtoASFunctionName(eHUDASFunctionStopInfoPanelScroll), NULL, 0);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BFlashHUD::receiveEvent(const BEvent& event, BThreadIndex threadIndex)
{
   return false;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashHUD::initButtonPanel()
{
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashHUD::initResourcePanel()
{      
   mResourcePanelVisible = true;

   int initresourcePanelASFunctionID = eHUDASFunctionInitResourcePanel;
   if (gGame.isSplitScreen())
   {
      if (mpUser == gUserManager.getSecondaryUser())
         initresourcePanelASFunctionID = eHUDASFunctionInitResourcePanelSplitRight;
      else
         initresourcePanelASFunctionID = eHUDASFunctionInitResourcePanelSplitLeft;
   }
   
   mpMovie->invokeActionScript(getProtoASFunctionName(initresourcePanelASFunctionID), "%s", getProtoKeyFrameName(eHUDKeyframeEaseIn));   
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashHUD::updateResourcePanel()
{
   if (!mResourcePanelVisible)
      return;

   if (!mpUser)
      return;

   BDEBUG_ASSERT(mpUser);

//-- FIXING PREFIX BUG ID 3258
   const BPlayer* pPlayer = mpUser->getPlayer();   
//--
   
   long civID=pPlayer->getCivID();
         
   sLocalTempUString.empty();

   int resource1ControlPathID = eControlResourcePanelResource1;
   int resource2ControlPathID = eControlResourcePanelResource2;
   int resource3ControlPathID = eControlResourcePanelResource3;

   if (gGame.isSplitScreen())
   {
      if (mpUser == gUserManager.getSecondaryUser())
      {
         resource1ControlPathID = eControlResourcePanelResource1SplitRight;
         resource2ControlPathID = eControlResourcePanelResource2SplitRight;
         resource3ControlPathID = eControlResourcePanelResource3SplitRight;
      }
      else
      {
         resource1ControlPathID = eControlResourcePanelResource1SplitLeft;
         resource2ControlPathID = eControlResourcePanelResource2SplitLeft;
         resource3ControlPathID = eControlResourcePanelResource3SplitLeft;
      }
   }
   
   int panelIndex=0;

   for (int i=0; i<gUIGame.getNumberPlayerStats(civID) && panelIndex<3; i++)
   {
      const BUIGamePlayerStat* pStat=gUIGame.getPlayerStat(civID, i);
      if (!pStat)
         continue;
      if (pStat->mType == BUIGamePlayerStat::cTypePop)
      {
         float curPop = pPlayer->getPopCount(pStat->mID)+pPlayer->getPopFuture(pStat->mID);
         float popMax = min(pPlayer->getPopCap(pStat->mID), pPlayer->getPopMax(pStat->mID));
         sLocalTempUString.locFormat(L"%.0f/%0.f", curPop, popMax);
      }
      else if (pStat->mType == BUIGamePlayerStat::cTypeResource)
      {
         int stat =    Math::ClampHigh(Math::FloatToIntTrunc(pPlayer->getResource(pStat->mID)), 99999);
         if (pStat->mID2==-1)
            sLocalTempUString.locFormat(L"%d", stat);
         else
         {
            int rate = Math::ClampHigh(Math::FloatToIntTrunc(pPlayer->getRate(pStat->mID2)), 99999);
            sLocalTempUString.locFormat(L"%d/+%d", stat, rate);
         }
      }
      else
         continue;

      GFxValue value;
      value.SetStringW(sLocalTempUString.getPtr());
      switch (panelIndex)
      {
         case 0: mpMovie->setVariable(getProtoControlPath(resource1ControlPathID), value, GFxMovie::SV_Normal); break;
         case 1: mpMovie->setVariable(getProtoControlPath(resource2ControlPathID), value, GFxMovie::SV_Normal); break;
         case 2: mpMovie->setVariable(getProtoControlPath(resource3ControlPathID), value, GFxMovie::SV_Normal); break;
      }
      panelIndex++;
   }
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashHUD::updateLabels()
{        
   for (int i = 0; i < eControlLabelTotal; i++)
   {            
      if (i >= eControlLabelUnitSelectionStart && i <= eControlLabelUnitSelectionEnd)
            continue;

      if ( i == eControlLabelGameTime)
         continue;

      //-- BTK Disbled Lables for buttons
      if (!mButtonPanelVisible && i >= eControlLabelButtonX && i<=eControlLabelButtonB)
         continue;

      //-- BTK Disbled Lables for dpad
      if (i >= eControlLabelDPadUp && i <= eControlLabelDPadRight)
         continue;

      if (i == eControlLabelButtonShoulderRight || i == eControlLabelButtonShoulderLeft)
         continue;

      if (!mScoresVisible && i >= eControlLabelPlayerScoreBegin && i <= eControlLabelPlayerScoreEnd)
         continue;

      if (mEnableStates.isSet(i) != 0)
         setText(i, mText[i]);
      else      
         setText(i, L"");
   }
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashHUD::refresh()
{   
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashHUD::setOverlayVisible(bool bON)
{
   if (bON == mOverlayVisible)
      return;

   mOverlayVisible = bON;

   if (mOverlayVisible)
      mpMovie->invokeActionScript(getProtoASFunctionName(eHUDASFunctionSetOverlayVisible), "%s", getProtoKeyFrameName(eHUDKeyframeEaseIn));
   else
      mpMovie->invokeActionScript(getProtoASFunctionName(eHUDASFunctionSetOverlayVisible), "%s", getProtoKeyFrameName(eHUDKeyframeEaseOut));      
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashHUD::setPowerOverlayVisible(int powerID, bool bON)
{
   if ((bON == mPowerOverlayVisible) && (mPowerOverlayID == powerID))
      return;
      
   int protoPowerID = -1;
   if (bON)
   {
      protoPowerID = powerID;
      mPowerOverlayID = powerID;
   }
   else
   {
      protoPowerID = mPowerOverlayID;
      mPowerOverlayID = -1;
   }

   mPowerOverlayVisible = bON;   

//-- FIXING PREFIX BUG ID 3259
   const BProtoPower* pProtopower = NULL;
//--
   pProtopower = gDatabase.getProtoPowerByID(protoPowerID);
   if (!pProtopower)
      return;

//-- FIXING PREFIX BUG ID 3260
   const BFlashProtoIcon* pProtoIcon = getProtoIcon(pProtopower->getUIPowerOverlayID());
//--
   if (!pProtoIcon)
      return;

   GFxValue value[2];
   value[0].SetString(pProtoIcon->mName2.c_str());
   value[1].SetBoolean(bON);

   mpMovie->invokeActionScript(getProtoASFunctionName(eHUDASFunctionSetPowerOverlayVisible), value, 2);   
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashHUD::setPowerOverlayText(int powerID, const BUString& text)
{
//-- FIXING PREFIX BUG ID 3261
   const BProtoPower* pProtopower = NULL;
//--
   pProtopower = gDatabase.getProtoPowerByID(powerID);
   if (!pProtopower)
      return;

//-- FIXING PREFIX BUG ID 3262
   const BFlashProtoIcon* pProtoIcon = getProtoIcon(pProtopower->getUIPowerOverlayID());
//--
   if (!pProtoIcon)
      return;

   GFxValue value[2];

   value[0].SetString(pProtoIcon->mName2.c_str());
   value[1].SetStringW(text.getPtr());
   mpMovie->invokeActionScript("setPowerOverlayText", value, 2);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashHUD::setText(int labelID, const BUString& text)
{   
   setText(labelID, text.getPtr());
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashHUD::setText(int labelID, const WCHAR* pText)
{   
   GFxValue value;
   value.SetStringW(pText);
   mpMovie->setVariable(getProtoControlPath(labelID), value, GFxMovie::SV_Normal);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------1
BManagedTextureHandle BFlashHUD::getRenderTargetTexture()
{
   if (mpMovie)
      return mpMovie->mRenderTargetHandle;

   return cInvalidManagedTextureHandle;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashHUD::setControlText(int labelID, const BUString& text)
{
   debugRangeCheck(labelID, eControlLabelTotal);
   mText[labelID] = text;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashHUD::setControlText(int labelID, const WCHAR* pText)
{
   debugRangeCheck(labelID, eControlLabelTotal);
   mText[labelID] = pText;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashHUD::setControlVisible(int labelID, bool bVisible)
{
   debugRangeCheck(labelID, eControlTotal);
   if (bVisible) 
      mEnableStates.set(labelID);
   else
      mEnableStates.unset(labelID);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashHUD::setAllControlsVisible(bool bVisible)
{
   for (int i = 0; i < eControlTotal; i++)
      setControlVisible(i, bVisible);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashHUD::displayChat(BChatMessage* pChat)
{
   int speaker = pChat->getSpeakerID();
   speaker++;
   GFxValue values[2];
   values[0].SetStringW(pChat->getChatString().getPtr());
   values[1].SetNumber(speaker);
   mpMovie->invokeActionScript(getProtoASFunctionName(eHUDASFunctionSetChatData), values, 2);
   mChatDisplayVisible = true;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashHUD::setChatVisible(bool bVisible)
{
   if (bVisible == mChatDisplayVisible)
      return;

   mChatDisplayVisible = bVisible;

   GFxValue value;
   value.SetBoolean(mChatDisplayVisible);   
   mpMovie->invokeActionScript(getProtoASFunctionName(eHUDASFunctionSetChatDisplayVisible), &value, 1);      
}



//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashHUD::setUnitStatsVisible(bool bON)
{
   if (bON == mUnitStatsVisible)
      return;

   mUnitStatsVisible = bON;

   GFxValue value;
   value.SetBoolean(mUnitStatsVisible);   
   mpMovie->invokeActionScript(getProtoASFunctionName(eHUDASFunctionSetUnitStatsVisible), &value, 1);      
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashHUD::setUnitStats(const BUString& name, float stat1, float stat2, float stat3, float stat4, float stat5)
{
   // did we change? 
   bool needUpdate = false;
   if ( (mUnitStatsData[0] != stat1) ||
        (mUnitStatsData[1] != stat2) ||
        (mUnitStatsData[2] != stat3) ||
        (mUnitStatsData[3] != stat4) ||
        (mUnitStatsData[4] != stat5) ||
        (mUnitStatName.compare(name) != 0) )
   {
      needUpdate = true;
   }

   if (!needUpdate)
      return;

   // cache the data
   mUnitStatName.set(name);
   mUnitStatsData[0] = stat1;
   mUnitStatsData[1] = stat2;
   mUnitStatsData[2] = stat3;
   mUnitStatsData[3] = stat4;
   mUnitStatsData[4] = stat5;

   GFxValue values[6];
   values[0].SetStringW(mUnitStatName.getPtr());
   values[1].SetNumber((int)stat1);
   values[2].SetNumber((int)stat2);
   values[3].SetNumber((int)stat3);
   values[4].SetNumber((int)stat4);
   values[5].SetNumber((int)stat5);

   mpMovie->invokeActionScript(getProtoASFunctionName(eHUDASFunctionSetUnitStatsData), values, 6);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashHUD::setScoresVisible(bool bON)
{
   if (bON == mScoresVisible)
      return;

   mScoresVisible = bON;

   GFxValue value;
   value.SetBoolean(mScoresVisible);   
   mpMovie->invokeActionScript(getProtoASFunctionName(eHUDASFunctionSetScoreVisible), &value, 1);      

   //-- set enables states
   for (int i = eControlLabelPlayerScoreBegin; i <=eControlLabelPlayerScoreEnd; ++i)
      setControlVisible(i, mScoresVisible);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashHUD::setPlayerScore(int playerID, const BUString& text)
{   
   int controlID = eControlLabelPlayerScoreBegin + playerID - 1;
   debugRangeCheckIncl(controlID, eControlLabelPlayerScoreBegin, eControlLabelPlayerScoreEnd);
   setControlText(controlID, text);
   setControlVisible(controlID, true);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashHUD::setPlayerColor(int playerID, int color)
{   
   if ( (playerID < 0) || (playerID >= cMaxPlayerSlots) )
      return;

   // don't spam flash
   if (mPlayerColors[playerID]==color)
      return;

   mPlayerColors[playerID]=color;

   GFxValue values[2];
   values[0].SetNumber(playerID);
   values[1].SetNumber(color);

   mpMovie->invokeActionScript("setPlayerColor", values, 2); 
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashHUD::setButtonState(int controlID, int keyFrameID)
{
   if ((controlID < eControlButtonBegin) || (controlID > eControlButtonEnd))
      return;

//-- FIXING PREFIX BUG ID 3264
   const BFlashProtoKeyFrame* pKeyframe = getProtoKeyFrame(keyFrameID);
//--
   BDEBUG_ASSERT(pKeyframe);
   if (!pKeyframe)
      return;

   int buttonIndex = controlID-eControlButtonBegin;

   debugRangeCheck(buttonIndex, eControlButtonCount);

   if (mButtonState[buttonIndex].mFrame != keyFrameID)
   {
      mButtonState[buttonIndex].mFrame=keyFrameID;

      GFxValue values[2];
      values[0].SetNumber((double)buttonIndex);
      values[1].SetString(pKeyframe->mName.c_str());      
            
      mpMovie->invokeActionScript(getProtoASFunctionName(eHUDASFunctionSetButtonState), values, 2); 
   }
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashHUD::setDPadButtonIcon(int controlID, int iconID, int frameID)
{
   if (!mpUser->getHUDItemEnabled(BUser::cHUDItemDpadHelp))
      return;

   if ((controlID < eControlDPadButtonBegin) || (controlID > eControlDPadButtonEnd))
      return;
  
   int buttonIndex = controlID-eControlDPadButtonBegin;
   debugRangeCheck(buttonIndex, eControlDPadButtonCount);

   if (mDPadIconState[buttonIndex].mID == iconID &&
       mDPadIconState[buttonIndex].mFrame == frameID)
       return;
   
//-- FIXING PREFIX BUG ID 3266
   const BFlashProtoKeyFrame* pKeyframe = getProtoKeyFrame(frameID);
//--
   BDEBUG_ASSERT(pKeyframe);
   if (!pKeyframe)
      return;
   
   GFxValue values[4];
//-- FIXING PREFIX BUG ID 3267
   const BFlashProtoIcon* pIcon = getProtoIcon(iconID);
//--
   values[0].SetNumber(buttonIndex);
   if (pIcon && !pIcon->mTexture.isEmpty())
   {            
      values[1].SetString(pIcon->mTexture.c_str());      
   }
   else
   {
      values[1].SetString(mPlaceholderIcon.c_str());
   }

   values[2].SetString(pKeyframe->mName.c_str());   
   values[3].SetBoolean(frameID == eHUDKeyframeOff ? false : true);
   mpMovie->invokeActionScript(getProtoASFunctionName(eHUDASFunctionSetDPadButtonIcon), values, 4);

   mDPadIconState[buttonIndex].mID = iconID;
   mDPadIconState[buttonIndex].mFrame = frameID;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int BFlashHUD::getInputFunctionIconID(int inputFunctionID)
{
   int iconID = -1;
   for (int i = 0; i < mInputFunctionIconLookup.getNumber(); ++i)
   {
      if (mInputFunctionIconLookup[i].mProtoID == inputFunctionID)
      {
         iconID = mInputFunctionIconLookup[i].mIndex;                  
         break;
      }
   }
   return iconID;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashHUD::setUnitSelectionVisible(bool bVisible)
{
   if (bVisible == mUnitSelectionVisible)
      return;

   mUnitSelectionVisible = bVisible;

   if (!mUnitSelectionVisible)
   {
      for (int i = 0; i < eUnitSelectionSlotCount; i++)
      {
         mUnitSlotID[i].clear();
         mUnitSlotID[i].mDirty = true;
         updateUnitSelectionDisplay(-1, false);
      }      
   }   
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashHUD::setUnitCardVisible(bool bVisible)
{
   if (bVisible == mUnitCardVisible)
      return;

   mUnitCardVisible = bVisible;

   if (!mUnitCardVisible)
   {
      mUnitCard.clear();
      mUnitCard.mDirty = true;
      updateUnitCardDisplay();      
   }
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashHUD::setUnitSelection(int slotID, int frameID, BEntityID squadID, int protoSquadID, int protoObjID, bool bTagged, int abilityFrameID, int count, const BUString& detailStr, const BUString& nameStr, const BUString& roleStr, bool bShowGamerTag, const BUString& gamerTagStr, DWORD color)
{   
   BSquad* pSquad = gWorld->getSquad(squadID);

   int iconID = -1;
   int ownerID = -1;
   int abilityIconID = -1;

   int vetTechLevel = 0;
   int abilityRecoverPercent = 0;
   if (pSquad)
   {
      vetTechLevel = pSquad->getTechPlusVetLevel();
      abilityRecoverPercent = Math::FloatToIntRound(pSquad->getRecoverPercent() * 100.0f);
   }


   if (protoSquadID!=-1)
   {    
//-- FIXING PREFIX BUG ID 3268
      const BProtoSquad* pProto = (pSquad ? pSquad->getPlayer()->getProtoSquad(protoSquadID) : gDatabase.getGenericProtoSquad(protoSquadID));
//--
      if (pProto)
      {
         iconID = pProto->getCircleMenuIconID();
         abilityIconID = pProto->getCircleMenuIconID();      
         ownerID = protoSquadID;

         if (pProto->getAltCircleMenuIconID() != -1)
         {
            if (pSquad)
            {
               int squadMode = pSquad->getSquadMode();
               if (squadMode == BSquadAI::cModeLockdown || squadMode == BSquadAI::cModeAbility)
                  iconID = pSquad->getProtoSquad()->getAltCircleMenuIconID();
            }
         }
      }
   }
   else if (protoObjID != -1)
   {
//-- FIXING PREFIX BUG ID 3269
      const BProtoObject* pProto = (pSquad ? pSquad->getPlayer()->getProtoObject(protoObjID) : gDatabase.getGenericProtoObject(protoObjID));
//--
      if (pProto)
      {
         iconID = pProto->getCircleMenuIconID();
         abilityIconID = pProto->getCircleMenuIconID();
         ownerID = protoObjID;
      }
   }
   else if (protoObjID == -1 && protoSquadID == -1)
   {
      frameID = eHUDKeyframeOff;
   }

   bool bVisible = frameID == eHUDKeyframeOff ? false : true;
        
   if ((mUnitSlotID[slotID].mOwnerID == ownerID)  &&
       (mUnitSlotID[slotID].mIconID  == iconID)   &&
       (mUnitSlotID[slotID].mAbilityIconID  == abilityIconID)   &&
       (mUnitSlotID[slotID].mFrame   == frameID)  &&
       (mUnitSlotID[slotID].mTagged  == bTagged)  &&
       (mUnitSlotID[slotID].mVisible == bVisible) &&
       (mUnitSlotID[slotID].mAbilityStateID == abilityFrameID)&&
       (mUnitSlotID[slotID].mShowGamerTag == bShowGamerTag)&&
       (mUnitSlotID[slotID].mColor == color) &&
       (mUnitSlotID[slotID].mCount == count) &&
       (mUnitSlotID[slotID].mVetTechLevel == vetTechLevel) &&
       (mUnitSlotID[slotID].mAbilityRechargePct == abilityRecoverPercent)              
       )
      return;
   
   //-- if we are trying to turn something on then cache the new values.
   if (bVisible)
   {
      mUnitSlotID[slotID].mDirty = true;
      mUnitSlotID[slotID].mOwnerID = ownerID;
      mUnitSlotID[slotID].mIconID  = iconID;
      mUnitSlotID[slotID].mAbilityIconID = abilityIconID;
      mUnitSlotID[slotID].mFrame   = frameID;
      mUnitSlotID[slotID].mVisible = bVisible;
      mUnitSlotID[slotID].mTagged  = bTagged;      
      mUnitSlotID[slotID].mDetailStr = detailStr;
      mUnitSlotID[slotID].mNameStr = nameStr;
      mUnitSlotID[slotID].mRoleStr = roleStr;
      mUnitSlotID[slotID].mGamerTagStr = gamerTagStr;
      mUnitSlotID[slotID].mAbilityStateID = abilityFrameID;
      mUnitSlotID[slotID].mShowGamerTag = bShowGamerTag;
      mUnitSlotID[slotID].mColor = color;
      mUnitSlotID[slotID].mCount = count;
      mUnitSlotID[slotID].mCountStr.locFormat(L"%u", count);
      mUnitSlotID[slotID].mVetTechLevel = vetTechLevel;
      mUnitSlotID[slotID].mVetTechLevelStr.locFormat(L"%u", vetTechLevel);
      mUnitSlotID[slotID].mAbilityRechargePct = abilityRecoverPercent;
   }
   else
   {      
      //-- clear out the slot's cache
      mUnitSlotID[slotID].clear();
      mUnitSlotID[slotID].mFrame = eHUDKeyframeOff;
      mUnitSlotID[slotID].mAbilityStateID = eHUDKeyframeOff;
      mUnitSlotID[slotID].mDirty = true;
   }
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashHUD::setUnitCard(int slotID, int frameID, int playerID, BEntityID squadID, int protoSquadID, int protoObjID, int abilityID, bool bTagged, int abilityFrameID, int count, const BUString& detailStr, const BUString& nameStr, const BUString& roleStr, bool bShowGamerTag, const BUString& gamerTagStr, DWORD color)
{   
   int iconID = -1;
   int ownerID = -1;
   int abilityIconID = -1;
   
//-- FIXING PREFIX BUG ID 3275
   const BSquad* pSquad = gWorld->getSquad(squadID);
//--

   int vetTechLevel = 0;
   int abilityRecoverPercent = 0;
   if (pSquad)
   {
      vetTechLevel = pSquad->getTechPlusVetLevel();
      abilityRecoverPercent = Math::FloatToIntRound(pSquad->getRecoverPercent() * 100.0f);
   }

   if (protoSquadID!=-1)
   {    
      BPlayer* pPlayer = gWorld->getPlayer(playerID);
//-- FIXING PREFIX BUG ID 3271
      const BProtoSquad* pProto = (pPlayer ? pPlayer->getProtoSquad(protoSquadID) : gDatabase.getGenericProtoSquad(protoSquadID));
//--
      if (pProto)
      {
         iconID = pProto->getCircleMenuIconID();
         ownerID = protoSquadID;
         if (pProto->getAltCircleMenuIconID() != -1)
         {            
            if (pSquad)
            {
               int squadMode = pSquad->getSquadMode();
               if (squadMode == BSquadAI::cModeLockdown || squadMode == BSquadAI::cModeAbility)
                  iconID = pSquad->getProtoSquad()->getAltCircleMenuIconID();
            }
         }
      }
   }
   else if (protoObjID != -1)
   {
      BPlayer* pPlayer = gWorld->getPlayer(playerID);
//-- FIXING PREFIX BUG ID 3272
      const BProtoObject* pProto = (pPlayer ? pPlayer->getProtoObject(protoObjID) : gDatabase.getGenericProtoObject(protoObjID));
//--
      if (pProto)
      {
         iconID = pProto->getCircleMenuIconID();
         ownerID = protoObjID;
      }
   }
   else if (protoObjID == -1 && protoSquadID == -1)
   {
      frameID = eHUDKeyframeOff;
   }

   if (abilityID!=-1)
   {
//-- FIXING PREFIX BUG ID 3273
      const BAbility* pAbility = gDatabase.getAbilityFromID(abilityID);
//--
      if (pAbility)
         abilityIconID = pAbility->getCircleMenuIconID();
   }

   bool bVisible = frameID == eHUDKeyframeOff ? false : true;
        
   if ((mUnitCard.mOwnerID == ownerID)  &&
       (mUnitCard.mIconID  == iconID)   &&
       (mUnitCard.mAbilityIconID  == abilityIconID)   &&
       (mUnitCard.mFrame   == frameID)  &&
       (mUnitCard.mTagged  == bTagged)  &&
       (mUnitCard.mVisible == bVisible) &&
       (mUnitCard.mAbilityStateID == abilityFrameID)&&
       (mUnitCard.mShowGamerTag == bShowGamerTag)&&
       (mUnitCard.mColor == color) &&
       (mUnitCard.mCount == count) &&
       (mUnitCard.mVetTechLevel == vetTechLevel) &&
       (mUnitCard.mAbilityRechargePct == abilityRecoverPercent) &&
       (mUnitCard.mNameStr.compare(nameStr) == 0))
      return;
   
   //-- if we are trying to turn something on then cache the new values.
   if (bVisible)
   {
      mUnitCard.mDirty = true;
      mUnitCard.mOwnerID = ownerID;
      mUnitCard.mIconID  = iconID;
      mUnitCard.mAbilityIconID = abilityIconID;
      mUnitCard.mFrame   = frameID;
      mUnitCard.mVisible = bVisible;
      mUnitCard.mTagged  = bTagged;      
      mUnitCard.mCount = count;
      mUnitCard.mCountStr.locFormat(L"%u", count);      
      mUnitCard.mDetailStr = detailStr;
      mUnitCard.mNameStr = nameStr;
      mUnitCard.mRoleStr = roleStr;
      mUnitCard.mGamerTagStr = gamerTagStr;
      mUnitCard.mAbilityStateID = abilityFrameID;
      mUnitCard.mShowGamerTag = bShowGamerTag;
      mUnitCard.mColor = color;
      mUnitCard.mVetTechLevel = vetTechLevel;
      mUnitCard.mVetTechLevelStr.locFormat(L"%u", vetTechLevel);
      mUnitCard.mAbilityRechargePct = abilityRecoverPercent;
   }
   else
   {      
      //-- clear out the slot's cache
      mUnitCard.clear();
      mUnitCard.mFrame = eHUDKeyframeOff;
      mUnitCard.mAbilityStateID = eHUDKeyframeOff;
      mUnitCard.mDirty = true;
   }
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashHUD::updateUnitSelectionDisplay(int subSelectIndex, bool bDisplayArrows )
{
   GFxValue icon[eUnitSelectionSlotCount];
   GFxValue frame[eUnitSelectionSlotCount];
   GFxValue visible[eUnitSelectionSlotCount];
   GFxValue countStr[eUnitSelectionSlotCount];
   GFxValue nameStr[eUnitSelectionSlotCount];
   GFxValue roleStr[eUnitSelectionSlotCount];
   GFxValue gamerTagStr[eUnitSelectionSlotCount];
   GFxValue tagged[eUnitSelectionSlotCount];
   GFxValue dirty[eUnitSelectionSlotCount];
   GFxValue abilityState[eUnitSelectionSlotCount];
   GFxValue showGamerTag[eUnitSelectionSlotCount];
   GFxValue color[eUnitSelectionSlotCount];
   GFxValue vetVisible[eUnitSelectionSlotCount];
   GFxValue vetTechLevelStr[eUnitSelectionSlotCount];
   GFxValue abilityRechargePct[eUnitSelectionSlotCount];   

   bool bIsAnyDirty = false;      
   for (int i = 0; i < eUnitSelectionSlotCount; ++i)
   {  
//-- FIXING PREFIX BUG ID 3276
      const BFlashProtoIcon* pIcon = getProtoIcon(mUnitSlotID[i].mIconID);
//--
      if (pIcon && !pIcon->mTexture.isEmpty())
      {
         icon[i].SetString(pIcon->mTexture.c_str());
      }
      else
      {
         icon[i].SetString(mPlaceholderIcon.c_str());
      }

//-- FIXING PREFIX BUG ID 3277
      const BFlashProtoKeyFrame* pKeyframe = getProtoKeyFrame(mUnitSlotID[i].mFrame);
//--
      if (pKeyframe)
      {
         frame[i].SetString(pKeyframe->mName.c_str());
      }

      tagged[i].SetBoolean(mUnitSlotID[i].mTagged);
      visible[i].SetBoolean(mUnitSlotID[i].mVisible);

//-- FIXING PREFIX BUG ID 3278
      const BFlashProtoKeyFrame* pAbilityStateKeyFrame = getProtoKeyFrame(mUnitSlotID[i].mAbilityStateID);
//--
      if (pAbilityStateKeyFrame)
         abilityState[i].SetString(pAbilityStateKeyFrame->mName.c_str());

      showGamerTag[i].SetBoolean(mUnitSlotID[i].mShowGamerTag);
      countStr[i].SetStringW(mUnitSlotID[i].mCountStr.getPtr());
      nameStr[i].SetStringW(mUnitSlotID[i].mNameStr.getPtr());
      roleStr[i].SetStringW(mUnitSlotID[i].mRoleStr.getPtr());
      gamerTagStr[i].SetStringW(mUnitSlotID[i].mGamerTagStr.getPtr());
      color[i].SetNumber(mUnitSlotID[i].mColor);

      vetVisible[i].SetBoolean(mUnitSlotID[i].mVetTechLevel > 0);
      vetTechLevelStr[i].SetStringW(mUnitSlotID[i].mVetTechLevelStr.getPtr());
      abilityRechargePct[i].SetNumber((double)mUnitSlotID[i].mAbilityRechargePct);
      
      if (mUnitSlotID[i].mDirty)
         bIsAnyDirty = true;

      dirty[i].SetBoolean(mUnitSlotID[i].mDirty);
      mUnitSlotID[i].mDirty = false;
   }

   if (bIsAnyDirty)
   {
      mpMovie->setVariableArray("_global.myScene.mUnitSlotIconID", icon, (int) eUnitSelectionSlotCount);
      mpMovie->setVariableArray("_global.myScene.mUnitSlotFrameID", frame, (int) eUnitSelectionSlotCount);
      mpMovie->setVariableArray("_global.myScene.mUnitSlotVisible", visible, (int) eUnitSelectionSlotCount);
      mpMovie->setVariableArray("_global.myScene.mUnitSlotTagged", tagged, (int) eUnitSelectionSlotCount);
      mpMovie->setVariableArray("_global.myScene.mUnitSlotCountStr", countStr, (int) eUnitSelectionSlotCount);
      mpMovie->setVariableArray("_global.myScene.mUnitSlotDirty", dirty, (int) eUnitSelectionSlotCount);
      mpMovie->setVariableArray("_global.myScene.mUnitSlotAbilityState", abilityState, (int) eUnitSelectionSlotCount);

      mpMovie->setVariableArray("_global.myScene.mUnitSlotNameStr", nameStr, (int) eUnitSelectionSlotCount);
      mpMovie->setVariableArray("_global.myScene.mUnitSlotRoleStr", roleStr, (int) eUnitSelectionSlotCount);
      mpMovie->setVariableArray("_global.myScene.mUnitSlotPlayerStr", gamerTagStr, (int) eUnitSelectionSlotCount);
      mpMovie->setVariableArray("_global.myScene.mUnitSlotShowGamerTag", showGamerTag, (int) eUnitSelectionSlotCount);
      mpMovie->setVariableArray("_global.myScene.mUnitSlotColor", color, (int) eUnitSelectionSlotCount);

      mpMovie->setVariableArray("_global.myScene.mUnitSlotVetVisible", vetVisible, (int) eUnitSelectionSlotCount);
      mpMovie->setVariableArray("_global.myScene.mUnitSlotVetTechLevelStr", vetTechLevelStr, (int) eUnitSelectionSlotCount);
      mpMovie->setVariableArray("_global.myScene.mUnitSlotAbilityRechargePct", abilityRechargePct, (int) eUnitSelectionSlotCount);
      
      GFxValue value[2];
      value[0].SetNumber(subSelectIndex);
      value[1].SetBoolean(bDisplayArrows);
      mpMovie->invokeActionScript("updateUnitSelection", value, 2);      
   }   
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashHUD::updateUnitCardDisplay()
{
   GFxValue value[14];
//-- FIXING PREFIX BUG ID 3279
   const BFlashProtoIcon* pIcon = getProtoIcon(mUnitCard.mIconID);
//--
   if (pIcon && !pIcon->mTexture.isEmpty())
   {
      value[0].SetString(pIcon->mTexture.c_str());
   }
   else
   {
      value[0].SetString(mPlaceholderIcon.c_str());
   }

//-- FIXING PREFIX BUG ID 3280
   const BFlashProtoKeyFrame* pKeyframe = getProtoKeyFrame(mUnitCard.mFrame);
//--
   if (pKeyframe)
   {
      value[1].SetString(pKeyframe->mName.c_str());
   }

//-- FIXING PREFIX BUG ID 3281
   const BFlashProtoIcon* pAbilityIcon = getProtoIcon(mUnitCard.mAbilityIconID);
//--
   if (pAbilityIcon && !pAbilityIcon->mTexture.isEmpty())
   {
      value[2].SetString(pAbilityIcon->mTexture.c_str());
   }
   else
   {
      value[2].SetString(mPlaceholderIcon.c_str());
   }
   
   value[3].SetBoolean(mUnitCard.mVisible);

//-- FIXING PREFIX BUG ID 3282
   const BFlashProtoKeyFrame* pAbilityStateKeyFrame = getProtoKeyFrame(mUnitCard.mAbilityStateID);
//--
   if (pAbilityStateKeyFrame)
      value[4].SetString(pAbilityStateKeyFrame->mName.c_str());

   value[5].SetStringW(mUnitCard.mCountStr.getPtr());
   value[6].SetStringW(mUnitCard.mNameStr.getPtr());
   value[7].SetStringW(mUnitCard.mRoleStr.getPtr());
   value[8].SetBoolean(mUnitCard.mShowGamerTag);
   value[9].SetStringW(mUnitCard.mGamerTagStr.getPtr());  
   value[10].SetNumber(mUnitCard.mColor);
   value[11].SetStringW(mUnitCard.mVetTechLevelStr.getPtr());
   value[12].SetNumber((double) mUnitCard.mAbilityRechargePct);
   value[13].SetBoolean(mUnitCard.mVetTechLevel > 0);
   
   if (mUnitCard.mDirty)
   {
      mUnitCard.mDirty=false;
      mpMovie->invokeActionScript("updateUnitCard", value, 14);
   }
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashHUD::setGameTimeVisible(bool bVisible)
{
   if (bVisible == mGameTimeVisible)
      return;

   mGameTimeVisible = bVisible;
   if (mGameTimeVisible)
      mpMovie->invokeActionScript(getProtoASFunctionName(eHUDASFunctionSetGameTimeVisible), "%s", getProtoKeyFrameName(eHUDKeyframeOn));
   else
   {
      mpMovie->invokeActionScript(getProtoASFunctionName(eHUDASFunctionSetGameTimeVisible), "%s", getProtoKeyFrameName(eHUDKeyframeOff));
   }
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashHUD::setResourcePanelVisible(bool bVisible)
{
   if (bVisible == mResourcePanelVisible)
      return;

   mResourcePanelVisible = bVisible;  

   int initresourcePanelASFunctionID = eHUDASFunctionInitResourcePanel;
   if (gGame.isSplitScreen())
   {
      if (mpUser == gUserManager.getSecondaryUser())
         initresourcePanelASFunctionID = eHUDASFunctionInitResourcePanelSplitRight;
      else
         initresourcePanelASFunctionID = eHUDASFunctionInitResourcePanelSplitLeft;
   }

   if (mResourcePanelVisible)
      mpMovie->invokeActionScript(getProtoASFunctionName(initresourcePanelASFunctionID), "%s", getProtoKeyFrameName(eHUDKeyframeEaseIn));   
   else
   {
      mpMovie->invokeActionScript(getProtoASFunctionName(initresourcePanelASFunctionID), "%s", getProtoKeyFrameName(eHUDKeyframeEaseOut));   
   }
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashHUD::setPowerPanelVisible(bool bVisible)
{
   /*
   if (bVisible == mPowerPanelVisible)
      return;

   mPowerPanelVisible = bVisible;
   if (mPowerPanelVisible)
      mpMovie->invokeActionScript(getProtoASFunctionName(eHUDASFunctionSetPowerPanelVisible), "%s", getProtoKeyFrameName(eHUDKeyframeOn));   
   else
   {
      mpMovie->invokeActionScript(getProtoASFunctionName(eHUDASFunctionSetPowerPanelVisible), "%s", getProtoKeyFrameName(eHUDKeyframeOff));   
   }
   */
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashHUD::setDPadPanelVisible(bool bVisible)
{
   if (bVisible == mDPadVisible)
      return;

   mDPadVisible = bVisible;

   if (mDPadVisible)
      mpMovie->invokeActionScript(getProtoASFunctionName(eHUDASFunctionSetDPadVisible), "%s", getProtoKeyFrameName(eHUDKeyframeEaseIn));
   else
   {
      mpMovie->invokeActionScript(getProtoASFunctionName(eHUDASFunctionSetDPadVisible), "%s", getProtoKeyFrameName(eHUDKeyframeEaseOut));
   }
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashHUD::setButtonPanelVisible(bool bVisible)
{
   if (bVisible == mButtonPanelVisible)
      return;

   mButtonPanelVisible = bVisible;

   if (mButtonPanelVisible)
      mpMovie->invokeActionScript(getProtoASFunctionName(eHUDASFunctionSetButtonPanelVisible), "%s", getProtoKeyFrameName(eHUDKeyframeEaseIn));
   else
   {
      setButtonState(BFlashHUD::eControlButtonA, BFlashHUD::eHUDKeyframeOn);
      setButtonState(BFlashHUD::eControlButtonB, BFlashHUD::eHUDKeyframeOn);
      setButtonState(BFlashHUD::eControlButtonX, BFlashHUD::eHUDKeyframeOn);
      setButtonState(BFlashHUD::eControlButtonY, BFlashHUD::eHUDKeyframeOn);
      mpMovie->invokeActionScript(getProtoASFunctionName(eHUDASFunctionSetButtonPanelVisible), "%s", getProtoKeyFrameName(eHUDKeyframeEaseOut));      
   }
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashHUD::setTechNotification(int techID, bool bVisible)
{
//-- FIXING PREFIX BUG ID 3283
   const BProtoTech* pProtoTech = gDatabase.getProtoTech(techID);
//--
   if (!pProtoTech)
      return;

   int notificationID = pProtoTech->getUINotificationID();
   if (notificationID == -1)
      return;

   //-- is already one going on?
   if (bVisible && (mTechNotificationID != -1 || mTechNotificationEndTimer > 0.0f))
      return;

//-- FIXING PREFIX BUG ID 3284
   const BFlashProtoIcon* pIcon = getProtoIcon(notificationID);
//--

   GFxValue values[2];
   values[0].SetString(pIcon->mName.c_str());

   if (bVisible)
   {
      static double duration = 5.0f;
      mTechNotificationEndTimer = gWorld->getGametimeFloat() + duration;
      mTechNotificationID = techID;

      const int loopCount = 100;      
      values[1].SetNumber((double)loopCount);         
   }
   else
   {
      values[1].SetNumber((int) -1);
      mTechNotificationEndTimer = 0.0f;
      mTechNotificationID = -1;
   }

   mpMovie->invokeActionScript(getProtoASFunctionName(eHUDASFunctionSetTechNotification), values, 2);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashHUD::setGameTime(const BUString& timeStr)
{   
   setText(eControlLabelGameTime, timeStr);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashHUD::updatePointingAt(float x, float y, bool playSound)
{ 
   mPointingAtX=x;
   mPointingAtY=y;

   long item=calcCircleIndex(x, y, mCircleCount, 0.0f);

   long closestIndex=-1;

   if(item!=-1)
   {
      long closestDist=0;
      for(uint i=0; i<mItems.getSize(); i++)
      {
         long calcOrder=mItems[i].mOrder;
         long dist1=0;
         long dist2=0;
         if(item>calcOrder)
         {
            dist1=item-calcOrder;
            dist2=calcOrder+(mCircleCount-item);

         }
         else if(item<calcOrder)
         {
            dist1=calcOrder-item;
            dist2=(mCircleCount-calcOrder)+item;
         }
         long dist=min(dist1, dist2);
         if(dist < 2 && (closestIndex==-1 || dist<closestDist))
         {
            closestIndex=i;
            closestDist=dist;
         }
      }
   }

   bool bOrderChangedOnAnItemSlot = false;
   if ((closestIndex != -1) && (closestIndex == mCurrentItem))
   {
      bOrderChangedOnAnItemSlot = (mItems[closestIndex].mOrder != mCurrentOrder);
   }

   if((closestIndex!=mCurrentItem) || bOrderChangedOnAnItemSlot)
   {  
      if (mCurrentItem!=-1 && mCurrentItem<mItems.getNumber())
      {
         //int frameID = mItems[mCurrentItem].mFrameIDNormal;
         //if (mItems[mCurrentItem].mItemType == eCircleMenuItemTypePower)
         int frameID = mItems[mCurrentItem].mUnavailable ? eHUDKeyframePending : eHUDKeyframeOn;

         setSlotIcon(mItems[mCurrentItem].mItemType, mItems[mCurrentItem].mOrder, frameID, mItems[mCurrentItem].mIconID, mItems[mCurrentItem].mOwnerProtoID, false);         

         int selectionPadFrameID = eHUDKeyframeOff;         
         if (mItems[mCurrentItem].mTrainPercent > 0.0f)
            selectionPadFrameID = eHUDKeyframeBuild;
         else if (mItems[mCurrentItem].mTrainPercent <= -1.0f)
            selectionPadFrameID = eHUDKeyframeQueued;

         setCircleMenuSelectionPad(mItems[mCurrentItem].mOrder, selectionPadFrameID);
      }

      mCurrentItem=closestIndex;      
           
      if (mCurrentItem!=-1)
      {
         setCircleMenuExtraInfoVisible(mpUser->getHUDItemEnabled(BUser::cHUDItemCircleMenuExtraInfo));

         mCurrentOrder=mItems[closestIndex].mOrder;

         if (mItems[mCurrentItem].mFree)
            setInfoPanelSlot(-2, eHUDKeyframeOff);
         else
            setInfoPanelSlot(mItems[mCurrentItem].mOrder, mItems[mCurrentItem].mUnavailable ? eHUDKeyframePending : eHUDKeyframeOn);

         //int frameID = mItems[mCurrentItem].mFrameIDHighlight;
         //if (mItems[mCurrentItem].mItemType == eCircleMenuItemTypePower)
         int frameID = mItems[mCurrentItem].mUnavailable ? eHUDKeyframeLocked : eHUDKeyframeOver;
         setSlotIcon(mItems[mCurrentItem].mItemType, mItems[mCurrentItem].mOrder, frameID, mItems[mCurrentItem].mIconID, mItems[mCurrentItem].mOwnerProtoID, false);         

         int selectionPadFrameID = mItems[mCurrentItem].mUnavailable ? eHUDKeyframeLocked : eHUDKeyframeOn;         
         setCircleMenuSelectionPad(mItems[mCurrentItem].mOrder, selectionPadFrameID);

         if(playSound)
            gUI.playRolloverSound();
      }
      else
      {
         setCircleMenuExtraInfoVisible(false);
         mCurrentOrder=-1;
         setInfoPanelSlot(-2, eHUDKeyframeOff);
      }
      
      updateInfoPanel();

      mCurrentItemTimer = 0.0f;
   }   

   /*
   if (mCurrentItem == -1)
   {      
      setCircleMenuExtraInfoVisible(false);
   }
   */
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
long BFlashHUD::calcCircleIndex(float x, float y, long indexCount, float offset)
{ 
   long index=-1;

   if(indexCount!=0 && (x!=0.0f || y!=0.0f))
   {
      float r=0.0f;
      if(x!=0.0f)
      {
         if(y!=0.0f)
         {
            if(x>0.0f)
            {
               if(y<0.0f)
                  r=cTwoPi+(float)atan(y/x);
               else
                  r=(float)atan(y/x);
            }
            else
               r=(float)atan(y/x)+cPi;
         }
         else
         {
            if(x>0.0f)
               r=0.0f;
            else
               r=cPi;
         }
      }
      else
      {
         if(y>0.0f)
            r=cPiOver2;
         else
            r=cThreePiOver2;
      }

      r = r + (cPiOver2 + (cPi / indexCount)) + offset;
      while(r<0.0f)
         r+=cTwoPi;
      while(r>=cTwoPi)
         r-=cTwoPi;
      index = (long)(r * (indexCount / cTwoPi));   
   }

   return index;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashHUD::clearCircleMenuItems()
{
   mItems.clear();   
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashHUD::clearSelection()
{
   mCurrentItem=-1;  
   mCurInfoPanelSlot=-1;
   mPointingAtX=0.0f;
   mPointingAtY=0.0f;
   clearCostPanelState();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashHUD::clearCostPanelState()
{
   for (int i = 0; i < cMaxCostPanelStats; i++)
      mCostPanelState[i].clear();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
long BFlashHUD::addCircleMenuItem(long order, long id, const BCost* pCost, const BPopArray* pPops, long trainCount, long trainLimit, float trainPercent, long techPrereqID, bool unavail, const BUString& label, const BUString& infoText, const BUString& infoText2, const BUString& infoDetail, int itemType, int iconID, int ownerProtoID, int unitStatProtoID)
{
   uint numItems = mItems.getSize();
   for (uint i = 0; i < numItems; i++)
   {
      // Remove the item with the matching order because it is about to get replaced
      if (mItems[i].mOrder == order)
      {
         mItems.erase(i);
         break;
      }
   }

   BFlashCircleMenuItem item;
   item.mOrder=order;
   item.mID=id;
   if(pCost)
      item.mCost=*pCost;
   if(pPops)
   {
      long count=pPops->getNumber();
      for(long i=0; i<count; i++)
      {
         BPop pop=pPops->get(i);
         item.mPops.add(pop);
      }
   }
   item.mTrainCount=trainCount;
   item.mTrainLimit=trainLimit;
   item.mTrainPercent=trainPercent;
   item.mTechPrereqID=techPrereqID;
   item.mInfoText=infoText;
   item.mInfoText2=infoText2;
   item.mInfoDetail=infoDetail;
   item.mInfoPercentage=label;
   item.mItemType=itemType;   
   item.mStatData.clear();
   item.mOwnerProtoID=ownerProtoID;
   item.mIconID=iconID;   
   item.mUnavailable=unavail;
   item.mUnitStatProtoID=unitStatProtoID;

   updateItemStatData(item);

   // ajl 11/19/07 - turn off unit stats display since they don't fit the new unit model
   //updateItemUnitStatData(item);
   
   long index=mItems.add(item);
   return index;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BFlashHUD::editCircleMenuItem(long order, long id, const BCost* pCost, const BPopArray* pPops, long trainCount, long trainLimit, float trainPercent, long techPrereqID, bool unavail, const BUString& label, const BUString& infoText, const BUString& infoText2, const BUString& infoDetail, int itemType, int iconID, int ownerProtoID, int unitStatProtoID)
{
   // returns true if the cost changed
   bool bCostChanged = false;
   uint numItems = mItems.getSize();
   for (uint i = 0; i < numItems; i++)
   {      
      if (mItems[i].mOrder == order)
      {
         mItems[i].mID = id;
         if(pCost)
         {
            for (long j=0; j<BCost::getNumberResources(); j++)
            {
               if (mItems[i].mCost.get(j) != pCost->get(j))
               {
                  bCostChanged = true;
                  break;
               }
            }
            mItems[i].mCost=*pCost;
         }
         if(pPops)
         {
            long count=pPops->getNumber();
            long count2=mItems[i].mPops.getNumber();
            for(long j=0; j<count; j++)
            {
               BPop pop=pPops->get(j);
               for (long k=0; k<count2; k++)
               {
                  if (mItems[i].mPops[k].mID == pop.mID)
                  {
                     mItems[i].mPops[k].mCount = pop.mCount;
                     break;
                  }
               }
               if (k == count2)
                  mItems[i].mPops.add(pop);
            }
         }
         mItems[i].mTrainCount=trainCount;
         mItems[i].mTrainLimit=trainLimit;
         mItems[i].mTrainPercent=trainPercent;
         mItems[i].mTechPrereqID=techPrereqID;
         mItems[i].mInfoText=infoText;
         mItems[i].mInfoText2=infoText2;
         mItems[i].mInfoDetail=infoDetail;
         mItems[i].mInfoPercentage=label;
         mItems[i].mItemType=itemType;   
         mItems[i].mStatData.clear();
         mItems[i].mOwnerProtoID=ownerProtoID;
         mItems[i].mIconID=iconID;
         mItems[i].mUnavailable=unavail;
         mItems[i].mUnitStatProtoID=unitStatProtoID;

         updateItemStatData(mItems[i]);
      }
   }
   return bCostChanged;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashHUD::removeCircleMenuItem(long index)
{
   mItems.erase(index);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
long BFlashHUD::getCircleMenuItemIndex(long id) const
{
   uint count=mItems.getSize();
   for(uint i=0; i<count; i++)
   {
      if(mItems[i].mID==id)
         return i;
   }
   return -1;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashHUD::refreshCircleMenu(bool bRefreshTrainProgressOnly)
{     
   if (bRefreshTrainProgressOnly)
   {
      updatePointingAt(mPointingAtX, mPointingAtY, false);
      updateCircleMenuDisplay();
      setCenterDisplayData();
      updateResearchSlotData();
   }
   else
   {
      clearCircleMenuDisplay(true);
      updatePointingAt(mPointingAtX, mPointingAtY, false);
      //trace("CurrentItem=%d", mCurrentItem);   
      updateCircleMenuDisplay();
      updateInfoPanel();         
      updateResearchSlotData();
   }
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashHUD::clearCircleMenuDisplay(bool force)
{
   for (int j = 0; j < mCircleCount; ++j)
   {  
      setSlotIcon(mCircleMenuState[j].mType, j, eHUDKeyframeOff, mCircleMenuState[j].mIconID, mCircleMenuState[j].mOwnerID, force);   
      setCircleMenuSelectionPad(j, eHUDKeyframeOff);
   }
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashHUD::updateCircleMenuDisplay()
{      
   int itemType;
   int frameID; 
   int ownerID; 
   int slotID; 
   int iconID;  
   int buildPanelFrameID;
   for (int i = 0; i < mItems.getNumber(); ++i)
   {
      itemType = mItems[i].mItemType;      
      frameID  = mItems[i].mUnavailable ? eHUDKeyframePending : eHUDKeyframeOn;
      ownerID  = mItems[i].mOwnerProtoID;
      slotID   = mItems[i].mOrder;
      iconID   = mItems[i].mIconID;
      buildPanelFrameID = eHUDKeyframeOff;

      if (mCurrentItem == i)
      {         
         frameID = mItems[i].mUnavailable ? eHUDKeyframeLocked : eHUDKeyframeOver;
         buildPanelFrameID = mItems[i].mUnavailable ? eHUDKeyframeLocked : eHUDKeyframeOn;
      }
      else if (mItems[i].mTrainPercent > 0.0f)
      {
         buildPanelFrameID = eHUDKeyframeBuild;
      }
      else if (mItems[i].mTrainPercent <= -1.0f)
      {
         buildPanelFrameID = eHUDKeyframeQueued;
      }
      
      setSlotIcon(itemType, slotID, frameID, iconID, ownerID, false);

      setCircleMenuSelectionPad(slotID, buildPanelFrameID);
   }
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashHUD::setSlotIcon(int itemType, int slotID, int frameID, int iconID, int ownerID, bool bForceToFrame)
{
   if (ownerID == -1)
      frameID = eHUDKeyframeOff;   

   if ((itemType == eCircleMenuItemTypeMisc) && (ownerID != -1) && (iconID == -1))
   {
      for (int i = 0; i < mMiscIconLookup.getNumber(); ++i)
      {
         if (mMiscIconLookup[i].mProtoID == ownerID)
         {
            iconID = mMiscIconLookup[i].mIndex;                  
            break;
         }
      }
   }

   if (!bForceToFrame)
   {
      if ((mCircleMenuState[slotID].mType   == itemType) &&  
          (mCircleMenuState[slotID].mIconID == iconID)   &&
          (mCircleMenuState[slotID].mFrame  == frameID))
      {
         return;
      }
   }  
   
   setCircleMenuSlotIcon(slotID, frameID, iconID);
   
   mCircleMenuState[slotID].mType    = itemType;
   mCircleMenuState[slotID].mFrame   = frameID;
   mCircleMenuState[slotID].mOwnerID = ownerID;
   mCircleMenuState[slotID].mIconID  = iconID;

   //trace("Slot[%d] Type=%d Frame=%d Owner=%d", slotID, itemType, frameID, ownerID); 
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashHUD::setSlotFrame(int itemType, int slotID, int frameID, int iconID, int ownerID, bool bForceToFrame)
{
   if (!bForceToFrame)
   {
      if ((mCircleMenuState[slotID].mType  == itemType) &&  
          (mCircleMenuState[slotID].mIconID == iconID) &&
          (mCircleMenuState[slotID].mFrame == frameID))
      {
         //trace("Skipped!");
         return;
      }
   }
      
   switch (itemType)
   {
      case eCircleMenuItemTypeUnit:
         {
            if (ownerID == -1)
               setCircleMenuIcon(getProtoASFunctionName(eHUDASFunctionSetCircleMenuUnitSlot), slotID, "", eHUDKeyframeOff);
            else
            {
               if (iconID != -1)
               {
//-- FIXING PREFIX BUG ID 3232
                  const BFlashProtoIcon* pIcon = getProtoIcon(iconID);
//--
                  if (pIcon)
                     setCircleMenuIcon(getProtoASFunctionName(eHUDASFunctionSetCircleMenuUnitSlot), slotID, pIcon->mName.c_str(), frameID);
               }
               else // search for it
               {
                  setCircleMenuIcon(getProtoASFunctionName(eHUDASFunctionSetCircleMenuUnitSlot), slotID, "placeholder", frameID);                  
               }
            }
         }
         
         break;
      case eCircleMenuItemTypeBuilding:
         {
            if (ownerID == -1)
               setCircleMenuIcon(getProtoASFunctionName(eHUDASFunctionSetCircleMenuBuildingSlot), slotID, "", eHUDKeyframeOff);
            else
            {
               if (iconID != -1)
               {
//-- FIXING PREFIX BUG ID 3233
                  const BFlashProtoIcon* pIcon = getProtoIcon(iconID);
//--
                  if (pIcon)
                  {
                     setCircleMenuIcon(getProtoASFunctionName(eHUDASFunctionSetCircleMenuBuildingSlot), slotID, pIcon->mName.c_str(), frameID);
                     //trace("SetBuildingIcon slot=%d iconID=%d name=%s frame=%d", slotID, iconID, pIcon->mName.c_str(), frameID);
                  }
               }
               else // search for it
               {
                  setCircleMenuIcon(getProtoASFunctionName(eHUDASFunctionSetCircleMenuBuildingSlot), slotID, "placeholder", frameID);               
               }
            }
         }         
         break;
      case eCircleMenuItemTypePower:
         {            
            if (ownerID == -1)
               setCircleMenuIcon(getProtoASFunctionName(eHUDASFunctionSetCircleMenuPowerSlot), slotID, "", eHUDKeyframeOff);
            else
            {
               if (iconID != -1)
               {
//-- FIXING PREFIX BUG ID 3234
                  const BFlashProtoIcon* pIcon = getProtoIcon(iconID);
//--
                  if (pIcon)
                     setCircleMenuIcon(getProtoASFunctionName(eHUDASFunctionSetCircleMenuPowerSlot), slotID, pIcon->mName.c_str(), frameID);
               }
               else // search for it
               {
                  setCircleMenuIcon(getProtoASFunctionName(eHUDASFunctionSetCircleMenuPowerSlot), slotID, "placeholder", frameID);                  
               }
            }
         }
         break;
      case eCircleMenuItemTypeTech:         
         {
            if (ownerID == -1)
               setCircleMenuIcon(getProtoASFunctionName(eHUDASFunctionSetCircleMenuTechSlot), slotID, "", eHUDKeyframeOff);
            else
            {
               if (iconID != -1)
               {
//-- FIXING PREFIX BUG ID 3235
                  const BFlashProtoIcon* pIcon = getProtoIcon(iconID);
//--
                  if (pIcon)
                  {
                     setCircleMenuIcon(getProtoASFunctionName(eHUDASFunctionSetCircleMenuTechSlot), slotID, pIcon->mName.c_str(), frameID);
                     //trace("SetTechIcon slot=%d iconID = %d name=%s frame=%d", slotID, iconID, pIcon->mName.c_str(), frameID);
                  }
               }
               else // search for it
               {
                  setCircleMenuIcon(getProtoASFunctionName(eHUDASFunctionSetCircleMenuTechSlot), slotID, "placeholder", frameID);                  
               }
            }
         }
         break;
      case eCircleMenuItemTypeMisc:
         {
            if (iconID != -1)
            {
//-- FIXING PREFIX BUG ID 3236
               const BFlashProtoIcon* pIcon = getProtoIcon(iconID);
//--
               if (pIcon)
                  setCircleMenuIcon(getProtoASFunctionName(eHUDASFunctionSetCircleMenuMiscSlot), slotID, pIcon->mName.c_str(), frameID);
            }
            else
            {
               for (int i = 0; i < mMiscIconLookup.getNumber(); ++i)
               {
                  if (mMiscIconLookup[i].mProtoID == ownerID)
                  {
//-- FIXING PREFIX BUG ID 3237
                     const BFlashProtoIcon* pIcon = getProtoIcon(mMiscIconLookup[i].mIndex);
//--
                     if (pIcon)
                     {
                        setCircleMenuIcon(getProtoASFunctionName(eHUDASFunctionSetCircleMenuMiscSlot), slotID, pIcon->mName.c_str(), frameID);
                        break;
                     }                  
                  }
               }
            }
         }
         break;
      case eCircleMenuItemTypeAbility:
         {            
            if (ownerID == -1)
               setCircleMenuIcon(getProtoASFunctionName(eHUDASFunctionSetCircleMenuAbilitySlot), slotID, "", eHUDKeyframeOff);
            else
            {
               if (iconID != -1)
               {
//-- FIXING PREFIX BUG ID 3238
                  const BFlashProtoIcon* pIcon = getProtoIcon(iconID);
//--
                  if (pIcon)
                     setCircleMenuIcon(getProtoASFunctionName(eHUDASFunctionSetCircleMenuAbilitySlot), slotID, pIcon->mName.c_str(), frameID);
               }
               else // search for it
               {
                  setCircleMenuIcon(getProtoASFunctionName(eHUDASFunctionSetCircleMenuAbilitySlot), slotID, "placeholder", frameID);                  
               }
            }
         }
         break;
      case eCircleMenuItemTypeMode:
         break;
   }   

   mCircleMenuState[slotID].mType = itemType;
   mCircleMenuState[slotID].mFrame = frameID;
   mCircleMenuState[slotID].mOwnerID = ownerID;
   mCircleMenuState[slotID].mIconID = iconID;

   //trace("Slot[%d] Type=%d Frame=%d Owner=%d", slotID, itemType, frameID, ownerID); 
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashHUD::setCircleMenuSelectionPad(int slotID, int frameID)
{
//-- FIXING PREFIX BUG ID 3239
   const BFlashProtoKeyFrame* pKeyframe = getProtoKeyFrame(frameID);
//--
   BDEBUG_ASSERT(pKeyframe);
   if (!pKeyframe)
      return;

   if (mCircleMenuState[slotID].mBuildPanelFrameID == frameID)
      return;

   mCircleMenuState[slotID].mBuildPanelFrameID = frameID;

   GFxValue value[2];
   value[0].SetNumber((double)slotID);
   value[1].SetString(pKeyframe->mName.c_str());
   
   mpMovie->invokeActionScript(getProtoASFunctionName(eHUDASFunctionSetBuildPanelSlot), value, 2);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashHUD::setCircleMenuIcon(const char* method, int slotID, int frame)
{
   GFxValue values[2];
   values[0].SetNumber((double)slotID);
   values[1].SetNumber((double)frame);
   mpMovie->invokeActionScript(method, values, 2);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashHUD::setCircleMenuIcon(const char* method, int slotID, const char* iconName, int frameID)
{
//-- FIXING PREFIX BUG ID 3241
   const BFlashProtoKeyFrame* pKeyframe = getProtoKeyFrame(frameID);
//--
   BDEBUG_ASSERT(pKeyframe);
   if (!pKeyframe)
      return;

   GFxValue values[4];
   values[0].SetNumber((double)slotID);
   values[1].SetString(iconName);
   values[2].SetString(pKeyframe->mName.c_str());
   values[3].SetBoolean(frameID == eHUDKeyframeOff ? false : true);
   mpMovie->invokeActionScript(method, values, 4);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashHUD::setCircleMenuSlotIcon(int slotID, int frameID, int iconID)
{
   debugRangeCheck(slotID,  8);
   debugRangeCheck(frameID, getProtoKeyFrameCount());
   
//-- FIXING PREFIX BUG ID 3243
   const BFlashProtoKeyFrame* pKeyframe = getProtoKeyFrame(frameID);
//--
   BDEBUG_ASSERT(pKeyframe);
   if (!pKeyframe)
      return;

   const BFlashProperties* pProps = getProperties();
   if (!pProps)
      return;


   GFxValue values[4];
   values[0].SetNumber((double)slotID);      
//-- FIXING PREFIX BUG ID 3244
   const BFlashProtoIcon* pIcon = getProtoIcon(iconID);
//--
   if (pIcon && !pIcon->mTexture.isEmpty())
   {            
      values[1].SetString(pIcon->mTexture.c_str());      
   }
   else
   {
      values[1].SetString(mPlaceholderIcon.c_str());
   }

   values[2].SetString(pKeyframe->mName.c_str());   
   values[3].SetBoolean(frameID == eHUDKeyframeOff ? false : true);
   mpMovie->invokeActionScript(getProtoASFunctionName(eHUDASFunctionSetCircleMenuSlotIcon), values, 4);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashHUD::setCircleMenuVisible(bool bVisible, int type)
{
   if (bVisible == mCircleMenuVisible)
      return;
   
   mCircleMenuVisible = bVisible;

   if (mCircleMenuVisible)
   {
      mpMovie->invokeActionScript(getProtoASFunctionName(eHUDASFunctionSetCircleMenuVisible), "%s", getProtoKeyFrameName(eHUDKeyframeEaseIn));
      if (type == eMenuGod)
         mpMovie->invokeActionScript(getProtoASFunctionName(eHUDASFunctionSetCircleMenuMode), "%s", getProtoKeyFrameName(eHUDKeyframeGod));
      else
         mpMovie->invokeActionScript(getProtoASFunctionName(eHUDASFunctionSetCircleMenuMode), "%s", getProtoKeyFrameName(eHUDKeyframeNormal));      
   }
   else
   {
      setCircleMenuExtraInfoVisible(false);
      mpMovie->invokeActionScript(getProtoASFunctionName(eHUDASFunctionSetCircleMenuVisible), "%s", getProtoKeyFrameName(eHUDKeyframeEaseOut));
      hideAllInfoPanels();  
      clearCircleMenuDisplay(true);      
      clearCircleMenuItems();
      clearSelection();      
   }
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashHUD::hideAllInfoPanels()
{   
   setInfoPanelSlot(-1, eHUDKeyframeOff);   
   mCurInfoPanelSlot=-1;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashHUD::updateInfoPanel()
{   
   setCenterDisplayData();
   setInfoPanelDescription();   
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashHUD::setInfoPanelSlot(int slotID, int selectorFrameID)
{   
   if (slotID == mCurInfoPanelSlot)
      return;

   mCurInfoPanelSlot = slotID;

   GFxValue value[2];
   value[0].SetNumber((double)slotID);      
   value[1].SetString(getProtoKeyFrameName(selectorFrameID));
   mpMovie->invokeActionScript(getProtoASFunctionName(eHUDASFunctionSetInfoPanelSlot), value, 2);      

   //trace("SetInfoPanelSlot(%d)", slotID);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashHUD::updateResearchSlotData()
{
   static bool bTestUpdate = true;
   if (!bTestUpdate)
      return;

   GFxValue values[3];
   for (int i = 0; i < mCircleCount; i++)
   {
      mCircleMenuState[i].mIsTraining = false;
   }

   for (int j = 0; j < mItems.getNumber(); ++j)
   {
      int slotID = mItems[j].mOrder;
      values[0].SetNumber((double)slotID);
      
      if (mItems[j].mTrainPercent >= cFloatCompareEpsilon || !mItems[j].mInfoPercentage.isEmpty())
      {
         //-- set the title bar text                        
         values[1].SetNumber((double)2);
         int percent = (int) mItems[j].mTrainPercent;
         values[2].SetNumber((double)percent);
         mpMovie->invokeActionScript(getProtoASFunctionName(eHUDASFunctionSetBuildDataSlot), values, 3);         

         GFxValue pctString;
         pctString.SetStringW(mItems[j].mInfoPercentage.getPtr());
         mpMovie->setVariable(getProtoControlPath(eControlResearchPosTextStart+slotID), pctString, GFxMovie::SV_Normal);

         if (mItems[j].mTrainPercent > 0.0f && mCurrentItem != j)
            setCircleMenuSelectionPad(slotID, eHUDKeyframeBuild);

         mCircleMenuState[slotID].mIsTraining = true;
      }         
   }   

   for (int k = 0; k < mCircleCount; k++)
   {
      if (!mCircleMenuState[k].mIsTraining)
      {
         values[0].SetNumber((double)k);
         values[1].SetNumber((double)-1);      
         values[2].SetNumber((double)-1);
         mpMovie->invokeActionScript(getProtoASFunctionName(eHUDASFunctionSetBuildDataSlot), values, 3);
         mpMovie->setVariable(getProtoControlPath(eControlResearchPosTextStart+k), "", GFxMovie::SV_Normal);
      }
   }
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashHUD::updateItemStatData(BFlashCircleMenuItem& item)
{
   //-- This fills out the cost values into the cost control
   //-- Activate the resource cost in the same order as they appear on the resource panel
   if (!mpUser)
      return;

   BDEBUG_ASSERT(mpUser);

//-- FIXING PREFIX BUG ID 3245
   const BPlayer* pPlayer = mpUser->getPlayer(); 
//--
   if(!pPlayer)
      return;

   long civID=pPlayer->getCivID();
   long playerStatCount=gUIGame.getNumberPlayerStats(civID);   
   
   float costAmount = 0.0f;
   int statCount = 0;
   bool bFree = true;
   for(long i=0; i<playerStatCount; i++)
   {      
      const BUIGamePlayerStat*  pStat=gUIGame.getPlayerStat(civID, i);
      if((statCount<cMaxStatItems) && 
         (pStat->mType==BUIGamePlayerStat::cTypeResource || pStat->mType==BUIGamePlayerStat::cTypePop))
      {
         item.mStatData.mStatIndex[statCount]=statCount;
         item.mStatData.mStatType[statCount]=pStat->mType;
         item.mStatData.mStatID[statCount]=pStat->mID;

         if(pStat->mType==BUIGamePlayerStat::cTypeResource)
         {
            costAmount  = item.mCost.get(pStat->mID);
            item.mStatData.mStatFrameID[statCount] = gUIGame.getResourceFlashUICostPanelFrame(pStat->mID);
         }
         else
         {
            item.mStatData.mStatFrameID[statCount] = gUIGame.getPopFlashUICostPanelFrame(pStat->mID);
            for(long j=0; j<item.mPops.getNumber(); j++)
            {
               if(item.mPops[j].mID==pStat->mID)
               {
                  costAmount = item.mPops[j].mCount;
                  break;
               }
            }            
         }

         if (costAmount > cFloatCompareEpsilon)
            bFree = false;

         item.mStatData.mStatAmount[statCount] = costAmount;
         statCount++;
      }
   }

   item.mFree = bFree;
   item.mStatData.mCount=statCount;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashHUD::updateItemUnitStatData(BFlashCircleMenuItem& item)
{
   BPlayer* pPlayer = mpUser->getPlayer(); 
   if(!pPlayer)
      return;

//-- FIXING PREFIX BUG ID 3246
   const BProtoObject* pStatsProtoObject = NULL;
//--
   BProtoSquad*  pStatProtoSquad = NULL;   
   int itemType =item.mItemType;
   int protoID = item.mUnitStatProtoID;
   if (itemType == eCircleMenuItemTypeUnit)
   {
      pStatProtoSquad = pPlayer->getProtoSquad(protoID);
      if (pStatProtoSquad == NULL)
         pStatsProtoObject = pPlayer->getProtoObject(protoID);
   }
   else if (itemType == eCircleMenuItemTypeBuilding)
      pStatsProtoObject = pPlayer->getProtoObject(protoID);
   else if (itemType == eCircleMenuItemTypeTech)
      pStatsProtoObject = pPlayer->getProtoObject(protoID);
   else
      return;

   if (pStatProtoSquad == NULL && pStatsProtoObject == NULL)
      return;
   
   uint statCount= gUIGame.getNumberUnitStats();
   uint curStat = 0;
   uint highestStatIndex = 0;
   BFixedString256 text;   
   for(uint j=0; j<statCount && curStat < cMaxUnitStatItems; j++)
   {      
      const BUIGameUnitStat* pUnitStat=gUIGame.getUnitStat(j);
      uint statValue = 0;      
      if (pUnitStat->mStatType == BUIGameUnitStat::cTypeAttackGrade)
      {
         if (pStatProtoSquad && pStatProtoSquad->getHasAttackRatings())
         {
            statValue = pStatProtoSquad->getAttackGrade((uint)pUnitStat->mStatData);
         }
         else if (pStatsProtoObject && pStatsProtoObject->getHasAttackRatings())
         {
            statValue = pStatsProtoObject->getAttackGrade((uint)pUnitStat->mStatData);
         }
         else 
            continue;

         int frameID = -1;
         for (int k = 0; k < mStatIconLookup.getNumber(); ++k)
         {
            if (mStatIconLookup[k].mProtoID == (int) j)
            {
               frameID = mStatIconLookup[k].mIndex;
               break;
            }
         }

         if (statValue > item.mUnitStatData.mStatAmount[highestStatIndex])
            highestStatIndex=curStat;
               
         item.mUnitStatData.mStatAmount[curStat] = (float) statValue;
         item.mUnitStatData.mStatFrameID[curStat] = frameID;
         item.mUnitStatData.mStatType[curStat] = pUnitStat->mStatType;
         item.mUnitStatData.mStatID[curStat] = j;
         item.mUnitStatData.mStatIndex[curStat] = -1;
         curStat++;
      }
   }

   item.mUnitStatData.mStatIndex[highestStatIndex] = 1;
   item.mUnitStatData.mCount = curStat;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashHUD::setCenterDisplayData()
{   
   if (!mpUser)
      return;

//-- FIXING PREFIX BUG ID 3248
   const BPlayer* pPlayer = mpUser->getPlayer(); 
//--
   if(!pPlayer)
      return;

   sLocalTempUString.empty();
   BFixedString256 centerCommand;
   BFixedString256 centerTextPath;
   
   GFxValue vInfoText;
   GFxValue vInfoText2;
   GFxValue vString;
   GFxValue vString2;

   int statIndex;
   if (mCurrentItem!=-1)
   {            
      if(mItems[mCurrentItem].mFree)
      {
         //-- set the movie display into a state where only the center text is visible (denoted by -2)
         setInfoPanelSlot(-2, eHUDKeyframeOff);         
         vInfoText.SetStringW(mItems[mCurrentItem].mInfoText.getPtr());
         mpMovie->setVariable(getProtoControlPath(eControlBaseTextLabel), vInfoText, GFxMovie::SV_Normal);         
         vInfoText2.SetStringW(mItems[mCurrentItem].mInfoText2.getPtr());
         mpMovie->setVariable(getProtoControlPath(eControlBaseText2Label), vInfoText2, GFxMovie::SV_Normal);
      }
      else
      {
         for (int i = 0; i < mItems[mCurrentItem].mStatData.mCount; ++i)
         {
            bool bCanAfford=true;
            bool bStatIsTrainlimit=false;

            statIndex = mItems[mCurrentItem].mStatData.mStatIndex[i];
            if (statIndex == 0)
            {
               centerTextPath = getProtoControlPath(eControlCostPanelStat1Label);
               centerCommand  = getProtoControlPath(eControlCostPanelStat1Icon);
            }
            else if (statIndex == 1)
            {
               centerTextPath = getProtoControlPath(eControlCostPanelStat2Label);
               centerCommand  = getProtoControlPath(eControlCostPanelStat2Icon);
            }
            else if (statIndex == 2)
            {
               centerTextPath = getProtoControlPath(eControlCostPanelStat3Label);
               centerCommand  = getProtoControlPath(eControlCostPanelStat3Icon);
            }

            float statCostAmount = mItems[mCurrentItem].mStatData.mStatAmount[i];
            int   statID     = mItems[mCurrentItem].mStatData.mStatID[i];
            int   statType       = mItems[mCurrentItem].mStatData.mStatType[i];
            if(statType == BUIGamePlayerStat::cTypeResource)
            {         
               if(pPlayer->getResource(statID) < statCostAmount)
                  bCanAfford=false;
            }
            else
            {  
               //-- handle train limits
               if (statCostAmount <= 0 && mItems[mCurrentItem].mTrainLimit != -1)
               {
                  bStatIsTrainlimit = true;
                  if (mItems[mCurrentItem].mTrainCount >= mItems[mCurrentItem].mTrainLimit)
                     bCanAfford=false;
               }
               else
               {
                  if(!pPlayer->checkPop(statID, statCostAmount))
                     bCanAfford=false;         
               }
            }
                                                
            //-- set the text
            if (bStatIsTrainlimit)
               sLocalTempUString.locFormat(L"%d / %d", mItems[mCurrentItem].mTrainCount, mItems[mCurrentItem].mTrainLimit);
            else
               sLocalTempUString.locFormat(L"%.0f", mItems[mCurrentItem].mStatData.mStatAmount[i]);

            int frameID = mItems[mCurrentItem].mStatData.mStatFrameID[i];

            //-- turn 0 pop stats off
            if (statType == BUIGamePlayerStat::cTypePop && (!bStatIsTrainlimit))
            {               
               if (mItems[mCurrentItem].mStatData.mStatAmount[i] <= cFloatCompareEpsilon)
               {
                  frameID = 0;
                  sLocalTempUString.set("");
               }
            }

            // BEK - PHX - 5449
            // don't turn the text red when the item is a tech and the tech is currently being researched
            if ((mItems[mCurrentItem].mItemType == BFlashHUD::eCircleMenuItemTypeTech) && (mItems[mCurrentItem].mTrainPercent > 0.0f))
               bCanAfford = true;

            vString.SetStringW(sLocalTempUString.getPtr());
            mpMovie->setVariable(centerTextPath, vString, GFxMovie::SV_Normal);

            vInfoText.SetStringW(mItems[mCurrentItem].mInfoText.getPtr());
            mpMovie->setVariable(getProtoControlPath(eControlCostPanelTitleBar), vInfoText, GFxMovie::SV_Normal);
            vInfoText2.SetStringW(mItems[mCurrentItem].mInfoText2.getPtr());
            mpMovie->setVariable(getProtoControlPath(eControlCostPanelTitleBar2), vInfoText2, GFxMovie::SV_Normal);

            mCostPanelState[statIndex].mFrame = frameID;
            mCostPanelState[statIndex].mCanAfford = bCanAfford;
                          
            GFxValue value[3];
            value[0].SetString(centerCommand.c_str());
            value[1].SetNumber((double) frameID);
            value[2].SetBoolean(bCanAfford);
            mpMovie->invokeActionScript(getProtoASFunctionName(eHUDASFunctionSetCostPanelStatIcon), value, 3);
         }
      }  
   }
   else
   {
      clearCostPanelState();
      //-- set the movie display into a state where only the center text is visible (denoted by -2)
      setInfoPanelSlot(-2, eHUDKeyframeOff);
      vString.SetStringW(mBaseText.getPtr());
      vString2.SetStringW(mBaseText2.getPtr());
      mpMovie->setVariable(getProtoControlPath(eControlBaseTextLabel), vString, GFxMovie::SV_Normal);
      mpMovie->setVariable(getProtoControlPath(eControlBaseText2Label), vString2, GFxMovie::SV_Normal);
   }
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashHUD::setInfoPanelSlotData()
{
   if (mCurrentItem == -1)
      return;

   int slotID = mItems[mCurrentItem].mOrder;
   
   int slotStartEnum = -1;
   switch (slotID)
   {
      case 0: slotStartEnum = eControlInfoPanelSlot1Start; break;
      case 1: slotStartEnum = eControlInfoPanelSlot2Start; break;
      case 2: slotStartEnum = eControlInfoPanelSlot3Start; break;
      case 3: slotStartEnum = eControlInfoPanelSlot4Start; break;
      case 4: slotStartEnum = eControlInfoPanelSlot5Start; break;
      case 5: slotStartEnum = eControlInfoPanelSlot6Start; break;
      case 6: slotStartEnum = eControlInfoPanelSlot7Start; break;
      case 7: slotStartEnum = eControlInfoPanelSlot8Start; break;
   }   
   
   //-- set the title bar text   
   GFxValue vString;
   vString.SetStringW(mItems[mCurrentItem].mInfoText.getPtr());
   mpMovie->setVariable(getProtoControlPath(slotStartEnum), vString, GFxMovie::SV_Normal);

   //-- this fills out the detail lines of the selected item over several text boxes.  We need to make the paths driven from an XML file
   if(!mItems[mCurrentItem].mInfoDetail.isEmpty())
   {                  
      BSmallDynamicSimArray<BUString> lines;
      uint detailCount=gUI.wordWrapText(mItems[mCurrentItem].mInfoDetail, cMaxDetailLines, lines);

      //trace("detailCount = %u, currentItem = %u, slot = %u", detailCount, mCurrentItem, slotID);
      sLocalTempUString.empty();
      GFxValue vString2;
      for (uint i = 0; i < cMaxDetailLines; i++)
      {
         if (i < detailCount)
            sLocalTempUString = lines[i];
         else
            sLocalTempUString = L"";

         int slotDetailID = slotStartEnum + 1 + i;

         vString2.SetStringW(sLocalTempUString.getPtr());
         mpMovie->setVariable(getProtoControlPath(slotDetailID), vString2, GFxMovie::SV_Normal);
         //trace("InfoLine[%u] = %s", i, BStrConv::toA(mInfoLines[i].mText));
      }      
   }
   else
   {
      for (int j= 0; j < cMaxDetailLines; ++j)
      {
         mpMovie->setVariable(getProtoControlPath(slotStartEnum+1+j), "", GFxMovie::SV_Normal);
      }      
   }
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashHUD::clearInfoPanelDescription()
{
   mpMovie->setVariable(getProtoControlPath(eControlInfoPanelDescriptionHeader), "", GFxMovie::SV_Normal);
   mpMovie->setVariable(getProtoControlPath(eControlInfoPanelDescriptionText), "", GFxMovie::SV_Normal);  
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashHUD::setInfoPanelDescription()
{
   if (mCurrentItem == -1 || !mCircleMenuExtraInfoVisible)
   {
      //clearInfoPanelDescription(); -- unnecessary
      return;
   }
   
   //-- set the title bar text
   GFxValue value;   
   if (!mItems[mCurrentItem].mInfoText.isEmpty())
   {
      value.SetStringW(mItems[mCurrentItem].mInfoText.getPtr());
      mpMovie->setVariable(getProtoControlPath(eControlInfoPanelDescriptionHeader), value, GFxMovie::SV_Normal);      
   }
   else
   {
      value.SetStringW(NULL);
      mpMovie->setVariable(getProtoControlPath(eControlInfoPanelDescriptionHeader), value, GFxMovie::SV_Normal);
   }
     
   GFxValue value2;
   if (!mItems[mCurrentItem].mInfoDetail.isEmpty())
      value2.SetStringW(mItems[mCurrentItem].mInfoDetail.getPtr());      
   else
      value2.SetStringW(NULL);      

   mpMovie->invokeActionScript(getProtoASFunctionName(eHUDASFunctionSetInfoPanelDescription), &value2, 1);
   mpMovie->invokeActionScript(getProtoASFunctionName(eHUDASFunctionShowInfoPanelRightStick), NULL, 0);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashHUD::clearInfoPanelStats()
{
   GFxValue values[4];
   int curStatID = 1;
   int totalStatCount = (eControlInfoPanelStatEnd-eControlInfoPanelStatStart)+1;
   for (int i = 0; i < totalStatCount; i++)
   {
      values[0].SetNumber((int)curStatID);
      values[1].SetString(getProtoKeyFrameName(eHUDKeyframeOff));
      values[2].SetNumber((int) -1);
      values[3].SetBoolean(false);
      mpMovie->invokeActionScript(getProtoASFunctionName(eHUDASFunctionSetInfoPanelStatData), values, 4);
      curStatID++;
   }
   mpMovie->setVariable(getProtoControlPath(eControlInfoPanelStatHeader), "", GFxMovie::SV_Normal);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashHUD::setInfoPanelStats()
{  
   clearInfoPanelStats();

   if (mCurrentItem == -1)
      return;
   
   BFixedString128 keyFrame;
   int curStatID = 1;
   GFxValue values[4];
   bool bDisplayGradeLabel = false;
   for(int j=0; j<mItems[mCurrentItem].mUnitStatData.mCount; j++)
   {            
      float statValue = 0.0f;      
      if (mItems[mCurrentItem].mUnitStatData.mStatType[j] == BUIGameUnitStat::cTypeAttackGrade)
      {
         statValue = mItems[mCurrentItem].mUnitStatData.mStatAmount[j];
         
         values[0].SetNumber((int)curStatID);
//-- FIXING PREFIX BUG ID 3250
         const BFlashProtoIcon* pIcon = getProtoIcon(mItems[mCurrentItem].mUnitStatData.mStatFrameID[j]);
//--
         if (pIcon)
            values[1].SetString(pIcon->mName.c_str());

         values[2].SetNumber((int)statValue);

         //-- set highest stat to yellow
         bool bHighestStat = mItems[mCurrentItem].mUnitStatData.mStatIndex[j] != -1 ? true : false;
         values[3].SetBoolean(bHighestStat);
         
         mpMovie->invokeActionScript(getProtoASFunctionName(eHUDASFunctionSetInfoPanelStatData), values, 4);

         curStatID++;

         bDisplayGradeLabel = true;
         //mpMovie->setVariable(getProtoControlPath(eControlInfoPanelStatStart+j),  text.c_str(), GFxMovie::SV_Normal);         
      }            
   }   

   if (bDisplayGradeLabel)
   {
      GFxValue locString;
      locString.SetStringW(gDatabase.getLocStringFromIndex(mHUDStringID[eHUDStringCombatGrade]).getPtr());
      mpMovie->setVariable(getProtoControlPath(eControlInfoPanelStatHeader), locString, GFxMovie::SV_Normal);
   }   
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashHUD::updateCostTextColors()
{
   if (mCurrentItem == -1)
      return;

   if (mItems[mCurrentItem].mFree)
      return;

   if (!mpUser)
      return;

//-- FIXING PREFIX BUG ID 3251
   const BPlayer* pPlayer = mpUser->getPlayer(); 
//--
   if(!pPlayer)
      return;

   //BFixedString16 str;   
   BFixedString256 centerControlPath;

   //int slotID = mItems[mCurrentItem].mOrder;

   int statIndex;
   for (int i = 0; i < mItems[mCurrentItem].mStatData.mCount; ++i)
   {
      bool bCanAfford=true;

      statIndex = mItems[mCurrentItem].mStatData.mStatIndex[i];
      if (statIndex == 0)
      {         
         centerControlPath  = getProtoControlPath(eControlCostPanelStat1Icon);
      }
      else if (statIndex == 1)
      {    
         centerControlPath  = getProtoControlPath(eControlCostPanelStat2Icon);
      }
      else if (statIndex == 2)
      {    
         centerControlPath  = getProtoControlPath(eControlCostPanelStat3Icon);
      }

      float statCostAmount = mItems[mCurrentItem].mStatData.mStatAmount[i];
      int   statID         = mItems[mCurrentItem].mStatData.mStatID[i];
      int   statType       = mItems[mCurrentItem].mStatData.mStatType[i];

      bool bStatIsTrainLimit = false;
      if(statType == BUIGamePlayerStat::cTypeResource)
      {         
         if(pPlayer->getResource(statID) < statCostAmount)
            bCanAfford=false;
      }
      else
      {        
         //-- handle train limits
         if (statCostAmount <= 0 && mItems[mCurrentItem].mTrainLimit != -1)
         {
            bStatIsTrainLimit = true;
            if (mItems[mCurrentItem].mTrainCount >= mItems[mCurrentItem].mTrainLimit)
               bCanAfford=false;
         }
         else
         {
            if(!pPlayer->checkPop(statID, statCostAmount))
               bCanAfford=false;         
         }
      }

      // BEK - PHX - 5449
      // don't turn the text red when the item is a tech and the tech is currently being researched
      if ((mItems[mCurrentItem].mItemType == BFlashHUD::eCircleMenuItemTypeTech) && (mItems[mCurrentItem].mTrainPercent > 0.0f))
         bCanAfford = true;

      int frameID = mItems[mCurrentItem].mStatData.mStatFrameID[i];      
      if (statType == BUIGamePlayerStat::cTypePop && !bStatIsTrainLimit)
      {
         if (mItems[mCurrentItem].mStatData.mStatAmount[i] <= cFloatCompareEpsilon)
         frameID = 0;
      }

      if ((mCostPanelState[statIndex].mFrame == frameID) && 
          (mCostPanelState[statIndex].mCanAfford == bCanAfford))
         continue;

      mCostPanelState[statIndex].mFrame = frameID;
      mCostPanelState[statIndex].mCanAfford = bCanAfford;
                  
      GFxValue value[3];
      value[0].SetString(centerControlPath.c_str());
      value[1].SetNumber((double) frameID);
      value[2].SetBoolean(bCanAfford);
      mpMovie->invokeActionScript(getProtoASFunctionName(eHUDASFunctionSetCostPanelStatIcon), value, 3);      
   }
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int BFlashHUD::lookupIconID(const char* pIconType, const char* pIconName)
{
   int count=getProtoIconCount();
   for(int i = 0; i < count; i++)
   {
//-- FIXING PREFIX BUG ID 3252
      const BFlashProtoIcon* pIcon = getProtoIcon(i);
//--
      if (pIcon->mType==pIconType && pIcon->mName==pIconName)
         return i;
   }
   return -1;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashHUD::setCircleMenuExtraInfoVisible(bool bVisible)
{
   // if nothing is selected ignore the command to make the panel visible.  Fixes Bug PHX-4883
   if (mCurrentItem == -1 && bVisible)
      return;

   if ((bVisible == mCircleMenuExtraInfoVisible))
      return;

   if (!bVisible)
      mCurrentItemTimer = 0.0f;
   
   mCircleMenuExtraInfoVisible = bVisible;

   GFxValue value;
   if (mCircleMenuExtraInfoVisible)
   {      
      value.SetString(getProtoKeyFrameName(eHUDKeyframeOn));
   }
   else
   {
      value.SetString(getProtoKeyFrameName(eHUDKeyframeOff));
   }

   mpMovie->invokeActionScript(getProtoASFunctionName(eHUDASFunctionSetCircleMenuExtraInfoVisible), &value, 1);

   setInfoPanelDescription();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BFlashHUD::getCircleMenuExtraInfoVisible() const
{
   return mCircleMenuExtraInfoVisible;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashHUD::addAlert(BVector alertPosition)
{
   // if the alert is not configured, then leave it off.
   if (!gConfig.isDefined(cConfigHUDAttackNotification))
      return;

   if (!mpMovie)
      return;

   if (!mpUser)
      return;

   BCamera  *pCamera = mpUser->getCamera();
   if (!pCamera)
      return;

   // if the battle is on the screen, don't display the alert
   if(gRender.getViewParams().isPointOnScreen(alertPosition) == true)
      return;

   // get the location on the terrain we are looking at
   BVector cameraLookAtPoint = mpUser->getCameraHoverPoint();
   cameraLookAtPoint.y=0;

   BVector cameraDir = pCamera->getCameraDir();
   cameraDir.y=0;
   cameraDir.safeNormalize();

   float angle = 0.0f;
   float degrees = 0.0;

   BVector baseDir = alertPosition-cameraLookAtPoint;
   baseDir.y=0;
   baseDir.safeNormalize();

   angle = cameraDir.angleBetweenVector(baseDir);
   if (cameraDir.cross(baseDir).y < 0.0f)
   {
      angle = cTwoPi-angle;
   }

   degrees = angle*cDegreesPerRadian;

   degrees+=mAngleNormalizer;       // normalize the angle
   if (degrees > 360)
      degrees-=360;  // adjust for wrap around

   // now figure out which UI section this is in
   for (int i=0; i<cMaxAttackSlots; i++)
   {
      if (degrees < mAttackSlotsAngleMin[i])
         continue;
      if (degrees <= mAttackSlotsAngleMax[i])
      {
         mAttackSlotsStates[i]=true;
         break; // we're done, we found the slot
      }
   }
  
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashHUD::updateAlerts()
{
   for (int i=0; i<cMaxAttackSlots; i++)
   {
      if (mAttackSlotsStates[i] != mAttackSlotsStatesCache[i])
      {
         // fire off the call to flash
         GFxValue values[2];
         values[0].SetNumber((double)(i+1));
         values[1].SetBoolean(mAttackSlotsStates[i]);
         mpMovie->invokeActionScript("setHUDAttackVisible", values, 2); 

         // update the cache
         mAttackSlotsStatesCache[i] = mAttackSlotsStates[i];
      }      

      // clear the attack states 
      mAttackSlotsStates[i] = false; 

   }

/*
   BHandle fontHandle = gFontManager.getFontDenmark14();
   gFontManager.setFont( fontHandle );
   gFontManager.renderStringBegin();

   BString myString;
   myString.format("Attack Notifications: : %d, %d, %d, %d, %d, %d, %d, %d", 
                        mAttackSlotsStatesCache[0], 
                        mAttackSlotsStatesCache[1], 
                        mAttackSlotsStatesCache[2], 
                        mAttackSlotsStatesCache[3], 
                        mAttackSlotsStatesCache[4], 
                        mAttackSlotsStatesCache[5], 
                        mAttackSlotsStatesCache[6], 
                        mAttackSlotsStatesCache[7] );

   float x = 30.0f;
   float y = 400.0f;
   gFontManager.drawText(fontHandle, x, y, myString, cColorWhite, 1.0f, 1.0f, BFontManager2::cJustifyLeft);

   gFontManager.renderStringEnd();
*/
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashHUD::updateTechNotification()
{
   double time = gWorld->getGametimeFloat();

   if (mTechNotificationEndTimer > 0.0f && time > mTechNotificationEndTimer)
   {
      setTechNotification(mTechNotificationID, false);
   }
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashHUD::setDimension(int x, int y, int width, int height)
{
   if (!mpMovie)
      return;

   mpMovie->setDimension(x, y, width, height);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashHUD::flashCircleMenuSlot(int slot, bool flashOn)
{
   GFxValue values[2];
   values[0].SetNumber(slot);
   values[1].SetBoolean(flashOn);
   mpMovie->invokeActionScript("flashCircleMenuSlot", values, 2);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashHUD::flashCircleMenuCenterPanel(int slot, bool flashOn)
{
   GFxValue values[2];
   values[0].SetNumber(slot);
   values[1].SetBoolean(flashOn);
   mpMovie->invokeActionScript("flashCircleMenuCenterPanel", values, 2);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashHUD::flashResourcePanelSlot(int slot, bool flashOn)
{
   GFxValue values[2];
   values[0].SetNumber(slot);
   values[1].SetBoolean(flashOn);
   mpMovie->invokeActionScript("flashResourcePanelSlot", values, 2);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashHUD::flashResourcePanel(bool flashOn)
{
   GFxValue value;
   value.SetBoolean(flashOn);
   mpMovie->invokeActionScript("flashResourcePanel", &value, 1);
}

