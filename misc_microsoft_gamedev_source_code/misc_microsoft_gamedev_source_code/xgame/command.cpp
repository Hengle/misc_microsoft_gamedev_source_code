//==============================================================================
// command.cpp
//
// Copyright (c) 1997-2006 Ensemble Studios
//==============================================================================

// Includes
#include "common.h"
#include "command.h"

// xgame
#include "commandmanager.h"
#include "ConfigsGame.h"
#include "EntityGrouper.h"
#include "objectmanager.h"
#include "syncmacros.h"
#include "unit.h"
#include "squad.h"
#include "army.h"
#include "world.h"
#include "scenario.h"
#include "simhelper.h"

// xnetwork
#include "serialbuffer.h"

// Defines
#ifndef BUILD_FINAL
   //#define DEBUGSERIALIZE
   //#define DEBUGCHECKSUM
#endif

//==============================================================================
// BCommand::BCommand
//==============================================================================
BCommand::BCommand(long type, long numberFlags) :
   BChannelPacket(type+BChannelPacketType::cCommandPacketsStart),
   mPlayerID(-1),
   mID(-1),
   mSenderType(-1),
   //mSenders doesn't need any ctor args.
   mRecipientType(-1),
   //mRecipients doesn't need any ctor args.   
   //mWaypoints doesn't need any ctor args.
   //mReferenceCount(0),
   //mQueueReferenceCount(0),
   mExecuteTime((DWORD)0),
   //mFromPlayerID(-1),
   mUrgencyCount(0),
   mSentTime(0),
   mUrgencyTimeThreshhold(0),
   mUrgencyCountThreshhold(-1),
   mSendTimedUrgent(false)
{
   BASSERT(type <= 255);
   mType = static_cast<unsigned char>(type);
   mFlags.setNumber(numberFlags);
   mFlags.clear();
}

//==============================================================================
// BCommand::~BCommand
//==============================================================================
BCommand::~BCommand(void)
{
}

//==============================================================================
// BCommand::setSenders
//==============================================================================
bool BCommand::setSenders(const long numberSenders, const long*  senders)
{
   if ((numberSenders <= 0) || (senders == NULL))
      return(false);
   if (mSenders.setNumber(numberSenders) == false)
      return(false);
   memcpy((long*)mSenders.getPtr(), senders, sizeof(long)*numberSenders);
   return(true);
}

//==============================================================================
// BCommand::setRecipients
//==============================================================================
bool BCommand::setRecipients(const long numberRecipients, const BEntityID*  recipients)
{
   //Simple bomb checks.
   if ((numberRecipients <= 0) || (recipients == NULL) || (mRecipientType < 0))
      return(false);

   //If we have units, we process them differently as we have to count the number
   //of valid commandable units before we add them to the recipient list.
   if (mRecipientType == cUnit)
   {
      //Bail if we don't have a world.
      if (gWorld == NULL)
         return(false);

      //Run through the desired recipients and filter out the non-commandable ones.
      mRecipients.setNumber(0);
      for (long i=0; i < numberRecipients; i++)
      {
         BUnit*  pUnit=gWorld->getUnit(recipients[i]);
         if (pUnit == NULL)
            continue;
         if (mRecipients.add(pUnit->getID()) < 0)
         {
            mRecipients.setNumber(0);
            return(false);
         }
      }

      //If we have no recipients, bail.
      if (mRecipients.getNumber() <= 0)
         return(false);
      return(true);
   }

   //Deal with the other types more simply.
   if (mRecipients.setNumber(numberRecipients) == false)
      return(false);
   memcpy((BEntityID*)mRecipients.getPtr(), recipients, sizeof(BEntityID)*numberRecipients);
   return(true);
}

//==============================================================================
// BCommand::getName
//==============================================================================
const char* BCommand::getName(void) const
{
   //fixme
   return "";
   //return(gDatabase.getGameCommandName(mType));
}

//==============================================================================
// BCommand::getWaypoint
//==============================================================================
const BVector& BCommand::getWaypoint(long index) const
{
   if ((index < 0) || (index >= mWaypoints.getNumber()) )
      return(cInvalidVector);
   return(mWaypoints[index]);
}

