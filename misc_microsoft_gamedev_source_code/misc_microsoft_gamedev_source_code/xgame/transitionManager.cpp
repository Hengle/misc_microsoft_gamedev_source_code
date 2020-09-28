//==============================================================================
// transitionManager.cpp
//
// Copyright (c) 2008 Ensemble Studios
//==============================================================================

// Includes
#include "common.h"
#include "transitionManager.h"
#include "generaleventmanager.h"
#include "image.h"
#include "renderDraw.h"
#include "primDraw2D.h"
#include "modemanager.h"
#include "modegame.h"
#include "timer.h"

//==============================================================================
// BFadeTransition::BFadeTransition
//==============================================================================
BFadeTransition::BFadeTransition() : 
   mStartingOpacity(0),
   mEndingOpacity(0),
   mTime(0),
   mFadeTime(0),
   mLeadIn(0),
   mLeadOut(0)
{
   mColor.set(0,0,0);
}

//==============================================================================
void BFadeTransition::update(float deltaTime)
{
   if (mTime < (mFadeTime + mLeadIn + mLeadOut))
      mTime += deltaTime;
}

//==============================================================================
void BFadeTransition::postRender()
{
   BASSERT(mFadeTime > 0.0f);

   float opacity;
   float pctComplete;

   if (mTime < mLeadIn)
      opacity = mStartingOpacity;
   else if (mTime >= (mLeadIn + mFadeTime))
      opacity = mEndingOpacity;
   else
   {
      pctComplete = min(((mTime-mLeadIn) / mFadeTime), 1.0f);

      if (mStartingOpacity > mEndingOpacity) // fading up
         opacity = (mStartingOpacity - mEndingOpacity) * (1.0f - pctComplete);
      else // ending > starting (fading down)
         opacity = (mEndingOpacity - mStartingOpacity) * pctComplete;
   }
   
   if(opacity != 0.0f)
   {
      DWORD color = ARGBToDWORD(int(opacity * 255), int(mColor.r * 255), int(mColor.g * 255), int(mColor.b * 255));      

      gRenderDraw.setRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
      gRenderDraw.setRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
      gRenderDraw.setRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
      gRenderDraw.setRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);
      gRenderDraw.setRenderState(D3DRS_ALPHATESTENABLE, FALSE);

      BPrimDraw2D::drawSolidRect2D(0, 0, (int)gRenderDraw.getDisplayMode().Width, (int)gRenderDraw.getDisplayMode().Height, 1.0f, 0.0f, 0.0f, 1.0f, color, color, cPosDiffuseVS, cDiffusePS);
   }
}

//==============================================================================
bool BFadeTransition::isComplete()
{
   // allow us to lock the cinematic
   if (mLeadOut < 0.0f)
      return false;

   return mTime >= (mFadeTime + mLeadIn + mLeadOut);
}

//==============================================================================
void BFadeTransition::setup(float startOpacity, float endOpacity, float fadeTime, float leadIn, float leadOut, const BColor &color)
{
   BASSERT(fabs(startOpacity-endOpacity) > 0);

   mStartingOpacity = startOpacity;
   mEndingOpacity = endOpacity;
   mFadeTime = fadeTime;
   mLeadIn = leadIn;
   mLeadOut = leadOut;
   mColor = color;
   mTime = 0.0f;
}

//==============================================================================
// BTransitionSequence::BTransitionSequence
//==============================================================================
BTransitionSequence::BTransitionSequence() : 
   mActiveTransition(0)
{
}

//==============================================================================
BTransitionSequence::~BTransitionSequence()
{
   for (int idx=0; idx<mTransitions.getNumber(); idx++)
   {
      delete mTransitions[idx];
   }
   mTransitions.clear();
   mActiveTransition = -1;
}

//==============================================================================
void BTransitionSequence::update(float deltaTime)
{  
   if (mActiveTransition >= mTransitions.getNumber())
      return;

   mTransitions[mActiveTransition]->update(deltaTime);

   if (mTransitions[mActiveTransition]->isComplete())
      mActiveTransition++;
}

//==============================================================================
void BTransitionSequence::postRender()
{
   if (mActiveTransition >= mTransitions.getNumber())
      return;

   mTransitions[mActiveTransition]->postRender();
}

//==============================================================================
bool BTransitionSequence::isComplete()
{
   // if we're done with everything we're complete
   if (mActiveTransition >= mTransitions.getNumber())
      return true;

   // if we're on the last transition, see if it is complete
   if (mActiveTransition == mTransitions.getNumber()-1)
      return mTransitions[mActiveTransition]->isComplete();

   // else we're somewhere else
   return false;
}

