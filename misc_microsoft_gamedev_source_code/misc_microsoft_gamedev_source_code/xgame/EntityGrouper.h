//==============================================================================
// EntityGrouper.h
// Copyright (c) 1999-2007 Ensemble Studios
//==============================================================================


// Includes
#pragma once
#include "Entity.h"

class BArmy;


//==============================================================================
//==============================================================================
class BEntityGroup : public IPoolable
{
public:
   BEntityGroup()             { }
   ~BEntityGroup()            { }

   virtual void               onAcquire(){ reset(); }
   virtual void               onRelease() { }
   DECLARE_FREELIST(BEntityGroup, 5);

   bool                       addEntity(BEntityID entityID);
   bool                       addEntities(const BEntityIDArray &entityIDs);
   const BEntityIDArray&      getEntities() const { return(mEntities); }

   void                       setCenter(BVector v) { mCenter = v; }
   BVector                    getCenter() const { return (mCenter); }

   //Misc.
   void                       render();
   void                       reset() { mCenter.zero(); mEntities.clear(); }

protected:
   BVector                    mCenter;
   BEntityIDArray             mEntities;
};



//==============================================================================
//==============================================================================
class BEntityGrouper
{
public:
   BEntityGrouper();
   ~BEntityGrouper();

   //Reset.  MUST be called before any grouping call (to clear the filters, etc.).
   void                       reset();

   //Filters.  Used to control how things are grouped.
   BEntityID                  getTargetID() const { return(mTargetID); }
   void                       setTargetID(BEntityID v) { mTargetID=v; }
   bool                       getModifiedCommand() const { return(mModifiedCommand); }
   void                       setModifiedCommand(bool v) { mModifiedCommand=v; }
   void                       setRadius(float v) { mRadiusSqr=v*v; }
   void                       groupBySpeed(bool bSpeed) { mSpeed = bSpeed; }
   void                       groupByProtoSquad(bool bProtoSquad) { mProtoSquad = bProtoSquad; }
   void                       setFlagCommonParentShortcut(bool v) { mFlagCommonParentShortcut = v; }

   //Grouping methods.  The results are placed into data inside this class,
   //so the expected usage is to have the BEntityGrouper figure out the groups
   //and then other calls will pull the results out.
   void                       groupEntities(const BEntityIDArray& entityIDs);
   //Methods to query the results.
   uint                       getNumberGroups() const { return(mGroups.getSize()); }
   const BEntityGroup*        getGroupByIndex(uint index) const;

   //Logical methods that can work indie from the grouping method(s).
   static BEntityID           commonParentID(const BEntityIDArray& entityIDs);
   static BEntityID           commonArmyIDForSquads(const BEntityIDArray& entityIDs, bool failOnInvalidSquads=true);
   static BArmy*              getOrCreateCommonArmyForSquads(const BEntityIDArray& squadIDs, BPlayerID playerID);

   //Misc methods.
   void                       render() const;

protected:
   //ActualGroupEntities does the work.
   void                       actualGroupEntities();
   void                       actualGroupSquadsByProtoSquad();
   //The createGroup methods are called by actualGroupEntities to "add" groups.
   bool                       createGroup(BEntityID eID, BVector center);
   bool                       createGroup(const BEntityIDArray& entityIDs, BVector center);
   bool                       createGroup(BEntityID eID);
   bool                       createGroup(const BEntityIDArray& entityIDs);
   //CleanUp cleans and deallocates everything.
   void                       cleanUp();



   //Variables.
   BDynamicArray<BEntityGroup*> mGroups;
   //Filter variables.
   BEntityID                  mTargetID;
   //FIXME: Worker variables that are here so that we don't realloc the space every time.
   BEntityIDArray             mEntityIDs;
   BEntityIDArray             mCurrentBest;
   BEntityIDArray             mOverallBest;

   //Filters.
   float                      mRadiusSqr;
   bool                       mReset:1;
   bool                       mModifiedCommand:1;
   bool                       mSpeed:1;                     // new filter, group by speed
   bool                       mProtoSquad:1;                // new filter, group by proto squad
   bool                       mFlagCommonParentShortcut:1;  // Do we allow the common parent shortcut?
};
