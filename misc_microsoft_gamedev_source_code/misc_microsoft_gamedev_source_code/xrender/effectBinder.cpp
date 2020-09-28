//============================================================================
//
//  effectBinder.cpp
//  
//  Copyright (c) 2008, Ensemble Studios
// 
//  rg
//
//============================================================================
#include "xrender.h"
#include "effectBinder.h"

//============================================================================
// Globals
//============================================================================
BBoundEffectManager gBoundEffectManager;

//============================================================================
// BBoundEffectPassData::BBoundEffectPassData
//============================================================================
BBoundEffectPassData::BBoundEffectPassData() : mpPixelShader(NULL), mpVertexShader(NULL)
{
}

//============================================================================
// BBoundEffectPassData::BBoundEffectPassData
//============================================================================
BBoundEffectPassData::BBoundEffectPassData(const BBoundEffectPassData& other)
{
   mpPixelShader = other.mpPixelShader;
   if (mpPixelShader)
      mpPixelShader->AddRef();
   
   mpVertexShader = other.mpVertexShader;
   if (mpVertexShader)
      mpVertexShader->AddRef();
   
   mIntrinsics = other.mIntrinsics;
}

//============================================================================
// BBoundEffectPassData::~BBoundEffectPassData
//============================================================================
BBoundEffectPassData::~BBoundEffectPassData()
{
   if (mpPixelShader)
      mpPixelShader->Release();
      
   if (mpVertexShader)
      mpVertexShader->Release();
}

//============================================================================
// BBoundEffectPassData::operator=
//============================================================================
BBoundEffectPassData& BBoundEffectPassData::operator= (const BBoundEffectPassData& rhs)
{
   clear();
   
   mpPixelShader = rhs.mpPixelShader;
   if (mpPixelShader)
      mpPixelShader->AddRef();

   mpVertexShader = rhs.mpVertexShader;
   if (mpVertexShader)
      mpVertexShader->AddRef();

   mIntrinsics = rhs.mIntrinsics;
   
   return *this;
}

//============================================================================
// BBoundEffectPassData::clear
//============================================================================
void BBoundEffectPassData::clear(void)
{
   if (mpPixelShader)
   {
      mpPixelShader->Release();
      mpPixelShader = NULL;
   }
   
   if (mpVertexShader)
   {
      mpVertexShader->Release();
      mpVertexShader = NULL;
   }
   
   mIntrinsics.clear();
}

