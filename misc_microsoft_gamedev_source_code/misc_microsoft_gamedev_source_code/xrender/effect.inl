//============================================================================
//
//  effect.inl
//  
//  Copyright (c) 2006, Ensemble Studios
//
//============================================================================

//============================================================================
// BFXLEffectPass::BFXLEffectPass
//============================================================================
inline BFXLEffectPass::BFXLEffectPass(FXLEffect* pEffect, FXLHANDLE handle) : 
   mpEffect(pEffect), 
   mHandle(handle)
{
}

//============================================================================
// BFXLEffectPass::BFXLEffectPass(const BFXLEffectPass& other)
//============================================================================
inline BFXLEffectPass::BFXLEffectPass(const BFXLEffectPass& other)
{
   mpEffect = other.mpEffect;
   mHandle = other.mHandle;
}

//============================================================================
//  BFXLEffectPass::operator= (const BFXLEffectPass& rhs)
//============================================================================
inline BFXLEffectPass& BFXLEffectPass::operator= (const BFXLEffectPass& rhs)
{
   mpEffect = rhs.mpEffect;
   mHandle = rhs.mHandle;
}

//============================================================================
// BFXLEffectPass::getHandle
//============================================================================
inline FXLHANDLE BFXLEffectPass::getHandle(void)
{
   return mHandle;
}

//============================================================================
// BFXLEffectPass::getEffect
//============================================================================
inline FXLEffect* BFXLEffectPass::getEffect(void)
{
   return mpEffect;
}

//============================================================================
// BFXLEffectPass::begin
//============================================================================
inline void BFXLEffectPass::begin(void)
{
   BDEBUG_ASSERT(mpEffect && (mHandle != cInvalidFXLHandle));
   mpEffect->BeginPass(mHandle);
}

//============================================================================
// BFXLEffectPass::commit
//============================================================================
inline void BFXLEffectPass::commit(void)
{
   BDEBUG_ASSERT(mpEffect && (mHandle != cInvalidFXLHandle));
   //mpEffect->Commit();
   mpEffect->CommitU();
}

//============================================================================
// BFXLEffectPass::commitU
//============================================================================
inline void BFXLEffectPass::commitU(void)
{
   BDEBUG_ASSERT(mpEffect && (mHandle != cInvalidFXLHandle));
   mpEffect->CommitU();
}

//============================================================================
// BFXLEffectPass::end
//============================================================================
inline void BFXLEffectPass::end(void)
{
   BDEBUG_ASSERT(mpEffect && (mHandle != cInvalidFXLHandle));
   mpEffect->EndPass();
}

//============================================================================
// BFXLEffectPass::getDesc
//============================================================================
inline void BFXLEffectPass::getDesc(FXLPASS_DESC& desc)
{
   BDEBUG_ASSERT(mpEffect && (mHandle != cInvalidFXLHandle));
   mpEffect->GetPassDesc(mHandle, &desc);
}

//============================================================================
// BFXLEffectPass::getNumAnnotations
//============================================================================
inline uint BFXLEffectPass::getNumAnnotations(void)
{
   FXLPASS_DESC desc;
   getDesc(desc);
   return desc.Annotations;
}

//============================================================================
// BFXLEffectPass::BFXLEffectTechnique
//============================================================================
inline BFXLEffectAnnotation BFXLEffectPass::getAnnotation(const char* pName)
{
   BDEBUG_ASSERT(mpEffect && (mHandle != cInvalidFXLHandle));
   FXLHANDLE handle = mpEffect->GetAnnotationHandle(mHandle, pName);
   return BFXLEffectAnnotation(mpEffect, handle);
}

//============================================================================
// BFXLEffectPass::getAnnotation
//============================================================================
inline BFXLEffectAnnotation BFXLEffectPass::getAnnotation(uint index)
{
   BDEBUG_ASSERT(mpEffect && (mHandle != cInvalidFXLHandle));
   BDEBUG_ASSERT(index < getNumAnnotations());
   FXLHANDLE handle = mpEffect->GetAnnotationHandleFromIndex(mHandle, index);
   return BFXLEffectAnnotation(mpEffect, handle);
}

//============================================================================
// BFXLEffectTechnique::getPass
//============================================================================
inline BFXLEffectPass BFXLEffectTechnique::getPass(const char* pName)
{
   return BFXLEffectPass(mpEffect, mpEffect->GetPassHandle(mHandle, pName));
}

//============================================================================
// BFXLEffectTechnique::getPassFromIndex
//============================================================================
inline BFXLEffectPass BFXLEffectTechnique::getPassFromIndex(const uint index)
{
   BDEBUG_ASSERT(index < getNumPasses());

   return BFXLEffectPass(mpEffect, mpEffect->GetPassHandleFromIndex(mHandle, index));
}
 
