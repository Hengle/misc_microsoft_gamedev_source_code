//==============================================================================
// aiteleporterzone.cpp
//
// Copyright (c) 2007 Ensemble Studios
//==============================================================================

// xgame
#include "common.h"
#include "aitypes.h"
#include "aiteleporterzone.h"

GFIMPLEMENTVERSION(BAITeleporterZone, 1);

//==============================================================================
//==============================================================================
BAITeleporterZone::BAITeleporterZone()
{
   mID = 0;
   resetNonIDData();
}


//==============================================================================
//==============================================================================
void BAITeleporterZone::resetNonIDData()
{
   mUnitID = cInvalidObjectID;
   mXMin = 0.0f;
   mZMin = 0.0f;
   mXMax = 0.0f;
   mZMax = 0.0f;
}


//==============================================================================
//==============================================================================
void BAITeleporterZone::setExtents(BVector boxExtent1, BVector boxExtent2)
{
   BASSERT(boxExtent1.x != boxExtent2.x);
   BASSERT(boxExtent1.z != boxExtent2.z);
   mXMin = static_cast<float>(Math::fSelectMin(boxExtent1.x, boxExtent2.x));
   mZMin = static_cast<float>(Math::fSelectMin(boxExtent1.z, boxExtent2.z));
   mXMax = static_cast<float>(Math::fSelectMax(boxExtent1.x, boxExtent2.x));
   mZMax = static_cast<float>(Math::fSelectMax(boxExtent1.z, boxExtent2.z));
}


//==============================================================================
//==============================================================================
void BAITeleporterZone::setPointRadius(BVector spherePos, float sphereRad)
{
   BASSERT(sphereRad > 0.0f);
   mX = spherePos.x;
   mY = spherePos.y;
   mZ = spherePos.z;
   mRadius = sphereRad;
}


//==============================================================================
//==============================================================================
bool BAITeleporterZone::save(BStream* pStream, int saveType) const
{
   GFWRITEVAR(pStream, BAITeleporterZoneID, mID);
   GFWRITEVAR(pStream, BEntityID, mUnitID);
   GFWRITEVAR(pStream, float, mData[0]);
   GFWRITEVAR(pStream, float, mData[1]);
   GFWRITEVAR(pStream, float, mData[2]);
   GFWRITEVAR(pStream, float, mData[3]);
   return (true);
}


//==============================================================================
//==============================================================================
bool BAITeleporterZone::load(BStream* pStream, int saveType)
{
   GFREADVAR(pStream, BAITeleporterZoneID, mID);
   GFREADVAR(pStream, BEntityID, mUnitID);
   GFREADVAR(pStream, float, mData[0]);
   GFREADVAR(pStream, float, mData[1]);
   GFREADVAR(pStream, float, mData[2]);
   GFREADVAR(pStream, float, mData[3]);
   return (true);
}


//==============================================================================
//==============================================================================
bool BAITeleporterZone::containsPosition(BVector pos) const
{
   BAITeleporterZoneType type = mID.getType();
   if (type == AITeleporterZoneType::cAABB)
   {
      if (pos.x < mXMin)
         return (false);
      if (pos.z < mZMin)
         return (false);
      if (pos.x > mXMax)
         return (false);
      if (pos.z > mZMax)
         return (false);
      return (true);
   }
   else if (type == AITeleporterZoneType::cSphere)
   {
      BVector zonePos(mX, mY, mZ);
      float radiusSqr = mRadius * mRadius;
      if (zonePos.xzDistanceSqr(pos) > radiusSqr)
         return (false);
      else
         return (true);
   }

   // Error
   BFAIL("Invalid AITeleporterZoneType.");
   return (false);
}