//============================================================================
// File: flashgateway.cpp
//
// Copyright (c) 2006 Ensemble Studios
//============================================================================
#include "xgamerender.h"
#include "flashgateway.h"
#include "flashmanager.h"
#include "flashinput.h"
#include "inputsystem.h"
#include "threading\lightWeightMutex.h"

#define USE_INSTANCE_MUTEX_LOCK

//============================================================================
// Globals
//============================================================================
BFlashGateway gFlashGateway;

//============================================================================
// BFlashMovieInstance::BFlashMovieInstance
//============================================================================
BFlashMovieInstance::BFlashMovieInstance() :
   mInstanceSlotIndex(-1),
   mRenderTargetHandle(cInvalidManagedTextureHandle),
   mpRenderTargetTexture(NULL)
{
}

//============================================================================
// BFlashMovieInstance::~BFlashMovieInstance
//============================================================================
BFlashMovieInstance::~BFlashMovieInstance()
{
}

//============================================================================
//============================================================================
void BFlashMovieInstance::render()
{
   if (-1 == mInstanceSlotIndex)
      return;
   gFlashGateway.render(this);
}

//============================================================================
//============================================================================
void BFlashMovieInstance::workerRender()
{
   ASSERT_RENDER_THREAD
   if (-1 == mInstanceSlotIndex)
      return;

   gFlashGateway.workerRender(mInstanceSlotIndex);
}

//============================================================================
//============================================================================
void BFlashMovieInstance::setVariable(const char* variablePath, const char* value, GFxMovie::SetVarType type)
{
   if (-1 == mInstanceSlotIndex)
      return;
   gFlashGateway.setInstanceVariable(this, variablePath, value, type);
}

//============================================================================
//============================================================================
void BFlashMovieInstance::setVariable(const char* variablePath, const GFxValue& value, GFxMovie::SetVarType type)
{
   if (-1 == mInstanceSlotIndex)
      return;
   gFlashGateway.setInstanceVariable(this, variablePath, value, type);
}

//============================================================================
//============================================================================
void BFlashMovieInstance::setVariableArray(const char* variablePath, GFxValue* pValue, int count, int startIndex, GFxMovie::SetVarType type)
{
   if (-1 == mInstanceSlotIndex)
      return;
   gFlashGateway.setInstanceVariableArray(this, variablePath, pValue, count, startIndex, type);
}

//============================================================================
//============================================================================
void BFlashMovieInstance::invokeActionScript(const char* method, const char* fmt, const char* value)
{
   if (-1 == mInstanceSlotIndex)
      return;
   gFlashGateway.invokeInstanceActionScript(this, method, fmt, value);
}

//============================================================================
//============================================================================
void BFlashMovieInstance::invokeActionScript(const char* method, const GFxValue* pArgs, int numArgs)
{
   if (-1 == mInstanceSlotIndex)
      return;
   gFlashGateway.invokeInstanceActionScript(this, method, pArgs, numArgs);
}

//============================================================================
//============================================================================
void BFlashMovieInstance::keyEvent(const GFxKeyEvent& event)
{
   if (-1 == mInstanceSlotIndex)
      return;

   gFlashGateway.keyEvent(this, event);
}

//============================================================================
//============================================================================
void BFlashMovieInstance::registerEventHandler(BEventReceiverHandle handle)
{
   if (-1 == mInstanceSlotIndex)
      return;

   gFlashGateway.registerEventHandler(this, handle);
}

//============================================================================
//============================================================================
void BFlashMovieInstance::unregisterEventHandler(BEventReceiverHandle handle)
{
   if (-1 == mInstanceSlotIndex)
      return;

   gFlashGateway.unregisterEventHandler(this, handle);
}

//============================================================================
//============================================================================
void BFlashMovieInstance::setDimension(int x, int y, int width, int height)
{
   if (-1 == mInstanceSlotIndex)
      return;
   gFlashGateway.setInstanceDimension(this, x, y, width, height);
}

//============================================================================
//============================================================================
void BFlashMovieInstance::releaseGPUHeapTextures()
{
   if (-1 == mInstanceSlotIndex)
      return;

   gFlashGateway.releaseInstanceGPUHeapTextures(this);
}

//============================================================================
// BFlashGateway::BFlashGateway
//============================================================================
BFlashGateway::BFlashGateway() :
   BEventReceiver(),
   mNumDataSlotsInUse(0), 
   mNumInstanceSlotsFree(0),
   mInstanceSlotsHighWaterMark(0),
   mFirstFreeInstanceSlotIndex(-1),
   mTotalInstances(0),
   mInitialized(false)
{

}

//============================================================================
// BFlashGateway::~BFlashGateway
//============================================================================
BFlashGateway::~BFlashGateway()
{
}

//============================================================================
// BFlashGateway::init
//============================================================================
void BFlashGateway::init(void)
{
   ASSERT_MAIN_THREAD
   BDEBUG_ASSERT(!mInitialized);
      
   clear();

   commandListenerInit();

   eventReceiverInit();

   mInitialized = true;
}

//============================================================================
// BFlashGateway::deinit
//============================================================================
void BFlashGateway::deinit(void)
{
   ASSERT_MAIN_THREAD

   if (!mInitialized)
      return;

   releaseAllInstances();      

   gRenderThread.blockUntilGPUIdle();

   commandListenerDeinit();

   eventReceiverDeinit();

   mInitialized = false;   
}

//============================================================================
// BFlashGateway::clear
//============================================================================
void BFlashGateway::clear(void)
{
   ASSERT_MAIN_THREAD

   for (uint i = 0; i < cMaxDataSlots; i++)
   {
      BDataSlot& dataSlot = mDataSlots[i];
      dataSlot.mName.setNULL();
      dataSlot.mRenderDataIndex = cInvalidIndex;
      dataSlot.mStatus = cSSFree;
   }

   for (uint i = 0; i < cMaxInstanceSlots; i++)
   {
      BInstanceSlot& instanceSlot = mInstanceSlots[i];
      instanceSlot.mDataSlotIndex = (BDataHandle)cInvalidIndex;
      instanceSlot.mpInstance = NULL;
      instanceSlot.mpRenderEffect = NULL;
      instanceSlot.mStatus = cSSFree;
      instanceSlot.mBeingDeleted = false;
   }

   mNumDataSlotsInUse = 0;
   mNumInstanceSlotsFree = 0;
   mInstanceSlotsHighWaterMark = 0;
   mFirstFreeInstanceSlotIndex = -1;
}

