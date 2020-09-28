//==============================================================================
// hintengine.h
//
// 
//
// Copyright (c) 2007 Ensemble Studios
//==============================================================================

// Includes

#include "common.h"
#include "configsgame.h"
#include "database.h"
#include "game.h"
#include "gamedirectories.h"
#include "HintManager.h"
#include "player.h"
#include "uimanager.h"
#include "user.h"
#include "usermanager.h"
#include "world.h"

// xsystem
#include "xmlreader.h"
#include "xmlwriter.h"

#include "hintengine.h"

class BConcept;

IMPLEMENT_FREELIST(BConcept, 5, &gSimHeap);


BConceptDatabase gBConceptDatabase;

GFIMPLEMENTVERSION(BHintEngine, 1);
enum
{
   cMarkerHintEngine=10000,
   cMarkerConcept,
   cMarkerParameterPage,
};

//==============================================================================
// BConceptDatabase::load
//==============================================================================
bool BConceptDatabase::load()
{
   if(mConcepts.size() > 0)
      return true; //already loaded

   BXMLReader reader;
   if (!reader.load(cDirData, "concepts.xml", XML_READER_LOAD_DISCARD_ON_CLOSE))
      return false;
   BXMLNode rootNode(reader.getRootNode());

   int conceptCount = rootNode.getNumberChildren();
   mConcepts.reserve(conceptCount);

   for (int i = 0; i < conceptCount; i++)
   {
      BXMLNode node(rootNode.getChild(i));
      BConceptProtoData *pProtoConcept = new BConceptProtoData();
      if (!pProtoConcept)
         return false;

      if (!pProtoConcept->load(node))
      {
         delete pProtoConcept;
         return false;
      }
      mConcepts.add(pProtoConcept);

		if(pProtoConcept->mID > mLargestDBID)
			mLargestDBID = pProtoConcept->mID;
   }

   return true;

}




//==============================================================================
// BConceptProtoData::load
//==============================================================================
bool BConceptProtoData::load(BXMLNode root)
{
   initValues();

   // Read id
   root.getAttribValueAsInt("id", mID);

	// Read dev name
   root.getAttribValueAsString("name", mDevName);

	// Read InitialWaitTime
   root.getAttribValueAsFloat("InitialWaitTime", mInitialWaitTime);

   

   // Iterate through children
   long numNodes = root.getNumberChildren();
   for (long i = 0; i < numNodes; i++)
   {
      BXMLNode node(root.getChild(i));
      const BPackedString name(node.getName());

      BSimString tempStr;
      if(name=="MessageStringID")
      {
         long id;
         if(node.getTextAsLong(id))
            mMessageStringID=gDatabase.getLocStringIndex(id);
      }
		else if(name=="InitialWaitTime")
      {
         float f;
         if(node.getTextAsFloat(f))
             mInitialWaitTime = f;
      }
		else if(name=="MessageDisplayTime")
      {
         float f;
         if(node.getTextAsFloat(f))
				mMessageDisplayTime = f;
		}

      else if(name=="TimesPerGame")
      {
         int i;
         if(node.getTextAsInt(i))
				mTimesPerGame = i;
		}
		else if(name=="MaxGamesAllowed")
      {
         int i;
         if(node.getTextAsInt(i))
				mMaxGamesAllowed = i;
		}
		else if(name=="CoolDownTime")
      {
         float f;
         if(node.getTextAsFloat(f))
				mCoolDownTime = f;
		}
		else if(name=="CoolDownIncrement")
      {
         float f;
         if(node.getTextAsFloat(f))
				mCoolDownIncrement = f;
		}
		else if(name=="Prerequisites")
      {
         BSimString varNodeText;
         if(node.getText(varNodeText))
         {
            BSimString token;
            long strLen = varNodeText.length();
            long loc = token.copyTok(varNodeText, strLen, -1, B(","));
            while (loc != -1)
            {
               mPrerequisites.add(token.asLong());
               loc = token.copyTok(varNodeText, strLen, loc+1, B(","));
            }
         }
		}

	}
	return true;
}

//==============================================================================
// BConceptProtoData::BConceptProtoData                                                
//==============================================================================
BConceptProtoData::BConceptProtoData() :
   mID(-1),
   mMessageStringID(-1),
   mbActivateBeforePrereqs(false),
   mInitialWaitTime(0.0f),
   mMessageDisplayTime(0.0f),
   mTimesPerGame(0),
   mMaxGamesAllowed(0),
   mCoolDownTime(300.0f),
   mCoolDownIncrement(0.0f)
{
}

//==============================================================================
// BConceptProtoData::initValues                                                
//==============================================================================
void BConceptProtoData::initValues()
{
   mCoolDownTime = 300;
   mCoolDownIncrement = 0;
}

//==============================================================================
// BConcept::onAcquire
//==============================================================================
void BConcept::onAcquire()
{
   mHasCommand = false;
   mNewCommand = 0;
   mStateChanged = false;
   mNewState = 0;
   mHasPreconditionResult = false;
   mPreconditionResult = BConceptPrecondition::cNotSet;
   mPreconditionTime = 0;
   mState = BConceptState::cIdle;
   mID = -1;
   mGamesReinforced = 0;
   mTimesReinforced = 0;
   mHintDisplayedCount = 0;
   mTimesReinforcedThisGame = 0;
   mbEventReady = false;
   mbActive = false;
   mbPermission = false;
   mInitialWaitTimeRemaining = 0.0f;
   mTerminalWaitTimeRemaining = 0.0f;
   mCoolDownTimer = 0;
   mLastCoolDownAmount = 0;
   mCoolDownTimerAccumulator = 0.0f;
   mpParentHint = NULL;
   mpHintEngine = NULL;
   mpProtoConcept = NULL;
   mbPrereqsMet = false;
   mbDirtyProfile = false;
   mpPlayer = NULL;
}

