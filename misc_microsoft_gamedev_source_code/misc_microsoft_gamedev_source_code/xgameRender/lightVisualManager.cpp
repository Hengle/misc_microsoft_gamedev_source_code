//==============================================================================
// lightVisualManager.cpp
//
// Copyright (c) 2006 Ensemble Studios
//==============================================================================

// Includes
#include "xgameRender.h"
#include "lightVisualManager.h"
#include "lightVisualData.h"
#include "lightVisualInstance.h"

// Globals
BLightVisualManager gLightVisualManager;

//==============================================================================
// BLightVisualManager::BLightVisualManager
//==============================================================================
BLightVisualManager::BLightVisualManager() 
{
   mDataList.reserve(64);
}

//==============================================================================
// BLightVisualManager::~BLightVisualManager
//==============================================================================
BLightVisualManager::~BLightVisualManager()
{
}

//==============================================================================
// BLightVisualManager::init
//==============================================================================
bool BLightVisualManager::init()
{
   return true;
}

//==============================================================================
// BLightVisualManager::deinit
//==============================================================================
void BLightVisualManager::deinit()
{
   long count=mDataList.getSize();
   for(long i=0; i<count; i++)
      delete mDataList[i];   
   mDataList.clear();
}

//==============================================================================
// BLightVisualManager::createData
//==============================================================================
long BLightVisualManager::createData(BXMLNode node)
{
   if (!node)
      return -1;

   BLightVisualData* pLightVisualData=new BLightVisualData;   
   pLightVisualData->load(node);

   int index = mDataList.getSize();
   mDataList.pushBack(&pLightVisualData, 1);

   return index;
}

//==============================================================================
// BLightVisualManager::getData
//==============================================================================
BLightVisualData* BLightVisualManager::getData(long index)
{
   debugRangeCheck(index, mDataList.getSize());
   return mDataList[index];
}

//==============================================================================
// BLightVisualManager::createInstance
//==============================================================================
BLightVisualInstance* BLightVisualManager::createInstance(long dataIndex, const BMatrix* pTransform)
{
   BLightVisualData* pData=getData(dataIndex);
   if(!pData)
      return NULL;

   BLightVisualInstance* pInstance=new BLightVisualInstance();
   if(!pInstance)
      return NULL;

   pInstance->init(pData, pTransform);
   return pInstance;
}

//==============================================================================
// BLightVisualManager::releaseInstance
//==============================================================================
void BLightVisualManager::releaseInstance(BLightVisualInstance* pInstance)
{
   if(pInstance)
   {
      pInstance->deinit();
      delete pInstance;
   }
}
