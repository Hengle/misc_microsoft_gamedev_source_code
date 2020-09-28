//============================================================================
//
//  fileSystem.h
//
//  Copyright 2002-2007 Ensemble Studios
//
//============================================================================
#pragma once

//----------------------------------------------------------------------------
//  Callback Prototypes
//----------------------------------------------------------------------------
typedef bool (CALLBACK FindFileFunc   )(const BString& Path, void* pParam);

class BFileTime
{
   public:
      ushort mYear;
      ushort mMonth;
      ushort mDayOfWeek;
      ushort mDay;
      ushort mHour;
      ushort mMinute;
      ushort mSecond;
      ushort mMilliseconds;

      void makeError(void)
      {
         memset(this, 0, sizeof(BFileTime));
      }
   
      bool operator==(const BFileTime &time) const
      {
         return equal(time);
      }

      bool operator>=(const BFileTime &time) const
      {
         return greaterThan(time) || equal(time);
      }

      bool operator>(const BFileTime &time) const
      {
         return greaterThan(time);
      }

      bool operator<=(const BFileTime &time) const
      {
         return !greaterThan(time) || equal(time);
      }

      bool operator<(const BFileTime &time) const
      {
         return !greaterThan(time) && !equal(time);
      }

      bool equal(const BFileTime &time) const
      {
         // can't just memcmp because of the random value of the pad byte

         if ((mYear == time.mYear) && (mMonth == time.mMonth) && (mDay == time.mDay) && (mHour == time.mHour) && 
             (mMinute == time.mMinute) && (mSecond == time.mSecond))
            return true;

         return false;
      }

      bool greaterThan(const BFileTime &time) const
      {
         if (mYear > time.mYear)
            return true;

         if ((mYear == time.mYear) && (mMonth > time.mMonth))
            return true;

         if ((mYear == time.mYear) && (mMonth == time.mMonth) && (mDay > time.mDay))
            return true;

         if ((mYear == time.mYear) && (mMonth == time.mMonth) && (mDay == time.mDay) && (mHour > time.mHour))
            return true;

         if ((mYear == time.mYear) && (mMonth == time.mMonth) && (mDay == time.mDay) && (mHour == time.mHour) && 
             (mMinute > time.mMinute))
            return true;

         if ((mYear == time.mYear) && (mMonth == time.mMonth) && (mDay == time.mDay) && (mHour == time.mHour) && 
             (mMinute == time.mMinute) && (mSecond > time.mSecond))
            return true;

         return false;
      }
};

//----------------------------------------------------------------------------
//  Subsystems
//----------------------------------------------------------------------------
#include "File.h"
#include "FileManager.h"
