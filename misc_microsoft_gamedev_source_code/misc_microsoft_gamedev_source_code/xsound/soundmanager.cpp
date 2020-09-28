//==============================================================================
// soundmanager.cpp
//
// Copyright (c) 2006 Ensemble Studios
//==============================================================================

// Includes
#include "common.h"
#include "soundmanager.h"
#include "asyncFileManager.h"
#include "Wwise_IDs.h"
#include "consoleOutput.h"
#include "xmlreader.h"
#include "..\xcore\xcore.h"
#include "AkDefaultLowLevelIO.h"  //-- XFS IO
#include "CAkDefaultLowLevelIO.h" //-- Win32 IO
#include "CAKFilePackageLowLevelIO.h" //-- Win32 IO for sound package
#include "xsystem.h"
#include "xfs.h"
#include "xmp.h"

#include <AK/SoundEngine/Common/AkSoundEngine.h>				// Sound Engine
#include <AK/MusicEngine/Common/AkMusicEngine.h>            // Music Engine
#include <AK/SoundEngine/Common/AkMemoryMgr.h>					// Memory Manager
#include <AK/SoundEngine/Common/IAkStreamMgr.h>					// Streaming Manager
#include <AK/SoundEngine/Platforms/XBox360/AkModule.h>			// Default memory and stream managers
#include <AK/SoundEngine/Platforms/XBox360/AkXBox360SoundEngine.h> // Platform-specific structures
#include <AK/Tools/XBox360/AkPlatformFuncs.h> // Platform-specific structures
#include <AK/SoundEngine/Common/AkQueryParameters.h>


#include <AK/Plugin/AkSilenceSourceFactory.h>
#include <AK/Plugin/AkReverbFXFactory.h>
#include <AK/Plugin/AkReverbLiteFXFactory.h>
#include <AK/Plugin/AkParametricEQFXFactory.h>
#include <AK/Plugin/AkPeakLimiterFXFactory.h>
#include <AK/Plugin/AkDelayFXFactory.h>
#include <AK/Plugin/AkCompressorFXFactory.h>
#include <AK/Plugin/AkToneSourceFactory.h>


#ifndef AK_OPTIMIZED
#include <AK/Comm/CommunicationCentralFactory.h>
#include <AK/Comm/ICommunicationCentral.h>
#include <AK/Comm/ProxyFrameworkFactory.h>
#include <AK/Comm/IProxyFrameworkConnected.h>
#endif

#ifndef BUILD_FINAL
#include "..\xsystem\xsystem.h"
#include "..\xsystem\config.h"
#include "..\xsystem\econfigenum.h"
#include "..\xsystem\fileUtils.h"
#include "string\bsnprintf.h"
#include "stream\byteStream.h"
#include "..\xgame\gamedirectories.h"
#endif

#include "..\xcore\threading\lightWeightMutex.h"

// Global items related to communication between Wwise and the game
#ifndef AK_OPTIMIZED
AkMemPoolId g_poolComm = AK_INVALID_POOL_ID;
AK::Comm::ICommunicationCentral * g_pCommCentral = NULL;         
AK::Comm::IProxyFrameworkConnected * g_pProxyFramework = NULL;
#define COMM_POOL_SIZE          (256 * 1024)
#define COMM_POOL_BLOCK_SIZE    (48)
#endif

#ifndef INVALID_FILE_ATTRIBUTES
   #define INVALID_FILE_ATTRIBUTES ((DWORD)-1)
#endif

// Globals
BSoundManager gSoundManager;
static float gSoundScapeScale = 1;

// Consts
static const BUString cSoundBankDir = "game:\\sound\\wwise_material\\GeneratedSoundBanks\\XBox360\\";
static const BUString cSoundBankInitFile = "init.bnk";
static const BUString cSoundBankPackageFile = "Sounds.pck";


#ifndef BUILD_FINAL
static unsigned long soundPhysMemAllocated=0;
static long soundNumPhysAllocations=0;
static unsigned long soundVirtMemAllocated=0;
static long soundNumVirtAllocations=0;
static unsigned long soundEngineMemAllocated=0;
static long soundNumEngineAllocations=0;
#endif

static const long    cMinWWiseID = 10; //-- 0, 1 or reserved, using 2-9 for const objects
static const long    cMaxWWiseID = 450 + cMinWWiseID;

BLightWeightMutex BSoundManager::mMutex;
namespace AK
{
//==============================================================================
// AllocHook
//==============================================================================
void * AllocHook( size_t in_size )
{
#ifndef BUILD_FINAL
   soundEngineMemAllocated+=in_size;
   soundNumEngineAllocations++;
#endif
   // Allocate the memory
   return gPrimaryHeap.New(in_size);
}

//==============================================================================
// FreeHook
//==============================================================================
void FreeHook( void * in_ptr )
{
   // Deallocate.
   if(in_ptr)
   {
#ifndef BUILD_FINAL
      int size=0;
      gPrimaryHeap.Details(in_ptr, &size);
      soundEngineMemAllocated-=size;         
      soundNumEngineAllocations--;
#endif
      gPrimaryHeap.Delete((void*)in_ptr);
   }
}

//==============================================================================
// VirtualAllocHook
//==============================================================================
void * VirtualAllocHook(void * in_pMemAddress, size_t in_size, DWORD in_dwAllocationType, DWORD in_dwProtect)
{
#ifndef BUILD_FINAL
   soundVirtMemAllocated += in_size;
   soundNumVirtAllocations++;
#endif

   return VirtualAlloc( in_pMemAddress, in_size, in_dwAllocationType, in_dwProtect );
}

//==============================================================================
// VirtualFreeHook
//==============================================================================
void VirtualFreeHook(void * in_pMemAddress, size_t in_size, DWORD in_dwFreeType)
{
#ifndef BUILD_FINAL
   soundVirtMemAllocated -= in_size;
   soundNumVirtAllocations--;
#endif
   VirtualFree( in_pMemAddress, in_size, in_dwFreeType );
}

//==============================================================================
// PhysicalAllocHook
//==============================================================================
void * PhysicalAllocHook(size_t in_size, ULONG_PTR in_ulPhysicalAddress, ULONG_PTR in_ulAlignment, DWORD in_dwProtect)
{
#ifndef BUILD_FINAL
   soundPhysMemAllocated += in_size;
   soundNumPhysAllocations++;
   //gConsoleOutput.output(cMsgFileManager, "sound phyical alloc:  %d (%f MB)", (long)in_size, ((float)in_size)/1048576.0f );
#endif
   return XPhysicalAlloc(in_size, in_ulPhysicalAddress, in_ulAlignment, in_dwProtect);
}

//==============================================================================
// PhysicalFreeHook
//==============================================================================
void	PhysicalFreeHook(void * in_pMemAddress)
{
#ifndef BUILD_FINAL
   soundPhysMemAllocated -= XPhysicalSize(in_pMemAddress);
   soundNumPhysAllocations--;
   //gConsoleOutput.output(cMsgFileManager, "sound phyical free:   %d (%f MB)", XPhysicalSize(in_pMemAddress), ((float)XPhysicalSize(in_pMemAddress))/1048576.0f );
#endif
   XPhysicalFree(in_pMemAddress);
}  
}

//==============================================================================
// BVectorToAkVector
//==============================================================================
static AkVector BVectorToAkVector(BVector rhs)
{
   AkVector lhs;
   lhs.X = rhs.x/gSoundScapeScale;
   lhs.Y = rhs.y/gSoundScapeScale;
   lhs.Z = rhs.z/gSoundScapeScale;
   return lhs;
}

//==============================================================================
// AKVectorToBVector
//==============================================================================
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
BSoundManager::BSoundManager() : 
   mpWin32SoundIO(NULL), 
   mpXFSSoundIO(NULL), 
   mMute(false), 
   mIsInitialized(false), 
   mSplitScreen(false), 
#ifndef BUILD_FINAL
   mOutputBaseTime(0),
   mbSoundCueOutput(false),
#endif
   mGameStartDelay(false),
   mpSoundInfoProvider(NULL),
   mPreGamePlaying(false),
   mInGamePlaying(false)

{
   mPlayerObjects[cPlayer1] = cWWisePlayer1ID;
   mPlayerObjects[cPlayer2] = cWWisePlayer2ID;   

   //-- Listen to our own events (this is kinda weird but cleaner than calling ourself
   addEventHandler(this);
}

//==============================================================================
// BSoundManager::~BSoundManager
//==============================================================================
BSoundManager::~BSoundManager()
{
   if(mpWin32SoundIO)
      delete mpWin32SoundIO;

   if(mpXFSSoundIO)
      delete mpXFSSoundIO;
}

//==============================================================================
// BSoundManager::setup
//==============================================================================
bool BSoundManager::setup()
{
   BASSERT(mpSoundInfoProvider);
   if(!mpSoundInfoProvider)
      return false;

   if (!loadXML())
      return false;

   bool win32Works = false;

   //-- Build init string
   BString fullInitPath = cSoundBankDir;
   fullInitPath.append(cSoundBankInitFile);
   
   DWORD fullAttributes = GetFileAttributes(fullInitPath.getPtr());
   if ((fullAttributes == INVALID_FILE_ATTRIBUTES) || (fullAttributes & FILE_ATTRIBUTE_DIRECTORY))
   {
      fullInitPath = cSoundBankDir;
      fullInitPath.append(cSoundBankPackageFile);
      fullAttributes = GetFileAttributes(fullInitPath.getPtr());
   }
   
   if ((fullAttributes == INVALID_FILE_ATTRIBUTES) || (fullAttributes & FILE_ATTRIBUTE_DIRECTORY))      
      win32Works = false;
   else
      win32Works = true;


   //-- Use win32 IO directly if we can access the file, and: 
   //-- The config is set, OR XFS is inactive (running off the harddrive), OR if archives are enabled.
   if (win32Works && 
       (gConfig.isDefined(cConfigUseWin32SoundIO) || (gXFS.isActive() == false) || (gFileManager.getConfigFlags() & BFileManager::cWillBeUsingArchives))
      )
   {
      if(!mpWin32SoundIO)
      {
            mpWin32SoundIO = new CAkFilePackageLowLevelIO();         
      }
      
      gConsoleOutput.printf("BSoundManager::setup: Using Win32 Sound I/O");
   }
   else
   {
      if(!mpXFSSoundIO)
      {
         mpXFSSoundIO = new BAkDefaultLowLevelIO();
         //mpXFSSoundIO->setSoundDirID(mpSoundInfoProvider->getSoundDirID());
      }
      
      gConsoleOutput.printf("BSoundManager::setup: Using XFS Sound I/O");
   }
   
   //Load up the ids of all the chatter events because they have to go through a special prep stage when being played
   loadChatterIDs();

   return true;
}

