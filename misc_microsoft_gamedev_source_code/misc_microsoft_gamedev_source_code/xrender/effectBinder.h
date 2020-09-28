//============================================================================
//
//  effectBinder.h
//  
//  Copyright (c) 2008, Ensemble Studios
//
//============================================================================
#pragma once
#include "effectIntrinsicManager.h"
#include "containers\hashMap.h"
#include "renderStateFilter.h"

//============================================================================
// class BBoundEffectPassDesc
//============================================================================
class BBoundEffectPassDesc
{
public:
   BBoundEffectPassDesc(BFXLEffectPass& pass, IDirect3DVertexDeclaration9* pDecl, uint streamStrides, bool bindVertexToPixelShaders) : 
      mPass(pass), 
      mpDecl(pDecl), 
      mStreamStrides(streamStrides), 
      mBindVertexToPixelShaders(bindVertexToPixelShaders) 
   { 
   }
   
   BBoundEffectPassDesc(const BBoundEffectPassDesc& other) : 
      mPass(other.mPass), 
      mpDecl(other.mpDecl), 
      mStreamStrides(other.mStreamStrides), 
      mBindVertexToPixelShaders(other.mBindVertexToPixelShaders) 
   {
   }
   
   BBoundEffectPassDesc& operator= (const BBoundEffectPassDesc& rhs) 
   { 
      mPass = rhs.mPass; 
      mpDecl = rhs.mpDecl; 
      mStreamStrides = rhs.mStreamStrides; 
      mBindVertexToPixelShaders = rhs.mBindVertexToPixelShaders;
   }
         
   BFXLEffectPass&               getPass(void) const { return mPass; }
   IDirect3DVertexDeclaration9*  getDecl(void) const { return mpDecl; }
   uint                          getStreamStrides(void) const { return mStreamStrides; }
   BOOL                          getBindVertexToPixelShaders(void) const { return mBindVertexToPixelShaders; }
   
   // This is a bitwise hash!
   size_t hash(void) const { return hashFast(this, sizeof(*this), 0); }
   
   bool operator== (const BBoundEffectPassDesc& other) const 
   { 
      return (mPass == other.mPass) && (mpDecl == other.mpDecl) && (mStreamStrides == other.mStreamStrides) && (mBindVertexToPixelShaders == other.mBindVertexToPixelShaders);
   }
                           
private:
   mutable BFXLEffectPass        mPass;
   IDirect3DVertexDeclaration9*  mpDecl;
   uint                          mStreamStrides;
   BOOL                          mBindVertexToPixelShaders;
};

//============================================================================
// class BBoundEffectPassData
//============================================================================
class BBoundEffectPassData
{
public:
   BBoundEffectPassData();
   BBoundEffectPassData(const BBoundEffectPassData& other);
   ~BBoundEffectPassData();
   
   BBoundEffectPassData& operator= (const BBoundEffectPassData& rhs);
   
   void clear(void);
   
   // Warning: If bindVertexToPixelShaders is false, Your pixel shader inputs MUST match the vertex shader's outputs!
   void init(BFXLEffect& effect, const BBoundEffectPassDesc& desc, bool bindVertexToPixelShaders);

   void setToDevice(IDirect3DDevice9* pDev, const BEffectIntrinsicTable& intrinsicTable);
   void setToDevice(BRenderStateFilter& renderStateFilter, const BEffectIntrinsicTable& intrinsicTable);
   
private:
   IDirect3DVertexShader9*                         mpVertexShader;
   IDirect3DPixelShader9*                          mpPixelShader;
   
   struct BRenderState 
   {
      BRenderState() { }
      BRenderState(D3DRENDERSTATETYPE state, DWORD value) : mState(state), mValue(value) { }
      
      D3DRENDERSTATETYPE mState;
      DWORD mValue;
   };
   BSmallDynamicRenderArray<BRenderState>          mRenderStates;
   
   struct BSamplerState
   {
      BSamplerState() { }
      BSamplerState(uint sampler, D3DSAMPLERSTATETYPE state, DWORD value) : mSampler(sampler), mState(state), mValue(value) { }
      
      uint mSampler;
      D3DSAMPLERSTATETYPE mState;
      DWORD mValue;
   };
   BSmallDynamicRenderArray<BSamplerState>         mSamplerStates;

   struct BIntrinsic
   {
      BIntrinsic() { }
      BIntrinsic(uint i, uint r, uint c) : mIntrinsicIndex((uchar)i), mRegIndex((uchar)r), mContext((uchar)c) { }
      
      uchar                      mIntrinsicIndex;
      uchar                      mRegIndex;
      uchar                      mContext;         // actually a FXLPARAMETER_CONTEXT
   };

   BSmallDynamicRenderArray<BIntrinsic>    mIntrinsics;
};

//============================================================================
// class BBoundEffectManager
//============================================================================
class BBoundEffectManager
{
   BBoundEffectManager(const BBoundEffectManager&);
   BBoundEffectManager& operator= (const BBoundEffectManager&);
   
public:
   BBoundEffectManager(uint maxExpectedBoundEffects = 512);
   ~BBoundEffectManager();
   
   void clear(void);
   
   // Warning: If bindVertexToPixelShaders is false, Your pixel shader inputs MUST match the vertex shader's outputs!
   BBoundEffectPassData* get(
      BFXLEffect& effect, 
      BFXLEffectPass& pass, 
      IDirect3DVertexDeclaration9* pDecl, 
      uint stream0Stride, uint stream1Stride,
      bool bindVertexToPixelShaders);
   
private:
   typedef BHashMap<
      BBoundEffectPassDesc, 
      BBoundEffectPassData, 
      BIntrusiveHasher<BBoundEffectPassDesc>, 
      BEqualTo<BBoundEffectPassDesc>, 
      true, 
      BRenderFixedHeapAllocator> BBoundEffectHashMap;
   
   BBoundEffectHashMap mBoundEffects;
};

extern BBoundEffectManager gBoundEffectManager;