//============================================================================
// BFlashGateway::getOrCreateData
//============================================================================
void BFlashGateway::getOrCreateData(const char* pName, uint assetCategory, BFlashGateway::BDataHandle& dataHandle)
{
   ASSERT_MAIN_THREAD

   int index = -1;
   findDataSlot(pName, index);

   if (index != -1 && index < (int) mNumDataSlotsInUse)
   {
      dataHandle = (BDataHandle)index;
      return;
   }

   if (mNumDataSlotsInUse == cMaxDataSlots)
   {
      BFATAL_FAIL("BFlashGateway::getOrCreateData: Increase cMaxDataSlots!");
   }

   dataHandle = (BDataHandle)mNumDataSlotsInUse;
   mNumDataSlotsInUse++;

   mDataSlots[dataHandle].mName.set(pName);
   mDataSlots[dataHandle].mRenderDataIndex = -1;
   mDataSlots[dataHandle].mStatus = cSSPending;


   BInitDataSlotData* pInitData = (BInitDataSlotData*)gRenderThread.submitCommandBegin(mCommandHandle, cRCInitDataSlot, sizeof(BInitDataSlotData), 16);
   pInitData->mAssetCategory = assetCategory;
   pInitData->mDataHandle = dataHandle;
   gRenderThread.submitCommandEnd(sizeof(BInitDataSlotData), 16);
}

//============================================================================
// BFlashGateway::createInstance
//============================================================================
BFlashMovieInstance* BFlashGateway::createInstance(BFlashGateway::BDataHandle dataHandle, bool bRenderToTexture)
{
   ASSERT_MAIN_THREAD

   debugRangeCheck(dataHandle, cMaxDataSlots);

#ifdef USE_INSTANCE_MUTEX_LOCK
   mInstanceSlotMutex.lock();
#endif

   debugRangeCheck(dataHandle, mNumDataSlotsInUse);   
   uint slotIndex = mInstanceSlotsHighWaterMark;      
   if (mNumInstanceSlotsFree)
   {
      slotIndex = mFirstFreeInstanceSlotIndex;
      BDEBUG_ASSERT(slotIndex < mInstanceSlotsHighWaterMark);
      BDEBUG_ASSERT(mInstanceSlots[slotIndex].mStatus == cSSFree);

      mFirstFreeInstanceSlotIndex = mInstanceSlots[mFirstFreeInstanceSlotIndex].mNextFreeInstanceSlotIndex;
      if (mFirstFreeInstanceSlotIndex != -1)
      {
         BDEBUG_ASSERT((uint)mFirstFreeInstanceSlotIndex != slotIndex);
         BDEBUG_ASSERT(mInstanceSlots[mFirstFreeInstanceSlotIndex].mStatus == cSSFree);
      }

      mNumInstanceSlotsFree--;
   }      
   else
   {
      if (mInstanceSlotsHighWaterMark == cMaxInstanceSlots)
      {
         BFATAL_FAIL("BFlashGateway::createInstance: Increase cMaxInstanceSlots!");
      }

      mInstanceSlotsHighWaterMark++;
   }   

   BFlashMovieInstance* pInstance = BAlignedAlloc::New<BFlashMovieInstance>(ALIGN_OF(BFlashMovieInstance), gSimHeap);   
   pInstance->mInstanceSlotIndex = slotIndex;
   pInstance->mRenderTargetHandle= cInvalidManagedTextureHandle;

   BInstanceSlot& slot = mInstanceSlots[slotIndex];
   BDEBUG_ASSERT((slot.mpInstance == NULL) && (!slot.mBeingDeleted));

   slot.mDataSlotIndex = dataHandle;
   slot.mpInstance = pInstance;
   slot.mpRenderEffect = NULL;
   slot.mStatus = cSSPending;

   mTotalInstances++;

#ifdef USE_INSTANCE_MUTEX_LOCK
   mInstanceSlotMutex.unlock();
#endif

   BInitInstanceSlotData* pInitData = (BInitInstanceSlotData*)gRenderThread.submitCommandBegin(mCommandHandle, cRCInitInstanceSlot, sizeof(BInitInstanceSlotData), 16);
   pInitData->mInstanceSlotIndex = slotIndex;
   pInitData->mbRenderToTexture = bRenderToTexture;
   gRenderThread.submitCommandEnd(sizeof(BInitInstanceSlotData), 16);
      
   return pInstance;
}

//============================================================================
// BFlashGateway::releaseInstance
//============================================================================
void BFlashGateway::releaseInstance(BFlashMovieInstance* pInstance)
{
   ASSERT_MAIN_THREAD

   if (!pInstance)
      return;

#ifdef USE_INSTANCE_MUTEX_LOCK
   mInstanceSlotMutex.lock();
#endif

      const int instanceSlotIndex = pInstance->mInstanceSlotIndex;
      debugRangeCheck<int, int>(instanceSlotIndex, cMaxInstanceSlots);
      BDEBUG_ASSERT(mInstanceSlots[instanceSlotIndex].mStatus != cSSFree);
      BDEBUG_ASSERT(pInstance == mInstanceSlots[instanceSlotIndex].mpInstance);
      BDEBUG_ASSERT(!mInstanceSlots[instanceSlotIndex].mBeingDeleted);

      // ajl 11/15/08 - Billy and I both looked at this and figured out this flag isn't needed
      // because it's now obsolete after the threading changes made yesterday and today. Those
      // changes along with this flag broke screen transitions where they would lose 1 render 
      // frame because the render command isn't processed before this releaseInstance call is made.
      //mInstanceSlots[instanceSlotIndex].mBeingDeleted = true;

      BAlignedAlloc::Delete<BFlashMovieInstance>(mInstanceSlots[instanceSlotIndex].mpInstance, gSimHeap);
      mInstanceSlots[instanceSlotIndex].mpInstance = NULL;

      mTotalInstances--;

#ifdef USE_INSTANCE_MUTEX_LOCK
   mInstanceSlotMutex.unlock();
#endif

   gRenderThread.submitCommand(*this, cRCDeleteInstanceSlot, instanceSlotIndex);
      
}

//============================================================================
//============================================================================
void BFlashGateway::registerEventHandler(BFlashMovieInstance* pInstance, BEventReceiverHandle handle)
{
   ASSERT_MAIN_THREAD
   if (!pInstance)
      return;
   
   const int instanceSlotIndex = pInstance->mInstanceSlotIndex;
   debugRangeCheck<int, int>(instanceSlotIndex, cMaxInstanceSlots);
   
   BInstanceRegisterEventHandlerData* pUpdateData = (BInstanceRegisterEventHandlerData*)gRenderThread.submitCommandBegin(mCommandHandle, cRCRegisterEventHandler, sizeof(BInstanceRegisterEventHandlerData), 16);

   pUpdateData->mHandle = handle;
   pUpdateData->mInstanceSlotIndex = instanceSlotIndex;

   gRenderThread.submitCommandEnd(sizeof(BInstanceRegisterEventHandlerData), 16);
}

//============================================================================
//============================================================================
void BFlashGateway::unregisterEventHandler(BFlashMovieInstance* pInstance, BEventReceiverHandle handle)
{
   ASSERT_MAIN_THREAD
   if (!pInstance)
      return;   

   const int instanceSlotIndex = pInstance->mInstanceSlotIndex;
   debugRangeCheck<int, int>(instanceSlotIndex, cMaxInstanceSlots);   

   BInstanceRegisterEventHandlerData* pUpdateData = (BInstanceRegisterEventHandlerData*)gRenderThread.submitCommandBegin(mCommandHandle, cRCUnregisterEventHandler, sizeof(BInstanceRegisterEventHandlerData), 16);

   pUpdateData->mHandle = handle;
   pUpdateData->mInstanceSlotIndex = instanceSlotIndex;

   gRenderThread.submitCommandEnd(sizeof(BInstanceRegisterEventHandlerData), 16);
}

