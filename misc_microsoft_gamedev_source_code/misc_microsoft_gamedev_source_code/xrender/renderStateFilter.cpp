//============================================================================
//
//  File: renderStateFilter.cpp
//  
//  Copyright (c) 2008, Ensemble Studios
//
//============================================================================
#include "xrender.h"
#include "BD3D.h"
#include "renderStateFilter.h"

//============================================================================
// Globals
//============================================================================
static uint8 gValidRenderStates[] =
{
   D3DRS_ZENABLE                    /2,   /* TRUE to enable Z buffering */
   D3DRS_ZFUNC                      /2,   /* D3DCMPFUNC */
   D3DRS_ZWRITEENABLE               /2,   /* TRUE to enable z writes */
   D3DRS_FILLMODE                   /2,   /* D3DFILLMODE */
   D3DRS_CULLMODE                   /2,   /* D3DCULL */
   D3DRS_ALPHABLENDENABLE           /2,   /* TRUE to enable alpha blending */
   D3DRS_SEPARATEALPHABLENDENABLE   /2,   /* TRUE to enable a separate blending function for the alpha channel */
   D3DRS_BLENDFACTOR                /2,   /* D3DCOLOR used for a constant blend factor during alpha blending for D3DBLEND_BLENDFACTOR, etc. */
   D3DRS_SRCBLEND                   /2,   /* D3DBLEND */
   D3DRS_DESTBLEND                  /2,   /* D3DBLEND */
   D3DRS_BLENDOP                    /2,   /* D3DBLENDOP setting */
   D3DRS_SRCBLENDALPHA              /2,   /* SRC blend factor for the alpha channel when D3DRS_SEPARATEDESTALPHAENABLE is TRUE */
   D3DRS_DESTBLENDALPHA             /2,   /* DST blend factor for the alpha channel when D3DRS_SEPARATEDESTALPHAENABLE is TRUE */
   D3DRS_BLENDOPALPHA               /2,   /* Blending operation for the alpha channel when D3DRS_SEPARATEDESTALPHAENABLE is TRUE */
   D3DRS_ALPHATESTENABLE            /2,   /* TRUE to enable alpha tests */
   D3DRS_ALPHAREF                   /2,  /* BYTE */
   D3DRS_ALPHAFUNC                  /2,  /* D3DCMPFUNC */
   D3DRS_STENCILENABLE              /2,  /* TRUE to enable stenciling */
   D3DRS_TWOSIDEDSTENCILMODE        /2,  /* TRUE to enable 2 sided stenciling */
   D3DRS_STENCILFAIL                /2,  /* D3DSTENCILOP to do if stencil test fails */
   D3DRS_STENCILZFAIL               /2,  /* D3DSTENCILOP to do if stencil test passes and Z test fails */
   D3DRS_STENCILPASS                /2,  /* D3DSTENCILOP to do if both stencil and Z tests pass */
   D3DRS_STENCILFUNC                /2,  /* D3DCMPFUNC stencilfn - stencil test passes if ((ref & mask) stencilfn (stencil & mask)) is true */
   D3DRS_STENCILREF                 /2,  /* BYTE reference value used in stencil test */
   D3DRS_STENCILMASK                /2,  /* BYTE mask value used in stencil test */
   D3DRS_STENCILWRITEMASK           /2,  /* BYTE write mask applied to values written to stencil buffer */
   D3DRS_CCW_STENCILFAIL            /2,  /* D3DSTENCILOP to do if CCW stencil test fails */
   D3DRS_CCW_STENCILZFAIL           /2,  /* D3DSTENCILOP to do if CCW stencil test passes and Z test fails */
   D3DRS_CCW_STENCILPASS            /2,  /* D3DSTENCILOP to do if both CCW stencil and Z tests pass */
   D3DRS_CCW_STENCILFUNC            /2,  /* D3DCMPFUNC stencilfn - CCW stencil test passes if ((ref & mask) stencilfn (stencil & mask)) is true */
   D3DRS_CCW_STENCILREF             /2,  /* BYTE reference value used in CCW stencil test */
   D3DRS_CCW_STENCILMASK            /2,  /* BYTE mask value used in CCW stencil test */
   D3DRS_CCW_STENCILWRITEMASK       /2,  /* BYTE write mask applied to CCW values written to stencil buffer */
   D3DRS_CLIPPLANEENABLE            /2,  /* TRUE to enable SetClipPlane */
   D3DRS_POINTSIZE                  /2,  /* FLOAT point size */
   D3DRS_POINTSIZE_MIN              /2,  /* FLOAT point size min threshold */
   D3DRS_POINTSPRITEENABLE          /2,  /* TRUE to enable point sprites */
   D3DRS_POINTSIZE_MAX              /2,  /* FLOAT point size max threshold */
   D3DRS_MULTISAMPLEANTIALIAS       /2,  /* TRUE to enable multisample antialiasing */
   D3DRS_MULTISAMPLEMASK            /2,  /* DWORD per-pixel and per-sample enable/disable */
   D3DRS_SCISSORTESTENABLE          /2,  /* TRUE to enable SetScissorRect */
   D3DRS_SLOPESCALEDEPTHBIAS        /2,  /* FLOAT depth-slope scaling bias */
   D3DRS_DEPTHBIAS                  /2,  /* FLOAT depth bias */
   D3DRS_COLORWRITEENABLE           /2,  /* D3DCOLORWRITEENABLE_ALPHA, etc. per-channel write enable */
   D3DRS_COLORWRITEENABLE1          /2,
   D3DRS_COLORWRITEENABLE2          /2,
   D3DRS_COLORWRITEENABLE3          /2,
   D3DRS_TESSELLATIONMODE           /2,  /* D3DTESSELLATIONMODE */
   D3DRS_MINTESSELLATIONLEVEL       /2,  /* FLOAT */
   D3DRS_MAXTESSELLATIONLEVEL       /2,  /* FLOAT */
   D3DRS_WRAP0                      /2,  /* D3DWRAPCOORD_0, etc. for 1st texture coord. set */
   D3DRS_WRAP1                      /2,
   D3DRS_WRAP2                      /2,
   D3DRS_WRAP3                      /2,
   D3DRS_WRAP4                      /2,
   D3DRS_WRAP5                      /2,
   D3DRS_WRAP6                      /2,
   D3DRS_WRAP7                      /2,
   D3DRS_WRAP8                      /2,
   D3DRS_WRAP9                      /2,
   D3DRS_WRAP10                     /2,
   D3DRS_WRAP11                     /2,
   D3DRS_WRAP12                     /2,
   D3DRS_WRAP13                     /2,
   D3DRS_WRAP14                     /2,
   D3DRS_WRAP15                     /2,

   // The following are Xbox 360 extensions:

   D3DRS_VIEWPORTENABLE             /2,  /* TRUE to enable viewport transformation */
   D3DRS_HIGHPRECISIONBLENDENABLE   /2,  /* TRUE to enable higher precision blending operations for 2_10_10_10 and 2_10_10_10_FLOAT */
   D3DRS_HIGHPRECISIONBLENDENABLE1  /2,  /*   render targets at the expense of running at half rate.  2_10_10_10 surfaces are expanded */
   D3DRS_HIGHPRECISIONBLENDENABLE2  /2,  /*   out to 10:10:10:10 before the blend and 2_10_10_10_FLOAT surfaces are expanded out to */
   D3DRS_HIGHPRECISIONBLENDENABLE3  /2,  /*   16:16:16:16 before the blend.  The default value is FALSE. */
   D3DRS_HALFPIXELOFFSET            /2,  /* TRUE to enable (0.5, 0.5) screen-space offset */
   D3DRS_PRIMITIVERESETENABLE       /2,  /* TRUE to enable primitive resets in indexed drawing.  The default value is FALSE. */
   D3DRS_PRIMITIVERESETINDEX        /2,  /* WORD Index reference value to trigger a primitive reset.  The default value is 0xFFFF. */
   D3DRS_ALPHATOMASKENABLE          /2,  /* TRUE to enable alpha to mask.  The default value is FALSE. */
   D3DRS_ALPHATOMASKOFFSETS         /2,  /* BYTE Packed offsets (2:2:2:2) to apply to the alpha value for each pixel in quad before it is converted to a mask. */
   D3DRS_GUARDBAND_X                /2,  /* FLOAT horizontal guard band factor */
   D3DRS_GUARDBAND_Y                /2,  /* FLOAT vertical guard band factor */
   D3DRS_DISCARDBAND_X              /2,  /* FLOAT horizontal discard band factor */
   D3DRS_DISCARDBAND_Y              /2,  /* FLOAT vertical discard band factor */
   D3DRS_HISTENCILENABLE            /2,  /* TRUE to enable early culling based on hi-stencil bit */
   D3DRS_HISTENCILWRITEENABLE       /2,  /* TRUE to enable update of hi-stencil bit based on hi-stencil test */
   D3DRS_HISTENCILFUNC              /2,  /* D3DHISTENCILCMPFUNC - bit is set to cull if (ref histencilfn stencil) is true */
   D3DRS_HISTENCILREF               /2,  /* BYTE reference value used in hi-stencil test */
   D3DRS_PRESENTINTERVAL            /2,  /* D3DPRESENT_INTERVAL_ONE, etc. */
   D3DRS_PRESENTIMMEDIATETHRESHOLD  /2,  /* BYTE percentage of DAC's progress in frame where a non-D3DPRESENT_INTERVAL_IMMEDIATE Present/Swap will be considered for immediate Present/Swap */
   D3DRS_HIZENABLE                  /2,  /* D3DHIZENABLEMODE that allows for manual control of hi-z enable */
   D3DRS_HIZWRITEENABLE             /2,  /* D3DHIZENABLEMODE that allows for manual control of hi-z write enable */
   D3DRS_LASTPIXEL                  /2,  /* TRUE to draw the last pixel of a line */
   D3DRS_LINEWIDTH                  /2,  /* FLOAT width of line */
   D3DRS_BUFFER2FRAMES              /2,  /* TRUE to enable D3DCREATE_BUFFER_2_FRAMES functionality */
};
static const uint cNumValidRenderStates = sizeof(gValidRenderStates)/sizeof(gValidRenderStates[0]);