//============================================================================
// BFXLEffectTechnique::BFXLEffectTechnique
//============================================================================
inline BFXLEffectTechnique::BFXLEffectTechnique() : 
   mpEffect(NULL), 
   mHandle(NULL) 
{
}

//============================================================================
// BFXLEffectTechnique::BFXLEffectTechnique
//============================================================================
inline BFXLEffectTechnique::BFXLEffectTechnique(FXLEffect* pEffect, FXLHANDLE handle) : 
   mpEffect(pEffect), 
   mHandle(handle) 
{
}

//============================================================================
// BFXLEffectTechnique::operator=
//============================================================================
inline BFXLEffectTechnique& BFXLEffectTechnique::operator= (const BFXLEffectTechnique& rhs)
{
   mpEffect = rhs.mpEffect;
   mHandle = rhs.mHandle;
   return *this;
}

//============================================================================
// BFXLEffectTechnique::clear
//============================================================================
inline void BFXLEffectTechnique::clear(void)
{
   mpEffect = NULL;
   mHandle = NULL;
}

//============================================================================
// BFXLEffectTechnique::getValid
//============================================================================
inline bool BFXLEffectTechnique::getValid(void)
{
   return mHandle != NULL;
}

//============================================================================
// BFXLEffectTechnique::begin
//============================================================================
inline void BFXLEffectTechnique::begin(DWORD flags)
{
   BDEBUG_ASSERT(NULL != mHandle);
   mpEffect->BeginTechnique(mHandle, flags);
}

//============================================================================
// BFXLEffectTechnique::end
//============================================================================
inline void BFXLEffectTechnique::end(void)
{
   mpEffect->EndTechnique();
}

//============================================================================
// BFXLEffectTechnique::beginPass
//============================================================================
inline void BFXLEffectTechnique::beginPass(uint index)
{
   BDEBUG_ASSERT(index < getNumPasses());
   mpEffect->BeginPassFromIndex(index);
}

//============================================================================
// BFXLEffectTechnique::endPass
//============================================================================
inline void BFXLEffectTechnique::endPass(void)
{
   mpEffect->EndPass();
}

//============================================================================
// BFXLEffectTechnique::commit
//============================================================================
inline void BFXLEffectTechnique::commit(void)
{
   // rg HACK HACK - so shared params work
   //mpEffect->Commit();
   mpEffect->CommitU();
}

//============================================================================
// BFXLEffectTechnique::commitU
//============================================================================
inline void BFXLEffectTechnique::commitU(void)
{
   mpEffect->CommitU();
}

//============================================================================
// BFXLEffectTechnique::getDesc
//============================================================================
inline void BFXLEffectTechnique::getDesc(FXLTECHNIQUE_DESC& desc)
{
   BDEBUG_ASSERT(NULL != mHandle);
   mpEffect->GetTechniqueDesc(mHandle, &desc);
}

//============================================================================
// BFXLEffectTechnique::getName
//============================================================================
inline const char* BFXLEffectTechnique::getName(void) 
{
   FXLTECHNIQUE_DESC desc;
   getDesc(desc);
   return desc.pName;
}

//============================================================================
// BFXLEffectTechnique::getNumPasses
//============================================================================
inline uint BFXLEffectTechnique::getNumPasses(void) 
{
   FXLTECHNIQUE_DESC desc;
   getDesc(desc);
   return desc.Passes;
}

//============================================================================
// BFXLEffectTechnique::getNumAnnotations
//============================================================================
inline uint BFXLEffectTechnique::getNumAnnotations(void) 
{
   FXLTECHNIQUE_DESC desc;
   getDesc(desc);
   return desc.Annotations;
}

//============================================================================
// BFXLEffectTechnique::BFXLEffectTechnique
//============================================================================
inline BFXLEffectAnnotation BFXLEffectTechnique::getAnnotation(const char* pName)
{
   BDEBUG_ASSERT(mpEffect && (mHandle != cInvalidFXLHandle));
   FXLHANDLE handle = mpEffect->GetAnnotationHandle(mHandle, pName);
   return BFXLEffectAnnotation(mpEffect, handle);
}

//============================================================================
// BFXLEffectPass::getAnnotation
//============================================================================
inline BFXLEffectAnnotation BFXLEffectTechnique::getAnnotation(uint index)
{
   BDEBUG_ASSERT(mpEffect && (mHandle != cInvalidFXLHandle));
   BDEBUG_ASSERT(index < getNumAnnotations());
   FXLHANDLE handle = mpEffect->GetAnnotationHandleFromIndex(mHandle, index);
   return BFXLEffectAnnotation(mpEffect, handle);
}

