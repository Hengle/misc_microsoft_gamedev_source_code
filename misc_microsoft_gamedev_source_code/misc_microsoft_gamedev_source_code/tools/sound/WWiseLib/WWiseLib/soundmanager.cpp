//==============================================================================
// soundmanager.cpp
//
// Copyright (c) 2006 Ensemble Studios
//==============================================================================

// Includes
#include "Stdafx.h"
#include "soundmanager.h"
#include "AkDefaultLowLevelIO.h"
#include "assert.h"
#include "atlstr.h"

#include <AK/SoundEngine/Common/AkSoundEngine.h>				// Sound Engine
#include <AK/SoundEngine/Common/AkMemoryMgr.h>					// Memory Manager
#include <AK/SoundEngine/Common/IAkStreamMgr.h>					// Streaming Manager
#include <AK/SoundEngine/Platforms/Windows/AkModule.h>			// Default memory and stream managers
#include <AK/SoundEngine/Platforms/Windows/AkWinSoundEngine.h> // Platform-specific structures


#include <AK/Plugin/AkVorbisFactory.h>
#include <AK/Plugin/AkSilenceSourceFactory.h>
#include <AK/Plugin/AkReverbFXFactory.h>
#include <AK/Plugin/AkReverbLiteFXFactory.h>
#include <AK/Plugin/AkParametricEQFXFactory.h>
#include <AK/Plugin/AkPeakLimiterFXFactory.h>
#include <AK/Plugin/AkDelayFXFactory.h>


#ifndef AK_OPTIMIZED
#include <AK/Comm/CommunicationCentralFactory.h>
#include <AK/Comm/ICommunicationCentral.h>
#include <AK/Comm/ProxyFrameworkFactory.h>
#include <AK/Comm/IProxyFrameworkConnected.h>
#endif

// Global items related to communication between Wwise and the game
#ifndef AK_OPTIMIZED
AkMemPoolId g_poolComm = AK_INVALID_POOL_ID;
AK::Comm::ICommunicationCentral * g_pCommCentral = NULL;         
AK::Comm::IProxyFrameworkConnected * g_pProxyFramework = NULL;
#define COMM_POOL_SIZE          (256 * 1024)
#define COMM_POOL_BLOCK_SIZE    (48)
#endif

// Globals
static BSoundManager gSoundManager;

namespace AK
{
   void * AllocHook( size_t in_size )
   {
      // Allocate the memory
      return malloc(in_size);
   }
   void FreeHook( void * in_ptr )
   {
      free(in_ptr);
   }

   void * VirtualAllocHook(void * in_pMemAddress, size_t in_size, DWORD in_dwAllocationType, DWORD in_dwProtect)
   {
      return VirtualAlloc( in_pMemAddress, in_size, in_dwAllocationType, in_dwProtect );
   }

   void VirtualFreeHook(void * in_pMemAddress, size_t in_size, DWORD in_dwFreeType)
   {
      VirtualFree( in_pMemAddress, in_size, in_dwFreeType );
   }
}

#if 0
static BVector AKVectorToBVector(AkVector rhs)
{
   BVector lhs;
   lhs.x = rhs.X;
   lhs.y = rhs.Y;
   lhs.z = rhs.Z;
   return lhs;
}
#endif

//==============================================================================
// BSoundManager::BSoundManager
//==============================================================================
BSoundManager::BSoundManager() 
{
   mPlayerObjects[cPlayer1] = 3;
   mPlayerObjects[cPlayer2] = 4;   
   mNumBanksToLoad = 0;
}

//==============================================================================
// BSoundManager::~BSoundManager
//==============================================================================
BSoundManager::~BSoundManager()
{
}


