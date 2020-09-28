// File: triangleRenderer.h
#pragma once

#include "dynamicTexture.h"
#include "textureManager.h"

class BTriangleRenderer
{
public:
   BTriangleRenderer();
   ~BTriangleRenderer();
   
   bool init(void);
   bool deinit(void);
   void render(const void* pData);
   
private:
   IDirect3DPixelShader9*  mpPixelShader;
   IDirect3DVertexShader9* mpVertexShader;
   IDirect3DVertexDeclaration9* mpVertexDecl;   
   BTextureHandle mTexture;
};
