//============================================================================
//
//  File: renderStateFilter.h
//  
//  Copyright (c) 2008, Ensemble Studios
//
//============================================================================
#pragma once

__forceinline BOOL GPU_GET_VERTEX_SHADER_CONSTANTB(GPUFLOW_CONSTANTS* pFlowConstants, DWORD Register)
{
   DWORD index = (Register + GPU_D3D_VERTEX_CONSTANTB_BASE) / 32;
   DWORD shift = (Register % 32);
   return (pFlowConstants->BooleanConstant[index] >> shift) & 1;
}

__forceinline BOOL GPU_GET_PIXEL_SHADER_CONSTANTB(GPUFLOW_CONSTANTS* pFlowConstants, DWORD Register)
{
   DWORD index = (Register + GPU_D3D_PIXEL_CONSTANTB_BASE) / 32;
   DWORD shift = (Register % 32);
   return (pFlowConstants->BooleanConstant[index] >> shift) & 1;
}

__forceinline void GPU_GET_VERTEX_SHADER_CONSTANTI(GPUFLOW_CONSTANTS* pFlowConstants, DWORD Register, int* pConstantData)
{
   DWORD index = Register + GPU_D3D_VERTEX_CONSTANTI_BASE;
   DWORD value = pFlowConstants->IntegerConstant[index];
   pConstantData[0] = value & 0xFF;
   pConstantData[1] = (value >> 8) & 0xFF;
   pConstantData[2] = (value >> 16) & 0xFF;
}

__forceinline void GPU_GET_PIXEL_SHADER_CONSTANTI(GPUFLOW_CONSTANTS* pFlowConstants, DWORD Register, int* pConstantData)
{
   DWORD index = Register + GPU_D3D_PIXEL_CONSTANTI_BASE;
   DWORD value = pFlowConstants->IntegerConstant[index];
   pConstantData[0] = value & 0xFF;
   pConstantData[1] = (value >> 8) & 0xFF;
   pConstantData[2] = (value >> 16) & 0xFF;
}

#pragma warning(push)
#pragma warning(disable:4324)
__declspec(align(16))
class BRenderStateFilter
{
   BRenderStateFilter(const BRenderStateFilter& other);
   BRenderStateFilter& operator= (const BRenderStateFilter& rhs);

public:
   BRenderStateFilter(IDirect3DDevice9* pDev, uint firstVSReg, uint numVSRegs, uint firstPSReg, uint numPSRegs);
   ~BRenderStateFilter();

   void gpuOwn(uint firstLiteralVSReg, uint numLiteralVSRegs, uint firstLiteralPSReg, uint numLiteralPSRegs);
   void gpuDisown(void);

   inline void setDevice(IDirect3DDevice9* pDev) { BDEBUG_ASSERT(!mGpuOwned); mpDev = pDev; }

   void resetDeferredState(void);
   void flushDeferredState(void);

   inline void setIndices(IDirect3DIndexBuffer9 *pIndexData) { mpDev->SetIndices(pIndexData); }

   inline void setStreamSource(UINT StreamNumber, IDirect3DVertexBuffer9 *pStreamData, UINT OffsetInBytes, UINT Stride) { mpDev->SetStreamSource(StreamNumber, pStreamData, OffsetInBytes, Stride); }

   inline void setTexture(DWORD Sampler, IDirect3DBaseTexture9* pTexture) 
   {
      BDEBUG_ASSERT(Sampler < cMaxD3DTextureSamplers);
#if 0
      IDirect3DBaseTexture9* pCurTexture;
      mpDev->GetTexture(Sampler, &pCurTexture);
      if (pCurTexture)
         pCurTexture->Release();
      if (pCurTexture != pTexture)
         mpDev->SetTexture(Sampler, pTexture);
#endif
#if 0
      // apparently not useful, because it overwrites a bunch of sampler states that would be need to be reset
      if ((pTexture) && (pTexture != mpTextures[Sampler]))
      {
         mpTextures[Sampler] = pTexture;
         mpDev->SetTextureFetchConstant(Sampler, pTexture);
      }
#endif      

      if (pTexture != mpTextures[Sampler])
      {
         mpTextures[Sampler] = pTexture;
         mpDev->SetTexture(Sampler, pTexture);
      }
   }
   
   inline void setRenderState(D3DRENDERSTATETYPE State, DWORD Value)
   {
      BDEBUG_ASSERT( ((State & 1) == 0) && (State < D3DRS_MAX) );
      if (Value != mRenderStates[State >> 1U])
      {
         mRenderStates[State >> 1U] = Value;
         mpDev->SetRenderState(State, Value);
      }
   }
   
   inline void getRenderState(D3DRENDERSTATETYPE State, DWORD* pValue)
   {
      BDEBUG_ASSERT( ((State & 1) == 0) && (State < D3DRS_MAX) );
      *pValue = mRenderStates[State >> 1U];
   }

   inline void setSamplerState(DWORD Sampler, D3DSAMPLERSTATETYPE Type, DWORD Value)
   {
      BDEBUG_ASSERT(Sampler < cMaxD3DTextureSamplers);
      BDEBUG_ASSERT( ((Type & 3) == 0) && (Type < D3DSAMP_MAX) );
      
      if (mSamplerStates[Sampler][Type >> 2U] != Value)
      {
         mSamplerStates[Sampler][Type >> 2U] = Value;
         mpDev->SetSamplerState(Sampler, Type, Value);
      }
   }

   inline void setVertexShader(IDirect3DVertexShader9* pVertexShader) { if (pVertexShader != mpVertexShader) { mpVertexShader = pVertexShader; mShadersDirty = true; } }
   inline void setPixelShader(IDirect3DPixelShader9* pPixelShader) { if (pPixelShader != mpPixelShader) { mpPixelShader = pPixelShader; mShadersDirty = true; } }

