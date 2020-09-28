// File: consoleGameHelper.cpp
#include "xcore.h"
#include "consoleGameHelper.h"
#include "fileManager.h"
#include "workdirsetup.h"
#include "xsystemlib.h"
#include "threading\eventDispatcher.h"

ATG::Console BConsoleGameHelper::mConsole;

//==============================================================================
// BConsoleGameHelper::setupGameDir
//==============================================================================
long BConsoleGameHelper::setupGameDir(const BSimString& dirName)
{
   BFixedStringMaxPath tempDirName;
   gFileManager.getDirListEntry(tempDirName, gFileManager.getProductionDirListID());
   tempDirName.append(dirName);
   
   long dirID;
   
   eFileManagerError result = gFileManager.createDirList(tempDirName, dirID);
   BVERIFY(cFME_SUCCESS == result);
   
   return dirID;
}

//==============================================================================
// BConsoleGameHelper::setupGameDirectories
//==============================================================================
bool BConsoleGameHelper::setupGameDirectories(void)
{
   cDirData       = setupGameDir("data\\");
   cDirArt        = setupGameDir("art\\");
   cDirScenario   = setupGameDir("scenario\\");
   cDirFonts      = setupGameDir("fonts\\");
   
   // Validate current directory
   if (cFME_SUCCESS != gFileManager.doesFileExist(gFileManager.getProductionDirListID(), "startup\\game.cfg"))
      return false;

   return true;
}

//==============================================================================
// BConsoleGameHelper::init
//==============================================================================
bool BConsoleGameHelper::setup(bool useXFS, bool xfsCopy)
{
   // XSystem
   XSystemInfo xsystemInfo;
   xsystemInfo.mUseXFS = useXFS;
   xsystemInfo.mXFSCopy = xfsCopy;
   xsystemInfo.mXFSConnectionPort = 1010;
   xsystemInfo.mStartupDir="startup\\";
   if (!XSystemCreate(&xsystemInfo))
   {
      gConsoleOutput.error("XSystemCreate() failed!\n");
      return false;
   }
         
   return true;
}

//==============================================================================
// BConsoleGameHelper::emptyThreadQueues
//==============================================================================
void BConsoleGameHelper::emptyThreadQueues(void)
{
   for (uint i = 0; i < 10; i++)
   {
      gEventDispatcher.pumpAllThreads(100, 10);

      for (uint threadIndex = cThreadIndexSim; threadIndex < cThreadIndexMax; threadIndex++)
      {
         if (gEventDispatcher.getThreadId(static_cast<BThreadIndex>(threadIndex)))
            gEventDispatcher.pumpUntilThreadQueueEmpty(static_cast<BThreadIndex>(threadIndex));
      }
   }      
}

//==============================================================================
// BConsoleGameHelper::deinit
//==============================================================================
bool BConsoleGameHelper::deinit(void)
{
   XSystemRelease();

   return true;      
}

//==============================================================================
// BConsoleGameHelper::getButtons
//==============================================================================
DWORD BConsoleGameHelper::getButtons(void)
{
   XINPUT_STATE state;
   DWORD res = XInputGetState(0, &state);
   if (ERROR_SUCCESS == res)
      return state.Gamepad.wButtons;
   
   return 0;
}   

//==============================================================================
// BConsoleGameHelper::waitForButtonPress
//==============================================================================
DWORD BConsoleGameHelper::waitForButtonPress(DWORD buttons)
{
   for ( ; ; )
   {
      XINPUT_STATE state;
      DWORD res = XInputGetState(0, &state);
      if (ERROR_SUCCESS == res)
      {
         if ((state.Gamepad.wButtons & buttons) == 0)
            break;
      }
      
      Sleep(16);
   }
   
   Sleep(16);
   
   for ( ; ; )
   {
      XINPUT_STATE state;
      DWORD res = XInputGetState(0, &state);
      if (ERROR_SUCCESS == res)
      {
         if ((state.Gamepad.wButtons & buttons) != 0)
            return state.Gamepad.wButtons;
      }
      Sleep(16);
   }
}   

//==============================================================================
// BConsoleGameHelper::consoleOutputFunc
//==============================================================================
void BConsoleGameHelper::consoleOutputFunc(void* data, BConsoleMessageCategory category, const char* pMessage)
{
   BDEBUG_ASSERT(pMessage);
   
   if (!mConsole.GetValid())
      return;
      
   mConsole.Format("%s", pMessage);
}

//==============================================================================
// BConsoleGameHelper::consoleInit
//==============================================================================
bool BConsoleGameHelper::consoleInit(void)
{
   HRESULT hres = mConsole.Create("game:\\Media\\Courier_New_11.xpr", D3DCOLOR_ARGB(255, 30, 30, 30), D3DCOLOR_ARGB(255, 255, 255, 255));
   if (FAILED(hres))
      return false;
      
   gConsoleOutput.init(consoleOutputFunc, NULL);
   
   gConsoleOutput.printf("xfsCopy");
   gConsoleOutput.printf("Built: %s %s\n", __DATE__, __TIME__);
      
   return true;
}

//==============================================================================
// BConsoleGameHelper::consoleDeinit
//==============================================================================
void BConsoleGameHelper::consoleDeinit(void)
{
   gConsoleOutput.deinit();
   
   mConsole.Destroy();
}

//==============================================================================
// BConsoleGameHelper::consoleRender
//==============================================================================
void  BConsoleGameHelper::consoleRender(void)
{
   if (!mConsole.GetValid())
      return;
            
   mConsole.Render();
}

//==============================================================================
// BConsoleGameHelper::consoleClear
//==============================================================================
void  BConsoleGameHelper::consoleClear(void)
{
   if (!mConsole.GetValid())
      return;

   mConsole.Clear();
}

//==============================================================================
// BConsoleGameHelper::setNoAutoRender
//==============================================================================
void  BConsoleGameHelper::setNoAutoRender(bool val)
{
   if (!mConsole.GetValid())
      return;

   mConsole.setNoAutoRender(val);
}
