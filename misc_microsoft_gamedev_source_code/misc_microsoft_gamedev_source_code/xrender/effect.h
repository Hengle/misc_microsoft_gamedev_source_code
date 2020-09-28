//============================================================================
//
//  effect.h
//  
//  Copyright (c) 2006, Ensemble Studios
//
//============================================================================
#pragma once

#include "math\generalMatrix.h"

#include "effectIntrinsicMapper.h"

// BRenderDraw is only used to determine if BeginScene() has been called, for debugging.
#include "renderDraw.h"

class BFXLEffect;
class BFXLEffectTechnique;

const FXLHANDLE cInvalidFXLHandle = static_cast<FXLHANDLE>(0);

//============================================================================
// class BFXLEffectAnnotation
//============================================================================
class BFXLEffectAnnotation
{
public:
   BFXLEffectAnnotation(FXLEffect* pEffect, FXLHANDLE handle) : 
      mpEffect(pEffect), 
      mHandle(handle)
   {
      initDesc();
   }
   
   BFXLEffectAnnotation(const BFXLEffectAnnotation& other) :
      mpEffect(other.mpEffect),
      mHandle(other.mHandle)
   {
      initDesc();
   }
   
   BFXLEffectAnnotation& operator= (const BFXLEffectAnnotation& rhs)
   {
      mHandle = rhs.mHandle;
      mpEffect = rhs.mpEffect;
      mDesc = rhs.mDesc;
      return *this;
   }
   
   bool isValid(void) const { return (mpEffect != NULL) && (mHandle != cInvalidFXLHandle); }

   FXLHANDLE getHandle(void) { return mHandle; }
   FXLEffect* getEffect(void) { return mpEffect; }
   const FXLANNOTATION_DESC& getDesc(void) { return mDesc; }
   
   const char* getName(void) { return mDesc.pName; }
   
   bool isBool(void) const 
   {
      return (mDesc.Content == FXLPACONTENT_DATA) && (mDesc.Type == FXLDTYPE_BOOL) && (mDesc.Class == FXLDCLASS_SCALAR);
   }
   
   bool isString(void) const
   {
      return (mDesc.Content == FXLPACONTENT_DATA) && (mDesc.Type == FXLDTYPE_STRING) && (mDesc.Size < MAX_PATH);
   }
   
   bool isFloat(void) const
   {
      return (mDesc.Content == FXLPACONTENT_DATA) && (mDesc.Type == FXLDTYPE_FLOAT) && (mDesc.Class == FXLDCLASS_SCALAR);
   }
   
   bool isInt(void) const 
   { 
      return (mDesc.Content == FXLPACONTENT_DATA) && (mDesc.Type == FXLDTYPE_INT) && (mDesc.Class == FXLDCLASS_SCALAR); 
   }
   
   bool getBool(void) const
   {
      BDEBUG_ASSERT(isBool());
      BOOL val = 0;
      mpEffect->GetAnnotation(mHandle, &val);
      return val != 0;
   }
   
   float getFloat(void) const
   {
      BDEBUG_ASSERT(isBool());
      float val = 0.0f;
      mpEffect->GetAnnotation(mHandle, &val);
      return val;
   }
   
   int getInt(void) const
   {
      BDEBUG_ASSERT(isInt());
      int val = 0;
      mpEffect->GetAnnotation(mHandle, &val);
      return val;
   }
   
   BFixedStringMaxPath& getString(BFixedStringMaxPath& str) const
   {
      BDEBUG_ASSERT(isString());
      str.empty();
      mpEffect->GetAnnotation(mHandle, str.getPtr());
      return str;
   }
         
private:
   FXLHANDLE mHandle;
   FXLEffect* mpEffect;
   
   FXLANNOTATION_DESC mDesc;
   
   void initDesc(void)
   {
      BDEBUG_ASSERT(mpEffect && (mHandle != cInvalidFXLHandle));
      mpEffect->GetAnnotationDesc(mHandle, &mDesc);
   }
};

//============================================================================
// class BFXLEffectPass
// Optional pass object, in case we want to get and use passes by name instead of using hardcoded indices in BFXLEffectTechnique.
//============================================================================
class BFXLEffectPass
{
public:
   BFXLEffectPass(FXLEffect* pEffect, FXLHANDLE handle);
   BFXLEffectPass(const BFXLEffectPass& other);
   BFXLEffectPass& operator= (const BFXLEffectPass& rhs);
   
   FXLHANDLE getHandle(void);
   FXLEffect* getEffect(void);
   
   void begin(void);
   void commit(void);
   void commitU(void);
   void end(void);     
   
   void getDesc(FXLPASS_DESC& desc); 
   uint getNumAnnotations(void);
   
   BFXLEffectAnnotation getAnnotation(const char* pName);
   BFXLEffectAnnotation getAnnotation(uint index);
   