//============================================================================
//============================================================================
void BFlashGateway::render(BFlashMovieInstance* pInstance)
{
   ASSERT_MAIN_THREAD
   if (!pInstance)
      return;   

   const int instanceSlotIndex = pInstance->mInstanceSlotIndex;
   debugRangeCheck<int, int>(instanceSlotIndex, cMaxInstanceSlots);   

   BInstanceRenderData* pUpdateData = (BInstanceRenderData*)gRenderThread.submitCommandBegin(mCommandHandle, cRCRenderInstance, sizeof(BInstanceRenderData), 16);

   pUpdateData->mInstanceSlotIndex = instanceSlotIndex;
   pUpdateData->mTexture = cInvalidManagedTextureHandle;

   gRenderThread.submitCommandEnd(sizeof(BInstanceRenderData), 16);
}

//============================================================================
//============================================================================
IDirect3DTexture9* BFlashGateway::getTexture(int instanceSlot)
{
   ASSERT_RENDER_THREAD   
   const int instanceSlotIndex = instanceSlot;

   if (instanceSlotIndex < 0 || instanceSlotIndex >= cMaxInstanceSlots)
      return NULL;

   if (mInstanceSlots[instanceSlotIndex].mStatus != cSSValid)
      return NULL;

   if (mInstanceSlots[instanceSlotIndex].mBeingDeleted)
      return NULL;

   if (!mInstanceSlots[instanceSlotIndex].mpRenderEffect)
      return NULL;

   return mInstanceSlots[instanceSlotIndex].mpRenderEffect->getRenderTargetTexture();
}

//============================================================================
//============================================================================
void BFlashGateway::workerRender(int instanceSlot)
{
   ASSERT_RENDER_THREAD

   const int instanceSlotIndex = instanceSlot;
   debugRangeCheck<int, int>(instanceSlotIndex, cMaxInstanceSlots);

   //-- if this is a cSSFree object don't render because it probably just got
   //-- deleted 
   if (mInstanceSlots[instanceSlotIndex].mStatus != cSSValid)
      return;

   //-- don't render if this is in the process of getting deleted
   if (mInstanceSlots[instanceSlotIndex].mBeingDeleted)
      return;

   if (!mInstanceSlots[instanceSlotIndex].mpRenderEffect)
      return;

   mInstanceSlots[instanceSlotIndex].mpRenderEffect->render();
}

//============================================================================
//============================================================================
void BFlashGateway::setInstanceVariable(BFlashMovieInstance* pInstance, const char* variablePath, const char* value, int type)
{
   ASSERT_MAIN_THREAD
   if (!pInstance)
      return;

   const int instanceSlotIndex = pInstance->mInstanceSlotIndex;
   debugRangeCheck<int, int>(instanceSlotIndex, cMaxInstanceSlots);
      
   BInstanceSetVariableData* pUpdateData = (BInstanceSetVariableData*)gRenderThread.submitCommandBegin(mCommandHandle, cRCSetInstanceVariable, sizeof(BInstanceSetVariableData), 16);

   pUpdateData->variablePath = variablePath;
   pUpdateData->value        = value;
   pUpdateData->type         = type;
   pUpdateData->mInstanceSlotIndex = instanceSlotIndex;

   gRenderThread.submitCommandEnd(sizeof(BInstanceSetVariableData), 16);
}

//============================================================================
//============================================================================
void BFlashGateway::setInstanceVariable(BFlashMovieInstance* pInstance, const char* variablePath, const GFxValue& value, int type)
{
   ASSERT_MAIN_THREAD   
   if (!pInstance)
      return;   

   const int instanceSlotIndex = pInstance->mInstanceSlotIndex;
   debugRangeCheck<int, int>(instanceSlotIndex, cMaxInstanceSlots);      

   uint stringSize = getGFxValueStringSizeForBuffer(value);

   BInstanceSetVariableDataEX* pUpdateData = (BInstanceSetVariableDataEX*)gRenderThread.submitCommandBegin(mCommandHandle, cRCSetInstanceVariableEX, sizeof(BInstanceSetVariableDataEX) + stringSize, 16);

   pUpdateData->variablePath = variablePath;
   pUpdateData->value        = value;
   pUpdateData->type         = type;
   pUpdateData->mInstanceSlotIndex = instanceSlotIndex;
   pUpdateData->stringSize   = stringSize;

   if (stringSize > 0)
      copyGFxValueStringToBuffer(value, stringSize, ((BYTE*)pUpdateData) + sizeof(BInstanceSetVariableDataEX));

   gRenderThread.submitCommandEnd(sizeof(BInstanceSetVariableDataEX) + stringSize, 16);
}

//============================================================================
//============================================================================
void BFlashGateway::setInstanceVariableArray(BFlashMovieInstance* pInstance, const char* variablePath, GFxValue* pValue, int count, int startIndex, GFxMovie::SetVarType type)
{
   ASSERT_MAIN_THREAD
   if (!pInstance)
      return;

   if (!pValue)
      return;
   
   BDEBUG_ASSERT(pValue);

   const int instanceSlotIndex = pInstance->mInstanceSlotIndex;
   debugRangeCheck<int, int>(instanceSlotIndex, cMaxInstanceSlots);
   
   if (count >= cMaxGFXArgs)
      count = cMaxGFXArgs;

   uint totalStringSize = 0;
   uint stringSizes[cMaxGFXArgs];
   for (int i=0; i<count; i++)
   {
      stringSizes[i] = getGFxValueStringSizeForBuffer(pValue[i]);
      totalStringSize += stringSizes[i];
   }

   BInstanceSetVariableArrayDataEX* pUpdateData = (BInstanceSetVariableArrayDataEX*)gRenderThread.submitCommandBegin(mCommandHandle, cRCSetInstanceVariableArrayEX, sizeof(BInstanceSetVariableArrayDataEX) + totalStringSize, 16);

   pUpdateData->variablePath = variablePath;
   for (int i=0; i<count; i++)
   {      
      if (pValue[i].GetType() == GFxValue::VT_String)
         pUpdateData->value[i].SetString(pValue[i].GetString());
      else if (pValue[i].GetType() == GFxValue::VT_StringW)
         pUpdateData->value[i].SetStringW(pValue[i].GetStringW());
      else
         pUpdateData->value[i] = pValue[i];
   }   

   pUpdateData->totalStringSize = totalStringSize;
   if (totalStringSize > 0)
   {
      BYTE* pStringBuffer = ((BYTE*)pUpdateData) + sizeof(BInstanceSetVariableArrayDataEX);
      for (int i=0; i<count; i++)
      {
         pUpdateData->stringSizes[i] = stringSizes[i];
         if (stringSizes[i] > 0)
            pStringBuffer = copyGFxValueStringToBuffer(pValue[i], stringSizes[i], pStringBuffer);
      }
   }

   pUpdateData->count        = count;
   pUpdateData->startIndex   = startIndex;
   pUpdateData->type         = type;
   pUpdateData->mInstanceSlotIndex = instanceSlotIndex;

   gRenderThread.submitCommandEnd(sizeof(BInstanceSetVariableArrayDataEX) + totalStringSize, 16);
}

