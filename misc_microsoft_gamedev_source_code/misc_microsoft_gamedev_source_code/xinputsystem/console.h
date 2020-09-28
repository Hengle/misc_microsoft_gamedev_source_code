//==============================================================================
// console.h
//
// Copyright (c) 2005-2006 Ensemble Studios
//==============================================================================

#ifndef _CONSOLE_H_
#define _CONSOLE_H_

#include "debugchannel.h"
#include "consoleOutput.h"
#include "threading\eventDispatcher.h"
#ifndef BUILD_FINAL
   #include "Timer.h"
#endif

//==============================================================================
// Forward declarations
class BXSCompiler;
class BXSData;
class BXSMessenger;
class BXSSource;
class BXSSyscallModule;

// Constants
const char cDefaultConsoleSourceString[]="console";

// Callback typedef
typedef bool (*REGISTER_CONSOLE_FUNCS_CALLBACK)(BXSSyscallModule* sm);
typedef BDynamicArray<BRenderString,  4, BDynamicArrayRenderHeapAllocator> BRenderStringArray;

//==============================================================================
// UnicodeConsoleParam
//==============================================================================
struct UnicodeConsoleParam
{
   BSimString mKey;
   BSimString mParam;
};


//-- Game specific console channels (also see the generic channels in BConsoleMessageCategory in consoleOutput.h)
enum
{
   cChannelCombat = cMsgMax,
   cChannelTriggers,
   cChannelAI,
   cChannelSim,
   cChannelUI,
   cChannelMax
};

//==============================================================================
class BConsole : public IConsoleInterface, BEventReceiverInterface
{
   public:

      enum
      {
         cErrOK,
         cErrGetBinaryFailed,
         cErrCouldNotCompile,
         cErrNothingToCompile,
         cErrFile,
      };
      
      // Constructors
      BConsole( void );

      // Destructors
      ~BConsole( void );

      // Functions
      bool              setup(REGISTER_CONSOLE_FUNCS_CALLBACK registerFuncs);
      bool              cleanup();
      

      // compiles and interprets the given command immediately, or delayed if in the middle of current execution
      long              execute(const char *command, const char *src=cDefaultConsoleSourceString);
      // like normal execute, but happens when in the middle of a console command execution, to happen at termination
      void              executeDelayed(const char *command);
      // execution happens next frame
      void              executeNextFrame(const char *command);

      // executes every command in a given file
      long              execFile(long dirID, const char *filename, const char *src=cDefaultConsoleSourceString);
      void              execFileDelayed(long dirID, const char *filename);

      // handle any accumulated delayed executes
      void              checkDelayed(void);

      // once-a-frame check
      void              update(void);

      // rg - vararg version not implemented yet. Use gConsoleOutput defined in xcore\consoleOutput.h for now.
      
      // rawOutput() may be be called from any thread. This prints a message to XFS's Debug log.
      void              rawOutput(const char* szMessage);
      
      // output() may be be called from any thread. This prints a message to XFS's Commands log.
      void              output(const char* szMessage) { output(cMsgConsole, szMessage); }
            
      // May be called from any thread.            
      void              output(long category, const char* szMessage, ...);

      // Outputs text regularly and also sends it to render thread for displaying
      // one timed string at a time
      void              outputStatusText(DWORD duration, DWORD color, long category, const char* szMessage, ...);

      void              outputInfoText(DWORD color, const char* szMessage, ...);
                  
      // see what file (if any) we are in the middle of execution on
      const BSimString&    getCurrentExecFile(void) { return mCurrentFile; }

      //Console compiler.
      BXSCompiler*      getConsoleCompiler( void ) const { return(mConsoleCompiler); }
      virtual bool      initializeConsoleCompiler(REGISTER_CONSOLE_FUNCS_CALLBACK registerFuncs);
      void              destroyConsoleCompiler( void );

      //-- Unicode Console Params
      void              addUnicodeConsoleParam   (const BSimString& param, BSimString&  key);
      void              removeUnicodeConsoleParam(const BSimString& key,   BSimString& param);
      
      BXSSyscallModule* getConsoleSyscalls(void) const { return mConsoleSyscalls; }
      
      void              setChannelEnabled(long category, bool enabled);
      bool              getChannelEnabled(long category) const;

/*
#ifndef BUILD_FINAL
      void              generateXSSyscallTypeFiles(class BXSSyscallModule *sm);
#endif
*/

   virtual bool registerRemoteOutputTarget(IRemoteOutputTarget *pTarget);
   virtual bool releaseRemoteOutputTarget( void );

   protected:
      // Variables

      // used for holding console output before gadgets are initialized
      //BSimString                 mTempText;

      // for next-chance delayed execution (to avoid re-entrance in XS)
      BSimString                 mDelayedFile;
      BSimString                 mDelayedCommand;

      // for next-frame delayed execution (to avoid race conditions or mid-iteration alterations)
      BSimString                 mNextFrameCommand;

