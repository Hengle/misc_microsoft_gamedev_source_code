//==============================================================================
// pathingLimiter.h
//
// Copyright (c) 2001-2007, Ensemble Studios
//==============================================================================
#pragma once
#include "simtypes.h"
#include "gamefilemacros.h"

#define PATHLIM_MAX_PLAYERS   9

struct BPlayerPathHist
{
   uint     platoonPathsDeniedThisFrame;  // Number of platoon Frames denied this frame
   uint     platoonDeniedFrames;          // Number of consecutive frames in which one platoon for this player has been denied
   uint     platoonMaxDeniedFrames;       // Highest number of consecutive frames in which one platoon was denied a path for this player
   uint     squadPathsDeniedThisFrame;    // Number of squad Frames denied this frame
   uint     squadDeniedFrames;            // Number of consecutive frames in which one squad for this player has been denied
   uint     squadMaxDeniedFrames;         // Highest number of consecutive frames in which one squad was denied a path for this player
   bool     denyPathing;                  // If true, then this player is denied pathing attempts this frame
};


//==============================================================================
// BPathingLimiter
//==============================================================================
class BPathingLimiter
{
   public:

      BPathingLimiter(void) :
         mNumPlatoonLRPs(0),
         mNumPlatoonSRPs(0),
         mNumSquadSRPs(0),

         mNumPlatoonLRPperFrame(0),
         mMaxPlatoonLRPperFrame(0),

         mNumPlatoonSRPperFrame(0),
         mMaxPlatoonSRPperFrame(0),

         mNumSquadSRPperFrame(0),
         mMaxSquadSRPperFrame(0),

         mReferenceFrame(0),
         mNumPathingFrames(0),
         mbPathHistInitialized(false)
         {}

         // DLM - added some higher level interfaces into this module.  Also, first comments.  :/  
         bool           requestPlatoonLRP(BPlayerID playerID);
         bool           requestPlatoonSRP(BPlayerID playerID);

         // jce [4/23/2008] -- squad versions of above high level calls
         bool           requestSquadSRP(BPlayerID playerID);

         void           initPlayerPathHist();
         void           prioritizePathing();
         bool           isPathHistInitialized() { return (mbPathHistInitialized); }


         void           incFramePlatoonLRPCount();
         void           incFramePlatoonSRPCount();
         void           incFrameSquadSRPCount();

         void           denyPlatoonFrame(int nPlayer);
         void           denySquadFrame(int nPlayer);


         void           setReferenceFrame(uint frame) { mReferenceFrame = frame; }
         uint           getReferenceFrame()             { return (mReferenceFrame); }
         uint           getNumPathingFrames()           { return (mNumPathingFrames); }


         void           resetFramePathingCounts();
         void           resetPathsDeniedLastFrame();

         uint           getNumPathingCallsThisFrame()                { return (mNumPlatoonLRPperFrame + mNumPlatoonSRPperFrame + mNumSquadSRPperFrame); }

         uint           getNumPlatoonLRPs()                          { return mNumPlatoonLRPs; }
         uint           getNumPlatoonSRPs()                          { return mNumPlatoonSRPs; }
         uint           getNumSquadSRPs()                            { return mNumSquadSRPs;   }

         float          getAvePlatoonLRPperFrame();
         float          getAvePlatoonSRPperFrame();
         float          getAveSquadSRPperFrame();

         uint           getMaxPlatoonLRPperFrame()                   { return mMaxPlatoonLRPperFrame; }
         uint           getMaxPlatoonSRPperFrame()                   { return mMaxPlatoonSRPperFrame; }
         uint           getMaxSquadSRPperFrame()                     { return mMaxSquadSRPperFrame;   }

         uint           getNumPlatoonPathingCallsThisFrame()         { return (mNumPlatoonLRPperFrame + mNumPlatoonSRPperFrame); }
         uint           getMaxPlatoonPathingCallsPerFrame()          { return mMaxPlatoonLRPperFrame; }

         uint           getNumSquadPathingCallsThisFrame()           { return mNumSquadSRPperFrame; }
         uint           getMaxSquadPathingCallsPerFrame()            { return mMaxSquadSRPperFrame; }

         int            getPathsDeniedThisFrame(int playerID);
         int            getFramesDenied(int playerID);
         bool           getDenyPathing(int playerID);

         long           getPlatoonFramesDenied(long playerID);
         long           getSquadFramesDenied(long playerID);
         long           getMaxPlatoonFramesDenied(long playerID);
         long           getMaxSquadFramesDenied(long playerID);

         GFDECLAREVERSION();
         bool save(BStream* pStream, int saveType) const;
         bool load(BStream* pStream, int saveType);

   private:

      // pathing history vars
      BPlayerPathHist            mPlayerHist[PATHLIM_MAX_PLAYERS];
      uint                       mNumPlatoonLRPs;                 // Total Platoon LRP
      uint                       mNumPlatoonSRPs;                 // Total Platoon SRP
      uint                       mNumSquadSRPs;                   // Total Squad SRP

      uint                       mNumPlatoonLRPperFrame;
      uint                       mMaxPlatoonLRPperFrame;
      uint                       mNumPlatoonSRPperFrame;
      uint                       mMaxPlatoonSRPperFrame;

      uint                       mNumSquadSRPperFrame;  // All squad calls are SPR cllas
      uint                       mMaxSquadSRPperFrame;

      uint                       mReferenceFrame;
      uint                       mNumPathingFrames;

      bool                       mbPathHistInitialized;


};