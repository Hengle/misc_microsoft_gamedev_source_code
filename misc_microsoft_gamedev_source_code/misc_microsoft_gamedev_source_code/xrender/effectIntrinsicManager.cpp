//============================================================================
// EffectIntrinsicManager.cpp
//
// Copyright (c) 2006 Ensemble Studios
//
// rg
//
//============================================================================
#include "xrender.h"
#include "effectIntrinsicManager.h"
#include "D3DTextureManager.h"

//============================================================================
// Globals
//============================================================================
BEffectIntrinsics gEffectIntrinsics;
BEffectIntrinsicManager gEffectIntrinsicManager;

//============================================================================
// class BEffectIntrinsics
//============================================================================
#define DECLARE_EFFECT_INTRINSIC(name, type, arraySize, shared) { #name, type, arraySize, shared },

BEffectIntrinsicDesc BEffectIntrinsics::mEffectIntrinsicDesc[cNumEffectIntrinsics] = 
{
   #include "effectIntrinsicDefs.h"
};

#undef DECLARE_EFFECT_INTRINSIC

WORD BEffectIntrinsics::mIntrinsicTableOffsets[cNumEffectIntrinsics + 1];
BEffectIntrinsics::BSharedParamDesc BEffectIntrinsics::mSharedParams[cNumEffectIntrinsics];

uint BEffectIntrinsics::mFirstSharedFloatReg = UINT_MAX;
uint BEffectIntrinsics::mLastSharedFloatReg;

//============================================================================
// BEffectIntrinsics::init
//============================================================================
void BEffectIntrinsics::init(void)
{
   if (mIntrinsicTableOffsets[1])
      return;
      
   uint offset = 0;

   for (uint i = 0; i < cNumEffectIntrinsics; i++)
   {
      const uint alignment = getTypeAlignment(BEffectIntrinsics::mEffectIntrinsicDesc[i].mType);
      const uint size = getTypeSize(BEffectIntrinsics::mEffectIntrinsicDesc[i].mType, BEffectIntrinsics::mEffectIntrinsicDesc[i].mArraySize);

      offset = Utils::AlignUpValue(offset, alignment);
      mIntrinsicTableOffsets[i] = static_cast<WORD>(offset);

      offset += size;
   }

   mIntrinsicTableOffsets[cNumEffectIntrinsics] = static_cast<WORD>(offset);
}