//==============================================================================
// BCommand::setWaypoints
//==============================================================================
bool BCommand::setWaypoints(const BVector* waypoints, const long numberWaypoints)
{
   if ((numberWaypoints <= 0) || (waypoints == NULL))
      return(false);
   if (mWaypoints.setNumber(numberWaypoints) == false)
      return(false);
   memcpy((BVector*)mWaypoints.getPtr(), waypoints, sizeof(BVector)*numberWaypoints);
   return(true);
}

//==============================================================================
// BCommand::addWaypoint
//==============================================================================
bool BCommand::addWaypoint(const BVector &waypoint)
{
   if (mWaypoints.add(waypoint) == -1)
      return(false);
   return(true);
}

//==============================================================================
// BCommand::getTargetPosition
//==============================================================================
BVector BCommand::getTargetPosition(void) const
{
   if (mWaypoints.getNumber() <= 0)
      return(cInvalidVector);
   return(mWaypoints[0]);
}

//==============================================================================
// BCommand::execute
//==============================================================================
bool BCommand::execute(void)
{
   switch(mRecipientType)
   {
      case BCommand::cUnit       : return processUnits();
      case BCommand::cSquad      : return processSquads(); 
      case BCommand::cArmy       : return processArmies(); 
      case BCommand::cPlayer     : return processPlayer();
      case BCommand::cGame       : return processGame();
   }
   return(false);
}

//==============================================================================
// BCommand::serialize
//==============================================================================
enum
{
   cCmdCompress            = 1,
   cCmdIDByte              = 2,
   cCmdIDLong              = 4,
   cCmdOneSenderByte       = 8,
   cCmdNonUnitRecipient    = 16,
   cCmdOneWaypoint         = 32,
   cCmdMultiWaypoints      = 64,
   cCmdWaypointY           = 128,
   cCmdExtraFlagData       = 256,
   cCmdUrgencyCount        = 512,
   cCmdCachedUnitSet       = 1024,
};

static BSerialBuffer workSB;

