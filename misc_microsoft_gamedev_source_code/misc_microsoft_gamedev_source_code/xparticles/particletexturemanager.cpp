//============================================================================
// File: particleTextureManager.h
//============================================================================

#include "xparticlescommon.h"
#include "particletexturemanager.h"
#include "particlesystemmanager.h"
#include "consoleOutput.h"

//-- files
#include "file.h"
#include "particleworkdirsetup.h"

//-- texture
#include "renderThread.h"
#include "D3DTextureLoader.h"
#include "asyncFileManager.h"
#include "color.h"
#include "math\halfFloat.h"

BParticleTextureManager gPSTextureManager;

const uint cMaxColorProgressionScanLines = 2048;
const uint cMaxScaleProgressionScanLines = 2048;
const uint cMaxIntensityProgressionScanLines = 2048;

//----------------------------------------------------------------------------
// BParticleProgressionTextureData::BParticleProgressionTextureData
//----------------------------------------------------------------------------
BParticleProgressionTextureData::BParticleProgressionTextureData():
   mWidth(0),
   mHeight(0),
   mUseCount(0),
   mpTexture(NULL),
   mpData(NULL),
   mPitch(0),
   mNextAvailableScanline(-1)
{
}

//----------------------------------------------------------------------------
// BParticleProgressionTextureData::~BParticleProgressionTextureData
//----------------------------------------------------------------------------
BParticleProgressionTextureData::~BParticleProgressionTextureData()
{
}

//----------------------------------------------------------------------------
// BParticleProgressionTextureData::clear
//----------------------------------------------------------------------------
void BParticleProgressionTextureData::clear()
{
   mWidth=0;
   mHeight=0;
   mUseCount=0;   
   mPitch=0;
   mpData = NULL;
   mNextAvailableScanline=-1;
   if (mpTexture)
   {
      mpTexture->Release();
      mpTexture = NULL;
   }      
}

//----------------------------------------------------------------------------
// BParticleRandomTextureData::BParticleRandomTextureData
//----------------------------------------------------------------------------
BParticleRandomTextureData::BParticleRandomTextureData():
   mWidth(0),
   mHeight(0),
   mpTexture(NULL),
   mpData(NULL),
   mPitch(0)
{
}

//----------------------------------------------------------------------------
// BParticleRandomTextureData::~BParticleRandomTextureData
//----------------------------------------------------------------------------
BParticleRandomTextureData::~BParticleRandomTextureData()
{
}

//----------------------------------------------------------------------------
// BParticleRandomTextureData::clear
//----------------------------------------------------------------------------
void BParticleRandomTextureData::clear()
{
   mWidth=0;
   mHeight=0;   
   mPitch=0;
   mpData = NULL;   
   if (mpTexture)
   {
      mpTexture->Release();
      mpTexture = NULL;
   }      
}

//----------------------------------------------------------------------------
// BParticleTextureArrayManager::BParticleTextureArrayManager
//----------------------------------------------------------------------------
BParticleTextureArrayManager::BParticleTextureArrayManager(uint maxTextures) :
   mMaxTextures((ushort)maxTextures),
   mWidth(0),
   mHeight(0)
{
}

//----------------------------------------------------------------------------
// BParticleTextureArrayManager::~BParticleTextureArrayManager
//----------------------------------------------------------------------------
BParticleTextureArrayManager::~BParticleTextureArrayManager()
{
}

//----------------------------------------------------------------------------
// BParticleTextureArrayManager::clear
//----------------------------------------------------------------------------
void BParticleTextureArrayManager::clear(void)
{
   mTextureLoader.clear();
   mTextureFilenames.clear();
#ifdef ENABLE_RELOAD_MANAGER
   mFileWatcher.clear(false);
#endif
   mWidth = 0;
   mHeight = 0;
}

//----------------------------------------------------------------------------
// BParticleTextureArrayManager::tick
//----------------------------------------------------------------------------
void BParticleTextureArrayManager::tick(void)
{
#ifdef ENABLE_RELOAD_MANAGER
   if (mFileWatcher.getAreAnyDirty())
      reloadTextures();
#endif      
}

//----------------------------------------------------------------------------
// BParticleTextureArrayManager::reload
//----------------------------------------------------------------------------
void BParticleTextureArrayManager::reloadTextures(void)
{
   for (uint i = 0; i < mTextureFilenames.getSize(); i++)
   {
      const BString& filename = mTextureFilenames[i];

      BFile file;  

      if (!file.openReadOnly(cDirParticlesRoot, filename))
      {
         gConsoleOutput.error("BParticleTextureArrayManager::reloadTextures: Unable to load %s\n", filename.getPtr());
         continue;
      }

      unsigned long fileSize;
      if (!file.getSize(fileSize))
      {
         gConsoleOutput.error("BParticleTextureArrayManager::reloadTextures: Unable to read %s\n", filename.getPtr());
         continue;
      }

      BDynamicParticleArray<uchar> data(fileSize);
      if (!file.read(data.getPtr(), fileSize))
      {
         gConsoleOutput.error("BParticleTextureArrayManager::reloadTextures: Unable to read %s\n", filename.getPtr());
         continue;
      }

      BD3DTextureLoader::BCreateParams textureCreateParams;
      textureCreateParams.mArraySize  = (uchar)mMaxTextures;
      textureCreateParams.mArrayIndex = (uchar)i;
      textureCreateParams.mLongTermAllocation = true;
      textureCreateParams.mManager = "Particles";
      textureCreateParams.mName = filename;

      if (!mTextureLoader.createFromDDXFileInMemory(data.getPtr(), fileSize, textureCreateParams))
      {
         gConsoleOutput.error("BParticleTextureArrayManager::reloadTextures: Failed creating DDX texture from file %s. It probably didn't have the same res or format as the other textures in this set!\n", filename.getPtr());
      }

      gConsoleOutput.resource("BParticleTextureArrayManager::reloadTextures: Loaded Texture %s, Resolution: %ux%u, Fmt: %s, Array: 0x%08X, ArrayIndex: %u\n", filename.getPtr(), mTextureLoader.getWidth(), mTextureLoader.getHeight(), getDDXDataFormatString(mTextureLoader.getDDXDataFormat()), this, textureCreateParams.mArrayIndex);
   }
}