//============================================================================
// BBoundEffectPassData::init
// rg [3/19/08] - 
//============================================================================
void BBoundEffectPassData::init(BFXLEffect& effect, const BBoundEffectPassDesc& desc, bool bindVertexToPixelShaders)
{
   clear();
   
   FXLPASS_DESC passDesc;
   desc.getPass().getDesc(passDesc);
   
   if ((passDesc.pPixelShaderFunction) && (passDesc.PixelShaderFunctionSize))
   {
      HRESULT hres = BD3D::mpDev->CreatePixelShader(passDesc.pPixelShaderFunction, &mpPixelShader);
      BVERIFY(SUCCEEDED(hres));
   }
   
   if ((passDesc.pVertexShaderFunction) && (passDesc.VertexShaderFunctionSize))
   {
      HRESULT hres = BD3D::mpDev->CreateVertexShader(passDesc.pVertexShaderFunction, &mpVertexShader);
      BVERIFY(SUCCEEDED(hres) && mpVertexShader);
      
      DWORD streamStrides[2] = { desc.getStreamStrides() & 0xFFFF, (desc.getStreamStrides() >> 16) & 0xFFFF };
      mpVertexShader->Bind(0, desc.getDecl(), streamStrides, bindVertexToPixelShaders ? mpPixelShader : NULL);
   }
   
   FXLEffect* pFXLEffect = effect.getEffect();
   BFXLEffectPass& pass = desc.getPass();
      
   for (uint i = 0; i < passDesc.RenderStates; i++)
   {
      D3DRENDERSTATETYPE stateType;
      DWORD stateValue;
      pFXLEffect->GetRenderState(pass.getHandle(), i, &stateType, &stateValue);
      mRenderStates.pushBack(BRenderState(stateType, stateValue));
   }
   
   for (uint i = 0; i < passDesc.SamplerStates; i++)
   {
      uint samplerIndex;
      D3DSAMPLERSTATETYPE stateType;
      DWORD stateValue;
      pFXLEffect->GetSamplerState(pass.getHandle(), i, &samplerIndex, &stateType, &stateValue);
      mSamplerStates.pushBack(BSamplerState(samplerIndex, stateType, stateValue));
   }
   
   const BEffectIntrinsicMapper& mapper = effect.getIntrinsicMapper();
   
   for (uint linkIndex = 0; linkIndex < mapper.getNumParamToIntrinsicLinks(); linkIndex++)
   {
      const uint paramIndex = mapper.getParamIndex(linkIndex);
      const uint intrinsicIndex = mapper.getIntrinsicIndex(linkIndex);
                  
      BFXLEffectParam param(effect.getParamFromIndex(paramIndex));
      
      FXLPARAMETER_CONTEXT context = param.getContext(pass);
      if (!context)
         continue;
      
      for (uint bitIndex = 0; bitIndex < 16; bitIndex++)
      {
         FXLPARAMETER_CONTEXT queryContext = (FXLPARAMETER_CONTEXT)(1 << bitIndex);
         if ((context & queryContext) == 0)
            continue;
            
         uint regIndex, regCount;
         param.getRegisters(pass, queryContext, regIndex, regCount);
         
         if (!regCount)
            continue;
         
         mIntrinsics.pushBack(BIntrinsic(intrinsicIndex, regIndex, queryContext));
      }
   }
}

//============================================================================
// BBoundEffectPassData::setToDevice
//============================================================================
void BBoundEffectPassData::setToDevice(IDirect3DDevice9* pDev, const BEffectIntrinsicTable& intrinsicTable)
{
   if (mpVertexShader)
   {
      IDirect3DVertexShader9* pVS;
      pDev->GetVertexShader(&pVS);
      if (pVS)
         pVS->Release();
      
      if (pVS != mpVertexShader)
      {
         pDev->SetVertexShader(mpVertexShader);
      }      
   }      
   
   if (mpPixelShader)
   {
      IDirect3DPixelShader9* pPS;
      pDev->GetPixelShader(&pPS);
      if (pPS)
         pPS->Release();

      if (pPS != mpPixelShader)
      {
         pDev->SetPixelShader(mpPixelShader);
      }      
   }
   
   for (uint i = 0; i < mRenderStates.getSize(); i++)
      pDev->SetRenderState(mRenderStates[i].mState, mRenderStates[i].mValue);
      
   for (uint i = 0; i < mSamplerStates.getSize(); i++)
      pDev->SetSamplerState(mSamplerStates[i].mSampler, mSamplerStates[i].mState, mSamplerStates[i].mValue);

   for (uint i = 0; i < mIntrinsics.getSize(); i++)
   {
      const BIntrinsic& intrinsic = mIntrinsics[i];
      
      const eEffectIntrinsicIndex intrinsicIndex   = static_cast<eEffectIntrinsicIndex>(intrinsic.mIntrinsicIndex);
      const uint regIndex                          = intrinsic.mRegIndex;
      const FXLPARAMETER_CONTEXT context           = static_cast<FXLPARAMETER_CONTEXT>(intrinsic.mContext);
      
      const BEffectIntrinsicDesc& intrinsicDesc = BEffectIntrinsics::getDesc(intrinsicIndex);
      
      switch (intrinsicDesc.mType)
      {
         case cIntrinsicTypeTexturePtr:
         {
            // rg [2/3/08] - Do we need to adjust the regIndex by D3DVERTEXTEXTURESAMPLER0 if it's a vertex shader sampler?
            BDEBUG_ASSERT((context & (FXLPCONTEXT_VERTEXSHADERSAMPLER | FXLPCONTEXT_PIXELSHADERSAMPLER)) != 0);
                        
            pDev->SetTexture(regIndex, *(D3DBaseTexture**)intrinsicTable.getPtr(intrinsicIndex));
            break;
         }
         default:
         {
            // currently unsupported
            BASSERT(0);
         }
      }      
   }
}