//============================================================================
// BEffectIntrinsics::initSharedParams
//============================================================================
void BEffectIntrinsics::initSharedParams(BFXLEffect* pEffect)
{
   BDEBUG_ASSERT(pEffect);
         
   for (uint index = 0; index < cNumEffectIntrinsics; index++)
   {
      const BEffectIntrinsicDesc& desc = BEffectIntrinsics::getDesc(static_cast<eEffectIntrinsicIndex>(index));
      if (!desc.mShared)
         continue;

      BFixedString64 name("g");
      name += BFixedString64(desc.mpName);

      BFXLEffectParam param((*pEffect)(name));
      if (!param.getValid())
      {
         // If this fails it means the effect intrinsic names in effectIntrinsicDefs.h didn't match the shared params defined in intrinsics.inc!
         BFATAL_FAIL("Failed getting handle of shared parameter!");
      }

      const bool validated = validateEffectParam((eEffectIntrinsicIndex)index, param);

      if (!validated)
      {
         trace("Unable to validate parameter \"%s\"!", name.getPtr());
         continue;
      }
      param.setRegisterUpdateMode(true);
                  
      FXLPARAMETER_DESC paramDesc;
      param.getDesc(paramDesc);
      
      if (paramDesc.Content == FXLPACONTENT_STRUCT)
      {
         BFATAL_FAIL("shared structs unsupported");
      }
      
      FXLHANDLE firstElementHandle = param.getHandle();
      uint arraySize = 1;
      if (paramDesc.Content == FXLPACONTENT_ARRAY)
      {
         firstElementHandle = pEffect->getEffect()->GetElementHandle(param.getHandle(), 0);
         arraySize = paramDesc.Elements;
      }
      
      FXLPARAMETER_DESC elementDesc;
      pEffect->getEffect()->GetParameterDesc(firstElementHandle, &elementDesc);
      
      if (elementDesc.Content != FXLPACONTENT_DATA)
         BFATAL_FAIL("First element must be DATA");
      
      BSharedParamType sharedParamType = cSPTInvalid;
      
      switch (elementDesc.Type)
      {
         case FXLDTYPE_FLOAT:
         {
            if (elementDesc.Class == FXLDCLASS_SCALAR)
            {
               if ((elementDesc.Rows != 1) || (elementDesc.Columns != 1))
                  BFATAL_FAIL("Unsupported rows/cols");
               if (elementDesc.Size != sizeof(float))
                  BFATAL_FAIL("Unsupported size");  
               sharedParamType = cSPTFloat;
            }
            else if (elementDesc.Class == FXLDCLASS_VECTOR)
            {
               if (elementDesc.Rows != 1)
                  BFATAL_FAIL("Unsupported rows");
               
               if (elementDesc.Columns == 3)
                  sharedParamType = cSPTFloat3;
               else if (elementDesc.Columns == 4)
                  sharedParamType = cSPTFloat4;
               else
                  BFATAL_FAIL("Unsupported cols");
                  
               if (elementDesc.Size != sizeof(float) * elementDesc.Columns)
                  BFATAL_FAIL("Unsupported size");  
            }
            else if (elementDesc.Class == FXLDCLASS_CMATRIX)
            {
               if ((elementDesc.Columns != 4) || (elementDesc.Rows != 4))
                  BFATAL_FAIL("Unsupported columns/rows");
               
               if (elementDesc.Size != sizeof(XMMATRIX))
                  BFATAL_FAIL("Unsupported size");
               
               sharedParamType = cSPTFloat4x4;
            }
            else
               BFATAL_FAIL("Unsupported class");
               
            break;
         }
         case FXLDTYPE_BOOL:
         {
            if (elementDesc.Class != FXLDCLASS_SCALAR)
               BFATAL_FAIL("Unsupported class");
            if ((elementDesc.Rows != 1) || (elementDesc.Columns != 1))
               BFATAL_FAIL("Unsupported rows/cols");
            if (elementDesc.Size != sizeof(BOOL))
               BFATAL_FAIL("Unsupported size");
            sharedParamType = cSPTBool;
            break;
         }
         default:
            BFATAL_FAIL("Unsupported type");
      }         
               
      UINT regIndex = 0;
      UINT regCount = 0;
      UINT contextFlags = 0;
      
      for (uint techniqueIndex = 0; techniqueIndex < pEffect->getNumTechniques(); techniqueIndex++)
      {
         BFXLEffectTechnique technique(pEffect->getTechniqueFromIndex(techniqueIndex));
         
         for (uint passIndex = 0; passIndex < technique.getNumPasses(); passIndex++)
         {
            BFXLEffectPass pass(technique.getPassFromIndex(passIndex));

            UINT flags = 1;
            while (flags < 256)
            {
               UINT tempRegIndex = 0;
               UINT tempRegCount = 0;
               
               pEffect->getEffect()->GetParameterRegister(pass.getHandle(), firstElementHandle, (FXLPARAMETER_CONTEXT)flags, &tempRegIndex, &tempRegCount);
                                 
               if (tempRegIndex != 0)
               {
                  contextFlags |= flags;
                  
                  if (regIndex == 0)
                  {
                     regIndex = tempRegIndex;
                     regCount = tempRegCount;
                  }
                  else 
                  {
                     if (regIndex != tempRegIndex)
                     {
                        BFATAL_FAIL("Inconsistent shared param register usage");
                     }
                     
                     regCount = Math::Max(regCount, tempRegCount);
                  }
               }
               flags <<= 1;
            }               
         }                  
      }
                              
      if (contextFlags)     
      {
         regCount *= arraySize;

         if (elementDesc.Type == FXLDTYPE_BOOL)
         {
            if (contextFlags & (FXLPCONTEXT_VERTEXSHADERCONSTANTF|FXLPCONTEXT_PIXELSHADERCONSTANTF|FXLPCONTEXT_VERTEXSHADERCONSTANTI|FXLPCONTEXT_PIXELSHADERCONSTANTI))
               BFATAL_FAIL("Bool param mapped to unsupported reg set");
         }
         else if (elementDesc.Type == FXLDTYPE_FLOAT)
         {
            if (contextFlags & (FXLPCONTEXT_VERTEXSHADERCONSTANTB|FXLPCONTEXT_PIXELSHADERCONSTANTB|FXLPCONTEXT_VERTEXSHADERCONSTANTI|FXLPCONTEXT_PIXELSHADERCONSTANTI))
               BFATAL_FAIL("Float param mapped to unsupported reg set");
         }
         
         if (mSharedParams[index].mType != cSPTInvalid)
         {
            if ((mSharedParams[index].mType != sharedParamType) || (mSharedParams[index].mFirstReg != regIndex) || (mSharedParams[index].mArraySize != arraySize))
               BFATAL_FAIL("Inconsistent shared params");
            
            mSharedParams[index].mNumRegs = static_cast<uchar>(Math::Max<uint>(mSharedParams[index].mNumRegs, regCount));
            mSharedParams[index].mContextFlags |= contextFlags;
         }
         else
         {
            mSharedParams[index].mType = sharedParamType;
            mSharedParams[index].mFirstReg = static_cast<uchar>(regIndex);
            mSharedParams[index].mNumRegs = static_cast<uchar>(regCount);
            mSharedParams[index].mContextFlags = contextFlags;
            mSharedParams[index].mArraySize = static_cast<uchar>(arraySize);
         }
      }         
   }
   
   for (uint i = 0; i < cNumEffectIntrinsics; i++)
   {
      switch (mSharedParams[i].mType)
      {
         case cSPTFloat:
         case cSPTFloat3:
         case cSPTFloat4:
         case cSPTFloat4x4:
         {
            mFirstSharedFloatReg = Math::Min<uint>(mFirstSharedFloatReg, mSharedParams[i].mFirstReg);
            mLastSharedFloatReg = Math::Max<uint>(mLastSharedFloatReg, mSharedParams[i].mFirstReg + mSharedParams[i].mNumRegs);
            break;
         }
      }
   }
}

//============================================================================
// BEffectIntrinsics::getIntrinsicSize
//============================================================================
uint BEffectIntrinsics::getIntrinsicSize(eEffectIntrinsicIndex index)  
{ 
   BDEBUG_ASSERT(index < cNumEffectIntrinsics); 

   return getTypeSize(mEffectIntrinsicDesc[index].mType, mEffectIntrinsicDesc[index].mArraySize); 
}

