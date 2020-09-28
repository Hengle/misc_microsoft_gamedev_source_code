//==============================================================================
// PeerTimeSync.h
//
// Copyright (c) 2000, Ensemble Studios
//==============================================================================

#ifndef _PeerTimeSync_H_
#define _PeerTimeSync_H_

//==============================================================================
// Includes

#include "Session.h"
#include "TimeSync.h"

//==============================================================================
// Forward declarations

class BClient;
 
//==============================================================================
// Const declarations

//==============================================================================
class BPeerTimeSync : public BTimeSync, public BSession::BSessionObserver
{
   public:      
      // Constructors
      BPeerTimeSync( BSession *session, BTimeSyncStrategy* strategy );

      // Destructors
      virtual HRESULT dispose(void);      

      // Functions           
      virtual void               service(void);

      DWORD                      getClientSendTime(long clientID)    { return mLastClientTime[clientID]; }
      virtual DWORD              getServerSendTime(void) { return 0; }      

      virtual void               clientConnected(const long clientIndex);            
      virtual void               sessionConnected(void) { checkForTimeStart(); }

      virtual void               linkDisconnected(const long clientIndex);
      
      virtual void               jumpToTime(DWORD time);       

      virtual bool               isWaitingFor(long clientIndex);

      virtual void               timingDataReceived(const long clientIndex, const void *data, const DWORD size);
      
      // Variables

   protected:                           
      virtual ~BPeerTimeSync( void );

      // returns the system wide average timing interval for a given receive time
      virtual unsigned char      getRecentTiming(DWORD forRecvTime);            
      // send out the send time marker
      virtual void               sendTimeMarker(DWORD timeMarker, DWORD lastTimeMarker);                  
      //virtual BTimeSyncChannel   *getChannel(void) { return mChannel; }     

   private:
      // Functions      
      void                       timingReceived(long clientID, long timing);
      DWORD                      getInitialHostRecvTime(void);      
      long                       getDisconnectingClientAmount(void);
      long                       getConnectedClientAmount(void);
      DWORD                      getInitialClientRecvTime(void);      
      long                       calculateEarliestTime(BClient *excludeClient=0);        
      //void                       updateUnsyncedClients(void);
      void                       checkForTimeStart(void);
      
      // Variables                 
             
      DWORD                      mLastClientTime[BSession::cMaxClients];      
      DWORD                      mLastSentTime;          
      BSimpleArray <long>        mUnsyncedClients;
      //BTimeSyncChannel           *mChannel;
}; // BTimeSync

//==============================================================================
#endif // _PeerTimeSync_H_

//==============================================================================
// eof: PeerTimeSync.h
//==============================================================================
