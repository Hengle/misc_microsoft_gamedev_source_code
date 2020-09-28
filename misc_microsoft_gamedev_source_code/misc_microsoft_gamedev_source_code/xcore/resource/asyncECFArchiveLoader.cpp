//============================================================================
//
// File: asyncECFArchiveLoader.cpp
// Copyright (c) 2007, Ensemble Studios
//
//============================================================================
#include "xcore.h"
#include "..\xsystem\timelineprofilersample.h"
#include "asyncECFArchiveLoader.h"
#include "math\randomUtils.h"
#include "hash\teaCrypt.h"
#include "resource\ecfUtils.h"
#include "resource\ecfArchiveTypes.h"
#include "resource\ecfHeaderIDs.h"
#include "stream\byteStream.h"
#include "hash\digitalSignature.h"
#include "pixHelpers.h"
#include "file\lowLevelFileIO.h"

//==============================================================================
// BAsyncECFArchiveLoader::BAsyncECFArchiveLoader
//==============================================================================
BAsyncECFArchiveLoader::BAsyncECFArchiveLoader()
{
   clear();
   
   mRequestQueue.reserve(8);
}

//==============================================================================
// BAsyncECFArchiveLoader::~BAsyncECFArchiveLoader
//==============================================================================
BAsyncECFArchiveLoader::~BAsyncECFArchiveLoader()
{
}

//==============================================================================
// BAsyncECFArchiveLoader::clear
//==============================================================================
void BAsyncECFArchiveLoader::clear(void)
{
   mPublicKeys.resize(0);
   
   mRequestQueue.resize(0);
   mpCurRequest = NULL;
   
   mArchiveFileHandle = INVALID_HANDLE_VALUE;
   mArchiveFileSize = 0;
   mArchiveFileOfs = 0;

   Utils::ClearObj(mpBuffers);

   mCurBufIndex = 0;

   mOverlappedEvent.reset();
   Utils::ClearObj(mOverlapped);
   mCurReadSize = 0;
   
   mCurChunkIndex = -1;
   mCurChunkOfs = 0;
   
   mCurState = cStateOpenArchive;
   
   mReadIsPending = false;
   mIsPaused = false;
   
   mChunkInfo.clear();
   
   mThreadIndex = cThreadIndexInvalid;

   mpLowLevelFileIO = ILowLevelFileIO::getDefault();
   
   mBufSize = cDefaultBufSize;
}

//==============================================================================
// BAsyncECFArchiveLoader::init
//==============================================================================
void BAsyncECFArchiveLoader::init(
   eThreadIndex threadIndex, 
   uint numPublicKeys, 
   const BSHA1* pPublicKeys, 
   const uint64 decryptKey[3], 
   ILowLevelFileIO* pLowLevelFileIO, 
   uint bufSize)
{
   BDEBUG_ASSERT(gEventDispatcher.getThreadId(threadIndex));
   BVERIFY(numPublicKeys);
   if (!bufSize)
      bufSize = cDefaultBufSize;
   
   if (cInvalidEventReceiverHandle != mEventHandle)
      return;
      
   clear();
   
   const uint cMinimumBufSize = 256U * 1024U;
   mBufSize = Math::Max<uint>(cMinimumBufSize, Utils::AlignUpValue(bufSize, 4096U));
   
   mpLowLevelFileIO = pLowLevelFileIO ? pLowLevelFileIO : ILowLevelFileIO::getDefault();
   
   mThreadIndex = threadIndex;
               
   mPublicKeys.resize(0);
   mPublicKeys.pushBack(pPublicKeys, numPublicKeys);
   memcpy(mDecryptKey, decryptKey, sizeof(mDecryptKey));
   
   eventReceiverInit(threadIndex);
   
   BEvent handleEvent;
   handleEvent.clear();
   handleEvent.mFromHandle = mEventHandle;
   handleEvent.mToHandle = mEventHandle;
   handleEvent.mEventClass = cEventReadFileEvent;
   gEventDispatcher.registerHandleWithEvent(mOverlappedEvent.getHandle(), handleEvent);
}

//==============================================================================
// BAsyncECFArchiveLoader::deinit
//==============================================================================
void BAsyncECFArchiveLoader::deinit(void)
{
   if (cInvalidEventReceiverHandle == mEventHandle)
      return;
      
   gEventDispatcher.deregisterHandle(mOverlappedEvent.getHandle(), mThreadIndex);      
      
   eventReceiverDeinit();
}

