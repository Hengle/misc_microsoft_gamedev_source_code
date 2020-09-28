//==============================================================================
// mpSimDataObject.h
// This is a near copy of mpSimObject from multiplayer
//  Done so that it doesn't break that object's interfaces
//
// Copyright (c) Ensemble Studios, 2006-2008
//==============================================================================

#pragma once

//==============================================================================
// Includes
#include "Channel.h"
#include "mptypes.h"
#include "mppackets.h"

//==============================================================================
// Forward declarations

class BMPGameSession;
class BMPTimingHandler;

//==============================================================================
// Const declarations

//==============================================================================
class BMPSimDataObject
{
public:
   class BMPSimObserver
   {
   public:
      virtual void commandReceived(const void* pData, DWORD size) { pData; size; }
   };

   class BMPPauseHandler
   {
   public:
      virtual void netSetPaused(bool val, PlayerID lPlayerID) = 0;
      virtual void netSingleStep() = 0;
   };

   class BMPTimingHandler
   {
   public:
      virtual HRESULT getLocalTiming(uint32& timing, uint32* deviationRemaining) = 0;
      virtual float getMSPerFrame(void) = 0;
   };

   class BMPPlayerHandler
   {
      public:
         virtual void playerDisconnected(PlayerID playerID, bool userInitiated) = 0;
   };

   BMPSimDataObject();
   BMPSimDataObject(BMPGameSession* pSessionObject);

   void init(BMPGameSession* pSessionObject);

   void addObserver(BMPSimObserver* pObserver) { mObserverList.Add(pObserver); }
   void removeObserver(BMPSimObserver* pObserver) { mObserverList.Remove(pObserver); }
   void setPauseHandler(BMPPauseHandler* pHandler) { mpPauseHandler = pHandler; }
   void setTimingHandler(BMPTimingHandler* pHandler) { mpTimingHandler = pHandler; }        
   void setPlayerHandler(BMPPlayerHandler* pHandler) { mpPlayerHandler = pHandler; }

   void sendCommand(BChannelPacket& commandPacket);
   void setPaused(bool v, PlayerID playerID);
   void singleStep();
   DWORD advanceGameTime();
   long getActiveClientCount() const;
   void getTimingCounters(DWORD& compensationAmount, DWORD& compensationInterval, DWORD& sendUpdateInterval, 
      DWORD& actualSendUpdateInterval, DWORD& pingApprox, DWORD& networkStall, DWORD& totalSendInterval, 
      BYTE& sessionRecentTiming, BYTE& localRecentTiming);

   // called from BMPGameSession
   void commandDataReceived(BMachineID machineID, const void* pData, DWORD size);
   void simDataReceived(const void* pData, DWORD size);
   void playerDisconnected(PlayerID playerID, bool userInitiated);
   BMPTimingHandler* getTimingHandler() const { return mpTimingHandler; }

   // debugging
   BMPGameSession* getMPGame() const { return mpMPSession; }

protected:
   class BMPSimObserverList : public BObserverList<BMPSimObserver>
   {
      DECLARE_OBSERVER_METHOD( commandReceived, (const void* pData, DWORD size), (pData, size) )
   };

private:
   BMPSimObserverList mObserverList;
   BMPGameSession*    mpMPSession;
   BMPPauseHandler*   mpPauseHandler;
   BMPTimingHandler*  mpTimingHandler; // the game implements this to provide timing info back to this class
   BMPPlayerHandler*  mpPlayerHandler;
};
