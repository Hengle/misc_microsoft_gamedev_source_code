// Effect compilation on 360 is way too slow to be practical.
#if 0
//============================================================================
//
// File: effectCompiler.cpp
//
//============================================================================
#include "xrender.h"
#include "effectCompiler.h"
#include "renderCommand.h"
//#include "threading\interlocked.h"
#include "asyncFileManager.h"
#include "renderEventClasses.h"
#include "consoleOutput.h"

BEffectCompiler gAsyncEffectCompiler;

BEffectCompilerRequestPacket::BEffectCompilerRequestPacket() :
   mRequestID(0),
   mDirID(-1),
   mFlags(0),
   mPrivateData(0),
   mEventHandle(0)
{
   mSucceeded = false;
   mPending = false;
}

BEffectCompilerRequestPacket::BEffectCompilerRequestPacket(long dirID, const char* pFileName, DWORD flags, const BD3DXMacroArray& defines, BEventReceiverHandle eventHandle) :
   mRequestID(0),
   mDirID(dirID),
   mFilename(pFileName),
   mFlags(flags),
   mDefines(defines),
   mPrivateData(0),
   mEventHandle(eventHandle)
{
   mSucceeded = false;
   mPending = false;
}

BEffectCompiler::BEffectCompiler() :
   BFiber(),
   mDefaultDirID(0),
   mEventHandle(cInvalidEventReceiverHandle),
   mNumOutstandingRequests(0),
   mNextRequestID(1),
   mQueuedEvents(256)
{
   mCurEffectEvent.clear();
   mCurFileEvent.clear();
}

BEffectCompiler::~BEffectCompiler()
{
}

void BEffectCompiler::init(long defaultDirID)
{  
   if (mInitialized)
      return;
      
   mInitialized = true;
   
   mDefaultDirID = defaultDirID;
   
   mNumOutstandingRequests = 0;
   mNextRequestID = 1;
            
   mEventHandle = gEventDispatcher.addClient(this, cThreadIndexRenderHelper);
}

void BEffectCompiler::deinit(void)
{
   if (!mInitialized)
      return;
   
   while (getNumOutstandingRequests())
   {
      gEventDispatcher.wait(0, NULL, 33);
   }
         
   gEventDispatcher.removeClientDeferred(mEventHandle);
   mEventHandle = cInvalidEventReceiverHandle;
   
   mInitialized = false;
}

BEffectCompilerRequestPacket* BEffectCompiler::newRequestPacket(void) 
{
   return new BEffectCompilerRequestPacket;
}

void BEffectCompiler::submitRequest(BEffectCompilerRequestPacket* pEffectPacket)
{
   BDEBUG_ASSERT(mInitialized && pEffectPacket);
   BDEBUG_ASSERT(pEffectPacket->getEventHandle() != cInvalidEventReceiverHandle);
   
   pEffectPacket->setPending(true);
   pEffectPacket->setRequestID(mNextRequestID);
   mNextRequestID++;

   gEventDispatcher.send(cInvalidEventReceiverHandle, mEventHandle, cRenderEventClassEffectCompileRequest, 0, 0, pEffectPacket);
   
   Sync::InterlockedIncrementExport(&mNumOutstandingRequests);
}

uint BEffectCompiler::getNumOutstandingRequests(void) const 
{
   Sync::MemoryBarrier();
   
   return mNumOutstandingRequests;
}
     
bool BEffectCompiler::processAsyncFileEvent(const BEvent& event)
{
   if (!mProcessingRequest)
   {
      BFATAL_FAIL("BEffectCompiler::processAsyncFileEvent: unexpected async file event");   
   }
   
   BAsyncFileManager::BRequestPacket* pFileRequestPacket = reinterpret_cast<BAsyncFileManager::BRequestPacket*>(event.mpPayload);
         
   BEffectCompilerRequestPacket* pCurEffectPacket = reinterpret_cast<BEffectCompilerRequestPacket*>(mCurEffectEvent.mpPayload);
   
   if (!pFileRequestPacket->getPrivateData0())
      initFiber();
      
   mCurFileEvent = event;
   
   int status = activateFiber();
               
   if (status == cFiberDone)
   {
      deinitFiber();
      
      gEventDispatcher.send(
         mEventHandle, 
         pCurEffectPacket->getEventHandle(), 
         cRenderEventClassEffectCompileResults, 
         mCurEffectEvent.mPrivateData, 
         mCurEffectEvent.mPrivateData2, 
         mCurEffectEvent.mpPayload);
            
      mCurEffectEvent.mpPayload = NULL;
      
      mProcessingRequest = false;
      
      Sync::InterlockedDecrementExport(&mNumOutstandingRequests);
      
      if (!mQueuedEvents.getEmpty())
      {  
         BEvent event;
         mQueuedEvents.popBack(event);
         initiateEffectCompile(event);
      }
   }
   else if (status == cFiberNeedsFile)
   {
      BAsyncFileManager::BRequestPacket* pFileRequest = gAsyncFileManager.newRequestPacket();
      
      pFileRequest->setFilename(mIncludeFilename);
      pFileRequest->setDirID((pCurEffectPacket->getDirID() != -1) ? pCurEffectPacket->getDirID() : mDefaultDirID);
      pFileRequest->setPrivateData0(1);
      pFileRequest->setReceiverHandle(mEventHandle);

      gAsyncFileManager.submitRequest(pFileRequest);
   }
   else
   {
      BFATAL_FAIL("BEffectCompiler::processAsyncFileEvent: invalid fiber return status");
   }         
               
   return false;
}

