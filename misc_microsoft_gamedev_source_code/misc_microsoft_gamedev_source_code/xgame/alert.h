//==============================================================================
// alert.h
//
// The alert system.
//
// Copyright (c) 2006-2007 Ensemble Studios
//==============================================================================
#pragma once


#include "simtypes.h"


class BPlayer;
typedef uint BAlertType;


//=============================================================================
// class BAlertID
//=============================================================================
class BAlertID
{
public:

   enum
   {
      cInvalidID        = UINT32_MAX, // Invalid ID
      cMaxIndex         = UINT16_MAX, // Max index allowed.
      cPlayerMask       = 0xE0000000, // Mask for the player
      cRefCountMask     = 0x1FFF0000, // Mask for the refcount.
      cIndexMask        = 0x0000FFFF, // Mask for the index.
      cRefCountBitShift = 16,         // # of bits to shift the refcount within the ID
      cPlayerBitShift   = 29,         // # of bits to shift the player within the ID
   };

   BAlertID() : mID(static_cast<uint32>(BAlertID::cInvalidID)) {}
   BAlertID(BPlayerID playerID, uint refCount, uint index) { set(playerID, refCount, index); }

   uint getIndex() const { return static_cast<uint>(mID & BAlertID::cIndexMask); }
   uint getRefCount() const { return static_cast<uint>((mID & BAlertID::cRefCountMask) >> BAlertID::cRefCountBitShift); }
   BPlayerID getPlayerID() const { return static_cast<BPlayerID>((mID & BAlertID::cPlayerMask) >> BAlertID::cPlayerBitShift); }
   bool isValid() const { return (mID == BAlertID::cInvalidID ? false : true); }
   void invalidate() { mID = static_cast<uint32>(BAlertID::cInvalidID); }
   void set(BPlayerID playerID, uint refCount, uint index) { BASSERT(index < BAlertID::cMaxIndex); mID = ((playerID << BAlertID::cPlayerBitShift) & BAlertID::cPlayerMask) | ((refCount << BAlertID::cRefCountBitShift) & BAlertID::cRefCountMask) | (index & BAlertID::cIndexMask);}
   void bumpRefCount() { BASSERT(getIndex() < BAlertID::cMaxIndex); set(getPlayerID(), getRefCount()+1, getIndex()); }
   bool operator == (const BAlertID rhs) const { return (mID == rhs.mID); }
   bool operator != (const BAlertID rhs) const { return (mID != rhs.mID); }

   // Because we store these in BTriggerVarInt
   operator int32() const { return (static_cast<int32>(mID)); }
   BAlertID(int32 val) { mID = static_cast<uint32>(val); }
   BAlertID& operator= (int32 val) { mID = static_cast<uint32>(val); return(*this); }

protected:
   uint32 mID;
};

__declspec(selectany) extern const BAlertID cInvalidAlertID = BAlertID();


//=============================================================================
// class BAlert
//=============================================================================
class BAlert
{
public:

   BAlert();
   void resetNonIDData();
   
   BVector getLocation() const { return (mLocation); }
   BAlertID& getID() { return (mID); }
   BAlertID getID() const { return (mID); }
   DWORD getCreateTime() const { return (mCreateTime); }
   DWORD getExpireTime() const { return (mExpireTime); }
   DWORD getDeleteTime() const { return (mDeleteTime); }
   BAlertType getType() const { return (mType); }
   BEntityID getUnitID() const { return (mEntityID); }
   BEntityID getAttackerID() const { return (mAttackerID); }
   BPlayerID getFlaringPlayerID() const { return (mFlaringPlayerID); }
   bool getQueued() const { return (mbQueued); }
   bool getExpired() const { return (mbExpired); }
   bool getVisible() const { return (mbVisible); }
   bool getPlayAlert() const { return (mbPlayAlert); }
   bool getAttackOnBase() const { return (mbAttackOnBase); }

   void setLocation(BVector v) { mLocation = v; }
   void setID(BAlertID v) { mID = v; }
   void setCreateTime(DWORD v) { mCreateTime = v; }
   void setExpireTime(DWORD v) { mExpireTime = v; }
   void setDeleteTime(DWORD v) { mDeleteTime = v; }
   void setType(BAlertType v) { mType = v; }
   void setEntityID(BEntityID v) { mEntityID = v;  }
   void setAttackerID(BEntityID v) { mAttackerID = v; }
   void setFlaringPlayerID(BPlayerID v) { mFlaringPlayerID = v; }
   void setQueued(bool v) { mbQueued = v; }
   void setExpired(bool v) { mbExpired = v; }
   void setVisible(bool v) { mbVisible = v; }
   void setPlayAlert(bool v) { mbPlayAlert = v; }
   void setAttackOnBase(bool v) { mbAttackOnBase = v; }

protected:

