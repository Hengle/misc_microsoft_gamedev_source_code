//=============================================================================
// placementrules.h
//
// Copyright (c) 2006 Ensemble Studios
//=============================================================================
#pragma once

#include "xmlreader.h"

//=============================================================================
// Forward declarations.
//=============================================================================
class BUnit;



//=============================================================================
// class BPlacementRuleUnit
//=============================================================================
class BPlacementRuleUnit
{
public:

   enum RuleType
   {
      cAvoidUnit = 0,
      cBeNearUnit
   };

   BPlacementRuleUnit::BPlacementRuleUnit(long type, float distance, long playerScope, long lifeScope, long ruleType) :mType(type), mDistance(distance), mPlayerScope(playerScope), mLifeScope(lifeScope), mRuleType(ruleType) {};
   BPlacementRuleUnit::BPlacementRuleUnit() {};

   long getUnitType() const { return mType; }
   float getDistance() const { return mDistance; }
   long getPlayerScope() const { return mPlayerScope; }
   long getLifeScope() const { return mLifeScope; }
   long getRuleType() const { return mRuleType; }

protected:

   long mType;
   float mDistance;
   long mPlayerScope;
   long mLifeScope;
   long mRuleType;
};


//=============================================================================
// class BPlacementRule
//=============================================================================
class BPlacementRule
{
public:
   enum
   {
      cRuleFoundationAny=0,
      cRuleFoundationSolid,
      cRuleFoundationFullyBuilt
   };

   enum
   {
      cSpecialTypeBuilder,
   };

   // Flags
   enum
   {
      cRenderFlag    = (1 << 0),
   };

   BPlacementRule(void) : mErrorStringID(-1), mSuccessStringID(-1) {}
   virtual                 ~BPlacementRule(void) {}

   virtual bool            evaluate(const BUnit* pBuilder, const long protoID, const long playerID, const BVector &location, const BVector &forward,
      const BVector &right, long losMode, DWORD flags, long *errorStringID, long *successStringID, BEntityIDArray *pCachedUnitsToTrack = NULL) const = 0;
   virtual bool            loadXML(BXMLNode node);
   const long              getErrorStringID(void) const {return(mErrorStringID);}
   virtual bool            addPlacementRuleUnits(BDynamicSimArray<BPlacementRuleUnit>& placementRuleUnits) { placementRuleUnits; return false; }

   int                     getSpecialType(const char* pName) const;

protected:
   long                    mErrorStringID;
   long                    mSuccessStringID;
};


//=============================================================================
// class BPlacementRuleAnd
//=============================================================================
class BPlacementRuleAnd : public BPlacementRule
{
public:
   BPlacementRuleAnd(void) {}
   virtual                 ~BPlacementRuleAnd(void);

   virtual bool            evaluate(const BUnit* pBuilder, const long protoID, const long playerID, const BVector &location, const BVector &forward,
      const BVector &right, long losMode, DWORD flags, long *errorStringID, long *successStringID, BEntityIDArray *pCachedUnitsToTrack = NULL) const;
   virtual bool            loadXML(BXMLNode node);

   virtual bool            addPlacementRuleUnits(BDynamicSimArray<BPlacementRuleUnit>& placementRuleUnits);

protected:
   BDynamicSimArray<BPlacementRule*> mChildRules;
};


//=============================================================================
// class BPlacementRuleOr
//=============================================================================
class BPlacementRuleOr : public BPlacementRule
{
public:
   BPlacementRuleOr(void) {}
   virtual                 ~BPlacementRuleOr(void);

   virtual bool            evaluate(const BUnit* pBuilder, const long protoID, const long playerID, const BVector &location, const BVector &forward,
      const BVector &right, long losMode, DWORD flags, long *errorStringID, long *successStringID, BEntityIDArray *pCachedUnitsToTrack = NULL) const;
   virtual bool            loadXML(BXMLNode node);

   virtual bool            addPlacementRuleUnits(BDynamicSimArray<BPlacementRuleUnit>& placementRuleUnits);

protected:
   BDynamicSimArray<BPlacementRule*> mChildRules;
};


//=============================================================================
// class BPlacementRuleDistanceAtMostFromType
//=============================================================================
class BPlacementRuleDistanceAtMostFromType : public BPlacementRule
{
public:
   BPlacementRuleDistanceAtMostFromType(void);
   virtual                 ~BPlacementRuleDistanceAtMostFromType(void) {}

   virtual bool            evaluate(const BUnit* pBuilder, const long protoID, const long playerID, const BVector &location, const BVector &forward,
      const BVector &right, long losMode, DWORD flags, long *errorStringID, long *successStringID, BEntityIDArray *pCachedUnitsToTrack = NULL) const;
   virtual bool            loadXML(BXMLNode node);
   virtual bool            addPlacementRuleUnits(BDynamicSimArray<BPlacementRuleUnit>& placementRuleUnits);

protected:
   float                   mDistance;
   long                    mType;
   long                    mPlayerScope;
   long                    mLifeScope;
   bool                    mFlagSpecialType:1;

};

//=============================================================================
// class BPlacementRuleDistanceAtLeastFromType
//=============================================================================
class BPlacementRuleDistanceAtLeastFromType : public BPlacementRule
{
public:
   BPlacementRuleDistanceAtLeastFromType(void);
   virtual                 ~BPlacementRuleDistanceAtLeastFromType(void) {}

   virtual bool            evaluate(const BUnit* pBuilder, const long protoID, const long playerID, const BVector &location, const BVector &forward,
      const BVector &right, long losMode, DWORD flags, long *errorStringID, long *successStringID, BEntityIDArray *pCachedUnitsToTrack = NULL) const;
   virtual bool            loadXML(BXMLNode node);