void BCommand::serialize(BSerialBuffer &sb)
{
   // Set flags that determine how to serialize the data to keep it small.
   WORD flags=0;

   if(mID>=0 && mID<256)
      flags+=cCmdIDByte;
   else if(mID!=-1)
      flags+=cCmdIDLong;

   if(mSenderType==cPlayer && mSenders.getNumber()==1 && (mSenders[0]==-1 || (mSenders[0]>=0 && mSenders[0]<255)))
      flags+=cCmdOneSenderByte;

   BYTE cachedUnitSet=0;
   if(mRecipientType!=cUnit)
      flags+=cCmdNonUnitRecipient;
   else
   {
      if(mRecipients.getNumber()>2)
      {
         long unitSet=gWorld->getCommandManager()->getCommandedUnitSet(mPlayerID, mRecipients);
         if(unitSet!=-1)
         {
            cachedUnitSet=(BYTE)unitSet;
            flags+=cCmdCachedUnitSet;
         }
      }
   }

   if(mWaypoints.getNumber()==1)
      flags+=cCmdOneWaypoint;
   else if(mWaypoints.getNumber()>1)
      flags+=cCmdMultiWaypoints;

   if(mWaypoints.getNumber()>0)
   {
      for(long i=0; i<mWaypoints.getNumber(); i++)
      {
         if(mWaypoints[i].y!=0.0f)
         {
            flags+=cCmdWaypointY;
            break;
         }
         else
         {
            // ajl 9/12/02 - This is here because the above comparison seems to lie.
            mWaypoints[i].y=0.0f;
         }
      }
   }

   for(long i=8; i<mFlags.getNumber(); i++)
   {
      if(mFlags.isBitSet(i))
      {
         flags+=cCmdExtraFlagData;
         break;
      }
   }

   if(mUrgencyCount)
      flags+=cCmdUrgencyCount;

   // Serialize the data into a temp buffer.
   workSB.resetDestination();

   // mPlayerID
   BYTE playerIDByte=(BYTE)(mPlayerID==-1 ? 255 : mPlayerID);
   workSB.add(playerIDByte);

   // mID
   if(flags&cCmdIDByte)
   {
      BYTE idByte=(BYTE)mID;
      workSB.add(idByte);
   }
   else if(flags&cCmdIDLong)
   {
      workSB.add(mID);
   }

   // mSenderType and mSenders
   if(flags&cCmdOneSenderByte)
   {
      BYTE senderByte=(BYTE)(mSenders[0]==-1 ? 255 : mSenders[0]);
      workSB.add(senderByte);
   }
   else
   {
      BYTE senderTypeByte=(BYTE)(mSenderType==-1 ? 255 : mSenderType);
      workSB.add(senderTypeByte);

      BYTE senderCountByte=(BYTE)mSenders.getNumber();
      workSB.add(senderCountByte);
      for (long i=0;i<mSenders.getNumber();i++)
      {
         workSB.add(mSenders[i]);
      }
   }

   // mRecipientType
   if(flags&cCmdNonUnitRecipient)
   {
      BYTE recipientTypeByte=(BYTE)(mRecipientType==-1 ? 255 : mRecipientType);
      workSB.add(recipientTypeByte);
   }

   // mRecipients
   if(flags&cCmdCachedUnitSet)
   {
      workSB.add(cachedUnitSet);
   }
   else
   {
      BYTE recipientCountByte=(BYTE)mRecipients.getNumber();
      workSB.add(recipientCountByte);
      for (long i=0;i<mRecipients.getNumber();i++)
      {
         workSB.add(mRecipients[i].asLong());
      }
   }

   // mWaypoints
   BYTE waypointCountByte=0;
   if(flags&cCmdOneWaypoint)
      waypointCountByte=1;
   else if(flags&cCmdMultiWaypoints)
   {
      waypointCountByte=(BYTE)mWaypoints.getNumber();
      workSB.add(waypointCountByte);
   }
   if(waypointCountByte>0)
   {
      for (long i=0;i<mWaypoints.getNumber();i++)
      {      
         workSB.add(mWaypoints[i].x);
         if(flags&cCmdWaypointY)
         {
            workSB.add(mWaypoints[i].y);
         }
         workSB.add(mWaypoints[i].z);
      }   
   }

   // mFlags
   BYTE flagCountByte=(BYTE)mFlags.getNumber();
   workSB.add(flagCountByte);
   unsigned char *bytes = const_cast<unsigned char*>(mFlags.getBits());
   if(flags&cCmdExtraFlagData)
   {
      long numberOfBytes = (mFlags.getNumber()/8) + (mFlags.getNumber()%8==0?0:1);
      workSB.add(bytes, numberOfBytes);
   }
   else
   {
      workSB.add(*bytes);
   }

   // mUrgencyCount
   if(flags&cCmdUrgencyCount)
   {
      workSB.add(mUrgencyCount);
   }

   // Serialize the data into the real buffer.
   BChannelPacket::serialize(sb);

   sb.add(mType);

   sb.add((short)flags);

   sb.add((unsigned char*)workSB.getBuffer(), workSB.getBufferSize());

   #ifdef DEBUGCHECKSUM
      DWORD baseChecksum=calcBaseChecksum();
      sb.add(baseChecksum);
   #endif

   #ifdef DEBUGSERIALIZE
   {
      sb.add(mType);
      sb.add(mPlayerID);
      sb.add(mID);
      sb.add(mSenderType);
      sb.add(mSenders.getNumber());
      for (long i=0;i<mSenders.getNumber();i++)
      {
         sb.add(mSenders[i]);
      }
      sb.add(mRecipientType);
      sb.add(mRecipients.getNumber());
      for (i=0;i<mRecipients.getNumber();i++)
      {
         sb.add(mRecipients[i]);   
      }
      sb.add(mWaypoints.getNumber());
      for (i=0;i<mWaypoints.getNumber();i++)
      {      
         sb.add(mWaypoints[i].x);
         sb.add(mWaypoints[i].y);
         sb.add(mWaypoints[i].z);
      }   
      sb.add(mFlags.getNumber());
      long numberOfBytes = (mFlags.getNumber()/8) + (mFlags.getNumber()%8==0?0:1);
      unsigned char *bytes = const_cast<unsigned char*>(mFlags.getBits());
      for(i=0; i<numberOfBytes; i++)
      {
         sb.add(&(bytes[i]), 1);
      }
      sb.add(mUrgencyCount);
   }
   #endif
}