//==============================================================================
// BConcept::setState
//==============================================================================
void BConcept::setState(BYTE state)
{
   if(mState == state)
      return;
   mState=state;
}

//==============================================================================
// BConcept::setActive
//==============================================================================
void BConcept::setActive()
{
   setState(BConceptState::cActive);

   mInitialWaitTimeRemaining = mpProtoConcept->mInitialWaitTime;

	mbActive = true;

   setNewCommand(BConceptCommand::cInit); //activate

}

//==============================================================================
// BConcept::setNotActive
//==============================================================================
void BConcept::setNotActive()
{
   //Qs... what if showing hint?

	mbActive = false;
 
   clearChildren(); //clear sub hints

   setNewCommand(BConceptCommand::cDeactivate);  //also calls hide in template...

   setState(BConceptState::cIdle);
	
   
   //RESET all of the precondition wait timers.
   mPreconditionTime = gWorld->getGametime();
   mpHintEngine->resetIdleReadyTimers();
}

//==============================================================================
// BConcept::setNotActive
//==============================================================================
void BConcept::setComplete()
{
	//mCompleted = true;

	mTimesReinforced++;
   mTimesReinforcedThisGame++;

   //First time learned this game?
   if(mTimesReinforcedThisGame == 1)
   {
      mGamesReinforced++;
   }

   mpHintEngine->updateProfile();

	mbDirtyProfile = true;

	//what else??
   //need to clean up events

   setNewCommand(BConceptCommand::cHide);

   ///is there better state for something that is terminal..
   setState(BConceptState::cTerminal);  
   mTerminalWaitTimeRemaining = 5.0;


   mCoolDownTimerAccumulator += mpProtoConcept->mCoolDownIncrement;


   mLastCoolDownAmount = (DWORD)(1000 * (mpProtoConcept->mCoolDownTime + mCoolDownTimerAccumulator));
   mCoolDownTimer = (DWORD)(gWorld->getGametime() + mLastCoolDownAmount);


   //RESET all of the precondition wait timers.
   mPreconditionTime = gWorld->getGametime();
   mpHintEngine->resetIdleReadyTimers();


         //stays terminal for a certain about of time then goes idle
         //gets reset if used as subhint
	//onComplete
	//	Hide hint 
	//	profile stuff
   //		active->idle
}

//==============================================================================
// BConcept::show
//==============================================================================
void BConcept::show()
{
   //This is BConcept::Show
   setNewCommand(BConceptCommand::cShow);
   setState(BConceptState::cShown);
   setNotReady();

   mHintDisplayedCount++;
}

//==============================================================================
// BConcept::hide //? need push?
//==============================================================================
void BConcept::hide()
{
   setNewCommand(BConceptCommand::cHide);
   setState(BConceptState::cActive);
   //setNotReady(); //?
}

//==============================================================================
// BConcept::push
//==============================================================================
void BConcept::push()
{
   setNewCommand(BConceptCommand::cHide);
   setState(BConceptState::cActive);
   setWaitTime(2);  //need to tune this
}

//==============================================================================
// BConcept::restart
//==============================================================================
void BConcept::restart()
{
   setActive();
   //anything else?
}

//==============================================================================
// BConcept::setWaitTime
//==============================================================================
void BConcept::setWaitTime(float time)
{
   mInitialWaitTimeRemaining = time;
}
//==============================================================================
// BConcept::update
//==============================================================================
void BConcept::update(float elapsedTime)
{
   if(mState == BConceptState::cActive)
   {
	   if(mInitialWaitTimeRemaining > 0)
	   {
		   mInitialWaitTimeRemaining -= elapsedTime;
		   if(mInitialWaitTimeRemaining <= 0)
		   {
			   setReady();  //ready to show if allowed
		   }
	   }
   }
   if(mState == BConceptState::cTerminal)
   {
      if(mTerminalWaitTimeRemaining > 0)
      {
         mTerminalWaitTimeRemaining -= elapsedTime;
		   if(mTerminalWaitTimeRemaining <= 0)
		   {
			   setNotActive();
		   }
      }
      //stays terminal for a certain about of time then goes idle
      //gets reset if used as subhint ?
   }


	//check prereqs?
	//if prereqs are met, then set this event to be active


}

//==============================================================================
// BConcept::update
//==============================================================================
void BConcept::setReady()
{
	mbEventReady = true;
	//cancel ready subcription
}
	
//==============================================================================
// BConcept::update
//==============================================================================
void BConcept::setNotReady()  //Redundant?
{
	mbEventReady = false;
}

//==============================================================================
// BConcept::update
//==============================================================================
void BConcept::loadProfileData()
{
	//not impl
}
//==============================================================================
// BConcept::setNewPreconditionResult
//==============================================================================
void BConcept::setNewPreconditionResult(BYTE preconditionResult)
{
   //consider reset only if going up?
   if(preconditionResult != mPreconditionResult)
   {
      mPreconditionTime = gWorld->getGametime();
   }
   mHasPreconditionResult = true; 
   mPreconditionResult = preconditionResult;

}
 //==============================================================================
// BConcept::getTimeWithPrecondition
// how long has the same result.  this resets if the result changes
//==============================================================================
float BConcept::getTimeWithPrecondition(DWORD gameTime)
{
   if(mPreconditionTime > gameTime)
      return 0;
   return (gameTime - mPreconditionTime) / 1000.0f;
}


bool BConcept::isCoolingDown(DWORD gameTime)
{
   return gameTime < mCoolDownTimer;
}

//==============================================================================
// BConcept::addChild
//==============================================================================
void BConcept::addChild(BConcept* pChild)
{
   pChild->mpParentHint = this;
   mSubHints.uniqueAdd(pChild);
}

