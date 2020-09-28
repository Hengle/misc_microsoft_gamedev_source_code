//============================================================================
// flashdecal.cpp
// Ensemble Studios (c) 2006
//============================================================================

#include "common.h"
#include "flashdecal.h"
#include "flashmanager.h"

//-- render
#include "render.h"
#include "renderDraw.h"
#include "renderThread.h"

//-- game classes
#include "user.h"
#include "usermanager.h"
#include "player.h"
#include "uigame.h"
#include "visualmanager.h"
#include "world.h"
#include "database.h"
#include "protoobject.h"
#include "protosquad.h"


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BFlashDecal::BFlashDecal()
{
   commandListenerInit();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BFlashDecal::~BFlashDecal()
{
   commandListenerDeinit();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BFlashDecal::init(const char* filename, const char* datafile)
{
   /*
   if (!filename)
      return false;
   */
   deinit();

   if (!datafile)
      return false;

   if (!loadData(datafile))
      return false;

   if (!initMovies())
      return false;
   
   if (!initLookups())
      return false;

   setFlag(cFlagInitialized, true);
   return true;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BFlashDecal::initMovies()
{
   const BFlashProperties* pProps = getProperties();

   
   BFixedString128 filename;
   BFlashGateway::BDataHandle dataHandle;

   int count  = pProps->getFlashFileCount();   
   for (int i = 0; i < count; ++i)
   {
      if (!pProps->getFlashFileByIndex(i, filename))
         continue;
      
      int movieIndex = -1;
      BFlashDecalHashmap::iterator it = mHashmap.find(filename);
      if (it != mHashmap.end())
         movieIndex = it->second;

      // if we alrady have an instance of the movie then add it to our id lookup array.
      if (movieIndex != -1)
      {
         mMovies.add(movieIndex);
      }
      else // create a new instance
      {
         gFlashGateway.getOrCreateData(filename.c_str(), cFlashAssetCategoryInGame, dataHandle);
         BFlashMovieInstance* pMovie = gFlashGateway.createInstance(dataHandle, true);
         gFlashGateway.registerEventHandler(pMovie, mSimEventHandle);
         trace("Loaded Decal: %s [index = %d]", filename.c_str(), i);

         int newIndex = mFlashMovieLookup.getSize();
         // add the new instance to the hashmap
         mHashmap.insert(filename, newIndex);
         mFlashMovieLookup.add(pMovie);

         // add the new instance index to our proto id lookup
         mMovies.add(newIndex);
      }      
   }

   mSimEnableStates.setNumber(mFlashMovieLookup.getNumber(), true);
   mSimEnableStates.clear();

   return true;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BFlashDecal::initLookups()
{
   const BFlashProperties* pProps = getProperties();

   for(int i = 0; i < getProtoIconCount(); i++)
   {
      BFlashProtoIcon* pIcon = getProtoIcon(i);
      BDEBUG_ASSERT(pIcon !=NULL);

      BFlashDecalLookup icon;
      icon.mIndex = i;

      icon.mProtoID = -1;
      icon.mIconDataIndex  = -1;
      icon.mIconDataIndex2 = -1;
      
      if (pIcon->mType.compare("uniticon") == 0)
      {         
         icon.mIconDataIndex  = pProps->findFlashFileHandle(pIcon->mName.c_str());
         icon.mIconDataIndex2 = pProps->findFlashFileHandle(pIcon->mName2.c_str());

         int protoObjID   = gDatabase.getProtoObject(pIcon->mOwnerName);
         int protoSquadID = gDatabase.getProtoSquad(pIcon->mOwnerName);
         if (protoSquadID != -1)
         {
            icon.mProtoID = protoSquadID;            
            BProtoSquad* pProtoSquad = gDatabase.getGenericProtoSquad(protoSquadID);  
            if (pProtoSquad)
            {
               pProtoSquad->setSelectionDecalID(icon.mIconDataIndex);
               mDecalLookup.add(icon);
            }
         }
         else if (protoObjID != -1)
         {
            icon.mProtoID = protoObjID;
            BProtoObject* pProtoObject = gDatabase.getGenericProtoObject(protoObjID);
            if (pProtoObject)
            {
               pProtoObject->setSelectionDecalID(icon.mIconDataIndex);
               mDecalLookup.add(icon);
            }
         }
      }
      else if (pIcon->mType.compare("clumpdecal") == 0)
      {                  
         icon.mIconDataIndex  = pProps->findHandle(pIcon->mName.c_str());
         icon.mIconDataIndex2 = pProps->findHandle(pIcon->mName2.c_str());

         int protoObjID   = gDatabase.getProtoObject(pIcon->mOwnerName);
         int protoSquadID = gDatabase.getProtoSquad(pIcon->mOwnerName);
         if (protoSquadID != -1)
         {
            icon.mProtoID = protoSquadID;            
            BProtoSquad* pProtoSquad = gDatabase.getGenericProtoSquad(protoSquadID);  
            if (pProtoSquad)
            {
               pProtoSquad->setClumpSelectionDecalID(icon.mIconDataIndex);
               pProtoSquad->setClumpSelectionDecalMaskID(icon.mIconDataIndex2);
               mDecalLookup.add(icon);
            }
         }
         else if (protoObjID != -1)
         {
            icon.mProtoID = protoObjID;
//-- FIXING PREFIX BUG ID 1327
            const BProtoObject* pProtoObject = gDatabase.getGenericProtoObject(protoObjID);
//--
            if (pProtoObject)
            {
               pProtoObject->setClumpSelectionDecalID(icon.mIconDataIndex);
               pProtoObject->setClumpSelectionDecalMaskID(icon.mIconDataIndex2);
               mDecalLookup.add(icon);
            }
         }
      }
      else if (pIcon->mType.compare("staticdecal") == 0)
      {                  
         icon.mIconDataIndex  = pProps->findHandle(pIcon->mName.c_str());
         icon.mIconDataIndex2 = pProps->findHandle(pIcon->mName2.c_str());

         int protoObjID   = gDatabase.getProtoObject(pIcon->mOwnerName);
         int protoSquadID = gDatabase.getProtoSquad(pIcon->mOwnerName);
         if (protoSquadID != -1)
         {
            icon.mProtoID = protoSquadID;            
            BProtoSquad* pProtoSquad = gDatabase.getGenericProtoSquad(protoSquadID);  
            if (pProtoSquad)
            {
               pProtoSquad->setStaticDecalID(icon.mIconDataIndex);               
               mDecalLookup.add(icon);
            }
         }
         else if (protoObjID != -1)
         {
            icon.mProtoID = protoObjID;
//-- FIXING PREFIX BUG ID 1329
            const BProtoObject* pProtoObject = gDatabase.getGenericProtoObject(protoObjID);
//--
            if (pProtoObject)
            {
               pProtoObject->setStaticDecalID(icon.mIconDataIndex);               
               mDecalLookup.add(icon);
            }
         }
      }
      else if (pIcon->mType.compare("miscicon") == 0)
      {         
         icon.mIconDataIndex  = pProps->findFlashFileHandle(pIcon->mName.c_str());
         icon.mIconDataIndex2 = pProps->findFlashFileHandle(pIcon->mName2.c_str());

         if (pIcon->mOwnerName.compare("circleselect") == 0)
         {
            gUIGame.setCircleSelectDecalHandle(icon.mIconDataIndex);
            mDecalLookup.add(icon);
         }
         else if (pIcon->mOwnerName.compare("placeholder") == 0)
         {
            gUIGame.setDefaultDecalHandle(icon.mIconDataIndex);
            mDecalLookup.add(icon);
         }
      }
   }

   return true;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashDecal::deinit()
{

   for (unsigned int i = 0; i < mFlashMovieLookup.getSize(); ++i)
   {
      if (mFlashMovieLookup[i])
      {
         gFlashGateway.unregisterEventHandler(mFlashMovieLookup[i], mSimEventHandle);
         gFlashGateway.releaseInstance(mFlashMovieLookup[i]);
         mFlashMovieLookup[i] = NULL;
      }
   }
   
   mFlashMovieLookup.resize(0);
   mMovies.resize(0);  
   mSimEnableStates.clear();   
   mHashmap.clear();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashDecal::enter()
{
   if (!getFlag(cFlagInitialized))
      return;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashDecal::leave()
{
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashDecal::update(float elapsedTime)
{
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashDecal::renderBegin()
{   
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashDecal::render()
{   
   SCOPEDSAMPLEID(FlashUIGame, 0xFFFF0000);
   if (!getFlag(cFlagInitialized))
      return;

   for (unsigned int i = 0; i < mFlashMovieLookup.getSize(); ++i)
   {
      if (mFlashMovieLookup[i] && mSimEnableStates.isBitSet(i))
         mFlashMovieLookup[i]->render();
   }   
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashDecal::renderEnd()
{
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BFlashDecal::handleInput(int port, BInputEventType event, BInputControlType controlType, BInputEventDetail& detail)
{
   return false;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BFlashDecal::receiveEvent(const BEvent& event, BThreadIndex threadIndex)
{
   return false;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BFlashDecal::getDecalUV(int decalID, XMHALF4& uv, XMHALF4& uv2)
{   
   debugRangeCheck(decalID, mDecalLookup.getNumber());

   uv = XMHALF4(0.0f,0.0f,0.0f,0.0f);
   uv2 = uv;

   const BFlashProperties* pProps = getProperties();
   if (!pProps)
      return false;
   
   if (!pProps->getHalf4(mDecalLookup[decalID].mIconDataIndex, uv))
      return false;

   if (mDecalLookup[decalID].mIconDataIndex2 != BFlashProperties::eInvalidFlashPropertyHandle)
      pProps->getHalf4(mDecalLookup[decalID].mIconDataIndex2, uv2);
   
   return true;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BFlashDecal::getDecalSize(int protoID, float& sizeX, float& sizeY, float& sizeZ)
{   
   for(uint i = 0; i < mDecalLookup.getSize(); ++i)
   {
      if (mDecalLookup[i].mProtoID != protoID)
         continue;

//-- FIXING PREFIX BUG ID 1323
      const BFlashProtoIcon* pProtoIcon = getProtoIcon(mDecalLookup[i].mIndex);
//--
      if (!pProtoIcon)
         continue;

      sizeX = XMConvertHalfToFloat(pProtoIcon->mSize.x);
      sizeY = XMConvertHalfToFloat(pProtoIcon->mSize.y);
      sizeZ = XMConvertHalfToFloat(pProtoIcon->mSize.z);
      return true;
   }

   return false;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int  BFlashDecal::getProtoIconIndex(int protoID)
{
   for(uint i = 0; i < mDecalLookup.getSize(); ++i)
   {
      if (mDecalLookup[i].mProtoID == protoID)
         return mDecalLookup[i].mIndex;
   }
   return -1;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int BFlashDecal::getMovieInstanceIndex(int decalID)
{
   ASSERT_MAIN_THREAD

   int index = decalID - 1;
   if (index < 0)
      index = gUIGame.getDefaultDecalHandle() - 1;      

   if ((index < 0) || (index >= static_cast<int>(mMovies.size())))
      return -1;

   debugRangeCheck(index, mMovies.size());   
   int flashMovieIndex = mMovies[index];

   debugRangeCheck(flashMovieIndex, mFlashMovieLookup.size());
   BDEBUG_ASSERT(mFlashMovieLookup[flashMovieIndex]!=NULL);
   if (flashMovieIndex < 0 || flashMovieIndex >= mFlashMovieLookup.getNumber())
      return -1;

   // mark that it is used
   mSimEnableStates.setBit(flashMovieIndex);
   return mFlashMovieLookup[flashMovieIndex]->mInstanceSlotIndex;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BManagedTextureHandle BFlashDecal::getDecalTextureHandle(BFlashPropertyHandle propertyHandle)
{
   ASSERT_MAIN_THREAD
   
   if (!mpProperties)
      return cInvalidManagedTextureHandle;

   BManagedTextureHandle textureHandle = cInvalidManagedTextureHandle;
   if (!mpProperties->getTextureHandle(propertyHandle, textureHandle))
      return cInvalidManagedTextureHandle;

   return textureHandle;   
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashDecal::workerRender()
{
   ASSERT_RENDER_THREAD

   for (uint i = 0; i < mWorkerRenderList.size(); i++)
      gFlashGateway.workerRender(mWorkerRenderList[i]);

#ifndef BUILD_FINAL
   gFlashManager.mWorkerStats.mDecalMoviesRenderedPerFrame=mWorkerRenderList.size();
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashDecal::relaseGPUHeapTextures()
{
   ASSERT_MAIN_THREAD

   for (uint i = 0; i < mFlashMovieLookup.size(); i++)
      mFlashMovieLookup[i]->releaseGPUHeapTextures();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashDecal::updateRenderThread()
{
   ASSERT_THREAD(cThreadIndexSim);

   SCOPEDSAMPLE(BFlashDecalUpdateRenderThread);

   BDynamicSimArray<int> renderList;

   for (long i = 0; i < mSimEnableStates.getNumber(); i++)
   {
      if (mSimEnableStates.isBitSet(i))
         renderList.add(mFlashMovieLookup[i]->mInstanceSlotIndex);
   }

   int count = renderList.size();

   mSimEnableStates.clear();

   void* pBuffer = NULL;
   if (count > 0)
   {   
      int bufferSize = sizeof(int) * count;
      pBuffer = gRenderThread.allocateFrameStorage(bufferSize);
      Utils::FastMemCpy(pBuffer, renderList.getPtr(), bufferSize);
   }

   uchar* pData = static_cast<uchar*>(gRenderThread.submitCommandBegin(mCommandHandle, eFDCUpdate, sizeof(BFlashDecalUpdateData)));               
   BFlashDecalUpdateData* pUpdateData = reinterpret_cast<BFlashDecalUpdateData*>(pData);
   pUpdateData->mCount = count;
   pUpdateData->mpData = pBuffer;
   gRenderThread.submitCommandEnd(sizeof(BFlashDecalUpdateData));     
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashDecal::processCommand(const BRenderCommandHeader& header, const uchar* pData)
{
   switch (header.mType)
   {
      case eFDCUpdate:
         {
            BDEBUG_ASSERT(header.mLen == sizeof(BFlashDecalUpdateData));

            const BFlashDecalUpdateData* pUpdateData = reinterpret_cast<const BFlashDecalUpdateData*>(pData);

            mWorkerRenderList.resize(0);            
//-- FIXING PREFIX BUG ID 1325
            const int* pIndecies = reinterpret_cast<const int*>(pUpdateData->mpData);
//--

            if (pUpdateData->mCount > 0)
            {
               BDEBUG_ASSERT(pIndecies!=NULL);
               for (int i = 0; i < pUpdateData->mCount; i++)
                  mWorkerRenderList.add(pIndecies[i]);
            }
            
            break;
         }   
   }
}