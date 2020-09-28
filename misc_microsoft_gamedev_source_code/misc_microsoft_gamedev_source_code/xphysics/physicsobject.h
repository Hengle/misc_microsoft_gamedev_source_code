//==============================================================================
// physicsobject.h
//
// Copyright (c) 2003, Ensemble Studios
//==============================================================================

#ifndef _PHYSICS_OBJECT
#define _PHYSICS_OBJECT

//==============================================================================
// Includes
//#include "common.h"

#include <Physics/Dynamics/Entity/hkpEntityActivationListener.h>
#include <Physics/Dynamics/Collide/hkpCollisionListener.h>
#include "physicslistener.h"
#include "Physics\Dynamics\Phantom\hkpAabbPhantom.h"
#include "gamefilemacros.h"

//==============================================================================
// Forward declarations
class hkpRigidBody;
class BShape;
class BPhysicsObjectBlueprint;
class BPhysicsObjectBlueprintOverrides;
class BPhysicsWorld;
class BConstraint;
class BPhysicsInfo;


class BPhysicsObjectParams
{   
   public:
      BPhysicsObjectParams() :
         position(0.0f, 0.0f, 0.0f),
         centerOffset(0.0f, 0.0f, 0.0f),
         centerOfMassOffset(0.0f, 0.0f, 0.0f),
         shapeHalfExtents(0.5f, 0.5f, 0.5f),
         pHavokShape(NULL),
         restitution(0.0f),
         friction(0.0f),
         angularDamping(0.0f),
         linearDamping(0.0f),
         mass(1.0f),
         collisionFilterInfo(-1),
         userdata(0),
         breakable(false),
         fixed(false)
         {}


      BVector position;
      BPhysicsMatrix rotation;
      BVector centerOffset;
      BVector centerOfMassOffset;
      BVector shapeHalfExtents;
      const hkpShape *pHavokShape;
      float restitution;
      float friction;
      float angularDamping;
      float linearDamping;
      float mass;
      long collisionFilterInfo;
      DWORD userdata;
      bool breakable;
      bool fixed;
};

//==============================================================================
// Const declarations
//   BVector mPosition;
   //BPhysicsMatrix  mRotation;
   //DWORD    mUserData;
//      BVector                 mCenterOfMassOffset;
//==============================================================================
class BPhysicsObject : public hkpCollisionListener, hkpEntityActivationListener
{
public:

   enum ePhysicsObjectType
   {
      cSimple,
      cClamshell,
      cNumPhysicsObjectTypes
   };

   //BPhysicsObject(BPhysicsWorld *world);
   //BPhysicsObject(BPhysicsWorld *world, hkpRigidBody *phkRigidBody, bool deleteOnDestruction = true);
   BPhysicsObject(BPhysicsWorld *world, const BPhysicsObjectBlueprint &blueprint, const BVector &position, const BVector &centerOffset, const BPhysicsMatrix &rotation, 
                                 DWORD userdata, bool fixed, const BPhysicsObjectBlueprintOverrides *pOverrides=NULL);

   BPhysicsObject(BPhysicsWorld *world, const BPhysicsObjectParams &params);
   BPhysicsObject(BPhysicsWorld *world);

   virtual ~BPhysicsObject();

   virtual ePhysicsObjectType getType() const { return cSimple; }

   hkpRigidBody    *getRigidBody(void) const                              { return mphkRigidBody; }
   void           setRigidBody(hkpRigidBody *phkRigidBody);

   virtual bool   addToWorld(void);
   virtual bool   removeFromWorld(void);

   void           setDeleteRigidBodyOnDestruction(bool flag);


   //-- THESE MAY PERFORM COORDINATE SPACE TRANSLATION
   void           getPosition(BVector &pos) const;
   void           getRotation(BPhysicsMatrix &rot) const;

   virtual void   setPosition(const BVector &pos);
   virtual void   setRotation(const BPhysicsMatrix &rot);

   BVector        getCenterOffset() const { return mCenterOffset; }
   void           setCenterOffset(BVector v) { mCenterOffset = v; }

