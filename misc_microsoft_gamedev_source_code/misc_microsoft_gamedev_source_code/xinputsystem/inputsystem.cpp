//==============================================================================
// inputsystem.cpp
//
// Copyright (c) 2005 Ensemble Studios
//==============================================================================

// Includes
#include "common.h"
#include "inputsystem.h"
#include "XeCR.h"
#include "config.h"
#include "configsinput.h"
#include "console.h"
#include "inputcontext.h"
#include "gamepad.h"
#include "gamepadmap.h"
#include "inputevent.h"
#include "keyboard.h"
#include "workdirsetup.h"
#include "xmlreader.h"
#include "Thread.h"
#include "threading\setThreadName.h"

#pragma warning ( disable : 4702 ) // unreachable code in input capture thread routine

// Globals
BInputSystem gInputSystem;

// Static data
BInputData                   BInputSystem::mInputData1;
BInputData                   BInputSystem::mInputData2;
BInputData*                  BInputSystem::mpInputDataWrite=NULL;
BInputData*                  BInputSystem::mpInputDataRead=NULL;
BThread*                     BInputSystem::mpCaptureInputThread=NULL;
BCriticalSection             BInputSystem::mCaptureInputLock(10000);
bool                         BInputSystem::mCaptureInput=false;

//==============================================================================
// BInputSystem::BInputSystem
//==============================================================================
BInputSystem::BInputSystem() :
   mInitialized(FALSE),
   mWindowHandle(NULL),
   mKeyboard(NULL),
   mGamepads(NULL),
   mControlMap(),
   mContextList(),
   mActiveContextes(),
#ifndef XBOX
   mDirectInput(NULL),
#endif
   mGamepadDeviceCount(0),
   mGamepadMaps(),
   mInputContextes(),
   mEventPool(),
   mEventAlloc(0),
   mUseXInput(false),
   mEventHandlers(),
   mInputIntefaces()
#ifndef BUILD_FINAL
   ,
   mpFileWatcher(NULL),
   mPathControlMap(-1),
   mPathControlSet(-1)
#endif
{
}

//==============================================================================
// BInputSystem::~BInputSystem
//==============================================================================
BInputSystem::~BInputSystem()
{
}

//==============================================================================
// BInputSystem::setup
//==============================================================================
bool BInputSystem::setup(HWND windowHandle, REGISTER_CONSOLE_FUNCS_CALLBACK registerConsoleFuncs)
{
   mWindowHandle=windowHandle;

   // Create keyboard
   mKeyboard=new BKeyboard();
   if(!mKeyboard || !mKeyboard->setup())
   {
      if (mKeyboard)
      {
         delete mKeyboard;
         mKeyboard = NULL;
      }
      return false;
   }
      
   if (gConfig.isDefined(cConfigChatPadRemapping))
   {
      mKeyboard->setChatPadRemapping(true);
      
      trace("ChatPad Remapping Enabled");
   }

#ifdef XBOX
   mUseXInput=true;
#else
   // First attempt to use Xinput instead of DirectInput
   XINPUT_STATE state;
   DWORD result = XInputGetState(0, &state);
   if (result == ERROR_SUCCESS)
      mUseXInput = true;
   else
   {
      // Setup DirectInput.
      DWORD errorCode = DirectInput8Create(GetModuleHandle(NULL), DIRECTINPUT_VERSION, IID_IDirectInput8, (VOID**)&mDirectInput, NULL);
      if (errorCode != DI_OK)
         return false;
   }
#endif

#ifndef BUILD_FINAL
   // Auto file reloading
   mpFileWatcher = new BFileWatcher();
#endif

   // Setup the gamepads
   if(!setupGamepads())
      return false;

   // Load the controls.
   setupControls();

   // Load controller configurations
   loadControlConfigs("controllerconfigs");

   // Console
   if(!gConsole.setup(registerConsoleFuncs))
      return false;

   mpInputDataWrite=&mInputData1;
   mpInputDataRead=&mInputData2;

   if (gConfig.isDefined(cConfigThreadInput))
   {
      mpCaptureInputThread = new BThread();
      if (!mpCaptureInputThread->createThread(BInputSystem::captureInput, NULL, 0, true))
         return false;
      mpCaptureInputThread->setThreadProcessor(2);
   }

   mInitialized = TRUE;

   return true;
}