//============================================================================
// BRenderStateFilter::BRenderStateFilter
//============================================================================
BRenderStateFilter::BRenderStateFilter(IDirect3DDevice9* pDev, uint firstVSReg, uint numVSRegs, uint firstPSReg, uint numPSRegs) :
   mpDev(BD3D::mpDev),
   mGpuOwned(false),   
   mFirstVSReg(firstVSReg), mNumVSRegs(numVSRegs),
   mFirstPSReg(firstPSReg), mNumPSRegs(numPSRegs),
   mpVertexShader(NULL), mpPixelShader(NULL),
   mShadersDirty(true),
   mpVSRegs(NULL), mpPSRegs(NULL),
   mVSGroupDirtyFlags(UINT64_MAX),
   mPSGroupDirtyFlags(UINT64_MAX),
   mFirstLiteralVSReg(0), mNumLiteralVSRegs(0),
   mFirstLiteralPSReg(0), mNumLiteralPSRegs(0),
   mCurRunPredication(0)
{
   BDEBUG_ASSERT(((mFirstVSReg & 3) == 0) && ((mFirstPSReg & 3) == 0));
   BDEBUG_ASSERT(((numVSRegs & 3) == 0) && ((numPSRegs & 3) == 0));
   
   Utils::ClearObj(mFlowConstants);
   Utils::ClearObj(mpTextures[cMaxD3DTextureSamplers]);
         
   if (mNumVSRegs)
      mpVSRegs = (XMVECTOR*)gRenderHeap.New(mNumVSRegs * sizeof(XMVECTOR), 0, true);
   
   if (mNumPSRegs)
      mpPSRegs = (XMVECTOR*)gRenderHeap.New(mNumPSRegs * sizeof(XMVECTOR), 0, true);

   Utils::ClearObj(mSamplerStates);      
   
   Utils::ClearObj(mRenderStates);
}

