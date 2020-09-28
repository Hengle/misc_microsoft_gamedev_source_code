//============================================================================
//
//  terraintraildecal.cpp
//  
//  Copyright (c) 2006, Ensemble Studios
//
//============================================================================
#pragma once

//game
#include "visiblemap.h"
#include "TerrainSimRep.h"
#include "usermanager.h"
#include "user.h"
#include "world.h"
#include "config.h"
#include "configsgame.h"

// terrain
#include "terrainribbon.h"



//============================================================================
// BTerrainRibbonGenerator
// This class represents a 'generator' that can create new ribbons on the fly.
// Overall this wraps up functionality to help users manage ribbons seamlessly
//============================================================================
class BTerrainTrailDecalGenerator
{
public:
   BTerrainTrailDecalGenerator():
      mCurrRibbonHandle(cInvalidBTerrainRibbonHandle)
      {

      }

      bool isSameParams(BRibbonCreateParams parms)
      {
         return mLastParms.equalTo(parms);
      }

      void addPointToTrail(float x, float z)
      {

#ifndef BUILD_FINAL
         if(gConfig.isDefined(cConfigDisableTrails))
         {
            endTrail();
            return;
         }
#endif

         if(mCurrRibbonHandle == cInvalidBTerrainRibbonHandle)
         {
            startTrail(mLastParms);
         }

         //if this point is as close as the last point, don't do it.
         if(mLastPoint.x != UINT_MAX  && mLastPoint.x != UINT_MAX )
         {
            BVector2 np(x,z);
            BVector2 diff = mLastPoint - np;
            float len = Math::fSqrt(diff.x * diff.x + diff.y * diff.y + diff.z * diff.z);
            if(len < mLastParms.mMinDist)
               return;
         }
         
         mLastPoint.x = x;
         mLastPoint.y = z;
         


         //if this point is under FOW, stop the strip
         BTeamID teamID = gUserManager.getUser(BUserManager::cPrimaryUser)->getTeamID();
         XMVECTOR checkPos = XMVectorSet(x,0,z,1);
         XMVECTOR simPosition = __vctsxs(XMVectorMultiply(checkPos, XMVectorReplicate(gTerrainSimRep.getReciprocalDataTileScale())), 0);
         long simX = simPosition.u[0];
         long simZ = simPosition.u[2];         
         if(gWorld->getFlagAllVisible() == false)
         {
            if (simX>=0 && simZ>=0 && simX<gVisibleMap.getMaxXTiles() && simZ<gVisibleMap.getMaxZTiles())
            {
               if (!(gVisibleMap.getVisibility(simX, simZ) & gVisibleMap.getTeamFogOffMask(teamID)))
               {
                  endTrail();
                  return;
               }
            }
         }


         gTerrainRibbonManager.addPointToRibbon(mCurrRibbonHandle,x,z,0,0);
      }

      void addPausePointToTrail()
      {
#ifndef BUILD_FINAL
         if(gConfig.isDefined(cConfigDisableTrails))
         {
            endTrail();
            return;
         }
#endif

         if(mCurrRibbonHandle == cInvalidBTerrainRibbonHandle)
            return;

         endTrail();
      }

      void startTrail(BRibbonCreateParams parms)
      {
#ifndef BUILD_FINAL
         if(gConfig.isDefined(cConfigDisableTrails))
         {
            endTrail();
            return;
         }
#endif

         if(mCurrRibbonHandle != cInvalidBTerrainRibbonHandle)
         {
            if(isSameParams(parms))
               return;  //we're dealing with the same trail that we've had..

            endTrail();
         }

         mCurrRibbonHandle = gTerrainRibbonManager.createRibbon(parms);
         parms.copyTo(mLastParms);
         mLastPoint.x = UINT_MAX;
         mLastPoint.y = UINT_MAX;
      }
      void endTrail()
      {
         if(mCurrRibbonHandle == cInvalidBTerrainRibbonHandle)
            return;

         gTerrainRibbonManager.stopRibbon(mCurrRibbonHandle);

         mCurrRibbonHandle = cInvalidBTerrainRibbonHandle;
         mLastPoint.x = UINT_MAX;
         mLastPoint.y = UINT_MAX;
      }

private:
   BTerrainRibbonHandle mCurrRibbonHandle;
   BRibbonCreateParams mLastParms;
   BVector2 mLastPoint;
   static const int cLastPointInvalid = UINT_MAX;

};

