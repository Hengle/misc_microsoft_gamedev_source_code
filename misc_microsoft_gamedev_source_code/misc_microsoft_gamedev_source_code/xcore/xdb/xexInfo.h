//==============================================================================
// xexInfo.h
//
// Copyright (c) 2006 Ensemble Studios
//==============================================================================
#pragma once
#ifdef XBOX
#include "stream\stream.h"

class BXEXChecksum
{
public:
   BXEXChecksum();
   ~BXEXChecksum();

   bool compute(DWORD& checksum, BStream& stream);
};

class BXEXInfo
{
   BString mLaunchPath;
   BString mModuleFullName;
   BString mModuleName;
   DWORD mBaseAddress;
   DWORD mSize;
   bool mValid;

   bool init(void);

public:
   BXEXInfo();
   ~BXEXInfo();

   bool getValid(void) const { return mValid; }

   // The launch path - drive may not be accessible on 360!
   const BString& getLaunchPath(void) const { return mLaunchPath; }
   
   // The full path+filename of the executable. Always points to "d:" drive.
   const BString getModuleFullName(void) const { return mModuleFullName; }
   
   // The name of the executable with no path.
   const BString& getModuleName(void) const { return mModuleName; }
   DWORD getBaseAddress(void) const { return mBaseAddress; }
   DWORD getSize(void) const { return mSize; }
};
#endif // XBOX