//==============================================================================
// BAsyncECFArchiveLoader::begin
//==============================================================================
bool BAsyncECFArchiveLoader::begin(const char* pFilename, BCallbackInterface* pCallback, uint64 callbackData, BLoadHandle& handle)
{
   handle = cInvalidLoadHandle;
   
   if (cInvalidEventReceiverHandle == mEventHandle)
      return false;
      
   BDEBUG_ASSERT(pFilename && (strlen(pFilename) < MAX_PATH));
   BDEBUG_ASSERT(pCallback);
   
   const uint nonce = RandomUtils::GenerateNonce32();
      
   BRequest* pRequest = new(gRenderHeap) BRequest;
   pRequest->mNonce = nonce;         
   pRequest->mpCallback = pCallback;
   pRequest->mCallbackData = callbackData;
   pRequest->mFilename.set(pFilename);
   
   handle = Utils::CreateUInt64((uint)pRequest, nonce);
    
   if (!gEventDispatcher.send(mEventHandle, mEventHandle, cEventClassLoadBegin, (uint)pRequest))
   {
      heapDelete(pRequest, gRenderHeap);
      return false;
   }
          
   return true;
}

//==============================================================================
// BAsyncECFArchiveLoader::cancel
//==============================================================================
bool BAsyncECFArchiveLoader::cancel(BLoadHandle handle, bool wait)
{
   if (cInvalidLoadHandle == handle)
      return false;

   if (cInvalidEventReceiverHandle == mEventHandle)
      return false;
         
   if (!gEventDispatcher.send(mEventHandle, mEventHandle, cEventClassLoadCancel, (uint)handle, (uint)(handle >> 32U)))
      return false;
   
   if (wait)
      gEventDispatcher.waitUntilThreadQueueEmpty(mThreadIndex);
   
   return true;
}

//==============================================================================
// BAsyncECFArchiveLoader::cancelAll
//==============================================================================
bool BAsyncECFArchiveLoader::cancelAll(bool wait)
{
   if (cInvalidEventReceiverHandle == mEventHandle)
      return false;

   if (!gEventDispatcher.send(mEventHandle, mEventHandle, cEventClassLoadCancelAll))
      return false;

   if (wait)
      gEventDispatcher.waitUntilThreadQueueEmpty(mThreadIndex);

   return true;
}

//==============================================================================
// BAsyncECFArchiveLoader::initBuffers
//==============================================================================
void BAsyncECFArchiveLoader::initBuffers(void)
{
   ASSERT_THREAD(mThreadIndex);
   
   for (uint i = 0; i < cNumBuffers; i++)
   {
      if (!mpBuffers[i])
      {
         mpBuffers[i] = VirtualAlloc(0, mBufSize, MEM_COMMIT | 
#ifdef XBOX         
         MEM_NOZERO | 
#endif         
         MEM_LARGE_PAGES | MEM_TOP_DOWN, PAGE_READWRITE);
         
         if (!mpBuffers[i])
         {
            BFATAL_FAIL("BAsyncECFArchiveLoader::initBuffers: Out of memory");
         }
      }
   }
   
   mCurBufIndex = 0;
}

//==============================================================================
// BAsyncECFArchiveLoader::deinitBuffers
//==============================================================================
void BAsyncECFArchiveLoader::deinitBuffers(void)
{
   ASSERT_THREAD(mThreadIndex);
   
   for (uint i = 0; i < cNumBuffers; i++)
   {
      if (mpBuffers[i])
      {
         const BOOL success = VirtualFree(mpBuffers[i], 0, MEM_RELEASE);
         success;
         BDEBUG_ASSERT(success);
         
         mpBuffers[i] = NULL;
      }
   }
}

//==============================================================================
// BAsyncECFArchiveLoader::deleteRequest
//==============================================================================
void BAsyncECFArchiveLoader::deleteRequest(BRequest* pRequest)
{
   ASSERT_THREAD(mThreadIndex);
   BDEBUG_ASSERT(pRequest);
   
   pRequest->mpCallback->message(pRequest->mCallbackData, cCMTDeletingRequest);

   Utils::ClearObj(*pRequest);
   heapDelete(pRequest, gRenderHeap);
}

//==============================================================================
// BAsyncECFArchiveLoader::deleteActiveRequest
//==============================================================================
void BAsyncECFArchiveLoader::deleteActiveRequest(void)
{
   ASSERT_THREAD(mThreadIndex);
   BDEBUG_ASSERT(mpCurRequest);
   
   deleteRequest(mpCurRequest);
   mpCurRequest = NULL;
}

