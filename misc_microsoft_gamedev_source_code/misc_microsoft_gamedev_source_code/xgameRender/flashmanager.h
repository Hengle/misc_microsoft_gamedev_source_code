//============================================================================
//
// flashmanager.h
// Copyright 2006 (c) Ensemble Studios
//============================================================================
#pragma once

#include "renderThread.h"

#include "scaleformIncludes.h"

#include "flashtexture.h"
#include "GRendererXbox360.h"
#include "renderToTextureXbox.h"
#include "xmlreader.h"
#include "flashallocator.h"
#include "flashtranslator.h"

class BFlashMovie;
class BFlashMovieDefinition;

//============================================================================
//============================================================================
class BFlashPlayerSettings
{
   public:
      BFlashPlayerSettings()
      {
         // Default values
         ScaleX = ScaleY     = 1.0f;
         TexLodBias          = -0.5f;
         AAMode              = AAMode_EdgeAA;
         BitDepth            = 32;
         Background          = 1;
         MeasurePerformance  = 1;
         FullScreen          = 1;
         HudState            = Hud_Hidden;

         VerboseParse        = 0;
         VerboseParseShape   = 0;
         VerboseParseAction  = 0;
         VerboseAction       = 0;
#ifdef BUILD_DEBUG
         Quiet               = 0;
         NoActionErrors      = 0;
#else
         Quiet               = 1;
         NoActionErrors      = 1;
#endif
         DoLoop              = 1;
         DoRender            = 1;
         DoSound             = 0;

         FastForward         = 0;

         ExitTimeout         = 0.0f;
         SleepDelay          = 31;

         // Clear file
         FileName[0]         = 0;
      }

      void initSafeAreaParams(int viewportWidth, int viewportHeight, float titleSafePct);

      enum AAModeType
      {
         AAMode_None,        // No anti-aliasing is used.
         AAMode_EdgeAA,      // Edge AA is used (if supported by GRenderer).
         AAMode_FSAA         // HW Full-screen AA is used (if supported by device).
      };

      enum    HudStateType
      {
         Hud_Hidden,
         Hud_Stats,
         Hud_Help
      };

   UInt        BitDepth;
   Float       ScaleX, ScaleY;
   Float       TexLodBias;
   AAModeType  AAMode;
   bool        Background;
   bool        MeasurePerformance; 
   bool        FullScreen;

   

   // Display Hud at startup
   HudStateType    HudState;

   // Verbose options
   bool    VerboseParse;
   bool    VerboseParseShape;
   bool    VerboseParseAction;
   bool    VerboseAction;
   bool    Quiet;
   bool    NoActionErrors;

   // Rendering state
   bool    DoLoop;
   bool    DoRender;
   bool    DoSound;

   // Set to play movie as fast as possible
   bool    FastForward;

   Float   ExitTimeout;
   UInt    SleepDelay;

   // PlaybackFile
   char    FileName[256];   


   // safe area params
   float mfSafeX1;
   float mfSafeY1;
   float mfSafeX2;
   float mfSafeY2;
   float mfSafeWidth;
   float mfSafeHeight;
   float mfCenterX;
   float mfCenterY;
   float mfWidth;
   float mfHeight;   
};


//============================================================================
//============================================================================
class BFlashPlayerLog : public GFxLog
{
public: 
   // We override this function in order to do custom logging.
   virtual void    LogMessageVarg(LogMessageType messageType, const char* pfmt, va_list argList)
   {
      #ifndef BUILD_FINAL
         // Output log to console
         vprintf(pfmt, argList);
      #endif
   }
};

//============================================================================
//============================================================================
class BFlashEventReceiver
{
   public:
       BFlashEventReceiver() {clear();};
      ~BFlashEventReceiver() {clear();};

      void clear()
      {
         mpMovie = NULL;
         mHandle = cInvalidEventReceiverHandle;
      }

      BFlashEventReceiver& operator=(const BFlashEventReceiver& rhs)
      {
         if (this == &rhs)
            return *this;

         mpMovie = rhs.mpMovie;
         mHandle = rhs.mHandle;
         return *this;
      }

   BFlashMovie*         mpMovie;
   BEventReceiverHandle mHandle;
};

//============================================================================
//============================================================================
class BFlashManager : public BEventReceiver, public GFxFSCommandHandler, public GFxFileOpener
{  
   public:
      BFlashManager();
     ~BFlashManager();

      bool init();
      bool initFonts(const BFixedString<128>& languageStr, long directory);
      void deinit();
      void clear();
      void update();   
      
