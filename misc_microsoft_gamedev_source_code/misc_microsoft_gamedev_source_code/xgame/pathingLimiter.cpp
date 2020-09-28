//==============================================================================
// pathingLimiter.cpp
//
// Copyright (c) 2001-2007, Ensemble Studios
//==============================================================================

// Includes
#include "common.h"
#include "pathingLimiter.h"
#include "world.h"

GFIMPLEMENTVERSION(BPathingLimiter, 2);

//==============================================================================
// 
//==============================================================================
void BPathingLimiter::initPlayerPathHist()
{
   for (int i=0; i<PATHLIM_MAX_PLAYERS; i++)
   {
      mPlayerHist[i].platoonPathsDeniedThisFrame = 0;
      mPlayerHist[i].platoonDeniedFrames = 0;
      mPlayerHist[i].platoonMaxDeniedFrames = 0;
      mPlayerHist[i].squadPathsDeniedThisFrame = 0;
      mPlayerHist[i].squadDeniedFrames = 0;
      mPlayerHist[i].squadMaxDeniedFrames = 0;
      mPlayerHist[i].denyPathing = false;
   }

   mbPathHistInitialized = true;
}

void BPathingLimiter::prioritizePathing()
{
   /* revisit this later
   uint  mostFramesDenied = 0;
   int  mostDelayedPlayer = -1;

   // Open pathing to all players
   // Check whether any human players were denied platoon paths for more than one frame
   for (int i=0; i < gWorld->getNumberPlayers(); i++)
   {
      if (!gWorld->getPlayer(i))
         break;

      mPlayerHist[i].denyPathing = false;
      
      if (gWorld->getPlayer(i)->isHuman())
      {
         if (mPlayerHist[i].framesDenied > 0)
         {
            if (mPlayerHist[i].framesDenied > mostFramesDenied)
            {
               mostFramesDenied = mPlayerHist[i].framesDenied;
               mostDelayedPlayer = i;
            }
         }
      }
   }

   // ...If so, deny all AI player path requests on next frame.
   // Also deny other players if the most delayed player has waited at least n frames (n=5?)
   for (int i=0; i < gWorld->getNumberPlayers(); i++)
   {
      if (!gWorld->getPlayer(i))
         break;

      if (mostFramesDenied > 1 && !gWorld->getPlayer(i)->isHuman())
         mPlayerHist[i].denyPathing = true;

      if ((mostFramesDenied > 5) && (i != mostDelayedPlayer))
         mPlayerHist[i].denyPathing = true;
   }
   */
}

void BPathingLimiter::resetPathsDeniedLastFrame()
{
   for (int i=0; i<PATHLIM_MAX_PLAYERS; i++)
   {
      // If we were denied at least one path this frame, then increment denied frames..
      if (mPlayerHist[i].platoonPathsDeniedThisFrame > 0)
      {
         mPlayerHist[i].platoonDeniedFrames++;
      }
      else
      {
         // otherwise, see if max deniedFrames needs to be updated, and then reset deniedFrames
         if (mPlayerHist[i].platoonDeniedFrames > mPlayerHist[i].platoonMaxDeniedFrames)
            mPlayerHist[i].platoonMaxDeniedFrames = mPlayerHist[i].platoonDeniedFrames;
         mPlayerHist[i].platoonDeniedFrames = 0;
      }
      // Regardless, reset paths denied this frame.
      mPlayerHist[i].platoonPathsDeniedThisFrame = 0;

      // Now do the same for squads..
      if (mPlayerHist[i].squadPathsDeniedThisFrame > 0)
      {
         mPlayerHist[i].squadDeniedFrames++;
      }
      else
      {
         // otherwise, see if max deniedFrames needs to be updated, and then reset deniedFrames
         if (mPlayerHist[i].squadDeniedFrames > mPlayerHist[i].squadMaxDeniedFrames)
            mPlayerHist[i].squadMaxDeniedFrames = mPlayerHist[i].squadDeniedFrames;
         mPlayerHist[i].squadDeniedFrames = 0;
      }
      // Regardless, reset paths denied this frame.
      mPlayerHist[i].squadPathsDeniedThisFrame = 0;
   }
}

void BPathingLimiter::resetFramePathingCounts()
{
   mNumPlatoonLRPperFrame = 0;
   mNumPlatoonSRPperFrame = 0;
   mNumSquadSRPperFrame = 0;
   resetPathsDeniedLastFrame();

   // DLM - assume this is called once per frame, so use it to increment our number of frames.
   mNumPathingFrames++;
}


float BPathingLimiter::getAvePlatoonLRPperFrame()
{
   return(mNumPathingFrames ? (static_cast<float>(mNumPlatoonLRPs)/static_cast<float>(mNumPathingFrames)) : 0.0f);
}