void BEffectCompiler::initiateEffectCompile(const BEvent& event)
{
   mCurEffectEvent = event;
   mProcessingRequest = true;
   
   BAsyncFileManager::BRequestPacket* pFileRequest = gAsyncFileManager.newRequestPacket();
   
   BEffectCompilerRequestPacket* pCurEffectPacket = reinterpret_cast<BEffectCompilerRequestPacket*>(mCurEffectEvent.mpPayload);
   
   pFileRequest->setFilename(pCurEffectPacket->getFilename());
   pFileRequest->setDirID((pCurEffectPacket->getDirID() != -1) ? pCurEffectPacket->getDirID() : mDefaultDirID);
   pFileRequest->setReceiverHandle(mEventHandle);
         
   gAsyncFileManager.submitRequest(pFileRequest);
}

bool BEffectCompiler::processEffectCompileEvent(const BEvent& event)
{
   if (mProcessingRequest)
      mQueuedEvents.pushFront(event);
   else
      initiateEffectCompile(event);
   
   return true;
}

bool BEffectCompiler::receiveEvent(const BEvent& event, BThreadIndex threadIndex)
{
   if (event.mEventClass == cEventClassAsyncFile)
   {
      return processAsyncFileEvent(event);      
   }
   else if (event.mEventClass == cRenderEventClassEffectCompileRequest)
   {
      return processEffectCompileEvent(event);      
   }
           
   return false;
}      
     
HRESULT BEffectCompiler::BFXLEffectIncludeInterface::Open(
   D3DXINCLUDE_TYPE IncludeType,
   LPCSTR pFileName,
   LPCVOID pParentData,
   LPCVOID *ppData,
   UINT *pBytes,
   LPSTR pFullPath,
   DWORD cbFullPath)
{
   BEffectCompilerRequestPacket* pCurEffectPacket = reinterpret_cast<BEffectCompilerRequestPacket*>(mpCompiler->mCurEffectEvent.mpPayload);
   
   BFixedString256 filename(pCurEffectPacket->getFilename());
   filename.removeFilename();
   //if (!filename.empty())
   //   filename.appendChar('\\');
   filename.append(pFileName);
      
   mpCompiler->mIncludeFilename.set(filename.c_str());

   strcpy_s(pFullPath, cbFullPath, filename.c_str());
      
   mpCompiler->fiberReturn(cFiberNeedsFile);
   
   BAsyncFileManager::BRequestPacket* pFileRequestPacket = reinterpret_cast<BAsyncFileManager::BRequestPacket*>(mpCompiler->mCurFileEvent.mpPayload);
   
   if (!pFileRequestPacket->getSucceeded())
      return E_FAIL;
      
   uchar* pData = new uchar[pFileRequestPacket->getDataLen()];
   *ppData = pData;
   Utils::FastMemCpy(pData, pFileRequestPacket->getData(), pFileRequestPacket->getDataLen());
   *pBytes = pFileRequestPacket->getDataLen();
   
   return S_OK;
}                     

HRESULT BEffectCompiler::BFXLEffectIncludeInterface::Close(LPCVOID pData)
{
   delete[] pData;
   return S_OK;
}
  
int BEffectCompiler::fiberMain(uint64 privateData)
{
   BAsyncFileManager::BRequestPacket curFilePacket(*reinterpret_cast<BAsyncFileManager::BRequestPacket*>(mCurFileEvent.mpPayload));
   if (!curFilePacket.getSucceeded())
   {
      gConsoleOutput.output(cMsgError, "BEffectCompiler::fiberMain: Async file IO request of file %s failed", curFilePacket.getFilename().c_str());
      return cFiberDone;
   }
      
   BEffectCompilerRequestPacket* pCurEffectPacket = reinterpret_cast<BEffectCompilerRequestPacket*>(mCurEffectEvent.mpPayload);
               
   BFXLEffectIncludeInterface includeInterface(this);
   
   ID3DXBuffer* pCompiledData = NULL;
   ID3DXBuffer* pErrorMessages = NULL;

   HRESULT hres = FXLCompileEffect(
      (char*)curFilePacket.getData(),
      curFilePacket.getDataLen(),
      pCurEffectPacket->getDefines().size() ? pCurEffectPacket->getDefines().getPtr() : NULL,  
      &includeInterface, 
      pCurEffectPacket->getFlags(), 
      &pCompiledData, 
      &pErrorMessages);

   if (SUCCEEDED(hres))
   {
      pCurEffectPacket->setSucceeded(true);
      pCurEffectPacket->setCompiledData(reinterpret_cast<const uchar*>(pCompiledData->GetBufferPointer()), pCompiledData->GetBufferSize());
   }

   if (pCompiledData) { pCompiledData->Release(); pCompiledData = NULL; }

   if (FAILED(hres))
   {
      char buf[512];
      sprintf_s(buf, sizeof(buf), "BFXLEffect::createFromFile: Effect \"%s\" failed to compile.", curFilePacket.getFilename().c_str());

      OutputDebugStringA(buf);
      OutputDebugStringA("\n");
      OutputDebugStringA((LPCSTR)pErrorMessages->GetBufferPointer());
      OutputDebugStringA("\n");

      if (pErrorMessages) { pErrorMessages->Release(); pErrorMessages = NULL; }

      //BASSERT(0 && "Effect compilation failed!");
   }
   else
   {
      if (pErrorMessages) { pErrorMessages->Release(); pErrorMessages = NULL; }
   }  
   
   return cFiberDone;
}
#endif
