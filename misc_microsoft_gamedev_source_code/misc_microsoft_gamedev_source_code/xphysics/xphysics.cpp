//============================================================================
// xphysics.cpp
//
// Copyright (c) 2003, Ensemble Studios
//============================================================================

// Includes
#include "common.h"
#include "xphysics.h"
#include "physicsinternal.h"
//#include "vehicleblueprintmanager.h"
//#include "memory\memoryheap.h"
#include "workdirsetup.h"
#include "threading\workDistributor.h"


#if 0
#pragma comment(lib, "hkserialize.lib")
#pragma comment(lib, "hkmath.lib")
#pragma comment(lib, "hkbase.lib")
#pragma comment(lib, "hkutilities.lib")
#pragma comment(lib, "hkdynamics.lib")
#pragma comment(lib, "hkcollide.lib")
#pragma comment(lib, "hkinternal.lib")
#pragma comment(lib, "hkconstraintsolver.lib")
#pragma comment(lib, "hkvisualize.lib")
#pragma comment(lib, "hkvehicle.lib")
#endif

//#pragma comment(lib, "hkexport2.lib")
//#pragma comment(lib, "hkgeometry2.lib")
//#pragma comment(lib, "hkgeometryutil2.lib")
//#pragma comment(lib, "hkserialize2.lib")

extern void*	(HK_CALL *hkSystemMalloc)(int size, int align);
extern void		(HK_CALL *hkSystemFree) (void* p);

// Defines

#if defined(HK_PLATFORM_GC)
#	define KEYCODE_ATTRIBUES __attribute__((section(".sdata")))
#else
#	define KEYCODE_ATTRIBUES
#endif

#define HAVOK_KEYCODE(code, value)					\
   extern const char         HK_KEYCODE[] KEYCODE_ATTRIBUES = code;	\
   extern const unsigned int HK_KEYVALUE  KEYCODE_ATTRIBUES = value

//---------------------------------------------------------------------
//-- WMJ
//-- This havok keycode is not final.  
//-- Last updated 10/24/2006  (final)
//---------------------------------------------------------------------

//HAVOK_KEYCODE("Ensemble",0xcca92ed8)
// Old keycode through sept 30
//HAVOK_KEYCODE("30SEP05.PhAn.Ensemble_HavokComplete3Evaluation_Xbox360", 0xad7b01aa)
// new keycode through december 30
//HAVOK_KEYCODE("31OCT05.PhAn.Ensemble_HavokComplete3Evaluation_Xbox360",0xad22123a);
HAVOK_KEYCODE("CLIENT.Ph.Ensemble_Microsoft_Phoenix_HavokPhysics", 0x4404dc01);

// Static globals
static long gXPhysicsRefCount=0;
static XPhysicsInfo gXPhysicsInfo;
static bool gXPhysicsMemoryInited = false;
// rg HACK HACK, reducing this to save memory until we really want to use Havok
int cStackSize = 128*1024;//0x100000;
char* gStackBuffers[cMaxPhysicsThreads] = {NULL, NULL, NULL, NULL, NULL, NULL};
BCountDownEvent mReleasePhysicsThreadResources;

#if MEMORY_SYSTEM_ENABLE

static void* HK_CALL Havok_Allocator(int size, int align)
{
   // Note: Rockall doesn't seem to allow alignment specification per allocation.
   // We'll probably want this when we move to the xbox.  MWC

   //-- actualSize is an in/out parameter.
   if (align <= 16)
   {
      return gPrimaryHeap.AlignedNew(size, align);
   }

   long actualSize = size;
   if (align > sizeof(DWORD))
   {
      // Allocate enough extra to allow Havok to align up the pointer. (Does it do this itself?)
      actualSize = size + align - 1;
   }

   return gPrimaryHeap.New(actualSize);
}

static void HK_CALL Havok_Deallocator(void* pData)
{
   gPrimaryHeap.Delete(pData);
}

#endif

static void errorReportFunction(const char* str, void* errorOutputObject)
{
#ifndef BUILD_FINAL
   sprintf("%s", str);
#endif//BUILD_FINAL
}

//==============================================================================
// Setup havok memory manager, base system, and singletons
// This is called before each game starts
//==============================================================================
void XPhysicsMemoryInit()
{
   if (gXPhysicsMemoryInited)
      return;

   // Initialize the base system including our memory system
   hkPoolMemory* memoryManager = new hkPoolMemory();
   hkThreadMemory *threadMemory = new hkThreadMemory(memoryManager, 16);
   hkBaseSystem::init(memoryManager, threadMemory, errorReportFunction );	
   memoryManager->removeReference();
   threadMemory->removeReference();

   // We now initialize the stack area (fast temporary memory to be used by the engine).
   { 
      gStackBuffers[0] = hkAllocate<char>(cStackSize, HK_MEMORY_CLASS_BASE);
      threadMemory->setStackArea(gStackBuffers[0], cStackSize );
   }

   // Setup singletons

   //-- add in our error handler
#pragma push_macro("new")
#undef new
   hkError::replaceInstance(new BPhysicsError());
#pragma pop_macro("new")

   hkStreambufFactory::getInstance().replaceInstance(static_cast<customStreambufFactory*>(customStreambufFactory::create()));

   gXPhysicsMemoryInited = true;
}