//============================================================================
// BEffectIntrinsicManager::getTypeSize
// any thread
//============================================================================
uint BEffectIntrinsics::getTypeSize(eEffectIntrinsicType type, uint num)
{
   switch (type)
   {
      case cIntrinsicTypeBool:            return num * sizeof(BOOL);
      case cIntrinsicTypeFloat:           return num * sizeof(float);
      case cIntrinsicTypeInt4:            return num * sizeof(int) * 4;
      case cIntrinsicTypeFloat2:          return num * sizeof(BVec2);
      case cIntrinsicTypeFloat3:          return num * sizeof(BVec3);
      case cIntrinsicTypeFloat4:          return num * sizeof(XMVECTOR);
      case cIntrinsicTypeFloat4x4:        return num * sizeof(XMMATRIX);
      case cIntrinsicTypeTexturePtr:      return num * sizeof(IDirect3DTexture9*);
      case cIntrinsicTypeTextureHandle:   return num * sizeof(BManagedTextureHandle); 
      default:
         BDEBUG_ASSERT(0);
   }

   return 0;      
}

//============================================================================
// BEffectIntrinsicManager::getTypeAlignment
// any thread
//============================================================================
uint BEffectIntrinsics::getTypeAlignment(eEffectIntrinsicType type)
{
   switch (type)
   {
      case cIntrinsicTypeFloat4:          
      case cIntrinsicTypeFloat4x4:        
         return 16;
   }
   return 4;      
}

//============================================================================
// BEffectIntrinsicManager::findIndex 
// any thread
//============================================================================
eEffectIntrinsicIndex BEffectIntrinsics::findIndex(const char* pName)
{
   for (uint i = 0; i < cNumEffectIntrinsics; i++)
      if (stricmp(pName, mEffectIntrinsicDesc[i].mpName) == 0)
         return static_cast<eEffectIntrinsicIndex>(i);
   return cInvalidIntrinsicIndex;
}

//============================================================================
// BEffectIntrinsicTable::BEffectIntrinsicTable
//============================================================================
BEffectIntrinsicTable::BEffectIntrinsicTable() : 
   mGenerationIndex(UINT_MAX)
{
   BEffectIntrinsics::init();
   
   clear();
}

//============================================================================
// BEffectIntrinsicTable::BEffectIntrinsicTable
//============================================================================
BEffectIntrinsicTable::BEffectIntrinsicTable(const BEffectIntrinsicTable& other)
{
   mIntrinsicTable = other.mIntrinsicTable;
   mGenerationIndex++;
}

//============================================================================
// BEffectIntrinsicTable::operator=
//============================================================================
const BEffectIntrinsicTable& BEffectIntrinsicTable::operator= (const BEffectIntrinsicTable& rhs)
{
   mIntrinsicTable.assignNoDealloc(rhs.mIntrinsicTable);
   mGenerationIndex++;
   return *this;
}

//============================================================================
// BEffectIntrinsicTable::clear
//============================================================================
void BEffectIntrinsicTable::clear(void)
{
   mIntrinsicTable.resize(BEffectIntrinsics::getIntrinsicTableSize());
   
   mIntrinsicTable.setAll(0);
   
   const BMatrix44 identity(BMatrix44::makeIdentity());
   BManagedTextureHandle defaultTextureHandle = cInvalidManagedTextureHandle;

   for (uint index = 0; index < cNumEffectIntrinsics; index++)
   {
      void* pDst = &mIntrinsicTable[BEffectIntrinsics::mIntrinsicTableOffsets[index]];

      switch (BEffectIntrinsics::mEffectIntrinsicDesc[index].mType)
      {
         case cIntrinsicTypeFloat4x4:
         {
            for (uint i = 0; i < BEffectIntrinsics::mEffectIntrinsicDesc[index].mArraySize; i++)
               memcpy((BMatrix44*)pDst + i, &identity, sizeof(BMatrix44));
            break;
         }
         case cIntrinsicTypeTextureHandle:
         {
            BDEBUG_ASSERT(BEffectIntrinsics::mEffectIntrinsicDesc[index].mArraySize == 1);
            memcpy(pDst, &defaultTextureHandle, sizeof(BManagedTextureHandle));
            break;
         }
      }
   }
}

//============================================================================
// BEffectIntrinsicTable::set
//============================================================================
void BEffectIntrinsicTable::set(eEffectIntrinsicIndex index, const void* pData, eEffectIntrinsicType type, uint num)
{
   BDEBUG_ASSERT((num > 0) && (index >= 0) && (index < cNumEffectIntrinsics) && (pData));
   BDEBUG_ASSERT((BEffectIntrinsics::mEffectIntrinsicDesc[index].mType == type) && (BEffectIntrinsics::mEffectIntrinsicDesc[index].mArraySize == num));

   const uint size = BEffectIntrinsics::getTypeSize(BEffectIntrinsics::mEffectIntrinsicDesc[index].mType, num);

   memcpy(&mIntrinsicTable[BEffectIntrinsics::mIntrinsicTableOffsets[index]], pData, size);

   mGenerationIndex++;
}

//============================================================================
// BFXLEffectIntrinsicPool::setMatrix
//============================================================================
void BEffectIntrinsicTable::setMatrix(eEffectIntrinsicIndex index, const XMMATRIX* pMatrices, uint num)
{
   set(index, pMatrices, cIntrinsicTypeFloat4x4, num);      
}

