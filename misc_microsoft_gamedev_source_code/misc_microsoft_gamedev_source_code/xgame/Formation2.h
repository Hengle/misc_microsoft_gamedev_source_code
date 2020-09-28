//==============================================================================
// formation2.h
// Copyright (c) 2005-2007 Ensemble Studios
//==============================================================================

#pragma once
#include "containers\staticArray.h"
#include "SimTypes.h"
#include "SimTarget.h"
#include "ObstructionManager.h"

class BEntity;
class BSquad;
class BPath;


//==============================================================================
//==============================================================================
struct BFormationCachedSquadData
{
public:
   BSquad*  getSquad() const              { return mpSquad; }
   void     setSquad(BSquad* pSquad)      { mpSquad = pSquad; }
   float    getRange() const              { return mRange; }
   void     setRange(float range)         { mRange = range; }
   float    getWidth() const              { return mWidth; }
   void     setWidth(float width)         { mWidth = width; }
   float    getDepth() const              { return mDepth; }
   void     setDepth(float depth)         { mDepth = depth; }

private:
   BSquad* mpSquad;
   float mRange;
   float mWidth;
   float mDepth;
};

//==============================================================================
//==============================================================================
class BFormationPosition2
{
   public:
      enum
      {
         cInvalidPriority=10000
      };
      
      BFormationPosition2() { }
      BFormationPosition2(const BFormationPosition2 &pos) 
                                       {
                                          mEntityID=pos.mEntityID;
                                          mPriority=pos.mPriority;
                                          mDynamicPriority=pos.mDynamicPriority;
                                          mPosition=pos.mPosition;
                                          mOffset=pos.mOffset;
                                          mOriginalLineIndex=pos.mOriginalLineIndex;
                                          mActualLineIndex=pos.mActualLineIndex;
                                          mTransform=pos.mTransform;
                                          mFlockVelocity=pos.mFlockVelocity;
                                       };
      ~BFormationPosition2() { }

      BEntityID                        getEntityID() const { return (mEntityID); }
      void                             setEntityID(BEntityID v) { mEntityID=v; }
      uint                             getPriority() const { return (mPriority); }
      void                             setPriority(uint v) { mPriority=(uint16)v; }
      uint                             getDynamicPriority() const { return (mDynamicPriority); }
      void                             setDynamicPriority(uint v) { mDynamicPriority=(uint16)v; }
      uint                             getOriginalLineIndex() const { return (mOriginalLineIndex); }
      void                             setOriginalLineIndex(uint v) { mOriginalLineIndex=(uint16)v; }
      uint                             getActualLineIndex() const { return (mActualLineIndex); }
      void                             setActualLineIndex(uint v) { mActualLineIndex=(uint16)v; }
      BVector                          getOffset() const { return (mOffset); }
      void                             setOffset(BVector v) { mOffset=v; }
      void                             adjustOffset(BVector v) { mOffset+=v; }
      BVector                          getPosition() const { return (mPosition); }
      void                             setPosition(BVector v) { mPosition=v; }
      BVector                          getFlockVelocity() const { return (mFlockVelocity); }
      void                             setFlockVelocity(BVector v) { mFlockVelocity=v; }

      bool                             getTransform() const { return (mTransform); }
      void                             setTransform(bool v) { mTransform=v; }

      void                             reset()
                                       {
                                          mEntityID.invalidate();
                                          mPriority=cInvalidPriority;
                                          mDynamicPriority=cInvalidPriority;
                                          mOffset=cInvalidVector;
                                          mPosition=cInvalidVector;
                                          mFlockVelocity=cOriginVector;
                                          mOriginalLineIndex=0;
                                          mActualLineIndex=0;
                                          mTransform=true;
                                       }

      bool save(BStream* pStream, int saveType) const;
      bool load(BStream* pStream, int saveType);

   protected:
      BVector                          mOffset;
      BVector                          mPosition;
      BVector                          mFlockVelocity;
      BEntityID                        mEntityID;
      uint16                           mPriority;
      uint16                           mDynamicPriority;
      uint16                           mOriginalLineIndex;
      uint16                           mActualLineIndex;
      bool                             mTransform:1;
};
typedef BSmallDynamicSimArray<BFormationPosition2> BFormationPosition2Array;



//==============================================================================
//==============================================================================
class BFormationPriorityEntry
{
   public:
      BFormationPriorityEntry() { }
      ~BFormationPriorityEntry() { }

      BEntityID                        getEntityID() const { return (mEntityID); }
      void                             setEntityID(BEntityID v) { mEntityID=v; }
      float                            getPercentComplete() const { return (mPercentComplete); }
      void                             setPercentComplete(float v) { mPercentComplete=v; }

      void                             reset()
                                       {
                                          mEntityID.invalidate();
                                          mPercentComplete=0.0f;
                                       }
   protected:
      BEntityID                        mEntityID;
      float                            mPercentComplete;
};
typedef BSmallDynamicSimArray<BFormationPriorityEntry> BFormationPriorityEntryArray;