//==============================================================================
// BCommand::deserialize
//==============================================================================
void BCommand::deserialize(BSerialBuffer &sb)
{   
   // Deserialize the data.
   BChannelPacket::deserialize(sb);

   sb.get(&mType);

   short tempFlags=0;
   sb.get(&tempFlags);
   WORD flags=(WORD)tempFlags;

   BSerialBuffer* useSB=&sb;

   // mPlayerID
   BYTE playerIDByte=0;
   useSB->get(&playerIDByte);
   mPlayerID=(playerIDByte==255 ? -1 : playerIDByte);

   // mID
   if(flags&cCmdIDByte)
   {
      BYTE idByte=0;
      useSB->get(&idByte);
      mID=idByte;
   }
   else if(flags&cCmdIDLong)
   {
      useSB->get(&mID);
   }
   else
      mID=-1;

   // mSenderType and mSenders
   if(flags&cCmdOneSenderByte)
   {
      BYTE senderByte=0;
      useSB->get(&senderByte);
      mSenderType=cPlayer;
      if(!mSenders.setNumber(1))
      {
         BASSERT(0);
         mSenders.setNumber(0);
      }
      else
         mSenders[0]=(senderByte==255 ? -1 : senderByte);
   }
   else
   {
      BYTE senderTypeByte=0;
      useSB->get(&senderTypeByte);
      mSenderType=(senderTypeByte==255 ? -1 : senderTypeByte);

      BYTE senderCountByte=0;
      useSB->get(&senderCountByte);
      if(!mSenders.setNumber(senderCountByte))
      {
         BASSERT(0);
         mSenders.setNumber(0);
         long tempSender;
         for (long i=0;i<senderCountByte;i++)
         {
            useSB->get(&tempSender);
         }
      }
      else
      {
         for (long i=0;i<mSenders.getNumber();i++)
         {
            useSB->get(&mSenders[i]);
         }
      }
   }

   // mRecipientType
   if(flags&cCmdNonUnitRecipient)
   {
      BYTE recipientTypeByte=0;
      useSB->get(&recipientTypeByte);
      mRecipientType=(recipientTypeByte==255 ? -1 : recipientTypeByte);
   }
   else
      mRecipientType=cUnit;

   // mRecipients
   if(flags&cCmdCachedUnitSet)
   {
      BYTE cachedUnitSet=0;
      useSB->get(&cachedUnitSet);
      gWorld->getCommandManager()->getCommandedUnits(mPlayerID, cachedUnitSet, mRecipients);
   }
   else
   {
      BYTE recipientCountByte=0;
      useSB->get(&recipientCountByte);
      if(!mRecipients.setNumber(recipientCountByte))
      {
         BASSERT(0);
         mRecipients.setNumber(0);
         for(long i=0; i<recipientCountByte; i++)
         {
            long tempRecipient;
            useSB->get(&tempRecipient);
         }
      }
      else
      {
         long temp;
         for (long i=0;i<mRecipients.getNumber();i++)
         {
            useSB->get(&temp);
            mRecipients[i] = BEntityID(temp);
         }
         if(mRecipientType==cUnit && mRecipients.getNumber()>2)
            gWorld->getCommandManager()->cacheCommandedUnits(mPlayerID, mRecipients);
      }
   }

   // mWaypoints
   BYTE waypointCountByte=0;
   if(flags&cCmdOneWaypoint)
      waypointCountByte=1;
   else if(flags&cCmdMultiWaypoints)
   {
      useSB->get(&waypointCountByte);
   }
   if(waypointCountByte>0)
   {
      if(!mWaypoints.setNumber(waypointCountByte))
      {
         BASSERT(0);
         mWaypoints.setNumber(0);
         for(long i=0; i<waypointCountByte; i++)
         {
            float tempVal;
            useSB->get(&tempVal);
            if(flags&cCmdWaypointY)
            {
               useSB->get(&tempVal);
            }
            useSB->get(&tempVal);
         }
      }
      else
      {
         for (long i=0;i<mWaypoints.getNumber();i++)
         {      
            useSB->get(&mWaypoints[i].x);
            if(flags&cCmdWaypointY)
            {
               useSB->get(&mWaypoints[i].y);
            }
            else
               mWaypoints[i].y=0.0f;
            useSB->get(&mWaypoints[i].z);
         }   
      }
   }
   else
      mWaypoints.setNumber(0);

   // mFlags
   BYTE flagCountByte=0;
   useSB->get(&flagCountByte);
   mFlags.setNumber(flagCountByte);
   mFlags.clear();
   unsigned char *bytes = const_cast<unsigned char*>(mFlags.getBits());
   if(flags&cCmdExtraFlagData)
   {
      long numberOfBytes = (mFlags.getNumber()/8) + (mFlags.getNumber()%8==0?0:1);
      useSB->get(&bytes, numberOfBytes);
   }
   else
   {
      useSB->get(&bytes, 1);
   }

   // mUrgencyCount
   if(flags&cCmdUrgencyCount)
   {
      useSB->get(&mUrgencyCount);
   }
   else
      mUrgencyCount=0;

   #ifdef DEBUGCHECKSUM
      DWORD baseChecksum;
      sb.get(&baseChecksum);
      BASSERT(baseChecksum==calcBaseChecksum());
   #endif

   #ifdef DEBUGSERIALIZE
   {
      BYTE checkBYTE;
      sb.get(&checkBYTE);
      BASSERT(mType==checkBYTE);

      long checkLong;
      sb.get(&checkLong);
      BASSERT(mPlayerID==checkLong);

      sb.get(&checkLong);
      BASSERT(mID==checkLong);

      sb.get(&checkLong);
      BASSERT(mSenderType==checkLong);

      sb.get(&checkLong);
      BASSERT(mSenders.getNumber()==checkLong);

      for (long i=0;i<mSenders.getNumber();i++)
      {
         sb.get(&checkLong);
         BASSERT(mSenders[i]==checkLong);
      }

      sb.get(&checkLong);
      BASSERT(mRecipientType==checkLong);

      sb.get(&checkLong);
      BASSERT(mRecipients.getNumber()==checkLong);

      for (i=0;i<mRecipients.getNumber();i++)
      {
         sb.get(&checkLong);
         BASSERT(mRecipients[i]==checkLong);
      }

      sb.get(&checkLong);
      BASSERT(mWaypoints.getNumber()==checkLong);

      float checkFloat;
      for (i=0;i<mWaypoints.getNumber();i++)
      {
         sb.get(&checkFloat);
         BASSERT(mWaypoints[i].x==checkFloat);
 
         sb.get(&checkFloat);
         BASSERT(mWaypoints[i].y==checkFloat);

         sb.get(&checkFloat);
         BASSERT(mWaypoints[i].z==checkFloat);
      } 

      sb.get(&checkLong);
      BASSERT(mFlags.getNumber()==checkLong);

      long numberOfBytes = (mFlags.getNumber()/8) + (mFlags.getNumber()%8==0?0:1);
      BASSERT(numberOfBytes<=4);
      BYTE bytes[4];
      for(i=0; i<numberOfBytes; i++)
      {
         sb.get(&(bytes[i]), 1);
      }
      BASSERT(memcmp(mFlags.getBits(), bytes, numberOfBytes)==0);

      sb.get(&checkBYTE);
      BASSERT(mUrgencyCount==checkBYTE);
   }
   #endif
}