//==============================================================================
// BSoundManager::loadXml
//==============================================================================
bool BSoundManager::loadXML()
{
   //-- Load our sound banks from soundinfo.xml
   //-- FIXME: rg [6/20/07] Can you change the sound code so it only loads soundInfo.xml one time (ever)?
   BXMLReader reader;
   if(!reader.load(mpSoundInfoProvider->getDataDirID() , "soundInfo.xml"))
      return false;

   mXmlStaticBanks.clear();

   BString bankName;
   BXMLNode root(reader.getRootNode());
   long nodeCount = root.getNumberChildren();
   for(long i=0; i<nodeCount; i++)
   {
      BXMLNode node(root.getChild(i));
      BPackedString name(node.getName());
      if(name=="SoundBanks")
      {
         long numBanks = node.getNumberChildren();
         for(long j=0; j<numBanks; j++)
         {
            BXMLNode bankNode(node.getChild(j));
            name = bankNode.getName();
            if(name == "StaticBank")
            {               
               bankNode.getText(bankName);
               mXmlStaticBanks.add(bankName);
            }
            if(name == "PregameBank")
            {
               bankNode.getText(mPreGameBank);               
            }
         }
      }
      if(name=="Sounds")
      {
         long numSounds = node.getNumberChildren();
         for(long j=0; j<numSounds; j++)
         {
            BXMLNode soundNode(node.getChild(j));
            BString soundEnumType;
            bool result = soundNode.getAttribValueAsString("name", soundEnumType);
            if(result)
            {        
               uint soundCueEnum = cSoundCueNone;
               if(soundEnumType ==      "SoundLocalMilitary")
                  soundCueEnum = cSoundLocalMilitary;
               else if(soundEnumType == "SoundGlobalMilitary")
                  soundCueEnum = cSoundGlobalMilitary;
               else if(soundEnumType == "SoundHotkeyBase")
                  soundCueEnum = cSoundHotkeyBase;
               else if(soundEnumType == "SoundHotkeyNode")
                  soundCueEnum = cSoundHotkeyNode;
               else if(soundEnumType == "SoundFlare")
                  soundCueEnum = cSoundFlare;
               else if(soundEnumType == "SoundFlareLook")
                  soundCueEnum = cSoundFlareLook;
               else if(soundEnumType == "SoundFlareHelp")
                  soundCueEnum = cSoundFlareHelp;
               else if(soundEnumType == "SoundFlareMeet")
                  soundCueEnum = cSoundFlareMeet;
               else if(soundEnumType == "SoundFlareAttack")
                  soundCueEnum = cSoundFlareAttack;
               else if(soundEnumType == "SoundSupportPowerAvailable")
                  soundCueEnum = cSoundSupportPowerAvailable;               
               else if(soundEnumType == "SoundAttackNotificationUNSCSev1")
                  soundCueEnum = cSoundAttackNotificationUNSCSev1;
               else if(soundEnumType == "SoundAttackNotificationUNSCBaseSev1")
                  soundCueEnum = cSoundAttackNotificationUNSCBaseSev1;
               else if(soundEnumType == "SoundAttackNotificationCovSev1")
                  soundCueEnum = cSoundAttackNotificationCovSev1;
               else if(soundEnumType == "SoundAttackNotificationCovBaseSev1")
                  soundCueEnum = cSoundAttackNotificationCovBaseSev1;
               else if(soundEnumType == "SoundResearchComplete")
                  soundCueEnum = cSoundResearchComplete;
               else if(soundEnumType == "SoundPlayerResigned")
                  soundCueEnum = cSoundPlayerResigned;
               else if(soundEnumType == "SoundPlayerDefeated")
                  soundCueEnum = cSoundPlayerDefeated;
               else if(soundEnumType == "SoundGameWon")
                  soundCueEnum = cSoundGameWon;
               else if(soundEnumType == "SoundPlaybackDone")
                  soundCueEnum = cSoundPlaybackDone;
               else if(soundEnumType == "SoundGroupSelect1")
                  soundCueEnum = cSoundGroupSelect1;
               else if(soundEnumType == "SoundGroupSelect2")
                  soundCueEnum = cSoundGroupSelect2;
               else if(soundEnumType == "SoundGroupSelect3")
                  soundCueEnum = cSoundGroupSelect3;
               else if(soundEnumType == "SoundGroupSelect4")
                  soundCueEnum = cSoundGroupSelect4;
               else if(soundEnumType == "SoundGroupCreate1")
                  soundCueEnum = cSoundGroupCreate1;
               else if(soundEnumType == "SoundGroupCreate2")
                  soundCueEnum = cSoundGroupCreate2;
               else if(soundEnumType == "SoundGroupCreate3")
                  soundCueEnum = cSoundGroupCreate3;
               else if(soundEnumType == "SoundGroupCreate4")
                  soundCueEnum = cSoundGroupCreate4;
               else if(soundEnumType == "SoundBattleLost")
                  soundCueEnum = cSoundBattleLost;
               else if(soundEnumType == "SoundBattleWon")
                  soundCueEnum = cSoundBattleWon;
               else if(soundEnumType == "SoundWinningBattle")
                  soundCueEnum = cSoundWinningBattle;
               else if(soundEnumType == "SoundLosingBattle")
                  soundCueEnum = cSoundLosingBattle;
               else if(soundEnumType == "SoundStopAll")
                  soundCueEnum = cSoundStopAll;
               else if(soundEnumType == "SoundStopAllExceptMusic")
                  soundCueEnum = cSoundStopAllExceptMusic;
               else if(soundEnumType == "SoundTributeReceived")
                  soundCueEnum = cSoundTributeReceived;
               else if(soundEnumType == "SoundSwitchArcadia")
                  soundCueEnum = cSoundSwitchArcadia;
               else if(soundEnumType == "SoundSwitchHarvest")
                  soundCueEnum = cSoundSwitchHarvest;
               else if(soundEnumType == "SoundSwitchSWI")
                  soundCueEnum = cSoundSwitchSWI;
               else if(soundEnumType == "SoundSwitchSWE")
                  soundCueEnum = cSoundSwitchSWE;
               else if(soundEnumType == "SoundMusicSwitchArcadia")
                  soundCueEnum = cSoundMusicSwitchArcadia;
               else if(soundEnumType == "SoundMusicSwitchHarvest")
                  soundCueEnum = cSoundMusicSwitchHarvest;
               else if(soundEnumType == "SoundMusicSwitchSWI")
                  soundCueEnum = cSoundMusicSwitchSWI;
               else if(soundEnumType == "SoundMusicSwitchSWE")
                  soundCueEnum = cSoundMusicSwitchSWE;
               else if(soundEnumType == "SoundObjectiveOnDisplay")
                  soundCueEnum = cSoundObjectiveOnDisplay;               
               else if(soundEnumType == "SoundMuteMusic")
                  soundCueEnum = cSoundMuteMusic;               
               else if(soundEnumType == "SoundUnmuteMusic")
                  soundCueEnum = cSoundUnmuteMusic;
               else if(soundEnumType == "SoundMuteMasterBus")
                  soundCueEnum = cSoundMuteMasterBus;               
               else if(soundEnumType == "SoundUnmuteMasterBus")
                  soundCueEnum = cSoundUnmuteMasterBus;                         

               //-- Music
               else if(soundEnumType == "SoundMusicPlayPreGame")
                  soundCueEnum = cSoundMusicPlayPreGame;        
               else if(soundEnumType == "SoundMusicStopPreGame")
                  soundCueEnum = cSoundMusicStopPreGame;        
               else if(soundEnumType == "SoundMusicSetStateMainTheme")
                  soundCueEnum = cSoundMusicSetStateMainTheme;        
               else if(soundEnumType == "SoundMusicSetStateCampaignMenu")
                  soundCueEnum = cSoundMusicSetStateCampaignMenu;        
               else if(soundEnumType == "SoundMusicSetStateSkirmishMenu")
                  soundCueEnum = cSoundMusicSetStateSkirmishMenu;              
               else if(soundEnumType == "SoundMusicPlayInGame")
                  soundCueEnum = cSoundMusicPlayInGame;        
               else if(soundEnumType == "SoundMusicStopInGame")
                  soundCueEnum = cSoundMusicStopInGame;        
               else if(soundEnumType == "SoundMusicGameWon")
                  soundCueEnum = cSoundMusicGameWon;        
               else if(soundEnumType == "SoundMusicGameLost")
                  soundCueEnum = cSoundMusicGameLost;     
               else if(soundEnumType == "SoundMusicStopPostGame")
                  soundCueEnum = cSoundMusicStopPostGame;

               else if(soundEnumType == "SoundDeflect")
                  soundCueEnum = cSoundDeflect;
               else if (soundEnumType == "StartBlur")
                  soundCueEnum = cSoundStartBlur;
               else if (soundEnumType == "SoundVOGNeedSupplies")
                  soundCueEnum = cSoundVOGNeedSupplies;
               else if (soundEnumType == "SoundVOGNeedReactors")
                  soundCueEnum = cSoundVOGNeedReactors;
               else if (soundEnumType == "SoundVOGNeedPop")
                  soundCueEnum = cSoundVOGNeedPop;
               else if (soundEnumType == "SoundVOGHeroDownForge")
                  soundCueEnum = cSoundVOGHeroDownForge;
               else if (soundEnumType == "SoundVOGHeroDownAnders")
                  soundCueEnum = cSoundVOGHeroDownAnders;
               else if (soundEnumType == "SoundVOGHeroDownSpartan")
                  soundCueEnum = cSoundVOGHeroDownSpartan;
               else if (soundEnumType == "SoundVOGHeroReviveForge")
                  soundCueEnum = cSoundVOGHeroReviveForge;
               else if (soundEnumType == "SoundVOGHeroReviveAnders")
                  soundCueEnum = cSoundVOGHeroReviveAnders;
               else if (soundEnumType == "SoundVOGHeroReviveSpartan")
                  soundCueEnum = cSoundVOGHeroReviveSpartan;

               else if (soundEnumType == "SoundVOGBaseDestroyed")
                  soundCueEnum = cSoundVOGBaseDestroyed;
               else if (soundEnumType == "SoundVOGLoss")
                  soundCueEnum = cSoundVOGLoss;
               else if (soundEnumType == "SoundVOGLossTeam")
                  soundCueEnum = cSoundVOGLossTeam;
               else if (soundEnumType == "SoundVOGWin")
                  soundCueEnum = cSoundVOGWin;
               else if (soundEnumType == "SoundVOGWinTeam")
                  soundCueEnum = cSoundVOGWinTeam;

               else if (soundEnumType == "SoundMusicSetStateSPCLost")
                  soundCueEnum = cSoundMusicSetStateSPCLost;

               //-- Store the sound cue index
               BASSERT(soundCueEnum != cSoundCueNone);    
               BString soundCueStr;
               soundNode.getText(soundCueStr);
               mXmlSoundCueStrings[soundCueEnum] = soundCueStr;
            }           
         }
      }
   }   

   return true;
}