//============================================================================
// BBoundEffectPassData::setToDevice
//============================================================================
void BBoundEffectPassData::setToDevice(BRenderStateFilter& renderStateFilter, const BEffectIntrinsicTable& intrinsicTable)
{
   if (mpVertexShader)
      renderStateFilter.setVertexShader(mpVertexShader);
   if (mpPixelShader)
      renderStateFilter.setPixelShader(mpPixelShader);

   for (uint i = 0; i < mRenderStates.getSize(); i++)
      renderStateFilter.setRenderState(mRenderStates[i].mState, mRenderStates[i].mValue);

   for (uint i = 0; i < mSamplerStates.getSize(); i++)
      renderStateFilter.setSamplerState(mSamplerStates[i].mSampler, mSamplerStates[i].mState, mSamplerStates[i].mValue);

   for (uint i = 0; i < mIntrinsics.getSize(); i++)
   {
      const BIntrinsic& intrinsic = mIntrinsics[i];

      const eEffectIntrinsicIndex intrinsicIndex   = static_cast<eEffectIntrinsicIndex>(intrinsic.mIntrinsicIndex);
      const uint regIndex                          = intrinsic.mRegIndex;
      const FXLPARAMETER_CONTEXT context           = static_cast<FXLPARAMETER_CONTEXT>(intrinsic.mContext);

      const BEffectIntrinsicDesc& intrinsicDesc = BEffectIntrinsics::getDesc(intrinsicIndex);

      switch (intrinsicDesc.mType)
      {
         case cIntrinsicTypeTexturePtr:
         {
            // rg [2/3/08] - Do we need to adjust the regIndex by D3DVERTEXTEXTURESAMPLER0 if it's a vertex shader sampler?
            BDEBUG_ASSERT((context & (FXLPCONTEXT_VERTEXSHADERSAMPLER | FXLPCONTEXT_PIXELSHADERSAMPLER)) != 0);

            renderStateFilter.setTexture(regIndex, *(D3DBaseTexture**)intrinsicTable.getPtr(intrinsicIndex));
            break;
         }
         default:
         {
            // currently unsupported
            BASSERT(0);
         }
      }      
   }
}

//============================================================================
// BBoundEffectManager::BBoundEffectManager
//============================================================================
BBoundEffectManager::BBoundEffectManager(uint maxExpectedBoundEffects) :
   mBoundEffects(maxExpectedBoundEffects)
{
}

//============================================================================
// BBoundEffectManager::~BBoundEffectManager
//============================================================================
BBoundEffectManager::~BBoundEffectManager()
{
   clear();
}

//============================================================================
// BBoundEffectManager::clear
//============================================================================
void BBoundEffectManager::clear(void)
{
   mBoundEffects.clear();
}

//============================================================================
// BBoundEffectManager::BBoundEffectManager
//============================================================================
BBoundEffectPassData* BBoundEffectManager::get(
   BFXLEffect& effect, 
   BFXLEffectPass& pass, 
   IDirect3DVertexDeclaration9* pDecl, 
   uint stream0Stride, uint stream1Stride,
   bool bindVertexToPixelShaders)
{
   BBoundEffectPassDesc queryPassDesc(pass, pDecl, stream0Stride | (stream1Stride << 16), bindVertexToPixelShaders);
   
   BBoundEffectHashMap::iterator findIt(mBoundEffects.find(queryPassDesc));
   if (findIt == mBoundEffects.end())
   {
      BBoundEffectHashMap::InsertResult result(mBoundEffects.insert(queryPassDesc, BBoundEffectPassData()));
      
      findIt = result.first;
         
      BBoundEffectPassData& passData = findIt->second;
      passData.init(effect, queryPassDesc, bindVertexToPixelShaders);
   }         
   
   return &findIt->second;
}