//============================================================================
// BRenderStateFilter::~BRenderStateFilter
//============================================================================
BRenderStateFilter::~BRenderStateFilter()
{
   gRenderHeap.Delete(mpVSRegs);
   gRenderHeap.Delete(mpPSRegs);
}

//============================================================================
// BRenderStateFilter::gpuOwn
//============================================================================
void BRenderStateFilter::gpuOwn(uint firstLiteralVSReg, uint numLiteralVSRegs, uint firstLiteralPSReg, uint numLiteralPSRegs)
{
   if (mGpuOwned)
      return;
      
   mFirstLiteralVSReg = firstLiteralVSReg;
   mNumLiteralVSRegs = numLiteralVSRegs;
   mFirstLiteralPSReg = firstLiteralPSReg;
   mNumLiteralPSRegs = numLiteralPSRegs;
   
   DWORD curStencilEnable; 
   mpDev->GetRenderState(D3DRS_STENCILENABLE, &curStencilEnable);                      
   
   mpDev->SetRenderState(D3DRS_HIZENABLE, curStencilEnable ? D3DHIZ_DISABLE : D3DHIZ_ENABLE);
   
   mpDev->SetPixelShader(NULL);
   mpDev->SetVertexShader(NULL);
            
   mpDev->GpuOwnShaders();
   
   if (numLiteralVSRegs)
      mpDev->GpuOwnVertexShaderConstantF(mFirstLiteralVSReg, mNumLiteralVSRegs);
      
   if (numLiteralPSRegs)
      mpDev->GpuOwnPixelShaderConstantF(mFirstLiteralPSReg, mNumLiteralPSRegs);
      
   if (mNumVSRegs)
      mpDev->GpuOwnVertexShaderConstantF(mFirstVSReg, mNumVSRegs);
      
   if (mNumPSRegs)
      mpDev->GpuOwnPixelShaderConstantF(mFirstPSReg, mNumPSRegs);
   
   mGpuOwned = true;
   
   if (mNumVSRegs) Utils::FastMemSet(mpVSRegs, 0, mNumVSRegs * sizeof(XMVECTOR));
   if (mNumPSRegs) Utils::FastMemSet(mpPSRegs, 0, mNumPSRegs * sizeof(XMVECTOR));
   
   mpPixelShader = NULL;
   mpVertexShader = NULL;
   Utils::ClearObj(mFlowConstants);
   
   resetDeferredState();
   
   int intConstants[16*4];

   mpDev->GetVertexShaderConstantI(0, intConstants, 16);
   setVertexShaderConstantI(0, intConstants, 16);

   mpDev->GetPixelShaderConstantI(0, intConstants, 16);
   setPixelShaderConstantI(0, intConstants, 16);

   BOOL boolConstants[128];

   mpDev->GetVertexShaderConstantB(0, boolConstants, 128);
   setVertexShaderConstantB(0, boolConstants, 128);

   mpDev->GetPixelShaderConstantB(0, boolConstants, 128);
   setPixelShaderConstantB(0, boolConstants, 128);
   
   for (uint i = 0; i < cMaxD3DTextureSamplers; i++)
   {
      mpDev->GetTexture(i, &mpTextures[i]);
      if (mpTextures[i])
      {
         mpTextures[i]->Release();
         mpDev->SetTexture(i, NULL);         
      }
   }

   for (uint i = 0; i < cMaxD3DTextureSamplers; i++)
      for (uint j = 0; j < (D3DSAMP_MAX >> 2U); j++)
         mpDev->GetSamplerState(i, (D3DSAMPLERSTATETYPE)(j << 2U), &mSamplerStates[i][j]);
        
   for (uint i = 0; i < cNumValidRenderStates; i++)
   {
      const uint rsIndex = gValidRenderStates[i];
      D3DRENDERSTATETYPE rs = (D3DRENDERSTATETYPE)(rsIndex << 1U);
      mpDev->GetRenderState(rs, &mRenderStates[rsIndex]);
   }
         
}

