//==============================================================================
// bassert.h
//
// Copyright (c) 2000-2006, Ensemble Studios
//==============================================================================
#pragma once

//==============================================================================
// Forward declaration
//==============================================================================
class BDebugCallstack;

//==============================================================================
// The TRIGGER_BREAK_EXCEPTION macro triggers a breakpoint if the debugger
// is connected, without explicitly calling a function. (Calling a function
// to trigger a breakpoint would convert leaf functions into slower non-leaf 
// functions.)
//==============================================================================


//==============================================================================
// Callback function prototype.
//==============================================================================
typedef void (*BASSERT_CALLBACK_FUNC)(
   const char *expression, const char *msg, const char *file, long line, bool fatal, bool noreturn, 
   const BDebugCallstack* pCallstack, void *param1, void *param2, const char* pFullAssertError );


//==============================================================================
// class BAssertCallbackEntry
//==============================================================================
class BAssertCallbackEntry
{
   public:
      BASSERT_CALLBACK_FUNC   mFunction;
      void                    *mParam1;
      void                    *mParam2;
};


//==============================================================================
// class BAssertIgnoreEntry
//==============================================================================
class BAssertIgnoreEntry
{
   public:
      char                    mFile[128];
      long                    mLine;
};


//==============================================================================
// class BAssert
//==============================================================================
class BAssert
{
      BAssert(const BAssert&);
      BAssert& operator= (const BAssert&);
   
   public:
                              BAssert();
      
      long                    fail(const char *expression, const char *msg, const char *file, long line, bool fatal, bool triggeredByException = false);
      __declspec(noreturn) void failNoReturn(const char *expression, const char *msg, const char *file, long line, bool fatal, bool triggeredByException = false);
      
      const char              *getFile(void) const {return(mFile);}
      long                    getLine(void) const {return(mLine);}

      const char              *getMessage(void) const {return(mMessage);}
      const char              *getExpression(void) const {return(mExpression);}
      bool                    isFatal(void) const {return(mFatal);}

      HFONT                   getLargeFont(void) const {return(mLargeFont);}
      HFONT                   getBoldFont(void) const {return(mBoldFont);}
      HFONT                   getFixedFont(void) const {return(mFixedFont);}

      const BCHAR_T          *getCallstackString(void) const {return(mCallstackString);}

      // The pre/post callbacks may be called from any thread!
      bool                    addPreCallback(BASSERT_CALLBACK_FUNC function, void *param1, void *param2);
      bool                    addPostCallback(BASSERT_CALLBACK_FUNC function, void *param1, void *param2);
      void                    clearCallbacks(void);
      void                    clearPreCallbacks(void);
      void                    clearPostCallbacks(void);

      void                    setIgnoreAsserts(bool ignore) {mIgnoreAsserts=ignore;}
      bool                    getIgnoreAsserts(void) const {return(mIgnoreAsserts);}

      void                    setShowDialog(bool ignore) {mShowDialog=ignore;}
      bool                    getShowDialog(void) const {return(mShowDialog);}

#ifdef XBOX      
      struct BExceptionAssertData
      {
         bool*         mpIgnoreFlag;
         const char*   mpExp;
         const char*   mpMsg;
         int           mLine;
         const char*   mpFile;
      };
      static volatile BExceptionAssertData mAssertData;
      
      enum { cXboxMagicBreakAddress = 0xDEAD0000 };
      int                     xboxHandleAssertException(_EXCEPTION_POINTERS* pExcept);
            
      typedef int (*BExceptionFilterFuncPtr)(_EXCEPTION_POINTERS* pExcept, DWORD data);
      bool                    xboxAddExceptionFilterFunc(BExceptionFilterFuncPtr pFunc, DWORD data); 
      bool                    xboxRemoveExceptionFilterFunc(BExceptionFilterFuncPtr pFunc, DWORD data);
      void                    xboxClearExceptionFilterFuncs(void) { mNumExceptionFilters = 0; }
#endif

      long                    getIsHandlingFailure(void) 
      { 
#ifdef XBOX         
         __lwsync();
#endif         
         return mHasFailed; 
      }

   protected:
      bool                    testCacheDrive();

      bool                    isAssertionOk(const char *file, long line);
      bool                    addCallback(BAssertCallbackEntry *list, long &count, BASSERT_CALLBACK_FUNC function, void *param1, void *param2);
      void                    sendReport(const char *expression, const char *msg, const char *file, long line, bool fatal, BDebugCallstack &callstack);

      long                    platformFail(const char *expression, const char *msg, const char *file, long line, bool fatal, bool noreturn, bool triggeredByException);

      const char              *mFile;
      const char              *mMessage;
      const char              *mExpression;
      const char              *mCallstack;
      long                    mLine;
      bool                    mFatal;

      bool                    mIgnoreAsserts;
      bool                    mShowDialog;
      bool                    mInitialized;
      bool                    mUseCacheDrive;
      bool                    mFirstWrite;

      HFONT                   mLargeFont;
      HFONT                   mBoldFont;
      HFONT                   mFixedFont;
            
      BCriticalSection        mCriticalSection;
      
