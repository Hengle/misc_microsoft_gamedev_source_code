//============================================================================
//
// flashmanager.cpp
// Copyright 2006 (c) Ensemble Studios
//
//============================================================================

#include "xgameRender.h"
#include "flashmanager.h"
#include "flashmoviedefinition.h"
#include "flashmovie.h"
#include "reloadManager.h"
#include "renderThread.h"
#include "renderDraw.h"
#include "BD3D.h"
#include "file.h"
#include "tiledAA.h"
#include "flashtexturemanager.h"
#include "flashXFSFile.h"
#include "xmlreader.h"
#include "GFxFontLib.h"
#include "configsgamerender.h"
#include "d3dtexturemanager.h"
#include "console.h"
#include "wordwrap.h"

#include "flashallocator.h"

#include <errno.h>

BFlashManager gFlashManager;

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashPlayerSettings::initSafeAreaParams(int viewportWidth, int viewportHeight, float titleSafePct)
{
   int lvw=viewportWidth;
   int lvh=viewportHeight;

   float fvw=(float)lvw;
   float fvh=(float)lvh;

   mfSafeX1=fvw*(1.0f-titleSafePct)*0.5f;
   mfSafeY1=fvh*(1.0f-titleSafePct)*0.5f;
   mfSafeX2=fvw-mfSafeX1;
   mfSafeY2=fvh-mfSafeY1;
   mfSafeWidth=fvw*titleSafePct;
   mfSafeHeight=fvh*titleSafePct;
   mfCenterX=fvw*0.5f;
   mfCenterY=fvh*0.5f;
   mfWidth=fvw;
   mfHeight=fvh;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BFlashManager::BFlashManager():
BEventReceiver(),
mSimEventHandle(cInvalidEventReceiverHandle),
mLoadSWFFiles(false)
{
   for (int i = 0; i < cFlashAssetCategoryTotal; i++)
      mpLoader[i]=NULL;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BFlashManager::~BFlashManager()
{
}

//----------------------------------------------------------------------------
//---------------------------------------------------------------------------o-
bool BFlashManager::init()
{
   ASSERT_MAIN_THREAD

   const int dataBlockSize = 128;
   mData.reserve(dataBlockSize);
   mData.clear();

   eventReceiverInit(cThreadIndexRender);

   // register as a SIM event receiver as well
   if (mSimEventHandle == cInvalidEventReceiverHandle)
      mSimEventHandle = gEventDispatcher.addClient(this, cThreadIndexSim);

   //-- register with the reload manager
   BReloadManager::BPathArray Paths;
   Paths.push_back("*.swf");
   gReloadManager.registerClient(Paths, BReloadManager::cFlagSynchronous|BReloadManager::cFlagSubDirs, getEventHandle());

   // Flash font definition file
   BSimString fontDefinitionFile;
   if (gConfig.get(cConfigFlashFontsFile, fontDefinitionFile))
      mFontDefinitionFile=fontDefinitionFile;
   else
      mFontDefinitionFile.set("FlashFonts.xml");


   //GPtr<GFxFileOpener> pFileOpener = *new GXFSFileOpener;
   

   // Create renderer      
   //if ((mpRenderer = GRendererXbox360::CreateRenderer()) != NULL)
   mpRenderer = BFlashRender::createRenderer();
   if (!mpRenderer)
      return false;

   mpRenderer->SetDependentVideoMode(BD3D::mpDev, &BD3D::mD3DPP, GRendererXbox360::VMConfig_NoSceneCalls, (HWND)0);
   mpRenderConfig = new GFxRenderConfig(mpRenderer);
   
   float maxPixelError = 1.0f;

   UINT rendererFlags = 0;
   if (mSettings.AAMode == BFlashPlayerSettings::AAMode_EdgeAA)
      rendererFlags |= GFxRenderConfig::RF_EdgeAA;
   
   if (gConfig.isDefined(cConfigFlashEnableDynamicTesselation))
      rendererFlags |= GFxRenderConfig::RF_Dynamic_Tessellation;

   mpRenderConfig->SetMaxCurvePixelError(maxPixelError);
   mpRenderConfig->SetRenderFlags(rendererFlags);
   
   mpRenderStats = new GFxRenderStats();

   mpParseControl = new GFxParseControl(); 
   mpParseControl->SetParseFlags(mSettings.VerboseParse ? GFxParseControl::VerboseParse : 0); 

   const float cFlashTitleSafePct = 0.85f;
   mSettings.initSafeAreaParams(BD3D::mD3DPP.BackBufferWidth, BD3D::mD3DPP.BackBufferHeight, cFlashTitleSafePct);

   uint wordWrapOption = WW_PROHIBITION;
   WordWrap_SetOption(wordWrapOption);

   uint translationType = GFxTranslator::WWT_Default; //GFxTranslator::WWT_Prohibition;

   BSimString wordWrapMode;   
   gConfig.get(cConfigFlashCustomWordWrappingMode, wordWrapMode);
   if (wordWrapMode.compare("default") == 0)
      translationType = GFxTranslator::WWT_Default;
   else if (wordWrapMode.compare("mgs") == 0)
      translationType = GFxTranslator::WWT_Custom;
   else if (wordWrapMode.compare("scaleform") == 0)
      translationType = GFxTranslator::WWT_Prohibition;   
   
   mpTranslator = new BFlashTranslator(translationType);  

   initFontCacheManager();
   
   initLoader(cFlashAssetCategoryCommon,  mpRenderer, mpRenderConfig, mpParseControl, mpRenderStats, &mLog, mpTranslator);   
   initLoader(cFlashAssetCategoryPreGame, mpRenderer, mpRenderConfig, mpParseControl, mpRenderStats, &mLog, mpTranslator);
   initLoader(cFlashAssetCategoryInGame,  mpRenderer, mpRenderConfig, mpParseControl, mpRenderStats, &mLog, mpTranslator);
         
   //-- initialize the texture manager;
   gEventDispatcher.send(cInvalidEventReceiverHandle, mEventHandle, cFlashInitTextureManager, 0, 0, NULL);

#ifndef BUILD_FINAL
   trace("Scaleform Version: %s", GFC_FX_VERSION_STRING);
#endif
   
   return true;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashManager::initFontCacheManager()
{
   mSharedFontCacheManager = *new GFxFontCacheManager();

   mFontCacheConfig.TextureWidth = 1024;
   mFontCacheConfig.TextureHeight = 1024;
   mFontCacheConfig.MaxNumTextures = 2;
   mFontCacheConfig.MaxSlotHeight = 48;
   mFontCacheConfig.SlotPadding = 2;
   mFontCacheConfig.TexUpdWidth = 256;
   mFontCacheConfig.TexUpdHeight = 512;
   mSharedFontCacheManager->SetTextureConfig(mFontCacheConfig);
   mSharedFontCacheManager->EnableDynamicCache(true);   
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashManager::initLoader(BFlashAssetCategory category, BFlashRender* pRenderer, GFxRenderConfig* pRenderConfig, GFxParseControl* pParseControl, GFxRenderStats* pRenderStats, GFxLog* pLog, BFlashTranslator* pTranslator)
{
   ASSERT_MAIN_THREAD
   BDEBUG_ASSERT(category < cFlashAssetCategoryTotal);

   mpLoader[category] = new GFxLoader();    
   mpLoader[category]->SetRenderConfig(pRenderConfig);     
   mpLoader[category]->SetFileOpener(this);   

   // Enable the dynamic font caching for version 2.0.
   mpLoader[category]->SetFontPackParams(0);
   mpLoader[category]->SetFontCacheManager(mSharedFontCacheManager);
   //mpLoader[category]->GetFontCacheManager()->EnableDynamicCache(true);

   mpLoader[category]->SetRenderStats(pRenderStats);
   mpLoader[category]->SetParseControl(pParseControl); 

   mpLoader[category]->SetLog(pLog);

   initCustomTextureLoaders(category, pRenderer);
   
   if (mpTranslator)
   {
      mpLoader[category]->SetTranslator(mpTranslator);
   }
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashManager::initCustomTextureLoaders(BFlashAssetCategory category, BFlashRender* pRenderer)
{
   //-- setup Custom Flash image loader
   uint textureCategoryBits = BD3DTextureManager::cScaleformCommon;
   switch (category)
   {
      case cFlashAssetCategoryCommon: textureCategoryBits = BD3DTextureManager::cScaleformCommon; break;
      case cFlashAssetCategoryPreGame: textureCategoryBits = BD3DTextureManager::cScaleformPreGame; break;
      case cFlashAssetCategoryInGame: textureCategoryBits = BD3DTextureManager::cScaleformInGame; break;
   }
   
   //-- setup DDX Texture loader / creator
   mpTextureCreator[category] = new BFlashTextureCreator(textureCategoryBits, false);
   mpLoader[category]->SetImageCreator(mpTextureCreator[category]);
   
   mpTextureLoader[category] = new BFlashTextureLoader(pRenderer, textureCategoryBits);
   mpLoader[category]->SetImageLoader(mpTextureLoader[category]);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashManager::deinit()
{
   ASSERT_MAIN_THREAD

   gEventDispatcher.send(cInvalidEventReceiverHandle, mEventHandle, cFlashManagerDeinit, 0, 0, NULL);
   
   gRenderThread.blockUntilGPUIdle();
        
   eventReceiverDeinit();

   if (mSimEventHandle!=cInvalidEventReceiverHandle)
   {
      gEventDispatcher.removeClientDeferred(mSimEventHandle, true);
      mSimEventHandle=cInvalidEventReceiverHandle;
   }   
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
GFxLoader* BFlashManager::getLoader(BFlashAssetCategory category)
{
   debugRangeCheck(category, cFlashAssetCategoryTotal);
   return mpLoader[category];
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BFlashManager::initFonts(const BFixedString<128>& languageStr, long directory)
{
   ASSERT_RENDER_THREAD

   BString language;
   language.set(languageStr);

   //-- Open file.
   BXMLReader reader;
   bool result = reader.load(directory, mFontDefinitionFile.getPtr());
   BASSERT(result);
   if (!result)
   {
      BFAIL("XML parsing error in FlashFonts.xml.  Flash fonts will default to English.");
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

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashManager::loadFont(BXMLNode node)
{
   ASSERT_RENDER_THREAD

   // set up the font loader
   mpFontLib = *new GFxFontLib;

   // set up the font map
   mpFontMap = *new GFxFontMap;;

   for (int i = 0; i < cFlashAssetCategoryTotal; i++)
   {
      mpLoader[i]->SetFontLib(mpFontLib);   
      mpLoader[i]->SetFontMap(mpFontMap.GetPtr());
   }

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
         fontFilename.append(BStrConv::toA(".gfx"));

         #ifndef BUILD_FINAL
         if (mLoadSWFFiles)
         {
            fontFilename.removeExtension();
            fontFilename.append(BStrConv::toA(".swf"));

            trace("Force Loading (.swf) file : %s", BStrConv::toA(fontFilename));
         }
         #endif

         // load up the movie and add get the fonts from it
         int dataIndex = -1;
         BFlashMovieDefinition* pFontMovieDef = getData(fontFilename, cFlashAssetCategoryCommon, &dataIndex);         
         BVERIFY(pFontMovieDef);         
         if (pFontMovieDef && dataIndex != -1)
         {
            trace("Flash Font Loaded: %s", fontFilename.getPtr());            
            if (pFontMovieDef && pFontMovieDef->mpMovieDef)
            {
               GPtr<GFxMovieDef> pFontDef = *pFontMovieDef->mpMovieDef;
               mpFontLib->AddFontsFrom(pFontDef, true);

               trace("Font Added to Font Lib: %s", fontFilename.getPtr());
            }
         }         
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
         mpFontMap->MapFont(fontTitle, fontName, fontFlags);
      }
   }

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashManager::update()
{
   ASSERT_RENDER_THREAD
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashManager::renderBegin()
{
   ASSERT_RENDER_THREAD

   SCOPEDSAMPLEID(Flash_Render, 0xFFFF0000);

   BD3D::mpDev->Clear(0, NULL, D3DCLEAR_STENCIL|D3DCLEAR_ZBUFFER, 0, 1.0f, 0);

   static bool wireframe = false;
   if (wireframe)
      BD3D::mpDev->SetRenderState(D3DRS_FILLMODE, D3DFILL_WIREFRAME);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashManager::render()
{
   ASSERT_RENDER_THREAD
   int count = mMovies.getSize();
   for (int i = 0; i < count; ++i)
   {
      BDEBUG_ASSERT(mMovies[i] != NULL);
      if (mMovies[i])
         mMovies[i]->render();
   }
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashManager::renderEnd()
{
   BD3D::mpDev->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);
   BD3D::mpDev->SetViewport(&gRenderDraw.getWorkerActiveRenderViewport().getViewport());

   ASSERT_RENDER_THREAD
}

//============================================================================
//============================================================================
int BFlashManager::find(const BString& filename)
{
   ASSERT_RENDER_THREAD
   int count = mData.getSize();
   for (int i = 0; i < count; ++i)
   {
      BDEBUG_ASSERT(mData[i]!=NULL);
      if (mData[i]->mFilename.compare(filename)==0)
      {
         return i;
      }
   }
   return -1;
}

//============================================================================
// BFlashMovieDefinition* BFlashManager::getData()
//============================================================================
BFlashMovieDefinition* BFlashManager::getData(const BString& filename, BFlashAssetCategory category, int* pIndex)
{
   ASSERT_RENDER_THREAD   
   BString filepath = filename;
#ifndef BUILD_FINAL
   if (mLoadSWFFiles)
   {
      filepath.removeExtension();
      filepath.append(BStrConv::toA(".swf"));

      trace("Force Loading (.swf) file : %s", BStrConv::toA(filepath));
   }
#endif

   int dataIndex = find(filepath);
   if (dataIndex != -1)
   {      
      BFlashMovieDefinition* pMovieDefinition = getData(dataIndex);      
      if (pMovieDefinition && pMovieDefinition->mStatus == BFlashMovieDefinition::cStatusInitialized)
      {
         if (!pMovieDefinition->load())
         {
            if (pIndex)
               *pIndex = -1;
            return NULL;
         }

         if (pIndex)
            *pIndex = dataIndex;

         return pMovieDefinition;
      }               
   }

   //- if we got here then we need to load it.
   int index = -1;
   if (!loadMovieDefinition(filepath, category, &index))
   {
      if (pIndex)
         *pIndex = -1;

      return NULL;
   }

   if (pIndex)
      *pIndex = index;
   
   return getData(index);
}

//=================================================================`===========
// const BFlashMovieDefinition& BFlashManager::getData();
//============================================================================
BFlashMovieDefinition* BFlashManager::getData(int index)
{
   ASSERT_RENDER_THREAD

   debugRangeCheck(index, mData.getSize());
   //return mData[index];
   return mData[index];
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BFlashManager::loadMovieDefinition(const BString& fileName, BFlashAssetCategory category, int* pIndex)
{
   ASSERT_RENDER_THREAD
   if (pIndex)
      *pIndex = -1;
      
   if (fileName.length() == 0)
      return false;

   BFlashMovieDefinition* pData = new BFlashMovieDefinition();

#ifndef BUILD_FINAL
   BRenderGAllocatorHeapStats curStats;
   curStats = gRenderGAllocator.getHeapStats();   
#endif
   bool ok = pData->init(fileName, category);
   if (!ok)
   {
      delete pData;
      return false;
   }

   ok = pData->load();
   if (!ok)
   {
      delete pData;
      return false;
   }

#ifndef BUILD_FINAL
   BRenderGAllocatorHeapStats curStats2;
   curStats2 = gRenderGAllocator.getHeapStats();
   curStats2.createDelta(curStats);      
   
   BFlashAssetAllocStats protoStat;
   protoStat.mAllocationSize = (uint) curStats2.mTotalAllocationSize;
   protoStat.mFilename = fileName;
   protoStat.mCategory = category;
   mFlashProtoAssetStats.add(protoStat);
               
   /*
   gConsole.output(cChannelUI, "Loaded Flash Movie : %s", fileName.getPtr());
   gConsole.output(cChannelUI, "  Total Allocation : %I64i", curStats2.mTotalAllocationSize);
   gConsole.output(cChannelUI, "  Total Allocations: %i", curStats2.mTotalAllocations);
   gConsole.output(cChannelUI, "  Total News       : %i", curStats2.mTotalNews);
   */
#endif
   
   int newIndex = mData.getSize();
   mData.pushBack(&pData, 1);
   if (pIndex)
      *pIndex = newIndex;

   return true;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashManager::releaseMovie(BFlashMovie* pMovie)
{
   ASSERT_RENDER_THREAD

   if (!pMovie)
      return;

   int count = mMovies.getSize();
   BFlashMovie* pBack = NULL;
   for (int i = 0; i < count; ++i)
   {
      if (mMovies[i] == pMovie)
      {
         pBack = mMovies.back();

         if (pBack != mMovies[i])
            mMovies[i] = pBack;

         mMovies.popBack();

         //-- decrement our ref count of the data
         BFlashMovieDefinition* pData = getData(pMovie->mDataIndex);
         BASSERT(pData!=NULL);
         if (pData)
         {
            pData->decRef();
            if (pData->getRefCount() == 0)
               pData->unload();
         }
         
         pMovie->mDataIndex = -1;

         pMovie->deinit();         
         delete pMovie;
         
         return;
      }
   }    
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BFlashMovie* BFlashManager::createMovie(int index, bool bRenderToTexture)
{
   SCOPEDSAMPLE(BFlashManager_createMovie); 
   ASSERT_RENDER_THREAD

   BFlashMovieDefinition* pData = getData(index);
   if (!pData)
      return NULL;

   #ifndef BUILD_FINAL
   trace("Creating Instance of Flash Movie: %s", pData->mFilename.getPtr());
   #endif
   
   //-- create a new Flash Movie 
   BFlashMovie* pNewFlashMovie = new BFlashMovie();
   BDEBUG_ASSERT(pNewFlashMovie != NULL);

   #ifndef BUILD_FINAL
   BRenderGAllocatorHeapStats curStats;
   curStats = gRenderGAllocator.getHeapStats();
   #endif

   if (!pNewFlashMovie->init(index, pData, bRenderToTexture))
   {
      pNewFlashMovie->deinit();
      delete pNewFlashMovie;
      return NULL;
   }

   #ifndef BUILD_FINAL
   BRenderGAllocatorHeapStats curStats2;
   curStats2 = gRenderGAllocator.getHeapStats();
   curStats2.createDelta(curStats);      
      
   BFlashAssetAllocStats protoStat;
   protoStat.mAllocationSize = (uint) curStats2.mTotalAllocationSize;
   protoStat.mFilename = pData->mFilename;
   protoStat.mCategory = pData->mAssetCategoryIndex;
   mFlashInstanceAssetStats.add(protoStat);
   
   gConsole.output(cChannelUI, "Created Instance of Flash Movie : %s", BStrConv::toA(pData->mFilename.getPtr()));
   gConsole.output(cChannelUI, "  Total Allocation : %I64i", curStats2.mTotalAllocationSize);
   gConsole.output(cChannelUI, "  Total Allocations: %i", curStats2.mTotalAllocations);
   gConsole.output(cChannelUI, "  Total News       : %i", curStats2.mTotalNews);
   #endif

   //-- increment our ref count
   pData->addRef();

   mMovies.add(pNewFlashMovie);
   return pNewFlashMovie;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BFlashManager::receiveEvent(const BEvent& event, BThreadIndex threadIndex)
{
   switch(threadIndex)
   {
      case cThreadIndexRender:
      {
         if (event.mEventClass == cEventClassReloadNotify)
         {
//-- FIXING PREFIX BUG ID 6446
            const BReloadManager::BNotificationPayload* pPayload = static_cast<BReloadManager::BNotificationPayload*>(event.mpPayload);
//--
#ifndef BUILD_FINAL
            trace("%s\n", pPayload->mPath.getPtr());
#endif
            gConsoleOutput.status("Reloading file: %s", pPayload->mPath.getPtr());
            
            BString filepath(pPayload->mPath); 
            bool bSuccess = false;
            int count = 0;
            int cMaxRetries = 40;
            while (!bSuccess && count < cMaxRetries)
            {
               bSuccess = reloadMovie(filepath);
               count++;
               if (!bSuccess)
               {
                  #ifndef BUILD_FINAL
                  trace("Failed to Reload file: %s  Trying again!!  Try #%u", BStrConv::toA(filepath), count);
                  #endif
                  Sleep(250);
               }
            }

            if (bSuccess)
               trace("File Reload succeeded: %s (%u Retries)", BStrConv::toA(filepath), count);            

            return bSuccess;
         }
         else if (event.mEventClass == cEventClassClientRemove)
         {
         }
         else if (event.mEventClass == cFlashInitTextureManager)
         {
            gFlashTextureManager.init();
         }
         else if (event.mEventClass == cFlashManagerDeinit)
         {
            gFlashTextureManager.deInit();
                                                                        
            if (mpRenderConfig)
            {
               mpRenderConfig->Release();
               mpRenderConfig = NULL;
            }

            if (mpRenderStats)
            {  
               mpRenderStats->Release();
               mpRenderStats = NULL;
            }

            if (mpParseControl)
            {
               mpParseControl->Release();
               mpParseControl=NULL;
            }

            //-- release the font cache manager -- setting it to NULL should make the GPtr release the object
            mSharedFontCacheManager = NULL;

            for (int i = 0; i < cFlashAssetCategoryTotal; i++)
            {
               if (mpLoader[i])
               {
                  mpLoader[i]->SetRenderConfig((GFxRenderConfig*)NULL);
                  mpLoader[i]->SetRenderStats(NULL);
                  mpLoader[i]->SetImageCreator(NULL);
                  delete mpLoader[i];
                  mpLoader[i] = NULL;
               }

               if (mpTextureLoader[i])
               {
                  mpTextureLoader[i]->Release();
                  mpTextureLoader[i]=NULL;
               }

               if (mpTextureCreator[i])
               {
                  mpTextureCreator[i]->Release();
                  mpTextureCreator[i] = NULL;
               }
            }
            
            if (mpRenderer)
            {
               mpRenderer->Release();
               mpRenderer = NULL;
            }
            
            for (uint i = 0; i < mMovies.getSize(); i++)
            {
               BASSERT(mMovies[i] != NULL);
               if (mMovies[i])
               {
                  mMovies[i]->deinit();
                  delete mMovies[i];
                  mMovies[i] = NULL;
               }
            }
            mMovies.clear();

            for (uint i = 0; i < mData.getSize(); i++)
               delete mData[i];
            mData.clear();

            if (mpTranslator)
            {
               mpTranslator->Release();
               mpTranslator = NULL;
            }
         }         
         break;
      }

      case cThreadIndexSim:
         {
#ifndef BUILD_FINAL         
            if (event.mEventClass == cFlashUpdateStats)
            {
//-- FIXING PREFIX BUG ID 6447
               const BFlashManager::BStats* pPayload = static_cast<BFlashManager::BStats*> (event.mpPayload);
//--
               ASSERT_MAIN_THREAD
               mSimStats = *pPayload;
            }
#endif            
            break;
         }
   }

   return false;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#ifndef BUILD_FINAL
void BFlashManager::refreshStats()
{
   ASSERT_RENDER_THREAD

   // Okay for this to use the primary heap.
   BFlashManager::BStats* pPayload = new BFlashManager::BStats(mWorkerStats);

   //-- can clear our worker stats now
   mWorkerStats.clear();   

   gEventDispatcher.send(cInvalidEventReceiverHandle, mSimEventHandle, cFlashUpdateStats, 0, 0, pPayload);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashManager::clearInstanceMemoryStats(uint category) 
{ 
   ASSERT_RENDER_THREAD;
   int numInstances = mFlashInstanceAssetStats.getNumber();
   for (int i = 0; i < numInstances; ++i)
   {
      if (mFlashInstanceAssetStats[i].mCategory == category)
      {
         if (i < (numInstances-1))
            mFlashInstanceAssetStats[i] = mFlashInstanceAssetStats[numInstances - 1];

         numInstances--;
         i--;         
      }
   }
   mFlashInstanceAssetStats.resize(numInstances);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashManager::clearProtoMemoryStats(uint category)
{
   int numProto = mFlashProtoAssetStats.getNumber();
   for (int j = 0; j < numProto; ++j)
   {
      if (mFlashProtoAssetStats[j].mCategory == category)
      {
         if (j < (numProto-1))
            mFlashProtoAssetStats[j] = mFlashProtoAssetStats[numProto - 1];

         numProto--;
         j--;         
      }
   }
   mFlashProtoAssetStats.resize(numProto);
}
#endif   

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BFlashMovieDefinition* BFlashManager::reloadData(int index)
{
   ASSERT_RENDER_THREAD
   if (mData[index] == NULL)
      return NULL;

   mData[index]->unload();

   if (!mData[index]->load())
      return NULL;

   return mData[index];   
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BFlashManager::reloadMovie(const BString& path)
{
   ASSERT_RENDER_THREAD
   int dataIndex = find(path);
   if (dataIndex == -1)
      return false;

   BFlashMovieDefinition* pData = reloadData(dataIndex);
   if (!pData)
      return false;

   int effectCount = mMovies.getSize();
   for (int i = 0; i < effectCount;  i++)
   {
      if (mMovies[i]->mDataIndex == dataIndex)
      {
         mMovies[i]->deinit();

         if (!mMovies[i]->init(dataIndex, pData, mMovies[i]->getFlag(BFlashMovie::eFlagRenderToTexture)))
            return false;

         //-- notify all event receivers that we just reloaded the movie
         int count = gFlashManager.getNumberEventReceivers();
         for (int i = 0; i < count; ++i)
         {
//-- FIXING PREFIX BUG ID 6449
            const BFlashEventReceiver* pReceiver = gFlashManager.getEventReceiver(i);
//--
            BDEBUG_ASSERT(pReceiver!=NULL);
            BASSERT(pReceiver!=NULL);
            BASSERT(pReceiver->mpMovie!=NULL);
            BASSERT(pReceiver->mpMovie->isInitialized());
            BASSERT(pReceiver->mpMovie->mpMovie.GetPtr()!=NULL);

            if (pReceiver==NULL)
               continue;

            if (pReceiver->mpMovie==NULL)
               continue;

            if (!pReceiver->mpMovie->isInitialized())
               continue;

            if (pReceiver->mpMovie == mMovies[i])
            {
               BFileReloadCommandPayload* pPayload = new BFileReloadCommandPayload(path);
               gEventDispatcher.send(cInvalidEventReceiverHandle, pReceiver->mHandle, cFlashEventFileReload, 0, 0, pPayload);
            }
         }         
      }
   }

   return true;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashManager::registerFSCallbackReceiver(BFlashMovie* pMovie, BEventReceiverHandle handle)
{
   //-- check if this is already in the list if so then don't add it.
   int count = mEventReceivers.getNumber();
   for (int i = 0; i < count; i++)
   {
      if ((mEventReceivers[i].mHandle == handle) && 
          (mEventReceivers[i].mpMovie == pMovie))
      {         
         return;
      }
   }

   BFlashEventReceiver event;
   event.mpMovie = pMovie;
   event.mHandle = handle;
   mEventReceivers.add(event);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashManager::unregisterFSCallbackReiver(BFlashMovie* pMovie, BEventReceiverHandle handle)
{
   int count = mEventReceivers.getNumber();
   for (int i = 0; i < count; i++)
   {
      if ((mEventReceivers[i].mHandle == handle) && 
          (mEventReceivers[i].mpMovie == pMovie))
      {
         mEventReceivers[i] = mEventReceivers.back();
         mEventReceivers.popBack();
         return;
      }
   }
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BFlashEventReceiver* BFlashManager::getEventReceiver(int index)
{
   debugRangeCheck(index, mEventReceivers.getNumber());
   return &mEventReceivers[index];
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
GFile* BFlashManager::OpenFile(const char *purl)
{
#if 0
   // fixme - swap out font files here
   BString f;
   f.set(purl);
   if (f.contains("Font_glyphs.swf"))
      f.set("art\\ui\\flash\\loc\\english\\Font_glyphs.swf");
   else if (f.contains("HWFontHandel.swf"))
      f.set("art\\ui\\flash\\loc\\english\\HWFontHandel.swf");
   else if (f.contains("HWFontConduit360.swf"))
      f.set("art\\ui\\flash\\loc\\english\\HWFontConduit360.swf");
  
   return new GXFSFile(0, f.getPtr());
#else
   return new GXFSFile(0, purl);
#endif

};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int BFlashManager::findCallBackCommand(const char* command)
{

   return -1;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int BFlashManager::findMovieParent(const GFxMovieView* pMovie)
{
   int count = mMovies.getNumber();
   for (int i = 0; i < count; ++i)
   {
      if (mMovies[i])
      {
         if (mMovies[i]->isInitialized())
         {
            if ((mMovies[i]->mpMovie) && (mMovies[i]->mpMovie == pMovie))
               return i;
         }
      }
   }
   return -1;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashManager::Callback(GFxMovieView* pmovie, const char* command, const char* args)
{
   if (!pmovie)
      return;

   //trace("FsCallback: %s %s\n", command, args);

   int callBackID  = gFlashManager.findCallBackCommand(command);
   callBackID;
   int sceneParent = gFlashManager.findMovieParent(pmovie);

   // found parent
   if (sceneParent != -1)
   {
      int count = gFlashManager.getNumberEventReceivers();
      for (int i = 0; i < count; ++i)
      {
//-- FIXING PREFIX BUG ID 6451
         const BFlashEventReceiver* pReceiver = gFlashManager.getEventReceiver(i);
//--         
         if (pReceiver==NULL)
            continue;
         if (pReceiver->mpMovie==NULL)
            continue;
         if (!pReceiver->mpMovie->isInitialized())
            continue;
         if (pReceiver->mpMovie->mpMovie.GetPtr()==NULL)
            continue;
         if (pReceiver->mHandle == cInvalidEventReceiverHandle)
            continue;

         if (pReceiver->mpMovie->mpMovie == pmovie)
         {
            BFSCommandPayload* pPayload = new BFSCommandPayload(command, args);
            gEventDispatcher.send(cInvalidEventReceiverHandle, pReceiver->mHandle, cFlashEventFSCommand, 0, 0, pPayload);
         }
      }
   }

   /*
   GFxLog *plog = pmovie->GetLog();
   if (plog)
   {            
      plog->LogMessage("FsCallback: '");
      plog->LogMessage(command);
      plog->LogMessage("' '");
      plog->LogMessage(args);
      plog->LogMessage("'\n");
   }
   */
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashManager::setEnableBatching(bool bEnable)
{
   if (!mpRenderer)
      return;

   mpRenderer->setEnableBatching(bEnable);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashManager::setEnableSWFLoading(bool bEnable)
{
   mLoadSWFFiles = bEnable;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashManager::setEnableWireframe(bool bEnable)
{
   if (!mpRenderer)
      return;

   mpRenderer->setEnableWireframe(bEnable);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashManager::unloadPregameUITextures()
{   
   ASSERT_RENDER_THREAD
   gFlashTextureManager.unloadPreGameUITextures();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashManager::loadPregameUITextures()
{
   ASSERT_RENDER_THREAD
   gFlashTextureManager.loadPreGameUITextures();
}