      // what, if anything, are we in the middle of processing?
      BSimString                 mCurrentFile;
      
      //XS stuff.
      BXSMessenger*           mConsoleMessenger;
      BXSCompiler*            mConsoleCompiler;
      BXSData*                mConsoleData;
      BXSSource*              mConsoleSource;
      BXSSyscallModule*       mConsoleSyscalls;
      IRemoteOutputTarget*    mpOutputTarget;

      //-- Unicode Console Params
      long                           mUnicodeConsoleParamKey;
      BCopyList<UnicodeConsoleParam> mUnicodeConsoleParams;
      
      BSimString              mConsoleChannels[cChannelMax];

      // are we currently executing a file or command?
      bool                    mExecuting;
      
      enum 
      {
         cConsoleEventClassOutput = cEventClassFirstUser
      };
      
      BEventReceiverHandle    mEventHandle;
      
      uint64                  mChannelEnabledMask;
      
      virtual bool receiveEvent(const BEvent& event, BThreadIndex threadIndex);
      
      static void outputCallback(void* data, BConsoleMessageCategory category, const char* pMsg);
      static void traceCallback(const char* pMsg);
      void createConsoleChannels(void);
      void addChannel(const char* channelName);
      
      // Functions for sending output text for render thread storage via
      // the event system + BStoredConsoleOutput
      #ifndef BUILD_FINAL
         void sendMessageForRenderStorage(long category, const char* szMessage);
         void sendCategoryHeadersForRenderStorage();
         void sendStatusTextForRenderStorage(DWORD duration, DWORD color, long category, const char* szMessage);
         void sendInfoTextForRenderStorage(DWORD color, const char* szMessage);
      #endif
};

#ifndef BUILD_FINAL

//==============================================================================
// class BStoredConsoleOutputPayload
//==============================================================================
class BStoredConsoleOutputPayload : public BEventPayload
{
   public:
      BStoredConsoleOutputPayload(long category, const char* pMessage)
      {
         mCategory = category;
         mString.set(pMessage);
      }
      
      virtual void deleteThis(bool delivered)
      {
         delete this;
      }

      long     mCategory;
      BString  mString;
};

//==============================================================================
// class BStoredConsoleOutputStatusTextPayload
//==============================================================================
class BStoredConsoleOutputStatusTextPayload : public BEventPayload
{
   public:
      BStoredConsoleOutputStatusTextPayload(DWORD duration, DWORD color, long category, const char* pMessage)
      {
         mDuration = duration;
         mColor = color;
         mCategory = category;
         mString.set(pMessage);
      }
      
      virtual void deleteThis(bool delivered)
      {
         delete this;
      }

      DWORD    mDuration;
      DWORD    mColor;
      long     mCategory;
      BString  mString;
};

//==============================================================================
// class BStoredConsoleOutput
// This class stores console output on the render thread so that it can be
// displayed (currently this is rendered in xgameRender\consoleRender).  This
// storage interface is here so that we don't introduce a dependency on
// xgameRender here.
//==============================================================================
class BStoredConsoleOutput : public BEventReceiver
{
   public:  
      BStoredConsoleOutput();
      ~BStoredConsoleOutput();

      enum
      {
         cStoredConsoleOutputEventClassMessage = cEventClassFirstUser,
         cStoredConsoleOutputEventClassHeader,
         cStoredConsoleOutputEventClassStatusText,
         cStoredConsoleOutputEventClassInfoText
      };

      // Init / deinit
      void                       init(int perCategoryStringLimit);
      void                       deinit();

      // Accessors
      const BRenderString&       getHeader(long category) const { return mHeaders[category]; }
      const BRenderStringArray&  getOutputMessageList(long category) const { return mOutputMessageList[category]; }
      const BRenderString&       getStatusText() const { return mStatusText; }
      BTimer&                    getStatusTextTimer() { return mStatusTextTimer; }
      DWORD                      getStatusTextDuration() const { return mStatusTextDuration; }
      long                       getStatusTextCategory() const { return mStatusTextCategory; }
      DWORD                      getStatusTextColor() const { return mStatusTextColor; }

      const BRenderString&       getInfoText() const { return mInfoText; }
      DWORD                      getInfoTextColor() const { return mInfoTextColor; }

   protected:

      virtual bool               receiveEvent(const BEvent& event, BThreadIndex threadIndex);

      // Console text
      BRenderString              mHeaders[cChannelMax];
      BRenderStringArray         mOutputMessageList[cChannelMax];
      int                        mPerCategoryStringLimit;

      // Status text data
      BRenderString              mStatusText;
      BTimer                     mStatusTextTimer;
      long                       mStatusTextCategory;
      DWORD                      mStatusTextDuration;
      DWORD                      mStatusTextColor;

      // Info text data
      BRenderString              mInfoText;
      DWORD                      mInfoTextColor;
};

extern BStoredConsoleOutput gStoredConsoleOutput;

#endif


extern BConsole gConsole;

#endif