//============================================================================
// BFXLEffectIntrinsicPool::get
// sim/render threads
//============================================================================
void BEffectIntrinsicTable::get(eEffectIntrinsicIndex index, void* pData, eEffectIntrinsicType type, uint num) const
{
   BDEBUG_ASSERT((index >= 0) && (index < cNumEffectIntrinsics) && (pData));
   BDEBUG_ASSERT(BEffectIntrinsics::mEffectIntrinsicDesc[index].mType == type);
   BDEBUG_ASSERT(BEffectIntrinsics::mEffectIntrinsicDesc[index].mArraySize == num);

   memcpy(pData, 
      &mIntrinsicTable[BEffectIntrinsics::mIntrinsicTableOffsets[index]], 
      BEffectIntrinsics::getTypeSize(BEffectIntrinsics::mEffectIntrinsicDesc[index].mType, BEffectIntrinsics::mEffectIntrinsicDesc[index].mArraySize));
}

//============================================================================
// BFXLEffectIntrinsicPool::getPtr
//============================================================================
const void* BEffectIntrinsicTable::getPtr(eEffectIntrinsicIndex index) const
{
   BDEBUG_ASSERT((index >= 0) && (index < cNumEffectIntrinsics));
   return &mIntrinsicTable[BEffectIntrinsics::mIntrinsicTableOffsets[index]];
}

//============================================================================
// BFXLEffectIntrinsicPool::getPtr
//============================================================================
void* BEffectIntrinsicTable::getPtr(eEffectIntrinsicIndex index) 
{
   BDEBUG_ASSERT((index >= 0) && (index < cNumEffectIntrinsics));
   return &mIntrinsicTable[BEffectIntrinsics::mIntrinsicTableOffsets[index]];
}

//============================================================================
// BEffectIntrinsicTable::getStateSnapshotBufSize
//============================================================================
uint BEffectIntrinsicTable::getStateSnapshotBufSize(void) const
{
   return sizeof(DWORD) * 2 + BEffectIntrinsics::getIntrinsicTableSize();
}

const DWORD cStateSnapshotStartMarker = 0xAABC9375;
const DWORD cStateSnapshotEndMarker = 0x77771234;

//============================================================================
// BEffectIntrinsicTable::createStateSnapshot
//============================================================================
void BEffectIntrinsicTable::createStateSnapshot(void* pDst, uint bufSize)
{
   BDEBUG_ASSERT(bufSize == getStateSnapshotBufSize());

   DWORD* pFirstDWORD = reinterpret_cast<DWORD*>(pDst);
   pFirstDWORD[0] = cStateSnapshotStartMarker;

   const DWORD stateSize = BEffectIntrinsics::getIntrinsicTableSize();

   Utils::FastMemCpy(reinterpret_cast<uchar*>(pDst) + sizeof(DWORD), mIntrinsicTable.getPtr(), stateSize);

   DWORD* pLastDWORD = reinterpret_cast<DWORD*>(reinterpret_cast<uchar*>(pDst) + sizeof(DWORD) + stateSize);
   pLastDWORD[0] = cStateSnapshotEndMarker;      
}

//============================================================================
// BEffectIntrinsicTable::applyStateSnapshot
//============================================================================
void BEffectIntrinsicTable::applyStateSnapshot(const void* pSrc, uint bufSize)
{
   BDEBUG_ASSERT(bufSize > sizeof(DWORD) * 2);
   BDEBUG_ASSERT(*reinterpret_cast<const DWORD*>(pSrc) == cStateSnapshotStartMarker);
   BDEBUG_ASSERT(*reinterpret_cast<const DWORD*>(reinterpret_cast<const uchar*>(pSrc) + bufSize - sizeof(DWORD)) == cStateSnapshotEndMarker);

   const DWORD intrinsicTableSize = bufSize - sizeof(DWORD) * 2;
   BDEBUG_ASSERT(intrinsicTableSize == BEffectIntrinsics::getIntrinsicTableSize());
   mIntrinsicTable.resize(intrinsicTableSize);
   Utils::FastMemCpy(mIntrinsicTable.getPtr(), reinterpret_cast<const uchar*>(pSrc) + sizeof(DWORD), intrinsicTableSize);

   mGenerationIndex++;
}

//============================================================================
// BFXLEffectIntrinsicPool::BFXLEffectIntrinsicPool
//============================================================================
BFXLEffectIntrinsicPool::BFXLEffectIntrinsicPool() :
   BFXLEffectPool(),
   mpDevice(NULL)
{
}

//============================================================================
// BFXLEffectIntrinsicPool::~BFXLEffectIntrinsicPool
//============================================================================
BFXLEffectIntrinsicPool::~BFXLEffectIntrinsicPool()
{
}

//============================================================================
// BFXLEffectIntrinsicPool::clear
//============================================================================
void BFXLEffectIntrinsicPool::clear()
{
   BFXLEffectPool::clear();
   
   BEffectIntrinsicTable::clear();
         
   mpDevice = NULL;
}

//============================================================================
// BFXLEffectIntrinsicPool::BFXLEffectIntrinsicPool
//============================================================================
void BFXLEffectIntrinsicPool::create(IDirect3DDevice9* pDevice)
{
   clear();
   
   BFXLEffectPool::create();
   
   mpDevice = pDevice ? pDevice : BD3D::mpDev;
}

//============================================================================
// BFXLEffectIntrinsicPool::initSharedParams
// Must be called at least one time after an effect is created using this effect 
// pool.
//============================================================================
void BFXLEffectIntrinsicPool::initSharedParams(BFXLEffect* pEffect)
{
   BEffectIntrinsics::initSharedParams(pEffect);

   // FIXME: updateAll() was crashing because it's called on the render thread during the load while the flash background player owns D3D.
   if ((mpDevice != BD3D::mpDev) || (gRenderThread.getHasD3DOwnership()))
      updateAll();
}

