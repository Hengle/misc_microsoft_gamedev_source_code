//============================================================================
//  File: ContentWriter.h
//
//  Copyright (c) 2007, Ensemble Studios
//============================================================================
#pragma once

#ifdef XBOX

#include "buffercache.h"
#include "threading\eventDispatcher.h"

class BContentFile;

//==============================================================================
// 
//==============================================================================
class BContentWriterPayload : public BEventPayload
{
   public:
      BContentWriterPayload(BBufferCache* pCache) : mpCache(pCache) { }

      BBufferCache* mpCache;

   private:
      virtual void deleteThis(bool delivered)
      {
         delivered;
         if (mpCache)
            delete mpCache;
         mpCache = NULL;
         delete this;
      }
};

//==============================================================================
// 
//==============================================================================
class BContentWriterPayloadReply : public BEventPayload
{
   public:
      BContentWriterPayloadReply(BBufferCache* pCache) : mpCache(pCache) { }

      BBufferCache* mpCache;

   private:
      virtual void deleteThis(bool delivered)
      {
         delivered;
         if (mpCache)
            delete mpCache;
         mpCache = NULL;
         delete this;
      }
};

//==============================================================================
// class BWriterASyncNotify
//==============================================================================
class BWriterASyncNotify
{
   public:
      virtual void notifyDone(DWORD eventID, void * task) = 0;
};

//============================================================================
// class BContentWriter
// This class is intended to accelerate lots of small sequential writes to a 
// content file.
//============================================================================
class BContentWriter : public BEventReceiverInterface
{
   public:
      enum { cDefaultCacheSize = 8192, };
     
      enum 
      {
         cStateOpen,
         cStateIOPending,
         cStateWaitForDelete,
      };

      BContentWriter(BContentFile* pFile);
      ~BContentWriter();

      bool flush();
      bool close();
      BOOL setFilePointer(uint64 filepointer);
      uint writeBytes(const void* p, uint n);

      // Helper methods
      uint writeInt(int data);
      uint writeUint(uint data);
      uint writeBool(bool data);
      bool writeBString(BString& string);

      // stuff to help with the threading
      bool isCacheEmpty() const { return mpCache->isCacheEmpty(); }  // Should be called before destroying the class, flush it returns false
      bool isIOPending() const { return (mIOCount != 0); }  // Can be called after isCacheEmpty returns true;
      bool deinit();                                  // Can be called after isIOPending returns true
      bool canDelete() const { return mCanDelete; }         // Can be called after deinit is called and returns true;

      void setState(uint state) { mState = state; }
      uint getState() const { return mState;}

      // notification methods (not used yet).
      void setNotify(BWriterASyncNotify* pNotify) { mpNotify = pNotify; }

      BContentFile* getFile() { return mpContentFile; }

   protected:

      bool flushBuffer();
      uint32 writeToFile(const void *p, uint n, uint64 filepointer);

      enum
      {
         cContentWriterEventWrite = cEventClassFirstUser,
         cContentWriterEventWriteReply,
         cContentWriterEventRemoved,
      };

      // BEventReceiverInterface - to receive events off other threads
      BEventReceiverHandle    mEventHandleIO;
      BEventReceiverHandle    mEventHandleSim;
      LONG mIOCount;
      bool mCanDelete;
      virtual bool receiveEvent(const BEvent& event, BThreadIndex threadIndex);


      int64          mFileOfs;
      BContentFile   *mpContentFile;
      bool           mGoAsync;
      uint           mState;

      // Cache 
      BBufferCache*   mpCache;
      BWriterASyncNotify *mpNotify;

}; // class BContentWriter

#endif // XBOX