//==============================================================================
// BAsyncECFArchiveLoader::reportMessage
//==============================================================================
void BAsyncECFArchiveLoader::reportMessage(BRequest* pRequest, eCallbackMessageType messageType)
{
   ASSERT_THREAD(mThreadIndex);
   BDEBUG_ASSERT(pRequest);
            
   pRequest->mpCallback->message(pRequest->mCallbackData, messageType);
}

//==============================================================================
// BAsyncECFArchiveLoader::openArchive
//==============================================================================
bool BAsyncECFArchiveLoader::openArchive(DWORD& nextState)
{
   ASSERT_THREAD(mThreadIndex);
         
   if (mRequestQueue.isEmpty())
   {
      deinitBuffers();
      
      nextState = cStateOpenArchive;
      return true;
   }
   
   if (mIsPaused)
   {
      nextState = cStateOpenArchive;
      return true;
   }
   
   mpCurRequest = mRequestQueue[0];
   BDEBUG_ASSERT(mpCurRequest);
   mRequestQueue.erase(0);
   
   reportMessage(mpCurRequest, cCMTLoadBegun);
      
   BDEBUG_ASSERT(INVALID_HANDLE_VALUE == mArchiveFileHandle);
   
   mArchiveFileHandle = mpLowLevelFileIO->createFileA(mpCurRequest->mFilename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED | FILE_FLAG_NO_BUFFERING, NULL);
   
   if (INVALID_HANDLE_VALUE == mArchiveFileHandle)
   {
      reportMessage(mpCurRequest, cCMTErrorOpenFailed);

      deleteActiveRequest();
      
      nextState = cStateOpenArchive;
      return false;
   }

   const BOOL success = mpLowLevelFileIO->getFileSizeEx(mArchiveFileHandle, (PLARGE_INTEGER)&mArchiveFileSize);
   if ((!success) || (mArchiveFileSize < 4096) || (mArchiveFileSize & 4095U))
   {
      mpLowLevelFileIO->closeHandle(mArchiveFileHandle);
         
      mArchiveFileHandle = INVALID_HANDLE_VALUE;

      reportMessage(mpCurRequest, cCMTErrorBadArchive);

      deleteActiveRequest();
      
      nextState = cStateOpenArchive;
      return false;
   }

   initBuffers();

   mArchiveFileOfs = 0;

   mOverlappedEvent.reset();
   Utils::ClearObj(mOverlapped);
   mOverlapped.hEvent = mOverlappedEvent.getHandle();
   mCurReadSize = 0;
   mReadIsPending = false;
   
   mCurChunkAllocation.free();
   mCurChunkTigerHashGen.clear();
   mCurChunkIndex = -1;
   mCurChunkOfs = 0;
   mChunkInfo.resize(0);
      
   nextState = cStateInitiateRead;
   return false;
}

//==============================================================================
// BAsyncECFArchiveLoader::initiateRead
//==============================================================================
bool BAsyncECFArchiveLoader::initiateRead(DWORD& nextState)
{
   ASSERT_THREAD(mThreadIndex);
   
   if (mIsPaused)
   {
      nextState = cStateInitiateRead;
      return true;
   }
               
   const uint64 bytesRemaining = mArchiveFileSize - mArchiveFileOfs;
   BDEBUG_ASSERT(0 == (bytesRemaining & 4095U));
   
   if (!bytesRemaining)
   {
      nextState = cStateFinishedArchive;
      return false;
   }
      
   mCurReadSize = (uint)Math::Min<uint64>(mBufSize, bytesRemaining);
   
   mOverlapped.Offset = (uint)mArchiveFileOfs;
   mOverlapped.OffsetHigh = (uint)(mArchiveFileOfs >> 32U);
   
   mReadIsPending = false;
   
   DWORD numBytesRead = 0;
   BOOL success;
   
   {
      //SCOPEDSAMPLE(ReadFile);
   
      success = mpLowLevelFileIO->readFile(mArchiveFileHandle, mpBuffers[mCurBufIndex], mCurReadSize, &numBytesRead, &mOverlapped);
   }
   
   if (success)
   {
      if (numBytesRead != mCurReadSize)
      {
         reportMessage(mpCurRequest, cCMTErrorReadFailed);
         
         nextState = cStateCloseArchive;
         return false;
      }

      nextState = cStateCompleteRead;
      return false;
   }
   
   const DWORD lastError = GetLastError();
   if (lastError == ERROR_IO_PENDING)
   {
      mReadIsPending = true;
      
      nextState = cStateWaitingForRead;
      return true;
   }
   
   reportMessage(mpCurRequest, cCMTErrorReadFailed);
   
   nextState = cStateCloseArchive;
   return false;
}