//============================================================================
// BFXLEffectIntrinsicPool::set
//============================================================================
void BFXLEffectIntrinsicPool::set(eEffectIntrinsicIndex index, const void* pData, eEffectIntrinsicType type, uint num)
{
   BEffectIntrinsicTable::set(index, pData, type, num);
      
   update(index);
}

//============================================================================
// BFXLEffectIntrinsicPool::setMatrix
//============================================================================
void BFXLEffectIntrinsicPool::setMatrix(eEffectIntrinsicIndex index, const XMMATRIX* pMatrices, uint num)
{
   BEffectIntrinsicTable::setMatrix(index, pMatrices, num);
 
   update(index);
}

//============================================================================
// BFXLEffectIntrinsicPool::update
//============================================================================
void BFXLEffectIntrinsicPool::update(eEffectIntrinsicIndex index)
{
   BDEBUG_ASSERT((index >= 0) && (index < cNumEffectIntrinsics));

   const BEffectIntrinsics::BSharedParamDesc& desc = BEffectIntrinsics::mSharedParams[index];
   if (!desc.mContextFlags)
      return;
      
   const void* pSrc = getPtr(index);
   
   switch (desc.mType)
   {
      case BEffectIntrinsics::cSPTBool:
      {
         if (desc.mContextFlags & FXLPCONTEXT_VERTEXSHADERCONSTANTB)
            mpDevice->SetVertexShaderConstantB(desc.mFirstReg, static_cast<const BOOL*>(pSrc), desc.mNumRegs);
         if (desc.mContextFlags & FXLPCONTEXT_PIXELSHADERCONSTANTB)
            mpDevice->SetPixelShaderConstantB(desc.mFirstReg, static_cast<const BOOL*>(pSrc), desc.mNumRegs);
                     
         break;
      }
      case BEffectIntrinsics::cSPTFloat:
      {
         float vec[4];
         vec[1] = 0.0f;
         vec[2] = 0.0f;
         vec[3] = 0.0f;
         for (uint i = 0; i < desc.mNumRegs; i++)
         {
            vec[0] = static_cast<const float*>(pSrc)[i];

            if (desc.mContextFlags & FXLPCONTEXT_VERTEXSHADERCONSTANTF)
               mpDevice->SetVertexShaderConstantF(desc.mFirstReg, vec, 1);
            if (desc.mContextFlags & FXLPCONTEXT_PIXELSHADERCONSTANTF)
               mpDevice->SetPixelShaderConstantF(desc.mFirstReg, vec, 1);
         }
         break;
      }
      case BEffectIntrinsics::cSPTFloat3:
      {
         float vec[4];
         vec[3] = 0.0f;
         for (uint i = 0; i < desc.mNumRegs; i++)
         {
            vec[0] = static_cast<const float*>(pSrc)[i * 3 + 0];
            vec[1] = static_cast<const float*>(pSrc)[i * 3 + 1];
            vec[2] = static_cast<const float*>(pSrc)[i * 3 + 2];
            
            if (desc.mContextFlags & FXLPCONTEXT_VERTEXSHADERCONSTANTF)
               mpDevice->SetVertexShaderConstantF(desc.mFirstReg, vec, 1);
            if (desc.mContextFlags & FXLPCONTEXT_PIXELSHADERCONSTANTF)
               mpDevice->SetPixelShaderConstantF(desc.mFirstReg, vec, 1);
         }
         break;
      }
      case BEffectIntrinsics::cSPTFloat4:
      {
         if (desc.mContextFlags & FXLPCONTEXT_VERTEXSHADERCONSTANTF)
            mpDevice->SetVertexShaderConstantF(desc.mFirstReg, static_cast<const float*>(pSrc), desc.mNumRegs);
         if (desc.mContextFlags & FXLPCONTEXT_PIXELSHADERCONSTANTF)
            mpDevice->SetPixelShaderConstantF(desc.mFirstReg, static_cast<const float*>(pSrc), desc.mNumRegs);
         break;
      }
      case BEffectIntrinsics::cSPTFloat4x4:
      {
         const XMMATRIX* pSrcMatrices = static_cast<const XMMATRIX*>(pSrc);
         XMMATRIX temp;
         for (uint i = 0; i < desc.mArraySize; i++)
         {
            temp = XMMatrixTranspose(pSrcMatrices[i]);
            
            if (desc.mContextFlags & FXLPCONTEXT_VERTEXSHADERCONSTANTF)
               mpDevice->SetVertexShaderConstantF(desc.mFirstReg, (const float*)&temp, 4);
               
            if (desc.mContextFlags & FXLPCONTEXT_PIXELSHADERCONSTANTF)
               mpDevice->SetPixelShaderConstantF(desc.mFirstReg, (const float*)&temp, 4);
         }
         break;
      }
      default:
      {
         BDEBUG_ASSERT(0);
      }
   }
}

//============================================================================
// BFXLEffectIntrinsicPool::update
//============================================================================
void BFXLEffectIntrinsicPool::update(eEffectIntrinsicIndex firstIndex, eEffectIntrinsicIndex lastIndex)
{
   for (int i = firstIndex; i <= lastIndex; i++)
      update(static_cast<eEffectIntrinsicIndex>(i));
}

//============================================================================
// BFXLEffectIntrinsicPool::updateAll
//============================================================================
void BFXLEffectIntrinsicPool::updateAll(void)
{
   update(static_cast<eEffectIntrinsicIndex>(0), static_cast<eEffectIntrinsicIndex>(cNumEffectIntrinsics - 1));
}