//============================================================================
// BFXLEffectParamTemplate<T>::BFXLEffectParamTemplate
//============================================================================
template<class T>
inline BFXLEffectParamTemplate<T>::BFXLEffectParamTemplate() :
   mpEffect(NULL),
   mHandle(cInvalidFXLHandle)
{
}

//============================================================================
// BFXLEffectParamTemplate<T>::BFXLEffectParamTemplate
//============================================================================
template<class T>
inline BFXLEffectParamTemplate<T>::BFXLEffectParamTemplate(T* pEffect, FXLHANDLE handle) : 
   mpEffect(pEffect), 
   mHandle(handle) 
{ 
}

//============================================================================
// BFXLEffectParamTemplate<T>::clear
//============================================================================
template<class T>
inline void BFXLEffectParamTemplate<T>::clear(void)
{
   mpEffect = NULL;
   mHandle = cInvalidFXLHandle;
}

//============================================================================
// BFXLEffectParamTemplate<T>::getValid
//============================================================================
template<class T>
inline bool BFXLEffectParamTemplate<T>::getValid(void) const
{
   return cInvalidFXLHandle != mHandle;
}

//============================================================================
// BFXLEffectParamTemplate<T>::getHandle
//============================================================================
template<class T>
inline FXLHANDLE BFXLEffectParamTemplate<T>::getHandle(void) const
{ 
   return mHandle; 
}

//============================================================================
// BFXLEffectParamTemplate<T>::getEffect
//============================================================================
template<class T>
inline T* BFXLEffectParamTemplate<T>::getEffect(void) const
{ 
   return mpEffect; 
}

//============================================================================
// BFXLEffectParamTemplate<T>::getDesc
//============================================================================
template<class T>
inline void BFXLEffectParamTemplate<T>::getDesc(FXLPARAMETER_DESC& desc) const
{
   BDEBUG_ASSERT(cInvalidFXLHandle != mHandle);
   mpEffect->GetParameterDesc(mHandle, &desc);
}

//============================================================================
// BFXLEffectParamTemplate<T>::isBool
//============================================================================
template<class T>
inline bool BFXLEffectParamTemplate<T>::isBool(void) const
{
   FXLPARAMETER_DESC desc;
   getDesc(desc);
   return (desc.Content == FXLPACONTENT_DATA) && (desc.Type == FXLDTYPE_BOOL) && (desc.Class == FXLDCLASS_SCALAR);
}

//============================================================================
// BFXLEffectParamTemplate<T>::isFloat
//============================================================================
template<class T>
inline bool BFXLEffectParamTemplate<T>::isFloat(void) const
{
   FXLPARAMETER_DESC desc;
   getDesc(desc);
   return (desc.Content == FXLPACONTENT_DATA) && (desc.Type == FXLDTYPE_FLOAT) && (desc.Class == FXLDCLASS_SCALAR);
}

//============================================================================
// BFXLEffectParamTemplate<T>::isIntVector
//============================================================================
template<class T>
inline bool BFXLEffectParamTemplate<T>::isIntVector(void) const
{
   FXLPARAMETER_DESC desc;
   getDesc(desc);
   return (desc.Content == FXLPACONTENT_DATA) && (desc.Type == FXLDTYPE_INT) && (desc.Class == FXLDCLASS_VECTOR);
}

//============================================================================
// BFXLEffectParamTemplate<T>::isVector
//============================================================================
template<class T>
inline bool BFXLEffectParamTemplate<T>::isVector(uint size) const
{
   FXLPARAMETER_DESC desc;
   getDesc(desc);
   return (desc.Content == FXLPACONTENT_DATA) && (desc.Type == FXLDTYPE_FLOAT) && (desc.Class == FXLDCLASS_VECTOR) && (desc.Columns == size);
}

//============================================================================
// BFXLEffectParamTemplate<T>::isMatrix
//============================================================================
template<class T>
inline bool BFXLEffectParamTemplate<T>::isMatrix(uint rows, uint cols) const
{
   FXLPARAMETER_DESC desc;
   getDesc(desc);
   return  (desc.Content == FXLPACONTENT_DATA) && (desc.Type == FXLDTYPE_FLOAT) &&  ((desc.Class == FXLDCLASS_RMATRIX) || (desc.Class == FXLDCLASS_CMATRIX)) && (desc.Rows == rows) && (desc.Columns == cols);
}

//============================================================================
// BFXLEffectParamTemplate<T>::isArray
//============================================================================
template<class T>
inline bool BFXLEffectParamTemplate<T>::isArray(uint num, uint expectedSize) const
{
   FXLPARAMETER_DESC desc;
   getDesc(desc);
   return (desc.Content == FXLPACONTENT_ARRAY) && (desc.Type == FXLDTYPE_CONTAINER) && (desc.Class == FXLDCLASS_CONTAINER) && (desc.Size == expectedSize);
}