   virtual bool            addPlacementRuleUnits(BDynamicSimArray<BPlacementRuleUnit>& placementRuleUnits);

protected:
   float                   mDistance;
   long                    mType;
   long                    mPlayerScope;
   long                    mLifeScope;
   long                    mFoundationType;
   bool                    mFlagIncludeObstructionRadius:1;
   bool                    mFlagSpecialType:1;

};


//=============================================================================
// class BPlacementRuleObstructionAtLeastFromType
//=============================================================================
class BPlacementRuleObstructionAtLeastFromType : public BPlacementRule
{
public:
   BPlacementRuleObstructionAtLeastFromType(void);
   virtual                    ~BPlacementRuleObstructionAtLeastFromType(void) {}

   virtual bool               evaluate(const BUnit* pBuilder, const long protoID, const long playerID, const BVector &location, const BVector &forward,
      const BVector &right, long losMode, DWORD flags, long *errorStringID, long *successStringID, BEntityIDArray *pCachedUnitsToTrack = NULL) const;

   virtual bool               loadXML(BXMLNode node);

   virtual bool            addPlacementRuleUnits(BDynamicSimArray<BPlacementRuleUnit>& placementRuleUnits);

protected:
   float                      mDistance;
   long                       mType;
   long                       mPlayerScope;
   long                       mLifeScope;
   long                       mFoundationType;
   bool                       mFlagSpecialType:1;

};



//=============================================================================
// class BPlacementRuleDistanceAtMostFromWater
//=============================================================================
/*
class BPlacementRuleDistanceAtMostFromWater : public BPlacementRule
{
public:
   BPlacementRuleDistanceAtMostFromWater(void);
   virtual                 ~BPlacementRuleDistanceAtMostFromWater(void) {}

   virtual bool            evaluate(const BUnit* pBuilder, const long protoID, const long playerID, const BVector &location, const BVector &forward,
      const BVector &right, long losMode, DWORD flags, long *errorStringID, long *successStringID, BEntityIDArray *pCachedUnitsToTrack = NULL) const;
   virtual bool            loadXML(const BXMLNode *node);

protected:
   float                   mDistance;
};
*//*
//=============================================================================
// class BPlacementRuleDistanceAtMostFromMapEdge
//=============================================================================
class BPlacementRuleDistanceAtMostFromMapEdge : public BPlacementRule
{
public:
   BPlacementRuleDistanceAtMostFromMapEdge(void);
   virtual                 ~BPlacementRuleDistanceAtMostFromMapEdge(void) {}

   virtual bool            evaluate(const BUnit* pBuilder, const long protoID, const long playerID, const BVector &location, const BVector &forward,
      const BVector &right, long losMode, DWORD flags, long *errorStringID, long *successStringID, BEntityIDArray *pCachedUnitsToTrack = NULL) const;
   virtual bool            loadXML(const BXMLNode *node);

protected:
   float                   mDistance;
};


//=============================================================================
// class BPlacementRuleMapType
//=============================================================================
class BPlacementRuleMapType : public BPlacementRule
{
public:
   BPlacementRuleMapType(void);
   virtual                 ~BPlacementRuleMapType(void) {}

   virtual bool            evaluate(const BUnit* pBuilder, const long protoID, const long playerID, const BVector &location, const BVector &forward,
      const BVector &right, long losMode, DWORD flags, long *errorStringID, long *successStringID, BEntityIDArray *pCachedUnitsToTrack = NULL) const;
   virtual bool            loadXML(const BXMLNode *node);

protected:
   long                    mMapType;
};


//=============================================================================
// class BPlacementRuleInsidePerimeterWall
//=============================================================================
class BPlacementRuleInsidePerimeterWall : public BPlacementRule
{
public:
   BPlacementRuleInsidePerimeterWall(void);
   virtual                 ~BPlacementRuleInsidePerimeterWall(void) {}

   virtual bool            evaluate(const BUnit* pBuilder, const long protoID, const long playerID, const BVector &location, const BVector &forward,
      const BVector &right, long losMode, DWORD flags, long *errorStringID, long *successStringID, BEntityIDArray *pCachedUnitsToTrack = NULL) const;
   virtual bool            loadXML(const BXMLNode *node);

protected:
   float                   mDistance;
   long                    mType;

};
*/

//=============================================================================
// class BPlacementRules
//=============================================================================
class BPlacementRules
{
public:
   BPlacementRules(void);
   ~BPlacementRules(void);

   static void             reset() { sUnitsToTrack.clear(); }

   bool                    evaluate(const BUnit* pBuilder, const long protoID, const long playerID, const BVector &location, const BVector &forward,
      const BVector &right, long losMode, DWORD flags, long *errorStringID, long * successStringID, BEntityIDArray *pCachedUnitsToTrack = NULL) const;

   bool                    loadXML(const BSimString &filename);

   static BPlacementRule*  createFromXML(BXMLNode node);

   const BSimString&       getFilename(void) const {return(mFilename);}
   
   const BDynamicSimArray<BPlacementRuleUnit> &getPlacementRuleUnits() const;

   static BEntityIDArray   &getUnitsToTrack();

   static float            sMaxDistance;

protected:
   BSimString              mFilename;

   // Everything is ANDed together at the top level.
   BPlacementRuleAnd       mRoot;      

   BDynamicSimArray<BPlacementRuleUnit> mPlacementRuleUnits;

   static BEntityIDArray sUnitsToTrack;
};
