//============================================================================
// File: fixedFuncShaders.cpp
//
// Copyright (c) 2006 Ensemble Studios
//============================================================================
#include "xrender.h"
#include "fixedFuncShaders.h"
#include "renderThread.h"
#include "BD3D.h"

#include "fixedFuncVSPos.hlsl.h"
#include "fixedFuncVSPosConstantDiffuse.hlsl.h"
#include "fixedFuncVSPosDiffuse.hlsl.h"
#include "fixedFuncVSPosDiffuseTex1.hlsl.h"
#include "fixedFuncVSPosDiffuseTex2.hlsl.h"
#include "fixedFuncVSPosTex1.hlsl.h"
#include "fixedFuncPSAlphaVis.hlsl.h"
#include "fixedFuncPSBlueVis.hlsl.h"
#include "fixedFuncPSDepthVis.hlsl.h"
#include "fixedFuncPSDiffuse.hlsl.h"
#include "fixedFuncPSDiffuseTex1.hlsl.h"
#include "fixedFuncPSDiffuseTex1OverBright.hlsl.h"
#include "fixedFuncPSDiffuseTex2.hlsl.h"
#include "fixedFuncPSGreenVis.hlsl.h"
#include "fixedFuncPSRedVis.hlsl.h"
#include "fixedFuncPSTex1.hlsl.h"
#include "fixedFuncPSTex2.hlsl.h"
#include "fixedFuncPSWhite.hlsl.h"

//============================================================================
// Globals
//============================================================================
BFixedFuncShaders gFixedFuncShaders;

//============================================================================
// Shader definitions
//============================================================================
namespace 
{   
   struct BFixedFuncShaderDef
   {
      long mID;

      const DWORD* mpShader;
   };

   static BFixedFuncShaderDef mFixedFuncDefs[] = 
   {
      { cPosVS, g_xvs_PosVS },
      { cPosTex1VS, g_xvs_PosTex1VS },
      { cPosDiffuseVS, g_xvs_PosDiffuseVS },
      { cPosDiffuseTex1VS, g_xvs_PosDiffuseTex1VS },
      { cPosDiffuseTex2VS, g_xvs_PosDiffuseTex2VS },
      { cPosConstantDiffuseVS, g_xvs_PosConstantDiffuseVS },

      { cDiffusePS, g_xps_DiffusePS },
      { cDiffuseTex1PS, g_xps_DiffuseTex1PS },
      { cDiffuseTex2PS, g_xps_DiffuseTex2PS },
      { cDiffuseTex1PSOverBright, g_xps_DiffuseTex1PSOverBright },

      { cWhitePS, g_xps_WhitePS },
      { cTex1PS, g_xps_Tex1PS },
      { cTex2PS, g_xps_Tex2PS },
      
      { cDepthVisPS, g_xps_DepthVisPS },
      { cAlphaVisPS, g_xps_AlphaVisPS },
      { cRedVisPS, g_xps_RedVisPS },
      { cGreenVisPS, g_xps_GreenVisPS },
      { cBlueVisPS, g_xps_BlueVisPS }
   };

   const int cNumFixedFuncDefs = sizeof(mFixedFuncDefs) / sizeof(mFixedFuncDefs[0]);
   
} // anonymous namespace   

//============================================================================
// BFixedFuncShaders::BFixedFuncShaders
//============================================================================
BFixedFuncShaders::BFixedFuncShaders() :
   mpFixedFuncShaders(NULL),
   mCommandListenerHandle(cInvalidCommandListenerHandle),
   mInitialized(false)
{
}

//============================================================================
// BFixedFuncShaders::~BFixedFuncShaders
//============================================================================
BFixedFuncShaders::~BFixedFuncShaders()
{
}

//============================================================================
// BFixedFuncShaders::init
//============================================================================
void BFixedFuncShaders::init(void)
{
   ASSERT_MAIN_THREAD

   if (mCommandListenerHandle == cInvalidCommandListenerHandle)
   {
      mCommandListenerHandle = gRenderThread.registerCommandListener(this);
   }
}   

//============================================================================
// BFixedFuncShaders::deinit
//============================================================================
void BFixedFuncShaders::deinit(void)
{
   ASSERT_MAIN_THREAD
   
   if (mCommandListenerHandle != cInvalidCommandListenerHandle)
   {
      gRenderThread.freeCommandListener(mCommandListenerHandle);
      mCommandListenerHandle = cInvalidCommandListenerHandle;
   }
}

//============================================================================
// BFixedFuncShaders::initDeviceData
//============================================================================
void BFixedFuncShaders::initDeviceData(void)
{
   initFixedFuncShaders();
}

//============================================================================
// BFixedFuncShaders::deinitDeviceData
//============================================================================
void BFixedFuncShaders::deinitDeviceData(void)
{
   deinitFixedFuncShaders();
}

//============================================================================
// BFixedFuncShaders::processCommand
//============================================================================
void BFixedFuncShaders::processCommand(const BRenderCommandHeader& header, const uchar* pData)
{
   BDEBUG_ASSERT(header.mLen == sizeof(eFixedFuncShaderIndex));
   
   workerSet(*reinterpret_cast<const eFixedFuncShaderIndex*>(pData));
}

