//==============================================================================
// userminigame.cpp
//
// Copyright (c) Ensemble Studios, 2005-2007
//==============================================================================

// Includes
#include "common.h"
#include "userminigame.h"
#include "user.h"
#include "world.h"
#include "camera.h"
#include "configsgame.h"
#include "uimanager.h"
#include "soundmanager.h"

//==============================================================================
// Constants
const float cButtonGoalValue = cPi * 0.75f;
const float cAngleOffsetForUI = cPi;
const float cButtonPressMaxSpeed = 2.0f;
const float cButtonPressAccel = 2.0f;
const float cButtonPressWindow = cPiOver4;
const float cButtonMinGoal1 = cPiOver2;
const float cButtonMaxGoal1 = cPi;
const uint cNumComboButtons = 2;
const BInputControlType cComboButtons[cNumComboButtons] = { cButtonX, cButtonY };

//==============================================================================
//==============================================================================
uint8 getMinigameWidgetButton(BInputControlType type)
{
   if (type == cButtonA)
      return BUIWidgets::cMinigameButtonTypeA;
   else if (type == cButtonB)
      return BUIWidgets::cMinigameButtonTypeB;
   else if (type == cButtonX)
      return BUIWidgets::cMinigameButtonTypeX;
   else if (type == cButtonY)
      return BUIWidgets::cMinigameButtonTypeY;
   else
   {
      BASSERT(0);
      return BUIWidgets::cMinigameButtonTypeA;
   }
}


//==============================================================================
//==============================================================================
BMinigame::BMinigame() :
   mCurrentButtonIndex(-1),
   mResult(0.0f),
   mTimeFactor(1.0f),
   mCurrentT(0.0f),
   mCurrentVelocity(0.0f),
   mType(cNone),
   mInputExitCondition(cNoExit)
{
   mFlagStarted = false;
}

//==============================================================================
//==============================================================================
BMinigame::~BMinigame()
{
}

//==============================================================================
//==============================================================================
void BMinigame::start(BUser* pUser, eMinigameType type, float timeFactor, bool bContinue)
{
}

//==============================================================================
//==============================================================================
void BMinigame::stop()
{

}

//==============================================================================
//==============================================================================
BMinigame::eState BMinigame::update(BUser* pUser)
{
   return cWorking;
}

//==============================================================================
//==============================================================================
void BMinigame::render()
{
}

//==============================================================================
//==============================================================================
bool BMinigame::handleInput(BUser* pUser, long port, long event, long controlType, BInputEventDetail& detail)
{
   return false;
}

//==============================================================================
//==============================================================================
bool BMinigame::isScrolling() const
{
   switch (mType)
   {
      case cOneButtonPress:
      case cTwoButtonPress:
      case cThreeButtonPress:
      default:
         return true;
   }
}
