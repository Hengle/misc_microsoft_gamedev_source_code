//============================================================================
// flashgateway.h
// Ensemble Studios (c) 2006
//============================================================================
#pragma once
#include "renderThread.h"
#include "flashmovie.h"

class BInputEventDetail;
//============================================================================
// class BFlashMovieInstance
//============================================================================
class BFlashMovieInstance
{
   public:
      BFlashMovieInstance();
     ~BFlashMovieInstance();

      void render();
      void setVariable(const char* variablePath, const char* value, GFxMovie::SetVarType type = GFxMovie::SV_Normal);
      void setVariable(const char* variablePath, const GFxValue& value, GFxMovie::SetVarType type = GFxMovie::SV_Normal);
      void setVariableArray(const char* variablePath, GFxValue* pValue, int count, int startIndex = 0, GFxMovie::SetVarType type = GFxMovie::SV_Normal);
      void invokeActionScript(const char* method, const char* fmt, const char* value);
      void invokeActionScript(const char* method, const GFxValue* pArgs, int numArgs);
      void keyEvent(const GFxKeyEvent& event);
      void registerEventHandler(BEventReceiverHandle handle);
      void unregisterEventHandler(BEventReceiverHandle handle);
      void setDimension(int x, int y, int width, int height);
      void releaseGPUHeapTextures();

      // Render thread functions
      void workerRender();

   int mInstanceSlotIndex;
   BManagedTextureHandle mRenderTargetHandle;
   IDirect3DTexture9* mpRenderTargetTexture;
};

struct BFlashLoadMovieData
{
   BFixedString256 mFilename;
};

//============================================================================
// class BFlashGateway
//============================================================================
class BFlashGateway : public BRenderCommandListener, public BEventReceiver
{
public:
    BFlashGateway();
   ~BFlashGateway();

   void init(void);
   void deinit(void);
   
   typedef int BDataHandle;
   void getOrCreateData(const char* pName, uint assetCategory, BDataHandle& dataHandle);

   BFlashMovieInstance* createInstance(BDataHandle dataHandle, bool bRenderToTexture = false);
   void releaseInstance(BFlashMovieInstance* pInstance);
   void registerEventHandler(BFlashMovieInstance* pInstance, BEventReceiverHandle handle);
   void unregisterEventHandler(BFlashMovieInstance* pInstance, BEventReceiverHandle handle);
   void render(BFlashMovieInstance* pInstance);
   void setInstanceVariable(BFlashMovieInstance* pInstance, const char* variablePath, const char* value, int type);
   void setInstanceVariable(BFlashMovieInstance* pInstance, const char* variablePath, const GFxValue& value, int type);
   void setInstanceVariableArray(BFlashMovieInstance* pInstance, const char* variablePath, GFxValue* pValue, int count, int startIndex, GFxMovie::SetVarType type);
   void invokeInstanceActionScript(BFlashMovieInstance* pInstance, const char* method, const char* fmt, const char* value);
   void invokeInstanceActionScript(BFlashMovieInstance* pInstance, const char* method, const GFxValue* pArgs, int argCount);
   void handleInput(BFlashMovieInstance* pInstance, int port, int event, int controlType, BInputEventDetail& detail);
   void setInstanceDimension(BFlashMovieInstance* pInstance, int x, int y, int width, int height);
   void releaseInstanceGPUHeapTextures(BFlashMovieInstance* pInstance);
      
   void initFontLibrary(const BString& language, long directory);

   void setEnableBatching(bool bEnable);
   void setEnableForceSWFLoading(bool bEnable);
   void setEnableWireframe(bool bEnable);

   void keyEvent(BFlashMovieInstance* pInstance, const GFxKeyEvent& event);

   void loadPregameUITextures();
   void unloadPregameUITextures();

   IDirect3DTexture9* getTexture(int instanceSlot);
   void workerRender(int instanceSlot);
   

   void releaseAllInstances(void);

   uint getNumInstances(void) const       { ASSERT_MAIN_THREAD return mTotalInstances; }
   uint getNumInstanceSlotsFree(void)     { ASSERT_MAIN_THREAD return mNumInstanceSlotsFree; }
   uint getNumInstanceHighWaterMark(void) { ASSERT_MAIN_THREAD return mInstanceSlotsHighWaterMark; }
   uint getNumDataSlotsInUse(void)        { ASSERT_MAIN_THREAD return mNumDataSlotsInUse; }

   void clearInstanceMemoryStats(int category);
   void clearProtoMemoryStats(int category);
      
private:
   enum eSlotStatus
   {
      cSSFree,

      cSSPending,
      cSSFailed,
      cSSValid,

      cSSTotal
   };

   struct BDataSlot
   {
      // All member variables that don't start with "mRender" can generally only be manipulated from the sim thread!
      BSimString mName;

      // mRenderDataIndex can only be read/written from the render thread! 
      int mRenderDataIndex;

      eSlotStatus mStatus;
   };

   void setDataSlotStatus(int slotIndex, int  status);
   void getDataSlotStatus(int slotIndex, int& status);
   void findDataSlot(const char* pName, int& index);

   // Can't use a dynamic array because this array will be read/written by multiple threads.
   enum { cMaxDataSlots = 512 };
   BDataSlot mDataSlots[cMaxDataSlots];
   uint mNumDataSlotsInUse;
   BLightWeightMutex mDataSlotMutex;