//============================================================================
// BFXLEffectParamTemplate<T>::isRowMatrix
//============================================================================
template<class T>
inline bool BFXLEffectParamTemplate<T>::isRowMatrix(uint rows, uint cols) const
{
   FXLPARAMETER_DESC desc;
   getDesc(desc);
   return (desc.Content == FXLPACONTENT_DATA) && (desc.Type == FXLDTYPE_FLOAT) && (desc.Class == FXLDCLASS_RMATRIX) && (desc.Rows == rows) && (desc.Columns == cols);
}

//============================================================================
// BFXLEffectParamTemplate<T>::isColMatrix
//============================================================================
template<class T>
inline bool BFXLEffectParamTemplate<T>::isColMatrix(uint rows, uint cols) const
{
   FXLPARAMETER_DESC desc;
   getDesc(desc);
   return (desc.Content == FXLPACONTENT_DATA) && (desc.Type == FXLDTYPE_FLOAT) && (desc.Class == FXLDCLASS_CMATRIX) && (desc.Rows == rows) && (desc.Columns == cols);
}

//============================================================================
// BFXLEffectParamTemplate<T>::isSampler
//============================================================================
template<class T>
inline bool BFXLEffectParamTemplate<T>::isSampler(void) const
{
   FXLPARAMETER_DESC desc;
   getDesc(desc);
   return (desc.Content == FXLPACONTENT_DATA) && (desc.Type == FXLDTYPE_SAMPLER);
}

//============================================================================
// BFXLEffectParamTemplate<T>::isShared
//============================================================================
template<class T>
inline bool BFXLEffectParamTemplate<T>::isShared(void) const
{
   FXLPARAMETER_DESC desc;
   getDesc(desc);
   return (desc.Flags & FXLPFLAG_SHARED) != 0;
}

//============================================================================
// BFXLEffectParamTemplate<T>::operator bool
//============================================================================
template<class T>
inline BFXLEffectParamTemplate<T>::operator bool(void) const
{
   BDEBUG_ASSERT(cInvalidFXLHandle != mHandle);
   BOOL val = FALSE;
   mpEffect->GetScalarB(mHandle, &val);
   return 0 != val;
}

//============================================================================
// BFXLEffectParamTemplate<T>::operator int
//============================================================================
template<class T>
inline BFXLEffectParamTemplate<T>::operator int(void) const
{
   BDEBUG_ASSERT(cInvalidFXLHandle != mHandle);
   int val = 0;
   mpEffect->GetScalarI(mHandle, &val);
   return val;
}

//============================================================================
// BFXLEffectParamTemplate<T>::operator float
//============================================================================
template<class T>
inline BFXLEffectParamTemplate<T>::operator float(void) const
{
   BDEBUG_ASSERT(cInvalidFXLHandle != mHandle);
   float val = 0.0f;
   mpEffect->GetScalarF(mHandle, &val);
   return val;
}

//============================================================================
// BFXLEffectParamTemplate<T>::operator D3DXVECTOR2
//============================================================================
template<class T>
inline BFXLEffectParamTemplate<T>::operator D3DXVECTOR2(void) const
{ 
   BDEBUG_ASSERT(cInvalidFXLHandle != mHandle);
   ensureVectorType(FXLDTYPE_FLOAT, 2);
   D3DXVECTOR2 val(0.0f, 0.0f);
   mpEffect->GetVectorF(mHandle, (FLOAT*)&val);
   return val;
}

//============================================================================
// BFXLEffectParamTemplate<T>::operator BVec2
//============================================================================
template<class T>
inline BFXLEffectParamTemplate<T>::operator BVec2(void) const
{
   BDEBUG_ASSERT(cInvalidFXLHandle != mHandle);
   ensureVectorType(FXLDTYPE_FLOAT, 2);
   BVec2 val(0.0f, 0.0f);
   mpEffect->GetVectorF(mHandle, val.getPtr());
   return val;
}

//============================================================================
// BFXLEffectParamTemplate<T>::operator BVec3
//============================================================================
template<class T>
inline BFXLEffectParamTemplate<T>::operator BVec3(void) const
{
   BDEBUG_ASSERT(cInvalidFXLHandle != mHandle);
   ensureVectorType(FXLDTYPE_FLOAT, 3);
   BVec3 val(0.0f, 0.0f, 0.0f);
   mpEffect->GetVectorF(mHandle, val.getPtr());
   return val;
}

//============================================================================
// BFXLEffectParamTemplate<T>::operator BVec4
//============================================================================
template<class T>
inline BFXLEffectParamTemplate<T>::operator BVec4(void) const
{
   BDEBUG_ASSERT(cInvalidFXLHandle != mHandle);
   ensureVectorType(FXLDTYPE_FLOAT, 4);
   BVec4 val(0.0f, 0.0f, 0.0f, 0.0f);
   mpEffect->GetVectorF(mHandle, val.getPtr());
   return val;
}

