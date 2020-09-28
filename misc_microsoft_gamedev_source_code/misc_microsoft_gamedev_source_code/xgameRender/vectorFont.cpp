//-----------------------------------------------------------------------------
// File: vectorFont.cpp
//-----------------------------------------------------------------------------
#include "xgameRender.h"
#include "vectorFont.h"
#include "simplexFont.h"
#include "BD3D.h"

//-----------------------------------------------------------------------------
// struct BFontVert
//-----------------------------------------------------------------------------
struct BFontVert
{
   BVec3 position;       // vertex position
   D3DCOLOR diffuse;
};
#define D3DFVF_MODELVERTEX (D3DFVF_XYZ|D3DFVF_DIFFUSE)

//-----------------------------------------------------------------------------
// struct BRHWVertex
//-----------------------------------------------------------------------------
struct BRHWVertex
{
   BVec4 position;       // vertex position
   D3DCOLOR diffuse;
};
#define D3DFVF_RHWVERTEX (D3DFVF_XYZRHW|D3DFVF_DIFFUSE)

//-----------------------------------------------------------------------------
// BVectorFont::renderText
//-----------------------------------------------------------------------------
void BVectorFont::renderText(
   const char* pStr, 
   const BVec3& startPos, 
   const BVec3& up,
   const BVec3& right, 
   const DWORD color)
{
   if (!pStr)
      return;

   const int cNumVertexBufferElements = 128;
   BFontVert vertexBuffer[cNumVertexBufferElements];
   
   BD3D::mpDev->SetFVF(D3DFVF_MODELVERTEX);
   
   int vbNext = 0;

   BVec3 pos(startPos);
   BVec3 startLinePos(startPos);

   while (*pStr)
   {
      int c = *pStr;
      if (c == '\n')
      {
         pos = startLinePos + -up * 30.0f;
         startLinePos = pos;
      }
      else
      {
         if ((c < 32) || (c > 126)) 
            c = 32;

         const char* pSrc = &gSimplexFont[c - 32][0];
         const int l = pSrc[0];

         for (int n = 0; n < l; n++)
         {
            const float xOfs = pSrc[2+n*2+0];
            const float yOfs = pSrc[2+n*2+1];
            
            if (-1 == pSrc[2+n*2+0])
            {
               if (vbNext > 1)
                  BD3D::mpDev->DrawPrimitiveUP(D3DPT_LINESTRIP, vbNext - 1, &vertexBuffer[0], sizeof(BFontVert));
               vbNext = 0;
            }
            else
            {
               BDEBUG_ASSERT(vbNext < cNumVertexBufferElements);
               vertexBuffer[vbNext].position = pos + right * xOfs + up * yOfs;
               vertexBuffer[vbNext].diffuse = color;
               vbNext++;
            }
         }

         if (vbNext > 1)
            BD3D::mpDev->DrawPrimitiveUP(D3DPT_LINESTRIP, vbNext - 1, &vertexBuffer[0], sizeof(BFontVert));
         vbNext = 0;
         
         pos += right * pSrc[1];
      }         
            
      pStr++;
   }
}
