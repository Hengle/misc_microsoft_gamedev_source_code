//==============================================================================
// Copyright (c) 1997-2000 Ensemble Studios
//==============================================================================
#pragma once

//==============================================================================
//Includes
#include "bitarray.h"
#include "gamefilemacros.h"

//==============================================================================
//Forward Declarations
class BEntity;
class BChunkWriter;
class BChunkReader;


//==============================================================================
// BPath Class
//==============================================================================
class BPath
{
   public:

      //Path results -- actually used by path finding, not by path itself.
      enum

      {
         cNone=-1,
         cError=0,
         cFailed,
         cInRangeAtStart,
         cPartial,
         cFull,
         cOutsideHulledAreaFailed,     
         cOutsideHulledAreaPartial,
         cReachedHull,                    // Interanl use only
         cFailedOutOfTime                 // Internal use only
      };

      enum
      {
         cPatrol                 = 0x01,
         cLinear                 = 0x02,
         cGoingForward           = 0x04,
         cIgnoredUnits           = 0x08,
         cIgnoredPassability     = 0x10,
         cLengthValid            = 0x20,  // Has length been updated.
         cJump                   = 0x40,
         cNumberPathFlags        = 7
      };

      BPath( long numberFlags = cNumberPathFlags);
      virtual ~BPath( void );

      const BPath&            operator=(const BPath& path);

      //Start and goal.
      const BVector&          getStart( void ) const { if (mWaypoints.getNumber() >= 2) return(mWaypoints[0]); return(cOriginVector); }
      const BVector&          getGoal( void ) const { if (mWaypoints.getNumber() >= 2) return(mWaypoints[mWaypoints.getNumber()-1]); return(cOriginVector); }

      //Waypoint methods.
      long                    getNumberWaypoints( void ) const { return(mWaypoints.getNumber()); }
      const BVector*          getWaypoints( void ) const { return(mWaypoints.getPtr()); }
      const BDynamicVectorArray& getWaypointList( void ) const { return(mWaypoints); }
      const BVector&          getWaypoint( long wpIndex ) const;
      long                    getWaypointIndex( const BVector& ) const;
      bool                    addWaypointAtStart( const BVector& wp );
      bool                    addWaypointBefore( long wpIndex, const BVector& wp );
      bool                    addWaypointAfter( long wpIndex, const BVector& wp );
      bool                    addWaypointAtEnd( const BVector& wp );
      bool                    addWaypointsAtEnd( const BDynamicVectorArray& wpArray );
      bool                    splitSegment( long segmentNumber );
      bool                    setWaypoint( long wpIndex, const BVector& wp );
      bool                    setWaypoints( const BVector* wps, long numWps );
      bool                    removeWaypoint( long wpIndex );
      bool                    removeWaypoint( const BVector& wp );
      void                    zeroWaypoints( void ) { mWaypoints.setNumber(0); }
      void                    offsetWaypoints( const BVector& o, long startIndex );

      //Distance methods.
      float                   getPathLength( void ) const { return(mPathLength); }
      float                   calculatePathLength( bool ignoreY );
      float                   calculatePathLengthConst(bool ignoreY) const;
      float                   calculateRemainingDistance( const BVector& point, long currentWaypoint, bool ignoreY ) const;
      float                   calculateDistanceFromEnd( const BVector& point, bool ignoreY ) const;
      bool                    calculatePointAlongPath(float distance, long startWaypoint, BVector& newPoint) const;
      bool                    calculatePointAlongPath(float distance, const BVector& startPos, long nextWaypoint, BVector& newPoint, float& realDistance, long& newNextWaypoint) const;
      bool                    calculatePointAlongPath(float distance, const BVector& startPos, long nextWaypoint, BDynamicVectorArray& newPoints, float& realDistance) const;
      bool                    calculatePointAlongPathFromEnd(float distance, BVector& newPoint) const;

      //Misc
      long                    findBestStartWaypointIndex(const BVector& location) const;
      BVector                 findClosestPoint(const BVector p) const;

      //Creation Time.
      DWORD                   getCreationTime( void ) const { return(mCreationTime); }
      void                    setCreationTime( DWORD v ) { mCreationTime=v; }

      //Flags.
      bool                    getFlag( long n ) const;
      void                    setFlag( long n, bool v );

      //Logical methods.
      long                    getNextPatrolWaypoint( long currentWaypoint, bool modifyPath );

      //Debug.
      void                    debugList( BEntity* e );
      void                    sync(void);
      //Render.
      bool                    render(DWORD lineColor=cDWORDGreen, DWORD pointColor=cDWORDWhite, bool drawOverTerrain=true);
      //Reset.
      void                    reset(bool releaseMemory=false);

      static const char       *getPathResultName(int nPathResult);

      GFDECLAREVERSION();
      bool save(BStream* pStream, int saveType) const;
      bool load(BStream* pStream, int saveType);

   protected:
      BDynamicVectorArray     mWaypoints;       // 16 bytes
      float                   mPathLength;      // 4 bytes
      DWORD                   mCreationTime;    // 4 bytes
      uchar                   mFlags;           // 1 byte

};