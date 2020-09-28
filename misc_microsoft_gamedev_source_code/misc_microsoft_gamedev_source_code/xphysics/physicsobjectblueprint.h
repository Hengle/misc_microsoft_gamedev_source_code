//==============================================================================
// physicsobjectblueprint.h
//
// Copyright (c) 2004, Ensemble Studios
//==============================================================================
#ifndef _PHYSICS_OBJECT_BLUEPRINT
#define _PHYSICS_OBJECT_BLUEPRINT

#include "bitarray.h"
//==============================================================================
// class BPhysicsObjectBlueprint
//==============================================================================
class BPhysicsObjectBlueprintOverrides
{
public:
   BPhysicsObjectBlueprintOverrides(void);
   ~BPhysicsObjectBlueprintOverrides(void);

   enum
   {
      cFlagValidShape,
      cFlagValidHalfExtents,
      cFlagValidCollisionFilter,
      cNumberFlags
   };

   //-- copy constructor
   BPhysicsObjectBlueprintOverrides(const BPhysicsObjectBlueprintOverrides &ref);

   //-- assignment operator
   BPhysicsObjectBlueprintOverrides&         operator=(const BPhysicsObjectBlueprintOverrides &ref);

   void              setShapeID( long shapeID )                      { setFlag(cFlagValidShape, true); mShapeID = shapeID; }
   long              getShapeID( void ) const                        { return mShapeID; }

   void              setCollisionFilterInfo( long collisionFilter )  { setFlag(cFlagValidCollisionFilter, true); mCollisionFilterInfo = collisionFilter; }
   long              getCollisionFilterInfo( void ) const            { return mCollisionFilterInfo; }

   void              setHalfExtents( const BVector & halfExtents)    { setFlag(cFlagValidHalfExtents, true); mHalfExtents = halfExtents; }
   const BVector&    getHalfExtents( void ) const                    { return mHalfExtents; }

   void              setFlag( long n, bool v )                       { if (v == true) mFlags.setBit(n); else mFlags.clearBit(n); }
   bool              getFlag( long n ) const                         { if (mFlags.isBitSet(n) > (DWORD)0) return(true); return(false); }
   
   void              reset( void );

protected:
   
   long           mShapeID;
   long           mCollisionFilterInfo;
   BVector        mHalfExtents;
   BBitArray      mFlags;

};

//==============================================================================
// class BPhysicsObjectBlueprint
//==============================================================================
class BPhysicsObjectBlueprint
{
   public:
      
                              BPhysicsObjectBlueprint(void);
                              ~BPhysicsObjectBlueprint(void);
      
      // Filename.
      void                    setFilename(const BCHAR_T *filename) {mFilename=filename;}
      const BSimString           &getFilename(void) const {return(mFilename);}

      // Load/unload.
      bool                    load(long dirID);
      void                    unload(void);

      void                    loadShape();
      void                    unloadShape();

      // Properties.
      float                   getMass(void) const {return(mMass);}
      float                   getFriction(void) const {return(mFriction);}
      float                   getRestitution(void) const {return(mRestitution);}
      const BVector           &getCenterOfMassOffset(void) const {return(mCenterOfMassOffset);}
      float                   getAngularDamping(void) const {return(mAngularDamping);}
      float                   getLinearDamping(void) const {return(mLinearDamping);}
      long                    getShape(void) const {return(mShape);}
      const BVector           &getHalfExtents(void) const {return mHalfExtents; }

   protected:
      void                    reset(void);
   
      BSimString                 mFilename;
      
      float                   mMass;
      float                   mFriction;
      float                   mRestitution;
      BVector                 mCenterOfMassOffset;
      float                   mAngularDamping;
      float                   mLinearDamping;
      long                    mShape;
      BVector                 mHalfExtents;
    

      bool                    mLoaded;
      bool                    mFailedToLoad;
      
      
};


#endif
//==============================================================================
// eof: physicsobjectblueprint.h
//==============================================================================
