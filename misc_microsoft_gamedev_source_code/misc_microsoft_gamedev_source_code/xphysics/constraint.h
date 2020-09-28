//==============================================================================
// constraint.h
//
// Copyright (c) 2003, 2004 Ensemble Studios
//==============================================================================



#ifndef _CONSTRAINT_H_
#define _CONSTRAINT_H_

//==============================================================================
// Includes

 #include <Physics/Dynamics/Constraint/Breakable/hkpBreakableListener.h>



//==============================================================================
// Forward declarations
class BPhysicsObject;
class hkpBreakableConstraintEvent;
class BPhysicsWorld;

//==============================================================================
// Const declarations


class BLimitedHingeBlueprint
{
public:
    BPhysicsObject   *mpObjectA; 
    BPhysicsObject   *mpObjectB; 
    BVector         mAxis;
    BVector         mPivot;
    float            mMinAngle;
    float            mMaxAngle;
    float            mFriction;
};

//==============================================================================
class BConstraint : public hkpBreakableListener
{
public:

   //-- constraint types
   enum
   {
      cConstraintTypeUnknown = -1,
      cConstraintTypeHinge,
      cConstraintTypeLimitedHinge,
      cConstraintTypePrismatic,
      cConstraintTypeWorld,
      cConstraintTypeRigid,
      cConstraintTypeBallAndSocket,
      cConstraintTypeCount,
   };

   BConstraint(BPhysicsWorld *world);

   BConstraint(BPhysicsWorld *world, hkpConstraintInstance *pConstraint);

   virtual ~BConstraint( void );

   //-- accessors
   hkpConstraintInstance *        getHavokConstraint( void )  { return mphkConstraint; }
   void                          setHavokConstraint(hkpConstraintInstance *pConstraint);


   //-- memory management
   void                 addReference(void);
   void                 releaseHavokConstraint(void);

   //-- world funcs
   bool                 addToWorld();
   bool                 removeFromWorld();

   //-- creation and allocation functions
   bool                 allocateHingeConstraint(const BPhysicsObject &objectA, const BPhysicsObject &objectB, const BVector &axis, const BVector& pivot);
   bool                 allocateLimitedHingeConstraint(const BLimitedHingeBlueprint &bp);
   bool                 allocatePrismaticConstraint(const BPhysicsObject *objectA, const BPhysicsObject *objectB, const BVector &axis, const BVector &pivot, float max, float min);
   bool                 allocateWorldConstraint(const BPhysicsObject &objectA);
   bool                 allocateRigidConstraint(const BPhysicsObject &objectA, const BPhysicsObject &objectB);

   bool                 allocateBallAndSocket(const BPhysicsObject &objectA, const BVector &point);

   bool                 makeBreakable(float breakForce, bool bRemoveWhenBroken);
   
   void                 setBreakableListener(IPhysicsEventObserver *pListener)                    { mpListener = pListener; }

   //-- breakable listener interface
   virtual void         constraintBrokenCallback (hkpBreakableConstraintEvent &event);

protected:

   hkpConstraintInstance*         mphkConstraint;

   long                          mType;
   IPhysicsEventObserver*        mpListener;
   BPhysicsWorld                 *mWorld;
};





//==============================================================================
#endif // _CONSTRAINT_H_

//==============================================================================
// eof: constraint.h
//==============================================================================