//----------------------------------------------------------------------------
// BParticleTextureArrayManager::findTexture
//----------------------------------------------------------------------------
int BParticleTextureArrayManager::findTexture(const BString& filename) const
{
   for (uint i = 0; i < mTextureFilenames.getSize(); i++)
      if (filename == mTextureFilenames[i])
         return i;
   
   return cInvalidIndex;
}

//----------------------------------------------------------------------------
// BParticleTextureArrayManager::canLoadTextures
//----------------------------------------------------------------------------
bool BParticleTextureArrayManager::canLoadTextures(const BStringArray& filenames, uint* pNumMatches) const
{
   if (pNumMatches) *pNumMatches = 0;
      
   if (filenames.getSize() > mMaxTextures)
      return false;
      
   if (!getNumTextures())
      return true;

   uint numFound = 0, numNotFound = 0;
         
   for (uint i = 0; i < filenames.getSize(); i++)
   {
      int texIndex = findTexture(filenames[i]);
      if (texIndex == cInvalidIndex)
         numNotFound++;
      else
         numFound++;
   }
   
   if (pNumMatches) *pNumMatches = numFound;
   
   if (numFound == filenames.getSize())
      return true;
   
   if (!numFound)
      return false;
   
   const uint maxAdditionalTextures = mMaxTextures - getNumTextures();
   if (numNotFound > maxAdditionalTextures)
      return false;
   
   return true;
}

//----------------------------------------------------------------------------
// BParticleTextureArrayManager::loadTextures
//----------------------------------------------------------------------------
bool BParticleTextureArrayManager::loadTextures(const BStringArray& filenames, BTextureArraySlotIndices& textureIndices)
{
   SCOPEDSAMPLE(BParticleTextureArrayManager_loadTextures)
   textureIndices.resize(filenames.getSize());
   textureIndices.setAll(-1);
   
   for (uint i = 0; i < filenames.getSize(); i++)
   {
      const BString& filename = filenames[i];
      
      int texIndex = findTexture(filename);
      if (texIndex != cInvalidIndex)
      {
         textureIndices[i] = (short)texIndex;
         continue;
      }
      
      if (getNumTextures() == mMaxTextures)
         return false;
                     
      BFile file;  
      
      if (!file.openReadOnly(cDirParticlesRoot, filename))
      {
         gConsoleOutput.error("BParticleTextureArrayManager::loadTextures: Unable to load %s\n", filename.getPtr());
         // Hopefully another texture will load successfully.
         textureIndices[i] = 0;
         continue;
      }

      unsigned long fileSize;
      if (!file.getSize(fileSize))
      {
         gConsoleOutput.error("BParticleTextureArrayManager::loadTextures: Unable to read %s\n", filename.getPtr());
         textureIndices[i] = 0;
         continue;
      }

      BDynamicParticleArray<uchar> data(fileSize);
      if (!file.read(data.getPtr(), fileSize))
      {
         gConsoleOutput.error("BParticleTextureArrayManager::loadTextures: Unable to read %s\n", filename.getPtr());
         textureIndices[i] = 0;
         continue;
      }

      BD3DTextureLoader::BCreateParams textureCreateParams;
      textureCreateParams.mArraySize  = (uchar)mMaxTextures;
      textureCreateParams.mArrayIndex = (uchar)mTextureFilenames.getSize();
      textureCreateParams.mLongTermAllocation = true;
      textureCreateParams.mManager = "Particles";
      textureCreateParams.mName = filename;
      
      if (!mTextureLoader.createFromDDXFileInMemory(data.getPtr(), fileSize, textureCreateParams))
      {
         gConsoleOutput.error("BParticleTextureArrayManager::loadTextures: Failed creating DDX texture from file %s. It probably didn't have the same res or format as the other textures in this set!\n", filename.getPtr());
         if (mTextureFilenames.getSize())
         {
            textureIndices[i] = (short)(mTextureFilenames.getSize() - 1);
            continue;
         }
         else
         {
            // Hopefully another texture will load successfully.
            textureIndices[i] = 0;
            continue;    
         }
      }
      
      gConsoleOutput.resource("BParticleTextureArrayManager::loadTextures: Loaded Texture %s, Resolution: %ux%u, Fmt: %s, Array: 0x%08X, ArrayIndex: %u\n", filename.getPtr(), mTextureLoader.getWidth(), mTextureLoader.getHeight(), getDDXDataFormatString(mTextureLoader.getDDXDataFormat()), this, textureCreateParams.mArrayIndex);
      textureIndices[i] = (short)mTextureFilenames.getSize();   
            
      if (!mTextureFilenames.getSize())
      {
         mWidth = (ushort)mTextureLoader.getWidth();
         mHeight = (ushort)mTextureLoader.getHeight();
      }
                  
      mTextureFilenames.pushBack(filename);
#ifdef ENABLE_RELOAD_MANAGER
      mFileWatcher.add(cDirParticlesRoot, filename);
#endif
   }
   
   return !mTextureFilenames.empty();
}

#include "DDXUtils.h"

