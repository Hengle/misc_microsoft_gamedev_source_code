//==============================================================================
// unitquery.h
//
// Copyright (c) 2006-2007 Ensemble Studios
//==============================================================================
#pragma once

class BConvexHull;
class BOPQuadHull;
class BBitArray;

//==============================================================================
// class BUnitQuery
// Data class used for doing unit queries by passing a single parm.
//==============================================================================
class BUnitQuery
{
public:

   // Query types
   enum
   {
      cQueryTypeNone,
      cQueryTypeTwoVector,
      cQueryTypePointRadius,
      cQueryTypeConvexHull,
      cQueryTypeOPQuadHull,
      cQueryTypeOBBNoRoll,
   };


   // Constructor / Destructors
   BUnitQuery();
   BUnitQuery(BVector point0, BVector point1);
   BUnitQuery(BVector center, float radius, bool circular);
   BUnitQuery(BConvexHull *pConvexHull);
   BUnitQuery(BOPQuadHull *pQuadHull);
   BUnitQuery(BVector center, BVector forward, BVector right, float extentX, float extentY, float extentZ);
   ~BUnitQuery();

   bool isType(long type) const { return(mQueryType == type); }
   long getType(void) const { return(mQueryType); }
   BVector getPoint0(void) const { return(mPoint0); }
   BVector getPoint1(void) const { return(mPoint1); }
   BVector getPoint2(void) const { return (mPoint2); }
   BVector getCenter(void) const { return(mCenter); }
   float getRadius(void) const { return(mRadius); }
   bool getCircular(void) const { return(mCircular); }
   BConvexHull* getConvexHull(void) const { return(mpConvexHull); }
   BOPQuadHull* getOPQuadHull(void) const { return(mpOPQuadHull); }

   void resetObjectTypeFilters(void) { mObjectTypeFilters.setNumber(0); }
   void resetPlayerFilters(void) { mPlayerFilters.setNumber(0); }
   void addObjectTypeFilter(long objectType) { mObjectTypeFilters.uniqueAdd(objectType); }
      void addPlayerFilter(long playerID) { mPlayerFilters.uniqueAdd(playerID); }
   //void addObjectTypeFilter(const BSmallDynamicSimLongArray& objectTypeList);
   bool passesQueryFilter(BUnit& unit) const;
   
   void setRelation(BPlayerID playerID, BRelationType relationType);
   BPlayerID getRelationPlayerID() const { return mRelationPlayerID; }
   BRelationType getRelationType() const { return mRelationType; }

   void setUnitVisibility(long playerID);
   

   // Flags
   void                       clearFlags();
   bool                       getFlagIgnoreLiving() const { return(mFlagIgnoreLiving); }
   void                       setFlagIgnoreLiving(bool v) { mFlagIgnoreLiving=v; }
   bool                       getFlagIgnoreDead() const { return(mFlagIgnoreDead); }
   void                       setFlagIgnoreDead(bool v) { mFlagIgnoreDead=v; }
   //bool                       getFlagAllowObjects() const { return(mFlagAllowObjects); }
   //void                       setFlagAllowObjects(bool v) { mFlagAllowObjects=v; }

protected:

   // Query Type
   long mQueryType;

   // Data for cQueryTypeTwoVector
   BVector mPoint0;
   BVector mPoint1;
   BVector mPoint2;

   // Data for cQueryTypePointRadius
   BVector mCenter;
   float mRadius;
   bool mCircular;

   // Relation
   BPlayerID mRelationPlayerID;
   BRelationType mRelationType;
   
   // Visibility
   BBitArray* mpVisibleUnitsArray;

   // Data for cQueryTypeConvexHull
   BConvexHull *mpConvexHull;

   // Data for cQueryTypeOPQuadHull
   BOPQuadHull *mpOPQuadHull;

   BSmallDynamicSimArray<long> mObjectTypeFilters;
   BSmallDynamicSimArray<long> mPlayerFilters;

   // Flags
   bool                       mFlagIgnoreLiving:1;
   bool                       mFlagIgnoreDead:1;
   //bool                       mFlagAllowObjects:1;
};
