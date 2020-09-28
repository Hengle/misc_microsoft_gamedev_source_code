//============================================================================
// EffectIntrinsicManager.h
//
// Copyright (c) 2006 Ensemble Studios
//============================================================================
#pragma once

#include "renderThread.h"
#include "effect.h"

//============================================================================
// Forward Declarations
//============================================================================
class BFXLEffect;
class BFXLEffectPool;
class BEffectIntrinsicMapper;

//============================================================================
// Effect Intrinsic Types
//============================================================================
enum eEffectIntrinsicType
{
   cIntrinsicTypeBool,
   cIntrinsicTypeFloat,
   cIntrinsicTypeInt4,
   cIntrinsicTypeFloat2,
   cIntrinsicTypeFloat3,
   cIntrinsicTypeFloat4,
   cIntrinsicTypeFloat4x4,
   cIntrinsicTypeTexturePtr,
   cIntrinsicTypeTextureHandle,

   cNumIntrinsicTypes,

   cInvalidIntrinsicType = 255
};

//============================================================================
// Effect Intrinsic Indices
//============================================================================
#define DECLARE_EFFECT_INTRINSIC(name, type, arraySize, shared) cIntrinsic##name,

enum eEffectIntrinsicIndex
{
   cInvalidIntrinsicIndex = -1,

#include "effectIntrinsicDefs.h"

   cNumEffectIntrinsics
};

#undef DECLARE_EFFECT_INTRINSIC

//============================================================================
// struct BEffectIntrinsicDesc
//============================================================================
struct BEffectIntrinsicDesc
{
   const char*          mpName;
   eEffectIntrinsicType mType;
   uchar                mArraySize;
   bool                 mShared : 1;
};

//============================================================================
// class BEffectIntrinsics
//============================================================================
class BEffectIntrinsics 
{
public:
   BEffectIntrinsics() { init(); }
   
   static void init(void); 
   
   static void initSharedParams(BFXLEffect* pEffect);
   
   // any thread
   static uint getIntrinsicTableOfs(eEffectIntrinsicIndex index) { BDEBUG_ASSERT((index <= cNumEffectIntrinsics) && mIntrinsicTableOffsets[1]); return mIntrinsicTableOffsets[index]; }
   
   // any thread
   static uint getSizeOfIntrinsicRange(eEffectIntrinsicIndex first, eEffectIntrinsicIndex last) {  BDEBUG_ASSERT((first < cNumEffectIntrinsics) && (last < cNumEffectIntrinsics) && (first <= last));  return mIntrinsicTableOffsets[last + 1] - mIntrinsicTableOffsets[first]; }
   
   // any thread
   // If this assert fires, you've tried to create an intrinsic table before the BEffectIntrinsics class has initialized itself.
   static uint getIntrinsicTableSize(void) { BDEBUG_ASSERT(mIntrinsicTableOffsets[1]); return mIntrinsicTableOffsets[cNumEffectIntrinsics]; }
         
   // any thread
   static uint getIntrinsicSize(eEffectIntrinsicIndex index);

   // any thread
   static uint getTypeSize(eEffectIntrinsicType type, uint num);

   // any thread
   static uint getTypeAlignment(eEffectIntrinsicType type);

   // any thread
   static eEffectIntrinsicIndex findIndex(const char* pName);
   
   // any thread
   static const BEffectIntrinsicDesc& getDesc(eEffectIntrinsicIndex index) { BDEBUG_ASSERT((index >= 0) && (index < cNumEffectIntrinsics)); return mEffectIntrinsicDesc[index]; }
   
