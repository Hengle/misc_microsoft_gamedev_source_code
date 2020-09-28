//============================================================================
//
//  deviceStateDumper.cpp
//  
//  Copyright (c) 2004-2006, Ensemble Studios
//
//============================================================================
#pragma once

#ifndef BUILD_FINAL

struct BStateDesc;

class BD3DDeviceStateDumper
{
public:
   enum
   {
      cDumpRenderStates       = 1,

      cDumpSamplerStates      = 4,
      cDumpVShaderConstants   = 8,
      cDumpPShaderConstants   = 16,
      cDumpVShader            = 32,
      cDumpPShader            = 64,
      cDumpVertexDecl         = 128,
      cDumpRenderTargets      = 256,
      cDumpTextures           = 512,
      cDumpVShaderBoolConstants = 1024,
      cDumpPShaderBoolConstants = 2048,

      cDumpAllStates          = 0x7FFFFFFF
   };
   
   // This method dumps the actual device state to the logtrace - NOT BRender's cached state.
   static void dumpState(int dumpFlags = cDumpAllStates);
   static void print(const char* pFmt, ...);
         
private:
   enum { NumTextures                = 8 };
   enum { NumTexturesLog2            = 3 };
   enum { NumSamplers               = 16 };
   enum { NumSamplersLog2           = 4  };
   enum { MaxRenderTargets          = 4 };
   enum { MaxVertexShaderConstantsF = 256 };
   enum { MaxPixelShaderConstantsF = 32 };
   
   static int mCurIndent;

   template<typename GetFunc> 
   static void dumpStates(
      DWORD numGroups,
      const BStateDesc* pBStateDesc, 
      int numBStateDescs, 
      GetFunc getFunc);
         
   static void D3DErrCheck(HRESULT hres);      
      
   static void indent(int i);   
   
   static DWORD getRenderStateUncachedUntyped(DWORD s, DWORD t);
   static DWORD getTextureStageStateUncachedUntyped(DWORD s, DWORD t);
   static DWORD getSamplerStateUncachedUntyped(DWORD s, DWORD t);
   static int getVertexDeclarationTypeSize(DWORD type);
   static const char* getVertexDeclarationTypeName(DWORD type);
   static const char* getVertexDeclarationUsageName(DWORD usage);
   static void dumpVertexDeclaration(D3DVERTEXELEMENT9* pElements);
   static void dumpRenderStates(void);
   
   static void dumpSamplerStates(void);
   static void dumpVShaderConstants(void);
   static void dumpPShaderConstants(void);
   static void dumpVShaderBoolConstants(void);
   static void dumpPShaderBoolConstants(void);
   static void disassembleShader(const DWORD* pTokens);
   static void dumpVShader(void);
   static void dumpPShader(void);
   static void dumpVertexDecl(void);
   static const char* getFormatString(D3DFORMAT format);
   static void dumpSurfaceInfo(IDirect3DSurface9* pSurf);
   static void dumpRenderTargets(void);
   static void dumpTextures(void);
};
#endif