//============================================================================
// BFXLEffectParamTemplate<T>::operator D3DXMATRIX
//============================================================================
template<class T>
inline BFXLEffectParamTemplate<T>::operator D3DXMATRIX(void) const
{
   BDEBUG_ASSERT(cInvalidFXLHandle != mHandle);
   ensureMatrixType(FXLDTYPE_FLOAT, 4, 4);
   D3DXMATRIX val(1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1);
   mpEffect->GetMatrixF(mHandle, (FLOAT*)&val);
   return val;
}

//============================================================================
// BFXLEffectParamTemplate<T>::operator BMatrix44
//============================================================================
template<class T>
inline BFXLEffectParamTemplate<T>::operator BMatrix44(void) const
{
   BDEBUG_ASSERT(cInvalidFXLHandle != mHandle);
   ensureMatrixType(FXLDTYPE_FLOAT, 4, 4);
   BMatrix44 val;
   val.setIdentity();
   mpEffect->GetMatrixF(mHandle, val.getPtr());
   return val;
}

//============================================================================
// getInt4
//============================================================================
template<class T>
void BFXLEffectParamTemplate<T>::getInt4(int* pVal4) const
{
   BDEBUG_ASSERT(cInvalidFXLHandle != mHandle);
   mpEffect->GetVectorI(mHandle, pVal4);
}

//============================================================================
// BFXLEffectParamTemplate<T>::operator=
//============================================================================
template<class T>
inline BFXLEffectParamTemplate<T>& BFXLEffectParamTemplate<T>::operator= (const BFXLEffectParamTemplate<T>& rhs)
{
   mpEffect = rhs.mpEffect;
   mHandle = rhs.mHandle;
   return *this;
}

//============================================================================
// BFXLEffectParamTemplate<T>::operator=
//============================================================================
template<class T>
inline BFXLEffectParamTemplate<T>& BFXLEffectParamTemplate<T>::operator= (bool val)
{
   BDEBUG_ASSERT(cInvalidFXLHandle != mHandle);
   BOOL bVal = val;
   mpEffect->SetScalarB(mHandle, &bVal);
   return *this;
}

//============================================================================
// BFXLEffectParamTemplate<T>::operator=
//============================================================================
template<class T>
inline BFXLEffectParamTemplate<T>& BFXLEffectParamTemplate<T>::operator= (const int val)
{
   BDEBUG_ASSERT(cInvalidFXLHandle != mHandle);
   mpEffect->SetScalarI(mHandle, &val);
   return *this;
}

//============================================================================
// BFXLEffectParamTemplate<T>::operator=
//============================================================================
template<class T>
inline BFXLEffectParamTemplate<T>& BFXLEffectParamTemplate<T>::operator= (float val)
{
   BDEBUG_ASSERT(cInvalidFXLHandle != mHandle);
   mpEffect->SetScalarF(mHandle, &val);
   return *this;
}

//============================================================================
// BFXLEffectParamTemplate<T>::operator=
//============================================================================
template<class T>
inline BFXLEffectParamTemplate<T>& BFXLEffectParamTemplate<T>::operator= (const int* pVal4)
{
   BDEBUG_ASSERT(cInvalidFXLHandle != mHandle);
   mpEffect->SetVectorI(mHandle, pVal4);
   return *this;
}

//============================================================================
// BFXLEffectParamTemplate<T>::operator=
//============================================================================
template<class T>
inline BFXLEffectParamTemplate<T>& BFXLEffectParamTemplate<T>::operator= (const D3DXVECTOR2& val)
{
   BDEBUG_ASSERT(cInvalidFXLHandle != mHandle);
   mpEffect->SetVectorF(mHandle, (float*)&val);
   return *this;
}

//============================================================================
// BFXLEffectParamTemplate<T>::operator=
//============================================================================
template<class T>
inline BFXLEffectParamTemplate<T>& BFXLEffectParamTemplate<T>::operator= (const BVec2& val)
{
   BDEBUG_ASSERT(cInvalidFXLHandle != mHandle);
   mpEffect->SetVectorF(mHandle, val.getPtr());
   return *this;
}

//============================================================================
// BFXLEffectParamTemplate<T>::operator=
//============================================================================
template<class T>
inline BFXLEffectParamTemplate<T>& BFXLEffectParamTemplate<T>::operator= (const D3DXVECTOR3& val)
{
   BDEBUG_ASSERT(cInvalidFXLHandle != mHandle);
   mpEffect->SetVectorF(mHandle, (float*)&val);
   return *this;
}