//==============================================================================
// BSoundManager::initSoundEngine
//==============================================================================
bool BSoundManager::initSoundEngine(const char *soundPath)
{
   // Create and initialize an instance of the default memory manager.
   AkMemSettings memSettings;
   memSettings.uMaxNumPools = 45;

   if ( AK::MemoryMgr::Init( &memSettings ) != AK_Success )
   {
      assert( ! "Could not create the memory manager." );
      return false;
   }

   // Initialize our instance of the default File System/LowLevelIO. 
   AKRESULT eResult = gLowLevelIO.Init();
   if ( eResult != AK_Success )
   {
      assert( !"Cannot initialize Low-Level IO" );
      return false;
   }

   // Streaming settings
   AkStreamMgrSettings stmSettings;
   stmSettings.uMemorySize = 32*1024;                          // 32 Kb for small objects memory.
   stmSettings.pLowLevelIO = &gLowLevelIO;                     // Low-level IO hook.

   // Create and initialize an instance of our stream manager.
   AK::IAkStreamMgr * pStreamMgr = AK::CreateStreamMgr( &stmSettings );
   if ( ! pStreamMgr )
   {
      assert( ! "Could not create the Streaming Manager." );
      return false;
   }

   // Create and init communications


   // Create default IO device.

   // Define properties for device thread
   AkThreadProperties deviceThreadProperties;
   deviceThreadProperties.nPriority = THREAD_PRIORITY_ABOVE_NORMAL;
   deviceThreadProperties.uStackSize = 8192;
   deviceThreadProperties.dwAffinityMask = 0;

   // Device settings and heuristics
   AkDeviceSettings deviceSettings;
   deviceSettings.uIOMemorySize = 1024*1024;                 // 1 Mb of memory for I/O.
   deviceSettings.uGranularity = 32*1024;                    // 16 Kb I/O granularity.
   deviceSettings.fTargetAutoStmBufferLength = 380;          // 380 ms buffering.
   deviceSettings.dwIdleWaitTime = INFINITE;                        // Does not stream passed target buffering length.
   deviceSettings.uSchedulerTypeFlags = AK_SCHEDULER_BLOCKING; // Use blocking I/O.
   deviceSettings.pThreadProperties = &deviceThreadProperties;

   eResult = gLowLevelIO.SetDevice( deviceSettings );
   if ( eResult != AK_Success )
   {
      assert( !"Cannot set default IO device" );
      return false;
   }

   //-- Fill out the default init settings
   AkInitSettings initSettings;
   AK::SoundEngine::GetDefaultInitSettings(initSettings);
   initSettings.uDefaultPoolSize = 1024*1024*4; //4MB

   //-- Fill out default platform settings
   AkPlatformInitSettings platformInitSettings;
   AK::SoundEngine::GetDefaultPlatformInitSettings(platformInitSettings);
   //platformInitSettings.threadLEngine.dwProcessor = 4;
   platformInitSettings.uLEngineDefaultPoolSize = 1024*1024*4; //4MB

   // Create the sound engine
   if ( AK::SoundEngine::Init( &initSettings, &platformInitSettings) == NULL )
   {
      assert(0);
      return false;
   }

   // Register Vorbis codec
    AK::SoundEngine::RegisterCodec( AKCOMPANYID_AUDIOKINETIC, AKCODECID_VORBIS, CreateVorbisFilePlugin, CreateVorbisBankPlugin );
    
   //-- Register the necessary plug-ins
   AK::SoundEngine::RegisterPlugin(AkPluginTypeSource, AKCOMPANYID_AUDIOKINETIC, AKSOURCEID_SILENCE, CreateSilenceSource, CreateSilenceSourceParams);
   AK::SoundEngine::RegisterPlugin(AkPluginTypeEffect, AKCOMPANYID_AUDIOKINETIC, AKEFFECTID_REVERB, CreateReverbFX, CreateReverbFXParams);
   AK::SoundEngine::RegisterPlugin(AkPluginTypeEffect, AKCOMPANYID_AUDIOKINETIC, AKEFFECTID_PARAMETRICEQ, CreateParametricEQFX, CreateParametricEQFXParams);
   AK::SoundEngine::RegisterPlugin(AkPluginTypeEffect, AKCOMPANYID_AUDIOKINETIC, AKEFFECTID_PEAKLIMITER, CreatePeakLimiterFX, CreatePeakLimiterFXParams);
   AK::SoundEngine::RegisterPlugin(AkPluginTypeEffect, AKCOMPANYID_AUDIOKINETIC, AKEFFECTID_DELAY, CreateDelayFX, CreateDelayFXParams);
   AK::SoundEngine::RegisterPlugin(AkPluginTypeEffect, AKCOMPANYID_AUDIOKINETIC, AKEFFECTID_REVERBLITE, CreateReverbLiteFX, CreateReverbLiteFXParams);


   //
   // Initialize communications (not in release build!)
   //
#ifndef AK_OPTIMIZED
   // Create the communications memory pool
   g_poolComm = pMemoryMgr->CreatePool(NULL, COMM_POOL_SIZE, COMM_POOL_BLOCK_SIZE, AkMalloc );
   assert( g_poolComm != AK_INVALID_POOL_ID );
   AK_SETPOOLNAME( g_poolComm, L"Communication" );

   // Create and initialize the Proxy Framework and Communication Central
   g_pProxyFramework = AkCreateProxyFramework( g_poolComm );
   g_pCommCentral  = AkCreateCommunicationCentral( g_poolComm );

   g_pCommCentral->Init( g_pProxyFramework, g_pProxyFramework );
   g_pProxyFramework->Init();
   g_pProxyFramework->SetNotificationChannel( g_pCommCentral->GetNotificationChannel() );
#endif

   //-- Set the language path
   gLowLevelIO.SetLangSpecificDirName(L"English(US)/" );
   CAtlStringW wideString(soundPath); 
   WCHAR * wideStringPtr=wideString.GetBuffer(0);
   gLowLevelIO.SetBasePath((AkLpCtstr)wideStringPtr);

   //-- Load the default sound banks
   AkBankID bankID;
   long result;

   result = AK::SoundEngine::LoadBank( L"Init.bnk", AK_DEFAULT_POOL_ID, bankID );
   assert(result == AK_Success);

   for(long i=0; i < mNumBanksToLoad; i++)
   {
      //const char *temp = mBanksToLoad[i].c_str();

      CAtlStringW wideString2(mBanksToLoad[i].c_str()); 
      WCHAR * wideStringPtr2=wideString2.GetBuffer(0);                                             
      result = AK::SoundEngine::LoadBank( (AkLpCtstr)wideStringPtr2, AK_DEFAULT_POOL_ID, bankID );
      assert(result == AK_Success);
   }
   

   //-- Add the UI game object
   AK::SoundEngine::RegisterGameObj(mPlayerObjects[cPlayer1], "Player1");
   AK::SoundEngine::RegisterGameObj(mPlayerObjects[cPlayer2], "Player2");


   return true;
}