   struct BInstanceSlot
   {
      // All member variables that don't start with "mRender" can generally only be manipulated from the sim thread!
      union
      {
         BDataHandle mDataSlotIndex;
         int mNextFreeInstanceSlotIndex;
      };

      BFlashMovieInstance* mpInstance;

      // mpRenderEffect can only be read/written from the render thread!
      BFlashMovie* mpRenderEffect;

      uchar mStatus;
      bool mBeingDeleted : 1;
   };

   // Can't use a dynamic array because this array will be read/written by multiple threads.
   enum { cMaxInstanceSlots = 8192,
          cMaxGFXArgs = 16,
         };
   BInstanceSlot mInstanceSlots[cMaxInstanceSlots];
   uint mInstanceSlotsHighWaterMark;
   int mFirstFreeInstanceSlotIndex;
   uint mNumInstanceSlotsFree;
   BLightWeightMutex mInstanceSlotMutex;
   
   uint mTotalInstances;

   bool mInitialized : 1;

   struct BInitFontLibraryData
   {
      BFixedString<128> mLanguage;
      int mDirectory;
   };

   struct BInitDataSlotData
   {
      int mAssetCategory;
      int mDataHandle;
   };

   struct BInitInstanceSlotData
   {
      uint mInstanceSlotIndex;
      bool mbRenderToTexture : 1;
   };

   struct BInstanceCommandData
   {
      BFixedString<128> method;
      BFixedString<64>  fmt;
      BFixedString<64>  value;
      uint              mInstanceSlotIndex;
   };

   struct BInstanceCommandDataEX
   {
      BFixedString<128> method;
      GFxValue          args[cMaxGFXArgs];
      uint              stringSizes[cMaxGFXArgs];
      uint              totalStringSize;
      int               argCount;
      uint              mInstanceSlotIndex;
   };

   struct BInstanceSetVariableData
   {
      BFixedString<256> value;
      BFixedString<128> variablePath;      
      int               type;
      uint              mInstanceSlotIndex;
   };

   struct BInstanceSetVariableDataEX
   {
      BFixedString<128> variablePath;
      GFxValue          value;
      int               type;
      uint              mInstanceSlotIndex;
      uint              stringSize;
   };

   struct BInstanceSetVariableArrayDataEX
   {
      BFixedString<128> variablePath;
      GFxValue          value[cMaxGFXArgs];
      uint              stringSizes[cMaxGFXArgs];
      uint              totalStringSize;
      int               count;
      int               startIndex;
      int               type;
      uint              mInstanceSlotIndex;
   };

   struct BInstanceSetDimensionData
   {
      int               mX;
      int               mY;
      int               mWidth;
      int               mHeight;
      uint              mInstanceSlotIndex;
   };

   struct BInstanceRenderData
   {
      uint mInstanceSlotIndex;
      BManagedTextureHandle mTexture;
   };

   struct BInstanceData 
   {
      uint mInstanceSlotIndex;
   };

   struct BInstanceRegisterEventHandlerData
   {
      BEventReceiverHandle mHandle;
      uint                 mInstanceSlotIndex;
   };

   struct BInstanceKeyEventData
   {
      GFxKeyEvent event;
      uint        mInstanceSlotIndex;
   };

   enum 
   {
      cRCInitDataSlot,

      cRCInitInstanceSlot,
      cRCUpdateInstanceMatrix,
      cRCDeleteInstanceSlot,
      cRCInvokeInstanceActionScript,
      cRCSetInstanceVariable,
      cRCInstanceKeyEvent,
      cRCRegisterEventHandler,
      cRCUnregisterEventHandler,
      cRCRenderInstance,
      cRCSetInstanceDimension,
      cRCInvokeInstanceActionScriptEX,
      cRCSetInstanceVariableEX,
      cRCReleaseInstanceGPUHeapTextures,
      cRCEnableBatching,
      cRCEnableForceSWFLoading,
      cRCSetInstanceVariableArrayEX,
      cRCUnloadPregameUITextures,
      cRCLoadPregameUITextures,
      cRCInitFontLibrary,
      cRCEnableWireframe,
      cRCClearInstanceMemoryStats,
      cRCClearProtoMemoryStats,
   };

   enum 
   {
      cECInitDataSlotReply = cEventClassFirstUser,
      cECInitInstanceSlotReply,
      cECDeleteInstanceSlotReply
   };

   void clear(void);

   virtual bool receiveEvent(const BEvent& event, BThreadIndex threadIndex);

   virtual void initDeviceData(void);
   virtual void frameBegin(void);
   virtual void processCommand(const BRenderCommandHeader& header, const uchar* pData);
   virtual void frameEnd(void);
   virtual void deinitDeviceData(void);

   uint getGFxValueStringSizeForBuffer(const GFxValue& value);
   BYTE* copyGFxValueStringToBuffer(const GFxValue& value, uint stringSize, BYTE* pStringBuffer);
   BYTE* setGFxValueStringFromBuffer(GFxValue& value, uint stringSize, BYTE* pStringBuffer);

};

//============================================================================
// externs
//============================================================================
extern BFlashGateway gFlashGateway;


