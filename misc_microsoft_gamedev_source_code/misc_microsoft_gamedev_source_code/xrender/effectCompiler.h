// Effect compilation on 360 is way too slow to be practical.

#if 0
//============================================================================
//
// File: effectCompiler.h
// rg [2/16/06] - Unfortunately, until we move the async file manager to a worker 
// thread, this class is so slow at include file loading that it's not really 
// usable. A cache would also help.
// 
//  Copyright (c) 2006, Ensemble Studios
//
//============================================================================
#pragma once

// xcore
#include "containers\queue.h"
#include "memory\alignedAlloc.h"
#include "threading\eventDispatcher.h"
#include "threading\stateSwitch.h"

class BEffectCompiler;

typedef BDynamicArray<D3DXMACRO> BD3DXMacroArray;

struct BD3DXMACROInit : D3DXMACRO
{
   BD3DXMACROInit(const char* pName, const char* pDefinition)
   {
      Name = pName;
      Definition = pDefinition;
   }
};

class BEffectCompilerRequestPacket : public BEventPayload
{
public:
   BEffectCompilerRequestPacket();
   BEffectCompilerRequestPacket(long dirID, const char* pFileName, DWORD flags, const BD3DXMacroArray& defines, BEventReceiverHandle eventHandle);
   
   virtual ~BEffectCompilerRequestPacket()
   {
   }
            
   void setDirID(long dirID) { mDirID = dirID; }
   long getDirID(void) const { return mDirID; }

   void setFilename(const char* pFilename) { mFilename.set(pFilename); }         
   const BFixedString256& getFilename(void) const { return mFilename; }

   void setFlags(DWORD flags) { mFlags = flags; }
   DWORD getFlags(void) const { return mFlags; }

   void setDefines(const BD3DXMacroArray& array) { mDefines = array; }
   const BD3DXMacroArray& getDefines(void) const { return mDefines; }

   void setEventHandle(BEventReceiverHandle handle) { mEventHandle = handle; }
   BEventReceiverHandle getEventHandle(void) const { return mEventHandle; }

   void setSucceeded(bool succeeded) { mSucceeded = succeeded; }
   bool getSucceeded(void) const { return mSucceeded; }

   void setPending(bool pending) { mPending = pending; }
   bool getPending(void) const { return mPending; }

   void setCompiledData(const uchar* pData, uint dataLen) { mCompiledData.resize(0); mCompiledData.pushBack(pData, dataLen); }
   const BDynamicArray<uchar>& getCompiledData(void) const { return mCompiledData; }
   
   void setPrivateData(uint privateData) { mPrivateData = privateData; }
   uint getPrivateData(void) const { return mPrivateData; }
   
   void setRequestID(uint64 requestID) { mRequestID = requestID; }
   uint64 getRequestID(void) const { return mRequestID; }

private:
   BEventReceiverHandle mEventHandle;
   uint64 mRequestID;
   
   long mDirID;
   DWORD mFlags;
   uint mPrivateData;
   
   BDynamicArray<uchar> mCompiledData;
   BD3DXMacroArray mDefines;
   BFixedString256 mFilename;
   
   bool mSucceeded : 1;
   bool mPending : 1;

   virtual void deleteThis(bool delivered)
   {
      delete this;
   }
   
   friend class BEffectCompiler;
};

class BEffectCompiler : public BEventReceiverInterface, BFiber
{
public:
   BEffectCompiler();
      
   virtual ~BEffectCompiler();
   
   void init(long defaultDirID = 0);
   void deinit(void);
   
   // This is only safe to call before kicking off work, for example in the main thread after init() is called.
   void setDefaultDirID(long defaultDirID) { mDefaultDirID = defaultDirID; }
   long getDefaultDirID(void) const { return mDefaultDirID; }
         
   BEffectCompilerRequestPacket* newRequestPacket(void);
   
   void submitRequest(BEffectCompilerRequestPacket* pEffectPacket);
   
   uint getNumOutstandingRequests(void) const;
   
private:
   BEventReceiverHandle mEventHandle;
   
   long mDefaultDirID;

   typedef BQueue<BEvent> BEventArray;
   BEventArray mQueuedEvents;

   BEvent mCurEffectEvent;
   BEvent mCurFileEvent;
   BFixedString256 mIncludeFilename;
   uint64 mNextRequestID;

   bool mProcessingRequest : 1;
   bool mInitialized : 1;

   volatile LONG mNumOutstandingRequests;

   enum
   {
      cFiberDone,
      cFiberNeedsFile
   };

   bool processAsyncFileEvent(const BEvent& event);
   
   void initiateEffectCompile(const BEvent& event);
   
   bool processEffectCompileEvent(const BEvent& event);
   
   virtual bool receiveEvent(const BEvent& event, BThreadIndex threadIndex);

   friend class BFXLEffectIncludeInterface;
   
   class BFXLEffectIncludeInterface : public ID3DXInclude 
   {
   public:
      BFXLEffectIncludeInterface(BEffectCompiler* pCompiler) : mpCompiler(pCompiler) { }

      virtual HRESULT Open(
         D3DXINCLUDE_TYPE IncludeType,
         LPCSTR pFileName,
         LPCVOID pParentData,
         LPCVOID *ppData,
         UINT *pBytes,
         LPSTR pFullPath,
         DWORD cbFullPath);
      
      virtual HRESULT Close(LPCVOID pData);
      
   protected:
      BEffectCompiler* mpCompiler;      
   };

   virtual int fiberMain(uint64 privateData);
};

extern BEffectCompiler gAsyncEffectCompiler;
#endif