//==============================================================================
// BInputSystem::shutdown
//==============================================================================
void BInputSystem::shutdown()
{
#ifndef BUILD_FINAL
   if(mpFileWatcher)
   {
      delete mpFileWatcher;
      mpFileWatcher=NULL;
   }
#endif

   if (mpCaptureInputThread)
   {
      setCaptureInput(false);
      delete mpCaptureInputThread;
      mpCaptureInputThread = NULL;
   }

   gConsole.cleanup();

   for(long i=0; i<mEventPool.getNumber(); i++)
      delete mEventPool[i];
   mEventPool.clear();

   for(long i=0; i<mGamepadMaps.getNumber(); i++)
      delete mGamepadMaps[i];
   mGamepadMaps.clear();

#ifndef XBOX
   if(mDirectInput)
   {
      mDirectInput->Release();
      mDirectInput=NULL;
   }
#endif

   for(long i=0; i<mContextList.getNumber(); i++)
      delete mContextList[i];
   mContextList.clear();
   mActiveContextes.clear();

   if(mGamepads)
   {
      delete[]mGamepads;
      mGamepads=NULL;
   }

   if(mKeyboard)
   {
      delete mKeyboard;
      mKeyboard=NULL;
   }

   mInitialized = FALSE;
}

#ifndef XBOX
//==============================================================================
// enumJoysticksCallback
//==============================================================================
static BOOL CALLBACK enumJoysticksCallback(const DIDEVICEINSTANCE* pdidInstance, VOID* pContext)
{
   BInputSystem* gamepadManager=(BInputSystem*)pContext;

   BSimString gamepadName(pdidInstance->tszProductName);
   gamepadName.trimRight();

   blog("gamepad: '%s'", gamepadName.getPtr());

   BGamepadMap* map=gamepadManager->lookupGamepadMap(gamepadName);
   if(!map)
      return DIENUM_CONTINUE;

   if(!gamepadManager->createGamepadDevice(pdidInstance->guidInstance, map))
      return DIENUM_CONTINUE;

   return DIENUM_CONTINUE;
}
#endif

//==============================================================================
// BInputSystem::setupGamepads
//==============================================================================
bool BInputSystem::setupGamepads()
{
   // Read in the gamepad maps
   BXMLReader reader;
   if(!reader.load(cDirProduction, B("data\\gamepads.xml")))
      return false;

   BXMLNode rootNode(reader.getRootNode());
   if(rootNode.getName()==B("Gamepads"))
   {
      for(long i=0; i<rootNode.getNumberChildren(); i++)
      {
         const BXMLNode gamepadNode(rootNode.getChild(i));
         if(gamepadNode.getName()==B("Gamepad"))
         {
            BGamepadMap* map=new BGamepadMap();
            if(map)
            {
               if(map->setup(gamepadNode))
               {
                  if(mGamepadMaps.add(map)==-1)
                     delete map;
               }
               else
                  delete map;
            }
         }
      }
   }

   // Create the gamepads
   mGamepads=new BGamepad[cMaxPorts];
   if(!mGamepads)
      return false;

   BGamepadMap* map=(mUseXInput ? lookupGamepadMap(B("Xbox")) : NULL);

   for (long i=0; i < cMaxPorts; i++)
   {
      if (!mGamepads[i].setup(i, map))
         return false;
   }

   if(mUseXInput)
   {
      for(long i=0; i<cMaxPorts; i++)
      {
         XINPUT_STATE state;
         DWORD result = XInputGetState(i, &state);
         if (result != ERROR_SUCCESS)
            break;
         mGamepadDeviceCount++;
      }
   }
#ifndef XBOX
   else
   {
      HRESULT hr = mDirectInput->EnumDevices(DI8DEVCLASS_GAMECTRL, enumJoysticksCallback, this, DIEDFL_ATTACHEDONLY);
      if( FAILED(hr) )
         return false;
   }
#endif

   return true;
}

//==============================================================================
// BInputSystem::setupControls
//==============================================================================
void BInputSystem::setupControls()
{
   // Clear previously loaded controls from existing gamepad contextes
   for(long i=0; i<mContextList.getNumber(); i++)
   {
      BInputContext* context=mContextList[i];
      context->clear();
   }

   // Clear previously mapped controls
   mControlMap.clearAll();

   // Load base control map
   BSimString name("controlmap");
   gConfig.get(cConfigControlMap, name);
   loadControlMap(name);

   // Load base controls
   name="controlset";
   gConfig.get(cConfigControlSet, name);
   loadControls(name);
}

