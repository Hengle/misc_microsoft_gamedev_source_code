//============================================================================
// UIWidgets.cpp
// Ensemble Studios (c) 2007
//============================================================================

#include "common.h"
#include "uiwidgets.h"

//-- render
#include "render.h"
#include "renderDraw.h"
#include "renderThread.h"

#include "camera.h"
#include "user.h"
#include "usermanager.h"
#include "player.h"
#include "uigame.h"
#include "visualmanager.h"
#include "database.h"
#include "world.h"
#include "hpbar.h"
#include "FontSystem2.h"
#include "timermanager.h"
#include "configsgame.h"
#include "flashmanager.h"

#include "modemanager.h"
#include "modegame.h"

GFIMPLEMENTVERSION(BUIWidgets, 2);

enum 
{
   cSaveMarkerUIWidget=10000,
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BUIWidgets::BUIWidgets():
mpMovie(NULL),
mWidgetPanelVisible(true),
mHitpointBarVisible(false),
mMiniGameVisible(false),
mGameMenuBackgroundVisible(false),
mHitpointBarEntity(cInvalidObjectID),
mLastHitpoints(0),
mTimerVisible(false),
mTimerID(-1),
mTimerLabelID(-1),
mCitizensSavedVisible(false),
mNumCitizensSaved(0),
mNumCitizensNeeded(300),
mProgressBarMax(100),
mCounterMax(10),
mCounterCurrent(0),
mLastTimerTime(0),
mElapsedTimerTime(0.0f),
mTalkingHeadVisible(false),
mbFirstRender(true),
mTimerShown(true),
mObjectiveWidgetsShown(true)
{
   initEventHandle();

   // clear it out.
   for (uint i = 0; i < BUIWidgets::cNumGarrisonContainers; i++)
   {
      mGarrisonContainerVisible[i] = false;
      mGarrisonContainerEntities[i] = cInvalidObjectID;
      mGarrisonContainerCounts[i] = 0;
      mGarrisonContainerUseEntity[i] = false;
   }

   for (int i=0; i<BUIWidgets::cNumReticulePointers; i++)
   {
      mPointerRotationFloat[i]=0.0f;
      mPointerRotation[i]=0;
      mReticulePointersVisible[i]=false;
      mReticulePointerType[i]=cReticulePointerTypeNone;
      mReticulePointerEntities[i]=cInvalidObjectID;
   }

   for (int i=0; i<BUIWidgets::cNumMinigameCircles; i++)
   {
      mMinigameRotationFloat[i]=0.0f;
      mMinigameRotationCache[i]=0;
   }
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BUIWidgets::~BUIWidgets()
{
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BUIWidgets::init(const char* filename, const char* datafile)
{
   if (!loadData(datafile))
      return false;

   gFlashGateway.getOrCreateData(filename, cFlashAssetCategoryInGame, mDataHandle);
   mpMovie = gFlashGateway.createInstance(mDataHandle, false);
   gFlashGateway.registerEventHandler(mpMovie, mSimEventHandle);

   setFlag(cFlagInitialized, true);

   initResolution();

   // initialize the talking head scene
   mTalkingHeadScene.init( "art\\ui\\flash\\hud\\hud_talking_head\\hud_talking_head.gfx", "art\\ui\\flash\\hud\\hud_talking_head\\hud_talking_head.xml" );
   mTalkingHeadScene.setVisible(true);

   mObjectiveProgressControl.init( this, "mObjectiveProgressWidget" );

   mBtnBar.init(getMovie());
   mBtnBar.setMovieClip("mGameMenuBackground.mButtonBarContainer");

   mTimerShown = true;
   mObjectiveWidgetsShown = true;
   return true;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BUIWidgets::deinit()
{
   if (mpMovie)
   {
      gFlashGateway.unregisterEventHandler(mpMovie, mSimEventHandle);
      gFlashGateway.releaseInstance(mpMovie);
   }
   mpMovie = NULL;
   mbFirstRender = true;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BUIWidgets::enter()
{
   if (!getFlag(cFlagInitialized))
      return;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BUIWidgets::leave()
{
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BUIWidgets::update(float elapsedTime)
{   
   updateReticulePointer();
   updateGarrisoned();
   updateTimer(elapsedTime);
   getTalkingHead().update();
   mObjectiveProgressControl.update();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BUIWidgets::renderBegin()
{   
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BUIWidgets::render()
{   
   SCOPEDSAMPLEID(FlashUIGame, 0xFFFF0000);
   if (mbFirstRender)
   {
      mbFirstRender = false;
      return;
   }

   if (mWidgetPanelVisible && mpMovie)
      mpMovie->render();

   renderTimer();

   if (!gConfig.isDefined(cConfigShowFlashObjWidget))
   {
      renderGarrisoned();
      renderCitizensSaved();
   }

   renderReticulePointer();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BUIWidgets::renderEnd()
{
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BUIWidgets::handleInput(int port, BInputEventType event, BInputControlType controlType, BInputEventDetail& detail)
{
   return false;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BUIWidgets::receiveEvent(const BEvent& event, BThreadIndex threadIndex)
{
   if (threadIndex == cThreadIndexSim)
   {
      switch (event.mEventClass)
      {
      case BFlashManager::cFlashEventFSCommand:
         {
            BFlashManager::BFSCommandPayload* pPayload = static_cast<BFlashManager::BFSCommandPayload*>(event.mpPayload);
            if (pPayload->mCommand=="sound")
            {
               if (pPayload->mArgs=="TalkingHead")
               {
                  getTalkingHead().playTextShowSound();
               }
            }
         }
         break;
      }
   }

   return false;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BManagedTextureHandle BUIWidgets::getRenderTargetTexture()
{
   if (mpMovie)
      return mpMovie->mRenderTargetHandle;

   return cInvalidManagedTextureHandle;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BUIWidgets::setDimension(int x, int y, int width, int height)
{
   if (!mpMovie)
      return;

   mpMovie->setDimension(x, y, width, height);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BUIWidgets::setWidgetsVisible(bool bVisible)
{
   mWidgetPanelVisible = bVisible;
}

//-- Timer Display
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BUIWidgets::setTimerVisible(bool bVisible, int timerID, int timerLabelID/*=-1*/, float startingElapsed/*=0.0f*/)
{
   if (bVisible)
   {
      if (timerID >= 0)
      {
         mpMovie->invokeActionScript( "fadeTimerIn", NULL, 0 );
         mTimerVisible = true;

         BGameTimer * pTimer = gTimerManager.getTimer(timerID);

         BASSERT(pTimer);

         if (pTimer)
         {
            mTimerID = timerID;
            mTimerLabelID = timerLabelID;
            if (timerLabelID >= 0)
               mTimerLabel = gDatabase.getLocStringFromID(timerLabelID);
            else
               mTimerLabel = L"";

            GFxValue value;
            value.SetStringW(mTimerLabel.getPtr());
            mpMovie->setVariable("_root.mTimer.mLabel.mText.htmlText", value, GFxMovie::SV_Normal);

            mElapsedTimerTime=startingElapsed;
            updateTimerWidget();
         }
      }
   }
   else if (timerID == mTimerID)
   {
      mpMovie->invokeActionScript( "fadeTimerOut", NULL, 0 );
      mTimerVisible = false;
      mTimerID = -1;
   }
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BUIWidgets::showTimer( void )
{
   GFxValue arg;
   arg.SetBoolean( true );
   mpMovie->invokeActionScript( "setTimerVisible", &arg, 1 );
   mTimerShown = true;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BUIWidgets::hideTimer( void )
{
   GFxValue arg;
   arg.SetBoolean( false );
   mpMovie->invokeActionScript( "setTimerVisible", &arg, 1 );
   mTimerShown = false;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BUIWidgets::updateTimer(float elapsedTime)
{
   if (!mTimerVisible)
      return;

   mElapsedTimerTime+=elapsedTime;
   if (mElapsedTimerTime <= 0.100f)
      return;

   // reset this
   mElapsedTimerTime = 0.0f;

   updateTimerWidget();

   return;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BUIWidgets::updateTimerWidget()
{
   BGameTimer * pTimer = gTimerManager.getTimer(mTimerID);
   if (!pTimer)
      return;

   DWORD time = pTimer->getCurrentTime();
   DWORD hours = time / (1000*60*60);
   time -= (hours*(1000*60*60));
   DWORD minutes = time / (1000*60);
   time -= (minutes*(1000*60));
   DWORD seconds = time / 1000;

   mTimerString.locFormat(L"%02d:%02d", minutes, seconds);

   GFxValue value;
   value.SetStringW(mTimerString.getPtr());
   mpMovie->setVariable("_root.mTimer.mTime.mText.htmlText", value, GFxMovie::SV_Normal);   
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BUIWidgets::renderTimer()
{
/*
   if (!mTimerVisible)
      return;

   BGameTimer * pTimer = gTimerManager.getTimer(mTimerID);
   if (!pTimer)
      return;

   BHandle fontHandle=gFontManager.getFontDenmark14();
   gFontManager.setFont( fontHandle );

   gFontManager.renderStringBegin();

   // format the dword into a time string HH:MM:SS


   DWORD time = pTimer->getCurrentTime();
   DWORD hours = time / (1000*60*60);
   time -= (hours*(1000*60*60));
   DWORD minutes = time / (1000*60);
   time -= (minutes*(1000*60));
   DWORD seconds = time / 1000;

   BUString t;
   t.format(L"Timer : %02d:%02d", minutes, seconds);
   
   gFontManager.drawText(fontHandle, gUI.mfSafeX2-700, gUI.mfSafeY1+25+gFontManager.getLineHeight(), t, cColorWhite, 1.0f, 1.0f, BFontManager2::cJustifyLeft);

   gFontManager.renderStringEnd();
*/
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BUIWidgets::flashTimer()
{
   // fixme - add code here when flash is available
   return;
}


//-- Garrisoned in - 3 UI widgets.
//----------------------------------------------------------------------------
// The garrison widget will show the number garrisoned in the unit
//----------------------------------------------------------------------------
void BUIWidgets::setGarrisonedVisible(int id, bool bVisible, BEntityID container)
{
   if ( (id<0) || (id>=cNumGarrisonContainers))
      return;

   mGarrisonContainerVisible[id] = bVisible;
   mGarrisonContainerEntities[id] = container;
   mGarrisonContainerUseEntity[id] = true;

   BUString label;
   switch( id )
   {
      case 0: label = gDatabase.getLocStringFromID(22281); break; // Cargo 1
      case 1: label = gDatabase.getLocStringFromID(22234); break; // Cargo 2
      case 2: label = gDatabase.getLocStringFromID(24840); break; // Cargo 3
   }

   setObjectiveWidgetVisible(cObjectiveWidget2+id, bVisible, cObjectiveWidgetIconPeople, &label);
   updateGarrisonedUnit(id, true);
}

//----------------------------------------------------------------------------
// The garrison widget will show the number provided
//----------------------------------------------------------------------------
void BUIWidgets::setGarrisonedVisible(int id, bool bVisible, int numGarrisoned)
{
   if ((id < 0) || (id >= cNumGarrisonContainers))
      return;

   mGarrisonContainerVisible[id] = bVisible;
   mGarrisonContainerCounts[id] = numGarrisoned;
   mGarrisonContainerUseEntity[id] = false;

   BUString label;
   switch( id )
   {
      case 0: label = gDatabase.getLocStringFromID(22281); break; // Cargo 1
      case 1: label = gDatabase.getLocStringFromID(22234); break; // Cargo 2
      case 2: label = gDatabase.getLocStringFromID(24840); break; // Cargo 3
   }

   setObjectiveWidgetVisible(cObjectiveWidget2 + id, bVisible, cObjectiveWidgetIconPeople, &label);
   updateGarrisonedUnit(id, true);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BUIWidgets::flashGarrisoned(int id)
{
   // fixme - add this
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BUIWidgets::updateGarrisonedUnit(int id, bool force)
{
   if (mGarrisonContainerUseEntity[id])
   {
      // format the dword into a time string HH:MM:SS
      BUnit* pUnit = gWorld->getUnit(mGarrisonContainerEntities[id]);
      if (!pUnit)
      {
         BASSERTM(false, "Trying to show garrisoned units of a unit that doesn't exist.");
         return;
      }

      int count = pUnit->getGarrisonedUnits().getNumber();

      // did our count change or are we forcing an update?
      if (!force && (count == mGarrisonContainerCounts[id]))
         return;

      // we changed, update our count
      mGarrisonContainerCounts[id] = count;

      // set the text
      mGarrisonContainerStrings[id].locFormat(L"%d", mGarrisonContainerCounts[id]);
      setObjectiveWidgetText(cObjectiveWidget2 + id, mGarrisonContainerStrings[id]);

      // set the icons
      float temp = (float)mGarrisonContainerCounts[id] / 50.0f;
      int barCount = (int)(temp * 10.0f);

      setObjectiveWidgetBar(cObjectiveWidget2 + id, cObjectiveWidgetProgressTypeBar, barCount);
   }
   else if (force)
   {
      // set the text
      mGarrisonContainerStrings[id].locFormat(L"%d", mGarrisonContainerCounts[id]);
      setObjectiveWidgetText(cObjectiveWidget2 + id, mGarrisonContainerStrings[id]);

      // set the icons
      float temp = (float)mGarrisonContainerCounts[id] / 50.0f;
      int barCount = (int)(temp * 10.0f);

      setObjectiveWidgetBar(cObjectiveWidget2 + id, cObjectiveWidgetProgressTypeBar, barCount);
   }
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BUIWidgets::updateGarrisoned()
{
   // display the counts
   for (int i=0; i<cNumGarrisonContainers; i++)
   {
      if (!mGarrisonContainerVisible[i])
         continue;

      updateGarrisonedUnit(i, false);
   }
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BUIWidgets::renderGarrisoned()
{
   BHandle fontHandle=gFontManager.getFontDenmark14();
   gFontManager.setFont( fontHandle );

   // display the counts
   for (int i=0; i<cNumGarrisonContainers; i++)
   {
      if (!mGarrisonContainerVisible[i])
         continue;


      // format the dword into a time string HH:MM:SS
      BUnit * pUnit = gWorld->getUnit(mGarrisonContainerEntities[i]);
      if (!pUnit)
      {
         BASSERTM(false, "Trying to show garrisoned units of a unit that doesn't exist.");
         continue;
      }

      int count = pUnit->getGarrisonedUnits().getNumber();

      BUString t;
      t.locFormat(L"Ship %d : %d", i+1, count);

      gFontManager.drawText(fontHandle, gUI.mfSafeX1, gUI.mfSafeY1+300.0f+(gFontManager.getLineHeight()*(i+1)), t);
   }
}

// Object counter (0-10)
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BUIWidgets::setCounterVisible(bool bVisible, int counterMax, int initialValue)
{
   // show our counter
   mCounterVisible = bVisible;
   setObjectiveWidgetVisible(cObjectiveWidget1, bVisible, cObjectiveWidgetIconGrey);

   if (mCounterVisible)
   {
      if (mCounterMax > cNumCounterObjects)
         return;

      // save off our max
      mCounterMax = counterMax;

      // set the current value
      updateCounter(initialValue);
   }
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BUIWidgets::updateCounter(int newValue)
{
   if ( (mCounterCurrent <0) || (mCounterCurrent>mCounterMax) )
      return;

   mCounterCurrent = newValue; 

   mCounterString.locFormat(L"%d/%d", mCounterCurrent, mCounterMax);
   setObjectiveWidgetText(cObjectiveWidget1, mCounterString);

   setObjectiveWidgetBar(cObjectiveWidget1, cObjectiveWidgetProgressTypeBar, mCounterCurrent);
}

//-- Citizens Saved Widget
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BUIWidgets::setCitizensSavedVisible(bool bVisible)
{ 
   mCitizensSavedVisible = bVisible;
   setObjectiveWidgetVisible(cObjectiveWidget1, bVisible, cObjectiveWidgetIconGrey);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BUIWidgets::setCitizensSaved(int total, int totalNeeded) 
{
   mNumCitizensSaved = total; 
   mNumCitizensNeeded = totalNeeded;

   mCitizensSavedString.locFormat(L"%d/%d", mNumCitizensSaved, mNumCitizensNeeded);
   setObjectiveWidgetText(cObjectiveWidget1, mCitizensSavedString);

   if (totalNeeded == 0)
      return;

   // let i
   float temp = (float)total/(float)totalNeeded;
   int count = (int)(temp*10);

   setObjectiveWidgetBar(cObjectiveWidget1, cObjectiveWidgetProgressTypeBar, count);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BUIWidgets::updateCitizensSaved()
{
   // fixme - add code here when flash comes on line
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BUIWidgets::renderCitizensSaved()
{
   if (!mCitizensSavedVisible)
      return;

   BHandle fontHandle=gFontManager.getFontDenmark14();
   gFontManager.setFont( fontHandle );

   BUString t;
   t.locFormat(L"Total Saved : %d", mNumCitizensSaved);

   gFontManager.drawText(fontHandle, gUI.mfSafeX1, gUI.mfSafeY1+300.0f, t);
}

//-- Objective reticule pointers
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BUIWidgets::setReticulePointerVisible(int id, bool bVisible, BEntityID container, int type)
{
   if (!mpMovie)
      return;

   if ( (id<0) || (id>=cNumReticulePointers))
      return;

   if (bVisible)
   {
      mReticulePointerType[id]=type;
      mReticulePointerEntities[id] = container;
   }
   else
      mReticulePointerType[id]=cReticulePointerTypeNone;

   mReticulePointersVisible[id]=bVisible;

   // turn on the flash for this
   GFxValue values[2];
   values[0].SetNumber((double)(id+1));
   values[1].SetBoolean(bVisible);
   mpMovie->invokeActionScript("showObjectiveReticule", values, 2); 
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BUIWidgets::setReticulePointerVisible(int id, bool bVisible, BVector area)
{
   if (!mpMovie)
      return;

   if ( (id<0) || (id>=cNumReticulePointers))
      return;

   if (bVisible)
   {
      mReticulePointerType[id]=cReticulePointerTypeArea;
      mReticulePointerArea[id] = area;
   }
   else
      mReticulePointerType[id]=cReticulePointerTypeNone;

   mReticulePointersVisible[id]=bVisible;

   // turn on the flash for this
   GFxValue values[2];
   values[0].SetNumber((double)(id+1));
   values[1].SetBoolean(bVisible);
   mpMovie->invokeActionScript("showObjectiveReticule", values, 2); 
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BUIWidgets::updateReticulePointer()
{
   if (!mpMovie)
      return;

   BUser * const pUser = gUserManager.getUser(BUserManager::cPrimaryUser);
   if (!pUser)
      return;

   BCamera  *pCamera = gUserManager.getUser(BUserManager::cPrimaryUser)->getCamera();
   if (!pCamera)
      return;

   // get the location on the terrain we are looking at
   BVector cameraLookAtPoint = pUser->getHoverPoint();
   cameraLookAtPoint.y=0;

   BVector cameraDir = pCamera->getCameraDir();
   cameraDir.y=0;
   cameraDir.safeNormalize();

   bool callActionScript = false;

   float angle = 0.0f;
   float degrees = 0.0;

   // get the directions for each of the visible pointers
   for (int i=0; i<cNumReticulePointers; i++)
   {
      if (mReticulePointerType[i] == cReticulePointerTypeNone)
         continue;

      BVector baseLoc;

      bool doUpdatePointer = false;
      switch (mReticulePointerType[i])
      {
         case cReticulePointerTypeArea:
            {
               baseLoc = mReticulePointerArea[i];
               BVector baseVec = baseLoc-cameraLookAtPoint;
               baseVec.y=0;
               if (baseVec.length() < 3.0f)
               {
                  if (mReticulePointersVisible[i])
                  {
                     setReticulePointerVisible(i, false, 0.0f);
                     mReticulePointerType[i] = cReticulePointerTypeArea;
                     mReticulePointerArea[i] = baseLoc;
                  }
                  break;
               }
               else
               {
                  if (!mReticulePointersVisible[i])
                     setReticulePointerVisible(i, true, mReticulePointerArea[i]);
               }
               doUpdatePointer = true;
            }
            break;
         case cReticulePointerTypeUnit:
            {
               BUnit * pUnit = gWorld->getUnit(mReticulePointerEntities[i]);
               if (!pUnit)
               {
                  setReticulePointerVisible(i, false, cInvalidObjectID, cReticulePointerTypeNone);
                  break;
               }
               if (pUser->isPointingAt(pUnit->getID()))
               {
                  if (mReticulePointersVisible[i])
                  {
                     setReticulePointerVisible(i, false, cInvalidObjectID, cReticulePointerTypeNone);
                     mReticulePointerType[i] = cReticulePointerTypeUnit;
                     mReticulePointerEntities[i] = pUnit->getID();
                  }
                  break;
               }
               else
               {
                  if (!mReticulePointersVisible[i])
                     setReticulePointerVisible(i, true, pUnit->getID(), cReticulePointerTypeUnit);
               }
               doUpdatePointer = true;
               baseLoc = pUnit->getPosition();
               baseLoc.y=0;
            }
            break;
         case cReticulePointerTypeSquad:
            {
               BSquad * pSquad = gWorld->getSquad(mReticulePointerEntities[i]);
               if (!pSquad)
               {
                  setReticulePointerVisible(i, false, cInvalidObjectID, cReticulePointerTypeNone);
                  break;
               }
               if (pUser->isPointingAt(pSquad->getID()))
               {
                  if (mReticulePointersVisible[i])
                  {
                     setReticulePointerVisible(i, false, cInvalidObjectID, cReticulePointerTypeNone);
                     mReticulePointerType[i] = cReticulePointerTypeSquad;
                     mReticulePointerEntities[i] = pSquad->getID();
                  }
                  break;
               }
               else
               {
                  if (!mReticulePointersVisible[i])
                     setReticulePointerVisible(i, true, pSquad->getID(), cReticulePointerTypeSquad);
               }
               doUpdatePointer = true;
               baseLoc = pSquad->getPosition();
               baseLoc.y=0;
            }
            break;
      }

      if (!doUpdatePointer)
         continue;

      BVector baseDir = baseLoc-cameraLookAtPoint;
      // BVector baseDir = cameraLookAtPoint-baseLoc;
      baseDir.y=0;
      baseDir.safeNormalize();

      angle = cameraDir.angleBetweenVector(baseDir);
      if (cameraDir.cross(baseDir).y < 0.0f)
      {
         // angle = -angle;
         angle = cTwoPi-angle;
      }

      degrees = angle*cDegreesPerRadian;
      int iDegrees = (int)(degrees*100);         // get a better resolution with a bit of caching
      if (mPointerRotation[i] == iDegrees)
         continue;

      // at least one of the pointers needs to be visible and have changed
      callActionScript = true;
      mPointerRotationFloat[i] = degrees;
      mPointerRotation[i] = iDegrees;
   }

   // do we need to update?
   if (!callActionScript)
      return;

   // make the call.
   GFxValue values[cNumReticulePointers];

   for (int i=0; i<cNumReticulePointers; i++)
      values[i].SetNumber((double)mPointerRotationFloat[i]);

   mpMovie->invokeActionScript("rotateObjectiveReticule", values, cNumReticulePointers);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BUIWidgets::renderReticulePointer()
{
   // updateReticulePointer();
}

// Helper Methods
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BUIWidgets::setObjectiveWidgetVisible(int id, bool bVisible, int icon, BUString* pLabel /*= NULL*/)
{
   if (!gConfig.isDefined(cConfigShowFlashObjWidget))
      return;

   // call flash to turn this thing on/off
   GFxValue values[4];
   values[0].SetNumber(id);
   values[1].SetBoolean(bVisible);
   values[2].SetNumber(icon);
   if( pLabel )
      values[3].SetStringW( pLabel->getPtr() );

   mpMovie->invokeActionScript("setObjectiveWidgetVisible", values, 4);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BUIWidgets::setObjectiveWidgetText(int id, const BUString& text)
{
   if (!gConfig.isDefined(cConfigShowFlashObjWidget))
      return;

   GFxValue values[2];
   values[0].SetNumber(id);
   values[1].SetStringW(text.getPtr());
   mpMovie->invokeActionScript("setObjectiveWidgetText", values, 2);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BUIWidgets::setObjectiveWidgetBar(int id, int type, int barsActive)
{
   if (!gConfig.isDefined(cConfigShowFlashObjWidget))
      return;

   GFxValue values[3];
   values[0].SetNumber(id);            // widget ID
   values[1].SetNumber(type);            // progress bar type
   values[2].SetNumber(barsActive);
   mpMovie->invokeActionScript("setObjectiveWidgetBar", values, 3);
}

//==============================================================================
// BUIWidgets::displayUserMessage
//==============================================================================
void BUIWidgets::displayUserMessage(const BUString& message)
{
   // display the hint
   GFxValue values[1];
   values[0].SetStringW(message.getPtr());
   mpMovie->invokeActionScript("showUserMessage", values, 1);
}


//==============================================================================
// BUIWidgets::hideHint
//==============================================================================
void BUIWidgets::setUserMessageVisible(bool bVisible, bool bForce /*= false*/)
{
   GFxValue values[2];
   values[0].SetBoolean(bVisible);
   values[1].SetBoolean(bForce);
   mpMovie->invokeActionScript("setUserMessageVisible", values, 2);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BUIWidgets::handleUIControlEvent( BUIControlEvent& event )
{
   return false;
}

//============================================================================
//============================================================================
void BUIWidgets::showObjectiveWidgets( void )
{
   mpMovie->invokeActionScript("showObjectiveWidgets", NULL, 0);
   mObjectiveWidgetsShown = true;
}

//============================================================================
//============================================================================
void BUIWidgets::hideObjectiveWidgets( void )
{
   mpMovie->invokeActionScript("hideObjectiveWidgets", NULL, 0);
   mObjectiveWidgetsShown = false;
}

//============================================================================
//============================================================================
bool BUIWidgets::save(BStream* pStream, int saveType) const
{
   GFWRITEVAR(pStream, int, mCounterCurrent);
   GFWRITEVAR(pStream, int, mCounterMax);
   //BUString mCounterString;
   //int mProgressBarMax;
   //BEntityID mHitpointBarEntity;
   //int mLastHitpoints;
   GFWRITEVAR(pStream, int, mTimerID);
   GFWRITEVAR(pStream, int, mTimerLabelID);
   //BSimUString mTimerLabel;
   //DWORD mLastTimerTime;
   //BSimUString mTimerString;
   GFWRITEVAR(pStream, float, mElapsedTimerTime);
   GFWRITEVAR(pStream, int, mNumCitizensSaved);
   GFWRITEVAR(pStream, int, mNumCitizensNeeded);
   //BUString mCitizensSavedString;
   GFWRITEPTR(pStream, sizeof(bool)*cNumGarrisonContainers, mGarrisonContainerVisible);
   GFWRITEPTR(pStream, sizeof(BEntityID)*cNumGarrisonContainers, mGarrisonContainerEntities);
   GFWRITEPTR(pStream, sizeof(bool)*cNumGarrisonContainers, mGarrisonContainerUseEntity);
   GFWRITEPTR(pStream, sizeof(int)*cNumGarrisonContainers, mGarrisonContainerCounts);
   //BUString mGarrisonContainerStrings[cNumGarrisonContainers];
   GFWRITEPTR(pStream, sizeof(bool)*cNumReticulePointers, mReticulePointersVisible);
   GFWRITEPTR(pStream, sizeof(int)*cNumReticulePointers, mReticulePointerType);
   for (uint i=0; i<cNumGarrisonContainers; i++)
      GFWRITEVECTOR(pStream, mReticulePointerArea[i])
   GFWRITEPTR(pStream, sizeof(BEntityID)*cNumReticulePointers, mReticulePointerEntities);
   GFWRITEPTR(pStream, sizeof(int)*cNumReticulePointers, mPointerRotation);
   GFWRITEPTR(pStream, sizeof(float)*cNumReticulePointers, mPointerRotationFloat);
   //int mMinigameRotationCache[cNumMinigameCircles];
   //float mMinigameRotationFloat[cNumMinigameCircles];
   //BUIButtonBar2 mBtnBar;
   GFWRITECLASS(pStream, saveType, mTalkingHeadScene.mTalkingHeadControl);
   GFWRITECLASS(pStream, saveType, mObjectiveProgressControl);
   GFWRITEBITBOOL(pStream, mWidgetPanelVisible);
   //GFWRITEBITBOOL(pStream, mHitpointBarVisible);
   //GFWRITEBITBOOL(pStream, mProgressBarVisible);
   GFWRITEBITBOOL(pStream, mTimerVisible);
   GFWRITEBITBOOL(pStream, mCitizensSavedVisible);
   GFWRITEBITBOOL(pStream, mCounterVisible);
   //bool mMiniGameVisible:1;
   //bool mGameMenuBackgroundVisible:1;
   //bool mTalkingHeadVisible:1;
   //bool mbFirstRender:1;
   GFWRITEBITBOOL(pStream, mTimerShown);
   GFWRITEMARKER(pStream, cSaveMarkerUIWidget);
   return true;
}

//============================================================================
//============================================================================
bool BUIWidgets::load(BStream* pStream, int saveType)
{
   GFREADVAR(pStream, int, mCounterCurrent);
   GFREADVAR(pStream, int, mCounterMax);
   //BUString mCounterString;
   //int mProgressBarMax;
   //BEntityID mHitpointBarEntity;
   //int mLastHitpoints;
   GFREADVAR(pStream, int, mTimerID);
   GFREADVAR(pStream, int, mTimerLabelID);
   //BSimUString mTimerLabel;
   //DWORD mLastTimerTime;
   //BSimUString mTimerString;
   GFREADVAR(pStream, float, mElapsedTimerTime);
   GFREADVAR(pStream, int, mNumCitizensSaved);
   GFREADVAR(pStream, int, mNumCitizensNeeded);
   //BUString mCitizensSavedString;
   GFREADPTR(pStream, sizeof(bool)*cNumGarrisonContainers, mGarrisonContainerVisible);
   GFREADPTR(pStream, sizeof(BEntityID)*cNumGarrisonContainers, mGarrisonContainerEntities);
   GFREADPTR(pStream, sizeof(bool)*cNumGarrisonContainers, mGarrisonContainerUseEntity);
   GFREADPTR(pStream, sizeof(int)*cNumGarrisonContainers, mGarrisonContainerCounts);
   //BUString mGarrisonContainerStrings[cNumGarrisonContainers];
   GFREADPTR(pStream, sizeof(bool)*cNumReticulePointers, mReticulePointersVisible);
   GFREADPTR(pStream, sizeof(int)*cNumReticulePointers, mReticulePointerType);
   for (uint i=0; i<cNumGarrisonContainers; i++)
      GFREADVECTOR(pStream, mReticulePointerArea[i])
   GFREADPTR(pStream, sizeof(BEntityID)*cNumReticulePointers, mReticulePointerEntities);
   GFREADPTR(pStream, sizeof(int)*cNumReticulePointers, mPointerRotation);
   GFREADPTR(pStream, sizeof(float)*cNumReticulePointers, mPointerRotationFloat);
   //int mMinigameRotationCache[cNumMinigameCircles];
   //float mMinigameRotationFloat[cNumMinigameCircles];
   //BUIButtonBar2 mBtnBar;
   if (mGameFileVersion >= 2)
   {
      GFREADCLASS(pStream, saveType, mTalkingHeadScene.mTalkingHeadControl);
      GFREADCLASS(pStream, saveType, mObjectiveProgressControl);
   }
   GFREADBITBOOL(pStream, mWidgetPanelVisible);
   //GFREADBITBOOL(pStream, mHitpointBarVisible);
   //GFREADBITBOOL(pStream, mProgressBarVisible);
   GFREADBITBOOL(pStream, mTimerVisible);
   GFREADBITBOOL(pStream, mCitizensSavedVisible);
   GFREADBITBOOL(pStream, mCounterVisible);
   //bool mMiniGameVisible:1;
   //bool mGameMenuBackgroundVisible:1;
   //bool mTalkingHeadVisible:1;
   //bool mbFirstRender:1;
   if (mGameFileVersion >= 2)
      GFREADBITBOOL(pStream, mTimerShown)
   GFREADMARKER(pStream, cSaveMarkerUIWidget);

   setCounterVisible(mCounterVisible, mCounterMax, mCounterCurrent);
   
   if( mTimerVisible )
      showTimer();
   else
      hideTimer();

   if( mTimerShown )
      setTimerVisible(mTimerShown, mTimerID, mTimerLabelID, mElapsedTimerTime);

   setCitizensSaved(mNumCitizensSaved, mNumCitizensNeeded);
   setCitizensSavedVisible(mCitizensSavedVisible);

   for (int i=0; i<cNumGarrisonContainers; i++)
   {
      if (mGarrisonContainerUseEntity[i])
         setGarrisonedVisible(i, mGarrisonContainerVisible[i], mGarrisonContainerEntities[i]);
      else
         setGarrisonedVisible(i, mGarrisonContainerVisible[i], mGarrisonContainerCounts[i]);
   }

   for (int i=0; i<cNumReticulePointers; i++)
   {
      if (mReticulePointerType[i] == cReticulePointerTypeArea)
         setReticulePointerVisible(i, mReticulePointersVisible[i], mReticulePointerArea[i]);
      else
         setReticulePointerVisible(i, mReticulePointersVisible[i], mReticulePointerEntities[i], mReticulePointerType[i]);
   }

   return true;
}
