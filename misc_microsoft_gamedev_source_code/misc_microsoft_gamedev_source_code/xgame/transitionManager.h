//==============================================================================
// transitionManager.h
//
// Copyright (c) 2008 Ensemble Studios
//==============================================================================
#pragma once

class BGeneralEventSubscriber;
class BTimer;

//============================================================================
class BTransition
{
public:
   virtual void     update(float deltaTime) = 0;
   virtual void     postRender() = 0;
   virtual bool     isComplete() = 0;
   virtual void     reset() = 0;
};

//============================================================================
// This is to handle a one way fade (ie. Fade down to black, or fade up from black)
class BFadeTransition : public BTransition
{
public:

   BFadeTransition();

   virtual void   update(float deltaTime);
   virtual void   postRender();
   virtual bool   isComplete();   
   virtual void   reset() { mTime = 0.0f; }

   void           setup(float startOpacity, float endOpacity, float fadeTime, float leadIn, float leadOut, const BColor &color);

private:
   float    mStartingOpacity;
   float    mEndingOpacity;
   float    mTime;
   float    mLeadIn;
   float    mLeadOut;
   float    mFadeTime;
   BColor   mColor;
};

//============================================================================
// This is for a sequence of fades. (ie. Fade down to black, hold for X, then fade up, or the reverse)
class BTransitionSequence : public BTransition
{
public:

   BTransitionSequence();
   ~BTransitionSequence();

   virtual void   update(float deltaTime);
   virtual void   postRender();
   virtual bool   isComplete();
   virtual void   reset() { }

   void           add(BTransition *transition);

private:
   BSmallDynamicSimArray<BTransition*> mTransitions;
   int            mActiveTransition;
};

//============================================================================
// BTransitionManager
//============================================================================
class BTransitionManager
{
public:
   BTransitionManager();
   ~BTransitionManager();

   void     postRender();
   void     update();

   void     doFadeDown(float fadeTime, float leadIn, float leadOut, const BColor& color, BGeneralEventSubscriber* pCompleted = NULL);
   void     doFadeUp(float fadeTime, float leadIn, float leadOut, const BColor& color, BGeneralEventSubscriber* pCompleted = NULL);
   void     doFadeDownHoldFadeUp(float fadeDown, float hold, float fadeUp, const BColor& color, BGeneralEventSubscriber* pCompleted = NULL);
   void     doFadeUpHoldFadeDown(float fadeUp, float hold, float fadeDown, const BColor& color, BGeneralEventSubscriber* pCompleted = NULL);

   void     startTransition(BTransition* transition, BGeneralEventSubscriber* pCompleted = NULL);

   void     setCompletedEventSubscriber(BGeneralEventSubscriber* pCompleted) { mpCompleted = pCompleted; }

   BGeneralEventSubscriber* getCompletedEventSubscriber() const { return (mpCompleted); }

   void     resetTransition();

   uint     getTransitionCounter() { return mTransitionCounter; }

   bool     getTransitionComplete();

private:

   double         mLastUpdateTime;
   BTimer*        mUpdateTimer;
   BGeneralEventSubscriber* mpCompleted;
   BTransition*   mpTransition;
   uint           mTransitionCounter;
};
