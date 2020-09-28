//============================================================================
// gfxMoviePlayer.cpp
// Ensemble Studios (c) 2006
//============================================================================
#include "xgameRender.h"
#include "gfxMoviePlayer.h"
#include "flashXFSFile.h"
#include "string\bstring.h"
#include "..\compression\xmlreader.h"
#include "..\xgame\LocaleManager.h"
#include "..\xgame\gamedirectories.h"
#include "..\compression\xmlreader.h"
#include "render.h"
#include "BD3D.h"

#include "wordwrap.h"
#include "flashtranslator.h"

//============================================================================
//============================================================================
BGFxMovieManager::BGFxMovieManager() :
   mRenderConfig(NULL),
   mRenderStats(NULL),   
   mRenderer(NULL),
   mFontLib(NULL),
   mFontMap(NULL),
   mFontLibraryMovie(NULL)
{
}

//============================================================================
//============================================================================
BGFxMovieManager::~BGFxMovieManager()
{
}

//============================================================================
//============================================================================
bool BGFxMovieManager::init(const FxPlayerSettings& settings, const BFixedString128& languageStr, long directory, const BFixedString128& fontFile, const BFixedString128& wordWrapMode)
{
   mSettings = settings;
       
   //-- setup loader   
   mParseControl = *new GFxParseControl(); 
   mParseControl->SetParseFlags(mSettings.VerboseParse ? GFxParseControl::VerboseParse : 0); 
   mLoader.SetParseControl(mParseControl); 

   mLoader.SetFontPackParams(0);
   mLoader.GetFontCacheManager()->EnableDynamicCache(true);
   
   mLoader.SetFileOpener(&mFileOpener); 
   mLoader.SetLog(&mLog);
   
   mSettings.ScaleX = 1.0f;
   mSettings.ScaleY = 1.0f;
                           
   BDEBUG_ASSERT(mRenderer.GetPtr() == NULL);
   // Create renderer      
   mRenderer = *(GRendererXbox360::CreateReferenceRenderer());
   if (!mRenderer.GetPtr())
      return false;
   
   mRenderer->SetDependentVideoMode(BD3D::mpDev, &BD3D::mD3DPP, GRendererXbox360::VMConfig_NoSceneCalls, (HWND)0);
   BDEBUG_ASSERT(mRenderConfig.GetPtr() == NULL);
   mRenderConfig = *new GFxRenderConfig(mRenderer);

   float maxPixelError = 1.0f;

   UINT rendererFlags = 0;
   if (mSettings.AAMode == FxPlayerSettings::AAMode_EdgeAA)
      rendererFlags |= GFxRenderConfig::RF_EdgeAA;
   mRenderConfig->SetMaxCurvePixelError(maxPixelError);
   mRenderConfig->SetRenderFlags(rendererFlags);
   mLoader.SetRenderConfig(mRenderConfig);     

   mRenderStats = *new GFxRenderStats();
   mLoader.SetRenderStats(mRenderStats);

   // setup word wrapping
   uint wordWrapOption = WW_PROHIBITION;
   WordWrap_SetOption(wordWrapOption);

   uint translationType = GFxTranslator::WWT_Default; //GFxTranslator::WWT_Prohibition;   
   if (wordWrapMode.compare("default") == 0)
      translationType = GFxTranslator::WWT_Default;
   else if (wordWrapMode.compare("mgs") == 0)
      translationType = GFxTranslator::WWT_Custom;
   else if (wordWrapMode.compare("scaleform") == 0)
      translationType = GFxTranslator::WWT_Prohibition;   
   
   mTranslator = *new BFlashTranslator(translationType);
   mLoader.SetTranslator(mTranslator);

   if (!initFonts(languageStr, directory, fontFile))
      return false;

   return true;
}

//============================================================================
//============================================================================
void BGFxMovieManager::deinit()
{
   mLoader.UnpinAll();

   //CLM [11.18.08] added check for null pointer referencing 
   if(mFontLibraryMovie)
      mFontLibraryMovie->UnpinResource();

   deinitFonts();

   mRenderConfig = NULL;   
   mRenderStats = NULL;
         
   mLoader.SetRenderConfig((GFxRenderConfig*)NULL);     
   mLoader.SetRenderStats((GFxRenderStats*)NULL);   
   mLoader.SetTranslator((GFxTranslator*) NULL);

   mParseControl=NULL;
   mTranslator=NULL;
   mRenderer = NULL;   
}