//==============================================================================
// BInputSystem::loadControlMap
//==============================================================================
bool BInputSystem::loadControlMap(const BCHAR_T* name)
{
   BSimString path;
   path.format(B("data\\%s.xml"), name);

   BXMLReader reader;
   if(!reader.load(cDirProduction, path))
   {
      BASSERT(0);
      return false;
   }

   BXMLNode root(reader.getRootNode());
   
   for(long i=0; i<root.getNumberChildren(); i++)
   {
      const BXMLNode child(root.getChild(i));

      const BPackedString name2(child.getName());

      if(name2==B("Map"))
      {
         BSimString nameStr;
         if(child.getAttribValue("name", &nameStr))
         {
            BSimString controlStr;
            if(child.getAttribValue("control", &controlStr))
            {
               long controlType=lookupControl(controlStr.getPtr());
               mControlMap.add(nameStr.getPtr(), controlType);
            }
         }
      }
   }

#ifndef BUILD_FINAL
   if (mpFileWatcher && mPathControlMap == -1)
      mPathControlMap = mpFileWatcher->add(cDirBase, path, 0);
#endif

   return true;
}

//==============================================================================
// BInputSystem::loadControls
//==============================================================================
bool BInputSystem::loadControls(const BCHAR_T* name)
{
   BSimString path;
   path.format(B("data\\%s.xml"), name);

   BXMLReader reader;
   if(!reader.load(cDirProduction, path))
   {
      BASSERT(0);
      return false;
   }

   BXMLNode root(reader.getRootNode());
   
   for(long i=0; i<root.getNumberChildren(); i++)
   {
      BXMLNode child(root.getChild(i));

      const BPackedString name2(child.getName());

      if(name2==B("Mode"))
      {
         BSimString modeName;
         if(child.getAttribValue("name", &modeName))
         {
            BInputContext* context=NULL;
            for(long j=0; j<mContextList.getNumber(); j++)
            {
               BInputContext* item=mContextList[j];
               if(modeName.compare(item->getName())==0)
               {
                  context=item;
                  break;
               }
            }

            if(context)
            {
               if(!context->setup(child, &gInputSystem))
                  return false;
            }
            else
            {
               BInputContext* context2=new BInputContext();
               if(!context2)
               {
                  BASSERT(0);
                  return false;
               }

               if(!context2->setup(child, &gInputSystem))
               {
                  delete context2;
                  return false;
               }

               if(mContextList.add(context2)==-1)
               {
                  delete context2;
                  return false;
               }
            }
         }
      }
   }

#ifndef BUILD_FINAL
   if (mpFileWatcher && mPathControlSet == -1)
      mPathControlSet = mpFileWatcher->add(cDirBase, path, 0);
#endif

   return true;
}

//==============================================================================
// BInputSystem::loadControlConfigs
//==============================================================================
bool BInputSystem::loadControlConfigs( const BCHAR_T* name )
{
   BSimString path;
   path.format( B( "data\\%s.xml" ), name );

   BXMLReader reader;
   if( !reader.load( cDirProduction, path ) )
   {
      BASSERT( 0 );
      return( false );
   }

   BXMLNode root(reader.getRootNode());

   for( long i = 0; i < root.getNumberChildren(); i++ )
   {
      const BXMLNode child(root.getChild( i ));
      const BPackedString name2(child.getName());

      if( name2 == B( "Config" ) )
      {
         BInputInterface inputInterface;
         if( !inputInterface.parseXML( child ) )
         {
            return( false );
         }

         mInputIntefaces.uniqueAdd( inputInterface );
      }
      else if( name2 == B( "FunctionStrings" ) )
      {
         for( long j = 0; j < child.getNumberChildren(); j++ )
         {
            const BXMLNode function(child.getChild( j ));            
            const BPackedString nodeName(function.getName());
            if( nodeName == B( "FunctionString" ) )
            {
               BSimString funcName;
               if( function.getAttribValue( "name", &funcName ) )
               {
                  long funcIndex = BInputInterface::lookupFunction( funcName.getPtr() );            
                  if( ( funcIndex < 0 ) && ( funcIndex >= BInputInterface::cInputFunctionNum ) )
                  {
                     BASSERTM( 0, "Function name not recognized!" );
                     return( false );
                  }
                    
                  function.getText(mInputFunctionStrings[funcIndex]);
               }
            }
         }
      }
   }

   return( true );
}