   // any thread
   template<class T>
   static bool validateEffectParam(eEffectIntrinsicIndex index, const T& param)
   {
      if (BEffectIntrinsics::mEffectIntrinsicDesc[index].mArraySize > 1)
      {
         return param.isArray(BEffectIntrinsics::mEffectIntrinsicDesc[index].mArraySize, 
            getTypeSize(BEffectIntrinsics::mEffectIntrinsicDesc[index].mType, BEffectIntrinsics::mEffectIntrinsicDesc[index].mArraySize));
      }
      else
      {
         switch (BEffectIntrinsics::mEffectIntrinsicDesc[index].mType)
         {
            case cIntrinsicTypeBool:            return param.isBool(); 
            case cIntrinsicTypeFloat:           return param.isFloat(); 
            case cIntrinsicTypeInt4:            return param.isIntVector(); 
            case cIntrinsicTypeFloat2:          return param.isVector(2); 
            case cIntrinsicTypeFloat3:          return param.isVector(3); 
            case cIntrinsicTypeFloat4:          return param.isVector(4); 
            case cIntrinsicTypeFloat4x4:        return param.isMatrix(4, 4); 
            case cIntrinsicTypeTexturePtr:      return param.isSampler(); 
            case cIntrinsicTypeTextureHandle:   return param.isSampler(); 
            default:                            BDEBUG_ASSERT(0);
         }            
      }         

      return false;
   }
         
   template<class T>
   static void updateEffectParam(T* pEffect, FXLHANDLE handle, eEffectIntrinsicIndex intrinsicIndex, const void* pData)
   {
      BDEBUG_ASSERT((pEffect) && (handle != cInvalidIntrinsicType) && (intrinsicIndex >= 0) && (intrinsicIndex < cNumEffectIntrinsics));

      //const void* pData = getPtr(intrinsicIndex);

      switch  (mEffectIntrinsicDesc[intrinsicIndex].mType)      
      {
         case cIntrinsicTypeTextureHandle:
         {
            BDEBUG_ASSERT(mEffectIntrinsicDesc[intrinsicIndex].mArraySize == 1);

            BManagedTextureHandle textureHandle = *static_cast<const BManagedTextureHandle*>(pData);

            if (cInvalidManagedTextureHandle != textureHandle)
            {
               BD3DTextureManager::BManagedTexture* pTexture = gD3DTextureManager.getManagedTextureByHandle(textureHandle);

               if (pTexture)
               {
                  IDirect3DBaseTexture9* pD3DTexture = pTexture->getD3DTexture().getBaseTexture();

                  if ((pTexture->getStatus() != BD3DTextureManager::BManagedTexture::cStatusLoaded) || (!pD3DTexture))
                     pD3DTexture = gD3DTextureManager.getDefaultTexture(cDefaultTextureWhite)->getD3DTexture().getBaseTexture();
   
                  pEffect->SetSampler(handle, pD3DTexture);   
               }                     
            }
            break;
         }            
         case cIntrinsicTypeFloat4:
         {
            if (mEffectIntrinsicDesc[intrinsicIndex].mArraySize > 1)
               pEffect->SetVectorArrayF4A(handle, static_cast<const FXLFLOATA*>(pData), mEffectIntrinsicDesc[intrinsicIndex].mArraySize);
            else
               pEffect->SetVectorFA(handle, static_cast<const FXLFLOATA*>(pData));
            break;
         }
         case cIntrinsicTypeFloat4x4:
         {
            if (mEffectIntrinsicDesc[intrinsicIndex].mArraySize > 1)
               // WTF? This SetMatrixArrayF() dosn't work!
               //pEffect->SetMatrixArrayF(handle, reinterpret_cast<const FXLFLOATA*>(pData), mEffectIntrinsicDesc[intrinsicIndex].mArraySize);
               pEffect->SetParameter(handle, pData);
            else
               pEffect->SetMatrixF4x4A(handle, static_cast<const FXLFLOATA*>(pData));
            break;
         }
         case cIntrinsicTypeBool:
         {
            if (mEffectIntrinsicDesc[intrinsicIndex].mArraySize > 1)
               pEffect->SetScalarArrayB(handle, static_cast<const BOOL*>(pData), mEffectIntrinsicDesc[intrinsicIndex].mArraySize);
            else
               pEffect->SetScalarB(handle, static_cast<const BOOL*>(pData));
            break;
         }
         default:
         {
            // rg [5/29/06] - Man SetParameter() is buggy! It won't set BOOL params in 2732.
            pEffect->SetParameter(handle, pData);
            break;
         }
      }         
   }

public:   
   static BEffectIntrinsicDesc   mEffectIntrinsicDesc[cNumEffectIntrinsics];
   static WORD                   mIntrinsicTableOffsets[cNumEffectIntrinsics + 1];