//==============================================================================
// BConcept::clearChildren
//==============================================================================
void BConcept::clearChildren()
{
   for(uint i = 0; i< mSubHints.size(); i++)
   {
      mSubHints[i]->mpParentHint = NULL;
      mSubHints[i]->clearChildren();
      mSubHints[i]->setNotActive();
   }

   mSubHints.clear();
}
//==============================================================================
// BConcept::walkSubHints
//==============================================================================
void BConcept::walkSubHints(BDynamicArray<BConcept*> &subHints)
{
   for(uint i=0; i < mSubHints.size(); i++)
   {
      if(mSubHints[i] == NULL)
         continue;

      mSubHints[i]->walkSubHints(subHints);
      subHints.uniqueAdd(mSubHints[i]);
   }
}

//==============================================================================
// BConcept::setPermission
//==============================================================================
void BConcept::setPermission(bool allowed)
{
   mbPermission = allowed;
}

//==============================================================================
// BConcept::getPermission
//==============================================================================
bool BConcept::getPermission()
{
   ////afo this was overbound splitting it out to isLearnedMaxTimesPerGame
   //if(mTimesReinforced >= mpProtoConcept->mTimesPerGame )
   //{
   //   return false;
   //}
   return mbPermission;
}


//==============================================================================
// BConcept::isLearnedMaxTimesPerGame
//==============================================================================
bool BConcept::isLearnedMaxTimesPerGame()
{
   if(mTimesReinforcedThisGame >= mpProtoConcept->mTimesPerGame )
   {
      return true;
   }
   if(mGamesReinforced >= mpProtoConcept->mMaxGamesAllowed )
   {
      return true;
   }
   return false;
}

//==============================================================================
// BConcept::arePrerequisitesMet
//==============================================================================
bool BConcept::arePrerequisitesMet()
{
   for(uint i=0; i < mpProtoConcept->mPrerequisites.size(); i++)
   {
      int id = mpProtoConcept->mPrerequisites[i];
//-- FIXING PREFIX BUG ID 3970
      const BConcept* pConcept = mpHintEngine->getConceptByDBID(id);
//--
      if(pConcept == NULL)
         continue;
      
      if(pConcept->mTimesReinforced == 0)
      {
         return false; //NOT LEARNED AT LEAST ONCE YET..ever
      }
   }

   return true;
}



//==============================================================================
// BConcept::isMainConceptAllowed
//==============================================================================
bool BConcept::isMainConceptAllowed()
{
   if(getPermission() == false)
      return false;
   if(arePrerequisitesMet() == false)
      return false;
   if(isLearnedMaxTimesPerGame() == true)
      return false;

   return true;
}


//==============================================================================
// BConcept::initRuntimeData
//==============================================================================
void BConcept::initRuntimeData(BHintEngine* pHintEngine, BPlayer* pPlayer, BConceptProtoData* pProtoConcept)
{
	mpPlayer = pPlayer;
	mpProtoConcept = pProtoConcept;

   //note: 1:1 ratio
   mID = pProtoConcept->mID;

   mpHintEngine = pHintEngine;

   reset();
}

//==============================================================================
// BConcept::reset
//==============================================================================
void BConcept::reset()
{
   //Runtime data
	mbActive = false;
 	mbPrereqsMet = false;
   mbDirtyProfile = false;
   mbEventReady = false;

   mCoolDownTimer = 0;
   mCoolDownTimerAccumulator = 0;
   mLastCoolDownAmount = 0;
   mbPermission = false;
   mTimesReinforced = 0;
   mGamesReinforced = 0;
   mTimesReinforcedThisGame = 0;

   mPreconditionTime = 0;

   //state!
   mState = BConceptState::cIdle;
   mHasPreconditionResult = false;
   mPreconditionResult = BConceptPrecondition::cNotSet;
   mStateChanged = false;
   mHasCommand = false;

}
//==============================================================================
// BConcept::resetCoolDown
//==============================================================================
void BConcept::resetCoolDown(bool resetAccumlator)
{
   mCoolDownTimer = 0;
   mLastCoolDownAmount = 0;
   if(resetAccumlator)
   {
      mCoolDownTimerAccumulator = 0;
      //
      mInitialWaitTimeRemaining = 0.1;
      //mState = BConceptState::cActive;
      //mbEventReady = true;
   }
   
   mPreconditionTime = 0;  //would make this instantly ready to go
}

//==============================================================================
// BConcept::resetIdleReadyTimer
//==============================================================================
void BConcept::resetIdleReadyTimer()
{
   DWORD timer = gWorld->getGametime() - mPreconditionTime;

   if(mLastCoolDownAmount > timer)
      mLastCoolDownAmount = mLastCoolDownAmount - timer;
   else
      mLastCoolDownAmount = 0;

   //if(mLastCoolDownAmount < 0)

   //Wait precondition wait time + cooldown time
   mPreconditionTime = gWorld->getGametime() + mLastCoolDownAmount;   
}

//==============================================================================
// BHintEngine::BHintEngine
//==============================================================================
BHintEngine::BHintEngine(BPlayer *pPlayer) :
   mTimeSinceLastHint(0.0f),
   mbHintMessageOn(false),
   mMinTimeBetweenHints(30.0f),
   mRescoreRate(1.0f),
   mTimeActivateWait(300.0f),
   mTimeActivateGood(30.0f),
   mTimeActivateUrgent(1.0f),
   mWaitForNextRescore(1.0f),
   mpTrackingData(NULL),
   mNeedsLazyUpdate(true)
{
	mpPlayer = pPlayer;
   mLastGameTime = gWorld->getGametime();
}

//==============================================================================
// BHintEngine::~BHintEngine
//==============================================================================
BHintEngine::~BHintEngine() 
{
 
}

