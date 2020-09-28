//==============================================================================
// notification.h
//
// Copyright (c) 2007 Ensemble Studios
//==============================================================================

#pragma once

#ifndef _NOTIFICATION_H_
#define _NOTIFICATION_H_

class BNotification;

// Global variable for the one BNotification object
extern BNotification gNotification;

//==============================================================================
// class BNotification
//==============================================================================
class BNotification
{
   public:
                              BNotification();

      bool                    setup();
      void                    shutdown();

      void                    update();                     // poll for any available events

      BOOL                    isSystemUIShowing() const { return mSystemUIShowing; }

      DWORD                   getNotification() const { return mID; }
      ULONG_PTR               getParam() const { return mParam; }

	  BOOL                    getInputDeviceChangedNotification();

   private:

      HANDLE                  mNotification;						// handle to the xbox event notification mechanism

      BOOL                    mSystemUIShowing;

      DWORD                   mID;                          // last received notification ID
      ULONG_PTR               mParam;                       // last received notification param

	  BOOL                    mInputDeviceChangedNotification;
};

#endif
