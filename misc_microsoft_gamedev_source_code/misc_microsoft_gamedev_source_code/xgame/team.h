//==============================================================================
// team.h
//
// Copyright (c) 2006-2007 Ensemble Studios
//==============================================================================
#pragma once

// Includes
#include "maximumsupportedplayers.h"

// Forward declarations
class BObject;
class BUnit;

//==============================================================================
// BTeam
//==============================================================================
class BTeam
{
   public:
                                          BTeam();
                                          ~BTeam();

      bool                                setup(long id);
      void                                reset();
      void                                update(float elapsedTime);

      long                                getID() const { return mID; }

      void                                addPlayer(long id) { mPlayers.add(id); }
      long                                getNumberPlayers() const { return mPlayers.getNumber(); }
      long                                getPlayerID(long index) const { return mPlayers[index]; }

      void                                addVisibleObject(BEntityID id);
      void                                removeVisibleObject(BEntityID id);
      bool                                isObjectVisible(BEntityID id) const;
      BBitArray*                          getVisibleUnits() { return &mVisibleUnits; }
      const BBitArray*                    getVisibleUnits() const { return &mVisibleUnits; }
      BBitArray*                          getVisibleProjectiles() { return &mVisibleProjectiles; }
      const BBitArray*                    getVisibleProjectiles() const { return &mVisibleProjectiles; }
      BBitArray*                          getVisibleObjects() { return &mVisibleObjects; }
      const BBitArray*                    getVisibleObjects() const { return &mVisibleObjects; }

      bool                                hasSpies() { return mHasSpies; }
      void                                setSpies(bool val=true) { mHasSpies = val; }

      //static functions for ease of use when scattered through the code.
      static bool                         doesTeamHaveSpies(BTeamID id);
      static void                         setSpiesForTeam(BTeamID id, bool val=true);
      static bool                         canTeamASeeTeamB( BTeamID teamIDA, BTeamID teamIDB );

      void                                onRelease(BEntityID id);

      bool                                isOnTeam(BPlayerID id) const { return mPlayers.contains(id); }

      GFDECLAREVERSION();
      bool save(BStream* pStream, int saveType) const;
      bool load(BStream* pStream, int saveType);

   protected:
                                                   // 56 bytes total
      BSmallDynamicSimArray<long> mPlayers;        // 8 bytes
      long mID;                                    // 4 bytes
      BBitArray mVisibleUnits;                     // 12 bytes
      BBitArray mVisibleProjectiles;               // 12 bytes
      BBitArray mVisibleObjects;                   // 12 bytes
      bool mHasSpies;                              // 4 bytes
};