//----------------------------------------------------------------------------
// getDDXTextureDesc
// This sucks - it causes us to load some textures twice!
//----------------------------------------------------------------------------
static bool getDDXTextureInfo(const BString& filename, uint& width, uint& height, eDDXDataFormat& format)
{
   BFile file;  

   if (!file.openReadOnly(cDirParticlesRoot, filename))
   {
      gConsoleOutput.error("getDDXTextureInfo: Unable to load texture %s\n", filename.getPtr());
      return false;
   }

   unsigned long fileSize;
   if (!file.getSize(fileSize))
   {
      gConsoleOutput.error("getDDXTextureInfo: Unable to load texture %s\n", filename.getPtr());   
      return false;
   }

   BDynamicParticleArray<uchar> data(fileSize);
   if (!file.read(data.getPtr(), fileSize))
   {
      gConsoleOutput.error("getDDXTextureInfo: Unable to read texture %s\n", filename.getPtr());
      return false;
   }
   
   BDDXDesc desc;   
   if (!BDDXUtils::getDesc(data.getPtr(), data.getSize(), desc))
   {
      gConsoleOutput.error("getDDXTextureInfo: Failed getting description of texture %s\n", filename.getPtr());
      return false;
   }

   width = desc.mWidth;
   height = desc.mHeight;
   format = desc.mDataFormat;
   if (getDDXDataFormatIsDXTQ(format))
      format = getDDXDXTQBaseFormat(format);
   
   return true;      
}

//----------------------------------------------------------------------------
// BParticleTextureManager::BParticleTextureManager
//----------------------------------------------------------------------------
BParticleTextureManager::BParticleTextureManager() :
   mInitialized(false)
{
}

//----------------------------------------------------------------------------
// BParticleTextureManager::~BParticleTextureManager
//----------------------------------------------------------------------------
BParticleTextureManager::~BParticleTextureManager()
{
}

//----------------------------------------------------------------------------
// BParticleTextureManager::init
//----------------------------------------------------------------------------
void BParticleTextureManager::init()
{
   ASSERT_MAIN_THREAD
   
   if (mInitialized)
      return;

   mInitialized = true;
}

