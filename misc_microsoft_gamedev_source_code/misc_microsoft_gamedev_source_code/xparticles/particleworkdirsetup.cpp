//==============================================================================
// File: particleworkdirsetup.cpp
//
// Copyright (c) 2003, Ensemble Studios
//==============================================================================

#include "xparticlescommon.h"
#include "workdirsetup.h"
#include "FileManager.h"

long cDirParticlesRoot           = -1;
long cDirParticlesEffects        = -1;
long cDirParticleFX              = -1;
//==============================================================================
// initializeInterfaceWorkingDirectoryIDs
//
//==============================================================================
void initializeParticleWorkingDirectoryIDs(void)
{
   ASSERT_MAIN_THREAD
   
   BString usProductionDir, tempDirName;
   eFileManagerError result = gFileManager.getDirListEntry(usProductionDir, cDirProduction);
   BVERIFY(cFME_SUCCESS == result);

   if(cDirProduction == -1)
   {
      BFAIL("Production directory not properly initialized... lots of other stuff is about to assert, no doubt :).");
      return;
   }

   tempDirName.set(B("art\\"));
   BString dir=usProductionDir;
   dir.append(tempDirName);
   result = gFileManager.createDirList(dir, cDirParticlesRoot);
   BVERIFY(cFME_SUCCESS == result);

   tempDirName.set(B("art\\effects"));   
   dir = usProductionDir;
   dir.append(tempDirName);
   result = gFileManager.createDirList(dir, cDirParticlesEffects);
   BVERIFY(cFME_SUCCESS == result);

   tempDirName.set(B("particles"));
   dir=usProductionDir;
   dir.append(tempDirName);
   result = gFileManager.createDirList(dir, cDirParticleFX);
   BVERIFY(cFME_SUCCESS == result);
}