//============================================================================
//============================================================================
void BFlashGateway::setInstanceDimension(BFlashMovieInstance* pInstance, int x, int y, int width, int height)
{
   ASSERT_MAIN_THREAD
   if (!pInstance)
      return;

   const int instanceSlotIndex = pInstance->mInstanceSlotIndex;
   debugRangeCheck<int, int>(instanceSlotIndex, cMaxInstanceSlots);

   BInstanceSetDimensionData* pUpdateData = (BInstanceSetDimensionData*)gRenderThread.submitCommandBegin(mCommandHandle, cRCSetInstanceDimension, sizeof(BInstanceSetDimensionData), 16);

   pUpdateData->mX = x;
   pUpdateData->mY = y;
   pUpdateData->mWidth = width;
   pUpdateData->mHeight = height;
   pUpdateData->mInstanceSlotIndex = instanceSlotIndex;

   gRenderThread.submitCommandEnd(sizeof(BInstanceSetDimensionData), 16);
}

//============================================================================
//============================================================================
void BFlashGateway::releaseInstanceGPUHeapTextures(BFlashMovieInstance* pInstance)
{
   ASSERT_MAIN_THREAD
   if (!pInstance)
      return;
   
   const int instanceSlotIndex = pInstance->mInstanceSlotIndex;
   debugRangeCheck<int, int>(instanceSlotIndex, cMaxInstanceSlots);
   
   BInstanceData* pUpdateData = (BInstanceData*)gRenderThread.submitCommandBegin(mCommandHandle, cRCReleaseInstanceGPUHeapTextures, sizeof(BInstanceData), 16);
   pUpdateData->mInstanceSlotIndex = instanceSlotIndex;

   gRenderThread.submitCommandEnd(sizeof(BInstanceData), 16);
}

//============================================================================
//============================================================================
void BFlashGateway::invokeInstanceActionScript(BFlashMovieInstance* pInstance, const char* method, const char* fmt, const char* value)
{
   ASSERT_MAIN_THREAD
   if (!pInstance)
      return;

   const int instanceSlotIndex = pInstance->mInstanceSlotIndex;
   debugRangeCheck<int, int>(instanceSlotIndex, cMaxInstanceSlots);
      
   BInstanceCommandData* pUpdateData = (BInstanceCommandData*)gRenderThread.submitCommandBegin(mCommandHandle, cRCInvokeInstanceActionScript, sizeof(BInstanceCommandData), 16);

   pUpdateData->method = method;
   pUpdateData->fmt    = fmt;
   pUpdateData->value  = value;
   pUpdateData->mInstanceSlotIndex = instanceSlotIndex;

   gRenderThread.submitCommandEnd(sizeof(BInstanceCommandData), 16);
}

//============================================================================
//============================================================================
void BFlashGateway::invokeInstanceActionScript(BFlashMovieInstance* pInstance, const char* method, const GFxValue* pArgs, int argCount)
{
   ASSERT_MAIN_THREAD
   if (!pInstance)
      return;      

   const int instanceSlotIndex = pInstance->mInstanceSlotIndex;
   debugRangeCheck<int, int>(instanceSlotIndex, cMaxInstanceSlots);
   debugRangeCheckIncl<int, int>(argCount, cMaxGFXArgs);   

   if (argCount >= cMaxGFXArgs)
      argCount = cMaxGFXArgs;

   uint totalStringSize = 0;
   uint stringSizes[cMaxGFXArgs];
   for (int i=0; i<argCount; i++)
   {
      stringSizes[i] = getGFxValueStringSizeForBuffer(pArgs[i]);
      totalStringSize += stringSizes[i];
   }

   BInstanceCommandDataEX* pUpdateData = (BInstanceCommandDataEX*)gRenderThread.submitCommandBegin(mCommandHandle, cRCInvokeInstanceActionScriptEX, sizeof(BInstanceCommandDataEX) + totalStringSize, 16);

   pUpdateData->method = method;
   pUpdateData->argCount = argCount;
   for (int i=0; i<argCount; i++)
   {      
      if (pArgs[i].GetType() == GFxValue::VT_String)
         pUpdateData->args[i].SetString(pArgs[i].GetString());
      else if (pArgs[i].GetType() == GFxValue::VT_StringW)
         pUpdateData->args[i].SetStringW(pArgs[i].GetStringW());
      else
         pUpdateData->args[i] = pArgs[i];
   }   

   pUpdateData->totalStringSize = totalStringSize;
   if (totalStringSize > 0)
   {
      BYTE* pStringBuffer = ((BYTE*)pUpdateData) + sizeof(BInstanceCommandDataEX);
      for (int i=0; i<argCount; i++)
      {
         pUpdateData->stringSizes[i] = stringSizes[i];
         if (stringSizes[i] > 0)
            pStringBuffer = copyGFxValueStringToBuffer(pArgs[i], stringSizes[i], pStringBuffer);
      }
   }

   pUpdateData->mInstanceSlotIndex = instanceSlotIndex;
   gRenderThread.submitCommandEnd(sizeof(BInstanceCommandDataEX) + totalStringSize, 16);
}

//============================================================================
//============================================================================
void BFlashGateway::keyEvent(BFlashMovieInstance* pInstance, const GFxKeyEvent& event)
{
   ASSERT_MAIN_THREAD
   if (!pInstance)
      return;
   
   const int instanceSlotIndex = pInstance->mInstanceSlotIndex;
   debugRangeCheck<int, int>(instanceSlotIndex, cMaxInstanceSlots);
   
   BInstanceKeyEventData* pUpdateData = (BInstanceKeyEventData*)gRenderThread.submitCommandBegin(mCommandHandle, cRCInstanceKeyEvent, sizeof(BInstanceKeyEventData), 16);

   pUpdateData->event  = event;
   pUpdateData->mInstanceSlotIndex = instanceSlotIndex;

   gRenderThread.submitCommandEnd(sizeof(BInstanceKeyEventData), 16);
}