#ifndef XBOX
//==============================================================================
// BInputSystem::createGamepadDevice
//==============================================================================
bool BInputSystem::createGamepadDevice(REFGUID guid, BGamepadMap* map)
{
   BGamepad& gamepad=getGamepad(mGamepadDeviceCount);
   if(!gamepad.createDevice(guid, map))
      return false;

   mGamepadDeviceCount++;
   return true;
}
#endif

//==============================================================================
// BInputSystem::enterContext
//==============================================================================
void BInputSystem::enterContext(const BCHAR_T* contextName)
{
   for(long i=0; i<mContextList.getNumber(); i++)
   {
      BInputContext* gamepadContext=mContextList[i];
      if(gamepadContext->getName()==contextName)
      {
         mActiveContextes.insertAtIndex(gamepadContext, 0);
         break;
      }
   }
}

//==============================================================================
// BInputSystem::leaveContext
//==============================================================================
void BInputSystem::leaveContext(const BCHAR_T* contextName)
{
   for(long i=0; i<mActiveContextes.getNumber(); i++)
   {
//-- FIXING PREFIX BUG ID 7801
      const BInputContext* gamepadContext=mActiveContextes[i];
//--
      if(gamepadContext && gamepadContext->getName()==contextName)
      {
         mActiveContextes.removeIndex(i);
         break;
      }
   }
}

//==============================================================================
// BInputSystem::isContextActive
//==============================================================================
bool BInputSystem::isContextActive(const BCHAR_T* contextName)
{
   for(long i=0; i<mActiveContextes.getNumber(); i++)
   {
//-- FIXING PREFIX BUG ID 7802
      const BInputContext* gamepadContext=mActiveContextes[i];
//--
      if(gamepadContext && gamepadContext->getName()==contextName)
         return true;
   }
   return false;
}

//==============================================================================
// BInputSystem::update
//==============================================================================
void BInputSystem::update(IInputEventHandler* pInputEventHandler)
{
   if (!mInitialized)
      return;

   SCOPEDSAMPLE(InputSystemUpdate);

   // SRL 7/29/08: passing in an input handler is a change so the flash background process (as well as others)
   // can get the benefit of our input handling system. It maybe called from a thread other than the
   // Sim thread so only input handling functionality is performed.
   if (!pInputEventHandler)
   {
#ifndef BUILD_FINAL
      // Handle auto data reloading
      if(mpFileWatcher)
      {
         if(mpFileWatcher->getIsDirty(mPathControlMap) || mpFileWatcher->getIsDirty(mPathControlSet))
            setupControls();
         mpFileWatcher->getAreAnyDirty(); // Forces a clear of all the dirty flags
      }
#endif
      gConsole.update();
   }

   BInputData* pInputData = NULL;

   if (mpCaptureInputThread)
   {
      BScopedCriticalSection lock(mCaptureInputLock);
      if (mCaptureInput)
      {
         if (mpInputDataWrite == &mInputData1)
         {
            mpInputDataWrite = &mInputData2;
            mpInputDataRead  = &mInputData1;
         }
         else
         {
            mpInputDataWrite = &mInputData1;
            mpInputDataRead  = &mInputData2;
         }
         for (uint i=0; i<XUSER_MAX_COUNT; i++)
         {
            mpInputDataWrite->mCount[i]=0;
            mpInputDataWrite->mIndex[i]=0;
         }
         pInputData = mpInputDataRead;
         doCapture(pInputData);
      }
   }

   for (long i=0; i < cMaxPorts; i++)
   {
      if (pInputEventHandler!=NULL)
         mGamepads[i].update(pInputData, pInputEventHandler, false);
      else
         mGamepads[i].update(pInputData, this);
   }

   if (!pInputEventHandler)
      mKeyboard->update();
}

//==============================================================================
// BInputSystem::getGamepad
//==============================================================================
BGamepad& BInputSystem::getGamepad(long port)
{
   return mGamepads[port];
}

//==============================================================================
// BInputSystem::getKeyboard
//==============================================================================
BKeyboard* BInputSystem::getKeyboard(void)
{
   return mKeyboard;
}