   bool operator== (const BFXLEffectPass& other) const { return (mHandle == other.mHandle) && (mpEffect == other.mpEffect); }

private:
   FXLHANDLE mHandle;
   FXLEffect* mpEffect;
};

//============================================================================
// class BFXLEffectTechnique
//============================================================================
class BFXLEffectTechnique
{
public:
   BFXLEffectTechnique();
   BFXLEffectTechnique(FXLEffect* pEffect, FXLHANDLE handle);
   BFXLEffectTechnique(const BFXLEffectTechnique& other);
   BFXLEffectTechnique& operator= (const BFXLEffectTechnique& rhs);
   
   void clear(void);

   bool getValid(void);

   FXLHANDLE getHandle(void);
   BFXLEffect* getEffect(void);
   
   void begin(DWORD flags = 0);
   
   void beginRestoreDefaultState(void) { return begin(FXL_RESTORE_DEFAULT_STATE); }
   
   void beginPass(uint index = 0);

   void commit(void);
   void commitU(void);
   
   void endPass(void);
   
   void end(void);

   void getDesc(FXLTECHNIQUE_DESC& desc);
   const char* getName(void);
   uint getNumPasses(void);
   uint getNumAnnotations(void);
   
   // Optional pass methods, in case we want to get and use passes by name instead of hardcoded indices.
   BFXLEffectPass getPass(const char* pName);
   BFXLEffectPass getPassFromIndex(uint index);
   
   BFXLEffectAnnotation getAnnotation(const char* pName);
   BFXLEffectAnnotation getAnnotation(uint index);
      
private:
   FXLHANDLE mHandle;
   FXLEffect* mpEffect;      
};   

//============================================================================
// class BFXLEffectParamTemplate
//============================================================================
template<class T=FXLEffect>
class BFXLEffectParamTemplate
{
public:
   BFXLEffectParamTemplate();
   BFXLEffectParamTemplate(T* pEffect, FXLHANDLE handle);
   BFXLEffectParamTemplate(const BFXLEffectParamTemplate& other);
   BFXLEffectParamTemplate& operator= (const BFXLEffectParamTemplate& rhs);
   
   void clear(void);

   FXLHANDLE getHandle(void) const;
   
   T* getEffect(void) const;
   
   bool getValid(void) const;

   void getDesc(FXLPARAMETER_DESC& desc) const;

   bool isBool(void) const;
   bool isFloat(void) const;
   bool isIntVector(void) const;
   bool isVector(uint size = 4) const;
   bool isMatrix(uint rows = 4, uint cols = 4) const;
   
   bool isArray(uint num, uint expectedSize) const;
      
   bool isRowMatrix(uint rows = 4, uint cols = 4) const;
   bool isColMatrix(uint rows = 4, uint cols = 4) const;
   bool isSampler(void) const;
   bool isShared(void) const;
   
   // rg [1/29/06] - TODO: Support arrays

   // scalar gets
   operator bool(void) const;
   operator int(void) const;
   operator float(void) const;

   // vector gets
   operator D3DXVECTOR2(void) const;
   operator D3DXVECTOR3(void) const;
   operator D3DXVECTOR4(void) const;
   operator BVec2(void) const;
   operator BVec3(void) const;
   operator BVec4(void) const;
   void getInt4(int* pVal4) const;

   // matrix gets
   operator D3DXMATRIX(void) const;
   operator BMatrix44(void) const;

   // scalar sets
   BFXLEffectParamTemplate& operator= (bool val);
   BFXLEffectParamTemplate& operator= (int val);
   BFXLEffectParamTemplate& operator= (float val);
   BFXLEffectParamTemplate& setArray(const float* pValues, uint numValues);
   
   // vector sets
   BFXLEffectParamTemplate& operator= (const int* pVal4);
   BFXLEffectParamTemplate& operator= (const D3DXVECTOR2& val);
   BFXLEffectParamTemplate& operator= (const D3DXVECTOR3& val);
   BFXLEffectParamTemplate& operator= (const D3DXVECTOR4& val);
   BFXLEffectParamTemplate& operator= (const BVec2& val);
   BFXLEffectParamTemplate& operator= (const BVec3& val);
   BFXLEffectParamTemplate& operator= (const BVec4& val);
   BFXLEffectParamTemplate& operator= (const XMVECTOR val);
   
   
   BFXLEffectParamTemplate& setArray(const D3DXVECTOR4* pValues, uint numValues);
   BFXLEffectParamTemplate& setArray(const BVec4* pValues, uint numValues);
   BFXLEffectParamTemplate& setArray(const XMVECTOR* pValues, uint numValues);
      
   // matrix sets
   BFXLEffectParamTemplate& operator= (const D3DXMATRIX& val);
   BFXLEffectParamTemplate& operator= (const BMatrix44& val);
   BFXLEffectParamTemplate& operator= (const XMMATRIX& val);   
   
   // texture
   BFXLEffectParamTemplate& operator= (IDirect3DBaseTexture9* pTex);

