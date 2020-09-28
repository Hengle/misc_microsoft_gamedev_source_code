//============================================================================
// File: fixedFuncShaders.h
//
// Copyright (c) 2006 Ensemble Studios
//============================================================================
#pragma once

#include "renderCommand.h"

//============================================================================
// Fixed func shader constants
//============================================================================
enum eFixedFuncShaderIndex
{
   // All vertex shaders transform the incoming position by c0,c1,c2,c3 and minimally expect a position and color in the stream.
   // The matrix in c0-c3 must be transposed.
   cPosVS,
   cPosTex1VS,
   cPosDiffuseVS,   
   cPosDiffuseTex1VS,
   cPosDiffuseTex2VS,
   cPosConstantDiffuseVS,
   
   cFirstPSIndex,

   // Iterated diffuse only
   cDiffusePS        = cFirstPSIndex,

   // Iterated diffuse * tex0
   cDiffuseTex1PS,

   // Iterated diffuse * tex0 * tex1
   cDiffuseTex2PS,

   // // Iterated diffuse * tex0 * 2.0f
   cDiffuseTex1PSOverBright, 

   // White
   cWhitePS,

   // tex0
   cTex1PS,

   // tex0 * tex1
   cTex2PS,
   
   cDepthVisPS,
   cAlphaVisPS,
   cRedVisPS,
   cGreenVisPS,
   cBlueVisPS,

   cNumFixedFuncShaders,

   cFixedFuncForceDWORD = 0x7FFFFFFF
};

//============================================================================
// class BFixedFuncShaders
//============================================================================
class BFixedFuncShaders : public BRenderCommandListenerInterface
{
public:
   BFixedFuncShaders();
   ~BFixedFuncShaders();
   
   // init/deinit are called from the main thread to register/unregister the object with gRenderThread.
   void init(void);
   void deinit(void);
         
   // Callable from the main or worker threads.
   void set(eFixedFuncShaderIndex index);   
   
   // Callable from the main or worker threads.
   void set(eFixedFuncShaderIndex vsIndex, eFixedFuncShaderIndex psIndex);
   
   // Callable from the worker thread only.
   IDirect3DVertexShader9* workerGetVertexShader(eFixedFuncShaderIndex shaderIndex);
   IDirect3DPixelShader9* workerGetPixelShader(eFixedFuncShaderIndex shaderIndex);
      
   // Callable from worker thread only.
   void workerSet(eFixedFuncShaderIndex index);   
   
   // Callable from the worker thread.
   void setIdentityTransform(void);
   
private:
   // mpFixedFuncShaders and mInitialized are read/written from the worker thread only.
   void** mpFixedFuncShaders;
   
   // Handle is managed by the main thread.   
   BCommandListenerHandle mCommandListenerHandle; 
   
   bool mInitialized : 1;
         
   bool initFixedFuncShaders(void);
   void deinitFixedFuncShaders(void);
   void frameBegin(void);
   void frameEnd(void);
   
   virtual void initDeviceData(void);
   virtual void processCommand(const BRenderCommandHeader& header, const uchar* pData);
   virtual void deinitDeviceData(void);
};

//============================================================================
// Externs
//============================================================================
extern BFixedFuncShaders gFixedFuncShaders;