      enum BCallbackCommand
      {
         eEnterMode,
         eTotalCallbackCommands,
      };

      enum BEventCallBackType
      {
         cFlashEventFSCommand = cEventClassFirstUser,
         cFlashEventFileReload,
         cFlashUpdateStats,
         cFlashInitTextureManager,
         cFlashManagerDeinit,
         cFlashUpdateMemoryStats,
      };

      GFxLoader*             getLoader(BFlashAssetCategory category);
      BFlashPlayerLog*       getLog() { return &mLog; };
      GPtr<GRendererXbox360> getRenderer() const {return mpRenderer;};
      const BFlashPlayerSettings& getSettings() const {return mSettings;};

      BFlashMovieDefinition* getData(const BString& filename, BFlashAssetCategory category, int* pIndex);
      BFlashMovieDefinition* getData(int index);
      int                    find(const BString& filename);      

      BFlashMovie*           createMovie(int index, bool bRenderToTexture);
      void                   releaseMovie(BFlashMovie* pMovie);
      void                   renderBegin();
      void                   render();
      void                   renderEnd();

      virtual bool           receiveEvent(const BEvent& event, BThreadIndex threadIndex);
      BFlashMovieDefinition* reloadData(int index);
      bool                   reloadMovie(const BString& path);

      void                   registerFSCallbackReceiver(BFlashMovie* pMovie, BEventReceiverHandle handle);
      void                   unregisterFSCallbackReiver(BFlashMovie* pMovie, BEventReceiverHandle handle);
      int                    getNumberEventReceivers() {return mEventReceivers.getNumber();};
      BFlashEventReceiver*   getEventReceiver(int index);

      void                   setEnableBatching(bool bEnable);
      void                   setEnableSWFLoading(bool bEnable);
      void                   setEnableWireframe(bool bEnable);

      void                   getFontCacheConfig(GFxFontCacheManager::TextureConfig& config) { config = mFontCacheConfig; }

      void                   unloadPregameUITextures();
      void                   loadPregameUITextures();


      // GFx CommandHandler API
      virtual void Callback(GFxMovieView* pmovie, const char* command, const char* args);

      // GFx FileOpener API
      virtual GFile* OpenFile(const char *purl);

      // FileOpen callback function used by loader
      //static GFile*   GCDECL FileOpener(const char* url);

      // "fscommand" callback, handles notification callbacks from ActionScript.
      //static void     GCDECL FsCallback(GFxMovieView* movie, const char* command, const char* args);      

      class BFSCommandPayload : public BEventPayload
      {
         public:
            BFSCommandPayload(const char* command, const char* args) : mCommand(command), mArgs(args){ }
            BFixedString128 mCommand;
            BFixedString128 mArgs;
         private:
            virtual void deleteThis(bool delivered) { delivered; delete this; }
      };

      class BFileReloadCommandPayload : public BEventPayload
      {
         public:
            BFileReloadCommandPayload(const char* path) : mCommand(path){ }
            BFixedString128 mCommand;
         private:
            virtual void deleteThis(bool delivered) { delivered; delete this; }
      };

#ifndef BUILD_FINAL   
      struct BStats :  public BEventPayload
      {
         public: 
            BStats(){clear();};

            void clear(void)
            {
               mResolvesPerFrame=0;
               mResolvedPixelsPerFrame=0;
               mMoviesRenderedPerFrame=0;
               mDrawCallsPerFrame=0;
               mASInvokesPerFrame=0;
               mASSetVariablePerFrame=0;
               mASInvokesTotalTime=0.0f;
               mLines=0;
               mPrimitives=0;
               mTriangles=0;
               mCPUTimeAdvance=0.0f;
               mCPUTimeDisplay=0.0f;     
               mDecalMoviesRenderedPerFrame=0;

               memset(mPSDrawCalls, 0, sizeof(mPSDrawCalls));
               memset(mVSDrawCalls, 0, sizeof(mVSDrawCalls));
            }

            // Okay for this to use the primary heap.
            virtual void deleteThis(bool delivered) { delete this; }            

            int      mPSDrawCalls[25];// == PS_Count in GRendererXbox360Impl.cpp
            int      mVSDrawCalls[6]; // == VS_Count in grendererxbox360Impl.cpp

            int      mResolvesPerFrame;
            int      mResolvedPixelsPerFrame;
            int      mMoviesRenderedPerFrame;
            int      mDecalMoviesRenderedPerFrame;
            int      mDrawCallsPerFrame;
            int      mASInvokesPerFrame;
            double   mASInvokesTotalTime;
            int      mASSetVariablePerFrame;
            int      mLines;
            int      mPrimitives;
            int      mTriangles;
            double   mCPUTimeAdvance;
            double   mCPUTimeDisplay;
      }; 