//==============================================================================
// BSoundManager::shutdown
//==============================================================================
void BSoundManager::shutdown()
{

   // Terminate Communication Services
#ifndef AK_OPTIMIZED
   // Destroy the Proxy Framework
   if ( g_pProxyFramework )
   {
      g_pProxyFramework->Term();
      g_pProxyFramework->Destroy();
      g_pProxyFramework = NULL;
   }

   // Destroy the Communication Central
   if ( g_pCommCentral )
   {
      g_pCommCentral->Term();
      g_pCommCentral->Destroy();
      g_pCommCentral = NULL;
   }

   // Destroy Communication Memory Pools
   if( AK::IAkMemoryMgr::Get() )
   {
      AK::IAkMemoryMgr::Get()->DestroyPool( g_poolComm );
   }
#endif


   // Terminate the sound engine
   AK::SoundEngine::Term();

   // Terminate the streaming manager
   if ( AK::IAkStreamMgr::Get() )
   {   
      AK::IAkStreamMgr::Get()->Destroy();
   }
   gLowLevelIO.Term();

   // Terminate the Memory Manager
   if(AK::MemoryMgr::IsInitialized())
      AK::MemoryMgr::Term();

}

//==============================================================================
// BSoundManager::update
//==============================================================================
void BSoundManager::update()
{
   if(AK::SoundEngine::IsInitialized() == false)
      return;

   AKRESULT result = AK::SoundEngine::RenderAudio();   
   assert(result == AK_Success);

#ifndef AK_OPTIMIZED
   if(g_pCommCentral)
      g_pCommCentral->Process();
#endif
}

//==============================================================================
// BSoundManager::playCue
//==============================================================================
 long BSoundManager::playCue(const char* cueName)
{
   if(AK::SoundEngine::IsInitialized() == false)
      return -1;

   CAtlStringW wideString(cueName); 
   WCHAR * wideStringPtr=wideString.GetBuffer(0);

   long handle = (long)AK::SoundEngine::PostEvent((AkLpCtstr)wideStringPtr, mPlayerObjects[cPlayer1]);

   wideString.ReleaseBuffer();

   if(handle == AK_INVALID_PLAYING_ID)
   {
      return -1;
   }

   return handle;
}

//==============================================================================
// BSoundManager::getCueIndex
//==============================================================================
long BSoundManager::getCueIndex(const char* cueName)
{
   if(AK::SoundEngine::IsInitialized() == false)
      return -1;

   CAtlStringW wideString(cueName); 
   WCHAR * wideStringPtr=wideString.GetBuffer(0);

   long eventID = AK::SoundEngine::GetIDFromString((AkLpCtstr)wideStringPtr);
   
   wideString.ReleaseBuffer();

   return eventID;
}

//==============================================================================
// BSoundManager::handleNotification
//==============================================================================
void BSoundManager::handleNotification(AkUInt32 in_callbackType, AkPlayingID in_playingID, AkUniqueID	in_eventID, AkGameObjectID in_gameObj, void * in_pCookie, void * in_pData, AkUInt32 in_uDataSize)
{
}

//==============================================================================
// BSoundManager::eventCallbackFunc
//==============================================================================
void BSoundManager::eventCallbackFunc(AkUInt32 in_callbackType, AkPlayingID in_playingID, AkUniqueID	in_eventID, AkGameObjectID in_gameObj, void * in_pCookie, void * in_pData, AkUInt32 in_uDataSize)
{
   gSoundManager.handleNotification(in_callbackType, in_playingID, in_eventID, in_gameObj, in_pCookie, in_pData, in_uDataSize);
}

//==============================================================================
// BSoundManager::setRTPCValue
//==============================================================================
void BSoundManager::setRTPCValue(AkLpCtstr name, float value)
{
   AK::SoundEngine::SetRTPCValue(name, value, mPlayerObjects[cPlayer1]);
}

//==============================================================================
// BSoundManager::setRTPCValue
//==============================================================================
bool BSoundManager::addBank(const char* bankName)
{
   if(mNumBanksToLoad >= cMaxBanks)
      return false;

   mBanksToLoad[mNumBanksToLoad] = bankName;
   mNumBanksToLoad++;
   return true;

}