//============================================================================
// BFXLEffectIntrinsicPool::updateAll
//============================================================================
void BFXLEffectIntrinsicPool::updateAllBool(void)
{
   for (int i = 0; i < cNumEffectIntrinsics; i++)
   {
      const BEffectIntrinsics::BSharedParamDesc& desc = BEffectIntrinsics::mSharedParams[i];
      if (!desc.mContextFlags)
         continue;
      if (desc.mType == BEffectIntrinsics::cSPTBool)
         update(static_cast<eEffectIntrinsicIndex>(i));
   }
}

//============================================================================
// BEffectIntrinsicMapper::BEffectIntrinsicMapper
//============================================================================
BEffectIntrinsicMapper::BEffectIntrinsicMapper() : 
   mPrevGenerationIndex(UINT_MAX),
   mpIntrinsicPool(&gEffectIntrinsicManager.getRenderEffectIntrinsicPool())
{
   BEffectIntrinsics::init();
}

//============================================================================
// BEffectIntrinsicMapper::clear
//============================================================================
void BEffectIntrinsicMapper::clear(void)
{
   mLinks.clear();

   mPrevGenerationIndex = UINT_MAX;
   
   mpIntrinsicPool = NULL;
}

//============================================================================
// BEffectIntrinsicMapper::init
//============================================================================
void BEffectIntrinsicMapper::init(BFXLEffect* pEffect, BFXLEffectIntrinsicPool* pPool, bool validateIntrinsics)
{
   clear();
   
   BDEBUG_ASSERT(pEffect);
   
   mpIntrinsicPool = pPool ? pPool : &gEffectIntrinsicManager.getRenderEffectIntrinsicPool();
   mpIntrinsicPool->initSharedParams(pEffect);
   
   FXLEffect* pFXLEffect = pEffect->getEffect();         
   if (!pFXLEffect)
      return;
      
   for (uint paramIndex = 0; paramIndex < pEffect->getNumParameters(); paramIndex++)
   {
      BFXLEffectParam param = pEffect->getParamFromIndex(paramIndex);
      if ((!param.getNumAnnotations()) || (param.isShared()))
         continue;

      FXLHANDLE annoHandle = pFXLEffect->GetAnnotationHandle(param.getHandle(), "intrinsic");
      if (annoHandle == cInvalidFXLHandle)
         continue;

      FXLANNOTATION_DESC annoDesc;
      pFXLEffect->GetAnnotationDesc(annoHandle, &annoDesc);

      if ((annoDesc.Content != FXLPACONTENT_DATA) || (annoDesc.Type != FXLDTYPE_STRING))
      {
         trace("ERROR: BEffectIntrinsicManager::setEffect: Param \"%s\" has a invalid intrinsic annotation!", param.getName());
         continue;
      }

      BStaticArray<char, 256> annotationBuf(annoDesc.Size);

      pFXLEffect->GetAnnotation(annoHandle, annotationBuf.getPtr());

      eEffectIntrinsicIndex intrinsicIndex = BEffectIntrinsics::findIndex(annotationBuf.getPtr());
      if (intrinsicIndex == cInvalidIntrinsicIndex)
      {
         trace("ERROR: BEffectIntrinsicManager::setEffect: Param \"%s\" has a invalid intrinsic annotation \"%s\"!", param.getName(), annotationBuf.getPtr());
         continue;
      }
      
      const bool validated = BEffectIntrinsics::validateEffectParam(intrinsicIndex, param);
      if (!validated)
      {
         trace("ERROR: BEffectIntrinsicManager::setEffect: Param \"%s\" has the wrong type!", param.getName());
         continue;
      }
                 
      bool found = false;
      
      // rg [6/7/06] - Yet another workaround for an FX Light bug
      if (!validateIntrinsics)
         found = true;
      else
      {
         if (!found)
         {
            // See if this parameter is actually used in any pass.
            for (uint techniqueIndex = 0; techniqueIndex < pEffect->getNumTechniques(); techniqueIndex++)
            {
               BFXLEffectTechnique technique(pEffect->getTechniqueFromIndex(techniqueIndex));
               
               for (uint passIndex = 0; passIndex < technique.getNumPasses(); passIndex++)
               {
                  BFXLEffectPass pass(technique.getPassFromIndex(passIndex));
                 
                  FXLPARAMETER_CONTEXT context = param.getContext(pass);
                  if (context)
                  {
                     found = true;
                     break;
                  }
                  
                  if (found)
                     break;
               }
               if (found)
                  break;
            }
         }
      }

      if (found)
         mLinks.pushBack(BParamToIntrinsicLink((uchar)paramIndex, (uchar)intrinsicIndex));
   }
}

//============================================================================
// BEffectIntrinsicMapper::apply
//============================================================================
void BEffectIntrinsicMapper::apply(BFXLEffect* pEffect, bool force)
{
   BDEBUG_ASSERT(pEffect);
      
   if (mLinks.isEmpty())
      return;

   FXLEffect* pFXLEffect = pEffect->getEffect();
   if (!pFXLEffect)
      return;
      
   const BEffectIntrinsicTable& table = *mpIntrinsicPool;
            
   const uint generationIndex = table.getGenerationIndex();
   if ((!force) && (mPrevGenerationIndex == generationIndex))
      return;
   mPrevGenerationIndex = generationIndex;

   for (uint linkIndex = 0; linkIndex < mLinks.size(); linkIndex++)
   {
      BEffectIntrinsics::updateEffectParam(
         pFXLEffect, 
         pFXLEffect->GetParameterHandleFromIndex(mLinks[linkIndex].mParamIndex), 
         static_cast<eEffectIntrinsicIndex>(mLinks[linkIndex].mIntrinsicIndex),
         table.getPtr(static_cast<eEffectIntrinsicIndex>(mLinks[linkIndex].mIntrinsicIndex)) );
   }
}