//==============================================================================
// BCommand::processUnits
//==============================================================================
bool BCommand::processUnits(void)
{
   //Bomb check.
   if ((mRecipientType != cUnit) || (gWorld == NULL))
      return(false);

   //Reset our queued reference count.
   //mQueueReferenceCount=0;

   //Go through each unit and call processUnit() on it.
   for (long i=0; i < mRecipients.getNumber(); i++)
   {
      if(!gWorld->validateEntityID(mRecipients[i]))
         continue;

      BUnit* pUnit=gWorld->getUnit(mRecipients[i]);
      if (pUnit == NULL)
         continue;

      if (pUnit->getPlayerID() != mPlayerID && pUnit->getPlayerID() != 0)
      {
         bool allow=false;
         if (gWorld->getFlagCoop())
         {
//-- FIXING PREFIX BUG ID 2026
            const BPlayer* pSendingPlayer = gWorld->getPlayer(mPlayerID);
//--
            const BPlayer* pUnitPlayer = pUnit->getPlayer();
            if (pSendingPlayer && pUnitPlayer)
            {
               if (pSendingPlayer->getTeamID() == pUnitPlayer->getTeamID() && pSendingPlayer->isHuman() && pUnitPlayer->isHuman())
                  allow = true;
            }
         }
         if (!allow)
            continue;
      }

      processUnit(pUnit);
   }

   return(true);
}

