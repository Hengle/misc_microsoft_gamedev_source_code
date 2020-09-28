//==============================================================================
// modemanager.cpp
//
// Copyright (c) 2002, Ensemble Studios
//==============================================================================

// Includes
#include "common.h"
#include "modemanager.h"
#include "mode.h"
#include "modeintro.h"
#include "modemenu.h"
#include "modepartyroom2.h"
#include "modegame.h"
#include "modeviewer.h"
#include "modecalibrate.h"
#include "modeflash.h"
#include "modecinematic.h"
#include "modecampaign2.h"

// Globals
BModeManager gModeManager;

//==============================================================================
// BModeManager::BModeManager
//==============================================================================
BModeManager::BModeManager() :
   mMode(NULL),
   mModes()
{
}

//==============================================================================
// BModeManager::~BModeManager
//==============================================================================
BModeManager::~BModeManager()
{
   shutdown();
}

//==============================================================================
// BModeManager::shutdown
//==============================================================================
void BModeManager::shutdown()
{
   if(mMode)
   {
      mMode->leave(NULL);
      mMode=NULL;
   }

   for(long i=0; i<mModes.getNumber(); i++)
   {
      if(mModes[i])
      {
         mModes[i]->shutdown();
         delete mModes[i];
         mModes[i]=NULL;
      }
   }
   mModes.clear();
}

//==============================================================================
// BModeManager::setup
//==============================================================================
bool BModeManager::setup()
{
   BMode* mode=NULL;
   for(long i=0; i<cModeCount; i++)
   {
      // Each cMode* enum needs to have a corresponding mode allocated
      // when modes are deprecated, both the enum and allocation need to be removed.
      switch(i)
      {
         case cModeIntro       : mode=new BModeIntro(i); break;
         case cModeMenu        : mode=new BModeMenu(i); break;
         case cModeGame        : mode=new BModeGame(i); break;
         case cModeViewer      : mode=new BModeViewer(i); break;
         case cModeCalibrate   : mode=new BModeCalibrate(i); break;
         case cModeFlash       : mode=new BModeFlash(i); break;
         case cModeCinematic   : mode=new BModeCinematic(i); break;
         case cModeModelView   : mode=new BModeViewer(i); break;
         case cModeCampaign2   : mode=new BModeCampaign2(i); break;
         case cModePartyRoom2  : mode=new BModePartyRoom2(i); break;
      }

      if(!mode)
      {
         BASSERT(0);
         return false;
      }

      if(!mode->setup())
      {
         BASSERT(0);
         delete mode;
         return false;
      }

      if(!gModeManager.addMode(mode))
      {
         delete mode;
         return false;
      }

      mode = NULL;
   }

   //setMode(cModeIntro);
   setMode(cModeMenu);
   //setMode(cModeViewer);
   //setMode(cModeGame);

   return true;
}

//==============================================================================
// BModeManager::addMode
//==============================================================================
bool BModeManager::addMode(BMode* mode)
{
   if(mModes.add(mode)==-1)
      return false;
   else
      return true;
}

//==============================================================================
// BModeManager::setMode
//==============================================================================
void BModeManager::setMode(long modeType)
{
   BMode* oldMode=mMode;
   BMode* newMode=(modeType==-1 ? NULL : getMode(modeType));

   if(newMode==oldMode)
      return;

   if(newMode)
      newMode->preEnter(oldMode);

   if (oldMode)
      oldMode->leave(newMode);

   mMode = newMode;
   
   if(newMode)
      newMode->enter(oldMode);

   if (oldMode)
      oldMode->postLeave(newMode);
}

//==============================================================================
// BModeManager::getMode
//==============================================================================
BMode* BModeManager::getMode(void)
{
   return (mMode);
}

//==============================================================================
// BModeManager::getMode
//==============================================================================
BMode* BModeManager::getMode(long modeType)
{
   for(long i=0; i<mModes.getNumber(); i++)
   {
      if(mModes[i]->getType()==modeType)
         return mModes[i];
   }
   return NULL;
}

//==============================================================================
// BModeManager::getModeType
//==============================================================================
long BModeManager::getModeType() const
{
   if (mMode)
      return mMode->getModeType();
   else
      return -1;
}

//==============================================================================
// BModeManager::update
//==============================================================================
bool BModeManager::update()
{
//-- FIXING PREFIX BUG ID 1254
   const BMode* saveMode=mMode;
//--

   if (mMode)
      mMode->update();

   return(mMode==saveMode);
}

//==============================================================================
// BModeManager::frameStart
//==============================================================================
void BModeManager::frameStart()
{
   if (mMode)
      mMode->frameStart();
}

//==============================================================================
// BModeManager::frameEnd
//==============================================================================
void BModeManager::frameEnd()
{
   if (mMode)
      mMode->frameEnd();
}

//==============================================================================
// BModeManager::renderBegin
//==============================================================================
void BModeManager::renderBegin()
{
   if (mMode)
      mMode->renderBegin();
}
//==============================================================================
// BModeManager::render
//==============================================================================
void BModeManager::render()
{
   if (mMode)
      mMode->render();
}

//==============================================================================
// BModeManager::renderEnd
//==============================================================================
void BModeManager::renderEnd()
{
   if (mMode)
      mMode->renderEnd();
}

//==============================================================================
// BModeManager::handleInput
//==============================================================================
bool BModeManager::handleInput(long port, long event, long controlType, BInputEventDetail& detail)
{
   if (mMode)
   {
      if(mMode->handleInput(port, event, controlType, detail))
         return true;
   }
   return false;
}

//==============================================================================
//static 
void BModeManager::shutdownFPSLogFile(bool reopen)
{
#ifdef ENABLE_FPS_LOG_FILE
   if (gModeManager.getModeGame())
   {
      gModeManager.getModeGame()->flushFPSLogDataToFile();
      gModeManager.getModeGame()->closeFPSLogFile();

      if (reopen)
      {
         gModeManager.getModeGame()->openFPSLogFile();
      }
   }
#endif
}
