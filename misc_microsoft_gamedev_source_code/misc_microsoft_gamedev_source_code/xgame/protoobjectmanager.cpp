//==============================================================================
// protoobjectmanager.cpp
//
// Copyright (c) 2006 Ensemble Studios
//==============================================================================

// Includes
#include "common.h"
#include "protoobjectmanager.h"

// xgame
#include "gamedirectories.h"
#include "protoobject.h"

// xsystem
#include "xmlreader.h"

//==============================================================================
// BProtoObjectManager::BProtoObjectManager
//==============================================================================
BProtoObjectManager::BProtoObjectManager() :
   mProtoObjects()
{
}

//==============================================================================
// BProtoObjectManager::~BProtoObjectManager
//==============================================================================
BProtoObjectManager::~BProtoObjectManager()
{
}

//==============================================================================
// BProtoObjectManager::setup
//==============================================================================
bool BProtoObjectManager::setup()
{
   BXMLReader reader;
   if(!reader.loadFileSAX(cDirData, B("object.xml")))
      return false;
}

//==============================================================================
// BProtoObjectManager::shutdown
//==============================================================================
void BProtoObjectManager::shutdown()
{
   long count=(long)mProtoObjects.size();
   for(long i=count-1; i>=0; i--)
   {
      BProtoObject* protoObject=mProtoObjects[i];
      delete protoObject;
   }
   mProtoObjects.clear();
}

//==============================================================================
// BPlayer::getProtoObject
//==============================================================================
BProtoObject* BProtoObjectManager::getProtoObject(long id) const
{
   if (id < 0 || id >= (long)mProtoObjects.size())
      return NULL;
   return mProtoObjects[id];
}