//============================================================================
//============================================================================
void BFlashGateway::handleInput(BFlashMovieInstance* pInstance, int port, int event, int controlType, BInputEventDetail& detail)
{
   ASSERT_MAIN_THREAD

   if (!pInstance)
      return;
   
   port; detail;
   if(event==cInputEventControlStart)
   {
      int flashInputKey = -1;
      switch(controlType)
      {
         case cDpadLeft:
            flashInputKey = gFlashInputMap[BFlashInputMap::cDpadLeft].mFlashInput;
            break;
         case cStickLeftLeft:
            flashInputKey = gFlashInputMap[BFlashInputMap::cStickLeftLeft].mFlashInput;
            break;
         case cDpadRight:
            flashInputKey = gFlashInputMap[BFlashInputMap::cDpadRight].mFlashInput;
            break;
         case cStickLeftRight:
            flashInputKey = gFlashInputMap[BFlashInputMap::cStickLeftRight].mFlashInput;
            break;
         case cDpadDown:
            flashInputKey = gFlashInputMap[BFlashInputMap::cDpadDown].mFlashInput;
            break;
         case cStickLeftDown:
            flashInputKey = gFlashInputMap[BFlashInputMap::cStickLeftDown].mFlashInput;
            break;
         case cDpadUp:
            flashInputKey = gFlashInputMap[BFlashInputMap::cDpadUp].mFlashInput;
            break;
         case cStickLeftUp:
            flashInputKey = gFlashInputMap[BFlashInputMap::cStickLeftUp].mFlashInput;
            break;
         case cButtonA:
            flashInputKey = gFlashInputMap[BFlashInputMap::cButtonA].mFlashInput;
            break;
         case cButtonB:
            flashInputKey = gFlashInputMap[BFlashInputMap::cButtonB].mFlashInput;
            break;
         case cButtonX:
            flashInputKey = gFlashInputMap[BFlashInputMap::cButtonX].mFlashInput;
            break;
         case cButtonY:
            flashInputKey = gFlashInputMap[BFlashInputMap::cButtonY].mFlashInput;
            break;
         case cButtonStart:
            flashInputKey = gFlashInputMap[BFlashInputMap::cButtonStart].mFlashInput;
            break;
         case cButtonBack:
            flashInputKey = gFlashInputMap[BFlashInputMap::cButtonBack].mFlashInput;
            break;
      }

      GFxKeyEvent event(GFxEvent::KeyDown, (GFxKey::Code) flashInputKey);
      keyEvent(pInstance, event);
   }
}

//============================================================================
// BFlashGateway::releaseAllInstances
//============================================================================
void BFlashGateway::releaseAllInstances(void)
{
   ASSERT_MAIN_THREAD

   for (uint i = 0; i < mInstanceSlotsHighWaterMark; i++)
   {
      BInstanceSlot& slot = mInstanceSlots[i];

      if ((slot.mStatus != cSSFree) && (!slot.mBeingDeleted))
      {
         BDEBUG_ASSERT(slot.mpInstance);
         releaseInstance(slot.mpInstance);
      }
   }

   gRenderThread.blockUntilWorkerIdle();

   for ( ; ; )
   {
      if (mNumInstanceSlotsFree == mInstanceSlotsHighWaterMark)
         break;

      gEventDispatcher.sleep(4);
   }

   mInstanceSlotsHighWaterMark = 0;
   mFirstFreeInstanceSlotIndex = -1;
   mNumInstanceSlotsFree = 0;
}

//============================================================================
// BFlashGateway::receiveEvent
//============================================================================
bool BFlashGateway::receiveEvent(const BEvent& event, BThreadIndex threadIndex)
{
   ASSERT_MAIN_THREAD

   switch (event.mEventClass)
   {
      case cECInitDataSlotReply:
         {
            const uint dataSlotIndex = debugRangeCheck<uint, uint>(event.mPrivateData, cMaxDataSlots);
            BDataSlot& dataSlot = mDataSlots[dataSlotIndex];

            BDEBUG_ASSERT(dataSlot.mStatus == cSSPending);                  
            if (event.mPrivateData2)
               dataSlot.mStatus = cSSValid;
            else
               dataSlot.mStatus = cSSFailed;

            break;
         }
      case cECInitInstanceSlotReply:
         {
            const uint instanceSlotIndex = debugRangeCheck<uint, uint>(event.mPrivateData, cMaxInstanceSlots);
            BInstanceSlot& instanceSlot = mInstanceSlots[instanceSlotIndex];            

            BDEBUG_ASSERT(instanceSlot.mStatus == cSSPending);                  
            if (event.mPrivateData2)
               instanceSlot.mStatus = cSSValid;
            else
               instanceSlot.mStatus = cSSFailed;

            break;
         }
      case cECDeleteInstanceSlotReply:
         {
            const uint instanceSlotIndex = debugRangeCheck<uint, uint>(event.mPrivateData, cMaxInstanceSlots);
            BInstanceSlot& instanceSlot = mInstanceSlots[instanceSlotIndex];

            //BDEBUG_ASSERT(instanceSlot.mBeingDeleted);
            
            instanceSlot.mStatus = cSSFree;
            instanceSlot.mBeingDeleted = false;

            instanceSlot.mNextFreeInstanceSlotIndex = mFirstFreeInstanceSlotIndex;
            mFirstFreeInstanceSlotIndex = instanceSlotIndex;

            mNumInstanceSlotsFree++;

            break;
         }
   }

   return false;
}

//============================================================================
// BFlashGateway::initDeviceData
//============================================================================
void BFlashGateway::initDeviceData(void)
{
}

//============================================================================
// BFlashGateway::frameBegin
//============================================================================
void BFlashGateway::frameBegin(void)
{
}