   const char* getName(void) const;
   uint getNumAnnotations(void) const;
   
   void setRegisterUpdateMode(bool manualUpdate);
   
   FXLPARAMETER_CONTEXT getContext(BFXLEffectPass& pass) const;
   uint getRegister(BFXLEffectPass& pass, FXLPARAMETER_CONTEXT context) const;
   void getRegisters(BFXLEffectPass& pass, FXLPARAMETER_CONTEXT context, uint& index, uint& count) const;
         
private:
   FXLHANDLE mHandle;
   T* mpEffect;

   void ensureVectorType(FXLDATA_TYPE Type, uint cols) const;
   void ensureMatrixType(FXLDATA_TYPE Type, uint rows, uint cols) const;
};

typedef BFXLEffectParamTemplate<FXLEffect> BFXLEffectParam;
typedef BFXLEffectParamTemplate<FXLEffectPool> BFXLEffectPoolParam;

//============================================================================
// class BFXLEffectPool
//============================================================================
class BFXLEffectPool
{
   friend class BFXLEffectParamTemplate<FXLEffectPool>;

public:
   BFXLEffectPool();
   ~BFXLEffectPool();

   void create(void);
   void clear(void);
   
   // Calls pPool->AddRef(), so you must still call Release eventually.
   void attach(FXLEffectPool* pPool);

   FXLEffectPool* getEffectPool(void);

   BFXLEffectPoolParam operator()(PCSTR pName) const;

private:
   FXLEffectPool* mpEffectPool;

   BFXLEffectPool(const BFXLEffectPool&);
   BFXLEffectPool& operator= (const BFXLEffectPool&);
};

//============================================================================
// class BFXLEffect
//============================================================================
class BFXLEffect
{
   friend class BFXLEffectParamTemplate<FXLEffect>;
   friend class BFXLEffectTechnique;

public:
   BFXLEffect();
   
   // These methods will add a ref to the FXLEffect.
   BFXLEffect(const BFXLEffect& other);
   BFXLEffect& operator= (const BFXLEffect& rhs);
      
   ~BFXLEffect();
   
   void clear(void);

   HRESULT createFromCompiledData(PDIRECT3DDEVICE9 pDevice, void* pCompiledData, BFXLEffectIntrinsicPool* pPool = NULL, bool validateIntrinsics = true);
      
   // Calls pEffect->AddRef(), so you must still call Release eventually.
   void attach(FXLEffect* pEffect, BFXLEffectIntrinsicPool* pPool = NULL);
         
   FXLEffect* getEffect(void) const; 
      
   BFXLEffectIntrinsicPool* getEffectIntrinsicPool(void) const { return mIntrinsicMapper.getIntrinsicPool(); }
         
   uint getNumParameters(void);
   
   // You must be certain the parameter exists! If not, the Nov. XDK FX Light would crash. Not sure about later XDK's.
   BFXLEffectParam operator() (PCSTR pName);
   BFXLEffectParam getParam(PCSTR pName);
   
   BFXLEffectParam getParamFromIndex(uint index);
   
   // Returns cInvalidIndex if parameter cannot be found.
   // Scans through all effect parameters.
   int findParamIndex(PCSTR pName);
   
   // Returns cInvalidIndex if parameter cannot be found.
   // Scans through all effect parameters.
   int findParamRegister(PCSTR pName, BFXLEffectPass& pass, FXLPARAMETER_CONTEXT context, bool setManualRegisterUpdateMode = false);
   
   // Sets the parameter to manual update mode. Returns the index of the parameter, or cInvalidIndex if not found.
   // Scans through all effect parameters.
   int setParamToManualRegUpdate(PCSTR pName);
   
   // Sets all non-shared and non-intrinsic params to manual update mode.
   void setAllUserParamsToManualRegUpdate(void);
   
   // Returns cInvalidFXLHandle if the parameter cannot be found. 
   // Scans through all effect parameters.
   // Returns cInvalidFXLHandle if the parameter cannot be found.
   FXLHANDLE getParamHandle(PCSTR pName);
               
   uint getNumTechniques(void);   
   BFXLEffectTechnique getTechnique(PCSTR pName);
   BFXLEffectTechnique getTechniqueFromIndex(uint index = 0);
   
   void getDesc(FXLEFFECT_DESC& desc);
   
   const BEffectIntrinsicMapper& getIntrinsicMapper(void) const   { return mIntrinsicMapper; }
         BEffectIntrinsicMapper& getIntrinsicMapper(void)         { return mIntrinsicMapper; }

   // updateIntrinsicParams() MUST be called at least once per frame before beginning any effect techniques!
   // If force is set all local (i.e. NON-SHARED) intrinsic params will be set to the effect, even if they haven't changed since the last update.
   void updateIntrinsicParams(bool force = false);
                       
private:
   FXLEffect*              mpEffect;
   BEffectIntrinsicMapper  mIntrinsicMapper;
};

#include "effect.inl"