//==============================================================================
// BHintEngine::init
//==============================================================================
void BHintEngine::initialize()
{
   if (gConfig.isDefined(cEnableHintSystem) == false)
      return;


   //load profile data
	
	//load concepts
   gBConceptDatabase.load();

	//load conepts into id based array
	
	//settings
   mRescoreRate = 1;
	mMinTimeBetweenHints = 30;

   mTimeActivateWait = 300;
   mTimeActivateGood = 30;
   mTimeActivateUrgent = 1;
  
	//runtime
	mTimeSinceLastHint = 0;
	mbHintMessageOn = false;
   mWaitForNextRescore = mRescoreRate;

	int highestDbid = gBConceptDatabase.mLargestDBID;

   //watch for id probs?
	mUserConcepts.resize(highestDbid + 1);
	for(int i=0; i < highestDbid; i++)
	{
		mUserConcepts[i] = NULL;
	}

	int numConcepts = highestDbid;//gBConceptDatabase.mConcepts.size();
	for(int i=0; i < numConcepts; i++)
	{
		BConceptProtoData* pProtoConcept = gBConceptDatabase.mConcepts[i];
      if(pProtoConcept == NULL)
         continue; //concept was deleted
		BConcept* pConcept = BConcept::getInstance();
		pConcept->initRuntimeData(this, mpPlayer, pProtoConcept);
		mUserConcepts[pConcept->getID()] = pConcept;
	}

   //mpTrackingData = this->mpUser->getPlayer()->getTrackingData();
   mpTrackingData = NULL;


   mNeedsLazyUpdate = true;

   mLastGameTime = gWorld->getGametime();
}

//==============================================================================
// BHintEngine::reset
//==============================================================================
void BHintEngine::reset( bool needsUpdate )
{
//runtime
	mTimeSinceLastHint = 0;
	mbHintMessageOn = false;
   mWaitForNextRescore = mRescoreRate;

   for(uint i=0; i < mUserConcepts.size(); i++)
   {
      BConcept* pConcept = mUserConcepts[i];
      if(pConcept == NULL)
         continue;
      pConcept->reset();
   }

   //if(mpTrackingData == NULL)
   {
      mNeedsLazyUpdate = needsUpdate;
   }

   mLastGameTime = gWorld->getGametime();
}

//==============================================================================
// BHintEngine::loadProfile
//==============================================================================
void BHintEngine::loadProfile()
{
   if (gConfig.isDefined(cHintSystemResetProfile) == true || gConfig.isDefined("ForceHintReset"))
   {
      gConfig.remove("ForceHintReset");
      return;
   }


   if(mpTrackingData != NULL)
   {
      //mpTrackingData->
      const BDynamicSimBYTEArray ids = mpTrackingData->getConceptIDs();
      const BDynamicSimBYTEArray timesReinforced = mpTrackingData->getConceptTimesReinforced();
      for(uint i=0; i<ids.size(); i++)
      {
         BConcept* pConcept = this->getConceptByDBID((int)ids[i]);
         if(pConcept)
            pConcept->setGamesReinforced(timesReinforced[i]);
         //if(timesReinforced[i] > 0)
         //{
         //   i = i;
         //}

      }
   }
}

//==============================================================================
// BHintEngine::updateProfile
//==============================================================================
void BHintEngine::updateProfile()
{
   if(mpTrackingData != NULL)
   {
      BDynamicSimBYTEArray ids;
      BDynamicSimBYTEArray timesReinforced;

      for(uint i=0; i < mUserConcepts.size(); i++)
      {
         BConcept* pConcept = mUserConcepts[i];
         if(pConcept == NULL)
            continue;
         ids.add((unsigned char)pConcept->getID());
         timesReinforced.add((unsigned char)pConcept->getGamesReinforced());
      }

      mpTrackingData->setConceptIDs(ids);
      mpTrackingData->setConceptTimesReinforced(timesReinforced);
   }
}



