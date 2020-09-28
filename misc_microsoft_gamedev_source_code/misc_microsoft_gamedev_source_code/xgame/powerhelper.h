//==============================================================================
// powerhelper.h
//
// Copyright (c) 2008 Ensemble Studios
//==============================================================================
#pragma once

#include "simtypes.h"

class BPlayer;
class BProtoAction;
class BPower;
class BPowerUser;
class BUnit;
class BProtoPower;

namespace BPowerHelper
{
   struct BBomberData
   {
      BBomberData();

      void clear();

      bool save(BStream* pStream, int saveType) const;
      bool load(BStream* pStream, int saveType);

      BEntityID BomberId;
      float BomberFlyinDistance;
      float BomberFlyinHeight;
      float BomberBombHeight;
      float BomberSpeed;
      float BombTime;
      float FlyoutTime;
      float AdditionalHeight;
   };

   struct BLimitData 
   {
      BLimitData();

      void clear();

      bool save(BStream* pStream, int saveType) const;
      bool load(BStream* pStream, int saveType);

      BObjectTypeID mOTID;
      uint          mLimit;
   };

   struct BHUDSounds
   {
      BHUDSounds();

      bool loadFromProtoPower(const BProtoPower* pProtoPower, int level);

      void clear();

      bool save(BStream* pStream, int saveType) const;
      bool load(BStream* pStream, int saveType);

      BCueIndex HudUpSound;
      BCueIndex HudAbortSound;
      BCueIndex HudFireSound;
      BCueIndex HudLastFireSound;
      BCueIndex HudStartEnvSound;
      BCueIndex HudStopEnvSound;
   };

   typedef BSmallDynamicSimArray<BLimitData> BLimitDataArray;
   typedef std::pair<BEntityID, BVector> BPlacementInfo;
   typedef BSmallDynamicSimArray<BPlacementInfo> BPlacementInfoArray;

   BEntityID createBomber(BProtoObjectID bomberProtoId, BBomberData& bomberData, const BVector& startLocation, BVector startDirection, BPlayerID playerId);
   bool updateBomberPosition(const BBomberData& bomberData, float totalElapsed, float lastUpdateElapsed);

   void impulsePhysicsAirUnits(const BVector& location, float radius, int percentChanceToImpulse);
   void impulsePhysicsAirUnit(BUnit& unit, float impulseScale = 1.0f, const BVector* impulseTarget = NULL, float radius = 0.0f);
   BProtoAction* getFirstProtoAction(BPlayer& player, BProtoObjectID protoObjectId);

   bool checkSquadPlacement(const BPlayer& player, const BVector& location, const BEntityIDArray& squads);
   bool getTransportableSquads(BPlayerID playerId, const BVector& location, float radius, BEntityIDArray& outSquads);

   BPowerUser* getPowerUser(const BPower& power); 

   bool checkPowerLocation(BPlayerID playerId, const BVector& targetLocation, const BPower* pSourcePower, bool strikeInvalid);
}