//============================================================================
// BEffectIntrinsicManager::BEffectIntrinsicManager
//============================================================================
BEffectIntrinsicManager::BEffectIntrinsicManager() :
   mPrevSubmittedGenerationIndex(0)
{
   BEffectIntrinsics::init();
}

//============================================================================
// BEffectIntrinsicManager::~BEffectIntrinsicManager
//============================================================================
BEffectIntrinsicManager::~BEffectIntrinsicManager()
{
}

//============================================================================
// BEffectIntrinsicManager::init
// sim thread   
//============================================================================
void BEffectIntrinsicManager::init(void)
{
   ASSERT_THREAD(cThreadIndexSim);

   if (cInvalidCommandListenerHandle != getCommandHandle())
      return;
   
   mPrevSubmittedGenerationIndex = UINT_MAX;

   mSimIntrinsicTable.clear();
      
   commandListenerInit();
}

//============================================================================
// BEffectIntrinsicManager::deinit
// sim thread
//============================================================================
void BEffectIntrinsicManager::deinit(void)
{
   ASSERT_THREAD(cThreadIndexSim);

   if (cInvalidCommandListenerHandle == getCommandHandle())
      return;

   commandListenerDeinit();
}

//============================================================================
// BEffectIntrinsicManager::set
//============================================================================
void BEffectIntrinsicManager::set(eEffectIntrinsicIndex index, const void* pData, eEffectIntrinsicType type, uint num, bool updateRenderThreadFlag)
{
   BDEBUG_ASSERT((num > 0) && (index >= 0) && (index < cNumEffectIntrinsics) && (pData));
   BDEBUG_ASSERT((BEffectIntrinsics::mEffectIntrinsicDesc[index].mType == type) && (BEffectIntrinsics::mEffectIntrinsicDesc[index].mArraySize == num));
         
   if (gRenderThread.isSimThread())
   {
      mSimIntrinsicTable.set(index, pData, type, num);
      
      if (updateRenderThreadFlag)
         updateRenderThread(index, index);
   }
   else 
   {
      ASSERT_THREAD(cThreadIndexRender);
      mRenderEffectIntrinsicPool.set(index, pData, type, num);
   }
}

//============================================================================
// BEffectIntrinsicManager::setMatrix
//============================================================================
void BEffectIntrinsicManager::setMatrix(eEffectIntrinsicIndex index, const XMMATRIX* pMatrices, uint num, bool updateRenderThreadFlag)
{
   set(index, pMatrices, cIntrinsicTypeFloat4x4, num, updateRenderThreadFlag);      
}

//============================================================================
// BEffectIntrinsicManager::get
// sim/render threads
//============================================================================
void BEffectIntrinsicManager::get(eEffectIntrinsicIndex index, void* pData, eEffectIntrinsicType type, uint num) const
{
   BDEBUG_ASSERT((index >= 0) && (index < cNumEffectIntrinsics) && (pData));
   BDEBUG_ASSERT(BEffectIntrinsics::mEffectIntrinsicDesc[index].mType == type);
   BDEBUG_ASSERT(BEffectIntrinsics::mEffectIntrinsicDesc[index].mArraySize == num);

   const BEffectIntrinsicTable& intrinsicTable = gRenderThread.isSimThread() ? mSimIntrinsicTable : mRenderEffectIntrinsicPool;

   intrinsicTable.get(index, pData, type, num);
}

//============================================================================
// BEffectIntrinsicManager::getPtr
//============================================================================
const void* BEffectIntrinsicManager::getPtr(eEffectIntrinsicIndex index) const
{
   BDEBUG_ASSERT((index >= 0) && (index < cNumEffectIntrinsics));
   
   const BEffectIntrinsicTable& intrinsicTable = gRenderThread.isSimThread() ? mSimIntrinsicTable : mRenderEffectIntrinsicPool;
 
   return intrinsicTable.getPtr(index);
}

//============================================================================
// BEffectIntrinsicManager::getPtr
//============================================================================
void* BEffectIntrinsicManager::getPtr(eEffectIntrinsicIndex index) 
{
   BDEBUG_ASSERT((index >= 0) && (index < cNumEffectIntrinsics));
   
   BEffectIntrinsicTable& intrinsicTable = gRenderThread.isSimThread() ? mSimIntrinsicTable : mRenderEffectIntrinsicPool;

   return intrinsicTable.getPtr(index);
}