//============================================================================
//============================================================================
bool BGFxMovieManager::initFonts(const BFixedString128& languageStr, long directory, const BFixedString128& fontDefinitionFile)
{
   BString language;
   language.set(languageStr);

   BString fontsFile;
   fontsFile.set(fontDefinitionFile);

   //-- Open file.
   BXMLReader reader;
   bool result = reader.load(directory, fontsFile.getPtr());
   //CLM [11.21.08] this shouldn't assert.
   if (!result)
   {
      trace("XML parsing error in FlashFonts.xml.  Flash fonts will default to English.");
      return false;
   }

   BXMLNode defaultNode;
   bool foundDefault=false;
   bool fontLoaded=false;

   //-- Grab the root node.
   BXMLNode rootNode(reader.getRootNode());

   //-- Run through all the child nodes and parse them.
   for(long nodeIndex = 0; nodeIndex < rootNode.getNumberChildren(); nodeIndex++)
   {
      BXMLNode node(rootNode.getChild(nodeIndex));

      //-- See if its an Source file name.
      if (node.getName().compare(B("Language"))==0)
      {
         BXMLAttribute attribSourceName;

         // do we have this attribute?
         if ( !node.getAttribute("name", &attribSourceName) )
            continue;

         // get the attribute value and see if it's our language
         BString temp;
         attribSourceName.getValue(temp);
         if (language.compare(temp) != 0)
         {
            if (temp.compare("Default") == 0)
            {
               foundDefault = true;
               defaultNode = node;
            }
            continue;
         }

         // this is our node.
         loadFont(node);
         fontLoaded=true;
      }
   }

   if (!fontLoaded && foundDefault)
      loadFont(defaultNode);

   return true;
}

//============================================================================
//============================================================================
void BGFxMovieManager::loadFont(BXMLNode node)
{
   // set up the font loader
   mFontLib = *new GFxFontLib();

   // set up the font map
   mFontMap = *new GFxFontMap();   

   mLoader.SetFontLib(mFontLib);   
   mLoader.SetFontMap(mFontMap);

   // Get all the <FontFile> nodes and all the <Map> nodes 
   for(long i= 0; i < node.getNumberChildren(); i++)
   {
      BXMLNode node2(node.getChild(i));
      if (node2.getName().compare(B("FontFile"))==0)
      {
         // Process the <FontFile> node
         BXMLAttribute fontFile;
         BString fontFilename;
         if (!node2.getAttribValueAsString("name", fontFilename))
         {
            BASSERT(0);
         }

         fontFilename.removeExtension();
         fontFilename.append(".swf");
         
         // load up the movie and add get the fonts from it
         mFontLibraryMovie=mLoader.CreateMovie(fontFilename.getPtr());
         if (mFontLibraryMovie)
            mFontLib->AddFontsFrom(mFontLibraryMovie, true);
      }
      else if (node2.getName().compare(B("FontMap"))==0)
      {
         // Process <Map>
         BString fontTitle;
         BString fontName;
         BString style;
         GFxFontMap::MapFontFlags fontFlags = GFxFontMap::MFF_Original;

         // get the font title
         if (!node2.getAttribValueAsString("fontTitle", fontTitle))
         {
            BASSERT(0);
            continue;
         }

         // get the font name
         if (!node2.getAttribValueAsString("fontName", fontName))
         {
            BASSERT(0);
            continue;
         }

         // get the font style
         if (node2.getAttribValueAsString("style", style))
         {
            if (style.compare("Normal")==0)
               fontFlags = GFxFontMap::MFF_Normal;
            else if (style.compare("Bold")==0)
               fontFlags = GFxFontMap::MFF_Bold;
            else if (style.compare("Italic")==0)
               fontFlags = GFxFontMap::MFF_Italic;
            else if (style.compare("BoldItalic")==0)
               fontFlags = GFxFontMap::MFF_BoldItalic;
         }

         // add the map
         bool bSuccess = mFontMap->MapFont(fontTitle, fontName, fontFlags);
         if (!bSuccess)
         {
            BASSERT(0);
            continue;
         }
      }
   }
}

//============================================================================
//============================================================================
void BGFxMovieManager::deinitFonts()
{
   mLoader.SetFontLib(NULL);
   mLoader.SetFontMap(NULL);

   mFontLib = NULL;
   mFontMap = NULL;      
   mFontLibraryMovie=NULL;
}

//============================================================================
//============================================================================
BGFXMoviePlayer::BGFXMoviePlayer()
{       
   Clear();
}