//==============================================================================
// BSoundManager::akAssertHook
//==============================================================================
void BSoundManager::akAssertHook(const char * in_pszExpression, const char * in_pszFileName, int in_lineNumber )
{
   //BASSERT_TRIGGER_EXCEPTION(0, in_pszExpression, in_pszFileName, in_lineNumber); 

   //This may be causing thread contention problems.  Removing so that the game doesn't hard crash
   //gConsoleOutput.output(cMsgError, "WWise Assert: %s  %s  line %d", in_pszExpression, in_pszFileName, in_lineNumber );

   //BASSERT(0);
}

//==============================================================================
// BSoundManager::initSoundEngine
//==============================================================================
bool BSoundManager::initSoundEngine()
{
   // Create and initialize an instance of the default memory manager.
   AkMemSettings memSettings;
   memSettings.uMaxNumPools = 45;

   if ( AK::MemoryMgr::Init( &memSettings ) != AK_Success )
   {
      BASSERT( ! "Could not create the memory manager." );
      return false;
   }

   // Initialize our instance of the default File System/LowLevelIO. 
   AKRESULT eResult = AK_Fail;
   if(mpXFSSoundIO)
      eResult = mpXFSSoundIO->Init();
   else if(mpWin32SoundIO)
      eResult = mpWin32SoundIO->Init();

   if ( eResult != AK_Success )
   {
      BASSERT( !"Cannot initialize Low-Level IO" );
      return false;
   }
   
   // Streaming settings
   AkStreamMgrSettings stmSettings;
   stmSettings.uMemorySize = 32*1024;                          // 32 Kb for small objects memory.
   if(mpXFSSoundIO)                                            // Low-level IO hook.
      stmSettings.pLowLevelIO = mpXFSSoundIO;                     
   else if(mpWin32SoundIO)
      stmSettings.pLowLevelIO = mpWin32SoundIO;

   // Create and initialize an instance of our stream manager.
   AK::IAkStreamMgr * pStreamMgr = AK::CreateStreamMgr( &stmSettings );
   if ( ! pStreamMgr )
   {
      BASSERT( ! "Could not create the Streaming Manager." );
      return false;
   }

   // Create and init communications


   // Create default IO device.

   
   // Define properties for device thread
   AkThreadProperties deviceThreadProperties;
   deviceThreadProperties.nPriority = THREAD_PRIORITY_NORMAL;
   deviceThreadProperties.dwProcessor = 5;
   deviceThreadProperties.uStackSize = AK_DEFAULT_STACK_SIZE;

   // Device settings and heuristics
   AkDeviceSettings deviceSettings;
   deviceSettings.uIOMemorySize = 2*1024*1024;               // 2 Mb of memory for I/O.
   deviceSettings.uGranularity = 32*1024;                    // 32 Kb I/O granularity.
   deviceSettings.fTargetAutoStmBufferLength = 380;          // 380 ms buffering.
   deviceSettings.dwIdleWaitTime = AK_INFINITE;              // Does not stream passed target buffering length.
   if(mpWin32SoundIO)
      deviceSettings.uSchedulerTypeFlags = AK_SCHEDULER_DEFERRED_LINED_UP;
   else
      deviceSettings.uSchedulerTypeFlags = AK_SCHEDULER_DEFERRED_LINED_UP; 
   deviceSettings.pThreadProperties = &deviceThreadProperties;

   if(mpXFSSoundIO)
      eResult = mpXFSSoundIO->SetDevice( deviceSettings );
   else if(mpWin32SoundIO)
      eResult = mpWin32SoundIO->SetDevice( deviceSettings );
   if ( eResult != AK_Success )
   {
      BASSERT( !"Cannot set default IO device" );
      return true;
   }

   //-- Fill out the default init settings
   AkInitSettings initSettings;
   AK::SoundEngine::GetDefaultInitSettings(initSettings);
   initSettings.pfnAssertHook = akAssertHook;
   initSettings.uDefaultPoolSize = 1024*1024*4.0; //4.0MB

   //-- Fill out default platform settings
   AkPlatformInitSettings platformInitSettings;
   AK::SoundEngine::GetDefaultPlatformInitSettings(platformInitSettings);

   platformInitSettings.threadLEngine.dwProcessor = 5;  
   platformInitSettings.threadLEngine.nPriority = 0;
   platformInitSettings.threadBankManager.dwProcessor =5;
   platformInitSettings.threadBankManager.nPriority = 0;
   platformInitSettings.uLEngineDefaultPoolSize= 1024*1024*3.5; //3.5MB;

   // Create the sound engine
   if ( AK::SoundEngine::Init( &initSettings, &platformInitSettings) == NULL )
   {
      BASSERT(0);
      return false;
   }

   // Init the music engine
    AkMusicSettings musicInit;
    AK::MusicEngine::GetDefaultInitSettings( musicInit );
        
    if ( AK::MusicEngine::Init( &musicInit ) != AK_Success )
    {
        BASSERT(0);
        return false;
    }


   //
   // Initialize communications (not in release build!)
   //
#ifndef AK_OPTIMIZED

   //Init music proxy
   AK::ProxyMusic::Init();
 
   // Create the communications memory pool
   g_poolComm = AK::MemoryMgr::CreatePool(NULL, COMM_POOL_SIZE, COMM_POOL_BLOCK_SIZE, AkMalloc );
   assert( g_poolComm != AK_INVALID_POOL_ID );
   AK_SETPOOLNAME( g_poolComm, L"Communication" );

   // Create and initialize the Proxy Framework and Communication Central
   g_pProxyFramework = AkCreateProxyFramework( g_poolComm );
   g_pCommCentral  = AkCreateCommunicationCentral( g_poolComm );

   g_pCommCentral->Init( g_pProxyFramework, g_pProxyFramework );
   g_pProxyFramework->Init();
   g_pProxyFramework->SetNotificationChannel( g_pCommCentral->GetNotificationChannel() );

#endif

   //-- Register the necessary plug-ins
   AK::SoundEngine::RegisterPlugin(AkPluginTypeSource, AKCOMPANYID_AUDIOKINETIC, AKSOURCEID_SILENCE, CreateSilenceSource, CreateSilenceSourceParams);
   AK::SoundEngine::RegisterPlugin(AkPluginTypeEffect, AKCOMPANYID_AUDIOKINETIC, AKEFFECTID_REVERB, CreateReverbFX, CreateReverbFXParams);
   AK::SoundEngine::RegisterPlugin(AkPluginTypeEffect, AKCOMPANYID_AUDIOKINETIC, AKEFFECTID_PARAMETRICEQ, CreateParametricEQFX, CreateParametricEQFXParams);
   AK::SoundEngine::RegisterPlugin(AkPluginTypeEffect, AKCOMPANYID_AUDIOKINETIC, AKEFFECTID_PEAKLIMITER, CreatePeakLimiterFX, CreatePeakLimiterFXParams);
   AK::SoundEngine::RegisterPlugin(AkPluginTypeEffect, AKCOMPANYID_AUDIOKINETIC, AKEFFECTID_DELAY, CreateDelayFX, CreateDelayFXParams);
   AK::SoundEngine::RegisterPlugin(AkPluginTypeEffect, AKCOMPANYID_AUDIOKINETIC, AKEFFECTID_REVERBLITE, CreateReverbLiteFX, CreateReverbLiteFXParams);
   AK::SoundEngine::RegisterPlugin(AkPluginTypeEffect, AKCOMPANYID_AUDIOKINETIC, AKEFFECTID_COMPRESSOR, CreateCompressorFX, CreateCompressorFXParams);
   AK::SoundEngine::RegisterPlugin(AkPluginTypeSource, AKCOMPANYID_AUDIOKINETIC, AKSOURCEID_TONE, CreateToneSource, CreateToneSourceParams);

   //-- Set the language path
   if(mpXFSSoundIO)
   {
      mpXFSSoundIO->SetLangSpecificDirName(L"English(US)\\" );
      mpXFSSoundIO->SetBasePath(L"sound\\wwise_material\\GeneratedSoundBanks\\XBox360\\");
   }
   else if(mpWin32SoundIO)
   {
      mpWin32SoundIO->SetLangSpecificDirName(L"English(US)\\" );
      mpWin32SoundIO->SetBasePath(L"game:\\sound\\wwise_material\\GeneratedSoundBanks\\XBox360\\");
   }

   //-- Load the static banks
   loadStaticSoundBanks();

   //-- Add the UI game object
   AK::SoundEngine::RegisterGameObj(mPlayerObjects[cPlayer1], "Player1");
   AK::SoundEngine::RegisterGameObj(mPlayerObjects[cPlayer2], "Player2");
#ifndef SOUND_REREGISTER_OBJECTS
   //If the game objects are kept persistent, then create them in engine init
   for( int i=cMinWWiseID+1; i<=cMaxWWiseID; i++ )
      AK::SoundEngine::RegisterGameObj(i);
#endif

   //-- Set player 1 to our listener 
   AkSoundPosition noUse;
   AK::SoundEngine::SetPosition(mPlayerObjects[cPlayer1], noUse, 0); 

   //-- Set player one to our active listener
   AK::SoundEngine::SetActiveListeners(mPlayerObjects[cPlayer1], 0x1 );


   //-- Set default options values
   setVolumeMusic(255);
   setVolumeSFX(255);
   setVolumeVoice(255);

   return true;
}