//==============================================================================
// BCommand::processSquads
//==============================================================================
bool BCommand::processSquads(void)
{
   //Bomb check.
   if ((mRecipientType != cSquad) || (gWorld == NULL))
      return(false);

   //Reset our queued reference count.
   //mQueueReferenceCount=0;

   //Go through each unit group and call processSquad() on it.
   for (long i=0; i < mRecipients.getNumber(); i++)
   {
      if(!gWorld->validateEntityID(mRecipients[i]))
         continue;

      BSquad* pSquad=gWorld->getSquad(mRecipients[i]);
      if (pSquad == NULL)
         continue;

      if (pSquad->getPlayerID() != mPlayerID && pSquad->getPlayerID() != 0)
         continue;

      processSquad(pSquad);
   }

   return(true);
}

//==============================================================================
// BCommand::processArmies
//==============================================================================
bool BCommand::processArmies(void)
{
   //Bomb check.
   if ((mRecipientType != cArmy) || (gWorld == NULL))
      return(false);

   //Reset our queued reference count.
   //mQueueReferenceCount=0;

   if (mType == cCommandWork && getTargetID().getType() == BEntity::cClassTypeUnit)
   {
      //Remove any recipients that are already capturing if this is a capture command.
//-- FIXING PREFIX BUG ID 2029
      const BUnit* pTargetUnit = gWorld->getUnit(getTargetID());
//--
      if (pTargetUnit && pTargetUnit->getFlagBeingCaptured())
      {
         for (long i=mRecipients.getNumber()-1; i>=0; i--)
         {
//-- FIXING PREFIX BUG ID 2027
            const BSquad* pSquad=gWorld->getSquad(mRecipients[i]);
//--
            if (pSquad)
            {
               for (uint j=0; j<pSquad->getNumberChildren(); j++)
               {
                  if (pTargetUnit->getFirstEntityRefByID(BEntityRef::cTypeCapturingUnit, pSquad->getChild(j)))
                  {
                     mRecipients.removeIndex(i);
                     break;
                  }
               }
            }
         }
      }

      //Remove any recipients that are currently being repaired.
      for (long i=mRecipients.getNumber()-1; i>=0; i--)
      {
//-- FIXING PREFIX BUG ID 2028
         const BSquad* pSquad=gWorld->getSquad(mRecipients[i]);
//--
         if (pSquad && pSquad->getFlagIsRepairing())
            mRecipients.removeIndex(i);
      }
   }

   // See if there's a common army for these squads.
   BEntityID commonArmyID = BEntityGrouper::commonArmyIDForSquads(mRecipients);
   BArmy* pArmy = gWorld->getArmy(commonArmyID);

   bool result = false;

   // If we don't have a common army, create one per player and add the appropriate squads to it.
   if (!pArmy)
   {
      // Group squads per player
      BPlayerSpecificEntityIDsArray playerEntityIDs = BSimHelper::groupEntityIDsByPlayer(mRecipients);
      uint numPlayerEntityIDs = playerEntityIDs.getSize();      
      bool validArmy = false;
      for (uint i = 0; i < numPlayerEntityIDs; i++)
      {
         uint numEntities = playerEntityIDs[i].mEntityIDs.getSize();
         if (numEntities > 0)
         {
            BObjectCreateParms objectParms;
            objectParms.mPlayerID = playerEntityIDs[i].mPlayerID;
            BArmy* pArmy = gWorld->createArmy(objectParms);
            if (pArmy)
            {
               //Create the list of squads to add to the army.
               BEntityIDArray squads;
               for (uint j = 0; j < numEntities; j++)
               {
                  if (!gWorld->validateEntityID(playerEntityIDs[i].mEntityIDs[j]))
                     continue;
//-- FIXING PREFIX BUG ID 2030
                  const BSquad* pSquad = gWorld->getSquad(playerEntityIDs[i].mEntityIDs[j]);
//--
                  if (pSquad == NULL)
                     continue;
                  if ((pSquad->getPlayerID() != mPlayerID) && (pSquad->getPlayerID() != 0))
                     continue;
                  squads.add(pSquad->getID());
               }

               // Platoon the squads
               if (gConfig.isDefined(cConfigClassicPlatoonGrouping))
                  pArmy->addSquads(squads, false);
               else
                  pArmy->addSquads(squads);

               // Test if army is valid
               if (pArmy->getNumberChildren() != 0)
               {
                  validArmy = true;
                  result = processArmy(pArmy);
                  // If fail bail early
                  if (!result)
                  {
                     return (false);
                  }
               }                  
            }               
         }
      }

      // If we had a valid army then we're good
      return (validArmy);
   }
   else
   {
      // If there's already an army, replatoon it so that we merge close squads
      // into the same platoons.  Don't replatoon if this is an alternate
      // (i.e. user waypoint) command as deleting the existing platoons will
      // lose the current waypoints.
      if (!getFlag(BCommand::cFlagAlternate))
         pArmy->checkAndReplatoon();

      //Do the actual job.
      result = processArmy(pArmy);
      return(result);
   }   
}