//==============================================================================
// BAsyncECFArchiveLoader::waitingForRead
//==============================================================================
bool BAsyncECFArchiveLoader::waitingForRead(DWORD& nextState)
{
   ASSERT_THREAD(mThreadIndex);
         
   if (!mpLowLevelFileIO->hasOverlappedIoCompleted(&mOverlapped))
   {
      nextState = cStateWaitingForRead;
      return true;
   }
   
   mReadIsPending = false;
   
   DWORD bytesRead = 0;
   BOOL success = mpLowLevelFileIO->getOverlappedResult(mArchiveFileHandle, &mOverlapped, &bytesRead, FALSE);
   
   if ((!success) || (bytesRead != mCurReadSize))
   {
      reportMessage(mpCurRequest, cCMTErrorReadFailed);
      
      nextState = cStateCloseArchive;
      return false;
   }
         
   nextState = cStateCompleteRead;
   return false;
}

//==============================================================================
// BAsyncECFArchiveLoader::decryptData
//==============================================================================
void BAsyncECFArchiveLoader::decryptData(const BYTE* pSrc, BYTE* pDst, uint size, uint64 bufFileOfs)
{
   BDEBUG_ASSERT((size & (cTeaCryptBlockSize - 1)) == 0);
   BDEBUG_ASSERT((bufFileOfs & (cTeaCryptBlockSize - 1)) == 0);
      
   uint numBlocks = size >> cTeaCryptBlockSizeLog2;
   uint curCounter = (uint)(bufFileOfs >> cTeaCryptBlockSizeLog2);
   uint curBufOfs = 0;

   for (uint i = 0; i < numBlocks; i++, curBufOfs += cTeaCryptBlockSize)
   {
      teaDecryptBlock64(cDefaultTeaCryptIV, 
         mDecryptKey[0], mDecryptKey[1], mDecryptKey[2], 
         pSrc + curBufOfs, pDst + curBufOfs, curCounter + i);
   }
}

//==============================================================================
// BAsyncECFArchiveLoader::processHeaderData
//==============================================================================
bool BAsyncECFArchiveLoader::processHeaderData(uint64 bufFileOfs, uint bufIndex, uint bufSize, uint& totalHeaderBytes)
{
   SCOPEDSAMPLE(BAsyncECFArchiveLoader_processHeaderData)
   bufFileOfs;
   
   ASSERT_THREAD(mThreadIndex);
   
   BYTE* pBuf = static_cast<BYTE*>(mpBuffers[bufIndex]);
   
   totalHeaderBytes = 0;
   
//-- FIXING PREFIX BUG ID 8046
   const BECFArchiveHeader& header = *reinterpret_cast<BECFArchiveHeader*>(pBuf);
//--
   
   if ((bufSize < sizeof(BECFArchiveHeader)) || 
       (cArchiveECFHeaderID != header.getID()) ||
       (BECFArchiveHeader::cMagic != header.mArchiveHeaderMagic)
      )
   {
      reportMessage(mpCurRequest, cCMTErrorBadArchive);
      return false;
   }
   
   if (!BECFUtils::checkHeader(BConstDataBuffer(pBuf, bufSize), true, true))
   {
      reportMessage(mpCurRequest, cCMTErrorBadArchive);
      return false;
   }
   
   totalHeaderBytes = header.getSize() + header.getNumChunks() * sizeof(BECFArchiveChunkHeader);
         
   if (
       (header.getSize() < sizeof(BECFArchiveHeader)) ||
       (totalHeaderBytes > bufSize) ||
       (header.getFileSize() != mArchiveFileSize) ||
       (header.getChunkExtraDataSize() != (sizeof(BECFArchiveChunkHeader) - sizeof(BECFChunkHeader))) ||
       (header.getNumChunks() < 2) ||
       (header.mSignatureSize < sizeof(DWORD) * 2)
      )
   {
      reportMessage(mpCurRequest, cCMTErrorBadArchive);
      return false;
   }
         
//-- FIXING PREFIX BUG ID 8047
   const BECFArchiveChunkHeader* pChunkHeaders = reinterpret_cast<BECFArchiveChunkHeader*>(pBuf + header.getSize());
//--
                     
   BSHA1Gen sha1Gen;
   sha1Gen.update32(0xA7F95F9C);
   sha1Gen.update32(header.getSize());
   sha1Gen.update32(header.getNumChunks());
   sha1Gen.update32(header.getChunkExtraDataSize());
   sha1Gen.update32(header.getFileSize());
   sha1Gen.update(pChunkHeaders, header.getNumChunks() * sizeof(BECFArchiveChunkHeader));
   BSHA1 archiveDigest(sha1Gen.finalize());
   
   BByteStream byteStream(pBuf + sizeof(BECFArchiveHeader), header.getSize() - sizeof(BECFArchiveHeader));
   
   uint publicKeyIndex;
   for (publicKeyIndex = 0; publicKeyIndex < mPublicKeys.getSize(); publicKeyIndex++)
   {
      //SCOPEDSAMPLE(VerifyMessage);      
      
      byteStream.seek(0);
      
      BDigitalSignature digitalSignature;
      if (digitalSignature.verifyMessage(byteStream, mPublicKeys[publicKeyIndex], archiveDigest))
         break;
   }
   
   if (publicKeyIndex == mPublicKeys.getSize())
   {
      reportMessage(mpCurRequest, cCMTErrorBadSignature);
      return false;
   }
                  
   mChunkInfo.resize(header.getNumChunks());
   
   for (uint i = 0; i < mChunkInfo.getSize(); i++)
   {
      uint64 ofs = pChunkHeaders[i].getOfs();
      uint size = pChunkHeaders[i].getSize();
      
      if ((ofs & 3) || (ofs < totalHeaderBytes) || ((ofs + size) > mArchiveFileSize))
      {
         reportMessage(mpCurRequest, cCMTErrorBadArchive);
         return false;
      }
      
      if (i)
      {
         if (ofs < (pChunkHeaders[i - 1].getOfs() + pChunkHeaders[i - 1].getSize()))
         {
            reportMessage(mpCurRequest, cCMTErrorBadArchive);
            return false;
         }
      }
                      
      mChunkInfo[i].mOfs = ofs;
      mChunkInfo[i].mSize = size;
      memcpy(mChunkInfo[i].mCompTiger128, pChunkHeaders[i].mCompTiger128, sizeof(mChunkInfo[i].mCompTiger128));
   }
   
   if (!mpCurRequest->mpCallback->headerData(mpCurRequest->mCallbackData, pBuf, totalHeaderBytes))
   {
      reportMessage(mpCurRequest, cCMTErrorHeaderDataCallbackFailed);
      return false;  
   }
         
   return true;
}