//==============================================================================
// BInputSystem::handleInput
//==============================================================================
bool BInputSystem::handleInput(long port, long event, long controlType, BInputEventDetail& detail)
{
   // First send the input to the event handlers
   for(long i=mEventHandlers.getNumber()-1; i>=0; i--)
   {
      if(mEventHandlers[i]->handleInput(port, event, controlType, detail))
         return true;
   }
   /*
   // First send the input to the gadget system.
   BInputEventGamepad* pEvent=getInputEventGamepad();
   if(pEvent)
   {
      pEvent->mPort=port;
      pEvent->mEvent=event;
      pEvent->mControlType=controlType;
      pEvent->mX=detail.mX;
      pEvent->mY=detail.mY;
      pEvent->mAnalog=detail.mAnalog;
      bool processed=gCore->getGadgetSys()->processInput(BInputManager::cEventGamepad, pEvent);
      clearPool();
      if(processed)
         return true;
   }
   */

   // Send the input to the active contexes
   for(long i=0; i<mActiveContextes.getNumber(); i++)
   {
      BInputContext* gamepadContext=mActiveContextes[i];
      if(gamepadContext->handleInput(port, event, controlType, detail))
         return true;
   }

   return false;
}

//==============================================================================
// BInputSystem::executeInputEvent
//==============================================================================
bool BInputSystem::executeInputEvent(long port, long event, long controlType, BInputEventDetail& detail, const BSimString& command, const BInputControl* pInputControl)
{
   if(!command.isEmpty())
   {
      // Parameter replacement
      BSimString command2;

      const BCHAR_T* str=command.getPtr();

      enum
      {
         cModeCopy,
         cModeQuotes,
         cModeReplace,
      };

      long mode=cModeCopy;
      BSimString token;

      while(*str!=NULL)
      {
         BCHAR_T c=*str;

         switch(mode)
         {
            case cModeCopy:
               if(c==B('#'))
               {
                  token.empty();
                  mode=cModeReplace;
               }
               else
               {
                  command2.append(&c, 1, 0);
                  if(c==B('"'))
                     mode=cModeQuotes;
               }
               break;

            case cModeQuotes:
               command2.append(&c, 1, 0);
               if(c==B('"'))
                  mode=cModeCopy;
               break;

            case cModeReplace:
               if(c==B('#'))
               {
                  if(token==B("x"))
                  {
                     token.format(B("%.6f"), detail.mX);
                     command2.append(token);
                  }
                  else if(token==B("y"))
                  {
                     token.format(B("%.6f"), detail.mY);
                     command2.append(token);
                  }
                  else if(token==B("analog"))
                  {
                     token.format(B("%.6f"), detail.mAnalog);
                     command2.append(token);
                  }
                  else if(token==B("port"))
                  {
                     token.format(B("%d"), port);
                     command2.append(token);
                  }
                  else if(token==B("event"))
                  {
                     token.format(B("%d"), event);
                     command2.append(token);
                  }
                  else if(token==B("control"))
                  {
                     token.format(B("%d"), controlType);
                     command2.append(token);
                  }
                  else if(token==B("start"))
                  {
                     token.format(B("%s"), event==cInputEventCommandStart||event==cInputEventCommandDouble ? B("true") : B("false"));
                     command2.append(token);
                  }
                  else if(token==B("stop"))
                  {
                     token.format(B("%s"), event==cInputEventCommandStop? B("true") : B("false"));
                     command2.append(token);
                  }
                  else if(token==B("repeat"))
                  {
                     token.format(B("%s"), event==cInputEventCommandRepeat? B("true"): B("false"));
                     command2.append(token);
                  }
                  else if(token==B("double"))
                  {
                     token.format(B("%s"), event==cInputEventCommandDouble ? B("true"): B("false"));
                     command2.append(token);
                  }
                  else
                  {
                     long replacementControl=lookupControlType(token);
                     if(replacementControl!=-1)
                     {
                        token.format(B("%s"), mGamepads[port].isControlActive(replacementControl) ? B("true") : B("false"));
                        command2.append(token);
                     }
                  }
                  token.empty();
                  mode=cModeCopy;
               }
               else
                  token.append(&c, 1, 0);
               break;
         }

         str++;
      }

      gConsole.execute(command2.getPtr());
   }

   return true;
}

//==============================================================================
// BInputSystem::lookupGamepadMap
//==============================================================================
BGamepadMap* BInputSystem::lookupGamepadMap(const BCHAR_T* gamepadName)
{
   for(long i=0; i<mGamepadMaps.getNumber(); i++)
   {
      BGamepadMap* map=mGamepadMaps[i];
      if(map->getName()==gamepadName)
         return map;
   }
   return NULL;
}

