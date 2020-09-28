//==============================================================================
// command.h
//
// Copyright (c) 1997-2007 Ensemble Studios
//==============================================================================
#pragma once

// Includes
#include "bitarray.h"
#include "packets.h"

// Forward declarations
class BUnit;
class BSquad;
class BArmy;

//==============================================================================
// BCommand
//==============================================================================
class BCommand : public BChannelPacket
{
   public:
      // Sender/recipient types
      enum
      {
         cUnit,
         cSquad,
         cArmy,
         cPlayer,
         cGame,
         cTrigger,
      };

      //Flags
      enum
      {
         cFlagAlternate,               //Alternate button pressed when command issued. Defaults to false.
         cNumberCommandFlags
      };

      // Preprocess flags
      enum
      {
         cPreProcessNoAction=0,
         cPreProcessDeleteSquad,
         cPreProcessCommandHandled
      };

                              BCommand(long type, long numberFlags);
      virtual                 ~BCommand();
      
      static long             getCommandType(const void* pData) { return static_cast<long>(((char*)pData)[BChannelPacket::getSignatureSize()]); }

      //Gets
      long                    getPlayerID( void ) const              { return(mPlayerID); }
      long                    getID( void ) const                    { return(mID); }
      long                    getSenderType( void ) const            { return(mSenderType); }
      long                    getNumberSenders( void ) const         { return(mSenders.getNumber()); }
      long*                   getSenders( void )                     { return(mSenders.getPtr()); }
      long                    getRecipientType( void ) const         { return(mRecipientType); }
      long                    getNumberRecipients( void ) const      { return(mRecipients.getNumber()); }
      BEntityID*              getRecipients( void )                  { return(mRecipients.getPtr()); }
      long                    getType( void ) const                  { return(mType); }
      const char*             getName( void ) const;
      DWORD                   getExecuteTime( void ) const           { return(mExecuteTime); }

      //Sets
      void                    setPlayerID( long v )                  { BASSERT(v <16); mPlayerID=v; }
      void                    setID( long v )                        { mID=v; }
      void                    setSenderType( const long v )          { mSenderType=v; }
      bool                    setSenders( const long numberSenders, const long*  pSenders );
      void                    setRecipientType( const long v )       { mRecipientType=v; }
      bool                    setRecipients( const long numberRecipients, const BEntityID*  pRecipients );
      void                    setExecuteTime( DWORD v )              { mExecuteTime=v; }

      //Reference count.                                                             
      //long                    getReferenceCount( void ) const        { return(mReferenceCount); }
      //void                    setReferenceCount( long v )            { mReferenceCount=v; }
      //void                    incrementReferenceCount( void )        { mReferenceCount++; }
      //void                    decrementReferenceCount( void )        { mReferenceCount--; if (mReferenceCount < 0) mReferenceCount=0; }

      //Queued reference count.  Used to denote how many entities still have to do initial
      //processing on this command.
      //long                    getQueueReferenceCount( void ) const   { return(mQueueReferenceCount); }
      //void                    setQueueReferenceCount( long v )       { mQueueReferenceCount=v; }
      //void                    incrementQueueReferenceCount( void )   { mQueueReferenceCount++; }
      //void                    decrementQueueReferenceCount( void )   { mQueueReferenceCount--; if (mQueueReferenceCount < 0) mQueueReferenceCount=0; }

      //Flag methods.
      virtual bool                    getFlag( long n ) const                { if (mFlags.isBitSet(n) > (DWORD)0) return(true); return(false); }
      virtual void                    setFlag( long n, bool v )              { if (v == true) mFlags.setBit(n); else mFlags.clearBit(n); }     

      //Waypoints.
      long                       getNumberWaypoints( void ) const       { return(mWaypoints.getNumber()); }
      BVector*                   getWaypoints( void )                   { return(mWaypoints.getPtr()); }
      const BVector&             getWaypoint( long index ) const;
      const BDynamicSimVectorArray&  getWaypointList( void ) const        { return(mWaypoints); }
      bool                       setWaypoints( const BVector*  pWaypoints, const long numberWaypoints );
      bool                       addWaypoint( const BVector &waypoint );

      //Facing
      virtual BVector         getFacing() const { return cInvalidVector; }

      //Rolled up "Target" methods.  If you don't know how to use these correctly, don't
      //use them:)
      virtual BEntityID       getTargetID( void ) const              { return(cInvalidObjectID); }
      virtual BVector         getTargetPosition( void ) const;
      virtual long            getAbilityID() const { return (-1); }
      virtual long            getSquadMode() const { return (-1); }
      virtual float           getAngle() const { return (0.0f); }

