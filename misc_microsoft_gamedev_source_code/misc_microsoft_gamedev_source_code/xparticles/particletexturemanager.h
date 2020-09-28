//============================================================================
// File: particleTextureManager.h
//============================================================================
#pragma once

#include "D3DTextureLoader.h"

#include "particleeffectdata.h"
#include "reloadManager.h"

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
class BParticleProgressionTextureEntry
{
   public:

      BParticleProgressionTextureEntry(){clear();};
     ~BParticleProgressionTextureEntry(){clear();};

      void clear()
      {
         mV=0.0f;
         mScanline = -1;
         pDefinition = NULL;
      };


   BParticleEmitterDefinition* pDefinition;
   float                       mV;
   int                         mScanline;
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
class BParticleProgressionTextureData
{
   public:
      BParticleProgressionTextureData();
     ~BParticleProgressionTextureData();
      void clear();

     int   mWidth;
     int   mHeight;
     int   mUseCount;
     IDirect3DTexture9* mpTexture;
     void* mpData;
     int   mPitch;
     int   mNextAvailableScanline;
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
class BParticleRandomTextureData
{
public:
   BParticleRandomTextureData();
   ~BParticleRandomTextureData();
   void clear();

   int   mWidth;
   int   mHeight;
   IDirect3DTexture9* mpTexture;
   void* mpData;
   int   mPitch;
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
class BParticleTextureArrayManager
{
   BParticleTextureArrayManager(const BParticleTextureArrayManager&);
   BParticleTextureArrayManager& operator= (const BParticleTextureArrayManager&);
   
public:
   enum { cDefaultMaxTextures = 16 };
   
   BParticleTextureArrayManager(uint maxTextures = cDefaultMaxTextures);
   ~BParticleTextureArrayManager();
   
   void clear(void);
   void tick(void);
   
   void reloadTextures(void);

   const BD3DTexture& getD3DTexture(void) const { return mTextureLoader.getD3DTexture(); }
         BD3DTexture& getD3DTexture(void)       { return mTextureLoader.getD3DTexture(); }
      
   typedef BDynamicParticleArray<BString> BStringArray;
   
   const BStringArray& getTextureFilenames(void) const { return mTextureFilenames; }
   
   uint getMaxTextures(void) const { return mMaxTextures; }
   
   uint getNumTextures(void) const { return mTextureFilenames.getSize(); }
   const BString& getTextureFilename(uint i) const { return mTextureFilenames[i]; }
         
   // Returns cInvalidIndex if not found;
   int findTexture(const BString& filename) const;
   
   uint getWidth(void) const { return mWidth; }
   uint getHeight(void) const { return mHeight; }
   eDDXDataFormat getDDXDataFormat(void) const { return mTextureLoader.getDDXDataFormat(); }

   bool canLoadTextures(const BStringArray& filenames, uint* pNumMatches = NULL) const;
   
   typedef BDynamicParticleArray<short> BTextureArraySlotIndices;
   bool loadTextures(const BStringArray& filenames, BTextureArraySlotIndices& textureIndices);
   uint getLevels(void) const { return mTextureLoader.getLevels(); }   
   uint getAllocationSize(void) const { return mTextureLoader.getAllocationSize(); }
         
private:
   BD3DTextureLoader    mTextureLoader;
   BStringArray         mTextureFilenames;
#ifdef ENABLE_RELOAD_MANAGER
   BFileWatcher         mFileWatcher;
#endif
   ushort               mMaxTextures;
   
   ushort               mWidth;
   ushort               mHeight;
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
class BParticleTextureManager
{
   public:
      BParticleTextureManager();
     ~BParticleTextureManager();

      void init();
      void deInit();
      void clear();
      void tick();

      bool initTextureManagement();
      void deinitTextureManagement(); 

      typedef int BTextureArrayIndex;
            
      bool loadTextureSet(
         const BEmitterTextureSet* pSet, 
         BTextureArrayIndex& textureArrayIndex, 
         BParticleTextureArrayManager::BTextureArraySlotIndices& textureSlotIndices,
         uint& width, uint& height);
         
      LPDIRECT3DBASETEXTURE9 getTexture(BTextureArrayIndex index) const;      
      uint getTextureArraySize(BTextureArrayIndex index) const;
      uint getTextureArrayNumUsed(BTextureArrayIndex index) const;
      
      IDirect3DTexture9* getColorProgressionTexture() const { return mColorTexture.mpTexture;}
      IDirect3DTexture9* getScaleProgressionTexture() const { return mScaleTexture.mpTexture;}
      IDirect3DTexture9* getIntensityProgressionTexture() const { return mIntensityTexture.mpTexture;}
      IDirect3DTexture9* getRandomTexture() const           { return mRandomTexture.mpTexture;};

      bool initProgressionTextures();
      void deinitProgressionTextures();
      bool initEmitterProgression(BParticleEmitterDefinition* RESTRICT pDefinition, int variance, XMVECTOR* RESTRICT progressionV);
      
      // For debugging/stats purposes only
      uint getNumTextureArrays() const { return mTextureArrayManagers.getSize(); }
#ifndef BUILD_FINAL      
      struct BTextureAllocStats
      {
         BDynamicParticleArray<BString> mFilenames;
         uint              mAllocationSize;

         uint16            mWidth;
         uint16            mHeight;
         eDDXDataFormat    mDDXFormat;
         uint8             mLevels;
         uint8             mArraySize;

         bool operator< (const BTextureAllocStats& rhs) const { return mAllocationSize > rhs.mAllocationSize; }
      };

      typedef BDynamicParticleArray<BTextureAllocStats> BTextureAllocStatsArray;
      void getTextureAllocStats(BTextureAllocStatsArray& stats);
#endif      
            
   private:
      //-- Helpers
      DWORD packArrayIndexData(int arrayIndex, int texIndex);
      void  unpackArrayIndexData(DWORD data, int& arrayIndex, int& texIndex);     

      //-- Progression Helpers
      bool  createProgressionTexture(int width, int height, D3DFORMAT format, BParticleProgressionTextureData* RESTRICT pData);
      void  initColorProgression(BParticleEmitterDefinition* RESTRICT pDefinition, int variance, float& colorProgressionV);
      void  initScaleProgression(BParticleEmitterDefinition* RESTRICT pDefinition, int variance, float& scaleProgressionV);
      void  initIntensityProgression(BParticleEmitterDefinition* RESTRICT pDefinition, int variance, float& intensityProgressionV);

      //-- Random Texture
      bool  createRandomTexture(int width, int height, D3DFORMAT format, BParticleRandomTextureData* RESTRICT pData);
      bool  initRandomTexture();
      void  deinitRandomTexture();
                        
      BDynamicParticleArray<BParticleTextureArrayManager*>     mTextureArrayManagers;

      BDynamicParticleArray<BParticleProgressionTextureEntry>  mColorProgressionEntries;
      BDynamicParticleArray<BParticleProgressionTextureEntry>  mScaleProgressionEntries;
      BDynamicParticleArray<BParticleProgressionTextureEntry>  mIntensityProgressionEntries;

      BParticleProgressionTextureData                          mColorTexture;
      BParticleProgressionTextureData                          mScaleTexture;
      BParticleProgressionTextureData                          mIntensityTexture;

      BParticleRandomTextureData                               mRandomTexture;
      
      bool mInitialized : 1;      
};

extern BParticleTextureManager gPSTextureManager;