float BPathingLimiter::getAvePlatoonSRPperFrame()
{
   return(mNumPathingFrames ? (static_cast<float>(mNumPlatoonSRPs)/static_cast<float>(mNumPathingFrames)) : 0.0f);
}

float BPathingLimiter::getAveSquadSRPperFrame()
{
   return(mNumPathingFrames ? (static_cast<float>(mNumSquadSRPs)/static_cast<float>(mNumPathingFrames)) : 0.0f);
}


int BPathingLimiter::getPathsDeniedThisFrame(int playerID)
{
   if ((playerID >= 0) && (playerID < PATHLIM_MAX_PLAYERS))
      return mPlayerHist[playerID].platoonPathsDeniedThisFrame + mPlayerHist[playerID].squadPathsDeniedThisFrame;
   else
   {
      BASSERTM(0, "PathLimiter index out of bounds.");
      return 0;
   }
}


int BPathingLimiter::getFramesDenied(int playerID)
{
   if ((playerID >= 0) && (playerID < PATHLIM_MAX_PLAYERS))
      return (mPlayerHist[playerID].platoonDeniedFrames + mPlayerHist[playerID].squadDeniedFrames);
   else
   {
      BASSERTM(0, "PathLimiter index out of bounds.");
      return (0);
   }
}

bool BPathingLimiter::getDenyPathing(int playerID)
{
   if ((playerID >= 0) && (playerID < PATHLIM_MAX_PLAYERS))
      return (mPlayerHist[playerID].denyPathing);
   else
   {
      BASSERTM(0, "PathLimiter index out of bounds.");
      return (true);
   }
}


long BPathingLimiter::getPlatoonFramesDenied(long playerID)
{
   if ((playerID >= 0) && (playerID < PATHLIM_MAX_PLAYERS))
      return (mPlayerHist[playerID].platoonDeniedFrames);
   else
   {
      BASSERTM(0, "PathLimiter index out of bounds.");
      return (0);
   }
}


long BPathingLimiter::getMaxPlatoonFramesDenied(long playerID)
{
   if ((playerID >= 0) && (playerID < PATHLIM_MAX_PLAYERS))
      return (mPlayerHist[playerID].platoonMaxDeniedFrames);
   else
   {
      BASSERTM(0, "PathLimiter index out of bounds.");
      return (0);
   }
}


long BPathingLimiter::getSquadFramesDenied(long playerID)
{
   if ((playerID >= 0) && (playerID < PATHLIM_MAX_PLAYERS))
      return (mPlayerHist[playerID].squadDeniedFrames);
   else
   {
      BASSERTM(0, "PathLimiter index out of bounds.");
      return (0);
   }
}


long BPathingLimiter::getMaxSquadFramesDenied(long playerID)
{
   if ((playerID >= 0) && (playerID < PATHLIM_MAX_PLAYERS))
      return (mPlayerHist[playerID].squadMaxDeniedFrames);
   else
   {
      BASSERTM(0, "PathLimiter index out of bounds.");
      return (0);
   }
}

//==============================================================================
// denyPlatoonFrame
//==============================================================================
void BPathingLimiter::denyPlatoonFrame(int nPlayer)
{
   ++mPlayerHist[nPlayer].platoonPathsDeniedThisFrame;
}

//==============================================================================
// denySquadFrame
//==============================================================================
void BPathingLimiter::denySquadFrame(int nPlayer)
{
   ++mPlayerHist[nPlayer].squadPathsDeniedThisFrame;
}

//==============================================================================
// incFramePlatoonLRPCount()
void BPathingLimiter::incFramePlatoonLRPCount()
{
   // Inc total number of platoon LRP's
   ++mNumPlatoonLRPs;

   // Inc # of Platoon LRP's, and update the max.
   if (++mNumPlatoonLRPperFrame > mMaxPlatoonLRPperFrame)
      mMaxPlatoonLRPperFrame = mNumPlatoonLRPperFrame;

}

//==============================================================================
// incFramePlatoonSRPCount()
void BPathingLimiter::incFramePlatoonSRPCount()
{
   // Inc total number of platoon LRP's
   ++mNumPlatoonSRPs;

   // Inc # of Platoon LRP's, and update the max.
   if (++mNumPlatoonSRPperFrame > mMaxPlatoonSRPperFrame)
      mMaxPlatoonSRPperFrame = mNumPlatoonSRPperFrame;

}

