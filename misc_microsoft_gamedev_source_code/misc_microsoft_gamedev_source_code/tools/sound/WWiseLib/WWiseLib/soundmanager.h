//====================================================`==========================
// soundmanager.h
//
// Copyright (c) 2006 Ensemble Studios
//==============================================================================

#ifndef _SOUNDMANAGER_H_
#define _SOUNDMANAGER_H_

// Includes
#include "Stdafx.h"
#include <string>
using std::string;
#pragma warning( push )
#pragma warning( disable : 4245 )
#include <AK/SoundEngine/Common/AkTypes.h>
#pragma warning( pop ) 


// Forward declarations
class BSoundManager;

// Externs
extern BSoundManager gSoundManager;

//==============================================================================
// Sound events
//==============================================================================
enum
{
   cSoundEventStop,
};

//==============================================================================
// BSoundManager
//==============================================================================
class BSoundManager
{
   public:

      //-- Listener player objects for split screen
      enum
      {
         cPlayer1,
         cPlayer2,      
         cPlayerMax,
      };

      enum
      {
         cMaxBanks = 40
      };

                              BSoundManager();
                              ~BSoundManager();

      bool                    initSoundEngine(const char *soundPath);
      bool                    addBank(const char* bankName);
      void                    shutdown();
      void                    update();


      long                    getCueIndex(const char* cueName);
      long                    playCue(const char* cueName);      
      void                    setRTPCValue(AkLpCtstr name, float value);

      void                    updateEmitter2D(long objectID, float pos2D, float volume);

      void                    handleNotification(AkUInt32 in_callbackType, AkPlayingID in_playingID, AkUniqueID	in_eventID, AkGameObjectID in_gameObj, void * in_pCookie, void * in_pData, AkUInt32 in_uDataSize);
      static void             eventCallbackFunc(AkUInt32 in_callbackType, AkPlayingID in_playingID, AkUniqueID	in_eventID, AkGameObjectID in_gameObj, void * in_pCookie, void * in_pData, AkUInt32 in_uDataSize);

private:

   long                                mPlayerObjects[cPlayerMax];
   string                              mBanksToLoad[cMaxBanks];
   long                                mNumBanksToLoad;
      
};

//-- Exposed Methods
extern "C" _declspec(dllexport) bool dllInitSound(char *soundEvent)
{
   bool result = gSoundManager.initSoundEngine(soundEvent);
   return result;
}

extern "C" _declspec(dllexport) bool dllAddBankToLoad(char *bankName)
{
   bool result = gSoundManager.addBank(bankName);
   return result;
}

extern "C" _declspec(dllexport) void dllUpdateSound()
{
   gSoundManager.update();
}

extern "C" _declspec(dllexport) void dllPlaySound(char *soundEvent)
{
   gSoundManager.playCue(soundEvent);
}

/*extern "C" _declspec(dllexport) void dllSetMasterVolume(float zeroToOnehundred)
{
   gSoundManager.setRTPCValue(L"master_volume", zeroToOnehundred);
}*/

#endif
