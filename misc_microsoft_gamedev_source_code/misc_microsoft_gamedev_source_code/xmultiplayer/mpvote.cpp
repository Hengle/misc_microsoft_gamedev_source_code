//==============================================================================
// mpvote.cpp
//
// Copyright (c) 2003, Ensemble Studios
//==============================================================================

//==============================================================================
// Includes
#include "multiplayercommon.h"
#include "mpvote.h"

//==============================================================================
// Const declarations

//==============================================================================
// BMPVoteNotify::~BMPVoteNotify
//==============================================================================
BMPVoteNotify::~BMPVoteNotify()
{
   detachVoteHandler(mHandler);
}

//==============================================================================
// BMPVoteNotify::castVote
//==============================================================================
void BMPVoteNotify::castVote(long type, long vote, long arg1)
{
   if (mHandler)
      mHandler->castVote(type, vote, arg1);
}

//==============================================================================
// BMPVoteNotify::startVote
//==============================================================================
void BMPVoteNotify::startVote(long type, long arg1)
{
   if (mHandler)
      mHandler->startVote(type, arg1);
}

//==============================================================================
// BMPVoteNotify::abortVote
//==============================================================================
void BMPVoteNotify::abortVote(long type, long arg1)
{
   if (mHandler)
      mHandler->abortVote(type, arg1);
}

//==============================================================================
// BMPVoteNotify::attachVoteHandler
//==============================================================================
void BMPVoteNotify::attachVoteHandler(BMPVoteHandler *handler)
{
   mHandler = handler;
}

//==============================================================================
// BMPVoteNotify::detachVoteHandler
//==============================================================================
void BMPVoteNotify::detachVoteHandler(BMPVoteHandler *handler)
{
   if (mHandler == handler)
      mHandler = NULL;
}


//==============================================================================
// BMPVoteHandler::~BMPVoteHandler
//==============================================================================
BMPVoteHandler::~BMPVoteHandler()
{
   for (long idx=0; idx<mNotify.getNumber(); idx++)
      mNotify[idx]->detachVoteHandler(this);
   mNotify.setNumber(0);
}

//==============================================================================
// BMPVoteHandler::attachVoteNotify
//==============================================================================
void BMPVoteHandler::attachVoteNotify(BMPVoteNotify *notify)
{
   if (notify)
   {
      notify->attachVoteHandler(this);
      mNotify.uniqueAdd(notify);
   }
}

//==============================================================================
// BMPVoteHandler::detachVoteNotify
//==============================================================================
void BMPVoteHandler::detachVoteNotify(BMPVoteNotify *notify)
{
   if (notify)
   {
      notify->detachVoteHandler(this);
      mNotify.remove(notify);
   }
}

//==============================================================================
// eof: mpvote.cpp
//==============================================================================