//==============================================================================
// BCommand::meterRecipientsAndWaypoints
//==============================================================================
bool BCommand::meterRecipientsAndWaypoints(BCommand*  pLastCommand, float distanceCheck)
{
   //gConsole.output(cChannelSim, " ");
   //gConsole.output(cChannelSim, " ");
   //gConsole.output(cChannelSim, " ");
   // check recipient type
   if (pLastCommand->getRecipientType() != mRecipientType)
   {
      //gConsole.output(cChannelSim, "  BC::meterRecipAndWaypts: FALSE1");
      return false;
   }

   // check number of recipients
   if (pLastCommand->getNumberRecipients() != mRecipients.getNumber())
   {
      //gConsole.output(cChannelSim, "  BC::meterRecipAndWaypts: FALSE2");
      return false;
   }

   // check that all recipients match
   for (long i=0;i<mRecipients.getNumber();i++)
   {
      if (pLastCommand->getRecipients()[i] != mRecipients[i])
      {
         //gConsole.output(cChannelSim, "  BC::meterRecipAndWaypts: FALSE3");
         return false;
      }
   }

   // check number of waypoints
   if (pLastCommand->getNumberWaypoints() != mWaypoints.getNumber())
   {
      //gConsole.output(cChannelSim, "  BC::meterRecipAndWaypts: FALSE4");
      return false;
   }

   // check distance at waypoints
   for (long i=0;i<mWaypoints.getNumber();i++)
   {
      /*gConsole.output(cChannelSim, "  I=%d, LastCommand=(%f, %f, %f), ThisCommand=(%f, %f, %f).",
         i,
         pLastCommand->getWaypoints()[i].x, pLastCommand->getWaypoints()[i].y, pLastCommand->getWaypoints()[i].z,
         mWaypoints[i].x, mWaypoints[i].y, mWaypoints[i].z
         );*/
      
      if (pLastCommand->getWaypoints()[i].distance(mWaypoints[i]) > distanceCheck)
      {
         //gConsole.output(cChannelSim, "  BC::meterRecipAndWaypts: FALSE4");
         return false;
      }
   }

   //gConsole.output(cChannelSim, "BC::meterRecipAndWaypts: TRUE (checking for meter).");
   return true;
}

//==============================================================================
// BCommand::meter
//==============================================================================
bool BCommand::meter(BCommand*  pLastCommand)
{
   //gConsole.output(cChannelSim, "BC::meter:");

   DWORD now = timeGetTime();

   if (!pLastCommand)
   {
      //gConsole.output(cChannelSim, "  FALSE1");
      return false;
   }

   if (mType != pLastCommand->getType())
   {
      //gConsole.output(cChannelSim, "  FALSE2");
      return false; // different command altogether
   }

   if (!pLastCommand->isTimeMetered() && !pLastCommand->isCountMetered())
   {
      //gConsole.output(cChannelSim, "  FALSE3");
      return false;
   }

   //gConsole.output(cChannelSim, "  Now=%d, SentTime=%d, Diff=%d.", now, pLastCommand->getSentTime(), now - pLastCommand->getSentTime());
   if ((pLastCommand->isTimeMetered()) && ((now - pLastCommand->getSentTime()) > mUrgencyTimeThreshhold))
   {
      //gConsole.output(cChannelSim, "  FALSE4");
      return false; // timer expired
   }

   if ((pLastCommand->isCountMetered()) && (pLastCommand->getUrgencyCount() >= mUrgencyCountThreshhold))
   {
      return false; // urgency count is too high
      //gConsole.output(cChannelSim, "  FALSE5");
   }

   setUrgencyCount(pLastCommand->getUrgencyCount()+1); // urgency goes up by one
   setSentTime(pLastCommand->getSentTime());

   //gConsole.output(cChannelSim, "  TRUE (metered).");
   return true;   
}