//==============================================================================
// BAsyncECFArchiveLoader::processChunkData
//==============================================================================
bool BAsyncECFArchiveLoader::processChunkData(uint64 bufFileOfs, uint bufIndex, uint bufSize, uint initialBufOfs)
{
   ASSERT_THREAD(mThreadIndex);
   BDEBUG_ASSERT(bufSize >= initialBufOfs);
   
   bufFileOfs += initialBufOfs;
   
//-- FIXING PREFIX BUG ID 8048
   const BYTE* pBuf = static_cast<const BYTE*>(mpBuffers[bufIndex]) + initialBufOfs;
//--
   uint bytesRemaining = bufSize - initialBufOfs;
   
   while ((bytesRemaining) && (mCurChunkIndex < (int)mChunkInfo.getSize()))
   {
      const uint curChunkSize = mChunkInfo[mCurChunkIndex].mSize;
      const uint64 curChunkFileOfs = mChunkInfo[mCurChunkIndex].mOfs;
            
      if (curChunkFileOfs > bufFileOfs)
      {  
         const uint bytesToSkip = (uint)Math::Min<uint64>(curChunkFileOfs - bufFileOfs, bytesRemaining);
         
         bufFileOfs     += bytesToSkip;
         pBuf           += bytesToSkip;
         bytesRemaining -= bytesToSkip;
         
         if (!bytesRemaining)
            break;
      }            
            
      if (!mCurChunkOfs)
      {
         //SCOPEDSAMPLE(ChunkAlloc);
         
         if (curChunkFileOfs != bufFileOfs)
         {
            reportMessage(mpCurRequest, cCMTErrorBadArchive);
            return false;
         }
         
         mCurChunkAllocation.free();
         
         if (!mpCurRequest->mpCallback->chunkAlloc(mpCurRequest->mCallbackData, mCurChunkIndex, mCurChunkAllocation, curChunkSize))
         {
            reportMessage(mpCurRequest, cCMTErrorChunkAllocFailed);
            return false;
         }
         
         if ((!mCurChunkAllocation.getPtr()) || (mCurChunkAllocation.getSize() < curChunkSize))
         {
            reportMessage(mpCurRequest, cCMTErrorChunkAllocFailed);
            return false;
         }
         
         mCurChunkTigerHashGen.clear();
      }
      
      const uint curChunkRemaining = curChunkSize - mCurChunkOfs;      
      const uint bytesToCopy = Math::Min(curChunkRemaining, bytesRemaining);

      {
         //SCOPEDSAMPLE(HashUpdate);      
         mCurChunkTigerHashGen.update(pBuf, bytesToCopy);
      }
      
      BDEBUG_ASSERT(bufFileOfs == (mCurChunkOfs + curChunkFileOfs));
      
      {
        //SCOPEDSAMPLE(MemCpy);      
         Utils::FastMemCpy(static_cast<BYTE*>(mCurChunkAllocation.getPtr()) + mCurChunkOfs, pBuf, bytesToCopy);
      }
      
      bufFileOfs     += bytesToCopy;
      pBuf           += bytesToCopy;
      bytesRemaining -= bytesToCopy;
            
      mCurChunkOfs   += bytesToCopy;                  
      
      if (mCurChunkOfs == curChunkSize)
      {
         BTigerHash tigerHash(mCurChunkTigerHashGen.finalize());
         for (uint i = 0; i < 16; i++)
         {
            if (tigerHash[i] != mChunkInfo[mCurChunkIndex].mCompTiger128[i])
            {
               reportMessage(mpCurRequest, cCMTErrorBadChunk);
               return false;
            }
         }
                  
         const bool success = mpCurRequest->mpCallback->chunkReady(mpCurRequest->mCallbackData, mCurChunkIndex, mCurChunkAllocation);
         
         mCurChunkAllocation.clear();
         
         if (!success)
         {
            reportMessage(mpCurRequest, cCMTErrorChunkReadyCallbackFailed);
            return false;
         }
                        
         mCurChunkOfs = 0;
         mCurChunkIndex++;  
      }
   }
   
   return true;
}