   BVector     mLocation;
   BAlertID    mID;
   BAlertType  mType;
   DWORD       mCreateTime;
   DWORD       mExpireTime;
   DWORD       mDeleteTime;

   // AlertType::cAttack specific
   BEntityID   mEntityID;
   BEntityID   mAttackerID;
   // AlertType::cFlare specific
   BPlayerID   mFlaringPlayerID;

   bool        mbQueued    : 1;
   bool        mbExpired   : 1;
   bool        mbVisible   : 1;
   bool        mbPlayAlert : 1;
   bool        mbAttackOnBase : 1;
};

typedef BSmallDynamicSimArray<BAlert> BAlertArray;
typedef BSmallDynamicSimArray<BAlertID> BAlertIDArray;
typedef int BAttackSeverity;

namespace AttackSeverity
{
   enum
   {
      cNone = 0,
      cSev1,
      cSev2,
      cSev3,
      // Update these if you have more.
      cMin = cNone,
      cMax = cSev3,
   };
}


//=============================================================================
// class BAlert
//=============================================================================
class BAlertManager
{
public:

   // Constructor
   BAlertManager(BPlayer* pPlayer);

   // Freelist logic
   BAlert* getAlert(BAlertID alertID);
   const BAlert* getAlert(BAlertID alertID) const;
   void deleteAlert(BAlertID alertID);

   // Functionality
   const BPlayer*       getPlayer() const { return (mpPlayer); }
   BPlayer*             getPlayer() { return (mpPlayer); }

   BPlayerID            getPlayerID() const;
   void                 clearAlerts();
   const BAlert*        peekGotoAlert() const;
   const BAlert*        getGotoAlert();
   BAlertID             getNextGotoAlertID();
   void                 setNextGotoAlertID(BAlertID v) { mNextGotoAlertID = v; }
   void                 setSquelchAlerts(bool squelch) { mbSquelchAlerts = squelch; }
   void                 setNewAlertsAllowed(bool allow) { mbNewAlertsAllowed = allow; }
   bool                 createAttackAlert(BVector pos, BEntityID attackingUnitID, BEntityID defenderUnitID);
   bool                 createFlareAlert(BVector pos, BPlayerID flaringPlayerID);
   bool                 createBaseDestructionAlert(BVector pos, BEntityID destroyedBaseID);
   bool                 createResearchCompleteAlert(BVector pos, BEntityID buildingID);
   bool                 createTrainingCompleteAlert(BVector pos, BEntityID squadID);
   bool                 createTransportCompleteAlert(BVector pos, BEntityID squadID);
   void                 update();
   const BAlertIDArray& getAlertIDs() const { return (mAlertIDs); }
   uint                 getFlareAlertIDs(BAlertIDArray& flareAlertIDs);
   uint                 getAttackAlertIDs(BAlertIDArray& attackAlertIDs);

   void                 render();

   // Alerts
   BAlertID getLastAllyFlareAlertID() const { return (mLastAllyFlare); }
   const BAlert* getLastAllyFlareAlert() const;
   void setLastAllyFlareAlertID(BAlertID alertID);

   BAlertID getLastAttackAlertID() const { return (mLastAttackAlert); }
   const BAlert* getLastAttackAlert() const;
   void setLastAttackAlertID(BAlertID alertID);

protected:

   BAlert*           createAlert();
   void              playSingleAttackAlert(BAlert *pAlert);
   bool              isAlertUIActive(const BAlert* pAlert=NULL) const;
   BAttackSeverity   getAttackSeverity(BEntityID defenderID, BEntityID attackerID);
   bool              isAttackOnBase(const BAlert* pAlert);

   BFreeList<BAlert, 4> mAlerts;
   BAlertIDArray mAlertIDs;
   BAlertID mNextGotoAlertID;
   DWORD mNextAttackAlertTime;
   BPlayer* mpPlayer;
   BAlertID mLastAllyFlare; // What is the latest ally flare.
   BAlertID mLastAttackAlert; // What is the latest attack alert.
   bool mbSquelchAlerts;
   bool mbNewAlertsAllowed;

   // Consts
   static const float cDistance;
   static const float cDistanceSqr;
   static const DWORD cAttackAlertExpireTime = 7000;
   static const DWORD cAttackAlertDeleteTime = 30000;
   static const DWORD cFlareAlertExpireTime  = 6000;
   static const DWORD cFlareAlertDeleteTime  = 30000;
   static const DWORD cAttackAlertCooldown   = 1000;
   static const DWORD cTransportAlertExpireTime = 4000;
   static const DWORD cTransportAlertDeleteTime = 30000;
   static const float cMinAlertDistanceSqr;
};