//============================================================================
//============================================================================
void BGFXMoviePlayer::Clear(void)
{
   // Scale toggle, on by default
   mScaleEnable         = 1;
   mPaused              = 0;

   // Clear timing 
   mSpeedScale          = 1.0f;
   mFrameCounter        = 0;
   mTimeStartTicks      = 0;
   mTimeTicks           = 0;

   mMovieStartTicks     = 0;
   mMovieLastTicks      = 0;
   mMovieTicks          = 0;

   mX = 0;
   mY = 0;
   mWidth = 0;
   mHeight = 0;
   
   mFirstFrame          = true;
}

//============================================================================
//============================================================================
BGFXMoviePlayer::~BGFXMoviePlayer()
{
}

//============================================================================
//============================================================================
bool BGFXMoviePlayer::Init(BGFxMovieManager* pManager)
{  
   BDEBUG_ASSERT(pManager);
   if (!pManager)
      return false;

   mpManager = pManager;   
   // It is important to initialize these sizes, in case OnSizeEnter gets called.

   mWidth  = BD3D::mD3DPP.BackBufferWidth;
   mHeight = BD3D::mD3DPP.BackBufferHeight;
   mX = (BD3D::mD3DPP.BackBufferWidth-mWidth)/2;
   mY = (BD3D::mD3DPP.BackBufferHeight-mWidth)/2;   
      
   mFirstFrame = true;         
   return true;
}


//============================================================================
//============================================================================
void BGFXMoviePlayer::UnloadMovie(void)
{
   mpMovieDef = NULL;
   mpMovie = NULL;                      
}

//============================================================================
//============================================================================
void BGFXMoviePlayer::Deinit(void)
{
   UnloadMovie();         
   Clear();
   mpManager = NULL;
}

//============================================================================
//============================================================================
bool BGFXMoviePlayer::Tick(void)
{
   if (!mpMovie)
      return false;
      
   BD3D::mpDev->Clear(0, NULL, D3DCLEAR_STENCIL|D3DCLEAR_ZBUFFER, 0, 1.0f, 0);

#ifndef BUILD_FINAL   
   static bool wireframe = false;
   if (wireframe)
      BD3D::mpDev->SetRenderState(D3DRS_FILLMODE, D3DFILL_WIREFRAME);
#endif      

   if (mFirstFrame)
   {
      mFirstFrame = false;
      
      mTimeStartTicks  = GetTickCount();

      mMovieStartTicks = (!mSettings.FastForward) ? mTimeStartTicks : 0;
      mMovieLastTicks  = mMovieStartTicks;
   }
         
   mTimeTicks = GetTickCount();

   if (!mSettings.FastForward)
      mMovieTicks = mTimeTicks;
   else // Simulate time.          
      mMovieTicks = mMovieLastTicks + (UInt32) (1000.0f / mMovieInfo.FPS);
      
   int     deltaTicks  = mMovieTicks - mMovieLastTicks;
   float   deltaT      = deltaTicks / 1000.f;
         

   mMovieLastTicks = mMovieTicks;

   // Potential out-of bounds range is not a problem here,
   // because it will be adjusted for inside of the player.
   if (mpMovie)
   {
      GViewport desc;
      desc.BufferWidth = BD3D::mD3DPP.BackBufferWidth;;
      desc.BufferHeight= BD3D::mD3DPP.BackBufferHeight;

      desc.Left = mX;
      desc.Top = mY;
      desc.Width = mWidth;
      desc.Height =mHeight;

      desc.Flags = GViewport::View_RenderTextureAlpha;
      mpMovie->SetViewport(desc);         
               
      const GColor bColor(0);
      mpMovie->SetBackgroundColor(bColor);      
      mpMovie->SetBackgroundAlpha(0.0f);      

      if (!mPaused)
         mpMovie->Advance(deltaT * mSpeedScale);
   }
   
   if (mpMovie)
      mpMovie->Display(); 

   mFrameCounter++;

#if 0
   // If we're reached the end of the movie, exit.
   if (!mSettings.DoLoop && mpMovie &&
      (mpMovie->GetCurrentFrame() + 1 == mpMovieDef->GetFrameCount()) )
   {
   
   }
#endif   

   BD3D::mpDev->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);
   
   //BD3D::mpDev->SetViewport(&gRenderDraw.getWorkerActiveRenderViewport().getViewport());

   return true;
}