//==============================================================================
// BAsyncECFArchiveLoader::processBuffer
//==============================================================================
bool BAsyncECFArchiveLoader::processBuffer(uint64 bufFileOfs, uint bufIndex, uint bufSize)
{
   ASSERT_THREAD(mThreadIndex);
   BDEBUG_ASSERT((bufSize & (cTeaCryptBlockSize - 1U)) == 0);
   
   SCOPEDSAMPLE(BAsyncECFArchiveLoader_processBuffer);
   
   BYTE* pBuf = static_cast<BYTE*>(mpBuffers[bufIndex]);

   {
      //SCOPEDSAMPLE(Decrypt);
      decryptData(pBuf, pBuf, bufSize, bufFileOfs);
   }
   
   uint bufOfs = 0;
   
   if (mCurChunkIndex < 0)
   {
      if (!processHeaderData(bufFileOfs, bufIndex, bufSize, bufOfs))
         return false;
         
      mCurChunkIndex = 0;
   }
      
   return processChunkData(bufFileOfs, bufIndex, bufSize, bufOfs);
}    

//==============================================================================
// BAsyncECFArchiveLoader::completeRead
//==============================================================================
bool BAsyncECFArchiveLoader::completeRead(DWORD& nextState)
{
   SCOPEDSAMPLE(BAsyncECFArchiveLoader_completeRead)
   ASSERT_THREAD(mThreadIndex);
         
   const uint64 bufOfs = mArchiveFileOfs;
   const uint bufIndex = mCurBufIndex;
   const uint bufSize = mCurReadSize;
         
   mArchiveFileOfs += mCurReadSize;
   mCurReadSize = 0;
   
   mCurBufIndex ^= 1;
   
   bool doneFlag = initiateRead(nextState);
   if (cStateCloseArchive == nextState)
      return doneFlag;
      
   if (!processBuffer(bufOfs, bufIndex, bufSize))
   {
      nextState = cStateCloseArchive;
      doneFlag = false;
   }
   
   return doneFlag;
}

//==============================================================================
// BAsyncECFArchiveLoader::finishedArchive
//==============================================================================
bool BAsyncECFArchiveLoader::finishedArchive(DWORD& nextState)
{
   ASSERT_THREAD(mThreadIndex);
         
   if (mCurChunkIndex != (int)mChunkInfo.size())
      reportMessage(mpCurRequest, cCMTErrorBadArchive);
   else   
      reportMessage(mpCurRequest, cCMTLoadFinished);
      
   nextState = cStateCloseArchive;
   return false;  
}