//==============================================================================
//==============================================================================
class BFormation2 : public IPoolable
{
   public:
      enum
      {
         eTypeStandard=0,
         eTypeFlock,
         eTypeGaggle,
         eTypeLine,
         eTypeAttack,
         eTypePlatoon,
         eTypeMob,
         eNumberTypes
      };

   
      BFormation2() { }
      virtual ~BFormation2() { }

      virtual void                     onAcquire() { }
      virtual void                     onRelease() { }
      virtual bool                     init();

      //ID.
      uint                             getID() const { return (mID); }
      void                             setID(uint v) { mID=v; }
      //Owner.
      BEntity*                         getOwner() { return (mpOwner); }
      void                             setOwner(BEntity* v) { mpOwner=v; }
      //Type.
      BFormationType                   getType() const { return (mType); }
      void                             setType(BFormationType v) { mType=v; }

      //Child management.
      bool                             addChild(BEntityID childID);
      bool                             removeChild(BEntityID childID);
      bool                             addChildren(const BEntityIDArray &childrenIDs);
      bool                             removeChildren();
      uint                             getDefaultChildPriority(BEntityID childID) const;
      uint                             getChildPriority(BEntityID childID) const;
      BEntityID                        getChildByPriority(uint priority) const;

      //Size.
      float                            getRadius() const { if (mRadiusX > mRadiusZ) return (mRadiusX); return (mRadiusZ); }
      float                            getRadiusX() const { return(mRadiusX); }
      float                            getRadiusZ() const { return(mRadiusZ); }
      //Scale
      void                             scale(float v);
      //ForwardAmount.
      float                            getForwardAmount() const;
      // Ratio
      float                            getLineRatio() const { return (mLineRatio); }
      void                             setLineRatio(float ratio) { mLineRatio = ratio; }

      //Formation Creation.
      bool                             getMakeNeeded() const { return (mMakeNeeded); }
      void                             setMakeNeeded(bool v) { mMakeNeeded=v; }
      bool                             getReassignNeeded() const { return (mReassignNeeded); }
      void                             setReassignNeeded(bool v) { mReassignNeeded=v; }
      bool                             getUseRandom() const { return (mUseRandom); }
      void                             setUseRandom(bool v) { mUseRandom = v; }
      void                             makeFormation();
      void                             makeStandardFormation();
      void                             makeLineFormation();
      bool                             makeAttackFormation(BSimTarget target);
      void                             makePlatoonLineFormation();
      void                             makeMobFormation();

      //Positions.
      void                             update(BMatrix matrix, const BPath *path=NULL);
      uint                             getNumberPositions() const { return (mPositions.getSize()); }
      const BFormationPosition2*       getFormationPosition(uint index) const; 
      const BFormationPosition2*       getFormationPosition(BEntityID childID) const; 
      BVector                          getTransformedFormationPositionFast(BEntityID childID, BVector formationPos, BVector formationForward, BVector formationRight) const;

      //Dynamic priorities.
      void                             calculateDynamicPriorities(BVector forward);
     
      //Misc.
      virtual void                     debugRender() const;
      DECLARE_FREELIST(BFormation2, 4);

      GFDECLAREVERSION();
      bool save(BStream* pStream, int saveType) const;
      bool load(BStream* pStream, int saveType);

   protected:
      void                             prioritizePositions();
      void                             reassignChildren();
      void                             reassignPlatoonChildren();
      void                             adjustPlatoonFormationOffsets();
      BEntityID                        findClosest(BEntityIDArray& entities, BVector position);
      void                             calculateLineDimensions(float totalWidth, float totalDepth, float ratio, int& nL, float& lineWidth) const;
      void                             setDynamicPriority(BEntityID childID, uint priority);
      void                             transform(BMatrix matrix);
      BVector                          computeFlockingForce(long formationPosIndex, BVector position, BVector velocity, BEntityIDArray &ignoreList, BVector formationPos, BVector formationDir, BVector spinePoint);
      void                             resetComputeForceObstructions() { mComputeForceObstructions.resize(0); mGetObstructions = true; }
      float                            computePositionErrorSqr();
      
      BVector                          mPrevForward;
      BEntityIDArray                   mChildren;
      BFormationPosition2Array         mPositions;
      BEntity*                         mpOwner;
      uint                             mID;
      float                            mRadiusX;
      float                            mRadiusZ;
      float                            mLineRatio;
      BFormationType                   mType;
      DWORD                            mReassignTimer;
      //float                            mPositionErrorAtLastAssignmentSqr;
      BObstructionNodePtrArray         mComputeForceObstructions;
      bool                             mGetObstructions;
      
      bool                             mMakeNeeded:1;
      bool                             mReassignNeeded:1;
      bool                             mDynamicPrioritiesValid:1;
      bool                             mUseRandom:1;
};