//==============================================================================
// BSoundManager::loadStaticSoundBanks
//==============================================================================
void BSoundManager::loadStaticSoundBanks(bool loadInit)
{   
   AkBankID bankID;
   AKRESULT result;
   //bool banksLoaded = true;
   BUString bankName = cSoundBankInitFile;
   
   //-- If we're using packaged file then load it now so we can create the LUT   
   BUString packageStr = cSoundBankPackageFile;
   if(mpWin32SoundIO)
   {
      result = mpWin32SoundIO->LoadFilePackage(packageStr.getPtr());
      //BString msg;
      //msg.format("Failed to load sound package: %s", packageStr.getPtr());
      //BASSERTM(result == AK_Success, msg);
   }

   //-- Load the init bank
   if( loadInit )
   {
#ifndef BUILD_FINAL
      AK::MemoryMgr::PoolStats poolStats;
      AK::MemoryMgr::GetPoolStats(2, poolStats);
      //gConsoleOutput.fileManager("%18s pre load     %6d allocs  %6d used  %8d reserved", BString(bankName).getPtr(), poolStats.uAllocs - poolStats.uFrees, poolStats.uUsed, poolStats.uReserved );
#endif
      result = AK::SoundEngine::LoadBank( bankName.getPtr(), AK_DEFAULT_POOL_ID, bankID );
      if(result != AK_Success)
      {
         gConfig.define(cConfigNoSound);
         return;
      }
#ifndef BUILD_FINAL
      AK::MemoryMgr::PoolStats poolStats2;
      AK::MemoryMgr::GetPoolStats(2, poolStats2);
      gConsoleOutput.fileManager("%18s loaded       %6d deltaMem %6d allocs  %6d used  %8d reserved", BString(bankName).getPtr(), poolStats2.uUsed - poolStats.uUsed, poolStats2.uAllocs - poolStats2.uFrees, poolStats2.uUsed, poolStats2.uReserved );
#endif
      //BASSERTM(result == AK_Success, BString(bankName).getPtr());               
   }

   mIsInitialized = true;  

   // actually load up the banks from the xml data
   for (uint i = 0; i < mXmlStaticBanks.getSize(); ++i)
      loadSoundBank(mXmlStaticBanks[i], false);
   
   // setup the cue indexes from the xml data
   for (uint i = 0; i < cSoundMax; ++i)
   {
      if(mXmlSoundCueStrings[i].isEmpty() == false)
      {
         BCueIndex cue = gSoundManager.getCueIndex(mXmlSoundCueStrings[i]);                  
         BASSERT(cue != cInvalidCueIndex);
         mSoundCues[i] = cue;
      }
   }
}

//==============================================================================
// BSoundManager::loadChatterIDs
// Load up the ids of all the chatter events because they have to go through a special prep stage when being played
//==============================================================================
void BSoundManager::loadChatterIDs()
{   
   mChatterIDArray.clear();

   BXMLReader reader;
   if(!reader.load(mpSoundInfoProvider->getDataDirID() , "chattertable.xml"))
      return;

   uint32 index;

   BXMLNode root(reader.getRootNode());
   long nodeCount = root.getNumberChildren();
   for(long i=0; i<nodeCount; i++)
   {
      BXMLNode node(root.getChild(i));
      BPackedString name(node.getName());
      if(name=="ID")
      {
         node.getTextAsUInt32(index);
         mChatterIDArray.add(index);
      }
   }
}

#ifdef SOUND_RELOAD_BANKS
//==============================================================================
// BSoundManager::unloadAllSoundBanks
//==============================================================================
void BSoundManager::unloadAllSoundBanks()
{
   //-- Unload the loaded Sound Banks
   for(long i=0; i < mLoadedBanks.getNumber(); i++)
   {
      unloadSoundBank(mLoadedBanks[i].mBankID, false);
      i--;
   }
   mLoadedBanks.clear();
}
#endif

//==============================================================================
// BSoundManager::shutdown
//==============================================================================
void BSoundManager::shutdown()
{

   //-- Clear the event params
   {
      BScopedCriticalSection lock(mSoundEventParamsArrayLock);
      mSoundEventParamsArray.clear();
   }      

   //-- Add the UI game object
   AK::SoundEngine::UnregisterGameObj(mPlayerObjects[cPlayer1]);
   AK::SoundEngine::UnregisterGameObj(mPlayerObjects[cPlayer2]);
#ifndef SOUND_REREGISTER_OBJECTS
   //If the game objects are kept persistent, then remove them on shutdown
   for( int i=cMinWWiseID+1; i<=cMaxWWiseID; i++ )
      AK::SoundEngine::UnregisterGameObj(i);
#endif

   //-- Unload the loaded Sound Banks
   for(long i=0; i < mLoadedBanks.getNumber(); i++)
   {
      unloadSoundBank(mLoadedBanks[i].mBankID, false);
      i--;
   }
   mLoadedBanks.clear();

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
   if(AK::MemoryMgr::IsInitialized())
      AK::MemoryMgr::DestroyPool( g_poolComm );

#endif

   // Terminate the music engine
   AK::MusicEngine::Term();

   // Terminate the sound engine
   AK::SoundEngine::Term();

   // Terminate the streaming manager
   if ( AK::IAkStreamMgr::Get() )
   {   
      AK::IAkStreamMgr::Get()->Destroy();
   }

   if(mpXFSSoundIO)
      mpXFSSoundIO->Term();
   else if(mpWin32SoundIO)
      mpWin32SoundIO->Term();

   // Terminate the Memory Manager
   if(AK::MemoryMgr::IsInitialized())
      AK::MemoryMgr::Term();

   mIsInitialized = false;
  
}

//==============================================================================
// BSoundManager::update
//==============================================================================
void BSoundManager::update()
{
   if(AK::SoundEngine::IsInitialized() == false)
      return;
   
   //-- Render Audio
   AKRESULT result = AK_Fail;
   result = AK::SoundEngine::RenderAudio();
   BASSERT(result == AK_Success);

#ifndef BUILD_FINAL
   mSoundPlayback.update(); 
#endif

   //-- Process our sound events
   processSoundEvents();

   //-- Process communications to Wwise Editor
#ifndef AK_OPTIMIZED
   if(g_pCommCentral)
      g_pCommCentral->Process();
#endif
}

//==============================================================================
// BSoundManager::processSoundEvents
//==============================================================================
void BSoundManager::processSoundEvents(void)
{
   BSoundEventParamsArray soundEvents;

   {
      // Make a copy of the current sound events array in a thread safe manner.
      BScopedCriticalSection lock(mSoundEventParamsArrayLock);

      soundEvents = mSoundEventParamsArray;

      // Now delete all the current events (don't use clear() because we want to keep the memory block around).
      mSoundEventParamsArray.resize(0);
   }
   uint numEvents = soundEvents.getSize();
   uint numHandlers = mEventHandlers.getSize();
   for(uint j=0; j < numHandlers; j++)
   {
      for(uint i = 0; i < numEvents; i++)
      {
         mEventHandlers[j]->handleSoundEvent(soundEvents[i]); //-- This sends events to ourself and to the world sound manager
      }
   }
}

