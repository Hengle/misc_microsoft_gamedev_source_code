//==============================================================================
// aiteleporterzone.h
//
// Copyright (c) 2008 Ensemble Studios
//==============================================================================
#pragma once

#include "aitypes.h"
#include "gamefilemacros.h"


//==============================================================================
// class BAITeleporterZone
//==============================================================================
class BAITeleporterZone
{
public:

   BAITeleporterZone();
   void resetNonIDData();

   void setID(BAITeleporterZoneID v) { mID = v; }
   void setID(uint refCount, BAITeleporterZoneType type, uint index) { mID.set(refCount, type, index); }
   void setUnitID(BEntityID unitID) { mUnitID = unitID; }
   void setExtents(BVector boxExtent1, BVector boxExtent2);
   void setPointRadius(BVector spherePos, float sphereRad);

   BAITeleporterZoneID getID() const { return (mID); }
   BAITeleporterZoneID& getID() { return (mID); }
   BEntityID getUnitID() const { return (mUnitID); }
   bool containsPosition(BVector pos) const;



   uint mDebugID;

   GFDECLAREVERSION();
   bool save(BStream* pStream, int saveType) const;
   bool load(BStream* pStream, int saveType);

protected:

   BAITeleporterZoneID mID;
   BEntityID mUnitID;
   union
   {
      struct  
      {
         float mXMin;
         float mZMin;
         float mXMax;
         float mZMax;
      };
      struct
      {
         float mX;
         float mY;
         float mZ;
         float mRadius;
      };
      struct
      {
         float mData[4];
      };
   };
};