//============================================================================
// BRenderStateFilter::gpuDisown
//============================================================================
void BRenderStateFilter::gpuDisown(void)
{
   if (!mGpuOwned)
      return;
      
   mpDev->GpuDisownShaders();
   
   if (mNumLiteralVSRegs)
      mpDev->GpuDisownVertexShaderConstantF(mFirstLiteralVSReg, mNumLiteralVSRegs);

   if (mNumLiteralPSRegs)
      mpDev->GpuDisownPixelShaderConstantF(mFirstLiteralPSReg, mNumLiteralPSRegs);

   if (mNumVSRegs)
      mpDev->GpuDisownVertexShaderConstantF(mFirstVSReg, mNumVSRegs);

   if (mNumPSRegs)
      mpDev->GpuDisownPixelShaderConstantF(mFirstPSReg, mNumPSRegs);

   mpDev->SetRenderState(D3DRS_HIZENABLE, D3DHIZ_AUTOMATIC);
   
   mpDev->SetPixelShader(NULL);
   mpDev->SetVertexShader(NULL);
      
   mGpuOwned = false;
}

//============================================================================
// BRenderStateFilter::resetDeferredState
//============================================================================
void BRenderStateFilter::resetDeferredState(void)
{
   mShadersDirty = true;
      
   mVSGroupDirtyFlags = UINT64_MAX;
   mPSGroupDirtyFlags = UINT64_MAX;
            
   mCurRunPredication = 0;
}

