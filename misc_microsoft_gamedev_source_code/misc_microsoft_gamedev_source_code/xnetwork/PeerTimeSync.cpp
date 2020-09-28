//==============================================================================
// PeerTimeSync.cpp
//
// Copyright (c) 2000, Ensemble Studios
//==============================================================================

// Includes
#include "precompiled.h"
#include "PeerTimeSync.h"
#include "Session.h"
#include "Client.h"
#include "NetPackets.h"

//==============================================================================
// Defines
//==============================================================================

//==============================================================================
class BPeerClientDisconnectPacket : public BChannelPacket
{
   public:      
      // send ctor
      BPeerClientDisconnectPacket(BTimingHistory *history, long clientID, DWORD lastSendTime) : BChannelPacket(BChannelPacketType::cPeerClientDisconnectPacket),
         mTimingHistory(history), mClientID(clientID), mLastSendTime(lastSendTime) {}      
      // recv ctor
      BPeerClientDisconnectPacket(BTimingHistory *history) : BChannelPacket(BChannelPacketType::cPeerClientDisconnectPacket),
         mTimingHistory(history) {}      
      
      virtual ~BPeerClientDisconnectPacket(void) {}  
   
      virtual void            serialize(BSerialBuffer &sb) 
      { 
         BChannelPacket::serialize(sb);  
         
         if (mTimingHistory)
            mTimingHistory->serialize(sb);
         sb.add(mClientID); 
         sb.add(mLastSendTime); 
      }
      virtual void            deserialize(BSerialBuffer &sb) 
      { 
         BChannelPacket::deserialize(sb);          

         if (mTimingHistory)
            mTimingHistory->deserialize(sb);
         sb.get(&mClientID); 
         sb.get(&mLastSendTime); 
      }            

   public:
      BTimingHistory          *mTimingHistory;
      long                    mClientID;
      DWORD                   mLastSendTime;
};

 

//==============================================================================
// BPeerTimeSync::BPeerTimeSync
//==============================================================================
BPeerTimeSync::BPeerTimeSync(BSession *session, BTimeSyncStrategy* strategy) :   
   BTimeSync(session, strategy),   
   mLastSentTime(0)
{
   for (long i=0;i<BSession::cMaxClients;i++)
      mLastClientTime[i] = 0;      

   getSession()->addObserver(this);
} // BPeerTimeSync::BPeerTimeSync

//==============================================================================
// BPeerTimeSync::~BPeerTimeSync
//==============================================================================
BPeerTimeSync::~BPeerTimeSync(void)
{
} // BPeerTimeSync::~BPeerTimeSync

//==============================================================================
// 
HRESULT BPeerTimeSync::dispose(void)
{
   getSession()->removeObserver(this);
   return BTimeSync::dispose();
}

//==============================================================================
// BPeerTimeSync::sendTimeMarker
//==============================================================================
void BPeerTimeSync::sendTimeMarker(DWORD timeMarker, DWORD lastTimeMarker)
{
   if (timeMarker > lastTimeMarker)
   {
      // we have to send out a full time sync
      if ((timeMarker - lastTimeMarker) >= 256)
      {
         nlog(cTimeSyncNL, "Sending out new timeMarker %ld, lastTimeMarker %ld, localTiming %ld", timeMarker, lastTimeMarker, getLocalTiming());
         BTimeSyncPacket packet(BChannelPacketType::cTimeSyncPacket, timeMarker, getLocalTiming());
         //ProfileStartEventTime(EVENT_SENDTIMEMARKER_SENDPACKET);
         mChannel->SendPacket(packet);
         //ProfileStopEventTime(EVENT_SENDTIMEMARKER_SENDPACKET);         
      }
      else // we can send out just an interval
      {
         nlog(cTimeSyncNL, "Sending out interval %ld for timeMarker %ld, lastTimeMarker %ld, localTiming %ld", timeMarker-lastTimeMarker, timeMarker, lastTimeMarker, getLocalTiming());
         BTimeIntervalPacket packet((unsigned char)(timeMarker - lastTimeMarker), getLocalTiming());
         //ProfileStartEventTime(EVENT_SENDTIMEMARKER_SENDPACKET);
         mChannel->SendPacket(packet);
         //ProfileStopEventTime(EVENT_SENDTIMEMARKER_SENDPACKET);         
      }
   }
}