//============================================================================
// BFixedFuncShaders::initFixedFuncShaders
//============================================================================
bool BFixedFuncShaders::initFixedFuncShaders(void)
{
   deinitFixedFuncShaders();

   mpFixedFuncShaders = new void* [cNumFixedFuncDefs];

   for (int shaderIndex = 0; shaderIndex < cNumFixedFuncDefs; shaderIndex++)
   {
      const BFixedFuncShaderDef& def = mFixedFuncDefs[shaderIndex];

      const bool vertexShader = (shaderIndex < cFirstPSIndex);

      BASSERT(def.mpShader);

      HRESULT hres;

      if (vertexShader)
         hres = BD3D::mpDev->CreateVertexShader(def.mpShader,reinterpret_cast<IDirect3DVertexShader9**>(&mpFixedFuncShaders[shaderIndex]));
      else
         hres = BD3D::mpDev->CreatePixelShader(def.mpShader, reinterpret_cast<IDirect3DPixelShader9**>(&mpFixedFuncShaders[shaderIndex]));

      if (FAILED(hres))
      {
         gRenderThread.panic("BRender::initFixedFuncShaders: Unable to create shader");
//         return false;
      }
   }
   
   mInitialized = true;

   return true;
}

//============================================================================
// BFixedFuncShaders::deinitFixedFuncShaders
//============================================================================
void BFixedFuncShaders::deinitFixedFuncShaders(void)
{
   mInitialized = false;
   
   if (mpFixedFuncShaders)
   {
      for (int shaderIndex = 0; shaderIndex < cNumFixedFuncDefs; shaderIndex++)
      {
         if (mpFixedFuncShaders[shaderIndex])
         {
            if (shaderIndex < cFirstPSIndex)
               reinterpret_cast<IDirect3DVertexShader9*>(mpFixedFuncShaders[shaderIndex])->Release();
            else
               reinterpret_cast<IDirect3DPixelShader9*>(mpFixedFuncShaders[shaderIndex])->Release();

            mpFixedFuncShaders[shaderIndex] = NULL;            
         }
      }      

      delete [] mpFixedFuncShaders;
      mpFixedFuncShaders = NULL;
   }      
}

//============================================================================
// BFixedFuncShaders::set
//============================================================================
void BFixedFuncShaders::set(eFixedFuncShaderIndex index)
{
   BASSERT(index < cNumFixedFuncDefs);
   ASSERT_MAIN_OR_WORKER_THREAD
   if (GetCurrentThreadId() == gRenderThread.getWorkerThreadId())
      workerSet(index);
   else
      gRenderThread.submitCommand(mCommandListenerHandle, 0, index);   
}

//============================================================================
// BFixedFuncShaders::set
//============================================================================
void BFixedFuncShaders::set(eFixedFuncShaderIndex vsIndex, eFixedFuncShaderIndex psIndex)
{
   set(vsIndex);
   set(psIndex);
}

//============================================================================
// BFixedFuncShaders::workerSet
//============================================================================
void BFixedFuncShaders::workerSet(eFixedFuncShaderIndex index)
{
   ASSERT_RENDER_THREAD
   BASSERT(index < cNumFixedFuncDefs);
   BASSERT(mpFixedFuncShaders[index]);

   if (index < cFirstPSIndex)
      BD3D::mpDev->SetVertexShader(reinterpret_cast<IDirect3DVertexShader9*>(mpFixedFuncShaders[index]));
   else
      BD3D::mpDev->SetPixelShader(reinterpret_cast<IDirect3DPixelShader9*>(mpFixedFuncShaders[index]));
}

//============================================================================
// BFixedFuncShaders::workerGetVertexShader
// Callable from any thread once initialized.
//============================================================================
IDirect3DVertexShader9* BFixedFuncShaders::workerGetVertexShader(eFixedFuncShaderIndex shaderIndex)
{
   // This seems safe
   //ASSERT_NOT_MAIN_THREAD
   BDEBUG_ASSERT(mInitialized);
   BDEBUG_ASSERT(shaderIndex < cFirstPSIndex);
   
   return reinterpret_cast<IDirect3DVertexShader9*>(mpFixedFuncShaders[shaderIndex]);
}

//============================================================================
// BFixedFuncShaders::workerGetPixelShader
// Callable from any thread once initialized.
//============================================================================
IDirect3DPixelShader9* BFixedFuncShaders::workerGetPixelShader(eFixedFuncShaderIndex shaderIndex)
{
   // This seems safe
   //ASSERT_NOT_MAIN_THREAD
   BDEBUG_ASSERT(mInitialized);
   BDEBUG_ASSERT((shaderIndex >= cFirstPSIndex) && (shaderIndex < cNumFixedFuncShaders));
   return reinterpret_cast<IDirect3DPixelShader9*>(mpFixedFuncShaders[shaderIndex]);
}

void BFixedFuncShaders::frameBegin(void)
{
}

void BFixedFuncShaders::frameEnd(void)
{
}

void BFixedFuncShaders::setIdentityTransform(void)
{
   ASSERT_RENDER_THREAD
   
   XMMATRIX xform = XMMatrixIdentity();   
   BD3D::mpDev->SetVertexShaderConstantF(0, reinterpret_cast<float*>(&xform), 4);
}