//============================================================================
// BRenderStateFilter::flushDeferredState
//============================================================================
void BRenderStateFilter::flushDeferredState(void)
{
   if (mVSGroupDirtyFlags)
   {
      if (mNumVSRegs)
      {
         const uint totalGroups = mNumVSRegs >> 2;
         
         int firstGroupIndex = -1, lastGroupIndex = -1;
         uint64 dirtyMask = mVSGroupDirtyFlags;
         
         D3DVECTOR4* pConstantData;
         for (uint groupIndex = 0; groupIndex < totalGroups; groupIndex++)
         {
            if (dirtyMask & 1)
            {
               if (firstGroupIndex < 0)
               {
                  firstGroupIndex = groupIndex;
                  lastGroupIndex = groupIndex;
               }
               else 
                  lastGroupIndex = groupIndex;  
            }
            else if (firstGroupIndex >= 0)
            {
               const uint totalRegs = (lastGroupIndex - firstGroupIndex + 1) << 2;
               HRESULT hres = mpDev->GpuBeginVertexShaderConstantF4(mFirstVSReg + (firstGroupIndex << 2), &pConstantData, totalRegs);
               if (SUCCEEDED(hres))
               {
                  memcpy(pConstantData, mpVSRegs + (firstGroupIndex << 2), totalRegs * sizeof(XMVECTOR));
                  mpDev->GpuEndVertexShaderConstantF4();
               }
               firstGroupIndex = -1;
            }
            
            dirtyMask >>= 1;
            if (!dirtyMask)
               break;
         }
         
         if (firstGroupIndex >= 0)
         {
            const uint totalRegs = (lastGroupIndex - firstGroupIndex + 1) << 2;
            HRESULT hres = mpDev->GpuBeginVertexShaderConstantF4(mFirstVSReg + (firstGroupIndex << 2), &pConstantData, totalRegs);
            if (SUCCEEDED(hres))
            {
               memcpy(pConstantData, mpVSRegs + (firstGroupIndex << 2), totalRegs * sizeof(XMVECTOR));
               mpDev->GpuEndVertexShaderConstantF4();
            }
         }
      }
      
      mVSGroupDirtyFlags = 0;
   }
   
   if (mPSGroupDirtyFlags)
   {
      if (mNumPSRegs)
      {
         const uint totalGroups = mNumPSRegs >> 2;

         int firstGroupIndex = -1, lastGroupIndex = -1;
         uint64 dirtyMask = mPSGroupDirtyFlags;

         D3DVECTOR4* pConstantData;
         for (uint groupIndex = 0; groupIndex < totalGroups; groupIndex++)
         {
            if (dirtyMask & 1)
            {
               if (firstGroupIndex < 0)
               {
                  firstGroupIndex = groupIndex;
                  lastGroupIndex = groupIndex;
               }
               else 
                  lastGroupIndex = groupIndex;  
            }
            else if (firstGroupIndex >= 0)
            {
               const uint totalRegs = (lastGroupIndex - firstGroupIndex + 1) << 2;
               HRESULT hres = mpDev->GpuBeginPixelShaderConstantF4(mFirstPSReg + (firstGroupIndex << 2), &pConstantData, totalRegs);
               if (SUCCEEDED(hres))
               {
                  memcpy(pConstantData, mpPSRegs + (firstGroupIndex << 2), totalRegs * sizeof(XMVECTOR));
                  mpDev->GpuEndPixelShaderConstantF4();
               }
               firstGroupIndex = -1;
            }

            dirtyMask >>= 1;
            if (!dirtyMask)
               break;
         }

         if (firstGroupIndex >= 0)
         {
            const uint totalRegs = (lastGroupIndex - firstGroupIndex + 1) << 2;
            HRESULT hres = mpDev->GpuBeginPixelShaderConstantF4(mFirstPSReg + (firstGroupIndex << 2), &pConstantData, totalRegs);
            if (SUCCEEDED(hres))
            {
               memcpy(pConstantData, mpPSRegs + (firstGroupIndex << 2), totalRegs * sizeof(XMVECTOR));
               mpDev->GpuEndPixelShaderConstantF4();
            }
         }
      }

      mPSGroupDirtyFlags = 0;
   }
   
   if (mShadersDirty)
   {
      mpDev->GpuLoadShaders(mpVertexShader, mpPixelShader, &mFlowConstants);
      mShadersDirty = false;
   }
}