//----------------------------------------------------------------------------
// BParticleTextureManager::deInit
//----------------------------------------------------------------------------
void BParticleTextureManager::deInit()
{
   ASSERT_MAIN_THREAD
   
   if (!mInitialized)
      return;
   
   mInitialized = false;

   clear();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BParticleTextureManager::initTextureManagement()
{
   mTextureArrayManagers.resize(0);
   mTextureArrayManagers.reserve(16);

   if (!initProgressionTextures())
      return false;
      
   if (!initRandomTexture())
      return false;
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BParticleTextureManager::deinitTextureManagement()
{
   clear();
   deinitProgressionTextures();
   deinitRandomTexture();
}

//----------------------------------------------------------------------------
// BParticleTextureManager::clear()
//----------------------------------------------------------------------------
void BParticleTextureManager::clear()
{
   for (uint i = 0; i < mTextureArrayManagers.getSize(); ++i)
   {
      ALIGNED_DELETE(mTextureArrayManagers[i], gParticleHeap);
      mTextureArrayManagers[i] = NULL;
   }
   mTextureArrayManagers.clear();   
}

//----------------------------------------------------------------------------
// BParticleTextureManager::tick
//----------------------------------------------------------------------------
void BParticleTextureManager::tick(void)
{
#ifndef BUILD_FINAL
   for (uint i = 0; i < mTextureArrayManagers.getSize(); i++)
   {
      if (mTextureArrayManagers[i])
         mTextureArrayManagers[i]->tick();
   }
#endif   
}

//----------------------------------------------------------------------------
// BParticleTextureManager::packArrayIndexData
//----------------------------------------------------------------------------
DWORD BParticleTextureManager::packArrayIndexData(int arrayIndex, int texIndex)
{
   DWORD output; 
   output = (arrayIndex << 16) | texIndex;
   return output;

}

//----------------------------------------------------------------------------
// BParticleTextureManager::unpackArrayIndexData
//----------------------------------------------------------------------------
void  BParticleTextureManager::unpackArrayIndexData(DWORD data, int& arrayIndex, int& texIndex)
{
   arrayIndex = (data & 0xFFFF0000) >> 16;
   texIndex   = data & 0x0000FFFF;
}

//----------------------------------------------------------------------------
// BParticleTextureManager::loadTextureSet
//----------------------------------------------------------------------------
bool BParticleTextureManager::loadTextureSet(
  const BEmitterTextureSet* pSet, 
  BTextureArrayIndex& textureArrayIndex, 
  BParticleTextureArrayManager::BTextureArraySlotIndices& textureSlotIndices,
  uint& width, uint& height)
{
   textureArrayIndex = cInvalidIndex;
   textureSlotIndices.resize(0);
   width = 0;
   height = 0;
   
   if (!pSet)
      return false;

   const uint numTextures = pSet->mStages.getSize();   

   const uint cMaxTextureArraySize = 64;
      
   if ((numTextures < 1) || (numTextures > cMaxTextureArraySize))
   {
      gConsoleOutput.error("BParticleTextureManager::loadTextureSet: Invalid number of textures");
      return false;
   }
      
   BParticleTextureArrayManager::BStringArray filenames(numTextures);
      
   for (uint i = 0; i < numTextures; i++)
   {
      filenames[i].set(pSet->mStages[i].mFilename); 
      filenames[i].removeExtension();
      filenames[i].append(".ddx");
      filenames[i].toLower();
   }
   
   int bestTextureArrayIndex = -1;

   uint bestNumMatches = 0;
   
   for (uint i = 0; i < mTextureArrayManagers.getSize(); i++)
   {
//-- FIXING PREFIX BUG ID 7288
      const BParticleTextureArrayManager& textureArray = *mTextureArrayManagers[i];
//--
      
      uint numMatches;
      if (!textureArray.canLoadTextures(filenames, &numMatches))
         continue;
    
      if (numMatches > bestNumMatches)     
      {
         bestNumMatches = numMatches;
         bestTextureArrayIndex = i;
      }
   }
   
   uint ddxWidth = 0;
   uint ddxHeight = 0;
   eDDXDataFormat format = cDDXDataFormatInvalid;
   
   if (bestTextureArrayIndex == -1)
   {
      if (!getDDXTextureInfo(filenames[0], ddxWidth, ddxHeight, format))
      {
         gConsoleOutput.error("BParticleTextureManager::loadTextureSet: Unable to load texture: %s", filenames[0].getPtr());
         return false;
      }
      
      gConsoleOutput.resource("BParticleTextureManager::loadTextureSet: Retrieved info for file %s: %u %u %s\n", filenames[0].getPtr(), ddxWidth, ddxHeight, getDDXDataFormatString(format));
         
      for (uint i = 0; i < mTextureArrayManagers.getSize(); i++)
      {
         BParticleTextureArrayManager& textureArray = *mTextureArrayManagers[i];               
         
         if ( (textureArray.getWidth() == ddxWidth) && (textureArray.getHeight() == ddxHeight) && (textureArray.getDDXDataFormat() == format) )
         {
            uint maxNewTextures = textureArray.getMaxTextures() - textureArray.getNumTextures();
            if (maxNewTextures >= numTextures)
            {
               bestTextureArrayIndex = i;
               break;
            }
         }
      }
   }

   bool createNewTextureArray = false;
   if (bestTextureArrayIndex == -1)
   {
      bestTextureArrayIndex = mTextureArrayManagers.getSize();

      uint textureArraySize = 8;            
      
      if (Math::Max(ddxWidth, ddxHeight) > 256)
         textureArraySize = 4;
      
      textureArraySize = Math::Max(numTextures, textureArraySize);
      mTextureArrayManagers.pushBack( ALIGNED_NEW_INIT(BParticleTextureArrayManager, gParticleHeap, textureArraySize) );
      
      createNewTextureArray = true;
   }
   
   BParticleTextureArrayManager& textureArray = *mTextureArrayManagers[bestTextureArrayIndex];
   
   if (!textureArray.loadTextures(filenames, textureSlotIndices))
   {
      gConsoleOutput.error("BParticleTextureManager::loadTextureSet: textureArray.loadTextures() failed");
      return false;
   }

#ifdef BUILD_DEBUG      
   BDEBUG_ASSERT(textureSlotIndices.getSize() == numTextures);
   for (uint i = 0; i < textureSlotIndices.getSize(); i++)
   {
      BDEBUG_ASSERT(textureSlotIndices[i] < (int)textureArray.getNumTextures());  
   }
#endif   
      
   textureArrayIndex = bestTextureArrayIndex;
   
   width = textureArray.getWidth();
   height = textureArray.getHeight();
   
   if (createNewTextureArray)
   {
      gConsoleOutput.resource("BParticleTextureManager::loadTextureSet: Created new array: %ux%u, Fmt: %s, Size: %u, Ptr: 0x%08X", width, height, getDDXDataFormatString(textureArray.getDDXDataFormat()), textureArray.getMaxTextures(), &textureArray);
   }
      
   return true;
}

#ifndef BUILD_FINAL
//----------------------------------------------------------------------------
// BParticleTextureManager::getTextureAllocStats
//----------------------------------------------------------------------------
void BParticleTextureManager::getTextureAllocStats(BTextureAllocStatsArray& stats)
{
   stats.clear();
   
   for (uint arrayIndex = 0; arrayIndex < mTextureArrayManagers.getSize(); arrayIndex++)
   {
//-- FIXING PREFIX BUG ID 7289
      const BParticleTextureArrayManager* pTextureArray = mTextureArrayManagers[arrayIndex];
//--
      if (!pTextureArray)
         continue;
      
      BTextureAllocStats* pStats = stats.enlarge(1);
      for (uint filenameIndex = 0; filenameIndex < pTextureArray->getNumTextures(); filenameIndex++)
      {
         pStats->mFilenames = pTextureArray->getTextureFilenames();
         pStats->mAllocationSize = pTextureArray->getAllocationSize();
         pStats->mWidth = (uint16)pTextureArray->getWidth();
         pStats->mHeight = (uint16)pTextureArray->getHeight();
         pStats->mDDXFormat = pTextureArray->getDDXDataFormat();
         pStats->mLevels = (uint8)pTextureArray->getLevels();
         pStats->mArraySize = (uint8)pTextureArray->getMaxTextures();
      }
   }
}
#endif
//----------------------------------------------------------------------------
// BParticleTextureManager::initProgressionTextures
//----------------------------------------------------------------------------
bool BParticleTextureManager::initProgressionTextures()
{   
   if (!createProgressionTexture(128, cMaxColorProgressionScanLines, D3DFMT_LIN_A8R8G8B8, &mColorTexture))
      return false;

   if (!createProgressionTexture(128, cMaxScaleProgressionScanLines, D3DFMT_LIN_A8R8G8B8, &mScaleTexture))
      return false;

   if (!createProgressionTexture(128, cMaxIntensityProgressionScanLines, D3DFMT_LIN_A8R8G8B8, &mIntensityTexture))
      return false;

   return true;
}
//----------------------------------------------------------------------------
// BParticleTextureManager::deinitProgressionTextures
//----------------------------------------------------------------------------
void BParticleTextureManager::deinitProgressionTextures()
{   
   mColorTexture.clear();
   mScaleTexture.clear();
   mIntensityTexture.clear();

   mScaleProgressionEntries.clear();
   mColorProgressionEntries.clear();
   mIntensityProgressionEntries.clear();
}

//----------------------------------------------------------------------------
// BParticleTextureManager::initRandomTexture
//----------------------------------------------------------------------------
bool BParticleTextureManager::initRandomTexture()
{
   if (!createRandomTexture(256, 256, D3DFMT_R16F, &mRandomTexture))
      return false;

   return true;
}

//----------------------------------------------------------------------------
// BParticleTextureManager::deinitRandomTexture
//----------------------------------------------------------------------------
void BParticleTextureManager::deinitRandomTexture()
{
   mRandomTexture.clear();
}

//----------------------------------------------------------------------------
// BParticleTextureManager::createRandomTexture
//----------------------------------------------------------------------------
bool BParticleTextureManager::createRandomTexture(int width, int height, D3DFORMAT format, BParticleRandomTextureData* RESTRICT pData)
{
   pData->mHeight = height;
   pData->mWidth  = width;
   pData->mpTexture = NULL;
   HRESULT hr = gRenderDraw.createTexture(pData->mWidth, pData->mHeight, 1, 0, format, 0, &pData->mpTexture, NULL);
   if (FAILED(hr))
      return false;

   D3DLOCKED_RECT rect;
   pData->mpTexture->LockRect(0, &rect, NULL, 0);
   pData->mPitch = rect.Pitch;
   pData->mpData = rect.pBits;

   float randomNumber;
   
   HALF* pPixels = static_cast<HALF*>(rect.pBits);
   for (int y = 0; y < height; ++y)
   {       
      for (int x = 0; x < width; ++x)
      {
         const uint ofs = XGAddress2DTiledOffset(x, y, width, sizeof(HALF));         
         randomNumber = Math::fRand(-1.0f, 1.0f);

         ushort h = HalfFloat::FloatToHalf(randomNumber);
         pPixels[ofs] = h;
      }
   }

   pData->mpTexture->UnlockRect(0);
   return true;
}



//----------------------------------------------------------------------------
// BParticleTextureManager::createProgressionTexture
//----------------------------------------------------------------------------
bool BParticleTextureManager::createProgressionTexture(int width, int height, D3DFORMAT format, BParticleProgressionTextureData* RESTRICT pData)
{
   pData->mHeight = height;
   pData->mWidth  = width;
   pData->mpTexture = NULL;
   HRESULT hr = gRenderDraw.createTexture(pData->mWidth, pData->mHeight, 1, 0, format, 0, &pData->mpTexture, NULL);
   if (FAILED(hr))
      return false;

   D3DLOCKED_RECT rect;
   pData->mpTexture->LockRect(0, &rect, NULL, 0);
   pData->mPitch = rect.Pitch;
   pData->mpData = rect.pBits;

   //-- set the texture to white
   memset(rect.pBits, 0x0000FFFF, rect.Pitch * pData->mHeight);
   //-- the first scan line is always used for values of 1.0f

   pData->mNextAvailableScanline = 1;
   pData->mUseCount = 1;

   pData->mpTexture->UnlockRect(0);
   return true;
}

//----------------------------------------------------------------------------
// BParticleTextureManager::initEmitterProgression
//----------------------------------------------------------------------------
bool BParticleTextureManager::initEmitterProgression(BParticleEmitterDefinition* RESTRICT pDefinition, int variance, XMVECTOR* RESTRICT progressionV)
{
   if (!pDefinition)
      return false;

   initColorProgression(pDefinition, variance, progressionV->x);
   initScaleProgression(pDefinition, variance, progressionV->y);
   initIntensityProgression(pDefinition, variance, progressionV->z);
   return true;
}

//----------------------------------------------------------------------------
// BParticleTextureManager::initColorProgression
//----------------------------------------------------------------------------
void BParticleTextureManager::initColorProgression(BParticleEmitterDefinition* RESTRICT pDefinition, int variance, float& colorProgressionV)
{  
   colorProgressionV = 0.0f;
   
   //-- do we have space left? If not then just use a flat progression
   if (mColorTexture.mUseCount >= mColorTexture.mHeight)
      return;
   //-- figure out which scanline to process
   int scanLine = mColorTexture.mNextAvailableScanline;

   //-- search if this definition has already an entry and if we do then just use that
   for (uint i = 0; i < mColorProgressionEntries.getSize(); ++i)
   {
      if (pDefinition == mColorProgressionEntries[i].pDefinition)
      {
         //-- we found it and we are not forcing a refresh then just return our entry         
         colorProgressionV = mColorProgressionEntries[i].mV;
         return;
      }
   }

   //-- we don't have an entry 
   XMVECTOR colorV, opacityV;
   XMCOLOR outColor;

   bool bAddNewEntry = false;
   
   if (pDefinition->mColor.mType == BEmitterColorData::eProgression)
   {           
      colorProgressionV = (float) scanLine/ ((float) (mColorTexture.mHeight));
      DWORD* pColorProgressionScanline = (DWORD*) (((BYTE*) mColorTexture.mpData) + (scanLine*mColorTexture.mPitch));
      DWORD* pStartScanline = pColorProgressionScanline;
            
      if (pDefinition->mOpacity.mUseProgression)
      {         

         float colorWidth = (float)mColorTexture.mWidth;
         float opacityWidth = (float)mColorTexture.mWidth;

         //-- if we are looping the opacity
         if (pDefinition->mOpacity.mpProgression->mLoop)
         {
            float cycles = pDefinition->mOpacity.mpProgression->mCycles;
            if (cycles <= cFloatCompareEpsilon)
               cycles = 1.0f;

            opacityWidth /= cycles;
         }

         //-- if we are looping the color
         if (pDefinition->mColor.mpProgression->mLoop)
         {
            float cycles = pDefinition->mColor.mpProgression->mCycles;
            if (cycles <= cFloatCompareEpsilon)
               cycles = 1.0f;
            colorWidth /= cycles;
         }         

         for (int x = 0; x < mColorTexture.mWidth; ++x)
         {              
            int opacityInterval = Math::FloatToIntTrunc((float) x / opacityWidth);
            int colorInterval   = Math::FloatToIntTrunc((float) x / colorWidth);
            int opacityStart    = (int)(opacityInterval * opacityWidth);
            int colorStart      = (int)(colorInterval * colorWidth);

            float opacityAlpha  = (float) (x-opacityStart) / (float) (opacityWidth - 1.0f);
            float colorAlpha    = (float) (x-colorStart) / (float) (colorWidth - 1.0f);

            opacityAlpha = Math::Clamp(opacityAlpha, 0.0f, 1.0f);
            colorAlpha   = Math::Clamp(colorAlpha, 0.0f, 1.0f);

            //trace("Pixel = %u, Opacity Alpha = %4.2f, Color Alpha = %4.2f", x, opacityAlpha, colorAlpha);

            pDefinition->mColor.mpProgression->getValue(colorAlpha, &colorV);         
            pDefinition->mOpacity.mpProgression->getValue(opacityAlpha, 0, &opacityV);
            colorV.w = opacityV.x;

            XMStoreColor(&outColor, colorV);
            *pColorProgressionScanline = outColor.c;
            pColorProgressionScanline++;
         }
      }
      else
      {         
         float colorWidth = (float)mColorTexture.mWidth;
         //-- if we are looping the color
         if (pDefinition->mColor.mpProgression->mLoop)
         {
            float cycles = pDefinition->mColor.mpProgression->mCycles;
            if (cycles <= cFloatCompareEpsilon)
               cycles = 1.0f;
            colorWidth /= cycles;
         }         

         for (int x = 0; x < mColorTexture.mWidth; ++x)
         {

            int colorInterval   = Math::FloatToIntTrunc((float) x / colorWidth);
            int colorStart      = (int)(colorInterval * colorWidth);

            float colorAlpha    = (float) (x-colorStart) / (float) (colorWidth - 1.0f);

            colorAlpha   = Math::Clamp(colorAlpha, 0.0f, 1.0f);
            pDefinition->mColor.mpProgression->getValue(colorAlpha, &colorV);         
            colorV.w = 1.0f;

            XMStoreColor(&outColor, colorV);
            *pColorProgressionScanline = outColor.c;
            pColorProgressionScanline++;
         }
      }

      BDEBUG_ASSERT(sizeof(DWORD)*mColorTexture.mWidth == (DWORD)mColorTexture.mPitch);
      gRenderDraw.invalidateGPUCache(pStartScanline, mColorTexture.mPitch);

      bAddNewEntry = true;
   }
   else if (pDefinition->mColor.mType == BEmitterColorData::eSingleColor)
   {
      colorProgressionV = (float) scanLine/ ((float) (mColorTexture.mHeight));
      DWORD* pColorProgressionScanline = (DWORD*) (((BYTE*) mColorTexture.mpData) + (scanLine*mColorTexture.mPitch));
      DWORD* pStartScanline = pColorProgressionScanline;

      if (pDefinition->mOpacity.mUseProgression)
      {
         float opacityWidth = (float)mColorTexture.mWidth;

         //-- if we are looping the opacity
         if (pDefinition->mOpacity.mpProgression->mLoop)
         {
            float cycles = pDefinition->mOpacity.mpProgression->mCycles;
            if (cycles <= cFloatCompareEpsilon)
               cycles = 1.0f;

            opacityWidth /= cycles;
         }

         for (int x = 0; x < mColorTexture.mWidth; ++x)
         {
            int opacityInterval = Math::FloatToIntTrunc((float) x / opacityWidth);
            int opacityStart    = (int)(opacityInterval * opacityWidth);
            float opacityAlpha  = (float) (x-opacityStart) / (float) (opacityWidth - 1.0f);

            opacityAlpha = Math::Clamp(opacityAlpha, 0.0f, 1.0f);
            //trace("Pixel = %u, Opacity Alpha = %4.2f, Color Alpha = %4.2f", x, opacityAlpha, colorAlpha);

            colorV = pDefinition->mColor.mColor;
            pDefinition->mOpacity.mpProgression->getValue(opacityAlpha, 0, &opacityV);
            colorV.w = opacityV.x;

            XMStoreColor(&outColor, colorV);
            *pColorProgressionScanline = outColor.c;
            pColorProgressionScanline++;
         }
      }
      else
      {
         for (int x = 0; x < mColorTexture.mWidth; ++x)
         {
            //float alpha = (float) x / (float) (mColorTexture.mWidth - 1.0f);
            colorV = pDefinition->mColor.mColor;
            colorV.w = 1.0f;

            XMStoreColor(&outColor, colorV);
            *pColorProgressionScanline = outColor.c;
            pColorProgressionScanline++;
         }
      }

      BDEBUG_ASSERT(sizeof(DWORD)*mColorTexture.mWidth == (DWORD)mColorTexture.mPitch);
      gRenderDraw.invalidateGPUCache(pStartScanline, mColorTexture.mPitch);
      bAddNewEntry = true;
   }
   else if (pDefinition->mColor.mType == BEmitterColorData::ePalletteColor)
   {
      colorProgressionV = (float) scanLine/ ((float) (mColorTexture.mHeight));
      DWORD* pColorProgressionScanline = (DWORD*) (((BYTE*) mColorTexture.mpData) + (scanLine*mColorTexture.mPitch));
      DWORD* pStartScanline = pColorProgressionScanline;

      float opacityWidth = (float)mColorTexture.mWidth;

      //-- if we are looping the opacity
      if (pDefinition->mOpacity.mpProgression->mLoop)
      {
         float cycles = pDefinition->mOpacity.mpProgression->mCycles;
         if (cycles <= cFloatCompareEpsilon)
            cycles = 1.0f;

         opacityWidth /= cycles;
      }
      
      for (int x = 0; x < mColorTexture.mWidth; ++x)
      {
         colorV = gVectorOne;
         if (pDefinition->mOpacity.mUseProgression)
         {
            int opacityInterval = Math::FloatToIntTrunc((float) x / opacityWidth);
            int opacityStart    = (int)(opacityInterval * opacityWidth);
            float opacityAlpha  = (float) (x-opacityStart) / (float) (opacityWidth - 1.0f);

            opacityAlpha = Math::Clamp(opacityAlpha, 0.0f, 1.0f);

            pDefinition->mOpacity.mpProgression->getValue(opacityAlpha, 0, &opacityV);
            colorV.w = opacityV.x;
         }

         XMStoreColor(&outColor, colorV);
         *pColorProgressionScanline = outColor.c;
         pColorProgressionScanline++;
      }

      BDEBUG_ASSERT(sizeof(DWORD)*mColorTexture.mWidth == (DWORD)mColorTexture.mPitch);
      gRenderDraw.invalidateGPUCache(pStartScanline, mColorTexture.mPitch);
      bAddNewEntry = true;
   }

   if (bAddNewEntry)
   {
      //-- add a new entry
      BParticleProgressionTextureEntry entry;
      entry.pDefinition = pDefinition;
      entry.mV = colorProgressionV;
      entry.mScanline = scanLine;
      mColorProgressionEntries.add(entry);

      //-- increment our use counts
      mColorTexture.mNextAvailableScanline++;
      mColorTexture.mUseCount++;
   }
}

//----------------------------------------------------------------------------
// BParticleTextureManager::initScaleProgression
//----------------------------------------------------------------------------
void BParticleTextureManager::initScaleProgression(BParticleEmitterDefinition* RESTRICT pDefinition, int variance, float& scaleProgressionV)
{
   scaleProgressionV = 0.0f;

   if (!pDefinition->mScale.mUseXProgression &&
       !pDefinition->mScale.mUseYProgression &&
       !pDefinition->mScale.mUseZProgression)
       return;

   //-- do we have space left? If not then just use a flat progression
   if (mScaleTexture.mUseCount >= mScaleTexture.mHeight)
      return;

   int scanLine = mScaleTexture.mNextAvailableScanline;

   //-- search if this definition has already an entry and if we do then just use that
   for (uint i = 0; i < mScaleProgressionEntries.getSize(); ++i)
   {
      if (pDefinition == mScaleProgressionEntries[i].pDefinition)
      {
         scaleProgressionV = mScaleProgressionEntries[i].mV;
         return;
      }
   }

   //-- we don't have an entry 
   scaleProgressionV = (float) scanLine / ((float) (mScaleTexture.mHeight));
   DWORD* pProgressionScanline = (DWORD*) (((BYTE*) mScaleTexture.mpData) + (scanLine*mScaleTexture.mPitch));
   DWORD* pStartScanline = pProgressionScanline;

   XMVECTOR vOut, xValue, yValue, zValue;
   XMCOLOR outColor;


   float xWidth = (float)mScaleTexture.mWidth;
   float yWidth = (float)mScaleTexture.mWidth;
   float zWidth = (float)mScaleTexture.mWidth;

   //-- if we are looping the opacity
   if (pDefinition->mScale.mUseXProgression && pDefinition->mScale.mpProgression->mXProgression.mLoop)
   {
      float cycles = pDefinition->mScale.mpProgression->mXProgression.mCycles;
      if (cycles <= cFloatCompareEpsilon)
         cycles = 1.0f;

      xWidth /= cycles;
   }

   if (pDefinition->mScale.mUseYProgression && pDefinition->mScale.mpProgression->mYProgression.mLoop)
   {
      float cycles = pDefinition->mScale.mpProgression->mYProgression.mCycles;
      if (cycles <= cFloatCompareEpsilon)
         cycles = 1.0f;

      yWidth /= cycles;
   }

   if (pDefinition->mScale.mUseZProgression && pDefinition->mScale.mpProgression->mZProgression.mLoop)
   {
      float cycles = pDefinition->mScale.mpProgression->mZProgression.mCycles;
      if (cycles <= cFloatCompareEpsilon)
         cycles = 1.0f;

      zWidth /= cycles;
   }

   for (int x = 0; x < mScaleTexture.mWidth; ++x)
   {      
      vOut = gVectorOne;

      if (pDefinition->mScale.mUseXProgression)
      {
         int interval = Math::FloatToIntTrunc((float) x / xWidth);
         int start    = (int)(interval * xWidth);
         float alpha  = (float) (x-start) / (float) (xWidth - 1.0f);

         alpha = Math::Clamp(alpha, 0.0f, 1.0f);

         pDefinition->mScale.mpProgression->getXValue(alpha, variance, &xValue);
         vOut.x = xValue.x;
      }

      if (pDefinition->mScale.mUseYProgression)
      {
         int interval = Math::FloatToIntTrunc((float) x / yWidth);
         int start    = (int)(interval * yWidth);
         float alpha  = (float) (x-start) / (float) (yWidth - 1.0f);

         alpha = Math::Clamp(alpha, 0.0f, 1.0f);

         pDefinition->mScale.mpProgression->getYValue(alpha, variance, &yValue);
         vOut.y = yValue.x;
      }

      if (pDefinition->mScale.mUseZProgression)
      {
         int interval = Math::FloatToIntTrunc((float) x / zWidth);
         int start    = (int)(interval * zWidth);
         float alpha  = (float) (x-start) / (float) (zWidth - 1.0f);

         alpha = Math::Clamp(alpha, 0.0f, 1.0f);

         pDefinition->mScale.mpProgression->getZValue(alpha, variance, &zValue);
         vOut.z = zValue.x;
      }

      vOut.w = 1.0f;

      XMStoreColor(&outColor, vOut);
      *pProgressionScanline = outColor.c;
      pProgressionScanline++;
   }

   gRenderDraw.invalidateGPUCache(pStartScanline, mScaleTexture.mPitch);

   //-- add a new entry
   BParticleProgressionTextureEntry entry;
   entry.pDefinition = pDefinition;
   entry.mV = scaleProgressionV;
   entry.mScanline = scanLine;
   mScaleProgressionEntries.add(entry);

   //-- increment our use counts
   mScaleTexture.mNextAvailableScanline++;
   mScaleTexture.mUseCount++;      
}

//----------------------------------------------------------------------------
// BParticleTextureManager::initIntensityProgression
//----------------------------------------------------------------------------
void BParticleTextureManager::initIntensityProgression(BParticleEmitterDefinition* RESTRICT pDefinition, int variance, float& intensityProgressionV)
{
   intensityProgressionV = 0.0f;

   if (!pDefinition->mIntensity.mUseProgression)
      return;

   //-- do we have space left? If not then just use a flat progression
   if (mIntensityTexture.mUseCount >= mIntensityTexture.mHeight)
      return;

   //-- search if this definition has already an entry and if we do then just use that
   for (uint i = 0; i < mIntensityProgressionEntries.getSize(); ++i)
   {
      if (pDefinition == mIntensityProgressionEntries[i].pDefinition)
      {
         intensityProgressionV = mIntensityProgressionEntries[i].mV;
         return;
      }
   }

   //-- we don't have an entry 
   intensityProgressionV = (float) mIntensityTexture.mNextAvailableScanline / ((float) (mIntensityTexture.mHeight));
   DWORD* pProgressionScanline = (DWORD*) (((BYTE*) mIntensityTexture.mpData) + (mIntensityTexture.mNextAvailableScanline*mIntensityTexture.mPitch));
   DWORD* pStartScanline = pProgressionScanline;

   XMVECTOR vOut, value;
   XMCOLOR outColor;

   float intensityWidth = (float)mIntensityTexture.mWidth;

   //-- if we are looping the opacity
   if (pDefinition->mIntensity.mpProgression->mLoop)
   {
      float cycles = pDefinition->mIntensity.mpProgression->mCycles;
      if (cycles <= cFloatCompareEpsilon)
         cycles = 1.0f;

      intensityWidth /= cycles;
   }

   for (int x = 0; x < mIntensityTexture.mWidth; ++x)
   {
      vOut = gVectorOne;

      int interval = Math::FloatToIntTrunc((float) x / intensityWidth);
      int start    = (int)(interval * intensityWidth);
      float alpha  = (float) (x-start) / (float) (intensityWidth - 1.0f);

      alpha = Math::Clamp(alpha, 0.0f, 1.0f);

      pDefinition->mIntensity.mpProgression->getValue(alpha, variance, &value);

      vOut = XMVectorReplicate(value.x);
      XMStoreColor(&outColor, vOut);
      *pProgressionScanline = outColor.c;
      pProgressionScanline++;
   }

   gRenderDraw.invalidateGPUCache(pStartScanline, mIntensityTexture.mPitch);

   //-- add a new entry
   BParticleProgressionTextureEntry entry;
   entry.pDefinition = pDefinition;
   entry.mV = intensityProgressionV;
   mIntensityProgressionEntries.add(entry);

   //-- increment our use counts
   mIntensityTexture.mNextAvailableScanline++;
   mIntensityTexture.mUseCount++;      
}

//----------------------------------------------------------------------------
// BParticleTextureManager::getTexture
//----------------------------------------------------------------------------
LPDIRECT3DBASETEXTURE9 BParticleTextureManager::getTexture(BTextureArrayIndex index) const
{
   ASSERT_RENDER_THREAD   
   BDEBUG_ASSERT((index >= 0) && (index < (int)mTextureArrayManagers.getSize()) && mTextureArrayManagers[index]);

   if (index < 0)
      return NULL;

   if (index >= mTextureArrayManagers.getSize())
      return NULL;

   if (mTextureArrayManagers[index] == NULL)
      return NULL;
   
   return mTextureArrayManagers[index]->getD3DTexture().getBaseTexture();
}

//----------------------------------------------------------------------------
// BParticleTextureManager::getTextureArraySize
//----------------------------------------------------------------------------
uint BParticleTextureManager::getTextureArraySize(BTextureArrayIndex index) const
{
   ASSERT_RENDER_THREAD   
   BDEBUG_ASSERT((index >= 0) && (index < (int)mTextureArrayManagers.getSize()) && mTextureArrayManagers[index]);

   if (index < 0)
      return 0;

   if (index >= mTextureArrayManagers.getSize())
      return 0;

   if (mTextureArrayManagers[index] == NULL)
      return 0;

   return mTextureArrayManagers[index]->getMaxTextures();
}

//----------------------------------------------------------------------------
// BParticleTextureManager::getTextureArrayNumUsed
//----------------------------------------------------------------------------
uint BParticleTextureManager::getTextureArrayNumUsed(BTextureArrayIndex index) const
{
   ASSERT_RENDER_THREAD   
   BDEBUG_ASSERT((index >= 0) && (index < (int)mTextureArrayManagers.getSize()) && mTextureArrayManagers[index]);

   if (index < 0)
      return 0;

   if (index >= mTextureArrayManagers.getSize())
      return 0;

   if (mTextureArrayManagers[index] == NULL)
      return 0;

   return mTextureArrayManagers[index]->getNumTextures();
}