//============================================================================
// Load a new movie from a file and initialize timing
//============================================================================
bool BGFXMoviePlayer::LoadMovie(long dirID, const char *pfilename)
{
   BDEBUG_ASSERT(mpManager);
   if (!mpManager)
      return false;

   // Try to load the new movie
   GFxMovieInfo        newMovieInfo;
   
   BString filename;
   gFileManager.constructQualifiedPath(dirID, BString(pfilename), filename);
   if (filename.findLeft("game:\\") == 0)
      filename.remove(0, 6);
      
   pfilename = filename.getPtr();

   GFxLoader* pLoader = mpManager->getLoader();
   if (!pLoader)
      return false;

   // Get info about the width & height of the movie.
   if (!pLoader->GetMovieInfo(pfilename, &newMovieInfo))
   {
      fprintf(stderr, "Error: failed to get info about %s\n", pfilename);
      return false;
   }
   // Load the actual new movie and crate instance.
   // Don't use library: this will ensure that the memory is released.
   mpMovieDef = *pLoader->CreateMovie(pfilename, GFxLoader::LoadAll);
   if (!mpMovieDef)
   {
      fprintf(stderr, "Error: failed to create a movie from '%s'\n", pfilename);
      return false;
   }
   mpMovie = *mpMovieDef->CreateInstance();    
   if (!mpMovie)
   {
      mpMovieDef = NULL;
      fprintf(stderr, "Error: failed to create movie instance\n");
      return false;
   }

   // If this succeeded, replace the old movie with the new one.
   memcpy(&mMovieInfo, &newMovieInfo, sizeof(GFxMovieInfo));
      
   // Copy short filename (i.e. after last '/'),
   SInt len = strlen(pfilename);
   for (SInt i=len; i>0; i--)
   {
      if (pfilename[i]=='/' || pfilename[i]=='\\')
      {
         pfilename = pfilename+i+1;
         break;
      }
   }

   mpMovieDef->SetLog(mpManager->getLog());
   mpMovie->SetLog(mpManager->getLog());
   
   // Disable pause.
   mPaused          = 0;

   // Init timing for the new piece.
   mFrameCounter    = 0;
   // Time ticks: always rely on a timer
   mTimeStartTicks  = GetTickCount();

   // Movie logical ticks: either timer or setting controlled
   mMovieStartTicks = (!mSettings.FastForward) ? mTimeStartTicks : 0;
   mMovieLastTicks  = mMovieStartTicks;
   mFirstFrame = true;
   
   // Update the view
   UpdateViewSize();
   mX = (BD3D::mD3DPP.BackBufferWidth-mWidth)/2;
   mY = (BD3D::mD3DPP.BackBufferHeight-mHeight)/2;

   mpMovie->SetViewScaleMode(GFxMovieView::SM_ShowAll);

   initResolution();

   return true;
}

//============================================================================
// Updates the view size based on the mScaleEnable flag and window size.
//============================================================================
void BGFXMoviePlayer::UpdateViewSize()
{
   if (mScaleEnable)
   {
      SInt width = GTL::gmax<SInt>(BD3D::mD3DPP.BackBufferWidth, 4);
      SInt height= GTL::gmax<SInt>(BD3D::mD3DPP.BackBufferHeight, 4);

      // Determine movie size and location based on the aspect ratio  
      float hw = (Float) mMovieInfo.Height / (Float) mMovieInfo.Width;
      if (width * hw > height)
      {
         // Use height as the basis for aspect ratio
         mWidth   = (SInt)((float) height / hw);
         mHeight  = height;
      }
      else
      {
         // Use width
         mWidth   = width;
         mHeight  = (SInt) (width * hw);
      }

      // Compute input scale
      mSettings.ScaleX = (Float) mWidth / (Float) mMovieInfo.Width;
      mSettings.ScaleY = (Float) mHeight / (Float) mMovieInfo.Height;

      static bool bRetainAspectRatio = false;
      if (!bRetainAspectRatio)
      {
         if (width<=mMovieInfo.Width)
            mWidth = width;
         if (height<=mMovieInfo.Height)
            mHeight = height;
      }

   }
   else
   {
      // No scaling, just center the image
      mWidth   = mMovieInfo.Width;
      mHeight  = mMovieInfo.Height;
      mSettings.ScaleX = mSettings.ScaleY = 1.0f;
   }
}

