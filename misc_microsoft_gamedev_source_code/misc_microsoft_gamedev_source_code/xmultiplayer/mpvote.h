//==============================================================================
// MPVote.h
//
// Copyright (c) 2003, Ensemble Studios
//==============================================================================

#ifndef _MPVOTE_H_
#define _MPVOTE_H_

//==============================================================================
// Includes
#include "mptypes.h"

//==============================================================================
// Forward declarations
class BMPVoteHandler;

//==============================================================================
// Const declarations

//==============================================================================
class BMPVoteNotify
{
public:
   BMPVoteNotify() : mHandler(NULL) {}
   virtual ~BMPVoteNotify();

   virtual void      castVote(long type, long vote, long arg1 = -1);
   virtual void      startVote(long type, long arg1 = -1);
   virtual void      abortVote(long type, long arg1 = -1);
   virtual bool      incomingVote(long type, PlayerID from, long vote, long arg1 = -1) = 0;
   virtual void      voteStarted(long type, long arg1 = -1) = 0;
   virtual void      voteAborted(long type, long arg1 = -1) = 0;

   void              attachVoteHandler(BMPVoteHandler *handler);
   void              detachVoteHandler(BMPVoteHandler *handler);
protected:
   BMPVoteHandler    *mHandler;
};

//==============================================================================
class BMPVoteHandler
{
public:
   BMPVoteHandler() {}
   virtual ~BMPVoteHandler();

   virtual void      attachVoteNotify(BMPVoteNotify *notify);
   virtual void      detachVoteNotify(BMPVoteNotify *notify);
   virtual void      castVote(long type, long vote, long arg1 = -1) = 0;
   virtual void      startVote(long type, long arg1 = -1) = 0;
   virtual void      abortVote(long type, long arg1 = -1) = 0;

protected:
   void              notifyVoteResults(long type, PlayerID fromPlayerID, long vote, long arg1 = -1)
   {
      for (long idx=0; idx<mNotify.getNumber(); idx++)
      {
         mNotify[idx]->incomingVote(type, fromPlayerID, vote, arg1);
      }
   }

   void              notifyVoteStart(long type, long arg1 = -1)
   {
      for (long idx=0; idx<mNotify.getNumber(); idx++)
      {
         mNotify[idx]->voteStarted(type, arg1);
      }
   }

   void              notifyVoteAbort(long type, long arg1 = -1)
   {
      for (long idx=0; idx<mNotify.getNumber(); idx++)
      {
         mNotify[idx]->voteAborted(type, arg1);
      }
   }

private:
   BDynamicSimArray<BMPVoteNotify*> mNotify;
   
};

//==============================================================================
#endif // _MPVOTE_H_

//==============================================================================
// eof: mpvote.h
//==============================================================================