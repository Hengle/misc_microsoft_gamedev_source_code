//==============================================================================
// DesignObjectmanager.h
//
// DesignObjectmanager manages all DesignObjects
//
// Copyright (c) 2006 Ensemble Studios
//==============================================================================
#pragma once


// Constants
const long cDesignSphereBlockSize=100;
const long cDesignLineBlockSize=100;


//=================================
// BDesignObjectData
//
// BDesignObjectData class
//=================================
class BDesignObjectData 
{
   public:      
      enum
      {
         cPhysicsLine=999,
         cTerrainCollisionsToggleLine=998,
         cOneWayBarrierLine=997,
         cCoreSlideLine=995,
      };
      
      BDesignObjectData();
      ~BDesignObjectData() {}

      void                       load(BXMLNode  xmlNode);
      bool                       getValuesAsVector(uint index, BVector& result);
      bool                       isPhysicsLine() const;
      bool                       isTerrainCollisionToggleLine() const;
      bool                       isOneWayBarrierLine() const;

      float mValues[12];  //Yes, we need to clean this up:)
      float mAttract;
      float mRepel;
      float mChokePoint;
      float mValue;

      BSimString mType;
};

//=================================
// BDesignSphere
//
// DesignObject class
//=================================
class BDesignSphere 
{
   public:

      BDesignSphere();
      ~BDesignSphere() {}

      void                       load(BXMLNode  xmlNode);

      virtual void               onAcquire() {}
      virtual void               onRelease() {}
      DECLARE_FREELIST(BDesignSphere, 5);

      BDesignObjectData          mDesignData;
      long                       mID;
      BVector                    mPosition;
      float                      mRadius;
};

//=================================
// BDesignLine
//
// DesignObject class
//=================================
class BDesignLine 
{
   public:

      BDesignLine();
      ~BDesignLine() {}

      void                       load(BXMLNode  xmlNode);
      bool                       intersects(BVector point1, BVector point2, BVector& intersectionPoint);
      bool                       incidenceIntersects(BVector forward, BVector point1, BVector point2, BVector& intersectionPoint);
      bool                       imbeddedIncidenceIntersects(BVector forward, BVector point1, BVector point2, BVector& intersectionPoint);
      void                       closestPointToLine(BVector point1, BVector direction, BVector& closestPoint);

      virtual void               onAcquire() {}
      virtual void               onRelease() { mPoints.clear(); }
      DECLARE_FREELIST(BDesignLine, 5);

      BDesignObjectData          mDesignData;
      BLocationArray             mPoints;
      BDesignLineID              mID;
};

//==========================================
// BDesignObjectManager
//
// Management and access to game DesignObjects
//==========================================
class BDesignObjectManager
{
   public:

      BDesignObjectManager();
      virtual ~BDesignObjectManager();      

      // Management functions 
      bool                       init();
      void                       reset();

      void                       load(BXMLNode  xmlNode);

      uint                       getDesignSphereCount() { return mDesignSpheres.size(); }
      BDesignSphere&             getDesignSphere(uint index) { return *(mDesignSpheres.get(index)); }

      uint                       getDesignLineCount() { return mDesignLines.size(); }
      BDesignLine&               getDesignLine(uint index) { return *(mDesignLines.get(index)); }
      BDesignLine&               getDesignLine(BDesignLineID id);
      bool                       intersectsPhysicsLine(BVector forward, BVector point1, BVector point2, BVector& intersectionPoint) const;

   protected:
      long                       getIndexFromID(long ID);

      BDynamicArray<BDesignSphere*>    mDesignSpheres;
      BDynamicArray<BDesignLine*>      mDesignLines;
};