//==============================================================================
// BSoundManager::handleSoundEvent
//==============================================================================
void BSoundManager::handleSoundEvent(BSoundEventParams& params)
{
   if(params.eventType == cSoundEventStop)
   {
      if(mQueuedSoundEvents.getSize() > 0)
      {
         if(mQueuedSoundEvents[0] == params.in_eventID)
         {
            mQueuedSoundEvents.removeIndex(0);            
            if(mQueuedSoundEvents.getSize() > 0)
            {
               playCue(mQueuedSoundEvents[0]);
            }
         }
      }
   }
   else if(params.eventType == cSoundEventPrepare)
   {
      //gConsoleOutput.fileManager("prepare returned");

      //If a PrepareEvent just finished, then the event data is loaded and we need to play the event
      if(params.in_bankID != AK_INVALID_UNIQUE_ID)
      {
         //First, find the entry in the array to get the wiseID
         int i=0;
         int wiseID = -1; 
         for(; i<mPreparedEventArray.getNumber(); i++)
         {
            if ((mPreparedEventArray[i].eventID == params.in_bankID) && (mPreparedEventArray[i].posted == false))
            {
               wiseID = mPreparedEventArray[i].wiseObjID;
               break;
            }
         }
         //If we found it, lets post the event to play the sound.  Mark the entry as posted so it can be deleted later.
         if (wiseID >= 0)
         {
            if (params.in_status == AK_Success)
            {
               AkCallbackFunc callbackFuncPtr = &BSoundManager::callbackFunc;
               BCueHandle playingID = AK::SoundEngine::PostEvent(params.in_bankID, wiseID, AK_EndOfEvent, callbackFuncPtr);
               BASSERT(playingID != AK_INVALID_UNIQUE_ID);
               //gConsoleOutput.fileManager("  event posted: %10u %10u", params.in_bankID, wiseID);
            }
            else
            {
      #ifndef BUILD_FINAL
               if( params.in_status == AK_Cancelled )
                  gConsoleOutput.fileManager("  prepare failed: AK_Cancelled");
               else if( params.in_status == AK_IDNotFound )
                  gConsoleOutput.fileManager("  prepare failed: AK_IDNotFound");
               else if( params.in_status == AK_InsufficientMemory )
                  gConsoleOutput.fileManager("  prepare failed: AK_InsufficientMemory");
               else if( params.in_status == AK_BankReadError )
                  gConsoleOutput.fileManager("  prepare failed: AK_BankReadError");
               else if( params.in_status == AK_WrongBankVersion )
                  gConsoleOutput.fileManager("  prepare failed: AK_WrongBankVersion");
               else if( params.in_status == AK_InvalidFile )
                  gConsoleOutput.fileManager("  prepare failed: AK_InvalidFile");
               else if( params.in_status == AK_InvalidParameter )
                  gConsoleOutput.fileManager("  prepare failed: AK_InvalidParameter");
               else if( params.in_status == AK_Fail )
                  gConsoleOutput.fileManager("  prepare failed: AK_Fail");
               else 
                  gConsoleOutput.fileManager("  prepare failed: unknown error %10u", params.in_status);
               gConsoleOutput.fileManager("  eventID: %10u  wiseID: %10u", params.in_bankID, wiseID);
      #endif
            }
            //mark as posted even if there was an error.  it should be removed from our list.
            mPreparedEventArray[i].posted = true;
         }
         //else
         //   gConsoleOutput.fileManager("  wise id not found in list: %10u", wiseID);
      }
      //else
      //{
      //   gConsoleOutput.fileManager("  prepare returned invaild id");
      //}

   }
   else if(params.eventType == cSoundEventUnprepare)
   {
      //gConsoleOutput.fileManager("unprepare returned");
      //Nothing to do in this case yet.
   }

#ifndef BUILD_FINAL
   //In a debug build, lets show the memory use if this call back is from a load or prepare command.
   if((params.eventType == cSoundEventBankLoaded)||(params.eventType == cSoundEventPrepare)||(params.eventType == cSoundEventUnprepare))
   {
      AK::MemoryMgr::PoolStats poolStats;
      AK::MemoryMgr::GetPoolStats(2, poolStats);
      if((params.eventType == cSoundEventBankLoaded))
         gConsoleOutput.fileManager("%u bank loaded       %6d allocs  %6d used  %8d reserved", (long)params.in_bankID, poolStats.uAllocs - poolStats.uFrees, poolStats.uUsed, poolStats.uReserved );
      if((params.eventType == cSoundEventPrepare))
         gConsoleOutput.fileManager("%u event prepared    %6d allocs  %6d used  %8d reserved", (long)params.in_bankID, poolStats.uAllocs - poolStats.uFrees, poolStats.uUsed, poolStats.uReserved );
      if((params.eventType == cSoundEventUnprepare))
         gConsoleOutput.fileManager("%u event unprepared  %6d allocs  %6d used  %8d reserved", (long)params.in_bankID, poolStats.uAllocs - poolStats.uFrees, poolStats.uUsed, poolStats.uReserved );
   }
#endif

}

//==============================================================================
// BSoundManager::playCue
//==============================================================================
 BCueHandle BSoundManager::playCue(const char* cueName, BWwiseObjectID wiseObjectID, bool queue)
{
   if(AK::SoundEngine::IsInitialized() == false)
      return cInvalidCueHandle;

   if(BString(cueName).isEmpty())
      return cInvalidCueHandle;
   
   BCueIndex index = getCueIndex(cueName);
   if(index != cInvalidCueIndex)
   {
      return playCue(index, wiseObjectID, queue);
   }
   else
   {
#ifndef BUILD_FINAL   
      if(gConfig.isDefined(cConfigDebugSound))
      {   
         BString msg;
         msg.format("Invalid cue Name: %s", cueName);
         BASSERTM(index != cInvalidCueIndex, msg);            
      }
#endif
      gConsoleOutput.output(cMsgError, "Unable to play sound event: %s", cueName);
      return cInvalidCueHandle;
   }   
}

//==============================================================================
// BSoundManager::playCue
//==============================================================================
BCueHandle BSoundManager::playCue(BCueIndex cueIndex, BWwiseObjectID wiseObjectID, bool queue)
{
   if(AK::SoundEngine::IsInitialized() == false)
      return cInvalidCueHandle;

   if(cueIndex == cInvalidCueIndex)
   {
#ifndef BUILD_FINAL   
      if(gConfig.isDefined(cConfigDebugSound))
      {   
         BString msg;
         msg.format("Invalid CueIndex");
         BASSERTM(cueIndex != cInvalidCueIndex, msg);            
      }
#endif
      gConsoleOutput.output(cMsgError, "Invalid CueIndex passed to playCue");
      return cInvalidCueHandle;
   }

   if(mGameStartDelay)
   {
      BQueuedSoundInfo info;
      info.mCueIndex = cueIndex;
      info.mWiseObjectID = wiseObjectID;
      mQueuedSoundsToAddWhenGameStarts.add(info);
      return cInvalidCueHandle;
   }


   AkGameObjectID id = cInvalidWwiseObjectID;
   //-- If an object was specified, then register the object, and post the event
   if(wiseObjectID != cInvalidWwiseObjectID)
   {
      id = wiseObjectID;
      //AK::SoundEngine::RegisterGameObj(id);
      if (mSplitScreen)
         AK::SoundEngine::SetActiveListeners( id, 0x3 );
      else
         AK::SoundEngine::SetActiveListeners( id, 0x1 );
   }
   else
   {
      id = mPlayerObjects[cPlayer1];

      //-- If this sound is suppose to be queued, then queue it up if the queue is full
      if(queue)
      {
         if(mQueuedSoundEvents.getSize() > cMaxSoundQueueSize)
            return cInvalidCueHandle;
         else
         {            
            if(mQueuedSoundEvents.getSize() == 0)
            {
               BCueHandle handle = playCue(cueIndex, wiseObjectID, false);
               if(handle != AK_INVALID_PLAYING_ID)
                  mQueuedSoundEvents.add(cueIndex);

               return handle;
            }
            else
               return cInvalidCueHandle;
         }
      }
   }

   //Check to see if this is a special chatter event that has to be prepared.  If not, post the event
   if( !mChatterIDArray.contains(cueIndex) )
   {
      AkCallbackFunc callbackFuncPtr = &BSoundManager::callbackFunc;
      BCueHandle playingID = AK::SoundEngine::PostEvent(cueIndex, id, AK_EndOfEvent, callbackFuncPtr);
#ifndef BUILD_FINAL      
      if( mbSoundCueOutput )
      {
         AK::MemoryMgr::PoolStats poolStats;
         AK::MemoryMgr::GetPoolStats(2, poolStats);
         gConsoleOutput.fileManager("cue %10u %10u %10u     %d %d", cueIndex, id, timeGetTime() - mOutputBaseTime, poolStats.uAllocs - poolStats.uFrees, poolStats.uUsed );
      }
#endif
      if(playingID == AK_INVALID_PLAYING_ID)
      {
#ifndef BUILD_FINAL
         if(gConfig.isDefined(cConfigDebugSound))
         {   
            BString cueName = "unknown";
            for(int i=0; i<mNameAndHashPairArray.getNumber(); i++)
            {
               if(mNameAndHashPairArray[i].cueIndex == cueIndex)
               {
                  cueName = mNameAndHashPairArray[i].cueName;
                  break;
               }
            }

            BString msg;
            BString buffer;
            msg.format("**Playing Invalid CueIndex %u: %s", cueIndex, cueName.asANSI(buffer));
            gConsoleOutput.output(cMsgError, msg);
            BASSERTM(playingID != AK_INVALID_PLAYING_ID, msg);            
         }
#endif
         return cInvalidCueHandle;
      }
      return playingID;
   }
   else
   {
      //We must prepare(load) the event before we can play it

      //First, unprepare the oldest event 
      if (mPreparedEventArray.getNumber() > 1)
      {
         if(mPreparedEventArray[0].posted)
         {
            AkBankCallbackFunc callbackFuncPtr = &BSoundManager::unprepareEventCallbackFunc;
            AkUniqueID idToPrepare = mPreparedEventArray[0].eventID;
            AkUniqueID *pEventsIDArray = &idToPrepare;
            AKRESULT eResult = AK::SoundEngine::PrepareEvent( AK::SoundEngine::Preparation_Unload, pEventsIDArray, 1, callbackFuncPtr, NULL  ); // 1 is the array size
            if( eResult == AK_Success )
            {
               //gConsoleOutput.fileManager("Event popped %10u", idToPrepare);
               mPreparedEventArray.removeIndex(0, true);
            }
            //else
            //{
            //   gConsoleOutput.fileManager("Unprepare failed %10u", idToPrepare);
            //}
         }
         //else
         //{
         //   gConsoleOutput.fileManager("sound never posted %10u", mPreparedEventArray[0].eventID);
         //}
      }
      //If we weren't able to unprepare the top one, we cannot add this new one.  The sound will be dropped.
      if (mPreparedEventArray.getNumber() > 1)
      {
         //gConsoleOutput.fileManager("Unable to prepare cue: %10u", cueIndex);
         //for(int i=0; i<mPreparedEventArray.getNumber(); i++)
         //   gConsoleOutput.fileManager("   %d %10u %d %10u ", i, mPreparedEventArray[i].eventID, (mPreparedEventArray[i].posted != 0)? 1: 0, mPreparedEventArray[i].wiseObjID);
         return cInvalidCueHandle;
      }

      //Now prepare the new event
      AkBankCallbackFunc callbackFuncPtr = &BSoundManager::prepareEventCallbackFunc;
      AkUniqueID idToPrepare = cueIndex;
      AkUniqueID *pEventsIDArray = &idToPrepare;
      // Preparing an event:
      AKRESULT eResult = AK::SoundEngine::PrepareEvent( AK::SoundEngine::Preparation_Load, pEventsIDArray, 1, callbackFuncPtr, NULL ); // 1 is the array size
      if( eResult == AK_Success )
      {
         BSoundPrepareEventData newEvent;
         newEvent.eventID = cueIndex;
         newEvent.wiseObjID = id;
         newEvent.posted = false;
         mPreparedEventArray.push_back(newEvent);
         //gConsoleOutput.fileManager("Event pushed %10u %10u", newEvent.eventID, newEvent.wiseObjID);
      }
      BASSERT(eResult==AK_Success);
      return cInvalidCueHandle;
   }
}