   virtual void   setMass(float fMass);
   float          getMass( void ) const;


   virtual void   setAngularDamping(float damping);
   float          getAngularDamping() const;

   void           setLinearDamping(float damping);
   float          getLinearDamping() const;

   float          getRestitution() const;
   float          getFriction() const;

   void           getLinearVelocity(BVector &velocity) const;
   void           getAngularVelocity(BVector &velocity) const;

   virtual void   setLinearVelocity(const BVector &velocity);
   virtual void   setAngularVelocity(const BVector &velocity);

   void           getCenterOfMassLocation(BVector &pos) const;

   void           applyPointImpulse(const BVector &impulse, const BVector &point);
   virtual void   applyImpulse(const BVector &impulse);
   virtual void   applyAngularImpulse(const BVector& impulse);
   void           applyForce(const BVector& force);
   void           applyForce(const BVector& force, const BVector& point);
   void           applyTorque(const BVector& torque);

   //-- properties and additional data
   void           setProperty(long property, DWORD value);
   virtual void   setKeyframed(bool flag);
   bool           isKeyframed( void ) const;

   // Collision filter
   virtual void   setCollisionFilterInfo(long info);
   virtual long   getCollisionFilterInfo(void) const;
   virtual void   updateCollisionFilter( void );

   //-- activation
   void           forceActivate(void);
   void           forceDeactivate(void);
   bool           isActive( void ) const;
   void           enableDeactivation( bool flag );
   bool           isDeactivationEnabled() const;

   //-- shapes
   void           setShape(BShape &shape, long newID = -1);
   long           getShapeID( void ) const { return mShapeID; }
   virtual void   renderShape(void);

   //-- info
   void           setInfoID(long id) { mInfoID=id; }
   long           getInfoID() const { return mInfoID; }

   // actions
   uint           getNumActions() const { return (mphkRigidBody ? mphkRigidBody->getNumActions() : 0); }
   hkpAction*     getAction(uint i) const { BASSERT((i >= 0) && (i < getNumActions())); return (mphkRigidBody ? mphkRigidBody->getAction(i) : NULL); }

   //-- collision - TODO - get rid of one of these interfaces
   virtual void   addHavokCollisionListener(hkpCollisionListener* pListener);
   virtual void   removeHavokCollisionListener(hkpCollisionListener* pListener);
   bool           addGameCollisionListener(BPhysicsCollisionListener *pListener);
   bool           removeGameCollisionListener(BPhysicsCollisionListener *pListener);
   void           clearAllCollisionListeners(void);

   //-- the hkpCollisionListener Interface
   virtual void contactPointAddedCallback(	hkpContactPointAddedEvent& event);
   virtual void contactPointRemovedCallback( hkpContactPointRemovedEvent& event );
   virtual void contactProcessCallback( hkpContactProcessEvent& event );
   virtual void contactPointConfirmedCallback( hkpContactPointConfirmedEvent& event);

   //-- the hkpEntityActivationListener Interface
   virtual void entityDeactivatedCallback(hkpEntity* entity);
   virtual void entityActivatedCallback(hkpEntity* entity);

   //-- user data
   void*          getUserData(void)          { return mpUserData; }
   void           setUserData(void* data)    { mpUserData = data; }

   bool           isInWorld( void ) const;
   BPhysicsWorld  *getWorld(void) {return(mWorld);}
   
   static BPointerList<BPhysicsObject>& getPhysicsObjectList( void ) { return mPhysicsObjects; }


   bool  raySegmentIntersects(const BVector &origin, const BVector &vector, bool segment, float &intersectDistanceSqr, BVector* pNormal = NULL) const;

   bool  getLoadedKeyframed() const { return mbLoadedKeyframed; }

   GFDECLAREVERSION();
   virtual bool save(BStream* pStream, int saveType) const;
   virtual bool load(BStream* pStream, int saveType);

protected:

   void           dispatchContactPointAddedEvent(	hkpContactPointAddedEvent& event);
   void           dispatchContactPointRemovedEvent( hkpContactPointRemovedEvent& event );
   void           dispatchContactProcessCallbackEvent( hkpContactProcessEvent& event );


