//==============================================================================
// xboxFileUtils.cpp
//
// Copyright (c) 2007, Ensemble Studios
//==============================================================================

#include "xcore.h"
#include "xboxFileUtils.h"

#ifdef XBOX

bool BXboxFileUtils::mCacheInitialized = false;

#ifndef LTCG
#define ENABLE_CACHE_DRIVE
#endif

#if defined(BUILD_FINAL) && defined(ENABLE_CACHE_DRIVE)
#pragma message("========== ARE YOU SURE YOU WANT THE CACHE DRIVE ENABLED?! ==========")
#endif

//============================================================================
// Check if the cache partition was already mounted
//============================================================================
bool BXboxFileUtils::isCacheInitialized()
{
#ifdef ENABLE_CACHE_DRIVE
   if (mCacheInitialized)
      return true;

   char volumeName[64];
   char fileSystemName[64];
   DWORD serialNumber;
   DWORD maximumFileNameLength;
   DWORD fileSystemFlags;

   BOOL retval = GetVolumeInformation( "CACHE:\\", volumeName, sizeof(volumeName), &serialNumber, &maximumFileNameLength, &fileSystemFlags, fileSystemName, sizeof(fileSystemName));

   if (!retval)
      return false;

#if 0
   // rg [11/06/07] - The Nov. XDK changed the file system to STFC, so this check failed.
   // All we care about is whether or not the volume exists, so don't do this check.
   if (stricmp(fileSystemName, "STFS") == 0)
   {
      mCacheInitialized = true;
   }
#endif   

   mCacheInitialized = true;

   return mCacheInitialized;
#else
   return false;
#endif
}

//============================================================================
// BXboxFileUtils::initXboxCachePartition
// Creates the cache partition if one does not exist.
//============================================================================
void BXboxFileUtils::initXboxCachePartition()
{
   if (isCacheInitialized())
      return;

   // [11/19/08 dpm - do not mount the utility drive, allow the sound system to mount it for us]
   return;

   //DWORD status = XMountUtilityDrive(TRUE, 8192, 65536);

   //if (ERROR_SUCCESS != status)
   //{
   //   // ajl 10/8/08 - the mount will fail if the user has no hard drive so don't fatal assert here.
   //   //BFATAL_FAIL("BXboxFileUtils::initXboxCachePartition: Unable to mount utility drive");
   //}
}

#endif // XBOX