      volatile                long mHasFailed;

      enum
      {
#ifdef XBOX
         cCallstackStringSize = 1,
#else      
         cCallstackStringSize = 16284,
#endif         
         cCallbackListSize = 32,
         cIgnoreListSize = 16
      };
      
      BCHAR_T                 mCallstackString[cCallstackStringSize];
      BAssertIgnoreEntry      mIgnoreList[cIgnoreListSize];
      long                    mIgnoreListCount;
      BAssertCallbackEntry    mPreCallbackList[cCallbackListSize];
      long                    mPreCallbackListCount;
      BAssertCallbackEntry    mPostCallbackList[cCallbackListSize];
      long                    mPostCallbackListCount;

#ifdef XBOX
      enum { cMaxExceptionFilters = 16 };
      uint mNumExceptionFilters;
      std::pair<BExceptionFilterFuncPtr, DWORD> mExceptionFilters[cMaxExceptionFilters];
      BCriticalSection mExceptionFilterCriticalSection;
#endif      
      
};

// Macros for assertion exceptions

#ifdef XBOX
   #if defined(CODE_ANALYSIS_ENABLED)
      #define BASSERT_TRIGGER_EXCEPTION(exp, msg, file, line)		exit(1)
   #elif BUILD_CHECKED
      #define BASSERT_TRIGGER_EXCEPTION(exp, msg, file, line) \
         do { \
            BAssert::mAssertData.mpFile = __FILE__; \
            BAssert::mAssertData.mLine = __LINE__; \
            *((volatile DWORD*)BAssert::cXboxMagicBreakAddress) = (DWORD)BAssert::cXboxMagicBreakAddress; \
         } while(0)
   #else
      #define BASSERT_TRIGGER_EXCEPTION(exp, msg, file, line) \
         do {  \
            static bool ignoreFlag; \
            if (!ignoreFlag) \
            { \
               BAssert::mAssertData.mpExp = #exp; \
               BAssert::mAssertData.mpMsg = msg; \
               BAssert::mAssertData.mpFile = __FILE__; \
               BAssert::mAssertData.mLine = __LINE__; \
               BAssert::mAssertData.mpIgnoreFlag = &ignoreFlag; \
               *((volatile DWORD*)BAssert::cXboxMagicBreakAddress) = (DWORD)BAssert::cXboxMagicBreakAddress; \
            } \
         } while(0)
   #endif   
#endif

extern BAssert gAssertionSystem;

// rg [1/27/06] - __declspec(noreturn) versions are intended for use on 360. 
// This always functions that use asserts, fails, etc. to be still optimized as leaf functions by the compiler, 
// which is a big perf. win, particularly in debug builds.

// rg [1/27/06] - The FATAL assert/fail macros are included in all builds, even final. They are marked as noreturn,
// so the perf. impact should be minimal. Obviously, don't use them if you don't intend on having them checked in 
// final builds.
//
// The non-fatal macros may continue execution, depending on how the build was configured. The fatal variants will
// never continue execution. 
// 
// The BDEBUG variants are only ever checked in DEBUG builds.

// FATAL variants can't return -- ever. 
#define BFATAL_ASSERT(exp)          do { if (!(exp)) gAssertionSystem.failNoReturn(#exp, NULL, __FILE__, __LINE__, true); } while(0)
#define BFATAL_ASSERTM(exp, msg)    do { if (!(exp)) gAssertionSystem.failNoReturn(#exp, msg, __FILE__, __LINE__, true); } while(0)
#define BFATAL_FAIL(msg)            do { gAssertionSystem.failNoReturn(NULL, msg, __FILE__, __LINE__, true); } while(0)

#define BVERIFY(exp)                BFATAL_ASSERT(exp)
#define BVERIFYM(exp, msg)          BFATAL_ASSERTM(exp, msg)