//============================================================================
// BFXLEffectParamTemplate<T>::operator=
//============================================================================
template<class T>
inline BFXLEffectParamTemplate<T>& BFXLEffectParamTemplate<T>::operator= (const BVec3& val)
{
   BDEBUG_ASSERT(cInvalidFXLHandle != mHandle);
   mpEffect->SetVectorF(mHandle, val.getPtr());
   return *this;
}

//============================================================================
// BFXLEffectParamTemplate<T>::operator=
//============================================================================
template<class T>
inline BFXLEffectParamTemplate<T>& BFXLEffectParamTemplate<T>::operator= (const D3DXVECTOR4& val)
{
   BDEBUG_ASSERT(cInvalidFXLHandle != mHandle);
   mpEffect->SetVectorF(mHandle, (float*)&val);
   return *this;
}

//============================================================================
// BFXLEffectParamTemplate<T>::operator=
//============================================================================
template<class T>
inline BFXLEffectParamTemplate<T>& BFXLEffectParamTemplate<T>::operator= (const BVec4& val)
{
   BDEBUG_ASSERT(cInvalidFXLHandle != mHandle);
   mpEffect->SetVectorF(mHandle, val.getPtr());
   return *this;
}

//============================================================================
// BFXLEffectParamTemplate<T>::operator=
//============================================================================
template<class T>
inline BFXLEffectParamTemplate<T>& BFXLEffectParamTemplate<T>::operator= (const XMVECTOR val)
{
   BDEBUG_ASSERT(cInvalidFXLHandle != mHandle);
   XMFLOAT4A fVal;
   XMStoreFloat4A(&fVal, val);
   mpEffect->SetVectorF(mHandle, (float*)&fVal);
   return *this;
}

//============================================================================
// BFXLEffectParamTemplate<T>::operator=
//============================================================================
template<class T>
inline BFXLEffectParamTemplate<T>& BFXLEffectParamTemplate<T>::operator= (const D3DXMATRIX& val)
{
   BDEBUG_ASSERT(cInvalidFXLHandle != mHandle);
   mpEffect->SetMatrixF(mHandle, (FLOAT*)&val);
   return *this;
}

//============================================================================
// BFXLEffectParamTemplate<T>::operator=
//============================================================================
template<class T>
inline BFXLEffectParamTemplate<T>& BFXLEffectParamTemplate<T>::operator= (const BMatrix44& val)
{
   BDEBUG_ASSERT(cInvalidFXLHandle != mHandle);
   mpEffect->SetMatrixF(mHandle, (FLOAT*)&val);
   return *this;
}

//============================================================================
// BFXLEffectParamTemplate<T>::operator=
//============================================================================
template<class T>
inline BFXLEffectParamTemplate<T>& BFXLEffectParamTemplate<T>::operator= (const XMMATRIX& val)
{
   BDEBUG_ASSERT(cInvalidFXLHandle != mHandle);
   mpEffect->SetMatrixF(mHandle, (FLOAT*)&val);
   return *this;
}

//============================================================================
// BFXLEffectParamTemplate<T>::operator=
//============================================================================
template<class T>
inline BFXLEffectParamTemplate<T>& BFXLEffectParamTemplate<T>::operator= (IDirect3DBaseTexture9* pTex)
{
   BDEBUG_ASSERT(cInvalidFXLHandle != mHandle); 
   mpEffect->SetSampler(mHandle, pTex);
   return *this;
}

//============================================================================
// BFXLEffectParamTemplate<T>::setArray
//============================================================================
template<class T>
inline BFXLEffectParamTemplate<T>& BFXLEffectParamTemplate<T>::setArray(const D3DXVECTOR4* pValues, uint numValues)
{
   BDEBUG_ASSERT(cInvalidFXLHandle != mHandle);
   mpEffect->SetVectorArrayF(mHandle, reinterpret_cast<const float*>(pValues), numValues);
   return *this;
}
//============================================================================
// BFXLEffectParamTemplate<T>::setArray
//============================================================================
template<class T>
inline BFXLEffectParamTemplate<T>& BFXLEffectParamTemplate<T>::setArray(const float* pValues, uint numValues)
{
   BDEBUG_ASSERT(cInvalidFXLHandle != mHandle);
   mpEffect->SetScalarArrayF(mHandle, (pValues), numValues);
   return *this;
}

//============================================================================
// BFXLEffectParamTemplate<T>::setArray
//============================================================================
template<class T>
inline BFXLEffectParamTemplate<T>& BFXLEffectParamTemplate<T>::setArray(const BVec4* pValues, uint numValues)
{
   BDEBUG_ASSERT(cInvalidFXLHandle != mHandle);
   mpEffect->SetVectorArrayF(mHandle, reinterpret_cast<const float*>(pValues), numValues);
   return *this;
}

