//==============================================================================
// userminigame.h
//
// Copyright (c) 2005-2008 Ensemble Studios
//==============================================================================

#pragma once

class BUser;

//==============================================================================
// BMinigameButtonPress
//==============================================================================
class BMinigameButtonPress
{
   public:
      BMinigameButtonPress() : mButtonType(cButtonX), mMinT(0.0f), mMaxT(0.0f), mPressed(false) { }

      BInputControlType mButtonType;
      float mMinT;
      float mMaxT;
      bool mPressed;
};

//==============================================================================
// BMinigame
//==============================================================================
class BMinigame
{
   public:
      BMinigame();
      virtual ~BMinigame();

      enum eMinigameType
      {
         cNone,
         cOneButtonPress,
         cTwoButtonPress,
         cThreeButtonPress
      };

      enum eState
      {
         cWorking,
         cComplete
      };

      enum eInputExitConditions
      {
         cNoExit,
         cExit,
         cBadPress
      };

      // Start / stop
      void           reset() { mType = cNone; }
      void           start(BUser* pUser, eMinigameType type, float timeFactor, bool bContinue);
      void           stop();

      // Updating
      eState         update(BUser* pUser);
      void           render();
      bool           handleInput(BUser* pUser, long port, long event, long controlType, BInputEventDetail& detail);

      // Accessors
      bool           isScrolling() const;
      float          getResult() const { return mResult; }
      eMinigameType  getType() const { return mType; }

   protected:

      BSmallDynamicSimArray<BMinigameButtonPress>  mButtonPresses;
      int                                          mCurrentButtonIndex;
      float                                        mResult;
      float                                        mTimeFactor;
      float                                        mCurrentT;
      float                                        mCurrentVelocity;
      eMinigameType                                mType;
      eInputExitConditions                         mInputExitCondition;
      bool                                         mFlagStarted:1;
};