//============================================================================
// BRenderStateFilter::setVertexShaderConstantF
//============================================================================
void BRenderStateFilter::setVertexShaderConstantF(UINT StartRegister, CONST float* pConstantData, DWORD Vector4fCount)
{
   BDEBUG_ASSERT((StartRegister >= mFirstVSReg) && ((StartRegister + Vector4fCount) <= (mFirstVSReg + mNumVSRegs)));
   
   DWORD* pCurState = (DWORD*)(&mpVSRegs[StartRegister - mFirstVSReg]);
   
   for (uint i = Vector4fCount; i > 0; i--, StartRegister++, pCurState += 4, pConstantData += 4)
   {
      DWORD x = ((const DWORD*)pConstantData)[0], y = ((const DWORD*)pConstantData)[1], z = ((const DWORD*)pConstantData)[2], w = ((const DWORD*)pConstantData)[3];
      
      if ((x != pCurState[0]) || (y != pCurState[1]) || (z != pCurState[2]) || (w != pCurState[3]))
      {
         pCurState[0] = x;
         pCurState[1] = y;
         pCurState[2] = z;
         pCurState[3] = w;
         
         setVSGroupDirtyFlag(StartRegister);
      }
   }
}

//============================================================================
// BRenderStateFilter::setPixelShaderConstantF
//============================================================================
void BRenderStateFilter::setPixelShaderConstantF(UINT StartRegister, CONST float* pConstantData, DWORD Vector4fCount)
{
   BDEBUG_ASSERT((StartRegister >= mFirstPSReg) && ((StartRegister + Vector4fCount) <= (mFirstPSReg + mNumPSRegs)));

   DWORD* pCurState = (DWORD*)(&mpPSRegs[StartRegister - mFirstPSReg]);

   for (uint i = Vector4fCount; i > 0; i--, StartRegister++, pCurState += 4, pConstantData += 4)
   {
      DWORD x = ((const DWORD*)pConstantData)[0], y = ((const DWORD*)pConstantData)[1], z = ((const DWORD*)pConstantData)[2], w = ((const DWORD*)pConstantData)[3];

      if ((x != pCurState[0]) || (y != pCurState[1]) || (z != pCurState[2]) || (w != pCurState[3]))
      {
         pCurState[0] = x;
         pCurState[1] = y;
         pCurState[2] = z;
         pCurState[3] = w;

         setPSGroupDirtyFlag(StartRegister);
      }
   }
}

//============================================================================
// BRenderStateFilter::setPixelShaderConstantF
//============================================================================
void BRenderStateFilter::setZeroPixelShaderConstantF(UINT StartRegister, DWORD Vector4fCount)
{
   BDEBUG_ASSERT((StartRegister >= mFirstPSReg) && ((StartRegister + Vector4fCount) <= (mFirstPSReg + mNumPSRegs)));

   DWORD* pCurState = (DWORD*)(&mpPSRegs[StartRegister - mFirstPSReg]);

   for (uint i = Vector4fCount; i > 0; i--, StartRegister++, pCurState += 4)
   {
      if ((0 != pCurState[0]) || (0 != pCurState[1]) || (0 != pCurState[2]) || (0 != pCurState[3]))
      {
         pCurState[0] = 0;
         pCurState[1] = 0;
         pCurState[2] = 0;
         pCurState[3] = 0;

         setPSGroupDirtyFlag(StartRegister);
      }
   }
}

//============================================================================
// BRenderStateFilter::setPixelShaderConstantF
//============================================================================
void BRenderStateFilter::setCommandBufferPredication(DWORD TilePredication, DWORD RunPredication)
{
   BDEBUG_ASSERT(!TilePredication);
   
   if (RunPredication != mCurRunPredication)
   {
      resetDeferredState();
   
      mpDev->SetCommandBufferPredication(0, RunPredication);      
      
      mCurRunPredication = RunPredication;
   }
}

