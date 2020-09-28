//==============================================================================
// unitactionairtrafficcontrol.h
// Copyright (c) 2006-2007 Ensemble Studios
//==============================================================================

#pragma once
#include "action.h"

#define MAX_LANDING_SPOTS 8
//==============================================================================
// BLandingSpot
//==============================================================================
class BLandingSpot
{
public:
   void           init() { worldPos=cInvalidVector; forward.zero(); forward.x = 1.0f; aircraftID=cInvalidObjectID; };

   bool save(BStream* pStream, int saveType) const;
   bool load(BStream* pStream, int saveType);

   BVector        worldPos;
   BVector        forward;
   BEntityID      aircraftID;
};

//==============================================================================
//==============================================================================
class BUnitActionAirTrafficControl : public BAction
{
   public:
      BUnitActionAirTrafficControl() { }
      virtual ~BUnitActionAirTrafficControl() { }

      //Connect/disconnect.
      virtual bool               connect(BEntity* pOwner, BSimOrder* pOrder);
      virtual void               disconnect();

      //Init.
      virtual bool               init();

      virtual void               notify(DWORD eventType, BEntityID senderID, DWORD data, DWORD data2);   
      virtual bool               update(float elapsed);

      BVector                    requestLandingSpot(BUnit* aircraft, BVector& forward); // return cInvalidVector if request denied
      void                       releaseSpot(BUnit* aircraft);

      DECLARE_FREELIST(BUnitActionAirTrafficControl, 5);

      virtual bool save(BStream* pStream, int saveType) const;
      virtual bool load(BStream* pStream, int saveType);

   protected:

      void                       initSpotCoords();
      void                       releaseAllAircraft(); // If the controller unit is destroyed, it needs to notify its aircraft that it is no longer available

      BLandingSpot               mSpot[MAX_LANDING_SPOTS];
};