//==============================================================================
// BAsyncECFArchiveLoader::closeArchive
//==============================================================================
bool BAsyncECFArchiveLoader::closeArchive(DWORD& nextState)
{
   ASSERT_THREAD(mThreadIndex);
   
   if (!mpCurRequest)
   {
      nextState = cStateOpenArchive;
      return false;
   }
      
   if (mReadIsPending)
   {
      mOverlappedEvent.wait();
      mReadIsPending = false;
   }
   
   BOOL success = mpLowLevelFileIO->closeHandle(mArchiveFileHandle);
   success;
   BDEBUG_ASSERT(success);
   mArchiveFileHandle = INVALID_HANDLE_VALUE;
   
   mArchiveFileSize = 0;
   mArchiveFileOfs = 0;
   mCurBufIndex = 0;
   mOverlappedEvent.reset();
   mCurReadSize = 0;
   
   mCurChunkAllocation.free();
   mCurChunkTigerHashGen.clear();
   mCurChunkIndex = -1;
   mCurChunkOfs = 0;
   
   mChunkInfo.clear();
         
   deleteActiveRequest();
         
   nextState = cStateOpenArchive;
   return false;
}

//==============================================================================
// BAsyncECFArchiveLoader::tickState
//==============================================================================
void BAsyncECFArchiveLoader::tickState(void)
{
   ASSERT_THREAD(mThreadIndex);
   SCOPEDSAMPLE(BAsyncECFArchiveLoader_tickState); 
                  
   bool doneFlag = true;
   
   do
   {
      DWORD nextState = mCurState;
      
      switch (mCurState)  
      {
         case cStateOpenArchive:
         {
            doneFlag = openArchive(nextState);
            break;
         }
         case cStateInitiateRead:
         {
            doneFlag = initiateRead(nextState);
            break;
         }
         case cStateWaitingForRead:
         {
            doneFlag = waitingForRead(nextState);
            break;
         }
         case cStateCompleteRead:
         {
            doneFlag = completeRead(nextState);
            break;
         }
         case cStateFinishedArchive:
         {
            doneFlag = finishedArchive(nextState);
            break;
         }
         case cStateCloseArchive:
         {
            doneFlag = closeArchive(nextState);
            break;
         }
      }
      
      mCurState = nextState;
   
   } while (!doneFlag);
}

//==============================================================================
// BAsyncECFArchiveLoader::processLoadBegin
//==============================================================================
void BAsyncECFArchiveLoader::processLoadBegin(const BEvent& event)
{
   ASSERT_THREAD(mThreadIndex);
   
   BRequest* pRequest = (BRequest*)event.mPrivateData;
   
   mRequestQueue.pushBack(pRequest);
   
   tickState();
}

//==============================================================================
// BAsyncECFArchiveLoader::processReadFileEvent
//==============================================================================
void BAsyncECFArchiveLoader::processReadFileEvent(const BEvent& event)
{
   ASSERT_THREAD(mThreadIndex);
   event;
         
   tickState();
}

//==============================================================================
// BAsyncECFArchiveLoader::cancelRequest
//==============================================================================
void BAsyncECFArchiveLoader::cancelRequest(uint index)
{
   BRequest* pRequest = mRequestQueue[index];
   
   pRequest->mpCallback->message(pRequest->mCallbackData, cCMTErrorLoadCanceled);

   deleteRequest(pRequest);

   mRequestQueue.erase(index);
}

//==============================================================================
// BAsyncECFArchiveLoader::cancelCurrentRequest
//==============================================================================
void BAsyncECFArchiveLoader::cancelCurrentRequest(void)
{
   if (!mpCurRequest)
      return;
   
   reportMessage(mpCurRequest, cCMTErrorLoadCanceled);

   DWORD nextState;
   closeArchive(nextState);
   
   mCurState = nextState;
}

//==============================================================================
// BAsyncECFArchiveLoader::processLoadCancel
//==============================================================================
void BAsyncECFArchiveLoader::processLoadCancel(const BEvent& event)
{
   ASSERT_THREAD(mThreadIndex);
   
   BRequest* pRequest = (BRequest*)event.mPrivateData;
      
   if (pRequest == mpCurRequest)
   {
      cancelCurrentRequest();
      
      tickState();
                  
      return;
   }

   const int index = mRequestQueue.find(pRequest);
   if (index == -1)
   {
      trace("BAsyncECFArchiveLoader::processLoadCancel: Ignoring invalid handle");
      return;
   }
   
   if (pRequest->mNonce != event.mPrivateData2)
   {
      trace("BAsyncECFArchiveLoader::processLoadCancel: Ignoring invalid handle");
      return;
   }
      
   cancelRequest(index);
}