      void getSimStats(BStats& stats) { ASSERT_MAIN_THREAD; stats = mSimStats; } 
      void refreshStats();
      BStats mWorkerStats;
      BStats mSimStats;      
            
      struct BFlashAssetAllocStats
      {
         BString  mFilename;
         uint     mAllocationSize;
         uint     mCategory;
         
         BFlashAssetAllocStats& operator=(const BFlashAssetAllocStats& rhs)
         {
            if (this == &rhs)
               return *this;

            mFilename = rhs.mFilename;
            mAllocationSize = rhs.mAllocationSize;
            mCategory = rhs.mCategory;
            return *this;
         }

         bool operator< (const BFlashAssetAllocStats& rhs) const { return mAllocationSize > rhs.mAllocationSize; }
      };
      typedef BDynamicArray<BFlashAssetAllocStats> BFlashAssetAllocStatsArray;

      void getProtoAllocStats(BFlashAssetAllocStatsArray& stats)
      {
         stats.clear();
         for (uint i = 0; i < mFlashProtoAssetStats.getSize(); ++i)
         {
            BFlashAssetAllocStats* p = stats.enlarge(1);
            p->mAllocationSize = mFlashProtoAssetStats[i].mAllocationSize;
            p->mFilename = mFlashProtoAssetStats[i].mFilename;
            p->mCategory = mFlashProtoAssetStats[i].mCategory;
         }
      }

      void getInstanceAllocStats(BFlashAssetAllocStatsArray& stats)
      {
         stats.clear();
         for (uint i = 0; i < mFlashInstanceAssetStats.getSize(); ++i)
         {
            BFlashAssetAllocStats* p = stats.enlarge(1);
            p->mAllocationSize = mFlashInstanceAssetStats[i].mAllocationSize;
            p->mFilename = mFlashInstanceAssetStats[i].mFilename;
            p->mCategory = mFlashInstanceAssetStats[i].mCategory;
         }
      }

      void getFlashFontCacheStats(BFlashAssetAllocStats& stats)
      {         
         stats = mFlashFontCacheStats;
      }

      void clearInstanceMemoryStats(uint category);
      void clearProtoMemoryStats(uint category);

      BFlashAssetAllocStatsArray mFlashProtoAssetStats;
      BFlashAssetAllocStatsArray mFlashInstanceAssetStats;    
      BFlashAssetAllocStats      mFlashFontCacheStats;
#endif
      
   private:      
      void initLoader(BFlashAssetCategory category, BFlashRender* pRenderer, GFxRenderConfig* pRenderConfig, GFxParseControl* pParseControl, GFxRenderStats* pRenderStats, GFxLog* pLog, BFlashTranslator* pTranslator);
      void initCustomTextureLoaders(BFlashAssetCategory category, BFlashRender* pRenderer);
      void initFontCacheManager();
      bool loadMovieDefinition(const BString& fileName, BFlashAssetCategory category, int* pIndex);
      void releaseMovieDefintion(BFlashMovieDefinition* pData);

      int  findCallBackCommand(const char* command);
      int  findMovieParent(const GFxMovieView* pMovie);
      void loadFont(BXMLNode node);

      BDynamicArray<BFlashMovieDefinition*> mData;
      BDynamicArray<BFlashMovie*>           mMovies;
      BDynamicArray<BFlashEventReceiver>    mEventReceivers;            

      GPtr<GFxFontLib> mpFontLib;
      GPtr<GFxFontMap> mpFontMap;

      BFlashPlayerSettings   mSettings;
      // Renderer we use
      BFlashRender*          mpRenderer;
      GFxRenderConfig*       mpRenderConfig;
      GFxRenderStats*        mpRenderStats;
      GFxParseControl*       mpParseControl;
      GPtr<GFxFontCacheManager> mSharedFontCacheManager;
      BFlashTextureCreator*  mpTextureCreator[cFlashAssetCategoryTotal];
      BFlashTextureLoader*   mpTextureLoader[cFlashAssetCategoryTotal];
      GFxLoader*             mpLoader[cFlashAssetCategoryTotal];
      BFlashPlayerLog        mLog;
      BFlashTranslator*      mpTranslator;

      GFxFontCacheManager::TextureConfig mFontCacheConfig;

      BEventReceiverHandle   mSimEventHandle;
      bool                   mLoadSWFFiles;
      BString                mFontDefinitionFile;

      

};

extern BFlashManager gFlashManager;