//==============================================================================
// squadactionmove.h
// Copyright (c) 2005-2007 Ensemble Studios
//==============================================================================

#pragma once
#include "movementHelper.h"

class BUnitOpp;

// Helper class used by the platoon and squad for finding paths
class BFindPathHelper
{
   public:
      BFindPathHelper();

      // Must set all this data before using the utility functions
      void                       setTarget(const BSimTarget* target);
      BSimTarget&                getTarget() { return mTarget; }
      void                       setEntity(BEntity* pEntity, bool ignorePlatoonMates=true);
      void                       setPathingRadius(float radius)                        { mPathingRadius = radius; }
      void                       setPathableAsFlyingUnit(bool value)                   { mFlagPathableAsFlyingUnit = value; }
      void                       setPathableAsFloodUnit(bool value)                    { mFlagPathableAsFloodUnit = value; }
      void                       setPathableAsScarabUnit(bool value)                   { mFlagPathableAsScarabUnit = value; }
      void                       enableReducePath(bool value)                          { mFlagReducePath = value; }
      void                       enablePathAroundSquads(bool value)                    { mFlagPathAroundSquads = value; }
      void                       enableSkipLRP(bool value)                             { mFlagSkipLRP = value; }

      // Path utility functions
      uint                       findPath(BPath& path, const BDynamicSimVectorArray &waypoints, const BVector* overrideStartPosition, long targetID, bool useTargetRange);
      uint                       findPath(BPath& path, BVector pos1, BVector pos2, bool failOnPathing, long targetID, bool useTargetRange);
      bool                       appendPath(BPath& sourcePath, BPath& destinationPath);
      bool                       reducePath(BPath& path);
      BSimCollisionResult        checkForCollisions(BVector pos1, BVector pos2, float radius, DWORD lastPathTime);

   private:
      uint                       fixupPathAroundObstructions(BPath& path, float radius, float range, int pathClass);
      void                       backPath(BPath& path, float radius, bool generateWaypoints);

      bool                       isCollisionEnabledWithEntity(BEntity* pEntity) const;
      void                       updatePathingLimits(bool longRange);
      bool                       testForOneWayBarrierLineCross(BVector &p1, BVector &p2, BVector &suggestedStop);

      BEntityIDArray             mIgnoreUnits;
      BSimTarget                 mTarget;
      BEntity*                   mpEntity;
      float                      mPathingRadius;      
      bool                       mFlagPathableAsFlyingUnit;
      bool                       mFlagPathableAsFloodUnit;
      bool                       mFlagPathableAsScarabUnit;
      bool                       mFlagReducePath;
      bool                       mFlagPathAroundSquads;
      bool                       mFlagSkipLRP;      
};