//==============================================================================
// BHintEngine::update
//==============================================================================
void BHintEngine::update(float elapsedTime)
{
   DWORD currentGametime = gWorld->getGametime();

   elapsedTime = (currentGametime - mLastGameTime) / 1000.0f;
   mLastGameTime = currentGametime;

   if(mNeedsLazyUpdate)
   {
      mNeedsLazyUpdate = false;
      mpTrackingData = mpPlayer->getTrackingData();
      loadProfile();
   }


   //do this periodically
   //request for scoring ..score over time?
   //could be smart and to this less if there is already a top hint active?
   if(mWaitForNextRescore > 0)
   {
      mWaitForNextRescore -= elapsedTime;
      if(mWaitForNextRescore <= 0)
      {
         for(uint i=0; i < mUserConcepts.size(); i++)
         {
            BConcept* pConcept = mUserConcepts[i];
            if(pConcept == NULL || pConcept->isMainConceptAllowed() == false)
               continue;
            if(pConcept->hasNewCommand())
               continue;

            pConcept->setNewCommand(BConceptCommand::cCheckPreconditions); 
         }   
         mWaitForNextRescore = mRescoreRate;
      }
   }

   //Updaate All Hints
   for(uint i=0; i < mUserConcepts.size(); i++)
   {
      if(mUserConcepts[i] == NULL)
         continue;
      mUserConcepts[i]->update(elapsedTime);

      ////update sub hints???
      //BDynamicArray<BConcept*>  subHints = mUserConcepts[i]->walkSubHints(true);
      //for(uint j=0; j < subHints.size(); j++)
      //{
      //   subHints[j]->update(elapsedTime);
      //}
   }


   //Check concept states
   bool hasShownConcept = false;
   bool hasActiveconcepts = false;
   bool hasReadyconcepts = false;
   bool hasReadySubConcept = false;
   BConcept* pReadyConcept = NULL;   
   BConcept* pReadySubConcept = NULL;
   BConcept* pShownConcept = NULL;

   static BDynamicArray<BConcept*> readyConcepts;
   readyConcepts.setNumber(0);
   
   for(uint i=0; i < mUserConcepts.size(); i++)
   {
      BConcept* pConcept = mUserConcepts[i];
      if(pConcept == NULL)// || pConcept->isMainConceptAllowed() == false)  //afo: just checked permission and completed before
         continue;                                                       //
      if(pConcept->getState() == BConceptState::cShown )
      {
         hasShownConcept = true;
         pShownConcept = pConcept;
      }
      if(pConcept->getState() == BConceptState::cActive)// && pConcept->isMainConceptAllowed() == true)
      {
         hasActiveconcepts = true;
         //if active check sub concepts
         //Sometimes the designers will want the sub concepts to show up first...
         static BDynamicArray<BConcept*>  subHints;
         subHints.setNumber(0);
         pConcept->walkSubHints(subHints);
         for(uint i=0; i < subHints.size(); i++)
         {
            BConcept* pSubConcept = subHints[i];
            if(pSubConcept == NULL)
               continue;
            if(pSubConcept->isReady() )
            {
               hasReadySubConcept = true;
               pReadySubConcept = pSubConcept;
               readyConcepts.add(pSubConcept);
            } 
         }
      }   
      if(pConcept->isReady() && pConcept->isTopLevelConcept())// && pConcept->isMainConceptAllowed() == true)
      {

         hasReadyconcepts = true;
         pReadyConcept = pConcept;
         readyConcepts.add(pReadyConcept);
      }     
   }


   //show ready sub hints
   if(hasShownConcept == false && hasReadySubConcept == true )
   { 
      pReadySubConcept->show();
   }   
   //show ready main hints
   else if(hasShownConcept == false && hasReadyconcepts == true && pReadyConcept->isMainConceptAllowed() == true)
   {
      pReadyConcept->show(); 
   }
   //ELSE, look for ready sub concepts that may need to bubble up
   else if(pShownConcept != NULL)
   {
      hasReadyconcepts = false;
      static BDynamicArray<BConcept*>  subHints;
      subHints.setNumber(0);
      pShownConcept->walkSubHints(subHints);
      for(uint i=0; i < subHints.size(); i++)
      {
         BConcept* pConcept = subHints[i];
         if(pConcept == NULL)// || pConcept->getPermission() == false)  //this is less checing than before..
            continue;
         if(pConcept->isReadyNoActiveCheck() )
         {
            hasReadyconcepts = true;
            pReadyConcept = pConcept;
         } 
      }
      //ready sub concept
      if( hasReadyconcepts == true)
      {
         pShownConcept->push();
         pReadyConcept->show(); 
      }
   }


   //Do we have and active/shown/or ready concept.   If so Bail.   
   //Only sub hints can be activated now.  
   if(hasActiveconcepts == true || hasReadyconcepts == true || hasShownConcept == true)
      return;


   //If nothing is active then score hints to find one to activate
   static BDynamicArray<BDynamicArray<BConcept*>> scoreBuckets;
   scoreBuckets.resize(BConceptPrecondition::cSize);
   for (uint i=0; i<scoreBuckets.size(); i++)
      scoreBuckets[i].setNumber(0);

   BConcept* pBestConcept = NULL;
   //score   
   //int bestscore = -1;
   //pick idle hints that could be activated
   for(uint i=0; i < mUserConcepts.size(); i++)
   {
      BConcept* pConcept = mUserConcepts[i];
      if(pConcept == NULL || pConcept->isMainConceptAllowed() == false)
         continue;
      if(pConcept->getState() != BConceptState::cIdle)
         continue;

      if(pConcept->hasPreconditionResult())
      {
         int precondValue = pConcept->getPreconditionResult();
         scoreBuckets[precondValue].add(pConcept);
      }
   }
   
   int topBucket = BConceptPrecondition::cNotSet;

   DWORD gameTime = gWorld->getGametime();
   //best bucket with entries (that are not cooling down)
   for(int i = BConceptPrecondition::cSize-1; i > BConceptPrecondition::cBlocked; i--)
   {   
      for(uint j=0; j < scoreBuckets[i].size(); j++)
      {
         if(scoreBuckets[i][j]->isCoolingDown(gameTime) == false)
         {
            topBucket = i;            
            break;
         }
      }      
      if(topBucket != BConceptPrecondition::cNotSet)
         break;
   }

   //find best time 
   float largestTime = 0;
   BDynamicArray<BConcept*> &topBucketItems = scoreBuckets[topBucket];  
   for(uint i=0; i < topBucketItems.size(); i++)
   {
      float time = topBucketItems[i]->getTimeWithPrecondition(gameTime);
      if(time > largestTime)
      {
         largestTime = time;
         pBestConcept = topBucketItems[i];
      }
   }


   bool okToActivate = false;

   //need way to delay activation
   if(topBucket == BConceptPrecondition::cWait && largestTime > mTimeActivateWait)
   {
      okToActivate = true;
   }
   if(topBucket == BConceptPrecondition::cGood && largestTime > mTimeActivateGood)
   {
      okToActivate = true;
   }
   if(topBucket == BConceptPrecondition::cUrgent && largestTime > mTimeActivateUrgent)
   {
      okToActivate = true;
   }

   //Activate best concept:
   if(okToActivate && pBestConcept != NULL)
   {
      //this is a top level concept!
      pBestConcept->setParentHint(NULL);

      pBestConcept->setActive();
   }

}

void BHintEngine::resetIdleReadyTimers()
{
   for(uint i=0; i < mUserConcepts.size(); i++)
   {
      BConcept* pConcept = mUserConcepts[i];
      if(pConcept == NULL)// || pConcept->isMainConceptAllowed() == false)  //afo: just checked permission and completed before
         continue;    
      pConcept->resetIdleReadyTimer();
   }

}


void BHintEngine::setPermission(bool allowAll, const BInt32Array& list)
{
   mAllowedConcepts.clear();
   if(allowAll)
   {      
      for(uint i=0; i < mUserConcepts.size(); i++)
      {
         if(mUserConcepts[i] == NULL)
            continue;
         mAllowedConcepts.add(mUserConcepts[i]->getID());
      }
   }
   else
   {
      mAllowedConcepts = list;
   }

   //process this..
   for(uint i=0; i < mUserConcepts.size(); i++)
   {
      BConcept* pConcept = mUserConcepts[i];
      if(pConcept == NULL)
         continue;
      int id = pConcept->getID();
      bool allowed = mAllowedConcepts.contains(id);
      pConcept->setPermission( allowed );
   }
}