//==============================================================================
// BPeerTimeSync::jumpToTime
//==============================================================================
void BPeerTimeSync::jumpToTime(DWORD time)
{
   setEarliestAllowedRecvTime(calculateEarliestTime());     

   BTimeSync::jumpToTime(time);
}

//==============================================================================
// BPeerTimeSync::getInitialHostRecvTime
//==============================================================================
DWORD BPeerTimeSync::getInitialHostRecvTime(void)
{  
   return 0;         
}

//==============================================================================
// BPeerTimeSync::getInitialClientRecvTime
//==============================================================================
DWORD BPeerTimeSync::getInitialClientRecvTime(void)
{   
   nlog(cSessionNL, "BPeerTimeSync::getInitialClientRecvTime");
   DWORD earliest = 0;   
   // check all clients except for us
   for (long i=0;i<getSession()->getClientCount();i++)
   {
      nlog(cSessionNL, "  client %ld", i);
      /*bool unsyncedClient = false;
      for (long j=0;j<mUnsyncedClients.getNumber();j++)
      {
         if (mUnsyncedClients[j] == i) unsyncedClient = true;
         nlog(cSessionNL, "  mUnsyncedClients[%ld] = %ld, i = %ld", j, mUnsyncedClients[j], i);
      }

      if (unsyncedClient)
      {
         nlog(cSessionNL, "  unsyncedClient == true, continuing");
         continue;
      }*/

      if ( 
            (getSession()->getClient(i) != getSession()->getLocalClient()) &&                 
            (getSession()->getClient(i)) && 
            (getSession()->getClient(i)->isConnected())
         )
      {
         nlog(cSessionNL, "  mLastClientTime[%ld] = %ld", i, mLastClientTime[i]);

         if (!earliest)
            earliest = mLastClientTime[i];
         else if (mLastClientTime[i] < earliest)
            earliest = mLastClientTime[i];
      }
   }

   nlog(cSessionNL, "  earliest-(earliest mod BTimeSyncStrategy::cConstantUpdateInterval) %ld", earliest-(earliest%BTimeSyncStrategy::cConstantUpdateInterval));

   // truncate to nearest update interval  
   return earliest-(earliest%BTimeSyncStrategy::cConstantUpdateInterval);
}

//==============================================================================
// BPeerTimeSync::service
//==============================================================================
void BPeerTimeSync::service(void)
{   
   BTimeSync::service();

   if (!isTimeRolling())
   {
      if (getSession()->isConnected() && getSession()->isHosted())
      {
         jumpToTime(getInitialHostRecvTime());
         mLastClientTime[getSession()->getHostID()] = getSendTime();
         nlog(cTimeSyncNL, "Host initial time info: getRecvTime() %ld", getRecvTime());
         getSession()->clientTimeSynced(getSession()->getHostID()); // FIXME: Sync time joining
         setTimeRolling(true);
      }
      else
         return; // we're waiting on hearing our first times
   }
   else
   {
      //ProfileStartEventTime(EVENT_CALCULATEEARLIESTTIME);
      setEarliestAllowedRecvTime(calculateEarliestTime());
      //ProfileStopEventTime(EVENT_CALCULATEEARLIESTTIME);      
   }
}

