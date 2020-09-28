//============================================================================
//
// flashmoviedefinition.cpp
//
//============================================================================
#include "xgameRender.h"
#include "workdirsetup.h"
#include "renderThread.h"
#include "flashmoviedefinition.h"

#include "flashmanager.h"

//============================================================================
//============================================================================
BFlashMovieDefinition::BFlashMovieDefinition():
mpMovieDef(NULL),
mRefCount(0),
mAssetCategoryIndex(0),
mStatus(cStatusInvalid)
{
}

//============================================================================
//============================================================================
BFlashMovieDefinition::~BFlashMovieDefinition()
{
   unload();
}

//============================================================================
//============================================================================
bool BFlashMovieDefinition::init(const char* pFilename, uint category)
{
   ASSERT_RENDER_THREAD
   BString filename;
   gFileManager.constructQualifiedPath(cDirProduction, BString(pFilename), filename);
   if (filename.findLeft("game:\\") == 0)
      filename.remove(0, 6);

   mFilename = filename;
   mAssetCategoryIndex = category;

   GFxLoader* pLoader = gFlashManager.getLoader((BFlashAssetCategory)mAssetCategoryIndex);
   if (!pLoader)
      return false;
   
   // Get info about the width & height of the movie.
   if (!pLoader->GetMovieInfo(BStrConv::toA(mFilename), &mMovieInfo))
   {
      fprintf(stderr, "Error: failed to get info about %s\n", mFilename.getPtr());
      return false;
   }

   mStatus = cStatusInitialized;
   return true;
}

//============================================================================
//============================================================================
bool BFlashMovieDefinition::load()
{
   ASSERT_RENDER_THREAD
   // Try to load the new movie   
   GFxLoader* pLoader = gFlashManager.getLoader((BFlashAssetCategory)mAssetCategoryIndex);
   if (!pLoader)
      return false;

   BFlashPlayerLog* pLog = gFlashManager.getLog();
   if (!pLog)
      return false;
   
   // Load the actual new movie and crate instance.
   // Don't use library: this will ensure that the memory is released.
   uint loadConstants = GFxLoader::LoadAll;// GFxLoader::LoadImports; //GFxLoader::LoadAll;
   mpMovieDef = pLoader->CreateMovie(BStrConv::toA(mFilename), loadConstants);
   if (!mpMovieDef)
   {
      fprintf(stderr, "Error: failed to create a movie from '%s'\n", mFilename.getPtr());
      return false;
   }

   mpMovieDef->SetLog(pLog);

   mStatus = cStatusLoaded;
   return true;
}

//============================================================================
//============================================================================
void BFlashMovieDefinition::unload()
{  
   ASSERT_RENDER_THREAD
   if (mpMovieDef)
      mpMovieDef->Release();

   mpMovieDef = NULL;
   mStatus = cStatusInitialized;
}