   enum BSharedParamType
   {
      cSPTInvalid,
      
      cSPTBool,
      cSPTFloat,
      cSPTFloat3,
      cSPTFloat4,
      cSPTFloat4x4,

      cSPTNumTypes
   };

   struct BSharedParamDesc
   {
      DWORD                mContextFlags;
      BSharedParamType     mType;
      uchar                mFirstReg;
      uchar                mNumRegs;
      uchar                mArraySize;
   };

   static BSharedParamDesc       mSharedParams[cNumEffectIntrinsics];   
      
   static uint                   mFirstSharedFloatReg;
   static uint                   mLastSharedFloatReg;
};

//============================================================================
// class BEffectIntrinsicTable
//============================================================================
class BEffectIntrinsicTable
{
public:
   BEffectIntrinsicTable();
   BEffectIntrinsicTable(const BEffectIntrinsicTable& other);
   const BEffectIntrinsicTable& operator= (const BEffectIntrinsicTable& rhs);
   
   void clear(void);

   void set(eEffectIntrinsicIndex index, const void* pData, eEffectIntrinsicType type, uint num = 1);
   void setMatrix(eEffectIntrinsicIndex index, const XMMATRIX* pMatrices, uint num = 1);
   void get(eEffectIntrinsicIndex index, void* pData, eEffectIntrinsicType type, uint num = 1) const;
   const void* getPtr(eEffectIntrinsicIndex index) const;
   void* getPtr(eEffectIntrinsicIndex index);

   typedef BDynamicArray<uchar, 16> BIntrinsicTableData;
   const BIntrinsicTableData& getTableData(void) const  { return mIntrinsicTable; }
         BIntrinsicTableData& getTableData(void)        { return mIntrinsicTable; }

   uint getGenerationIndex(void) const { return mGenerationIndex; }
   void incGenerationIndex(void) { mGenerationIndex++; }
      
   uint getStateSnapshotBufSize(void) const;
   void createStateSnapshot(void* pDst, uint bufSize);
   void applyStateSnapshot(const void* pSrc, uint bufSize);

private:
   BIntrinsicTableData     mIntrinsicTable;

   uint                    mGenerationIndex;
};

//============================================================================
// class BEffectIntrinsicPool
//============================================================================
class BFXLEffectIntrinsicPool : public BFXLEffectPool, public BEffectIntrinsicTable
{
public:
   BFXLEffectIntrinsicPool();
   BFXLEffectIntrinsicPool(const BFXLEffectIntrinsicPool& other) : BEffectIntrinsicTable(other) { mpDevice = other.mpDevice; }
   ~BFXLEffectIntrinsicPool();
      
   void create(IDirect3DDevice9* pDevice = NULL);
   void clear();
   
   const BEffectIntrinsicTable& getTable(void) const  { return *this; }
         BEffectIntrinsicTable& getTable(void)        { return *this; }
   
   IDirect3DDevice9* getDevice(void) const { return mpDevice; }
   void setDevice(IDirect3DDevice9* pDevice) { mpDevice = pDevice; }
      
   // Must call after at least one effect uses to this pool.
   void initSharedParams(BFXLEffect* pEffect);
         
   void set(eEffectIntrinsicIndex index, const void* pData, eEffectIntrinsicType type, uint num = 1);
   void setMatrix(eEffectIntrinsicIndex index, const XMMATRIX* pMatrices, uint num = 1);
         
   void update(eEffectIntrinsicIndex index);
   void update(eEffectIntrinsicIndex firstIndex, eEffectIntrinsicIndex lastIndex);
   void updateAll(void);
   void updateAllBool(void);

