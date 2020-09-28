//==============================================================================
// FileTransferMgr.h
//
// Copyright (c) 2000-2007, Ensemble Studios
//==============================================================================
/*
#ifndef __FILETRANSFERMGR_H__
#define __FILETRANSFERMGR_H__

//==============================================================================
// Includes

#include "Session.h"

//==============================================================================
// Forward declarations
class BFileStream;
class BFileTransferMgr;

//==============================================================================
class BFileTransferGameInterface
{
   public:
      BFileTransferGameInterface() : mFileTransferMgr(NULL) {}
      virtual ~BFileTransferGameInterface();

      void           attachFileTransferMgr(BFileTransferMgr* mgr) { mFileTransferMgr=mgr; }

      virtual void   clientDisconnected(DWORD index)=0;
      virtual void   transferStartFileRecv(DWORD clientID, long streamID, long &dirID, BSimString &filename )=0;
      virtual void   transferBatchStart(const BCHAR_T* filename, DWORD clientID)=0;
      virtual void   transferBatchStop(const BCHAR_T* filename, DWORD clientID)=0;
      virtual void   transferStarted(DWORD clientID, long streamID)=0;
      virtual void   transferUpdate(DWORD clientID, long streamID, float percent)=0;
      virtual void   transferComplete(DWORD clientID, long streamID, long result)=0;
      virtual void   transferCRCResult(const BCHAR_T* filename, DWORD clientID, DWORD crc)=0;
      virtual void   transferRequest(const BCHAR_T* filename, DWORD clientID, DWORD crc)=0;
      virtual bool   getFileCRCInfo(const BCHAR_T* filename, long dirID, DWORD& crc) const=0;
      virtual bool   getFileInfo(const BCHAR_T* filename, long& dirID, long& type, long& size, DWORD& crc)=0;

   protected:
      BFileTransferMgr *mFileTransferMgr;
};

//==============================================================================
class BFileTransferPlayerInterface
{
public:
   virtual DWORD getClientIDFromPlayerID(long playerID) = 0;
   virtual long  getPlayerIDFromClientID(DWORD clientID) = 0;
};

//==============================================================================
class BFileTransferMgr : public BSession::BSessionEventObserver
{                      
   public:
      enum
      {
         cResultComplete,
         cResultAbort,
         cResultTimeout,
         cResultMax
      };
         
      BFileTransferMgr(BSession* session, BFileTransferPlayerInterface* playerInterface);
      virtual ~BFileTransferMgr();

      void                       attachGameInterface(BFileTransferGameInterface* iface) { mGameInterface=iface; }
      void                       removeGameInterface(BFileTransferGameInterface* iface) { iface; BASSERT(mGameInterface==iface); mGameInterface=NULL; }

      BFileTransferGameInterface *getGameInterface() { return mGameInterface; }

      DWORD                      getClientID(long playerID);
      long                       getPlayerID(DWORD clientID);

      void                       service(void);

      // request a file from the host
      bool                       requestFile(DWORD clientID, const BCHAR_T* filename, DWORD localCRC, long& streamID);
      bool                       requestFile(const BCHAR_T* filename, DWORD localCRC);

      bool                       startBatch(const BCHAR_T *name, DWORD clientID);
      bool                       stopBatch(const BCHAR_T *name, DWORD clientID);
      bool                       sendFile(const BCHAR_T *filename, DWORD clientID);
      bool                       requestFileCRC(const BCHAR_T* filename, DWORD clientID);

      virtual void               processSessionEvent(const BSessionEvent* pEvent);
      
   protected:
      bool                       requestFile(BClient *client, const BCHAR_T *filename, DWORD localCRC);

      void                       removeTransfer(DWORD clientID, long streamID, long result);
      bool                       handleClientData(DWORD clientIndex, const void *data, DWORD size);

      virtual void               clientDisconnected(DWORD index);

      BFileStream                *getStream(DWORD clientID, long streamID);

      BFileTransferPlayerInterface  *mPlayerInterface;
      BFileTransferGameInterface    *mGameInterface;
      BSession                      *mSession;
      BDynamicSimArray<BFileStream*>    mStreams[BSession::cMaxClients];
};

//==============================================================================
#endif // __FILETRANSFERMGR_H__

*/