//==============================================================================
// BPeerTimeSync::calculateEarliestTime
//==============================================================================
long BPeerTimeSync::calculateEarliestTime(BClient *excludeClient)
{    
   bool connecting = false;
   DWORD earliest = 0;        
   long earliestClient=-1;
   // check all clients
   for (long i=0;i<getSession()->getClientCount();i++)
   {
      if (           
            (getSession()->getClient(i)) &&             
            (getSession()->getClient(i)->isConnected()) &&  
            (
               !excludeClient ||
               (excludeClient && (excludeClient != getSession()->getClient(i)))
            )
         )
      {
         if (mLastClientTime[i] == 0) // means that someone is connected and hasn't told us their time yet
         {
            //return getEarliestAllowedRecvTime();
            connecting = true;
            continue;
         }

         nlog(cTimeSyncNL, "mLastClientTime[%ld] = %ld", i, mLastClientTime[i]);        

         if (!earliest)
         {
            earliest = mLastClientTime[i];
            earliestClient = i;            
         }
         else if (mLastClientTime[i] < earliest)         
         {
            earliest = mLastClientTime[i];                     
            earliestClient = i;            
         }
      }
   }   

   if (earliestClient!=-1)
   {
      BClient *client = getSession()->getClient(earliestClient);

      nlog(cTimeSyncNL, "earliestClient %ld, earliest %ld, disconnectCount %ld, getConnectedClientAmount() %ld, getDisconnectingClientAmount() %ld", earliestClient, earliest, client->getDisconnectCount(), getConnectedClientAmount(), getDisconnectingClientAmount());

      // is client all disconnected and we're blocked waiting for him?      
      if (
            client && 
            client->getDisconnectCount() &&
            (client->getDisconnectCount() >= getConnectedClientAmount()-getDisconnectingClientAmount()) &&
            !canAdvanceTime()
         )
      {  
         nlog(cSessionNL, "  timeSyncClientLeft %ld", getSession()->getClientID(client));
         nlog(cSessionNL, "    client->getDisconnectCount() %ld, getConnectedClientAmount() %ld, getDisconnectingClientAmount() %ld", client->getDisconnectCount(), getConnectedClientAmount(), getDisconnectingClientAmount());         
         // if so, we're ready to drop him
         getSession()->timeSyncClientLeft(getSession()->getClientID(client));         
      }         
   }

   // advance time 
   nlog(cTimeSyncNL, "Earliest time calculation %ld", earliest);   

   if (connecting)
      return getEarliestAllowedRecvTime();
   else
      return earliest;      
}

//==============================================================================
// BPeerTimeSync::timingDataReceived
//==============================================================================
void BPeerTimeSync::timingDataReceived(const long clientIndex, const void *data, const DWORD size)
{
   // receive sendtime marker from client   
   if (BChannelPacket::getType(data) == BChannelPacketType::cTimeIntervalPacket)
   {
      BTimeIntervalPacket packet;
      packet.deserializeFrom(data, size);

      // add interval to client's time
      BASSERT(mLastClientTime[clientIndex]); // we should have heard an initial time update from this guy by now!
      if (!mLastClientTime[clientIndex])
      {
         nlog(cTimeSyncNL, "ERROR: Adding an interval without an initial time - fromClient %ld!", clientIndex);                  
      }
      mLastClientTime[clientIndex] += packet.mInterval;      
      nlog(cTimeSyncNL, "mLastClientTime[%ld]=%ld (interval %ld)", clientIndex, mLastClientTime[clientIndex], packet.mInterval);
   
      setEarliestAllowedRecvTime(calculateEarliestTime());      
      timingReceived(clientIndex, (long)packet.mTiming);      
   }   
   else if (BChannelPacket::getType(data) == BChannelPacketType::cTimeSyncInitPacket)
   {     
      BTimeSyncPacket packet(BChannelPacketType::cTimeSyncInitPacket);
      packet.deserializeFrom(data, size);
      
      // set client time     
      //BASSERT(mLastClientTime[clientIndex] <= packet.mTime);
      if (getSession()->isHosted() && (mLastClientTime[clientIndex] == 0))      
         // this is a newly joined client, so notify the session that he's all synced up
         getSession()->clientTimeSynced(clientIndex); // FIXME: Sync time joining

      mLastClientTime[clientIndex] = packet.mTime;      
      nlog(cSessionNL, "Got client %ld init time %ld", clientIndex, mLastClientTime[clientIndex]);     
      
      /*for (long i=0;i<mUnsyncedClients.getNumber();i++)
      {
         if (mUnsyncedClients[i] == clientIndex)
         {
            mUnsyncedClients.remove(mUnsyncedClients[i]);
            break;
         }
      }*/

      checkForTimeStart();

      timingReceived(clientIndex, (long)packet.mTiming);
   }   
   else if (BChannelPacket::getType(data) == BChannelPacketType::cTimeSyncPacket)
   {     
      BTimeSyncPacket packet(BChannelPacketType::cTimeSyncPacket);
      packet.deserializeFrom(data, size);

      if (isTimeRolling())
      {      
         // set client time     
         //BASSERT(mLastClientTime[clientIndex] <= packet.mTime);
         if (getSession()->isHosted() && (mLastClientTime[clientIndex] == 0))      
            // this is a newly joined client, so notify the session that he's all synced up
            getSession()->clientTimeSynced(clientIndex); // FIXME: Sync time joining

         mLastClientTime[clientIndex] = packet.mTime;         
         nlog(cTimeSyncNL, "Got client %ld time %ld", clientIndex, mLastClientTime[clientIndex]);      

         timingReceived(clientIndex, (long)packet.mTiming);
      }            
   }   
   else if (BChannelPacket::getType(data) == BChannelPacketType::cPeerClientDisconnectPacket)
   {
      BTimingHistory history;
      BPeerClientDisconnectPacket packet(&history);
      packet.deserializeFrom(data, size);     
      
      BClient *client = getSession()->getClient(packet.mClientID);

      nlog(cSessionNL, "got peerClientDisconnectPacket from client %ld, client index %ld, mLastClientTime %ld, packet.mLastSendTime %ld", clientIndex, packet.mClientID, mLastClientTime[packet.mClientID], packet.mLastSendTime);

      // bring me up to date with latest client 
      if (client)
      {         
         for (long i=0;i<history.getTimingAmount();i++)
            if (history.getTiming(i))
               client->getTimingHistory()->updateTiming(history.getTiming(i)->mTiming, history.getTiming(i)->mSendTime);

         client->getTimingHistory()->setMostRecentTiming(history.getMostRecentTiming());

         // if we haven't heard a time from this guy, then he isn't all connected yet
         // So, just drop him out immediately
         if (mLastClientTime[packet.mClientID] == 0)
         {
            nlog(cSessionNL, "  we haven't heard a time from this guy, so we're just gonna drop him");
            getSession()->timeSyncClientLeft(packet.mClientID);
         }
         else
         {
            if (packet.mLastSendTime > mLastClientTime[packet.mClientID])
               mLastClientTime[packet.mClientID] = packet.mLastSendTime;         

            if(!client->isLocal())
               client->incDisconnectCount();         

            nlog(cSessionNL, "  new disconnectCount %ld", client->getDisconnectCount());
                 
            setEarliestAllowedRecvTime(calculateEarliestTime());
         }
      }       
   }
}