//==============================================================================
void BTransitionSequence::add(BTransition *transition)
{
   mTransitions.add(transition);
}


//==============================================================================
// BTransitionManager::BTransitionManager
//==============================================================================
BTransitionManager::BTransitionManager() : 
   mpTransition(NULL),
   mpCompleted(NULL),
   mTransitionCounter(0)
{
   mUpdateTimer = new BTimer();
}

//==============================================================================
BTransitionManager::~BTransitionManager()
{
   if (mpTransition)
      delete mpTransition;
   mpTransition = NULL;
   
   // jce [11/20/2008] -- Clean up update timer which wasn't getting deleted.
   delete mUpdateTimer;
   mUpdateTimer = NULL;
}

//==============================================================================
void BTransitionManager::update()
{
   float deltaTime=0.0f;

   if (!mUpdateTimer->isStarted())
   {
      mLastUpdateTime=0.0;
      mUpdateTimer->start();
   }

   deltaTime = (float)(mUpdateTimer->getElapsedSeconds() - mLastUpdateTime);
   mLastUpdateTime = mUpdateTimer->getElapsedSeconds();

   // don't update if we're supposed to be paused.
   if (gModeManager.inModeGame() && gModeManager.getModeGame()->getPaused())
      return;

   if (mpTransition)
   {
      mpTransition->update(deltaTime);
      if (mpTransition->isComplete())
      {
         gGeneralEventManager.eventTrigger(BEventDefinitions::cFadeCompleted, cInvalidPlayerID);         

         delete mpTransition;
         mpTransition = NULL;
      }
   }   
}

//==============================================================================
void BTransitionManager::postRender()
{
   if (mpTransition)
      mpTransition->postRender();
}

//==============================================================================
void BTransitionManager::doFadeDown(float fadeTime, float leadIn, float leadOut, const BColor& color, BGeneralEventSubscriber* pCompleted)
{
   BFadeTransition *fade = new BFadeTransition();
   fade->setup(0.0f, 1.0f, fadeTime, leadIn, leadOut, color);
   startTransition(fade, pCompleted);
}

//==============================================================================
void BTransitionManager::doFadeUp(float fadeTime, float leadIn, float leadOut, const BColor& color, BGeneralEventSubscriber* pCompleted)
{
   BFadeTransition *fade = new BFadeTransition();
   fade->setup(1.0f, 0.0f, fadeTime, leadIn, leadOut, color);
   startTransition(fade, pCompleted);
}

//==============================================================================
void BTransitionManager::doFadeDownHoldFadeUp(float fadeDown, float hold, float fadeUp, const BColor& color, BGeneralEventSubscriber* pCompleted)
{
   BFadeTransition *fDown = new BFadeTransition();
   fDown->setup(0.0f, 1.0f, fadeDown, 0.0f, hold, color);
   
   BFadeTransition *fUp = new BFadeTransition();
   fUp->setup(1.0f, 0.0f, fadeUp, 0.0f, 0.0f, color);
   
   BTransitionSequence *sequence = new BTransitionSequence();
   sequence->add(fDown);
   sequence->add(fUp);

   startTransition(sequence, pCompleted);
}

//==============================================================================
void BTransitionManager::doFadeUpHoldFadeDown(float fadeUp, float hold, float fadeDown, const BColor& color, BGeneralEventSubscriber* pCompleted)
{
   BFadeTransition *fUp = new BFadeTransition();
   fUp->setup(1.0f, 0.0f, fadeUp, 0.0f, hold, color);

   BFadeTransition *fDown = new BFadeTransition();
   fDown->setup(0.0f, 1.0f, fadeDown, 0.0f, 0.0f, color);

   BTransitionSequence *sequence = new BTransitionSequence();
   sequence->add(fUp);
   sequence->add(fDown);

   startTransition(sequence, pCompleted);
}

//==============================================================================
void BTransitionManager::startTransition(BTransition* transition, BGeneralEventSubscriber* pCompleted)
{
   if (mpTransition)
   {
      if (mpCompleted)
         mpCompleted->setFired();
      delete mpTransition;      
   }

   mpTransition = transition;
   mpCompleted = pCompleted;
   mTransitionCounter++;
}

//==============================================================================
void BTransitionManager::resetTransition()
{ 
   if (mpTransition)
      mpTransition->reset();
   if (mUpdateTimer && mUpdateTimer->isStarted())
   {
      mUpdateTimer->stop();
      mLastUpdateTime=0.0;
      mUpdateTimer->start();
   }
}

//==============================================================================
bool BTransitionManager::getTransitionComplete()
{
   return !mpTransition || mpTransition->isComplete();
}