void BHintEngine::scoreConcepts()
{
   //moved to BHintEngine::update for now  

}

void BHintEngine::onPrecondition(int conceptID, int precondition)
{
   BConcept* pConcept = getConceptByDBID(conceptID);
   if(pConcept == NULL)
      return;
   pConcept->setNewPreconditionResult((BYTE)precondition);
}
void BHintEngine::onStateChange(int conceptID, int state)
{
   BConcept* pConcept = getConceptByDBID(conceptID);
   if(pConcept == NULL)
      return;
   pConcept->setNewState((BYTE)state);

   //todo relocate this
   if(state == BConceptStateResult::cComplete)
   {
      pConcept->setComplete();
   }
   if(state == BConceptStateResult::cRestart)
   {
      pConcept->restart();
   }
   if(state == BConceptStateResult::cCancel)
   {
      pConcept->setNotActive();
   }
}

//==============================================================================
// BHintEngine::startSubHint
// initTime used if > 0 
//==============================================================================
void BHintEngine::startSubHint(int conceptID, int parentConceptID, float initTime = -1 )
{
   BConcept* pConcept = getConceptByDBID(conceptID);
   if(pConcept == NULL)
      return;

   pConcept->setActive();

   //overload the time of how long the subhint will wait before being ready
   if(initTime >= 0)
   {
      pConcept->setWaitTime(initTime);
   }

   BConcept* pParentConcept = getConceptByDBID(parentConceptID);
   if(pParentConcept == NULL)
      return;

   pParentConcept->addChild(pConcept);

   //todo
   //set up tree / parent relationship
}

void BHintEngine::stopSubHints(int conceptID)
{
   //implicit:  every hint that is not this one:
   //for(uint i=0; i < mUserConcepts.size(); i++)
   //{
   //   if(mUserConcepts[i] == NULL)
   //      continue;
   //   if(mUserConcepts[i]->getID() == conceptID)
   //      continue;
   //   mUserConcepts[i]->setNotActive();
   //}

   BConcept* pConcept = getConceptByDBID(conceptID);
   if(pConcept == NULL)
      return;

   pConcept->clearChildren();
}


//==============================================================================
// BHintEngine::hasStateChange
//==============================================================================
bool BHintEngine::hasStateChange(int conceptID)
{
   BConcept* pConcept = getConceptByDBID(conceptID);
   if(pConcept == NULL)
      return false;
   return pConcept->hasStateChanged();
   
}

//==============================================================================
// BHintEngine::hasCommand
//==============================================================================
bool BHintEngine::hasCommand(int conceptID)
{
   BConcept* pConcept = getConceptByDBID(conceptID);
   if(pConcept == NULL)
      return false;
   return pConcept->hasNewCommand();
}
//==============================================================================
// BHintEngine::popState
//==============================================================================
int BHintEngine::popState(int conceptID)
{
   BConcept* pConcept = getConceptByDBID(conceptID);
   if(pConcept == NULL)
      return -1;
   return pConcept->getNewState();

}

//==============================================================================
// BHintEngine::popCommand
//==============================================================================
int BHintEngine::popCommand(int conceptID)
{
   BConcept* pConcept = getConceptByDBID(conceptID);
   if(pConcept == NULL)
      return -1;
   return pConcept->getNewCommand();
}
   
//==============================================================================
// BHintEngine::peekState
//==============================================================================
int BHintEngine::peekState(int conceptID)
{
   BConcept* pConcept = getConceptByDBID(conceptID);
   if(pConcept == NULL)
      return -1;
   return pConcept->peekNewState();
}


//==============================================================================
// BHintEngine::resetConceptCoolDown
//==============================================================================
void BHintEngine::resetConceptCoolDown(int conceptID, bool resetAccumlator)
{
   BConcept* pConcept = getConceptByDBID(conceptID);
   if(pConcept == NULL)
      return;
   pConcept->resetCoolDown(resetAccumlator);
}


//==============================================================================
// BHintEngine::getConceptByDBID
//==============================================================================
BConcept* BHintEngine::getConceptByDBID(int dbid)
{
   for(uint i=0; i<mUserConcepts.size(); i++)
   {
      if(mUserConcepts[i] == NULL)
         continue;
      if(mUserConcepts[i]->getID() == dbid)
         return mUserConcepts[i];
   }
   return NULL;
}

//==============================================================================
// BHintEngine::displayHint
//==============================================================================
void BHintEngine::displayHint(BConcept* pConcept)
{
	//mpCurrentDisplayedHint = pConcept;
	//mbHintMessageOn = true;
}

//==============================================================================
// BHintEngine::removeHint
//==============================================================================
void BHintEngine::removeHint()
{
	//mbHintMessageOn = false;
	//mTimeSinceLastHint = 0;
}

//==============================================================================
// BHintEngine::getConceptPage
//==============================================================================
BParameterPage* BHintEngine::getConceptPage(int conceptID, int pageID)
{
   BConcept* pConcept = getConceptByDBID(conceptID);
   if(pConcept == NULL)
      return NULL;
   return pConcept->getPage(pageID);
}

//==============================================================================
// BConcept::getPage (these are 0 indexed!!)
//==============================================================================
BParameterPage* BConcept::getPage(uint page)
{
   //page(s) not initialized
   if(page >= mPages.size())
   {
      int numPages = mPages.size();
      int pagesRequested = page + 1;
      for(int i = numPages; i < pagesRequested; i++)
      {
         BParameterPage* pNewPage = new BParameterPage();
         pNewPage->clear();
         mPages.add(pNewPage);
      }
   }

   return mPages[page];
}

//==============================================================================
// BConcept::clearPages
//==============================================================================
void BConcept::clearPages()
{
   for(uint i=0; i< mPages.size(); i++)
   {
      mPages[i]->clear();
   }
}


