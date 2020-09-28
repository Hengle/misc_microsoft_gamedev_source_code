//==============================================================================
// cinematicbars.cpp
//
// Copyright (c) 2008 Ensemble Studios
//==============================================================================

// Includes
#include "common.h"
#include "cinematicbars.h"
#include "render.h"
#include "primDraw2D.h"
#include "camera.h"
#include "user.h"



// Defines
#define CINEMATIC_BARS_SLIDEIN_TIME     1.0f
#define CINEMATIC_BARS_MAX_HEIGHT       92.2      // 2.39 : 1 aspect ratio
//#define CINEMATIC_BARS_MAX_HEIGHT       87.7      // 2.35 : 1 aspect ratio



//============================================================================
// BCinematicBars::BCinematicBars()
//============================================================================
BCinematicBars::BCinematicBars()
{
}

//============================================================================
// ~BCinematicBars::BCinematicBars()
//============================================================================
BCinematicBars::~BCinematicBars()
{

}

//============================================================================
// bool BCinematicBars::init()
//============================================================================
bool BCinematicBars::init()
{
   reset();
   return true;
}

//============================================================================
// void BCinematicBars::reset()
//============================================================================
void BCinematicBars::reset()
{
   mCurrentTime = 0;
   mbPlaying = false;
   mbUIVisible = true;
}

void BCinematicBars::update(float deltaTime)
{
   if (mbPlaying)
      mCurrentTime += deltaTime;
}

//============================================================================
// void BCinematicBars::update(float deltaTime)
//============================================================================
void BCinematicBars::postrender()
{
   if (!mbPlaying)
      return;

   // render bars
   int screenSizeX = gRender.getWidth();
   int screenSizeY = gRender.getHeight();

   float barFactor = Math::Lerp(mStartFactor, mEndFactor, (mCurrentTime / CINEMATIC_BARS_SLIDEIN_TIME));

   barFactor = Math::Min(barFactor, 1.0f);  // 0 = fully out, 1 = fully in

   int currentBarWidth = int(CINEMATIC_BARS_MAX_HEIGHT * barFactor);

   // Top bar
   BPrimDraw2D::drawSolidRect2D(0, 0, screenSizeX, currentBarWidth, 1.0f, 0.0f, 0.0f, 1.0f, 0xFF000000, 0xFF000000, cPosDiffuseVS, cDiffusePS);

   // Bottom bar
   BPrimDraw2D::drawSolidRect2D(0, screenSizeY - currentBarWidth, screenSizeX, screenSizeY, 1.0f, 0.0f, 0.0f, 1.0f, 0xFF000000, 0xFF000000, cPosDiffuseVS, cDiffusePS);

   if (mCurrentTime >= CINEMATIC_BARS_SLIDEIN_TIME && mbUIVisible == true)
   {
      BUser* pPrimaryUser = gUserManager.getPrimaryUser();
      BUIContext* pPrimaryUIContext = pPrimaryUser->getUIContext();
     
      gUIManager->setupCinematicUI(false);

      pPrimaryUIContext->setUnitSelectionVisible(true);
      pPrimaryUIContext->setUnitCardVisible(true);
      pPrimaryUIContext->setUnitStatsVisible(true);
      pPrimaryUser->setFlagOverrideUnitPanelDisplay(false);
         
      mbPlaying = false;
   }
}

void BCinematicBars::fadeIn()
{
   mStartFactor = 0.0f;
   mEndFactor = 1.0f;
   mCurrentTime = 0;
   mbPlaying = true;
   mbUIVisible = false;
}

void BCinematicBars::fadeOut()
{
   mStartFactor = 1.0f;
   mEndFactor = 0.0f;
   mCurrentTime = 0;
   mbPlaying = true;
   mbUIVisible = true;
}

bool BCinematicBars::visible()
{
   return mbPlaying || mEndFactor == 1.0f;
}