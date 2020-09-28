//==============================================================================
// modeintro.cpp
//
// Copyright (c) 2005 Ensemble Studios
//==============================================================================

// Includes
#include "common.h"
#include "modeintro.h"
#include "game.h"
#include "modemanager.h"
#include "render.h"
#include "world.h"
#include "fontsystem2.h"

// Constants
const float cIntroTime=5.0f;

//==============================================================================
// BModeIntro::BModeIntro
//==============================================================================
BModeIntro::BModeIntro(long modeType) :
   BMode(modeType),
   mX(0.0f),
   mY(0.0f)
{
}

//==============================================================================
// BModeIntro::~BModeIntro
//==============================================================================
BModeIntro::~BModeIntro()
{
}

//==============================================================================
// BModeIntro::setup
//==============================================================================
bool BModeIntro::setup()
{
   return BMode::setup();
}

//==============================================================================
// BModeIntro::preEnter
//==============================================================================
void BModeIntro::preEnter(BMode* lastMode)
{
   return BMode::preEnter(lastMode);
}

//==============================================================================
// BModeIntro::enter
//==============================================================================
void BModeIntro::enter(BMode* lastMode)
{
   return BMode::enter(lastMode);
}

//==============================================================================
// BModeIntro::leave
//==============================================================================
void BModeIntro::leave(BMode* newMode)
{
   return BMode::leave(newMode);
}

//==============================================================================
// BModeIntro::update
//==============================================================================
void BModeIntro::update()
{
   if(gGame.getTotalTime()>=cIntroTime)
   {
      gModeManager.setMode(BModeManager::cModeMenu);
      return;
   }

   float elapsed=(gGame.getFrameTime()*0.001f);
   mX+=(elapsed*50);
   if(mX>=(float)gRender.getWidth())
      mX=0.0f;
   mY+=(elapsed*50);
   if(mY>=(float)gRender.getHeight())
      mY=0.0f;

   BMode::update();
}

//==============================================================================
// BModeIntro::renderBegin
//==============================================================================
void BModeIntro::renderBegin()
{
}

//==============================================================================
// BModeIntro::render
//==============================================================================
void BModeIntro::render()
{
   gFontManager.render2D();
}

//==============================================================================
// BModeIntro::renderEnd
//==============================================================================
void BModeIntro::renderEnd()
{
}

//==============================================================================
// BModeIntro::handleInput
//==============================================================================
bool BModeIntro::handleInput(long port, long event, long controlType, BInputEventDetail& detail)
{
   port; detail;
   if(event==cInputEventControlStart && controlType==cButtonA)
   {
      gModeManager.setMode(BModeManager::cModeMenu);
      return true;
   }
   return false;
}