//==============================================================================
// BSoundManager::getCueIndex
//==============================================================================
BCueIndex BSoundManager::getCueIndex(const char* cueName)
{
   BString tempString = cueName;
   if(tempString.isEmpty())
      return cInvalidCueIndex;

   BUString tempStringU;
   AkLpCtstr name = tempString.asUnicode(tempStringU);

   BCueIndex hashedCueIndex = AK::SoundEngine::GetIDFromString(name);

#ifndef BUILD_FINAL   
   if(gConfig.isDefined(cConfigDebugSound))
   {   
      //Save off the pairing of anything requested in here.  Even if it is a bogus event, it will get back a valid hash number(cueIndex).
      //When we go to play that cue index in this debug mode, we want to be able to spit out the bad name.
      int i=0;
      for( ; i<mNameAndHashPairArray.getNumber(); i++)
      {
         if(mNameAndHashPairArray[i].cueIndex == hashedCueIndex)
            break;
      }
      if( i == mNameAndHashPairArray.getNumber() )  //not found
      {
         BSoundNameAndHashPair newPair;
         newPair.cueIndex = hashedCueIndex;
         newPair.cueName = tempString;
         mNameAndHashPairArray.add(newPair);
      }

      if(!tempString.contains("_xtnd"))
      {
         BCueIndex index = mpSoundInfoProvider->getSoundCueIndex(cueName);
         if(index == cInvalidCueIndex)
         {
            BString msg;
            msg.format("Cue Name not found in soundtable.xml: %s", cueName);
            BASSERTM(index != cInvalidCueIndex, msg);            
            gConsoleOutput.output(cMsgError, msg);
         }    
         else if(hashedCueIndex != index)
         {
            BString msg;
            msg.format("Cue index value differs from soundTable: %s", cueName);
            BASSERTM(hashedCueIndex == index, msg);
            gConsoleOutput.output(cMsgError, msg);
         }
      }
   }
#endif

   return hashedCueIndex;
}

//==============================================================================
// BSoundManager::updateListener
//==============================================================================
void BSoundManager::updateListener(int listener, const BVector& pos, const BVector& forward, const BVector& up, const BVector& velocity)
{
   if(AK::SoundEngine::IsInitialized() == false)
      return;

   AkListenerPosition listenerPosition;
   listenerPosition.OrientationFront = BVectorToAkVector(forward);
   listenerPosition.OrientationTop = BVectorToAkVector(up);
   listenerPosition.Position = BVectorToAkVector(pos);
   //listenerPosition.Velocity = BVectorToAkVector(velocity);

   BDEBUG_ASSERT(Math::IsValidFloat(pos.x));
   BDEBUG_ASSERT(Math::IsValidFloat(pos.y));
   BDEBUG_ASSERT(Math::IsValidFloat(pos.z));

   BDEBUG_ASSERT(Math::IsValidFloat(forward.x));
   BDEBUG_ASSERT(Math::IsValidFloat(forward.y));
   BDEBUG_ASSERT(Math::IsValidFloat(forward.z));

   BDEBUG_ASSERT(Math::IsValidFloat(up.x));
   BDEBUG_ASSERT(Math::IsValidFloat(up.y));
   BDEBUG_ASSERT(Math::IsValidFloat(up.z));

   AK::SoundEngine::SetListenerPosition( listenerPosition, listener );   
}

//==============================================================================
// BSoundManager::updateSound3D
//==============================================================================
void BSoundManager::updateSound3D(BWwiseObjectID wiseObjectID, BVector pos, BVector orientation)
{
   if(AK::SoundEngine::IsInitialized() == false)
      return;

   BDEBUG_ASSERT(wiseObjectID != cInvalidWwiseObjectID);
   if(wiseObjectID == cInvalidWwiseObjectID)
      return;

   BDEBUG_ASSERT(Math::IsValidFloat(pos.x));
   BDEBUG_ASSERT(Math::IsValidFloat(pos.y));
   BDEBUG_ASSERT(Math::IsValidFloat(pos.z));   

   AkSoundPosition position;
   position.Position = BVectorToAkVector(pos);
   position.Orientation.X = orientation.x;
   position.Orientation.Y = orientation.y;
   position.Orientation.Z = orientation.z;

   //position.Velocity.X = 0.0f;
   //position.Velocity.Y = 0.0f;
   //position.Velocity.Z = 0.0f;

   AK::SoundEngine::SetPosition(wiseObjectID, position);
}

//==============================================================================
// BSoundManager::updateFOWvolume
//==============================================================================
void BSoundManager::updateFOWvolume(long wiseObjectID, float &currentFOWVolume, bool visible, double updateDelta, float fadeInRate, float fadeOutRate)
{
   if(AK::SoundEngine::IsInitialized() == false)
      return;

   BDEBUG_ASSERT(wiseObjectID != -1);
   if(wiseObjectID == -1)
      return;

   float previousFOWVolume = currentFOWVolume;

   if( visible && (currentFOWVolume == -1.0f) ) //uninitialized and we are visible, so start at full volume.
      currentFOWVolume = 100.0f;

   if( visible )
      currentFOWVolume += static_cast<float>((updateDelta / fadeInRate));  //rate is unit change per second.
   else
      currentFOWVolume -= static_cast<float>((updateDelta / fadeOutRate));

   if( currentFOWVolume < 0.0f )
      currentFOWVolume = 0.0f;
   if( currentFOWVolume > 100.0f )
      currentFOWVolume = 100.0f;

   if( currentFOWVolume != previousFOWVolume )
   {
      AK::SoundEngine::SetRTPCValue(AK::GAME_PARAMETERS::FOW_DISTANCE, currentFOWVolume, wiseObjectID);
      //gConsoleOutput.debug("%10u  wise %d  volume %f", timeGetTime(), wiseObjectID, currentFOWVolume);
   }
}

//==============================================================================
// BSoundManager::updateEnginePitch
//==============================================================================
void BSoundManager::updateEnginePitch(long wiseObjectID, float &currentEnginePitch, float targetEnginePitch, double updateDelta, float pitchChangeRate, float maxChange)
{
   if(AK::SoundEngine::IsInitialized() == false)
      return;

   BDEBUG_ASSERT(wiseObjectID != -1);
   if(wiseObjectID == -1)
      return;

   float previousEnginePitch = currentEnginePitch;

   float deltaChange = static_cast<float>((updateDelta / pitchChangeRate));  //rate is unit change per second.
   if( deltaChange > maxChange )
      deltaChange = maxChange;

   if( targetEnginePitch > currentEnginePitch )
   {
      currentEnginePitch += deltaChange;
      if( currentEnginePitch > targetEnginePitch )  //make sure we didn't go too far
         currentEnginePitch = targetEnginePitch;
   }
   else
   {
      currentEnginePitch -= deltaChange;
      if( currentEnginePitch < targetEnginePitch )  //make sure we didn't go too far
         currentEnginePitch = targetEnginePitch;
   }

   if( currentEnginePitch < 0.0f )
      currentEnginePitch = 0.0f;
   if( currentEnginePitch > 100.0f )
      currentEnginePitch = 100.0f;

   if( currentEnginePitch != previousEnginePitch )
   {
      AK::SoundEngine::SetRTPCValue(AK::GAME_PARAMETERS::ENGINE_VELOCITY, currentEnginePitch, wiseObjectID);
      //gConsoleOutput.debug("%10u  wise %d  pitch %f", timeGetTime(), wiseObjectID, currentEnginePitch);
   }
}

//==============================================================================
// BSoundManager::updateFOWvolume
//==============================================================================
void BSoundManager::setRTPCValue(long wiseObjectID, unsigned long rtpcID, float value)
{
   if(AK::SoundEngine::IsInitialized() == false)
      return;

   BDEBUG_ASSERT(wiseObjectID != -1);
   if(wiseObjectID == -1)
      return;

   AK::SoundEngine::SetRTPCValue(rtpcID, value, wiseObjectID);
}

//==============================================================================
// BSoundManager::removeObject
//==============================================================================
void BSoundManager::addObject(BWwiseObjectID wiseObjectID)
{
    if(AK::SoundEngine::IsInitialized() == false)
      return;

   AK::SoundEngine::RegisterGameObj(wiseObjectID);
}

//==============================================================================
// BSoundManager::removeObject
//==============================================================================
void BSoundManager::removeObject(BWwiseObjectID wiseObjectID)
{
    if(AK::SoundEngine::IsInitialized() == false)
      return;

   AK::SoundEngine::UnregisterGameObj(wiseObjectID);
}

//==============================================================================
// BSoundManager::handleNotification
//==============================================================================
void BSoundManager::handleNotification(AkUInt32 in_callbackType, AkPlayingID in_playingID, AkUniqueID	in_eventID, AkGameObjectID in_gameObj, void * in_pCookie, void * in_pData, AkUInt32 in_uDataSize)
{
   //-- Sound Event 

   // Called from a worker thread!
   long eventType = -1;
   if(in_callbackType == AK_EndOfEvent)
      eventType = cSoundEventStop;   

   // Called from a worker thread!
   {
      BScopedCriticalSection lock(mSoundEventParamsArrayLock);
      
      BSoundEventParams* p = mSoundEventParamsArray.pushBackNoConstruction(1);
      
      p->eventType      = eventType;
      p->in_playingID   = in_playingID;
      p->in_eventID     = in_eventID;
      p->in_gameObj     = in_gameObj;     
   }      
}

