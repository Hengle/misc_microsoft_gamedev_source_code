// File: gfxMoviePlayer.h
#pragma once

#include "scaleformincludes.h"

class BXMLNode;
class BFlashTranslator;

//============================================================================
// mSettings class stores playback settings determined
// based on the comand-line parameters
//============================================================================
class   FxPlayerSettings
{

public:
   enum AAModeType
   {
      AAMode_None,        // No anti-aliasing is used.
      AAMode_EdgeAA,      // Edge AA is used (if supported by GRenderer).
      AAMode_FSAA         // HW Full-screen AA is used (if supported by device).
   };

   Float       ScaleX, ScaleY;
   Float       TexLodBias;
   AAModeType  AAMode;
   bool        Background;

   // Verbose options
   bool    VerboseParse;
   bool    VerboseParseShape;
   bool    VerboseParseAction;
   bool    VerboseAction;
   bool    Quiet;

   // Rendering state
   bool    DoLoop;

   // Set to play movie as fast as possible
   bool    FastForward;

   FxPlayerSettings()
   {
      // Default values
      ScaleX = ScaleY     = 1.0f;
      TexLodBias          = -0.5f;
      AAMode              = AAMode_EdgeAA;

      Background          = 1;

      VerboseParse        = 0;
      VerboseParseShape   = 0;
      VerboseParseAction  = 0;
      VerboseAction       = 0;
      Quiet               = 0;

      DoLoop              = 1;

      FastForward         = 0;
   }
};

//============================================================================
//============================================================================
class BGFXMoviePlayerFileOpener: public GFxFileOpener
{
   public:
      BGFXMoviePlayerFileOpener() {};
      virtual ~BGFXMoviePlayerFileOpener() {};
      virtual GFile* OpenFile(const char *purl);
};

//============================================================================
//============================================================================
class BGFxPlayerLog : public GFxLog
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
class BGFxMovieManager
{
   public: 

      BGFxMovieManager();
      virtual ~BGFxMovieManager();
      bool init(const FxPlayerSettings& settings, const BFixedString128& languageStr, long directory, const BFixedString128& fontFile, const BFixedString128& wordwrapMode);
      void deinit();

      GFxLoader* getLoader() { return &mLoader; };
      GRendererXbox360* getRenderer() const { return mRenderer.GetPtr(); };
      const FxPlayerSettings& getSettings() const { return mSettings; };
      BGFxPlayerLog* getLog() { return &mLog; }

   private:

      bool BGFxMovieManager::initFonts(const BFixedString128& languageStr, long directory, const BFixedString128& fontDefinitionFile);
      void loadFont(BXMLNode node);
      void deinitFonts();

      // Loaded movie data2
      GFxLoader              mLoader;
      GPtr<GFxRenderConfig>  mRenderConfig;
      GPtr<GFxRenderStats>   mRenderStats;   
      GPtr<GFxParseControl>  mParseControl;
      GPtr<BFlashTranslator> mTranslator;
      GPtr<GRendererXbox360> mRenderer;   
      FxPlayerSettings       mSettings;
      BGFXMoviePlayerFileOpener mFileOpener;
      GPtr<GFxFontLib>       mFontLib;
      GPtr<GFxFontMap>       mFontMap;
      GPtr<GFxMovieDef>      mFontLibraryMovie;
      BGFxPlayerLog          mLog;
};

//============================================================================
//============================================================================
class  BGFXMoviePlayer
{
private:
   typedef FxPlayerSettings::AAModeType AAModeType;   

   FxPlayerSettings    mSettings;
   GFxMovieInfo        mMovieInfo;
   GPtr<GFxMovieDef>   mpMovieDef;
   GPtr<GFxMovieView>  mpMovie;
   BGFxMovieManager*   mpManager;
      
   // Movie timing state
   float               mSpeedScale;         // Advance speed, def 1.0f
   SInt                mFrameCounter;       // Frames rendered, for FPS
   // Time ticks: always rely on a timer, for FPS
   UInt32              mTimeStartTicks;     // Ticks during the start of playback
   UInt32              mTimeTicks;          // Current ticks
   
   // Movie logical ticks: either timer or setting controlled
   UInt32              mMovieStartTicks;
   UInt32              mMovieLastTicks;
   UInt32              mMovieTicks;
   
   // Scale toggle, on by default
   bool                mScaleEnable;

   // This variable is set when the movie is paused in the player.
   bool                mPaused;
   
   bool                mFirstFrame;

   // Width, height during sizing
   int                 mWidth;
   int                 mHeight;
   int                 mX;
   int                 mY;
   
   // Updates the view size based on the mScaleEnable flag and window size.
   void            UpdateViewSize();
   void            Clear();

   void initResolution();
   void setDimension(int x, int y, int width, int height);
   
public:
   BGFXMoviePlayer();
   ~BGFXMoviePlayer();

   bool           Init(BGFxMovieManager* pManager);

   void           Deinit(void);
   
   bool           Tick(void);

   // Load a new movie from a file and initialize timing
   bool           LoadMovie(long dirID, const char *pfilename);
   
   void           UnloadMovie(void);

   // Helper used to convert key codes and route them to GFxPlayer
   void           KeyEvent(UInt key, bool down);
   
   void           Invoke(const char* method, const GFxValue* pArgs, int argCount);
   void           Invoke(const char* method, const char* fmt, const char* args);
   bool           SetVariable(const char* variablePath, const char* value);
};