//==============================================================================
// incFrameSquadSRPCount()
void BPathingLimiter::incFrameSquadSRPCount()
{
   // Inc total number of Squad LRP's
   ++mNumSquadSRPs;

   // Inc # of Squad LRP's, and update the max.
   if (++mNumSquadSRPperFrame > mMaxSquadSRPperFrame)
      mMaxSquadSRPperFrame = mNumSquadSRPperFrame;

}

//==============================================================================
// requestPlatoonLRP
// basically the platoon asking to path.  True if it can, false if it can't.  
// If you want to know why, feel free to interrogate the limiter personally. 
//==============================================================================
bool BPathingLimiter::requestPlatoonLRP(BPlayerID playerID)
{
   // Update pathing attempt numbers..
   incFramePlatoonLRPCount();

   uint numPlatoonPathingCalls = mNumPlatoonLRPperFrame + mNumPlatoonSRPperFrame;
   if (mPlayerHist[playerID].denyPathing || (numPlatoonPathingCalls >= gDatabase.getMaxPlatoonPathingCallsPerFrame()))
   {
      denyPlatoonFrame(playerID);
      return false;
   }

   return true;
}



//==============================================================================
// requestPlatoonSRP
// I'm sure there's a million things I'm not doing correctly here.  
// But this should be the place where you request a short range (low-level)
// path for the platoon.  If it's allowed, then assume we RAN the path and
// increments counts accordingly.  
//==============================================================================
bool BPathingLimiter::requestPlatoonSRP(BPlayerID playerID)
{
   // Update pathing attempt numbers..
   incFramePlatoonSRPCount();

   uint numPlatoonPathingCalls = mNumPlatoonLRPperFrame + mNumPlatoonSRPperFrame;
   if (mPlayerHist[playerID].denyPathing || (numPlatoonPathingCalls >= gDatabase.getMaxPlatoonPathingCallsPerFrame()))
   {
      denyPlatoonFrame(playerID);
      return false;
   }

   return true;
}


//==============================================================================
// requestSquadSRP
// basically the squad asking to path.  True if it can, false if it can't.  
// If you want to know why, feel free to interrogate the limiter personally. 
//==============================================================================
bool BPathingLimiter::requestSquadSRP(BPlayerID playerID)
{
   // Update pathing attempt numbers..
   incFrameSquadSRPCount();

   if (mPlayerHist[playerID].denyPathing || (mNumSquadSRPperFrame >= gDatabase.getMaxSquadPathingCallsPerFrame()))
   {
      denySquadFrame(playerID);
      return false;
   }

   return true;
}


//============================================================================
//============================================================================
bool BPathingLimiter::save(BStream* pStream, int saveType) const
{
   GFWRITEPTR(pStream, sizeof(BPlayerPathHist)*PATHLIM_MAX_PLAYERS, mPlayerHist);
   GFWRITEVAR(pStream, uint, mNumPlatoonLRPs);
   GFWRITEVAR(pStream, uint, mNumPlatoonSRPs);
   GFWRITEVAR(pStream, uint, mNumSquadSRPs);

   GFWRITEVAR(pStream, uint, mNumPlatoonLRPperFrame);
   GFWRITEVAR(pStream, uint, mMaxPlatoonLRPperFrame);
   GFWRITEVAR(pStream, uint, mNumPlatoonSRPperFrame);
   GFWRITEVAR(pStream, uint, mMaxPlatoonSRPperFrame);

   GFWRITEVAR(pStream, uint, mNumSquadSRPperFrame);
   GFWRITEVAR(pStream, uint, mMaxSquadSRPperFrame);

   GFWRITEVAR(pStream, uint, mReferenceFrame);
   GFWRITEVAR(pStream, uint, mNumPathingFrames);

   GFWRITEVAR(pStream, bool, mbPathHistInitialized);

   return true;
}

