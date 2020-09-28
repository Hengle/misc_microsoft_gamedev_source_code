//============================================================================
//
//  effect.cpp
//  
//  Copyright (c) 2006, Ensemble Studios
//
//  rg [2/15/06] - This is a simple, lightweight wrapper around the FX Light 
//  framework.
//
//============================================================================
#include "xrender.h"
#include "effect.h"
#include "effectIntrinsicManager.h"

//============================================================================
// BFXLEffect::BFXLEffect
//============================================================================
BFXLEffect::BFXLEffect() :
   mpEffect(NULL)
{
}

//============================================================================
// BFXLEffect::BFXLEffect
//============================================================================
BFXLEffect::BFXLEffect(const BFXLEffect& other) : 
   mpEffect(other.mpEffect)
{
   if (mpEffect)
   {
      mpEffect->AddRef();
      
      mIntrinsicMapper.init(this, other.getEffectIntrinsicPool());
   }
}

//============================================================================
// BFXLEffect::operator=
//============================================================================
BFXLEffect& BFXLEffect::operator= (const BFXLEffect& rhs)
{
   if (this != &rhs)
   {
      clear();
         
      mpEffect = rhs.mpEffect;
      if (mpEffect)
         mpEffect->AddRef();
      
      mIntrinsicMapper.init(this, rhs.getEffectIntrinsicPool());
   }
   return *this;
}

//============================================================================
// BFXLEffect::clear
//============================================================================
void BFXLEffect::clear(void)
{
   if (mpEffect)
   {
      if (gEventDispatcher.getThreadIndex() != cThreadIndexRender)
      {
         trace("BFXLEffect::clear: This method should be called from the render thread!");   
      }
      
      mpEffect->Release();
      mpEffect = NULL;

      mIntrinsicMapper.clear();
   }
}

//============================================================================
// BFXLEffect::~BFXLEffect
//============================================================================
BFXLEffect::~BFXLEffect()
{
   clear();
}

//============================================================================
// BFXLEffect::attach
// Calls AddRef(), so the caller must still release the effect object themselves.
//============================================================================
void BFXLEffect::attach(FXLEffect* pEffect, BFXLEffectIntrinsicPool* pPool)
{
   clear();
   
   mpEffect = pEffect;
   if (mpEffect)
   {
      mpEffect->AddRef();
            
      mIntrinsicMapper.init(this, pPool);
   }
}

//============================================================================
// BFXLEffect::createFromCompiledData
//============================================================================
HRESULT BFXLEffect::createFromCompiledData(PDIRECT3DDEVICE9 pDevice, void* pCompiledData, BFXLEffectIntrinsicPool* pPool, bool validateIntrinsics)
{
   clear();
   
   BDEBUG_ASSERT(pCompiledData);
   
   if (!pPool)
      pPool = &gEffectIntrinsicManager.getRenderEffectIntrinsicPool();

   HRESULT hres = FXLCreateEffect(pDevice, pCompiledData, pPool->getEffectPool(), &mpEffect);
   if (FAILED(hres))
      return hres;
   
   mIntrinsicMapper.init(this, pPool, validateIntrinsics);

   return S_OK;
}

//============================================================================
// BFXLEffect::findParamIndex
//============================================================================
int BFXLEffect::findParamIndex(PCSTR pName)
{
   for (uint paramIndex = 0; paramIndex < getNumParameters(); paramIndex++)
   {
      FXLHANDLE param = getParamFromIndex(paramIndex).getHandle();
      
      FXLPARAMETER_DESC paramDesc;
      
      mpEffect->GetParameterDesc(param, &paramDesc);      
      
      if (0 == strcmp(pName, paramDesc.pName))
         return paramIndex;
   }
   
   return cInvalidIndex;
}

//============================================================================
// BFXLEffect::findParamRegister
//============================================================================
int BFXLEffect::findParamRegister(PCSTR pName, BFXLEffectPass& pass, FXLPARAMETER_CONTEXT context, bool setManualRegisterUpdateMode)
{
   int index = findParamIndex(pName);
   if (index < 0)
      return cInvalidIndex;
   
   FXLHANDLE handle = getParamFromIndex(index).getHandle();
         
   uint regIndex = 0, regCount = 0;
   
   mpEffect->GetParameterRegister(
      pass.getHandle(),
      handle,
      context,
      &regIndex,
      &regCount);
      
   if (setManualRegisterUpdateMode)
      mpEffect->SetParameterRegisterUpdate(handle, FXLREGUPDATE_MANUAL);
      
   return regIndex;      
}

//============================================================================
// BFXLEffect::setParamToManualRegUpdate
//============================================================================
int BFXLEffect::setParamToManualRegUpdate(PCSTR pName)
{
   int index = findParamIndex(pName);
   if (index < 0)
      return cInvalidIndex;
   
   mpEffect->SetParameterRegisterUpdate(getParamFromIndex(index).getHandle(), FXLREGUPDATE_MANUAL);

   return index;      
}

//============================================================================
// BFXLEffect::setAllUserParamsToManualRegUpdate
// This sets all USER (non-intrinsic/shared) params to manual updating.
//============================================================================
void BFXLEffect::setAllUserParamsToManualRegUpdate(void)
{
   for (uint index = 0; index < getNumParameters(); index++)
   {
      const FXLHANDLE handle = getParamFromIndex(index).getHandle();
      
      FXLPARAMETER_DESC paramDesc;
      mpEffect->GetParameterDesc(handle, &paramDesc);
      if (paramDesc.Flags & FXLPFLAG_SHARED)
         continue;
      
      if (paramDesc.Annotations)
      {
         FXLHANDLE annoHandle = mpEffect->GetAnnotationHandle(handle, "intrinsic");
         if (annoHandle != cInvalidFXLHandle)
            continue;
      }
         
      mpEffect->SetParameterRegisterUpdate(handle, FXLREGUPDATE_MANUAL);
   }
}

//============================================================================
// BFXLEffect::getParamHandle
//============================================================================
FXLHANDLE BFXLEffect::getParamHandle(PCSTR pName)
{
   int index = findParamIndex(pName);
   
   if (index < 0)
      return cInvalidFXLHandle;

   return getParamFromIndex(index).getHandle();      
}
