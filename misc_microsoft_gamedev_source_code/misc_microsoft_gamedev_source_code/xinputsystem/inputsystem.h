//==============================================================================
// inputsystem.h
//
// Copyright (c) 2005 Ensemble Studios
//==============================================================================
#ifndef __INPUTSYSTEM_H__
#define __INPUTSYSTEM_H__
#pragma once

// Includes
#ifndef XBOX
   #define DIRECTINPUT_VERSION 0x800
   #include <dinput.h>
#endif

#include "bitarray.h"
#include "console.h"
#include "inputcontrolenum.h"
#include "inputinterface.h"
#include "string\stringtable.h"
#include "reloadManager.h"

// Forward declarations
class BGamepad;
class BGamepadMap;
class BInputContext;
class BInputEventGamepad;
class BInputSystem;
class BKeyboard;
class BInputInterface;
class BThread;
class BInputControl;
class BFileWatcher;

// Externs
extern BInputSystem gInputSystem;

//==============================================================================
// Input events
//==============================================================================
enum BInputEventType
{
   cInputEventControlStart,
   cInputEventControlRepeat,
   cInputEventControlStop,
   cInputEventCommandStart,
   cInputEventCommandRepeat,
   cInputEventCommandStop,
   cInputEventCommandDouble,
   cInputEventDeviceRemoved,
   cInputEventDeviceInserted,
};

//==============================================================================
// BInputEventDetail
//==============================================================================
class BInputEventDetail
{
   public:
      BInputEventDetail() : mTime(0), mX(0.0f), mY(0.0f), mAnalog(0.0f) { }

      int64    mTime;
      float    mX;
      float    mY;
      float    mAnalog;
};

//==============================================================================
// BInputData
//==============================================================================
class BInputData
{
   public:
      enum { cMaxInputRecords = 60 };
      XINPUT_STATE   mInputStates[XUSER_MAX_COUNT][cMaxInputRecords];
      int64          mTimeStamps[XUSER_MAX_COUNT][cMaxInputRecords];
      uint           mIndex[XUSER_MAX_COUNT];
      uint           mCount[XUSER_MAX_COUNT];
};

//==============================================================================
// IInputEventHandler
//==============================================================================
class IInputEventHandler
{
   public:
      virtual bool handleInput(long port, long event, long controlType, BInputEventDetail& detail) = 0;
};

class IInputControlEventHandler
{
   public:
      virtual bool executeInputEvent(long port, long event, long controlType, BInputEventDetail& detail, const BSimString& command, const BInputControl* pInputControl) = 0;
};

//==============================================================================
// BInputSystem
//==============================================================================
class BInputSystem : public IInputControlEventHandler, public IInputEventHandler
{
   public:
      enum
      {
#ifdef XBOX
         cMaxPorts=XUSER_MAX_COUNT,
#else
         cMaxPorts=4,
#endif
      };

      enum
      {
         cControlStatusNone=0,
         cControlStatusDown,
         cControlStatusUp,
      };

                           BInputSystem();
                           ~BInputSystem();

      bool                 setup(HWND windowHandle, REGISTER_CONSOLE_FUNCS_CALLBACK registerConsoleFuncs);
      void                 update(IInputEventHandler* pInputEventHandler=NULL);
      void                 shutdown();

      void                 setCaptureInput(bool val);

      void                 addEventHandler(IInputEventHandler* handler) { mEventHandlers.add(handler); }
      void                 removeEventHandler(IInputEventHandler* handler) { mEventHandlers.remove(handler); }

      void                 enterContext(const BCHAR_T* contextName);
      void                 leaveContext(const BCHAR_T* contextName);
      bool                 isContextActive(const BCHAR_T* contextName);

#ifndef XBOX
      IDirectInput8*       getDirectInput() { return mDirectInput; }
#endif
      BKeyboard*           getKeyboard(void);
      BGamepad&            getGamepad(long port);
      HWND                 getWindowHandle() const { return mWindowHandle; }

      bool                 handleInput(long port, long event, long controlType, BInputEventDetail& detail);

      // IInputControlEventHandler 
      bool                 executeInputEvent(long port, long event, long controlType, BInputEventDetail& detail, const BSimString& command, const BInputControl* pInputControl);

#ifndef XBOX
      bool                 createGamepadDevice(REFGUID guid, BGamepadMap* map);
#endif

      BGamepadMap*         lookupGamepadMap(const BCHAR_T* gamepadName);
      long                 lookupControlType(const BCHAR_T* controlName);

      long                 getControlMap(const BSimString& name);
      long                 getControlMap(const char* name);

      void                 setupControls();

      BInputEventGamepad*  getInputEventGamepad();
      void                 clearPool();

      bool                 getUseXInput() { return mUseXInput; }

      BInputInterfaceArray* getInputInterfaces(){ return( &mInputIntefaces ); }
      long                  getControllerConfigIndex(const char* pName);
      BSimString            getInputFunctionString( BInputInterface::BInputFunctions function );

   protected:
      bool                 setupGamepads();
      bool                 loadControlMap(const BCHAR_T* name);
      bool                 loadControls(const BCHAR_T* name);
      bool                 loadControlConfigs( const BCHAR_T* name );

      static void* _cdecl  captureInput(void* pVal);
      static void          doCapture(BInputData* pInputData);

      BOOL                                mInitialized;
      HWND                                mWindowHandle;
      BKeyboard*                          mKeyboard;
      BGamepad*                           mGamepads;
      BStringTable<long>                  mControlMap;
      BSmallDynamicSimArray<BInputContext*>        mContextList;
      BSmallDynamicSimArray<BInputContext*>        mActiveContextes;
#ifndef XBOX
      IDirectInput8*                      mDirectInput;
#endif
      long                                mGamepadDeviceCount;
      BSmallDynamicSimArray<BGamepadMap*>          mGamepadMaps;
      BBitArray                           mInputContextes;
      BSmallDynamicSimArray<BInputEventGamepad*>   mEventPool;
      long                                mEventAlloc;
      bool                                mUseXInput;
      BSmallDynamicSimArray<IInputEventHandler*>   mEventHandlers;

      BInputInterfaceArray                mInputIntefaces;
      BSimString                          mInputFunctionStrings[BInputInterface::cInputFunctionNum];

      static BInputData                   mInputData1;
      static BInputData                   mInputData2;
      static BInputData*                  mpInputDataWrite;
      static BInputData*                  mpInputDataRead;

      static BThread*                     mpCaptureInputThread;
      static BCriticalSection             mCaptureInputLock;
      static bool                         mCaptureInput;

#ifndef BUILD_FINAL
      BFileWatcher*              mpFileWatcher;
      BFileWatcher::BPathHandle  mPathControlMap;
      BFileWatcher::BPathHandle  mPathControlSet;
#endif

};

#endif //__INPUTSYSTEM_H__