//==============================================================================
// MPSimObject.h
//
// Copyright (c) 2003, Ensemble Studios
//==============================================================================

#ifndef _MPSIMOBJECT_H_
#define _MPSIMOBJECT_H_

//==============================================================================
// Includes
#include "Channel.h"
#include "mptypes.h"
#include "mppackets.h"

//==============================================================================
// Forward declarations

class BMPGame;

//==============================================================================
// Const declarations

//==============================================================================
class BMPSimObject
{
   public:
      class BMPSimObserver
      {
         public:
            virtual void commandReceived(PlayerID fromPlayerID, const void *data, DWORD size) { fromPlayerID; data; size; }
      };

      class BMPPauseHandler
      {
         public:
            virtual void setPaused(bool val, bool mpset, PlayerID lPlayerID) = 0;
      };

      class BMPTimingHandler
      {
         public:
            virtual HRESULT getLocalTiming(unsigned char *timing, DWORD *deviationRemaining) = 0;
            virtual float getMSPerFrame(void) = 0;
      };
      
      class BMPPlayerHandler
      {
         public:
            virtual void   playerDisconnected(PlayerID playerID) = 0;
      };

      class BMPGCHandler
      {
         public:
            virtual void gcDataReceived(PlayerID fromPlayerID, const void *data, DWORD size) = 0;
      };

      BMPSimObject(BMPGame *game);      

      void addObserver(BMPSimObserver *o) { mObserverList.Add(o); }
      void removeObserver(BMPSimObserver *o) { mObserverList.Remove(o); }
      void setPauseHandler(BMPPauseHandler *handler) { mPauseHandler = handler; }
      void setTimingHandler(BMPTimingHandler *handler) { mTimingHandler = handler; }        
      void setPlayerHandler(BMPPlayerHandler *handler) { mPlayerHandler = handler; }
      void setGCHandler(BMPGCHandler* handler) { mGCHandler = handler; }

      void sendCommand(BChannelPacket &commandPacket);
      void sendGCData(BChannelPacket &gcPacket);
      void setPaused(bool v);
      DWORD advanceGameTime(void);
      long getActiveClientCount(void) const;
      void getTimingCounters(DWORD &compensationAmount, DWORD &compensationInterval, DWORD &sendUpdateInterval, 
                             DWORD &actualSendUpdateInterval, DWORD &pingApprox, DWORD &networkStall, DWORD &totalSendInterval, 
                             BYTE &sessionRecentTiming, BYTE &localRecentTiming);

      // called from MPGame
      void commandDataReceived(ClientID fromClientID, const void *data, DWORD size);
      void simDataReceived(ClientID fromClientID, const void *data, DWORD size);
      void gcDataReceived(ClientID fromClientID, const void *data, DWORD size);
      void playerDisconnected(PlayerID playerID);
      BMPTimingHandler *getTimingHandler(void) { return mTimingHandler; }
      
      // debugging
      BMPGame* getMPGame(void) { return mMPGame; }

   protected:
      class BMPSimObjectObserverList : public BObserverList< BMPSimObserver >
      {
         DECLARE_OBSERVER_METHOD( commandReceived, (long fromPlayerID, const void *data, DWORD size), (fromPlayerID, data, size) )
      };
   
   private:
      BMPSimObjectObserverList mObserverList;
      BMPGame *mMPGame;
      BMPPauseHandler *mPauseHandler;
      BMPTimingHandler *mTimingHandler; // the game implements this to provide timing info back to this class
      BMPPlayerHandler *mPlayerHandler;
      BMPGCHandler *mGCHandler;
};

//==============================================================================
#endif // _MPSIMOBJECT_H_

//==============================================================================
// eof: mpsimobject.h
//==============================================================================