//==============================================================================
// BSoundManager::handleNotification
//==============================================================================
void BSoundManager::handleNotification(AkBankID in_bankID, AKRESULT in_eLoadResult, AkMemPoolId in_memPoolId, int in_prepare, void *in_pCookie)
{
   //-- Bank loaded

   // Called from a worker thread!
   {
      BScopedCriticalSection lock(mSoundEventParamsArrayLock);
      
      BSoundEventParams* p = mSoundEventParamsArray.pushBackNoConstruction(1);
      
      if (in_prepare == 0)
         p->eventType = cSoundEventPrepare;
      else if (in_prepare == 1)
         p->eventType = cSoundEventUnprepare;
      else
         p->eventType = cSoundEventBankLoaded;            
      p->in_bankID      = in_bankID;
      p->in_status      = in_eLoadResult;
   }      
}

//==============================================================================
// BSoundManager::callbackFunc
//==============================================================================
void BSoundManager::callbackFunc(AkCallbackType in_eType, AkCallbackInfo* in_pCallbackInfo)
{
   //-- Only handle end events for now
   if(in_eType == AK_EndOfEvent)
   {
      AkEventCallbackInfo* pEventCallback = static_cast<AkEventCallbackInfo*>(in_pCallbackInfo);
      gSoundManager.handleNotification(in_eType, pEventCallback->playingID, pEventCallback->eventID, pEventCallback->gameObjID, pEventCallback->pCookie, NULL, 0);
   }
   
}

//==============================================================================
// BSoundManager::bankCallbackFunc
//==============================================================================
void BSoundManager::bankCallbackFunc(AkBankID in_bankID, AKRESULT	in_eLoadResult, AkMemPoolId in_memPoolId, void *in_pCookie)
{
   gSoundManager.handleNotification(in_bankID, in_eLoadResult, in_memPoolId, -1, in_pCookie); //0 for prepare, 1 for unprepare, -1 for regular bank
}

//==============================================================================
// BSoundManager::prepareEventCallbackFunc
//==============================================================================
void BSoundManager::prepareEventCallbackFunc(AkBankID in_bankID, AKRESULT	in_eLoadResult, AkMemPoolId in_memPoolId, void *in_pCookie)
{
   gSoundManager.handleNotification(in_bankID, in_eLoadResult, in_memPoolId, 0, in_pCookie); //0 for prepare, 1 for unprepare, -1 for regular bank
}

//==============================================================================
// BSoundManager::unprepareEventCallbackFunc
//==============================================================================
void BSoundManager::unprepareEventCallbackFunc(AkBankID in_bankID, AKRESULT	in_eLoadResult, AkMemPoolId in_memPoolId, void *in_pCookie)
{
   gSoundManager.handleNotification(in_bankID, in_eLoadResult, in_memPoolId, 1, in_pCookie); //0 for prepare, 1 for unprepare, -1 for regular bank
}

//==============================================================================
// BSoundManager::setSoundScapeScale
//==============================================================================
void BSoundManager::setSoundScapeScale(float scale)
{
   gSoundScapeScale = scale;
}

#ifndef BUILD_FINAL
//==============================================================================
// BSoundManager::getMemoryInformation
//==============================================================================
void BSoundManager::getMemoryInformation(unsigned long* engineMem, long* engineAlloc, unsigned long* physMem, long* physAlloc, unsigned long* virtMem, long* virtAlloc)
{
   //-- Engine
   *engineMem = soundEngineMemAllocated;
   *engineAlloc = soundNumEngineAllocations;
   
   //-- Physical
   *physMem = soundPhysMemAllocated;
   *physAlloc = soundNumPhysAllocations;

   //-- Virtual
   *virtMem = soundVirtMemAllocated;
   *virtAlloc = soundNumVirtAllocations;
}
#endif

//==============================================================================
// BSoundManager::loadSoundBank
//==============================================================================
AkBankID BSoundManager::loadSoundBank(const BString &bankName, bool asynch)
{
   SCOPEDSAMPLE(BSoundManager_loadSoundBank);

   if(mIsInitialized == false)
      return AK_INVALID_BANK_ID;


   AkBankID bankID=AK_INVALID_BANK_ID;
   AKRESULT result=AK_Fail;  

   //-- Make sure we're not loading a bank that has already been loaded
   for(uint i=0; i < mLoadedBanks.getSize(); i++)
   {
      if(mLoadedBanks[i].mBankName == bankName)
      {
         return AK_INVALID_BANK_ID;
      }
   }   

   if(bankName.isEmpty())
      return AK_INVALID_BANK_ID;


   BUString unicodeString = BUString(bankName);

#ifndef BUILD_FINAL
      AK::MemoryMgr::PoolStats poolStats;
      AK::MemoryMgr::GetPoolStats(2, poolStats);
 //     gConsoleOutput.fileManager("%18s pre load     %6d allocs  %6d used  %8d reserved", BString(bankName).getPtr(), poolStats.uAllocs - poolStats.uFrees, poolStats.uUsed, poolStats.uReserved );
#endif

   if(asynch)
   {
      AkBankCallbackFunc callBackFuncPtr = &BSoundManager::bankCallbackFunc;
      result = AK::SoundEngine::LoadBank(unicodeString.getPtr(), callBackFuncPtr, NULL, AK_DEFAULT_POOL_ID, bankID); 
   }
   else
   {
      result = AK::SoundEngine::LoadBank( unicodeString.getPtr(), AK_DEFAULT_POOL_ID, bankID );
   }

   if(result != AK_Success)
   {
      BString errorMsg;
      errorMsg.format("Failed to Load: %s Code: %d", BString(bankName).getPtr(), result);
      BASSERTM(result == AK_Success, errorMsg.getPtr());
   }

   if(result == AK_Success)
   {
      //-- Reload bank code
      BBankInfo info;
      info.mBankName = bankName;
      info.mBankID = bankID;
      mLoadedBanks.add(info);

#ifndef BUILD_FINAL
      if( !asynch )
      {
         AK::MemoryMgr::PoolStats poolStats2;
         AK::MemoryMgr::GetPoolStats(2, poolStats2);
         gConsoleOutput.fileManager("%18s loaded      %6d deltaMem %6d allocs  %6d used  %8d reserved", BString(bankName).getPtr(), poolStats2.uUsed - poolStats.uUsed, poolStats2.uAllocs - poolStats2.uFrees, poolStats2.uUsed, poolStats2.uReserved );
      }
#endif
   }
      
   return bankID;
}

//==============================================================================
// BSoundManager::unloadSoundBank
//==============================================================================
bool BSoundManager::unloadSoundBank(const BString &bankName, bool asynch)
{
   for(uint i=0; i < mLoadedBanks.getSize(); i++)
   {
      if(mLoadedBanks[i].mBankName == bankName)
      {
         unloadSoundBank(mLoadedBanks[i].mBankID, asynch);
         return true;
      }
   }
   return false;
}

//==============================================================================
// BSoundManager::unloadSoundBank
//==============================================================================
bool BSoundManager::unloadSoundBank(AkBankID bankID, bool asynch)
{
   if(mIsInitialized == false)
      return false;

   if(bankID == AK_INVALID_BANK_ID)
      return false;

   AKRESULT result = AK_Fail;
   if(asynch)
   {      
      result = AK::SoundEngine::UnloadBank(bankID, NULL, NULL);
   }
   else
   {      
      result = AK::SoundEngine::UnloadBank( bankID, NULL);
   }

   //-- Remove the unloaded bank from the bank list
   if(result == AK_Success)
   {
      for(int i = mLoadedBanks.getSize()-1; i >= 0 ; i--)
      {
         if(mLoadedBanks[i].mBankID == bankID)
         {
            mLoadedBanks.removeIndex(i);
         }
      }
      return true;
   }
   else
      return false;
}

//==============================================================================
// BSoundManager::worldReset
//==============================================================================
void BSoundManager::worldReset(void)
{
   mQueuedSoundEvents.clear();
}

//==============================================================================
// BSoundManager::toggleMute
//==============================================================================
void BSoundManager::toggleMute(void)
{
   if(mMute == false)
   {
      BCueIndex cue = getCueIndexByEnum(cSoundMuteMasterBus);
      playCue(cue);
      mMute = true;
   }
   else
   {
      BCueIndex cue = getCueIndexByEnum(cSoundUnmuteMasterBus);
      playCue(cue);
      mMute = false;
   }
}

//==============================================================================
// BSoundManager::overrideBackgroundMusic
//==============================================================================
void BSoundManager::overrideBackgroundMusic(bool val)
{
   //DJBFIXME: E3 fix. What is this method really suppose to be doing?
   BCueIndex cueIndex = cInvalidCueIndex;
   if(val)
      cueIndex = gSoundManager.getCueIndexByEnum(cSoundMuteMusic);
   else
      cueIndex = gSoundManager.getCueIndexByEnum(cSoundUnmuteMusic);

   if (cueIndex!=cInvalidCueIndex)
      gSoundManager.playCue(cueIndex);

   /*/if(val)
      XMPOverrideBackgroundMusic();
   else
      XMPRestoreBackgroundMusic();
   */
}

//==============================================================================
// BSoundManager::resetSoundManager
//==============================================================================
void BSoundManager::resetSoundManager()
{
   if(mIsInitialized == false)
      return;

   shutdown();
   initSoundEngine();
}

//==============================================================================
// BSoundManager::setSplitScreen
//==============================================================================
void BSoundManager::setSplitScreen(bool val)
{
   mSplitScreen = val;
}