//============================================================================
// BEffectIntrinsicManager::updateRenderThread
// Submits a command to update the render thread's intrinsic table. 
// A copy of the sim thread's intrinsic table will be placed into the command buffer.
// Callable from the sim thread only.
//============================================================================
void BEffectIntrinsicManager::updateRenderThread(eEffectIntrinsicIndex firstIndex, eEffectIntrinsicIndex lastIndex, bool force)
{
   ASSERT_THREAD(cThreadIndexSim);
   BDEBUG_ASSERT((firstIndex < cNumEffectIntrinsics) && (firstIndex <= lastIndex) && (lastIndex < cNumEffectIntrinsics));
   
   const bool wholeRange = (firstIndex == 0) && (lastIndex == (cNumEffectIntrinsics - 1));
   
   if ((!force) && (mSimIntrinsicTable.getGenerationIndex() == mPrevSubmittedGenerationIndex) && (wholeRange))
      return;
      
   const uint len = BEffectIntrinsics::getSizeOfIntrinsicRange(firstIndex, lastIndex);
   
   uchar* pDst = (uchar*)gRenderThread.submitCommandBegin(mCommandHandle, cCommandUpdateIntrinsics, sizeof(BUpdateIntrinsicsData) + len);
   
   BUpdateIntrinsicsData* pUpdateData = reinterpret_cast<BUpdateIntrinsicsData*>(pDst);
   pUpdateData->mFirstIndex = firstIndex;
   pUpdateData->mLastIndex = lastIndex;
   
   Utils::FastMemCpy(
      pDst + sizeof(BUpdateIntrinsicsData), 
      mSimIntrinsicTable.getTableData().getPtr() + BEffectIntrinsics::mIntrinsicTableOffsets[pUpdateData->mFirstIndex],
      len);
      
   gRenderThread.submitCommandEnd(sizeof(BUpdateIntrinsicsData) + len);
   
   if (wholeRange)
      mPrevSubmittedGenerationIndex = mSimIntrinsicTable.getGenerationIndex();
}

//============================================================================
// BEffectIntrinsicManager::updateAllSharedIntrinsics
//============================================================================
void BEffectIntrinsicManager::updateAllSharedIntrinsics(void)
{
   ASSERT_THREAD(cThreadIndexRender);
   
   mRenderEffectIntrinsicPool.updateAll();
}

//============================================================================
// BEffectIntrinsicManager::updateAllSharedIntrinsics
//============================================================================
void BEffectIntrinsicManager::updateAllSharedBoolIntrinsics(void)
{
   ASSERT_THREAD(cThreadIndexRender);

   mRenderEffectIntrinsicPool.updateAllBool();
}

//============================================================================
// BEffectIntrinsicManager::getSharedFloatIntrinsicRegisterRange
//============================================================================
void BEffectIntrinsicManager::getSharedFloatIntrinsicRegisterRange(uint& firstReg, uint& numRegs)
{
   firstReg = gEffectIntrinsics.mFirstSharedFloatReg;
   numRegs = gEffectIntrinsics.mLastSharedFloatReg - gEffectIntrinsics.mFirstSharedFloatReg;
}

//============================================================================
// BEffectIntrinsicManager::getGenerationIndex
// sim/render threads
//============================================================================
uint BEffectIntrinsicManager::getGenerationIndex(void) const
{
   return gRenderThread.isSimThread() ? mSimIntrinsicTable.getGenerationIndex() : mRenderEffectIntrinsicPool.getGenerationIndex();
}

//============================================================================
// BEffectIntrinsicManager::initDeviceData
//============================================================================
void BEffectIntrinsicManager::initDeviceData(void)
{
   mRenderEffectIntrinsicPool.create();
}

//============================================================================
// BEffectIntrinsicManager::frameBegin
//============================================================================
void BEffectIntrinsicManager::frameBegin(void)
{
}

//============================================================================
// BEffectIntrinsicManager::processCommand
//============================================================================
void BEffectIntrinsicManager::processCommand(const BRenderCommandHeader& header, const uchar* pData)
{
   switch (header.mType)
   {
      case cCommandUpdateIntrinsics:
      {
         const BUpdateIntrinsicsData* pUpdateData = reinterpret_cast<const BUpdateIntrinsicsData*>(pData);
      
         BEffectIntrinsicTable& table = mRenderEffectIntrinsicPool.getTable();
         BDEBUG_ASSERT(header.mLen >= sizeof(BUpdateIntrinsicsData) && (header.mLen <= sizeof(BUpdateIntrinsicsData) + table.getTableData().getSize()));
         BDEBUG_ASSERT((pUpdateData->mFirstIndex < cNumEffectIntrinsics) && (pUpdateData->mLastIndex < cNumEffectIntrinsics) && (pUpdateData->mLastIndex >= pUpdateData->mFirstIndex));
         
         uchar* pStart = table.getTableData().getPtr() + BEffectIntrinsics::mIntrinsicTableOffsets[pUpdateData->mFirstIndex];
         const uint len = BEffectIntrinsics::getSizeOfIntrinsicRange((eEffectIntrinsicIndex)pUpdateData->mFirstIndex, (eEffectIntrinsicIndex)pUpdateData->mLastIndex);
         
         BDEBUG_ASSERT(header.mLen == sizeof(BUpdateIntrinsicsData) + len);
         
         Utils::FastMemCpy(pStart, pData + sizeof(BUpdateIntrinsicsData), len);
         
         mRenderEffectIntrinsicPool.incGenerationIndex();
         
         mRenderEffectIntrinsicPool.update(static_cast<eEffectIntrinsicIndex>(pUpdateData->mFirstIndex), static_cast<eEffectIntrinsicIndex>(pUpdateData->mLastIndex));

         break;
      }
   }
}

//============================================================================
// BEffectIntrinsicManager::frameEnd
//============================================================================
void BEffectIntrinsicManager::frameEnd(void)
{
}

//============================================================================
// BEffectIntrinsicManager::deinitDeviceData
//============================================================================
void BEffectIntrinsicManager::deinitDeviceData(void)
{
   mSimIntrinsicTable.clear();   
   mRenderEffectIntrinsicPool.clear();
}
