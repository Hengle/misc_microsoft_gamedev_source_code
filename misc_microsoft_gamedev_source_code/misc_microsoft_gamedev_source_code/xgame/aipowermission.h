//==============================================================================
// aipowermission.h
//
// Copyright (c) 2008 Ensemble Studios
//==============================================================================
#pragma once

// xgame includes
#include "aitypes.h"
#include "gamefilemacros.h"

class BPowerCleansing;
class BPowerOrbital;
class BPowerCarpetBombing;
class BPowerRage;
class BPowerRepair;
class BPowerWave;

//==============================================================================
//==============================================================================
class BAIPowerMission : public BAIMission
{
public:
   BAIPowerMission()
   {
      mType = MissionType::cPower;
   };
   ~BAIPowerMission()
   {
   }

   virtual void resetNonIDData();
   virtual void update(DWORD currentGameTime);
   virtual void updateStateCreate(DWORD currentGameTime);
   virtual void updateStateWorking(DWORD currentGameTime);
   virtual bool processTerminalConditions(DWORD currentGameTime);

   BPowerID getPowerID() const { return (mPowerID); }
   BEntityID getOwnerID() const { return (mOwnerID); }
   void setPowerID(BPowerID v) { mPowerID = v; }
   void setOwnerID(BEntityID v) { mOwnerID = v; }

   GFDECLAREVERSION();
   bool save(BStream* pStream, int saveType) const;
   bool load(BStream* pStream, int saveType);

protected:

   void updateCleansing(BPowerCleansing* pPower, DWORD currentGameTime);
   void updateRage(BPowerRage* pPower, DWORD currentGameTime);
   void updateWave(BPowerWave* pPower, DWORD currentGameTime);
   void updateOrbital(BPowerOrbital* pPower, DWORD currentGameTime);


   BVector mPreviousPosition;       // The previous position we used.
   DWORD mTimestampNextInput;       // Effective time of next input.
   float mTimeElapsedSinceLastInput;

   DWORD mTimeElapsedSinceLastRageJump;
   DWORD mTimeElapsedSinceLastRageMove;
   DWORD mTimeElapsedSinceLastOrbitalBomb;

   BPowerID mPowerID;               // The ID of the power we are controlling.
   BEntityID mOwnerID;              // The ID of the squad who owns the mission if any... (mainly for covenant.)
};