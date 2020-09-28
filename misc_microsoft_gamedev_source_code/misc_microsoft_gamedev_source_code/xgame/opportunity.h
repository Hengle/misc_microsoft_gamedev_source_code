//==============================================================================
// opportunity.h
//
// Copyright (c) 2007 Ensemble Studios
//==============================================================================
#pragma once

//-- Forward Declarations
class BOpportunity;
class BOpportunityList;
class BEntity;
class BProtoAction;
class BSquad;
class BUnit;
class BTactic;
class BPlayer;
class BOpportunityResult;
typedef BDynamicSimArray <BOpportunity> BOpportunityArray;

//==============================================================================
//==============================================================================
class BDefaultSquadBehavior
{
   public:

      void findOpportunities(BOpportunityList* pList, BEntity* pEntity, bool isStrafing);
      void calculatePriorities(BOpportunityList* pList, BEntity* pEntity);

      void findAttackOpportunities(BOpportunityList* pList, BSquad* pSquad, BUnit* pSampleUnit, BTactic* pTactic, BPlayer* pPlayer, bool isStrafing);
      void findGatherOpportunities(BOpportunityList* pList, BSquad* pSquad, BUnit* pSampleUnit, BTactic* pTactic, BPlayer* pPlayer);
      void findAutoRepairOpportunities(BOpportunityList* pList, BSquad* pSquad, BUnit* pSampleUnit, BTactic* pTactic, BPlayer* pPlayer);

      BProtoAction* getBeamProtoAction(BEntity* pEntity);
      bool getLaunchInfo(BEntity* pEntity, BProtoAction* protoAction, BVector& position, BVector& forward);

      // Temp sorting stuff
      static BDynamicSimArray<BOpportunityResult>  mTempOpportunityResults;
      static BDynamicSimUIntArray                  mTempOpportunityOrder;
};
extern BDefaultSquadBehavior* gDefaultSquadBehavior;

//==============================================================================
//==============================================================================
class BOpportunity
{
public:

   enum 
   {
      eOpportunityTypeNone = 0,
      eOpportunityTypeAttack,                //-- Attack target in range      
      eOpportunityTypeAttackSecondary,       //-- Secondary attack the unit can store, so it can pick it's secondary attack when it looses its primary attack. (No Latency)
      eOpportunityTypeAttackMove,            //-- Exactly what it says
      eOpportunityTypeMove,                  //-- Move:)
      eOpportunityTypeGather,                //-- Gather nearby resources
      eOpportunityTypeMax,
      eOpportunityTypeAutoRepair,
   };

   BOpportunity() :  mTargetPosition(cInvalidVector), 
                     mTargetID(cInvalidObjectID), 
                     mpProtoAction(NULL), 
                     mMovementDistance(0.0f), 
                     mPriority(0), 
                     mType(eOpportunityTypeNone)
                     {}

   ~BOpportunity(){}   

    BOpportunity& operator=(const BOpportunity& rhs)
      {
         mTargetPosition=rhs.mTargetPosition;
         mTargetID=rhs.mTargetID;
         mType=rhs.mType;         
         mPriority=rhs.mPriority;
         mpProtoAction=rhs.mpProtoAction;
         mMovementDistance=rhs.mMovementDistance;
         return *this;
      }

   bool save(BStream* pStream, int saveType) const;
   bool load(BStream* pStream, int saveType);

   BVector        mTargetPosition;
   BEntityID      mTargetID;
   BProtoAction*  mpProtoAction;
   float          mMovementDistance;
   float          mPriority;
   uint8          mType;
};

//==============================================================================
//==============================================================================
class BOpportunityList
{
public:
   BOpportunityList();
   ~BOpportunityList();

   void                    setBehavior(BEntity* pOwner);
   void                    findOpportunities(BEntity* pEntity, bool isStrafing);
   void                    calculatePriorities(BEntity* pEntity);
   const BOpportunity&     getOpportunity(uint index) {return mOpportunities.get(index);}
   uint                    getNumOpportunities() const {return mOpportunities.getNumber();}
   bool                    getTopOpportunity(BOpportunity &opportunity);

   //-- Accesor methods for behavior classes
   void                    addOpportunity(BEntityID targetID, BVector targetPosition, uint8 opportunityType, BProtoAction* pAction, float movementDistance);
   void                    clearOpportunities();
   void                    setPriority(uint index, float priority);
   void                    setTopOpportunityIndex(int32 index) {mTopOpportunityIndex = index;}

   // Frees memory used by this class
   void                    deinit();
   
   bool save(BStream* pStream, int saveType) const;
   bool load(BStream* pStream, int saveType);

private:
   BOpportunityArray                mOpportunities;
   BDefaultSquadBehavior*           mEntityBehavior;
   int32                            mTopOpportunityIndex;
};


//==============================================================================
//==============================================================================
class BOpportunityResult
{
   public:
      //Constructors and Destructor.
      BOpportunityResult( void ) {}
      BOpportunityResult( uint index, float priority, float distance, bool currentTarget ) : mIndex(index), mPriority(priority), mDistance(distance), mCurrentTarget(currentTarget) {}
      ~BOpportunityResult( void ) {}

      //Index
      uint                       getIndex( void ) const { return(mIndex); }
      void                       setIndex( uint v ) { mIndex=v; }
      //Priority
      float                      getPriority( void ) const { return(mPriority); }
      void                       setPriority( float v ) { mPriority=v; }
      //Distance
      float                      getDistance( void ) const { return(mDistance); }
      void                       setDistance( float v ) { mDistance=v; }
      //Current target
      float                      getCurrentTarget( void ) const { return(mCurrentTarget); }
      void                       setCurrentTarget( bool v ) { mCurrentTarget=v; }

      XMFINLINE bool             operator<(const BOpportunityResult &oppResultToCompare) const;

   protected:

      uint                       mIndex;
      float                      mPriority;
      float                      mDistance;
      bool                       mCurrentTarget;
};