//==============================================================================
//
void BPeerTimeSync::checkForTimeStart(void)
{
   nlog(cSessionNL, "BPeerTimeSync::checkForTimeStart");

   if (!isTimeRolling() && getSession()->isConnected())
   {
      // Check if we have heard times for all clients in the session
      // if so, jump us forward in time
      // and send out our time
               
      for (long j=0;j<getSession()->getClientCount();j++)
      {
         if (getSession()->getClient(j))
         {
            nlog(cSessionNL, "  client %ld, local client connected? %ld, other client connected? %ld, local client? %ld, lastClientTime %ld",
               j,
               getSession()->getLocalClient()?getSession()->getLocalClient()->isConnected():0,            
               getSession()->getClient(j)?getSession()->getClient(j)->isConnected():0,
               (getSession()->getClient(j) != getSession()->getLocalClient()),
               mLastClientTime[j]);
         }

         if (  
               getSession()->getLocalClient() &&
               getSession()->getLocalClient()->isConnected() && // we're connected, and
               getSession()->getClient(j) &&  // is client valid, and
               getSession()->getClient(j)->isConnected() && // client connected, and 
               (getSession()->getClient(j) != getSession()->getLocalClient()) && // client isn't us, and
               !mLastClientTime[j] // client hasn't told us his time yet?          
            )
            return; // if so, drop out. We haven't heard from all clients yet
      }
      setTimeRolling(true);
      mLastClientTime[getSession()->getLocalClientID()] = getInitialClientRecvTime();
      // loop through all unsynced clients and update them also
      //updateUnsyncedClients();
      // calculate fresh values and assign them         
      setEarliestAllowedRecvTime(getInitialClientRecvTime());
      jumpToTime(getInitialClientRecvTime());  
      nlog(cSessionNL, "Client initial time info: getRecvTime() %ld, getInitialClientRecvTime() %ld", getRecvTime(), getInitialClientRecvTime());
   }
   else
      setEarliestAllowedRecvTime(calculateEarliestTime());
}

//==============================================================================
// BPeerTimeSync::isWaitingFor
//==============================================================================
bool BPeerTimeSync::isWaitingFor(long clientIndex)
{
   if (mLastClientTime[clientIndex] == getEarliestAllowedRecvTime())
      return true;
   else
      return false;
}