//============================================================================
// BFlashGateway::processCommand
//============================================================================
void BFlashGateway::processCommand(const BRenderCommandHeader& header, const uchar* pData)
{
   ASSERT_RENDER_THREAD

   switch (header.mType)
   {
      case cRCInitDataSlot:
         {
            BDEBUG_ASSERT(header.mLen >= sizeof(BInitDataSlotData));
            const BInitDataSlotData* pInitData = reinterpret_cast<const BInitDataSlotData*>(Utils::AlignUp(pData, 16));

            const uint dataSlotIndex = debugRangeCheck<uint, uint>(pInitData->mDataHandle, cMaxDataSlots);
            BDataSlot& dataSlot = mDataSlots[dataSlotIndex];

            gFlashManager.getData(dataSlot.mName.getPtr(), (BFlashAssetCategory) pInitData->mAssetCategory, &dataSlot.mRenderDataIndex);

            int slotStatus = cSSFailed;
            if (dataSlot.mRenderDataIndex >= 0)
               slotStatus = cSSValid;
            setDataSlotStatus(dataSlotIndex, slotStatus);

            //gEventDispatcher.send(mEventHandle, mEventHandle, cECInitDataSlotReply, dataSlotIndex, (dataSlot.mRenderDataIndex >= 0));

            break;
         }
      case cRCInitInstanceSlot:
         {
            BDEBUG_ASSERT(header.mLen >= sizeof(BInitInstanceSlotData));
            const BInitInstanceSlotData* pInitData = reinterpret_cast<const BInitInstanceSlotData*>(Utils::AlignUp(pData, 16));

#ifdef USE_INSTANCE_MUTEX_LOCK
            mInstanceSlotMutex.lock();
#endif

               BInstanceSlot& instanceSlot = mInstanceSlots[debugRangeCheck<uint, uint>(pInitData->mInstanceSlotIndex, cMaxInstanceSlots)];
               debugRangeCheck<uint, uint>(instanceSlot.mDataSlotIndex, cMaxDataSlots);
               const BDataSlot& dataSlot = mDataSlots[instanceSlot.mDataSlotIndex];

               instanceSlot.mpRenderEffect = NULL;
               if (dataSlot.mRenderDataIndex != -1)
               {
                  instanceSlot.mpRenderEffect = gFlashManager.createMovie(dataSlot.mRenderDataIndex, pInitData->mbRenderToTexture);
                  BASSERT(instanceSlot.mpInstance);
                  BASSERT(instanceSlot.mpRenderEffect);
                  if (instanceSlot.mpInstance && instanceSlot.mpRenderEffect)
                  {
                     instanceSlot.mpInstance->mRenderTargetHandle = instanceSlot.mpRenderEffect->getRenderTargetHandle();
                     instanceSlot.mpInstance->mpRenderTargetTexture = instanceSlot.mpRenderEffect->getRenderTargetTexture();
                  }
               }

               BDEBUG_ASSERT(instanceSlot.mStatus == cSSPending);
               instanceSlot.mStatus = cSSFailed;
               if (instanceSlot.mpRenderEffect != NULL)
                  instanceSlot.mStatus = cSSValid;
            
#ifdef USE_INSTANCE_MUTEX_LOCK
            mInstanceSlotMutex.unlock();
#endif

            //gEventDispatcher.send(mEventHandle, mEventHandle, cECInitInstanceSlotReply, pInitData->mInstanceSlotIndex, instanceSlot.mpRenderEffect != NULL);

            break;
         }            
      case cRCDeleteInstanceSlot:
         {
            BDEBUG_ASSERT(header.mLen == sizeof(uint));
            const uint instanceSlotIndex = *reinterpret_cast<const uint*>(pData);
            debugRangeCheck<uint, uint>(instanceSlotIndex, cMaxInstanceSlots);

#ifdef USE_INSTANCE_MUTEX_LOCK
            mInstanceSlotMutex.lock();
#endif

               BInstanceSlot& instanceSlot = mInstanceSlots[instanceSlotIndex];
               //BDEBUG_ASSERT(instanceSlot.mBeingDeleted);

               if (instanceSlot.mpRenderEffect)
               {  
                  gFlashManager.releaseMovie(instanceSlot.mpRenderEffect);
                  instanceSlot.mpRenderEffect = NULL;
               }

               instanceSlot.mStatus = cSSFree;
               instanceSlot.mBeingDeleted = false;

               instanceSlot.mNextFreeInstanceSlotIndex = mFirstFreeInstanceSlotIndex;
               mFirstFreeInstanceSlotIndex = instanceSlotIndex;

               mNumInstanceSlotsFree++;

#ifdef USE_INSTANCE_MUTEX_LOCK
            mInstanceSlotMutex.unlock();
#endif

            //gEventDispatcher.send(mEventHandle, mEventHandle, cECDeleteInstanceSlotReply, instanceSlotIndex);

            break;
         }
      case cRCInvokeInstanceActionScript:
         {
            BDEBUG_ASSERT(header.mLen >= sizeof(BInstanceCommandData));
            const BInstanceCommandData* pUpdateData = reinterpret_cast<const BInstanceCommandData*>(Utils::AlignUp(pData, 16));

            BInstanceSlot& instanceSlot = mInstanceSlots[pUpdateData->mInstanceSlotIndex];


            if (instanceSlot.mpRenderEffect && 
                instanceSlot.mStatus == cSSValid && 
                !instanceSlot.mBeingDeleted)
            {
               instanceSlot.mpRenderEffect->invoke(pUpdateData->method, pUpdateData->fmt, pUpdateData->value);
            }

            break;
         }
      case cRCInvokeInstanceActionScriptEX:
         {
            BDEBUG_ASSERT(header.mLen >= sizeof(BInstanceCommandDataEX));
            const BInstanceCommandDataEX* pUpdateData = reinterpret_cast<const BInstanceCommandDataEX*>(Utils::AlignUp(pData, 16));

            BInstanceSlot& instanceSlot = mInstanceSlots[pUpdateData->mInstanceSlotIndex];

            if (instanceSlot.mpRenderEffect &&
                instanceSlot.mStatus == cSSValid &&
                !instanceSlot.mBeingDeleted
               )
            {
               if (pUpdateData->totalStringSize > 0)
               {
                  BYTE* pStringBuffer = ((BYTE*)pUpdateData) + sizeof(BInstanceCommandDataEX);
                  GFxValue args[cMaxGFXArgs];
                  for (int i=0; i<pUpdateData->argCount; i++)
                  {
                     args[i] = pUpdateData->args[i];
                     if (pUpdateData->stringSizes[i] > 0)
                        pStringBuffer = setGFxValueStringFromBuffer(args[i], pUpdateData->stringSizes[i], pStringBuffer);
                  }
                  instanceSlot.mpRenderEffect->invoke(pUpdateData->method, args, pUpdateData->argCount);
               }
               else
                  instanceSlot.mpRenderEffect->invoke(pUpdateData->method, pUpdateData->args, pUpdateData->argCount);
            }

            break;
         }
      case cRCSetInstanceVariable:
         {
            BDEBUG_ASSERT(header.mLen >= sizeof(BInstanceSetVariableData));
            const BInstanceSetVariableData* pUpdateData = reinterpret_cast<const BInstanceSetVariableData*>(Utils::AlignUp(pData, 16));

            BInstanceSlot& instanceSlot = mInstanceSlots[pUpdateData->mInstanceSlotIndex];

            if (instanceSlot.mpRenderEffect &&
                instanceSlot.mStatus == cSSValid &&
                !instanceSlot.mBeingDeleted)
            {
               instanceSlot.mpRenderEffect->setVariable(pUpdateData->variablePath, pUpdateData->value, (GFxMovie::SetVarType)pUpdateData->type);
            }

            break;
         }
      case cRCSetInstanceVariableEX:
         {
            BDEBUG_ASSERT(header.mLen >= sizeof(BInstanceSetVariableDataEX));
            const BInstanceSetVariableDataEX* pUpdateData = reinterpret_cast<const BInstanceSetVariableDataEX*>(Utils::AlignUp(pData, 16));

            BInstanceSlot& instanceSlot = mInstanceSlots[pUpdateData->mInstanceSlotIndex];

            if (instanceSlot.mpRenderEffect &&
                instanceSlot.mStatus == cSSValid &&
                !instanceSlot.mBeingDeleted)
            {
               if (pUpdateData->stringSize > 0)
               {
                  GFxValue value = pUpdateData->value;
                  setGFxValueStringFromBuffer(value, pUpdateData->stringSize, ((BYTE*)pUpdateData) + sizeof(BInstanceSetVariableDataEX));
                  instanceSlot.mpRenderEffect->setVariable(pUpdateData->variablePath, value, (GFxMovie::SetVarType)pUpdateData->type);
               }
               else
                  instanceSlot.mpRenderEffect->setVariable(pUpdateData->variablePath, pUpdateData->value, (GFxMovie::SetVarType)pUpdateData->type);
            }

            break;
         }
      case cRCSetInstanceVariableArrayEX:
         {
            BDEBUG_ASSERT(header.mLen >= sizeof(BInstanceSetVariableArrayDataEX));
            const BInstanceSetVariableArrayDataEX* pUpdateData = reinterpret_cast<const BInstanceSetVariableArrayDataEX*>(Utils::AlignUp(pData, 16));

            BInstanceSlot& instanceSlot = mInstanceSlots[pUpdateData->mInstanceSlotIndex];

            if (instanceSlot.mpRenderEffect &&
                instanceSlot.mStatus == cSSValid &&
                !instanceSlot.mBeingDeleted)
            {
               if (pUpdateData->totalStringSize > 0)
               {
                  BYTE* pStringBuffer = ((BYTE*)pUpdateData) + sizeof(BInstanceSetVariableArrayDataEX);
                  GFxValue value[cMaxGFXArgs];
                  for (int i=0; i<pUpdateData->count; i++)
                  {
                     value[i] = pUpdateData->value[i];
                     if (pUpdateData->stringSizes[i] > 0)
                        pStringBuffer = setGFxValueStringFromBuffer(value[i], pUpdateData->stringSizes[i], pStringBuffer);
                  }
                  instanceSlot.mpRenderEffect->setVariableArray(pUpdateData->variablePath, value, pUpdateData->count, pUpdateData->startIndex, (GFxMovie::SetVarType)pUpdateData->type);
               }
               else
                  instanceSlot.mpRenderEffect->setVariableArray(pUpdateData->variablePath, pUpdateData->value, pUpdateData->count, pUpdateData->startIndex, (GFxMovie::SetVarType)pUpdateData->type);
            }

            break;
         }
      case cRCSetInstanceDimension:
         {
            BDEBUG_ASSERT(header.mLen >= sizeof(BInstanceSetDimensionData));
            const BInstanceSetDimensionData* pUpdateData = reinterpret_cast<const BInstanceSetDimensionData*>(Utils::AlignUp(pData, 16));

            BInstanceSlot& instanceSlot = mInstanceSlots[pUpdateData->mInstanceSlotIndex];

            if (instanceSlot.mpRenderEffect &&
                instanceSlot.mStatus == cSSValid &&
                !instanceSlot.mBeingDeleted)
            {
               instanceSlot.mpRenderEffect->setDimensions(pUpdateData->mX, pUpdateData->mY, pUpdateData->mWidth, pUpdateData->mHeight);
            }

            break;
         }

      case cRCRenderInstance:
         {
            BDEBUG_ASSERT(header.mLen >= sizeof(BInstanceRenderData));
            const BInstanceRenderData* pUpdateData = reinterpret_cast<const BInstanceRenderData*>(Utils::AlignUp(pData, 16));

            BInstanceSlot& instanceSlot = mInstanceSlots[pUpdateData->mInstanceSlotIndex];

            if (instanceSlot.mpRenderEffect &&
                instanceSlot.mStatus == cSSValid &&
                !instanceSlot.mBeingDeleted)
            {
               instanceSlot.mpRenderEffect->render();
            }

            break;
         }
      case cRCRegisterEventHandler:
         {
            BDEBUG_ASSERT(header.mLen >= sizeof(BInstanceRegisterEventHandlerData));
            const BInstanceRegisterEventHandlerData* pUpdateData = reinterpret_cast<const BInstanceRegisterEventHandlerData*>(Utils::AlignUp(pData, 16));

            mInstanceSlotMutex.lock();

               BInstanceSlot& instanceSlot = mInstanceSlots[pUpdateData->mInstanceSlotIndex];
               if (instanceSlot.mpRenderEffect &&
                   instanceSlot.mStatus == cSSValid &&
                   !instanceSlot.mBeingDeleted)
               {
                  gFlashManager.registerFSCallbackReceiver(instanceSlot.mpRenderEffect, pUpdateData->mHandle);
               }

            mInstanceSlotMutex.unlock();

            break;
         }

      case cRCUnregisterEventHandler:
         {
            BDEBUG_ASSERT(header.mLen >= sizeof(BInstanceRegisterEventHandlerData));
            const BInstanceRegisterEventHandlerData* pUpdateData = reinterpret_cast<const BInstanceRegisterEventHandlerData*>(Utils::AlignUp(pData, 16));

            mInstanceSlotMutex.lock();

               BInstanceSlot& instanceSlot = mInstanceSlots[pUpdateData->mInstanceSlotIndex];
               if (instanceSlot.mpRenderEffect)
               {
                  gFlashManager.unregisterFSCallbackReiver(instanceSlot.mpRenderEffect, pUpdateData->mHandle);
               }

            mInstanceSlotMutex.unlock();

            break;     
         }

      case cRCInstanceKeyEvent:
         {
            BDEBUG_ASSERT(header.mLen >= sizeof(BInstanceKeyEventData));
            const BInstanceKeyEventData* pUpdateData = reinterpret_cast<const BInstanceKeyEventData*>(Utils::AlignUp(pData, 16));

            BInstanceSlot& instanceSlot = mInstanceSlots[pUpdateData->mInstanceSlotIndex];

            if (instanceSlot.mpRenderEffect &&
                instanceSlot.mStatus == cSSValid &&
                !instanceSlot.mBeingDeleted)
            {
               instanceSlot.mpRenderEffect->handleEvent(pUpdateData->event);
            }

            break;
         }
      case cRCReleaseInstanceGPUHeapTextures:
         {
            BDEBUG_ASSERT(header.mLen >= sizeof(BInstanceData));
            const BInstanceData* pUpdateData = reinterpret_cast<const BInstanceData*>(Utils::AlignUp(pData, 16));

            BInstanceSlot& instanceSlot = mInstanceSlots[pUpdateData->mInstanceSlotIndex];

            if (instanceSlot.mpRenderEffect)
            {
               instanceSlot.mpRenderEffect->releaseGPUHeapTexture();
            }

            break;
         }
      case cRCEnableBatching:
         {
            BDEBUG_ASSERT(header.mLen == sizeof(uint));
            const uint value = *reinterpret_cast<const uint*>(pData);
            bool enable = value > 0 ? true : false;
            gFlashManager.setEnableBatching(enable);
            break;
         }
      case cRCEnableForceSWFLoading:
         {
            BDEBUG_ASSERT(header.mLen == sizeof(uint));
            const uint value = *reinterpret_cast<const uint*>(pData);
            bool enable = value > 0 ? true : false;
            gFlashManager.setEnableSWFLoading(enable);
            break;
         }
      case cRCEnableWireframe:
         {
            BDEBUG_ASSERT(header.mLen == sizeof(uint));
            const uint value = *reinterpret_cast<const uint*>(pData);
            bool enable = value > 0 ? true : false;
            gFlashManager.setEnableWireframe(enable);
            break;
         }
      case cRCUnloadPregameUITextures:
         {
            BDEBUG_ASSERT(header.mLen == sizeof(uint));
            gFlashManager.unloadPregameUITextures();
            break;
         }
      case cRCLoadPregameUITextures:
         {
            BDEBUG_ASSERT(header.mLen == sizeof(uint));
            gFlashManager.loadPregameUITextures();
            break;
         }
      case cRCInitFontLibrary:
         {
            BDEBUG_ASSERT(header.mLen >= sizeof(BInitFontLibraryData));
            const BInitFontLibraryData* pUpdateData = reinterpret_cast<const BInitFontLibraryData*>(Utils::AlignUp(pData, 16));            
            gFlashManager.initFonts(pUpdateData->mLanguage, pUpdateData->mDirectory);
            break;
         }
      case cRCClearInstanceMemoryStats:
         {
            BDEBUG_ASSERT(header.mLen == sizeof(uint));
            #ifndef BUILD_FINAL
            const uint value = *reinterpret_cast<const uint*>(pData);
            gFlashManager.clearInstanceMemoryStats(value);
            #endif
            break;
         }
      case cRCClearProtoMemoryStats:
         {
            BDEBUG_ASSERT(header.mLen == sizeof(uint));
            #ifndef BUILD_FINAL
            const uint value = *reinterpret_cast<const uint*>(pData);
            gFlashManager.clearProtoMemoryStats(value);
            #endif
            break;
         }
   }   
}