//==============================================================================
// BParameterPage::clear
//==============================================================================
void BParameterPage::clear()
{
   //mVector;
   mSquadList.clear(); 
   mUnitList.clear(); 
   mEntityFilterSet.clearFilters();
   //mFloat;
   //mObjectType;
   
   mHasVector = false;
   mHasSquadList = false; 
   mHasUnitList = false; 
   mHasEntityFilterSet = false;
   mHasFloat = false;
   mHasObjectType = false; 
   mHasLocStringID = false;

}

//==============================================================================
//==============================================================================
bool BHintEngine::save(BStream* pStream, int saveType) const
{
   uint itemCount = mUserConcepts.size();
   GFWRITEVAR(pStream, uint, itemCount);
   GFVERIFYCOUNT(itemCount, 1000);
   for (uint i=0; i<itemCount; i++)
   {
      BConcept* pConcept = mUserConcepts[i];
      if (!pConcept)
         continue;
      int id = pConcept->getID();
      GFWRITEVAR(pStream, int, id);
      GFVERIFYCOUNT(id, 1000);
      GFWRITECLASSPTR(pStream, saveType, pConcept);
   }
   GFWRITEVAL(pStream, int, INT_MAX);

   GFWRITEVAR(pStream, float, mTimeSinceLastHint);

   //BDynamicArray<float> mHintShownTimes;
   //BDynamicArray<float> mHintClosedTimes;

   GFWRITEVAR(pStream, bool, mbHintMessageOn);

   //float mHintMessageTimeRemaining;
   //BConcept* mpCurrentDisplayedHint;

   GFWRITEARRAY(pStream, int32, mAllowedConcepts, uint16, 1000);

   //float mMinTimeBetweenHints;
   //float mRescoreRate;
   //float mTimeActivateWait;
   //float mTimeActivateGood;
   //float mTimeActivateUrgent;

   GFWRITEVAR(pStream, float, mWaitForNextRescore);

   //BPlayer* mpPlayer;

   GFWRITEVAR(pStream, DWORD, mLastGameTime);

   //BHumanPlayerAITrackingData* mpTrackingData;
   //bool mNeedsLazyUpdate;

   GFWRITEMARKER(pStream, cMarkerHintEngine);

   return true;
}

//==============================================================================
//==============================================================================
bool BHintEngine::load(BStream* pStream, int saveType)
{
   uint itemCount = 0;
   GFREADVAR(pStream, uint, itemCount);
   GFVERIFYCOUNT(itemCount, 1000);
   for (;;)
   {
      int id;
      GFREADVAR(pStream, int, id);
      if (id == INT_MAX)
         break;
      GFVERIFYCOUNT(id, 1000);
      BConcept* pConcept = getConceptByDBID(id);
      if (!pConcept)
      {
         BConcept concept;
         GFREADCLASS(pStream, saveType, concept);
      }
      else
         GFREADCLASSPTR(pStream, saveType, pConcept);
   }

   GFREADVAR(pStream, float, mTimeSinceLastHint);

   //BDynamicArray<float> mHintShownTimes;
   //BDynamicArray<float> mHintClosedTimes;

   GFREADVAR(pStream, bool, mbHintMessageOn);

   //float mHintMessageTimeRemaining;
   //BConcept* mpCurrentDisplayedHint;

   GFREADARRAY(pStream, int32, mAllowedConcepts, uint16, 1000);

   //float mMinTimeBetweenHints;
   //float mRescoreRate;
   //float mTimeActivateWait;
   //float mTimeActivateGood;
   //float mTimeActivateUrgent;

   GFREADVAR(pStream, float, mWaitForNextRescore);

   //BPlayer* mpPlayer;

   GFREADVAR(pStream, DWORD, mLastGameTime);

   //BHumanPlayerAITrackingData* mpTrackingData;
   mNeedsLazyUpdate = true;

   GFREADMARKER(pStream, cMarkerHintEngine);

   return true;
}

//==============================================================================
//==============================================================================
bool BConcept::save(BStream* pStream, int saveType) const
{
   GFWRITEVAR(pStream, bool, mHasCommand);
   GFWRITEVAR(pStream, BYTE, mNewCommand);
   GFWRITEVAR(pStream, bool, mStateChanged);
   GFWRITEVAR(pStream, BYTE, mNewState);
   GFWRITEVAR(pStream, bool, mHasPreconditionResult);
   GFWRITEVAR(pStream, BYTE, mPreconditionResult);
   GFWRITEVAR(pStream, DWORD, mPreconditionTime);
   GFWRITEVAR(pStream, BYTE, mState);
   //int mID;
   GFWRITEVAR(pStream, int, mGamesReinforced);
   GFWRITEVAR(pStream, int, mTimesReinforced);
   GFWRITEVAR(pStream, int, mHintDisplayedCount);
   GFWRITECLASSPTRARRAY(pStream, saveType, BParameterPage, mPages, uint16, 1000);
   GFWRITEVAR(pStream, int,  mTimesReinforcedThisGame);
   GFWRITEVAR(pStream, bool, mbEventReady);
   GFWRITEVAR(pStream, bool, mbActive);
   GFWRITEVAR(pStream, bool, mbPermission);
   GFWRITEVAR(pStream, float, mInitialWaitTimeRemaining);
   GFWRITEVAR(pStream, float, mTerminalWaitTimeRemaining);
   GFWRITEVAR(pStream, DWORD, mCoolDownTimer);
   GFWRITEVAR(pStream, DWORD, mLastCoolDownAmount);
   GFWRITEVAR(pStream, float, mCoolDownTimerAccumulator);

   uint8 subHitCount = (uint8)mSubHints.size();
   GFWRITEVAR(pStream, uint8, subHitCount);
   GFVERIFYCOUNT(subHitCount, 200);
   for (uint8 i=0; i<subHitCount; i++)
   {
      int id = (mSubHints[i] ? mSubHints[i]->getID() : -1);
      GFWRITEVAR(pStream, int, id);
      GFVERIFYCOUNT(id, 1000);
   }

   int parentHint = (mpParentHint ? mpParentHint->getID() : -1);
   GFWRITEVAR(pStream, int, parentHint);
   GFVERIFYCOUNT(parentHint, 1000);

   //BHintEngine* mpHintEngine;

   //BConceptProtoData* mpProtoConcept;
   GFWRITEVAR(pStream, bool, mbPrereqsMet);
   GFWRITEVAR(pStream, bool, mbDirtyProfile);
   //BPlayer* mpPlayer;

   GFWRITEMARKER(pStream, cMarkerConcept);

   return true;
}

