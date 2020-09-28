//==============================================================================
// gamedirectories.cpp
//
// Copyright (c) 2005 Ensemble Studios
//==============================================================================

// Includes
#include "common.h"
#include "gamedirectories.h"
#include "filemanager.h"

// Directory IDs
long cDirData              = -1;
long cDirArt               = -1;
long cDirScenario          = -1;
long cDirSound             = -1;
long cDirFonts             = -1;
long cDirTerrain           = -1;
long cDirPhysics           = -1;
long cDirRenderEffects     = -1;
long cDirTactics           = -1;
long cDirPlacementRules    = -1;
long cDirTriggerScripts    = -1;
long cDirRecordGame        = -1;
long cDirSaveGame          = -1;
long cDirCampaign          = -1;
long cDirTalkingHead       = -1;
long cDirAIData            = -1;


//==============================================================================
// setupGameDir
//==============================================================================
static long setupGameDir(const BSimString& dirName)
{
   BFixedStringMaxPath tempDirName;
   
   eFileManagerError result = gFileManager.getDirListEntry(tempDirName, cDirProduction);
   BVERIFY(cFME_SUCCESS == result);
   
   tempDirName.append(dirName);
   
   long dirID;
   result = gFileManager.createDirList(tempDirName, dirID);
   BVERIFY(cFME_SUCCESS == result);
   
   return dirID;
}

//==============================================================================
// setupGameDirectories
//
// Sets up all of our game-specific directories with the file manager. These 
// can later be obtained from the file manager using their ID.
//==============================================================================
bool setupGameDirectories()
{
   // Validate current directory
   if (gFileManager.doesFileExist(cDirProduction, "startup\\game.cfg") != cFME_SUCCESS)
      return false;

   // Game directories
   cDirData = setupGameDir("data\\");
   cDirArt = setupGameDir("art\\");
   //Changing dir to WTF for precert1 - eric
   //cDirTalkingHead = setupGameDir("wtf\\talkingheads");
   cDirTalkingHead = setupGameDir("video\\talkingheads");
   cDirScenario = setupGameDir("Scenario\\");
   cDirSound = setupGameDir("sound\\");
   cDirFonts = setupGameDir("Fonts\\");
   cDirTerrain = setupGameDir("art\\terrain\\");
   cDirPhysics = setupGameDir("physics");
   cDirRenderEffects = setupGameDir("shaders\\");
   cDirTactics = setupGameDir("data\\tactics\\");
   cDirPlacementRules = setupGameDir("data\\placementrules\\");
   cDirTriggerScripts = setupGameDir("data\\triggerscripts\\");
   cDirRecordGame = setupGameDir("recordgame\\");
   cDirSaveGame = setupGameDir("savegame\\");
   cDirCampaign = setupGameDir("campaign\\");
   cDirAIData = setupGameDir("data\\aidata");

   return true;
}