//============================================================================
// BFlashGateway::frameEnd
//============================================================================
void BFlashGateway::frameEnd(void)
{
}

//============================================================================
// BFlashGateway::deinitDeviceData
//============================================================================
void BFlashGateway::deinitDeviceData(void)
{
}

//============================================================================
// BFlashGateway::initFontLibrary()
//============================================================================
void BFlashGateway::initFontLibrary(const BString& language, long directory)
{   
   BInitFontLibraryData* pUpdateData = (BInitFontLibraryData*)gRenderThread.submitCommandBegin(mCommandHandle, cRCInitFontLibrary, sizeof(BInitFontLibraryData), 16);

   pUpdateData->mDirectory = directory;
   pUpdateData->mLanguage = language;   
   gRenderThread.submitCommandEnd(sizeof(BInitFontLibraryData), 16);
}

//============================================================================
// BFlashGateway::setEnableBatching()
//============================================================================
void BFlashGateway::setEnableBatching(bool bEnable)
{
   gRenderThread.submitCommand(*this, cRCEnableBatching, bEnable ? 1 : 0);
}

//============================================================================
// BFlashGateway::setEnableForceSWFLoading()
//============================================================================
void BFlashGateway::setEnableForceSWFLoading(bool bEnable)
{
   gRenderThread.submitCommand(*this, cRCEnableForceSWFLoading, bEnable ? 1 : 0);
}

