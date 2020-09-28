//==============================================================================
// formationmanager.cpp
// Copyright (c) 2005-2007 Ensemble Studios
//==============================================================================

#include "common.h"
#include "formationmanager.h"
#include "formation2.h"

BFormationManager gFormationManager;



//==============================================================================
//==============================================================================
BFormationManager::BFormationManager()
{
}

//==============================================================================
//==============================================================================
BFormationManager::~BFormationManager()
{
}

//==============================================================================
//==============================================================================
bool BFormationManager::init()
{
   mIDCounter=0;
   return(true);
}

//==============================================================================
//==============================================================================
void BFormationManager::reset()
{
   init();
   BFormation2::mFreeList.clear();
}

//==============================================================================
//==============================================================================
BFormation2* BFormationManager::createFormation2()
{
   BFormation2* pReturn=BFormation2::getInstance();
   if (!pReturn)
      return(NULL);
   
   //Init it.
   if (!pReturn->init())
   {
      BFormation2::releaseInstance(pReturn);
      return(NULL);
   }

   //Set the ID.
   mIDCounter++;
   pReturn->setID(mIDCounter);

   //Done.
   return(pReturn);
}

//==============================================================================
//==============================================================================
void BFormationManager::releaseFormation2(BFormation2* pFormation)
{   
   if (!pFormation)
      return;
   BFormation2::releaseInstance(pFormation);
}