//==============================================================================
// BSoundManager::setGameStartDelay
//==============================================================================
void BSoundManager::setGameStartDelay(bool val)
{
   mGameStartDelay = val;
   if(val == false)
   {
      for(uint i = 0; i < mQueuedSoundsToAddWhenGameStarts.getSize(); i++)
         playCue(mQueuedSoundsToAddWhenGameStarts[i].mCueIndex, mQueuedSoundsToAddWhenGameStarts[i].mWiseObjectID, false);

      mQueuedSoundsToAddWhenGameStarts.clear();
   }
}

//==============================================================================
// BSoundManager::getLoadedSoundBanks
//==============================================================================
BDynamicArray<BString> BSoundManager::getLoadedSoundBanks() const
{
   BDynamicArray<BString> loadedBanks;
   for(uint i=0; i < mLoadedBanks.getSize(); i++)
   {
      loadedBanks.add(mLoadedBanks[i].mBankName);
   }

   return loadedBanks;
}

//==============================================================================
// BSoundManager::getCueIndexByEnum
//==============================================================================
BCueHandle BSoundManager::getCueIndexByEnum(uint soundEnum)
{
   if(soundEnum < cSoundMax)
      return mSoundCues[soundEnum];
   
   return cInvalidCueIndex;
}

//==============================================================================
// BSoundManager::getCueIndexByEnum
//==============================================================================
BCueHandle BSoundManager::playSoundCueByEnum(uint soundEnum)
{
   BCueIndex cue = getCueIndexByEnum(soundEnum);

   if(soundEnum == cSoundMusicPlayInGame)
      mInGamePlaying = true;
   else if(soundEnum == cSoundMusicStopInGame)
      mInGamePlaying = false;
   else if(soundEnum == cSoundMusicPlayPreGame)
      mPreGamePlaying = true;
   else if(soundEnum == cSoundMusicStopPreGame)
      mPreGamePlaying = false;

   return playCue(cue);
}

//==============================================================================
// BSoundManager::getDefaultBankMemoryStats
//==============================================================================
void BSoundManager::getDefaultBankMemoryStats(uint &currAllocations, uint &currMemAllocated )
{
   AK::MemoryMgr::PoolStats poolStats;
   AK::MemoryMgr::GetPoolStats(2, poolStats);
   currAllocations = poolStats.uAllocs - poolStats.uFrees;
   currMemAllocated = poolStats.uUsed;
}

//==============================================================================
// BSoundManager::setVolumeVoice
//==============================================================================
void BSoundManager::setVolumeVoice(uint8 val)
{
   if( mIsInitialized )
      AK::SoundEngine::SetRTPCValue(AK::GAME_PARAMETERS::VOICE_VOLUME, val);
}
//==============================================================================
// BSoundManager::setVolumeSFX
//==============================================================================
void BSoundManager::setVolumeSFX(uint8 val)
{
   if( mIsInitialized )
      AK::SoundEngine::SetRTPCValue(AK::GAME_PARAMETERS::SFX_VOLUME, val);
}
//==============================================================================
// BSoundManager::setVolumeMusic
//==============================================================================
void BSoundManager::setVolumeMusic(uint8 val)
{
   if( mIsInitialized )
      AK::SoundEngine::SetRTPCValue(AK::GAME_PARAMETERS::MUSIC_VOLUME, val);
} 

//==============================================================================
// BSoundManager::threadSafePlayCue
//==============================================================================
void BSoundManager::threadSafePlayCue(BCueIndex index)
{
   BScopedLightWeightMutex scopedMutex(mMutex);
   
   if(!AK::SoundEngine::IsInitialized())
      return;

	BCueHandle playingID = AK::SoundEngine::PostEvent(index, cWWisePlayer1ID, AK_EndOfEvent, callbackFunc);
   if(playingID == cInvalidCueHandle)
   {
      AkEventCallbackInfo info;
      info.eventID = index;
      info.playingID = playingID;
      info.gameObjID = cWWisePlayer1ID;      
      BSoundManager::callbackFunc(AK_EndOfEvent, &info);
   }
   else
   {
      AK::SoundEngine::RenderAudio();
   }
}

#ifndef BUILD_FINAL
//==============================================================================
// BSoundManager::toggleSoundCueOutput
//==============================================================================
bool BSoundManager::toggleSoundCueOutput()
{
   mbSoundCueOutput = !mbSoundCueOutput;

   if( mbSoundCueOutput )
      mOutputBaseTime = timeGetTime();

   gConsoleOutput.fileManager("//   event ID    object ID    time   currAllocs  memUsed");
   return(mbSoundCueOutput);
}

//==============================================================================
// BSoundPlayback::update
//==============================================================================
void BSoundPlayback::update()
{
   if( mbPlayback != true )
      return;

   if( mCurrentLine >= mNumLines )
   {
#ifdef SOUND_REREGISTER_OBJECTS
      //unregister the objects
      for(long i=cMaxWWiseID; i > cMinWWiseID; i--)
      {
         gSoundManager.removeObject(i);
      }
#endif
#ifdef SOUND_RELOAD_BANKS
      gSoundManager.unloadAllSoundBanks();
      gSoundManager.loadStaticSoundBanks(false);
#endif

      mbPlayback = false;
      return;
   }

   AK::MemoryMgr::PoolStats poolStats;
   AkCallbackFunc callbackFuncPtr = &BSoundManager::callbackFunc;

   while( (mCurrentLine < mNumLines) && (mArray[mCurrentLine].time <= (timeGetTime()-mBaseTime)) )
   {
      //play the sound
      AK::SoundEngine::PostEvent(mArray[mCurrentLine].cueIndex, mArray[mCurrentLine].objectID, AK_EndOfEvent, callbackFuncPtr);
         AK::MemoryMgr::GetPoolStats(2, poolStats);
         gConsoleOutput.fileManager("cue %10u %10u %10u     %d %d", mArray[mCurrentLine].cueIndex, mArray[mCurrentLine].objectID, mArray[mCurrentLine].time, poolStats.uAllocs - poolStats.uFrees, poolStats.uUsed );
      mCurrentLine++;
   }

}

//==============================================================================
// BSoundPlayback::init
//==============================================================================
void BSoundPlayback::init(const char* filename)
{
   mNumLines = 0;
   mArray.clear();

   //load file
   BSoundPlaybackEntry newEntry;

#ifdef XBOX  
   BSimString path;

   BSimString buf;
   BSimString buf2;
   
   BFixedString<1024> line;

   uchar* pFileData = NULL;
   unsigned long fileSize = 0;
   bool readHDFile = false;
   
#ifndef BUILD_FINAL   
   BSimString qualifiedPath;
   if (cFME_SUCCESS == gFileManager.constructQualifiedPath(cDirRecordGame, filename, qualifiedPath))
   {
      BSimString fullPath("d:\\");
      fullPath += qualifiedPath;
      FILE* pFile = NULL;
      if (0 == fopen_s(&pFile, fullPath, "rb"))
      {
         fseek(pFile, 0, SEEK_END);
         fileSize = ftell(pFile);
         rewind(pFile);
         
         pFileData = new uchar[Math::Max<unsigned long>(1U, fileSize)];
         
         if (fread(pFileData, 1, fileSize, pFile) != fileSize)
         {
            BFATAL_FAIL("BConfig::read: Failed reading config file");
         }
         
         readHDFile = true;
         
         fclose(pFile);
         
         gConsoleOutput.resource("BConfig::read: Loaded %s from hard drive!", qualifiedPath.getPtr());
         trace("BConfig::read: Loaded %s from hard drive!", qualifiedPath.getPtr());
      }
   }
#endif   

   if (!readHDFile)
   {
      if (!BFileUtils::loadFile(cDirRecordGame, filename, (void**)&pFileData, &fileSize))
         BASSERT(0); //return(cErrFileNotFound);
      
      gConsoleOutput.resource("Loaded %s using file system", filename);
   }
   
   BByteStream byteStream(pFileData, fileSize);
   
   long index, index2;
   for ( ; ; )
   {
      if (byteStream.bytesLeft() == 0)
         break;
         
      line.readLine(byteStream);
      
      // checking this makes it stop processing the last line of the cfg file twice

      buf.set(line);

      // handle bad data in file
      index = buf.findLeft(B('\r'));
      if (index != -1)
         buf.remove(index, (buf.length() - index));

      //Only handle the lines that start with 'cue'
      if (buf.findLeft(B("cue"),0) != 0)
         continue;

      //Get the event ID
      index = buf.findLeft(B(" "), 0);
      while( buf.getChar(index) == ' ' )
         index++;
      index2 = buf.findLeft(B(" "), index);
      while( buf.getChar(index2+1) == ' ' )
         index2++;
      buf2.set(buf, index2 - index, index );
      newEntry.cueIndex = (DWORD)buf2.asInt64();

      //Get the sound object ID
      index = index2+1;
      index2 = buf.findLeft(B(" "), index);
      while( buf.getChar(index2+1) == ' ' )
         index2++;
      buf2.set(buf, index2 - index, index );
      newEntry.objectID = buf2.asLong();

      //Get the time (in ms, offset from 0)
      index = index2+1;
      index2 = buf.findLeft(B(" "), index);
      while( buf.getChar(index2+1) == ' ' )
         index2++;
      buf2.set(buf, index2 - index, index );
      newEntry.time = buf2.asLong();

      //Add the entry to our array
      mArray.add(newEntry);
      mNumLines++;
   }
   
#ifndef BUILD_FINAL   
   if (readHDFile)
      delete[] pFileData;
   else
#endif   
      BFileUtils::unloadFile(pFileData);

#endif // XBOX

#ifdef SOUND_REREGISTER_OBJECTS
      //register the sound objects
      for(long i=cMaxWWiseID; i > cMinWWiseID; i--)
      {
         gSoundManager.addObject(i);
      }
#endif

   //set timer to zero
   mBaseTime = timeGetTime();
   mCurrentLine = 0;
   
   mbPlayback = true;
}
#endif