//============================================================================
// Helper used to convert key codes and route them to GFxPlayer
//============================================================================
void BGFXMoviePlayer::KeyEvent(UInt key, bool down)
{
   GFxKey::Code    c(GFxKey::VoidSymbol);

   /*
   if (key >= 'A' && key <= 'Z')
   {
   c = (GFxKey::Code) ((key - 'A') + GFxKey::A);
   }
   else if (key >= VK_F1 && key <= VK_F15)
   {
   c = (GFxKey::Code) ((key - VK_F1) + GFxKey::F1);
   }
   else if (key >= VK_NUMPAD0 && key <= VK_NUMPAD9)
   {
   c = (GFxKey::Code) ((key - VK_NUMPAD0) + GFxKey::KP_0);
   }
   else
   */
   {
      // many keys don't correlate, so just use a look-up table.
      struct
      {
         int         vk;
         GFxKey::Code    gs;
      } table[] =
      {
         { VK_RETURN,    GFxKey::Return },
         { VK_ESCAPE,    GFxKey::Escape },
         { VK_LEFT,      GFxKey::Left },
         { VK_UP,        GFxKey::Up },
         { VK_RIGHT,     GFxKey::Right },
         { VK_DOWN,      GFxKey::Down },

         // @@ TODO fill this out some more
         { 0, GFxKey::VoidSymbol }
      };

      for (int i = 0; table[i].vk != 0; i++)
      {
         if (key == (UInt)table[i].vk)
         {
            c = table[i].gs;
            break;
         }
      }
   }

   if (c != GFxKey::VoidSymbol)
   {
      if (mpMovie)
      {
         GFxKeyEvent event(down ? GFxEvent::KeyDown : GFxKeyEvent::KeyUp, c);
         mpMovie->HandleEvent(event);         
      }
   }
}

//============================================================================
//============================================================================
void BGFXMoviePlayer::Invoke(const char* method, const GFxValue* pArgs, int argCount)
{
   if (!mpMovie)
      return;

   SCOPEDSAMPLE(BGFXMoviePlayer_Invoke);
   
   mpMovie->Invoke(method, pArgs, argCount);
}

//============================================================================
//============================================================================
void BGFXMoviePlayer::Invoke(const char* method, const char* fmt, const char* value)
{
   if (!mpMovie)
      return;

   //const char* pResult = mpMovie->InvokeArgs(method, fmt, args);
   //va_list v = "focus";
   //mpMovie->Invoke("menu.btn_01.gotoandplay", "%s", v);
   SCOPEDSAMPLE(BGFXMoviePlayer_Invoke);
   
   mpMovie->Invoke(method, fmt, value); //"_root.menu.btn_01.gotoandplay", "%s", value);
}

//============================================================================
//============================================================================
bool BGFXMoviePlayer::SetVariable(const char* variablePath, const char* value)
{
   if (!mpMovie)
      return false;

   BOOL result = mpMovie->SetVariable(variablePath, value, GFxMovie::SV_Sticky);
   return (result > 0);
}

//============================================================================
//============================================================================
GFile* BGFXMoviePlayerFileOpener::OpenFile(const char* url)
{
   return new GXFSFile(0, url);
}

//============================================================================
//============================================================================
void BGFXMoviePlayer::initResolution()
{  
   float authoringDimensionWidth  = 1800.0f;
   float authoringDimensionHeight = 1300.0f;
   float authoringSafeAreaWidth   = 1400.0f;
   float authoringSafeAreaHeight  = 900.0f;   
   const float backBufferSafeAreaScalar = 0.9f;
      
   const float backBufferSafeAreaWidth = BD3D::mD3DPP.BackBufferWidth * backBufferSafeAreaScalar;
   const float backBufferSafeAreaHeight = BD3D::mD3DPP.BackBufferHeight * backBufferSafeAreaScalar;

   float scalar = 1.0f;
   BRender::BAspectRatioMode aspectRatioMode = (BRender::BAspectRatioMode) gRender.getAspectRatioMode();

   if ((aspectRatioMode==BRender::cAspectRatioMode16x9) || (authoringDimensionWidth == 1280))
      scalar = backBufferSafeAreaHeight / authoringSafeAreaHeight;
   else if (aspectRatioMode==BRender::cAspectRatioMode4x3)
      scalar = backBufferSafeAreaWidth / authoringSafeAreaWidth;

   int newWidth = Math::FloatToIntRound(scalar * authoringDimensionWidth);
   int newHeight = Math::FloatToIntRound(scalar * authoringDimensionHeight);
   int newX = Math::FloatToIntRound( 0.5f * (((float)BD3D::mD3DPP.BackBufferWidth) - newWidth));
   int newY = Math::FloatToIntRound( 0.5f * (((float)BD3D::mD3DPP.BackBufferHeight) - newHeight));

   //-- set the correct dimension of the movie
   setDimension(newX, newY, newWidth, newHeight);   
}

//============================================================================
//============================================================================
void BGFXMoviePlayer::setDimension(int x, int y, int width, int height)
{
   mX = x;
   mY = y;
   mWidth = width;
   mHeight = height;
}