   inline void setVertexShaderConstantI(UINT RegisterIndex, CONST INT* pConstantData, UINT RegisterCount)
   {
      for (uint j = RegisterCount; j > 0; j--, RegisterIndex++, pConstantData += 4)
      {
         int curValues[3];
         GPU_GET_VERTEX_SHADER_CONSTANTI(&mFlowConstants, RegisterIndex, curValues);
         if ((pConstantData[0] != curValues[0]) || (pConstantData[1] != curValues[1]) || (pConstantData[2] != curValues[2]))
         {
            GPU_SET_VERTEX_SHADER_CONSTANTI(&mFlowConstants, RegisterIndex, pConstantData);
            mShadersDirty = true;
         }
      }
   }

   inline void setPixelShaderConstantI(UINT RegisterIndex, CONST INT* pConstantData, UINT RegisterCount)
   {
      for (uint j = RegisterCount; j > 0; j--, RegisterIndex++, pConstantData += 4)
      {
         int curValues[3];
         GPU_GET_PIXEL_SHADER_CONSTANTI(&mFlowConstants, RegisterIndex, curValues);
         if ((pConstantData[0] != curValues[0]) || (pConstantData[1] != curValues[1]) || (pConstantData[2] != curValues[2]))
         {
            GPU_SET_PIXEL_SHADER_CONSTANTI(&mFlowConstants, RegisterIndex, pConstantData);
            mShadersDirty = true;
         }
      }
   }

   inline void setVertexShaderConstantB(UINT StartRegister, CONST BOOL* pConstantData, UINT BoolCount)
   {
      for (uint j = BoolCount; j > 0; j--, StartRegister++, pConstantData++)
      {
         const BOOL newValue = *pConstantData;
         if (newValue != GPU_GET_VERTEX_SHADER_CONSTANTB(&mFlowConstants, StartRegister))
         {
            GPU_SET_VERTEX_SHADER_CONSTANTB(&mFlowConstants, StartRegister, newValue);
            mShadersDirty = true;
         }
      }
   }
   
   inline void setVertexShaderConstantB(UINT StartRegister, BOOL newValue)
   {
      if (newValue != GPU_GET_VERTEX_SHADER_CONSTANTB(&mFlowConstants, StartRegister))
      {
         GPU_SET_VERTEX_SHADER_CONSTANTB(&mFlowConstants, StartRegister, newValue);
         mShadersDirty = true;
      }
   }

   inline void setPixelShaderConstantB(UINT StartRegister, CONST BOOL* pConstantData, UINT BoolCount)
   {
      for (uint j = BoolCount; j > 0; j--, StartRegister++, pConstantData++)
      {
         const BOOL newValue = *pConstantData;
         if (newValue != GPU_GET_PIXEL_SHADER_CONSTANTB(&mFlowConstants, StartRegister))
         {
            GPU_SET_PIXEL_SHADER_CONSTANTB(&mFlowConstants, StartRegister, newValue);
            mShadersDirty = true;
         }
      }
   }
   
   inline void setPixelShaderConstantB(UINT StartRegister, BOOL newValue)
   {
      if (newValue != GPU_GET_PIXEL_SHADER_CONSTANTB(&mFlowConstants, StartRegister))
      {
         GPU_SET_PIXEL_SHADER_CONSTANTB(&mFlowConstants, StartRegister, newValue);
         mShadersDirty = true;
      }
   }

   void setVertexShaderConstantF(UINT StartRegister, CONST float* pConstantData, DWORD Vector4fCount);
   void setPixelShaderConstantF(UINT StartRegister, CONST float* pConstantData, DWORD Vector4fCount);
   void setZeroPixelShaderConstantF(UINT StartRegister, DWORD Vector4fCount);
   
   void setCommandBufferPredication(DWORD TilePredication, DWORD RunPredication);

private:
   IDirect3DDevice9*       mpDev;
   
   uint                    mFirstVSReg, mNumVSRegs;
   uint                    mFirstPSReg, mNumPSRegs;
   
   uint                    mFirstLiteralVSReg, mNumLiteralVSRegs;
   uint                    mFirstLiteralPSReg, mNumLiteralPSRegs;
   
   DWORD                   mCurRunPredication;

   XMVECTOR*               mpVSRegs;
   uint64                  mVSGroupDirtyFlags;
   void setVSGroupDirtyFlag(uint registerIndex){ BDEBUG_ASSERT((registerIndex >= mFirstVSReg) && (registerIndex < (mFirstVSReg + mNumVSRegs))); mVSGroupDirtyFlags |= Utils::BBitMasks::get64((registerIndex - mFirstVSReg) >> 2); }

   XMVECTOR*               mpPSRegs;
   uint64                  mPSGroupDirtyFlags;
   void setPSGroupDirtyFlag(uint registerIndex) { BDEBUG_ASSERT((registerIndex >= mFirstPSReg) && (registerIndex < (mFirstPSReg + mNumPSRegs))); mPSGroupDirtyFlags |= Utils::BBitMasks::get64((registerIndex - mFirstPSReg) >> 2); }

   IDirect3DVertexShader9* mpVertexShader;
   IDirect3DPixelShader9*  mpPixelShader;
   
   IDirect3DBaseTexture9*  mpTextures[cMaxD3DTextureSamplers];
   
   DWORD                   mRenderStates[D3DRS_MAX >> 1U];
   DWORD                   mSamplerStates[cMaxD3DTextureSamplers][D3DSAMP_MAX >> 2U];
   
   bool                    mShadersDirty;
   bool                    mGpuOwned;

   GPUFLOW_CONSTANTS       mFlowConstants;
};
#pragma warning(pop)