//==============================================================================
// XPhysicsCreate
//==============================================================================
bool XPhysicsCreate(XPhysicsInfo* info)
{
   if(gXPhysicsRefCount==0)
   {
      if(info)
         gXPhysicsInfo=*info;

#if MEMORY_SYSTEM_ENABLE
      hkSystemMalloc = Havok_Allocator;
      hkSystemFree   = Havok_Deallocator;
#endif

     gPhysics = new BPhysics;
     if(!gPhysics || !gPhysics->setup())
        return(false);
     gPhysics->getShapeManager().setBaseDirectoryID(gXPhysicsInfo.mDirPhysics);
     //gModelManager->registerShapeIDFunc(BShapeManager::shapeIDFunc);
     gPhysics->getPhysicsObjectBlueprintManager().setBaseDirectoryID(gXPhysicsInfo.mDirPhysics);
     //-- physics owns this and will delete it when done.
     gPhysics->setRenderInterface(gXPhysicsInfo.mpRenderInterface);
   }

   gXPhysicsRefCount++;

   return (true);
}

//==============================================================================
// XPhysicsThreadRelease
//==============================================================================
bool XPhysicsReleaseThreadResources(uint privateData, BEventPayload* pPayload)
{
   if (!isPhysicsThreadInited())
   {
      mReleasePhysicsThreadResources.decrement();
      return true;
   }

   // This function is essentially the same as hkBaseSystem::clearThreadResources
   // but it checks for null pointers first.  It also releases the stack memory.

   #if HK_CONFIG_MONITORS == HK_CONFIG_MONITORS_ENABLED
      if (&(hkMonitorStream::getInstance()))
	      hkMonitorStream::getInstance().quit();
   #endif

   if (&(hkThreadMemory::getInstance()))
   {
      hkThreadMemory::getInstance().setStackArea(0, 0);
      if (gStackBuffers[gEventDispatcher.getThreadIndex()])
      {
         hkDeallocate<char>(gStackBuffers[gEventDispatcher.getThreadIndex()]);
         gStackBuffers[gEventDispatcher.getThreadIndex()] = NULL;
      }
	   hkThreadMemory::getInstance().releaseCachedMemory();
	   hkThreadMemory::replaceInstance( HK_NULL );
	   //hkThreadMemory::quit();
   }

   mReleasePhysicsThreadResources.decrement();
   return true;
}

//==============================================================================
// Shutdown havok memory manager and base system
// This is called after each game ends
//==============================================================================
void XPhysicsMemoryDeinit()
{
   if (!gXPhysicsMemoryInited)
      return;

   // Unload all the shapes as they contain hkpShape instances allocated from
   // the havok memory manager.  This ensures that they get reloaded next time
   if (gPhysics)
      gPhysics->getShapeManager().unloadAll();

   // Free our thread memory for the main thread (no need to call clearThreadResources as it is
   // called in hkBaseSystem::quit
   if (gStackBuffers[0])
   {
      hkDeallocate<char>(gStackBuffers[0]);
      gStackBuffers[0] = NULL;
   }

   // Call physics thread cleanup function on every thread and wait until they are done
   mReleasePhysicsThreadResources.clear();
   mReleasePhysicsThreadResources.set(5);
   for (uint i = 1; i < 6; i++)
   {
      gEventDispatcher.submitCallback(i, XPhysicsReleaseThreadResources);
   }
   gWorkDistributor.waitSingle(mReleasePhysicsThreadResources);

   //-- clean up the Havok memory;
   hkBaseSystem::quit();

   gXPhysicsMemoryInited = false;
}

//==============================================================================
// XPhysicsRelease
//==============================================================================
void XPhysicsRelease()
{
   if(gXPhysicsRefCount==0)
   {
      return;
   }

   gXPhysicsRefCount--;

   if(gXPhysicsRefCount==0)
   {
      //-- shutdown physics
      if(gPhysics)
      {
         gPhysics->shutdown();
         delete gPhysics;
         gPhysics = NULL;
      }
   }
}


//==============================================================================
// XPhysicsThreadInit
//==============================================================================
bool XPhysicsThreadInit()
{
   hkThreadMemory* pThreadMemory = &(hkThreadMemory::getInstance());
   if (pThreadMemory)
      return true;

   // Make a new hkThreadMemory instance and init it
   pThreadMemory = new hkThreadMemory(&hkMemory::getInstance(), 16); // set to 0 for debug memory tracking
   hkBaseSystem::initThread(pThreadMemory);
   pThreadMemory->removeReference();

   // We now initialize the stack area to 100k (fast temporary memory to be used by the engine).
   gStackBuffers[gEventDispatcher.getThreadIndex()] = hkAllocate<char>(cStackSize, HK_MEMORY_CLASS_BASE);
   pThreadMemory->setStackArea(gStackBuffers[gEventDispatcher.getThreadIndex()], cStackSize);

   return (true);
}