      //Alternate
      bool                    getAlternate( void ) const { return(getFlag(cFlagAlternate)); }

      //Execute method.  This is the method called by the command manager when it needs to execute
      //the command.  This will call the appropriate methods (processUnit, etc.) depending
      //on the recipient type.
      bool                    execute( void );

      virtual void            serialize(BSerialBuffer &sb);
      virtual void            deserialize(BSerialBuffer &sb);

      virtual void            sync();
      
      // Urgency functions
      virtual bool            meter(BCommand*  pLastSentCommand);   // determines whether we should actually send this command out or not
                                                                            // and if not, changes lastSentCommand to increase the urgency count

      void                    sent(void)                             { mSentTime = timeGetTime(); }
      void                    setSendTimedUrgent(bool val)           { mSendTimedUrgent = val; } // set whether or not to send secondary "urgent" command when the timer expires
      bool                    shouldSendTimedUrgent(void)            { return mSendTimedUrgent; }
      void                    setUrgencyCountThreshhold(long count)  { mUrgencyCountThreshhold = count; }
      void                    setUrgencyTimeThreshhold(DWORD time)   { mUrgencyTimeThreshhold = time; }
      long                    getSentTime(void)                      { return mSentTime; }
      void                    setSentTime(long time)                 { mSentTime = time; }
      long                    getUrgencyCount(void)                  { return (long) mUrgencyCount; }      
      void                    setUrgencyCount(long count)            { mUrgencyCount=(unsigned char)min(255, count); }            
      bool                    isCountMetered(void)                   { return (mUrgencyCountThreshhold != -1); }
      bool                    isTimeMetered(void)                    { return (mUrgencyTimeThreshhold != 0); }
      bool                    countHasExpired(void)                  { return (isCountMetered() && (mUrgencyCount >= mUrgencyCountThreshhold)); }
      bool                    timeHasExpired(void)                   { return (isTimeMetered() && ((timeGetTime()-mSentTime) > mUrgencyTimeThreshhold)); }      

      virtual void            serializeSignature(BSerialBuffer &sb)  { BChannelPacket::serializeSignature(sb); sb.add(mType); }
      virtual void            deserializeSignature(BSerialBuffer &sb) { BChannelPacket::deserializeSignature(sb); sb.get(&mType); }

      //Each command should override the virtual processXXXX methods as needed to do
      //the required specialization.
      virtual bool            processUnit( BUnit*  pUnit) { pUnit; return(false); }
      virtual bool            processSquad( BSquad*  pSquad ) { pSquad; return(false); }
      virtual bool            processArmy( BArmy*  pArmy ) { pArmy; return(false); }
      virtual bool            processPlayer( void ) { return(false); }
      virtual bool            processGame( void ) { return(false); }

      //IsDone.  Selectively implemented by a few commands.  Maybe more if it
      //turns out to be a good idea.  If you don't know how to not over-call/
      //over-use this, then don't call it at all.
      virtual bool            isDone( void )                         { return(false); }

   protected:            
      bool                    processUnits(void);
      bool                    processSquads(void);
      bool                    processArmies(void);

      static long             getSignatureSize(void)                 { return BChannelPacket::getSignatureSize()+sizeof(unsigned char); }     
      bool                    meterRecipientsAndWaypoints(BCommand*  pLastCommand, float distanceCheck);

      // ajl 9/12/02 - Added some checksum stuff to validate commands are being packed/unpacked correctly.
      DWORD                   calcBaseChecksum();

      bool                    isValidTerrainPosition(const BVector& position) const;

      //Variables that need to be passed around.
      long                    mPlayerID;
      long                    mID;
      long                    mSenderType;
      BDynamicSimLongArray    mSenders;
      long                    mRecipientType;
      BEntityIDArray     mRecipients;      
      BDynamicSimVectorArray      mWaypoints;
      BBitArray               mFlags;

      //Variables that DO NOT need to be passed around (they are reset when the command
      //is executed).  They DO HAVE to be saved, though.
      //long                    mReferenceCount;
      //long                    mQueueReferenceCount;
      DWORD                   mExecuteTime;

      long                    mSentTime;
      bool                    mSendTimedUrgent;
      unsigned char           mType;
      unsigned char           mUrgencyCount;

   private:
      long                    mUrgencyCountThreshhold;
      DWORD                   mUrgencyTimeThreshhold;
};

typedef BDynamicSimArray<BCommand*> BCommandPointerArray;