//==============================================================================
//==============================================================================
bool BConcept::load(BStream* pStream, int saveType)
{
   GFREADVAR(pStream, bool, mHasCommand);
   GFREADVAR(pStream, BYTE, mNewCommand);
   GFREADVAR(pStream, bool, mStateChanged);
   GFREADVAR(pStream, BYTE, mNewState);
   GFREADVAR(pStream, bool, mHasPreconditionResult);
   GFREADVAR(pStream, BYTE, mPreconditionResult);
   GFREADVAR(pStream, DWORD, mPreconditionTime);
   GFREADVAR(pStream, BYTE, mState);
   //int mID;
   GFREADVAR(pStream, int, mGamesReinforced);
   GFREADVAR(pStream, int, mTimesReinforced);
   GFREADVAR(pStream, int, mHintDisplayedCount);
   GFREADCLASSPTRARRAY(pStream, saveType, BParameterPage, mPages, uint16, 1000);
   GFREADVAR(pStream, int,  mTimesReinforcedThisGame);
   GFREADVAR(pStream, bool, mbEventReady);
   GFREADVAR(pStream, bool, mbActive);
   GFREADVAR(pStream, bool, mbPermission);
   GFREADVAR(pStream, float, mInitialWaitTimeRemaining);
   GFREADVAR(pStream, float, mTerminalWaitTimeRemaining);
   GFREADVAR(pStream, DWORD, mCoolDownTimer);
   GFREADVAR(pStream, DWORD, mLastCoolDownAmount);
   GFREADVAR(pStream, float, mCoolDownTimerAccumulator);

   uint8 subHitCount = (uint8)mSubHints.size();
   GFREADVAR(pStream, uint8, subHitCount);
   GFVERIFYCOUNT(subHitCount, 200);
   mSubHints.setNumber(subHitCount);
   for (uint8 i=0; i<subHitCount; i++)
      mSubHints[i] = NULL;
   for (uint8 i=0; i<subHitCount; i++)
   {
      int id = (mSubHints[i] ? mSubHints[i]->getID() : -1);
      GFREADVAR(pStream, int, id);
      GFVERIFYCOUNT(id, 1000);
      mSubHints[i] = mpHintEngine->getConceptByDBID(id);
   }

   int parentHint = (mpParentHint ? mpParentHint->getID() : -1);
   GFREADVAR(pStream, int, parentHint);
   GFVERIFYCOUNT(parentHint, 1000);
   mpParentHint = mpHintEngine->getConceptByDBID(parentHint);

   //BHintEngine* mpHintEngine;

   //BConceptProtoData* mpProtoConcept;
   GFREADVAR(pStream, bool, mbPrereqsMet);
   GFREADVAR(pStream, bool, mbDirtyProfile);
   //BPlayer* mpPlayer;

   GFREADMARKER(pStream, cMarkerConcept);

   return true;
}

//==============================================================================
//==============================================================================
bool BParameterPage::save(BStream* pStream, int saveType) const
{
   GFWRITEVECTOR(pStream, mVector);
   GFWRITEARRAY(pStream, BEntityID, mSquadList, uint16, 1000);
   GFWRITEARRAY(pStream, BEntityID, mUnitList, uint16, 1000);
   GFWRITECLASS(pStream, saveType, mEntityFilterSet);
   GFWRITEVAR(pStream, float, mFloat);
   GFWRITEVAR(pStream, long, mObjectType);
   GFWRITEVAR(pStream, uint32, mLocStringID);
   GFWRITEVAR(pStream, bool, mHasVector);
   GFWRITEVAR(pStream, bool, mHasSquadList); 
   GFWRITEVAR(pStream, bool, mHasUnitList); 
   GFWRITEVAR(pStream, bool, mHasEntityFilterSet);
   GFWRITEVAR(pStream, bool, mHasFloat);
   GFWRITEVAR(pStream, bool, mHasObjectType);  
   GFWRITEVAR(pStream, bool, mHasLocStringID);
   GFWRITEMARKER(pStream, cMarkerParameterPage);
   return true;
}

//==============================================================================
//==============================================================================
bool BParameterPage::load(BStream* pStream, int saveType)
{
   GFREADVECTOR(pStream, mVector);
   GFREADARRAY(pStream, BEntityID, mSquadList, uint16, 1000);
   GFREADARRAY(pStream, BEntityID, mUnitList, uint16, 1000);
   GFREADCLASS(pStream, saveType, mEntityFilterSet);
   GFREADVAR(pStream, float, mFloat);
   GFREADVAR(pStream, long, mObjectType);
   GFREADVAR(pStream, uint32, mLocStringID);
   GFREADVAR(pStream, bool, mHasVector);
   GFREADVAR(pStream, bool, mHasSquadList); 
   GFREADVAR(pStream, bool, mHasUnitList); 
   GFREADVAR(pStream, bool, mHasEntityFilterSet);
   GFREADVAR(pStream, bool, mHasFloat);
   GFREADVAR(pStream, bool, mHasObjectType);  
   GFREADVAR(pStream, bool, mHasLocStringID);
   GFREADMARKER(pStream, cMarkerParameterPage);
   return true;
}
