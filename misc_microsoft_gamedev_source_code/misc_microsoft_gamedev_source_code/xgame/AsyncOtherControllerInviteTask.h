//==============================================================================
// Copyright (c) 2008 Ensemble Studios
//==============================================================================

#pragma once

#include "asynctaskmanager.h"

// Forward Declarations

//==============================================================================
// BAsyncOtherControllerInviteTask - Confirm with the controller that they really 
// want to accept the invite.
//==============================================================================
class BAsyncOtherControllerInviteTask : public BAsyncTask
{

public:
   BAsyncOtherControllerInviteTask();
   virtual ~BAsyncOtherControllerInviteTask();

   bool confirmInvite(DWORD controllerPort);

   // overrides
   void processResult();
   bool update();
   BOOL isComplete();


protected:

   DWORD BAsyncOtherControllerInviteTask::showMessageBox();

   enum
   {
      cTaskStateWaitingForUI,
      cTaskStateMessageBoxUp,
      cTaskStateError,
   };


   LPCWSTR  mButtonTexts[2];

   DWORD    mControllerPort;

   MESSAGEBOX_RESULT mMessageBoxResult;

   long     mState;
   DWORD    mNextAttempt;
};