//============================================================================
// BFlashGateway::setEnableWireframe()
//============================================================================
void BFlashGateway::setEnableWireframe(bool bEnable)
{
   gRenderThread.submitCommand(*this, cRCEnableWireframe, bEnable ? 1 : 0);
}

//============================================================================
// BFlashGateway::loadPregameUITextures()
//============================================================================
void BFlashGateway::loadPregameUITextures()
{
   gRenderThread.submitCommand(*this, cRCLoadPregameUITextures, 0);
}

//============================================================================
// BFlashGateway::unloadPregameUITextures()
//============================================================================
void BFlashGateway::unloadPregameUITextures()
{
   gRenderThread.submitCommand(*this, cRCUnloadPregameUITextures, 0);
}

//============================================================================
// BFlashGateway::clearInstanceMemoryStats()
//============================================================================
void BFlashGateway::clearInstanceMemoryStats(int category)
{
   gRenderThread.submitCommand(*this, cRCClearInstanceMemoryStats, category);
}

//============================================================================
// BFlashGateway::clearProtoMemoryStats()
//============================================================================
void BFlashGateway::clearProtoMemoryStats(int category)
{
   gRenderThread.submitCommand(*this, cRCClearProtoMemoryStats, category);
}

//============================================================================
//============================================================================
uint BFlashGateway::getGFxValueStringSizeForBuffer(const GFxValue& value)
{
   uint stringSize = 0;
   int valType = value.GetType();
   if (valType == GFxValue::VT_String || valType == GFxValue::VT_ConvertString)
   {
      const char* pString = value.GetString();
      if (pString)
         stringSize = strlen(pString);
      stringSize += 1;
   }
   else if (valType == GFxValue::VT_StringW || valType == GFxValue::VT_ConvertStringW)
   {
      const WCHAR* pString = value.GetStringW();
      if (pString)
         stringSize = wcslen(pString) * 2;
      stringSize += 2;
   }
   return stringSize;
}

//============================================================================
//============================================================================
BYTE* BFlashGateway::copyGFxValueStringToBuffer(const GFxValue& value, uint stringSize, BYTE* pStringBuffer)
{
   if (stringSize > 0)
   {
      int valType = value.GetType();
      if (valType == GFxValue::VT_String || valType == GFxValue::VT_ConvertString)
      {
         if (stringSize > 1)
         {
//-- FIXING PREFIX BUG ID 6414
            const BYTE* pValString = reinterpret_cast<const BYTE*>(value.GetString());
//--
            memcpy(pStringBuffer, pValString, stringSize - 1);
         }
         pStringBuffer[stringSize - 1] = NULL;
      }
      else
      {
         if (stringSize > 2)
         {
//-- FIXING PREFIX BUG ID 6415
            const BYTE* pValString = reinterpret_cast<const BYTE*>(value.GetStringW());
//--
            memcpy(pStringBuffer, pValString, stringSize - 2);
         }
         pStringBuffer[stringSize - 1] = NULL;
         pStringBuffer[stringSize - 2] = NULL;
      }
   }
   return pStringBuffer + stringSize;
}

//============================================================================
//============================================================================
BYTE* BFlashGateway::setGFxValueStringFromBuffer(GFxValue& value, uint stringSize, BYTE* pStringBuffer)
{
   if (stringSize > 0)
   {
      int valType = value.GetType();
      switch (valType)
      {
         case GFxValue::VT_String:
            value.SetString((const char*)pStringBuffer);
            break;
         case GFxValue::VT_ConvertString:
            value.SetString((const char*)pStringBuffer);
            value.SetConvertString();
            break;
         case GFxValue::VT_StringW:
            value.SetStringW((const WCHAR*)pStringBuffer);
            break;
         case GFxValue::VT_ConvertStringW:
            value.SetStringW((const WCHAR*)pStringBuffer);
            value.SetConvertStringW();
            break;
      }
   }
   return pStringBuffer + stringSize;
}

//============================================================================
//============================================================================
void BFlashGateway::setDataSlotStatus(int slotIndex, int status)
{
   BScopedLightWeightMutex lock(mDataSlotMutex);
   rangeCheck(slotIndex, cMaxDataSlots);
   mDataSlots[slotIndex].mStatus = (eSlotStatus)status;
}

//============================================================================
//============================================================================
void BFlashGateway::getDataSlotStatus(int slotIndex, int& status)
{
   BScopedLightWeightMutex lock(mDataSlotMutex);
   rangeCheck(slotIndex, cMaxDataSlots);
   status = mDataSlots[slotIndex].mStatus;   
}

//============================================================================
//============================================================================
void BFlashGateway::findDataSlot(const char* pName, int& index)
{
   BScopedLightWeightMutex lock(mDataSlotMutex);

   index = -1;
   for (int i = 0; i < (int) mNumDataSlotsInUse; i++)
   {
      if (mDataSlots[i].mStatus != cSSFree)
      {
         if (stricmp(pName, mDataSlots[i].mName.getPtr()) == 0)
         {
            index = i;
            break;
         }
      }
   }
}