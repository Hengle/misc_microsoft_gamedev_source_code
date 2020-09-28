//============================================================================
// UILeaderPicker.cpp
// Ensemble Studios (c) 2007
//============================================================================

#include "common.h"
#include "uiLeaderPicker.h"

//-- render
#include "render.h"
#include "renderDraw.h"
#include "renderThread.h"

#include "flashmanager.h"
#include "user.h"
#include "usermanager.h"
#include "visualmanager.h"
#include "database.h"
#include "configsgame.h"
#include "econfigenum.h"
#include "inputcontrol.h"
#include "UIInputHandler.h"
#include "leaders.h"
#include "soundmanager.h"
#include "UIButtonBar.h"

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BUILeaderPicker::BUILeaderPicker():
mpMovie(NULL),
mpInputHandler(NULL),
mpEventHandler(NULL),
mSelectedLeader(0),
mPanelVisible(false),
mInputWaitForStart(false)
{
   initEventHandle();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BUILeaderPicker::~BUILeaderPicker()
{
   delete mpInputHandler;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BUILeaderPicker::init(const char* filename, const char* datafile)
{
   if (!filename)
      return false;

   if (!datafile)
      return false;

   if (!loadData(datafile))
      return false;

   gFlashGateway.getOrCreateData(filename, cFlashAssetCategoryPreGame, mDataHandle);
   mpMovie = gFlashGateway.createInstance(mDataHandle, false);
   gFlashGateway.registerEventHandler(mpMovie, mSimEventHandle);

   // Init the button bar
   mBtnBar.init(mpMovie);
   mBtnBar.setButtonStates(BUIButtonBar::cFlashButtonA, BUIButtonBar::cFlashButtonB, BUIButtonBar::cFlashButtonOff, BUIButtonBar::cFlashButtonOff, BUIButtonBar::cFlashButtonOff, BUIButtonBar::cFlashButtonOff );
   mBtnBar.setButtonTexts( gDatabase.getLocStringFromID(23437), 
      gDatabase.getLocStringFromID(23438), 
      gDatabase.getLocStringFromID(0), 
      gDatabase.getLocStringFromID(0), 
      gDatabase.getLocStringFromID(0), 
      gDatabase.getLocStringFromID(0) );


   // let's populate it here

   mpInputHandler = new BUIInputHandler();
   mpInputHandler->loadControls("art\\ui\\flash\\pregame\\leaderPicker\\LeaderPickerInput.xml", this);
   mpInputHandler->enterContext("Main");

   int numLeaders = gDatabase.getNumberLeaders();

   if (gConfig.isDefined(cConfigAlpha))
      mLeaderData.setNumber(3);
   else
      mLeaderData.setNumber(9);

   // initialize the leaders
   // random unsc
   int8 leaderIndex = 0;
   long civID = gDatabase.getCivID("UNSC");
   mLeaderData[leaderIndex].mFlashCivID=BLeaderPickerData::cLeaderPickerCiv0;
   mLeaderData[leaderIndex].mCiv=(int8)civID;
   mLeaderData[leaderIndex].mLeader=-1;
   mLeaderData[leaderIndex].mImageName="img://art\\ui\\flash\\shared\\textures\\pregame\\leaderImages\\randomUnsc.ddx";
   mLeaderData[leaderIndex].mNameLocID=gDatabase.getLocStringIndex(23718);
   mLeaderData[leaderIndex].mDescriptionLocID=gDatabase.getLocStringIndex(23719);

   // regular UNSC
   for (int i=0; i<numLeaders; i++)
   {
//-- FIXING PREFIX BUG ID 1498
      const BLeader* pLeader = gDatabase.getLeader(i);
//--
      if ( (pLeader->mLeaderCivID == civID) && !pLeader->mTest)
      {
         leaderIndex++;
         if (gConfig.isDefined(cConfigAlpha))
         {
            // don't add more than we should
            if (leaderIndex>=3)
               break;
         }
         mLeaderData[leaderIndex].mFlashCivID=BLeaderPickerData::cLeaderPickerCiv0;
         mLeaderData[leaderIndex].mCiv=(int8)civID;
         mLeaderData[leaderIndex].mLeader=(int8)i;
         mLeaderData[leaderIndex].mImageName=pLeader->mFlashPortrait;
         mLeaderData[leaderIndex].mNameLocID=pLeader->mNameIndex;
         mLeaderData[leaderIndex].mDescriptionLocID=pLeader->mDescriptionIndex;
      }
   }

   if (!gConfig.isDefined(cConfigAlpha))
   {
      // random all
      leaderIndex++;
      mLeaderData[leaderIndex].mFlashCivID=BLeaderPickerData::cLeaderPickerRandomCiv;
      mLeaderData[leaderIndex].mCiv=-1;
      mLeaderData[leaderIndex].mLeader=-1;
      mLeaderData[leaderIndex].mImageName="img://art\\ui\\flash\\shared\\textures\\pregame\\leaderImages\\random.ddx";
      mLeaderData[leaderIndex].mNameLocID=gDatabase.getLocStringIndex(23722);
      mLeaderData[leaderIndex].mDescriptionLocID=gDatabase.getLocStringIndex(23723);


      // covenant civ
      civID = gDatabase.getCivID("Covenant");

      // covenant
      for (int i=0; i<numLeaders; i++)
      {
//-- FIXING PREFIX BUG ID 1499
         const BLeader* pLeader = gDatabase.getLeader(i);
//--
         if ( (pLeader->mLeaderCivID == civID) && !pLeader->mTest)
         {
            leaderIndex++;
            mLeaderData[leaderIndex].mFlashCivID=BLeaderPickerData::cLeaderPickerCiv1;
            mLeaderData[leaderIndex].mCiv=(int8)civID;
            mLeaderData[leaderIndex].mLeader=(int8)i;
            mLeaderData[leaderIndex].mImageName=pLeader->mFlashPortrait;
            mLeaderData[leaderIndex].mNameLocID=pLeader->mNameIndex;
            mLeaderData[leaderIndex].mDescriptionLocID=pLeader->mDescriptionIndex;
         }
      }

      // random Covenant
      leaderIndex++;
      mLeaderData[leaderIndex].mFlashCivID=BLeaderPickerData::cLeaderPickerCiv1;
      mLeaderData[leaderIndex].mCiv=(int8)civID;
      mLeaderData[leaderIndex].mLeader=-1;
      mLeaderData[leaderIndex].mImageName="img://art\\ui\\flash\\shared\\textures\\pregame\\leaderImages\\randomCov.ddx";
      mLeaderData[leaderIndex].mNameLocID=gDatabase.getLocStringIndex(23720);
      mLeaderData[leaderIndex].mDescriptionLocID=gDatabase.getLocStringIndex(23721);
   }

   // send this data to Flash
   int firstLeaderSlot=0;
   if (gConfig.isDefined(cConfigAlpha))
      firstLeaderSlot=3;

   initLeaders((int8)firstLeaderSlot, (int8)mLeaderData.getNumber());

   // set up all the leaders in flash
   for (int i=0; i<mLeaderData.getNumber(); i++)
   {
      loadLeader((int8)i, mLeaderData[i].mImageName, mLeaderData[i].mFlashCivID, gDatabase.getLocStringFromIndex(mLeaderData[i].mNameLocID), gDatabase.getLocStringFromIndex(mLeaderData[i].mDescriptionLocID));
   }

   initResolution();

   mTitle.init(this, "mTitle");
   mTitle.setText(gDatabase.getLocStringFromID(25288));

   setFlag(cFlagInitialized, true);
   return true;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BUILeaderPicker::deinit()
{
   if (mpMovie)
   {
      mBtnBar.deinit();

      gFlashGateway.unregisterEventHandler(mpMovie, mSimEventHandle);
      gFlashGateway.releaseInstance(mpMovie);
   }
   mpMovie = NULL;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BUILeaderPicker::enter()
{
   if (!getFlag(cFlagInitialized))
      return;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BUILeaderPicker::leave()
{
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BUILeaderPicker::update(float elapsedTime)
{   
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BUILeaderPicker::renderBegin()
{   
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BUILeaderPicker::render()
{   
   SCOPEDSAMPLEID(FlashUIGame, 0xFFFF0000);
   if (mPanelVisible && mpMovie)
      mpMovie->render();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BUILeaderPicker::renderEnd()
{
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BUILeaderPicker::handleInput(int port, BInputEventType event, BInputControlType controlType, BInputEventDetail& detail)
{
   bool result = false;
   if (mpInputHandler)
      result = mpInputHandler->handleInput(port, event, controlType, detail);

   return result;
}

//----------------------------------------------------------------------------
// ----- IInputControlEventHandler - callback handler
//----------------------------------------------------------------------------
bool BUILeaderPicker::executeInputEvent(long port, long event, long controlType, BInputEventDetail& detail, const BSimString& command, const BInputControl* pInputControl)
{
   bool handled = false;
   if (command == "navLeft")
   {
      // working around an artifact of how this screen is brought up and the input system being used.
      if ( controlType==cTriggerLeft )
      {
         if (event == cInputEventCommandStart)
            mInputWaitForStart = false;

         if (mInputWaitForStart)
            return false;
      }

      if (controlType==cStickLeftLeft)
      {
         if (detail.mX == 0.0f)
            return false;
      }
      handled=changeLeaderSelection(false);
   }
   else if (command == "navRight")
   {
      // working around an artifact of how this screen is brought up and the input system being used.
      if ( controlType==cTriggerRight )
      {
         if (event == cInputEventCommandStart)
            mInputWaitForStart = false;

         if (mInputWaitForStart)
            return false;
      }

      if (controlType==cStickLeftRight)
      {
         if (detail.mX == 0.0f)
            return false;
      }
      handled=changeLeaderSelection(true);
   }   
   else
   {
      // ready, cancel, inviteFriends, playerOptions, gameOptions, changeLobby, switchTeams, kick
      if (mpEventHandler)
         handled=mpEventHandler->leaderPickerEvent(command);
   }

   if (handled)
   {
      // execute the sound that goes with this command.
      gSoundManager.playCue(pInputControl->getSoundEvent().getPtr());

   }
   return handled;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BUILeaderPicker::receiveEvent(const BEvent& event, BThreadIndex threadIndex)
{
   if (threadIndex == cThreadIndexSim)
   {
      switch (event.mEventClass)
      {
         case BFlashManager::cFlashEventFSCommand:
         {
//-- FIXING PREFIX BUG ID 1500
            const BFlashManager::BFSCommandPayload* pPayload = static_cast<BFlashManager::BFSCommandPayload*>(event.mpPayload);
//--
            if (pPayload->mCommand=="window")
            {
               if (pPayload->mArgs=="close")
               {
                  BSimString temp;
                  temp.set("close");
                  if (mpEventHandler)
                     mpEventHandler->leaderPickerEvent(temp);

                  setIsVisible(false); 
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
BManagedTextureHandle BUILeaderPicker::getRenderTargetTexture()
{
   if (mpMovie)
      return mpMovie->mRenderTargetHandle;

   return cInvalidManagedTextureHandle;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BUILeaderPicker::setDimension(int x, int y, int width, int height)
{
   if (!mpMovie)
      return;

   mpMovie->setDimension(x, y, width, height);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BUILeaderPicker::show()
{
   // based on the civ and leader, figure out who the selected leader is and set him.
   setSelectedLeader(mSelectedLeader);
   easeIn(true);
   setIsVisible(true);

   mInputWaitForStart = true;
   mpInputHandler->reset();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BUILeaderPicker::hide()
{ 
   easeIn(false);             // the FScommand will turn off the rendering when we are done
   // setIsVisible(false); 
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BUILeaderPicker::setCurrentLeader(int8 civ, int8 leader)
{
   for (int i=0; i<mLeaderData.getNumber(); i++)
   {
      if ( (civ==mLeaderData[i].mCiv) && (leader==mLeaderData[i].mLeader) )
      {
         mSelectedLeader=(int8)i;
         break;
      }
   }
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BUILeaderPicker::setPreviousLeader(int8 civ, int8 leader)
{
   setCurrentLeader(civ, leader);
   int8 currentLeader = mSelectedLeader;
   currentLeader--;

   if (currentLeader<0)
      currentLeader=(int8)(mLeaderData.getNumber()-1);     // wrap

   mSelectedLeader=currentLeader;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BUILeaderPicker::setNextLeader(int8 civ, int8 leader)
{
   setCurrentLeader(civ, leader);
   int8 currentLeader = mSelectedLeader;
   currentLeader++;

   if (currentLeader>=mLeaderData.getNumber() )
      currentLeader=0;  // wrap

   mSelectedLeader=currentLeader;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const BLeaderPickerData* BUILeaderPicker::getCurrentLeader()
{
   if ( (mSelectedLeader < 0) || (mSelectedLeader>=mLeaderData.getNumber()) )
      return NULL;

   return &mLeaderData[mSelectedLeader];
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BUILeaderPicker::setSelectedLeader(int8 selectedLeader)
{
   if (selectedLeader >= mLeaderData.getNumber())
      return;

   mSelectedLeader = selectedLeader;

   GFxValue values[1];
   values[0].SetNumber(selectedLeader);
   mpMovie->invokeActionScript(getProtoASFunctionName(eASFunctionSetSelectedLeader), values, 1);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BUILeaderPicker::easeIn(bool easeIn)
{
   GFxValue values[1];
   values[0].SetBoolean(easeIn);
   mpMovie->invokeActionScript(getProtoASFunctionName(eASFunctionEaseIn), values, 1);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BUILeaderPicker::initLeaders(int8 firstLeaderSlot, int8 numLeaders)
{
   GFxValue values[2];
   values[0].SetNumber(firstLeaderSlot);
   values[1].SetNumber(numLeaders);
   mpMovie->invokeActionScript(getProtoASFunctionName(eASFunctionInitLeaders), values, 2);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BUILeaderPicker::loadLeader(int8 slot, const BSimString& leaderImage, int8 civ, const BUString& leaderName, const BUString& leaderDescription)
{
   GFxValue values[5];
   values[0].SetNumber(slot);
   values[1].SetString(leaderImage.getPtr());
   values[2].SetNumber(civ);
   values[3].SetStringW(leaderName.getPtr());
   values[4].SetStringW(leaderDescription.getPtr());

   mpMovie->invokeActionScript(getProtoASFunctionName(eASFunctionLoadLeader), values, 5);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BUILeaderPicker::changeLeaderSelection(bool goRight)
{
   int8 currentLeader = mSelectedLeader;
   if (goRight)
      currentLeader++;
   else
      currentLeader--;

/*
   if ( (currentLeader<0) || (currentLeader>=mLeaderData.getNumber() ) )
      return false;
*/

   if (currentLeader<0)
   {
      currentLeader=(int8)(mLeaderData.getNumber()-1);     // wrap
   }
   else if (currentLeader>=mLeaderData.getNumber() )
   {
      currentLeader=0;  // wrap
   }


   mSelectedLeader=currentLeader;

   GFxValue values[1];
   values[0].SetBoolean(goRight);
   mpMovie->invokeActionScript(getProtoASFunctionName(eASFunctionChangeSelection), values, 1);
   return true;
}