//============================================================================
// BFXLEffectParamTemplate<T>::setArray
//============================================================================
template<class T>
inline BFXLEffectParamTemplate<T>& BFXLEffectParamTemplate<T>::setArray(const XMVECTOR* pValues, uint numValues)
{
   BDEBUG_ASSERT(cInvalidFXLHandle != mHandle);
   mpEffect->SetVectorArrayF(mHandle, reinterpret_cast<const float*>(pValues), numValues);
   return *this;
}

//============================================================================
// BFXLEffectParamTemplate<T>::getName
//============================================================================
template<class T>
inline const char* BFXLEffectParamTemplate<T>::getName(void) const
{
   BDEBUG_ASSERT(cInvalidFXLHandle != mHandle);
   FXLPARAMETER_DESC desc;
   mpEffect->GetParameterDesc(mHandle, &desc);
   return desc.pName;
}

//============================================================================
// BFXLEffectParamTemplate<T>::getNumAnnotations
//============================================================================
template<class T>
inline uint BFXLEffectParamTemplate<T>::getNumAnnotations(void) const
{
   FXLPARAMETER_DESC desc;
   getDesc(desc);
   return desc.Annotations;
}

#ifdef BUILD_DEBUG 
//============================================================================
// BFXLEffectParamTemplate<T>::ensureVectorType
//============================================================================  
template<class T>
inline void BFXLEffectParamTemplate<T>::ensureVectorType(FXLDATA_TYPE Type, uint cols) const
{
   FXLPARAMETER_DESC desc;
   getDesc(desc);
   BDEBUG_ASSERT(
      (desc.Content == FXLPACONTENT_DATA) && 
      (desc.Type == Type) && 
      (desc.Class == FXLDCLASS_VECTOR) && 
      (desc.Columns == cols));
}      

//============================================================================
// BFXLEffectParamTemplate<T>::ensureMatrixType
//============================================================================
template<class T>
inline void BFXLEffectParamTemplate<T>::ensureMatrixType(FXLDATA_TYPE Type, uint rows, uint cols) const
{
   BDEBUG_ASSERT(cInvalidFXLHandle != mHandle);
   FXLPARAMETER_DESC desc;
   mpEffect->GetParameterDesc(mHandle, &desc);
   BDEBUG_ASSERT(
      (desc.Content == FXLPACONTENT_DATA) && 
      (desc.Type == Type) && 
      ((desc.Class == FXLDCLASS_RMATRIX)||(desc.Class == FXLDCLASS_CMATRIX)) && 
      (desc.Rows == rows) && 
      (desc.Columns == cols));
}      
#else
//============================================================================
// BFXLEffectParamTemplate<T>::ensureVectorType
//============================================================================
template<class T>
inline void BFXLEffectParamTemplate<T>::ensureVectorType(FXLDATA_TYPE Type, uint cols) const
{
}

//============================================================================
// BFXLEffectParamTemplate<T>::ensureMatrixType
//============================================================================
template<class T>      
inline void BFXLEffectParamTemplate<T>::ensureMatrixType(FXLDATA_TYPE Type, uint rows, uint cols) const
{
}
#endif

//============================================================================
// BFXLEffectParamTemplate<T>::setRegisterUpdateMode
//============================================================================
template<class T>
inline void BFXLEffectParamTemplate<T>::setRegisterUpdateMode(bool manualUpdate)
{
   BDEBUG_ASSERT(cInvalidFXLHandle != mHandle);
   mpEffect->SetParameterRegisterUpdate(mHandle, manualUpdate ? FXLREGUPDATE_MANUAL : FXLREGUPDATE_AUTOMATIC);
}

//============================================================================
// BFXLEffectParamTemplate<T>::getContext
//============================================================================
template<class T>
inline FXLPARAMETER_CONTEXT BFXLEffectParamTemplate<T>::getContext(BFXLEffectPass& pass) const
{
   BDEBUG_ASSERT(cInvalidFXLHandle != mHandle);
   return mpEffect->GetParameterContext(pass.getHandle(), mHandle);
}

//============================================================================
// BFXLEffectParamTemplate<T>::getRegister
//============================================================================
template<class T>
inline uint BFXLEffectParamTemplate<T>::getRegister(BFXLEffectPass& pass, FXLPARAMETER_CONTEXT context) const
{
   BDEBUG_ASSERT(cInvalidFXLHandle != mHandle);
   uint index, count;
   mpEffect->GetParameterRegister(
      pass.getHandle(),
      mHandle,
      context,
      &index,
      &count);
   return index;      
}

//============================================================================
// BFXLEffectParamTemplate<T>::getRegisters
//============================================================================
template<class T>
inline void BFXLEffectParamTemplate<T>::getRegisters(BFXLEffectPass& pass, FXLPARAMETER_CONTEXT context, uint& index, uint& count) const
{
   BDEBUG_ASSERT(cInvalidFXLHandle != mHandle);
   mpEffect->GetParameterRegister(
      pass.getHandle(),
      mHandle,
      context,
      &index,
      &count);
}