//==============================================================================
// BCommand::calcBaseChecksum
//==============================================================================
DWORD BCommand::calcBaseChecksum()
{
   #ifdef DEBUGCHECKSUM
      DWORD checksum=0;
      Crc32Long(checksum, mPlayerID);
      Crc32Long(checksum, mID);
      Crc32Long(checksum, mSenderType);
      Crc32Long(checksum, mSenders.getNumber());
      for (long i=0;i<mSenders.getNumber();i++)
         Crc32Long(checksum, mSenders[i]);
      Crc32Long(checksum, mRecipientType);
      Crc32Long(checksum, mRecipients.getNumber());
      for (i=0;i<mRecipients.getNumber();i++)
         Crc32Long(checksum, mRecipients[i]);   
      Crc32Long(checksum, mWaypoints.getNumber());
      for (i=0;i<mWaypoints.getNumber();i++)
      {
         Crc32Float(checksum, mWaypoints[i].x);
         Crc32Float(checksum, mWaypoints[i].y);
         Crc32Float(checksum, mWaypoints[i].z);
      }
      Crc32Long(checksum, mFlags.getNumber());
      long numberOfBytes = (mFlags.getNumber()/8) + (mFlags.getNumber()%8==0?0:1);
      const unsigned char* flagBytes=mFlags.getBits();
      for(i=0; i<numberOfBytes; i++)
         Crc321Byte(checksum, flagBytes[i]);
      Crc321Byte(checksum, mUrgencyCount);
      return checksum;
   #else
      return 0;
   #endif
}

//==============================================================================
// BCommand::isValidTerrainPosition
//==============================================================================
bool BCommand::isValidTerrainPosition(const BVector& position) const
{
   if (position.x < gWorld->getSimBoundsMinX() ||
       position.z < gWorld->getSimBoundsMinZ() ||
       position.x >=  gWorld->getSimBoundsMaxX() ||
       position.z >=  gWorld->getSimBoundsMaxZ())
   {
      return false;
   }
   return true;
}

//==============================================================================
// BCommand::sync
//==============================================================================
void BCommand::sync()
{
   #ifdef SYNC_Command   
      long i;
      //syncCommandData("mPlayerID", mPlayerID);
      //syncCommandData("mID", mID);
      //syncCommandData("mSenderType", mSenderType);
      //syncCommandData("mSenders.getNumber", mSenders.getNumber());
      //for (i=0;i<mSenders.getNumber();i++)
      //   syncCommandData("mSender", mSenders[i]);
      //syncCommandData("mRecipientType", mRecipientType);
      //syncCommandData("mRecipients.getNumber", mRecipients.getNumber());
      if(mRecipients.getNumber()>2)
      {
         for (i=0;i<mRecipients.getNumber();i++)
            syncCommandData("mRecipients", mRecipients[i].asLong());
      }
      //syncCommandData("mWaypoints.getNumber", mWaypoints.getNumber());
      for (i=0;i<mWaypoints.getNumber();i++)
         syncCommandData("mWaypoint", mWaypoints[i]);
      // ajl 9/12/02 - Cant sync number of flags because the number is saved/loaded correctly.
      // Instead just sync bits that are on which should by in-sync.
      //syncCommandData("mFlags.getNumber", mFlags.getNumber());
      for(i=0; i<mFlags.getNumber(); i++)
      {
         if(mFlags.isBitSet(i))
            syncCommandCode("mFlags.isBitSet(i)");
      }
      syncCommandData("mUrgencyCount", mUrgencyCount);
   #endif
}