   void applyStateSnapshot(const void* pSrc, uint bufSize) { BEffectIntrinsicTable::applyStateSnapshot(pSrc, bufSize); updateAll(); }
   
   const BFXLEffectIntrinsicPool& operator= (const BEffectIntrinsicTable& rhs) { BEffectIntrinsicTable::operator=(rhs); updateAll(); return *this; }
   const BFXLEffectIntrinsicPool& operator= (const BFXLEffectIntrinsicPool& rhs) { BEffectIntrinsicTable::operator=(rhs); mpDevice = rhs.mpDevice; updateAll(); return *this; }
           
private:
   IDirect3DDevice9*       mpDevice;
};

//============================================================================
// class BEffectIntrinsicManager
//============================================================================
class BEffectIntrinsicManager : public BRenderCommandListener
{
   friend class BEffectIntrinsicMapper;

public:
   BEffectIntrinsicManager();
   ~BEffectIntrinsicManager();

   // sim thread   
   void init(void);

   // sim thread
   void deinit(void);

   // Sets the caller thread's copy of the intrinsic. 
   // Shared params are also updated if called from the render thread.
   // Matrices must not be transposed (i.e. they should be normal BMatrix/D3DX/XMMATRIX's, FX light will transpose for you).
   // Callable from the sim/render threads.
   void set(eEffectIntrinsicIndex index, const void* pData, eEffectIntrinsicType type, uint num = 1, bool updateRenderThreadFlag = true);
   void setMatrix(eEffectIntrinsicIndex index, const XMMATRIX* pMatrices, uint num = 1, bool updateRenderThreadFlag = true);

   // sim/render threads
   void get(eEffectIntrinsicIndex index, void* pData, eEffectIntrinsicType type, uint num = 1) const;
   const void* getPtr(eEffectIntrinsicIndex index) const;
   void* getPtr(eEffectIntrinsicIndex index);
   
   uint getGenerationIndex(void) const;

   // Submits a command to update the render thread's intrinsic table. 
   // A copy of the sim thread's intrinsic table will be placed into the command buffer.
   // Callable from the sim thread only.
   void updateRenderThread(eEffectIntrinsicIndex firstIndex = (eEffectIntrinsicIndex)0, eEffectIntrinsicIndex lastIndex = (eEffectIntrinsicIndex)(cNumEffectIntrinsics - 1), bool force = false);
   
   // Resends all intrinsics to the D3D device. Call this when you are unsure if they have been overwritten (like by 3rd party libraries).
   // render thread
   void updateAllSharedIntrinsics(void);
   void updateAllSharedBoolIntrinsics(void);
   
   void getSharedFloatIntrinsicRegisterRange(uint& firstReg, uint& numRegs);
   
   // sim thread   
   BEffectIntrinsicTable& getSimIntrinsicTable(void) { return mSimIntrinsicTable; }
   
   // render thread
   BEffectIntrinsicTable& getRenderIntrinsicTable(void) { return mRenderEffectIntrinsicPool; }
   
   // render thread
   BFXLEffectIntrinsicPool& getRenderEffectIntrinsicPool(void) { return mRenderEffectIntrinsicPool; }
                     
private:
   BEffectIntrinsicTable                           mSimIntrinsicTable;
   BFXLEffectIntrinsicPool                         mRenderEffectIntrinsicPool;

   uint                                            mPrevSubmittedGenerationIndex;
   
   enum eCommands
   {
      cCommandUpdateIntrinsics,
   };
   
   struct BUpdateIntrinsicsData
   {
      DWORD mFirstIndex;
      DWORD mLastIndex;
   };
         
   virtual void initDeviceData(void);
   virtual void frameBegin(void);
   virtual void processCommand(const BRenderCommandHeader& header, const uchar* pData);
   virtual void frameEnd(void);
   virtual void deinitDeviceData(void);
};

//============================================================================
// externs
//============================================================================
extern BEffectIntrinsicManager gEffectIntrinsicManager;
