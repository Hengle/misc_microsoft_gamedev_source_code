//==============================================================================
// Copyright (c) 2007 Ensemble Studios
//==============================================================================

// Includes
#include "common.h"
#include "AsyncOtherControllerInviteTask.h"
#include "xbox.h"
#include "game.h"
#include "database.h"
#include "LiveSystem.h"
#include "usermanager.h"



//==============================================================================
// BAsyncOtherControllerInviteTask::BAsyncOtherControllerInviteTask
//==============================================================================
BAsyncOtherControllerInviteTask::BAsyncOtherControllerInviteTask() :
   BAsyncTask()
{
}

//==============================================================================
// BAsyncOtherControllerInviteTask::BAsyncOtherControllerInviteTask
//==============================================================================
BAsyncOtherControllerInviteTask::~BAsyncOtherControllerInviteTask()
{
}

//==============================================================================
// BAsyncOtherControllerInviteTask::confirmInvite
//==============================================================================
bool BAsyncOtherControllerInviteTask::confirmInvite(DWORD controllerPort)
{
   // send the achievement off
/*
   DWORD XShowMessageBoxUI(
      DWORD dwUserIndex,
      LPCWSTR wszTitle,
      LPCWSTR wszText,
      DWORD cButtons,
      LPCWSTR *pwszButtons,
      DWORD dwFocusButton,
      DWORD dwFlags,
      MESSAGEBOX_RESULT *pResult,
      XOVERLAPPED *pOverlapped
      );
*/

   mControllerPort=controllerPort;
   mCheckTimeout=true;
   mTimeoutLength=5000;    // 5 second timeout
   mState = cTaskStateWaitingForUI;

   mNextAttempt=timeGetTime()+100;

   return true;
}

//==============================================================================
//==============================================================================
DWORD BAsyncOtherControllerInviteTask::showMessageBox()
{
   mButtonTexts[0]=gDatabase.getLocStringFromID(25578).getPtr();     // YES
   mButtonTexts[1]=gDatabase.getLocStringFromID(25579).getPtr();     // NO

   mMessageBoxResult.dwButtonPressed=1;      // NO.

   DWORD status = XShowMessageBoxUI(mControllerPort, gDatabase.getLocStringFromID(26004).getPtr(), gDatabase.getLocStringFromID(25993).getPtr(), 
      2, mButtonTexts, 1 /*NO*/, XMB_WARNINGICON, &mMessageBoxResult, &mOverlapped);

   // return the result
   return status;
}

//==============================================================================
//==============================================================================
BOOL BAsyncOtherControllerInviteTask::isComplete()
{
   if (mState == cTaskStateWaitingForUI)
      return FALSE;

   return __super::isComplete();
}



//==============================================================================
// BAsyncOtherControllerInviteTask::processResult
//==============================================================================
bool BAsyncOtherControllerInviteTask::update()
{
   if (hasTimedOut())
   {
      mState = cTaskStateError;
      return true;
   }

   switch (mState)
   {
      case cTaskStateWaitingForUI:
         {
            if (mNextAttempt > timeGetTime())
               break;

            if (gUserManager.isSystemUIShowing())
            {
               mNextAttempt = timeGetTime()+100;
               break;
            }

            DWORD result = showMessageBox();

            switch (result)
            {
               case ERROR_IO_PENDING:
                  // This is a success, now turn off our timeout and wait until the UI message box goes away.
                  mTimeoutLength=0;
                  mCheckTimeout=false;
                  mState = cTaskStateMessageBoxUp;
                  break;

               case ERROR_ACCESS_DENIED:
                  // There is another UI up, we need to try this again in another 100ms.

                  mNextAttempt = timeGetTime()+100;

                  DWORD overlappedResult;
                  XGetOverlappedResult(mpOverlapped, &overlappedResult, TRUE);
                  Utils::FastMemSet(&mOverlapped, 0, sizeof(mOverlapped));

                  break;

               default:
                  // This is an error case
                  mState = cTaskStateError;
                  mTimeoutLength=0;       // time ourselves out
                  break;
            }
         }
      case cTaskStateMessageBoxUp:
         break;
      case cTaskStateError:
         break;
   }

   return true;
}


//==============================================================================
// BAsyncOtherControllerInviteTask::processResult
//==============================================================================
void BAsyncOtherControllerInviteTask::processResult()
{
   BAsyncTask::processResult();

   if (hasTimedOut())
   {
      gLiveSystem->clearInviteInfo();
      return;
   }

   DWORD taskResult = 0;
   DWORD result = getResult(&taskResult);

   // bail if our call to get the result fails.
   if (result != ERROR_SUCCESS)
   {
      gLiveSystem->clearInviteInfo();
      return;
   }

   switch (taskResult)
   {
      case ERROR_SUCCESS:
         if (mMessageBoxResult.dwButtonPressed == 0)
         {
            // YES - go ahead and transfer
            gGame.acceptOtherControllerInvite();
            return;
         }
         break;

      case ERROR_CANCELLED:
         break;

      case ERROR_ACCESS_DENIED:
         break;
   }

   gLiveSystem->clearInviteInfo();
}