//============================================================================
// BFXLEffect::getEffect
//============================================================================
inline FXLEffect* BFXLEffect::getEffect(void) const 
{ 
   return mpEffect; 
}

//============================================================================
// BFXLEffect::operator()
//============================================================================
inline BFXLEffectParam BFXLEffect::operator() (PCSTR pName) 
{ 
   checkNull(mpEffect); return BFXLEffectParam(mpEffect, mpEffect->GetParameterHandle(pName)); 
}

//============================================================================
// BFXLEffect::getParam
//============================================================================
inline BFXLEffectParam BFXLEffect::getParam(PCSTR pName) 
{ 
   return (*this)(pName); 
}

//============================================================================
// BFXLEffect::getParamFromIndex
//============================================================================
inline BFXLEffectParam BFXLEffect::getParamFromIndex(uint index) 
{ 
   checkNull(mpEffect); 
   BDEBUG_ASSERT(index < getNumParameters());
   return BFXLEffectParam(mpEffect, mpEffect->GetParameterHandleFromIndex(index)); 
}

//============================================================================
// BFXLEffect::getTechnique
//============================================================================
inline BFXLEffectTechnique BFXLEffect::getTechnique(PCSTR pName) 
{ 
   checkNull(mpEffect); 
   return BFXLEffectTechnique(mpEffect, mpEffect->GetTechniqueHandle(pName)); 
}

//============================================================================
// BFXLEffect::getTechniqueFromIndex
//============================================================================
inline BFXLEffectTechnique BFXLEffect::getTechniqueFromIndex(uint index) 
{ 
   checkNull(mpEffect);       
   BDEBUG_ASSERT(index < getNumTechniques());
   return BFXLEffectTechnique(mpEffect, mpEffect->GetTechniqueHandleFromIndex(index)); 
}

//============================================================================
// BFXLEffect::getDesc
//============================================================================
inline void BFXLEffect::getDesc(FXLEFFECT_DESC& desc)
{
   checkNull(mpEffect); 
   mpEffect->GetEffectDesc(&desc);
}

//============================================================================
// BFXLEffect::getNumParameters
//============================================================================
inline uint BFXLEffect::getNumParameters(void)
{
   FXLEFFECT_DESC desc;
   getDesc(desc);
   return desc.Parameters;
}

//============================================================================
// BFXLEffect::getNumTechniques
//============================================================================
inline uint BFXLEffect::getNumTechniques(void)
{
   FXLEFFECT_DESC desc;
   getDesc(desc);
   return desc.Techniques;
}

//============================================================================
// BFXLEffect::updateIntrinsicParams
//============================================================================
inline void BFXLEffect::updateIntrinsicParams(bool force)
{ 
   BDEBUG_ASSERT(mpEffect); 
   mIntrinsicMapper.apply(this, force); 
}

//============================================================================
// BFXLEffectPool::BFXLEffectPool
//============================================================================
inline BFXLEffectPool::BFXLEffectPool() :
   mpEffectPool(NULL)
{
}

//============================================================================
// BFXLEffectPool::clear
//============================================================================
inline void BFXLEffectPool::clear(void)
{
   if (mpEffectPool)
   {
      mpEffectPool->Release();
      mpEffectPool = NULL;
   }
}

//============================================================================
// BFXLEffectPool::~BFXLEffectPool
//============================================================================
inline BFXLEffectPool::~BFXLEffectPool()
{
   clear();
}

//============================================================================
// BFXLEffectPool::create
//============================================================================
inline void BFXLEffectPool::create(void)
{
   clear();

   HRESULT hres = FXLCreateEffectPool(&mpEffectPool);
   if (FAILED(hres))
   {
      BFATAL_FAIL("BFXLEffectPool::create: FXLCreateEffectPool failed");
   }
}

//============================================================================
// BFXLEffectPool::attach
//============================================================================
inline void BFXLEffectPool::attach(FXLEffectPool* pPool)
{
   clear();
   mpEffectPool = pPool;
   if (pPool)
      pPool->AddRef();
}

//============================================================================
// BFXLEffectPool::getEffectPool
//============================================================================
inline FXLEffectPool* BFXLEffectPool::getEffectPool(void)  
{
   return mpEffectPool;
}

//============================================================================
// BFXLEffectPool::operator()
//============================================================================
inline BFXLEffectPoolParam BFXLEffectPool::operator()(PCSTR pName) const
{
   checkNull(mpEffectPool); 
   return BFXLEffectPoolParam(mpEffectPool, mpEffectPool->GetParameterHandle(pName));
}