//==============================================================================
// BPeerTimeSync::timingReceived
//==============================================================================
void BPeerTimeSync::timingReceived(long clientID, long timing)
{
   
   // if the session is still open, ignore timing   
   if (getSession()->getClient(clientID))
      getSession()->getClient(clientID)->getTimingHistory()->addTiming(timing, getClientSendTime(clientID));   
}

//==============================================================================
// BPeerTimeSync::getTiming
//==============================================================================
unsigned char BPeerTimeSync::getRecentTiming(DWORD forRecvTime)
{
   // loop through all clients and get recent timing to
   // figure out a new update system wide timing   
   long timing = 0;
   for (long i=0;i<getSession()->getClientCount();i++)
   {     
      if (getSession()->getClient(i))
      {
         getSession()->getClient(i)->getTimingHistory()->timeAdvanced(forRecvTime);
         if (getSession()->getClient(i)->getTimingHistory()->getRecentTiming() > timing)
            timing = getSession()->getClient(i)->getTimingHistory()->getRecentTiming();            
      }
   }  

   unsigned char t;
   t = (unsigned char)min(timing, 255);

   return t;
}

//==============================================================================
// BPeerTimeSync::getDisconnectingClientAmount
//==============================================================================
long BPeerTimeSync::getDisconnectingClientAmount(void)
{
   if (!getSession())
      return 0;

   long a = 0;

   for (long i=0;i<getSession()->getClientCount();i++)
   {
      if (getSession()->getClient(i) && (getSession()->getClient(i)->getDisconnectCount() > 0))
         a++;
   }

   return a;
}

//==============================================================================
// BPeerTimeSync::getConnectedClientAmount
//==============================================================================
long BPeerTimeSync::getConnectedClientAmount(void)
{
   if (!getSession())
      return 0;

   long a = 0;

   for (long i=0;i<getSession()->getClientCount();i++)
   {
      if (  getSession()->getClient(i) && 
            getSession()->getClient(i)->isConnected() &&
            (getSession()->getClient(i)->getDisconnectCount() <= 0) &&
            (mLastClientTime[i] > 0))
         a++;
   }

   return a;
}

//==============================================================================
// 
/*void BPeerTimeSync::updateUnsyncedClients(void)
{
   nlog(cSessionNL, "BPeerTimeSync::updateUnsyncedClients");
   nlog(cSessionNL, "  mUnsyncedClients.getNumber() %ld", mUnsyncedClients.getNumber());
   for (long i=0;i<mUnsyncedClients.getNumber();i++)
      mLastClientTime[mUnsyncedClients[i]] = mLastClientTime[getSession()->getLocalClientID()];      
   
   mUnsyncedClients.clear();
}*/

//==============================================================================
// 
void BPeerTimeSync::clientConnected(const long clientIndex)
{   
   BClient *client = getSession()->getClient(clientIndex);
   mLastClientTime[clientIndex] = 0;      

   if (!isTimeRolling()) 
   {
    /*  if (client != getSession()->getLocalClient())
      {
         nlog(cSessionNL, "adding mUnsyncedClients %ld, getNumber %ld", clientIndex, mUnsyncedClients.getNumber());
         mUnsyncedClients.add(clientIndex);
      }*/
      return;       
   }
   
   if (client != getSession()->getLocalClient())
   {     
      BTimeSyncPacket packet(BChannelPacketType::cTimeSyncInitPacket, getSendTime(), getLocalTiming());            
      mChannel->SendPacketTo(client, packet);
      nlog(cSessionNL, "Sending out new mSendTime %ld to new client %ld", getSendTime(), clientIndex);
   }   
} 



//==============================================================================
// 
void BPeerTimeSync::linkDisconnected(const long clientIndex)
{
   BClient *client = getSession()->getClient(clientIndex);   

   nlog(cSessionNL, "linkDisconnected index %ld", clientIndex);

   // tell everyone that this guy is disconnected from me
   BPeerClientDisconnectPacket packet(client->getTimingHistory(), clientIndex, mLastClientTime[clientIndex]);
   mChannel->SendPacket(packet);   
}

//==============================================================================
// BPeerTimeSync::
//==============================================================================



//==============================================================================
// eof: PeerTimeSync.cpp
//==============================================================================