//==============================================================================
// BAsyncECFArchiveLoader::processClientAdded
//==============================================================================
void BAsyncECFArchiveLoader::processClientAdded(void)
{
   ASSERT_THREAD(mThreadIndex);
}

//==============================================================================
// BAsyncECFArchiveLoader::processClientRemoved
//==============================================================================
void BAsyncECFArchiveLoader::cancelAllRequests(void)
{
   ASSERT_THREAD(mThreadIndex);
   
   for (int i = mRequestQueue.getSize() - 1; i >= 0; i--)
      cancelRequest(i);
   
   BDEBUG_ASSERT(mRequestQueue.isEmpty());

   if (mpCurRequest)
      cancelCurrentRequest();
   
   BDEBUG_ASSERT(!mReadIsPending);
   BDEBUG_ASSERT(!mpCurRequest);              
   BDEBUG_ASSERT(INVALID_HANDLE_VALUE == mArchiveFileHandle);
}   

//==============================================================================
// BAsyncECFArchiveLoader::pause
//==============================================================================
bool BAsyncECFArchiveLoader::pause(bool wait)
{
   if (cInvalidEventReceiverHandle == mEventHandle)
      return false;

   if (!gEventDispatcher.send(mEventHandle, mEventHandle, cEventClassPause))
      return false;

   if (wait)
      gEventDispatcher.waitUntilThreadQueueEmpty(mThreadIndex);

   return true;
}

//==============================================================================
// BAsyncECFArchiveLoader::resume
//==============================================================================
bool BAsyncECFArchiveLoader::resume(void)
{
   if (cInvalidEventReceiverHandle == mEventHandle)
      return false;

   if (!gEventDispatcher.send(mEventHandle, mEventHandle, cEventClassResume))
      return false;
   
   return true;
}

//==============================================================================
// BAsyncECFArchiveLoader::processCancelAll
//==============================================================================
void BAsyncECFArchiveLoader::processCancelAll(void)
{
   ASSERT_THREAD(mThreadIndex);
   
   cancelAllRequests();
}

//==============================================================================
// BAsyncECFArchiveLoader::processPause
//==============================================================================
void BAsyncECFArchiveLoader::processPause(void)
{
   ASSERT_THREAD(mThreadIndex);
   
   BDEBUG_ASSERT(!mIsPaused);
   mIsPaused = true;
   
   //trace("BAsyncECFArchiveLoader::processPause: %u", GetTickCount());
}

//==============================================================================
// BAsyncECFArchiveLoader::processResume
//==============================================================================
void BAsyncECFArchiveLoader::processResume(void)
{
   ASSERT_THREAD(mThreadIndex);
   
   BDEBUG_ASSERT(mIsPaused);
   mIsPaused = false;
   
   //trace("BAsyncECFArchiveLoader::processResume: %u", GetTickCount());
   
   tickState();
}

//==============================================================================
// BAsyncECFArchiveLoader::processClientRemoved
//==============================================================================
void BAsyncECFArchiveLoader::processClientRemove(bool terminating)
{
   ASSERT_THREAD(mThreadIndex);
   terminating;
   
   cancelAllRequests();
   
   mCurChunkAllocation.free();
   deinitBuffers();
   mChunkInfo.clear();
}

//==============================================================================
// BAsyncECFArchiveLoader::receiveEvent
//==============================================================================
bool BAsyncECFArchiveLoader::receiveEvent(const BEvent& event, BThreadIndex threadIndex)
{
   ASSERT_THREAD(mThreadIndex);
   
   threadIndex;
   
   switch (event.mEventClass)
   {
      case cEventClassClientAdded:
      {
         processClientAdded();
         break;
      }
      case cEventClassClientRemove:
      {
         processClientRemove(false);
         break;
      }
      case cEventClassThreadIsTerminating:
      {
         processClientRemove(true);
         break;
      }
      case cEventClassLoadBegin:
      {
         processLoadBegin(event);
         break;
      }
      case cEventClassLoadCancel:
      {
         processLoadCancel(event);
         break;
      }
      case cEventClassLoadCancelAll:
      {
         processCancelAll();
         break;
      }
      case cEventReadFileEvent:
      {
         processReadFileEvent(event);
         break;
      }
      case cEventClassPause:
      {
         processPause();
         break;
      }
      case cEventClassResume:
      {
         processResume();
         break;
      }
   }

   return true;
}