#ifdef ENABLE_BASSERT_NORETURN

   #ifdef XBOX
	   #if defined(CODE_ANALYSIS_ENABLED)
		   #define BASSERT(exp)                   __analysis_assume((exp))
		   #define BASSERTM(exp, msg)             __analysis_assume((exp))
           #define BFAIL(msg)                     exit(1)
		   #define BDEBUG_ASSERT(exp)             __analysis_assume((exp))
           #define BDEBUG_ASSERTM(exp, msg)       __analysis_assume((exp))
	   #else
         // Normal asserts.
         #ifdef ENABLE_BASSERT_NORMAL
            #define BASSERT(exp)                   do { if (!(exp)) { BASSERT_TRIGGER_EXCEPTION(#exp, NULL, __FILE__, __FILE__); } } while(0)
            #define BASSERTM(exp, msg)             do { if (!(exp)) { BASSERT_TRIGGER_EXCEPTION(#exp, msg, __FILE__, __FILE__); } } while(0)
            #define BFAIL(msg)                     do { BASSERT_TRIGGER_EXCEPTION(NULL, msg, __FILE__, __FILE__); } while(0);
         #else // !ENABLE_BASSERT_NORMAL
            #define BASSERT(exp)                   ((void)0)
            #define BASSERTM(exp, msg)             ((void)0)
            #define BFAIL(msg)                     ((void)0)
         #endif // ENABLE_BASSERT_NORMAL
         
         // Debug-only asserts.
         #ifdef ENABLE_BASSERT_DEBUGONLY
            #define BDEBUG_ASSERT(exp)             do { if (!(exp)) { BASSERT_TRIGGER_EXCEPTION(#exp, NULL, __FILE__, __FILE__); } } while(0)
            #define BDEBUG_ASSERTM(exp, msg)       do { if (!(exp)) { BASSERT_TRIGGER_EXCEPTION(#exp, msg, __FILE__, __FILE__); } } while(0)
         #else // !ENABLE_BASSERT_DEBUGONLY
            #define BDEBUG_ASSERT(exp)             ((void)0)
            #define BDEBUG_ASSERTM(exp, msg)       ((void)0)
         #endif // ENABLE_BASSERT_DEBUGONLY
	   #endif   // CODE_ANALYSIS_ENABLED     
   #else // !XBOX
  
      // Normal asserts.
      #ifdef ENABLE_BASSERT_NORMAL
         // Non-fatal versions
         #define BASSERT(exp)                   do { if (!(exp)) gAssertionSystem.failNoReturn(#exp, NULL, __FILE__, __LINE__, false); } while(0)
         #define BASSERTM(exp, msg)             do { if (!(exp)) gAssertionSystem.failNoReturn(#exp, msg, __FILE__, __LINE__, false); } while(0)
         #define BFAIL(msg)                     do { gAssertionSystem.failNoReturn(NULL, msg, __FILE__, __LINE__, false); } while(0)
      #else // !ENABLE_BASSERT_NORMAL
         #define BASSERT(exp)                   ((void)0)
         #define BASSERTM(exp, msg)             ((void)0)
         #define BFAIL(msg)                     ((void)0)
      #endif // ENABLE_BASSERT_NORMAL

      // Debug-only asserts.
      #ifdef ENABLE_BASSERT_DEBUGONLY
         // Normal versions
         #define BDEBUG_ASSERT(exp)             do { if (!(exp)) gAssertionSystem.failNoReturn(#exp, NULL, __FILE__, __LINE__, false); } while(0)
         #define BDEBUG_ASSERTM(exp, msg)       do { if (!(exp)) gAssertionSystem.failNoReturn(#exp, msg, __FILE__, __LINE__, false); } while(0)
      #else // !ENABLE_BASSERT_DEBUGONLY
         #define BDEBUG_ASSERT(exp)             ((void)0)
         #define BDEBUG_ASSERTM(exp, msg)       ((void)0)
      #endif
   #endif // XBOX      
   
   // Disable unreachable code warning. This sucks, but if not done we'll be flooded with useless warnings.
   #pragma warning(disable:4702) 

#else // !ENABLE_BASSERT_NORETURN

   // Normal asserts.
   #ifdef ENABLE_BASSERT_NORMAL
      // Non-fatal versions
      #define BASSERT(exp)                   (void)( (exp) || (gAssertionSystem.fail(#exp, NULL, __FILE__, __LINE__, false)));
      #define BASSERTM(exp, msg)             (void)( (exp) || (gAssertionSystem.fail(#exp, msg, __FILE__, __LINE__, false)));
      #define BFAIL(msg)                     (void)((gAssertionSystem.fail(NULL, msg, __FILE__, __LINE__, false)));
   #else
      #define BASSERT(exp)                   ((void)0)
      #define BASSERTM(exp, msg)             ((void)0)
      #define BFAIL(msg)                     ((void)0)
   #endif

   // Debug-only asserts.
   #ifdef ENABLE_BASSERT_DEBUGONLY
      // Normal versions
      #define BDEBUG_ASSERT(exp)             (void)( (exp) || (gAssertionSystem.fail(#exp, NULL, __FILE__, __LINE__, false)));
      #define BDEBUG_ASSERTM(exp, msg)       (void)( (exp) || (gAssertionSystem.fail(#exp, msg, __FILE__, __LINE__, false)));
   #else
      #define BDEBUG_ASSERT(exp)             ((void)0)
      #define BDEBUG_ASSERTM(exp, msg)       ((void)0)
   #endif
  
#endif  // ENABLE_BASSERT_NORETURN

#define BASSERT_WRITE_POINTER(x) BASSERT(!IsBadWritePtr(x, sizeof(DWORD)))
#define BASSERT_READ_POINTER(x)  BASSERT(!IsBadReadPtr(x, sizeof(DWORD)))
#define BASSERT_CODE_POINTER(x)  BASSERT(!IsBadCodePtr(x, sizeof(DWORD)))

#define BDEBUG_ASSERT_WRITE_POINTER(x) BDEBUG_ASSERT(!IsBadWritePtr(x, sizeof(DWORD)))
#define BDEBUG_ASSERT_READ_POINTER(x)  BDEBUG_ASSERT(!IsBadReadPtr(x, sizeof(DWORD)))
#define BDEBUG_ASSERT_CODE_POINTER(x)  BDEBUG_ASSERT(!IsBadCodePtr(x, sizeof(DWORD)))