//============================================================================
//============================================================================
bool BPathingLimiter::load(BStream* pStream, int saveType)
{
   // Loading is trickier.  Older version load a bunch of data we just
   // don't use anymore.  Set what we can, init the rest
   if (BPathingLimiter::mGameFileVersion < 2)
   {
      uint dummy = 0;
      DWORD dummy2 = 0;
      struct stOldFlags
      {
         bool bit1:1;
         bool bit2:1;
         bool bit3:1;
         bool bit4:1;
      };

      struct stOldPlayerHist
      {
         uint     pathsDeniedLastFrame;   // The number of platoon level pathing attempts this player's platoons made last frame
         uint     framesDenied;           // The number of consecutive frames before this one that at least one platoon path was denied to this player
         bool     denyPathing;            // If true: This player's platoons are not allowed to path this frame (to give others priority)
         long     maxPlatoonFramesDenied;
         long     maxSquadFramesDenied;
         long     platoonFramesDenied;
         long     squadFramesDenied;
      };
      typedef struct stOldPlayerHist BOldPlayerHist;
      BOldPlayerHist oldPlayerHist[PATHLIM_MAX_PLAYERS];

      GFREADPTR(pStream, sizeof(BOldPlayerHist)*PATHLIM_MAX_PLAYERS, oldPlayerHist);
      GFREADVAR(pStream, uint, dummy);
      GFREADVAR(pStream, uint, mNumPlatoonLRPs);
      GFREADVAR(pStream, uint, mNumPlatoonSRPs);
      GFREADVAR(pStream, uint, dummy);
      GFREADVAR(pStream, uint, dummy);
      GFREADVAR(pStream, uint, mNumSquadSRPs);
      GFREADVAR(pStream, uint, dummy);
      GFREADVAR(pStream, uint, dummy);
      GFREADVAR(pStream, uint, dummy);
      GFREADVAR(pStream, uint, dummy);
      GFREADVAR(pStream, uint, dummy);
      GFREADVAR(pStream, uint, dummy);
      GFREADVAR(pStream, uint, dummy);
      GFREADVAR(pStream, uint, mMaxPlatoonLRPperFrame);
      GFREADVAR(pStream, uint, mMaxPlatoonSRPperFrame);
      GFREADVAR(pStream, uint, dummy);
      GFREADVAR(pStream, uint, dummy);
      GFREADVAR(pStream, uint, dummy);
      GFREADVAR(pStream, uint, mMaxSquadSRPperFrame);
      GFREADVAR(pStream, uint, mReferenceFrame);
      GFREADVAR(pStream, uint, mNumPathingFrames);
      GFREADVAR(pStream, uint, dummy);
      GFREADVAR(pStream, uint, dummy);
      GFREADVAR(pStream, uint, dummy);
      GFREADVAR(pStream, uint, dummy);
      GFREADVAR(pStream, uint, dummy);
      GFREADVAR(pStream, uint, dummy);
      GFREADVAR(pStream, uint, dummy);
      GFREADVAR(pStream, uint, dummy);
      GFREADVAR(pStream, uint, dummy);
      GFREADVAR(pStream, uint, dummy);
      GFREADVAR(pStream, uint, dummy);
      GFREADVAR(pStream, DWORD, dummy2);
      GFREADVAR(pStream, DWORD, dummy2);
      GFREADVAR(pStream, DWORD, dummy2);
      GFREADVAR(pStream, DWORD, dummy2);
      struct stOldFlags oldFlags;
      GFREADBITBOOL(pStream, oldFlags.bit1);
      GFREADBITBOOL(pStream, oldFlags.bit2);
      GFREADBITBOOL(pStream, oldFlags.bit3);
      GFREADBITBOOL(pStream, oldFlags.bit4);
      mbPathHistInitialized = oldFlags.bit4;

      mNumPlatoonLRPperFrame = 0;
      mNumPlatoonSRPperFrame = 0;
      mNumSquadSRPperFrame = 0;
      for (long n = 0; n < PATHLIM_MAX_PLAYERS; n++)
      {
         mPlayerHist[n].platoonPathsDeniedThisFrame = 0;
         mPlayerHist[n].platoonDeniedFrames = 0;
         mPlayerHist[n].platoonMaxDeniedFrames = 0;
         mPlayerHist[n].squadPathsDeniedThisFrame = 0;
         mPlayerHist[n].squadDeniedFrames = 0;
         mPlayerHist[n].squadMaxDeniedFrames = 0;
         mPlayerHist[n].denyPathing = 0;
      }
   }
   else
   {
      // Future versions load this.. 
      GFREADPTR(pStream, sizeof(BPlayerPathHist)*PATHLIM_MAX_PLAYERS, mPlayerHist);
      GFREADVAR(pStream, uint, mNumPlatoonLRPs);
      GFREADVAR(pStream, uint, mNumPlatoonSRPs);
      GFREADVAR(pStream, uint, mNumSquadSRPs);

      GFREADVAR(pStream, uint, mNumPlatoonLRPperFrame);
      GFREADVAR(pStream, uint, mMaxPlatoonLRPperFrame);
      GFREADVAR(pStream, uint, mNumPlatoonSRPperFrame);
      GFREADVAR(pStream, uint, mMaxPlatoonSRPperFrame);

      GFREADVAR(pStream, uint, mNumSquadSRPperFrame);
      GFREADVAR(pStream, uint, mMaxSquadSRPperFrame);

      GFREADVAR(pStream, uint, mReferenceFrame);
      GFREADVAR(pStream, uint, mNumPathingFrames);
      GFREADVAR(pStream, bool, mbPathHistInitialized);

   }

   return true;
}