   //-- helper functions for building objects
   void           setupObjectRepresentation(const BPhysicsObjectParams &params, bool fromSave=false);

   BVector        mCenterOffset;
   BVector        mCenterOfMassOffset;
   BVector        mShapeHalfExtents;

   BPhysicsWorld  *mWorld;
   hkpRigidBody    *mphkRigidBody;

   BPointerList<BPhysicsCollisionListener>      mCollisionListeners;

   BHandle        mListHandle;
   long           mShapeID;            // Valid only if shape was made from .shp file, else it will be -1.  
                                       // This is used for shape reloading to work.
   long           mInfoID;
   void*          mpUserData;
   bool           mbDeleteRigidBodyOnDestruction;
   bool           mbRegisteredForCollisionCallback;
   bool           mbFixed;
   bool           mbBreakable;
   bool           mbLoadedKeyframed;


   static BPointerList<BPhysicsObject> mPhysicsObjects;
}; 

//==============================================================================
//==============================================================================
class BClamshellPhysicsObject : public BPhysicsObject
{
   public:
      BClamshellPhysicsObject(BPhysicsWorld *world, const BPhysicsObjectBlueprint &upperBlueprint,
                              const BPhysicsObjectBlueprint &lowerBlueprint, const BPhysicsObjectBlueprint &pelvisBlueprint,
                              const BVector &position, const BVector &centerOffset, const BPhysicsMatrix &rotation, 
                              float upperHeightOffset, float lowerHeightOffset, DWORD userdata, bool fixed, const BPhysicsObjectBlueprintOverrides *pOverrides=NULL);
      BClamshellPhysicsObject(BPhysicsWorld *world);
      virtual ~BClamshellPhysicsObject();

      virtual ePhysicsObjectType getType() const { return cClamshell; }

      virtual bool      addToWorld(void);
      virtual bool      removeFromWorld(void);

      virtual void      setPosition(const BVector &pos);
      virtual void      setRotation(const BPhysicsMatrix &rot);

      virtual void      setLinearVelocity(const BVector &velocity);
      virtual void      setAngularVelocity(const BVector &velocity);

      virtual void      applyImpulse(const BVector &impulse);
      virtual void      applyAngularImpulse(const BVector &impulse);
      void              applyPointImpulses(const BVector &upperImpulse, const BVector &lowerImpulse, const BVector &pelvisImpulse, const BVector &pelvisOffset);
      
      virtual void      setMass(float fMass);
      virtual void      setAngularDamping(float damping);
      virtual void      setLinearDamping(float damping);
      virtual void      setKeyframed(bool flag);

      // Collision filter
      virtual void      setCollisionFilterInfo(long info);
      virtual void      updateCollisionFilter( void );

      virtual void      addHavokCollisionListener(hkpCollisionListener* pListener);
      virtual void      removeHavokCollisionListener(hkpCollisionListener* pListener);
      
      void              getClamshellData(BVector& pos, BVector& fwd, BVector& up, BVector& right, float& angle) const;

      virtual void      renderShape(void);

      virtual BPhysicsObject* getUpperBody() { return mpUpperBody; }
      virtual BPhysicsObject* getLowerBody() { return mpLowerBody; }

      virtual bool save(BStream* pStream, int saveType) const;
      virtual bool load(BStream* pStream, int saveType);

   protected:
      void              setupHingeConstraints();

      BPhysicsObject*   mpUpperBody;
      BPhysicsObject*   mpLowerBody;
      BConstraint*      mpUpperHinge;
      BConstraint*      mpLowerHinge;
};

//==============================================================================
//==============================================================================
class BPhantom
{
   public:
      BPhantom(BPhysicsWorld *world, BVector min, BVector max,
                   hkpPhantomOverlapListener* pListener, long collisionFilterInfo, int userdata);
      virtual ~BPhantom();

      void           getAabbMinMax(BVector& min, BVector& max) const;

   protected:
      hkpAabbPhantom*    mpPhantom;
};

#endif
//==============================================================================
// eof: physicsobject.h
//==============================================================================