//==============================================================================
// BInputSystem::lookupControlType
//==============================================================================
long BInputSystem::lookupControlType(const BCHAR_T* controlName)
{
   long controlType=getControlMap(controlName);
   if(controlType==-1)
      controlType=lookupControl(controlName);
   return controlType;
}

//==============================================================================
// BInputSystem::getInputEventGamepad
//==============================================================================
BInputEventGamepad* BInputSystem::getInputEventGamepad()
{
   BInputEventGamepad* pEvent;
   if (mEventAlloc<mEventPool.getNumber())
      pEvent=mEventPool[mEventAlloc];
   else
   {
      BInputEventGamepad*& pool=mEventPool.grow();
      pEvent=new BInputEventGamepad;
      pool=pEvent;
   }

   pEvent->clear();
   mEventAlloc++;
   return pEvent;
}

//==============================================================================
// BInputSystem::clearPool
//==============================================================================
void BInputSystem::clearPool()
{
   mEventAlloc=0;
}

//==============================================================================
// BInputSystem::getControllerConfigIndex
//==============================================================================
long BInputSystem::getControllerConfigIndex(const char* pName)
{
   long count=mInputIntefaces.getNumber();
   for (long i=0; i<count; i++)
   {
      if (mInputIntefaces[i].getConfigName() == pName)
         return i;
   }
   return -1;
}

//==============================================================================
// BInputSystem::getInputFunctionString
//==============================================================================
BSimString BInputSystem::getInputFunctionString( BInputInterface::BInputFunctions function )
{
   BSimString result;
   if( ( function < 0 ) || ( function >= BInputInterface::cInputFunctionNum ) )
   {
      result.format( "" );
   }
   else
   {
      result = mInputFunctionStrings[function];
   }

   return( result );
}

//==============================================================================
// BInputSystem::getControlMap
//==============================================================================
long BInputSystem::getControlMap(const BSimString& name)
{
   long controlType=-1;
   mControlMap.find(name.getPtr(), &controlType);
   return controlType;
}

//==============================================================================
// BInputSystem::getControlMap
//==============================================================================
long BInputSystem::getControlMap(const char* name)
{
   long controlType=-1;
   mControlMap.find(name, &controlType);
   return controlType;
}

//==============================================================================
// BInputSystem::setCaptureInput
//==============================================================================
void BInputSystem::setCaptureInput(bool val)
{ 
   if (!mpCaptureInputThread)
      return;
   BScopedCriticalSection lock(mCaptureInputLock);
   if (mCaptureInput == val)
      return;
   if (val)
   {
      for (uint i=0; i<XUSER_MAX_COUNT; i++)
      {
         mpInputDataWrite->mCount[i]=0;
         mpInputDataWrite->mIndex[i]=0;
      }
      mCaptureInput = true;
   }
   else
   {
      mCaptureInput = false;
   }
}

//==============================================================================
// BInputSystem::captureInput
//==============================================================================
void* _cdecl BInputSystem::captureInput(void* pVal)
{
   pVal;
   SetThreadName(GetCurrentThreadId(), "InputThread");
   for (;;)
   {
      {
         BScopedCriticalSection lock(mCaptureInputLock);
         if (mCaptureInput)
            doCapture(gInputSystem.mpInputDataWrite);
      }
      Sleep(33);
   }
   return 0;
}

//==============================================================================
// BInputSystem::doCapture
//==============================================================================
void BInputSystem::doCapture(BInputData* pInputData)
{
   for (DWORD i=0; i < XUSER_MAX_COUNT; i++)
   {
      uint index = pInputData->mIndex[i];
      uint count = pInputData->mCount[i];
      if (XInputGetState(i, &(pInputData->mInputStates[i][index])) == ERROR_SUCCESS)
      {
         LARGE_INTEGER time;
         QueryPerformanceCounter(&time);
         pInputData->mTimeStamps[i][index]=time.QuadPart;
         if (count < BInputData::cMaxInputRecords)
            count++;
         if (index == BInputData::cMaxInputRecords - 1)
            index = 0;
         else
            index++;
         pInputData->mIndex[i] = index;
         pInputData->mCount[i] = count;
      }
   }
}
