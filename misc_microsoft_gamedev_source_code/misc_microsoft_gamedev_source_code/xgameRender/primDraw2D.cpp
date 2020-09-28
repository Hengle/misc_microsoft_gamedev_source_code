//============================================================================
//
//  primDraw2D.cpp
//
//  Copyright (c) 1997-2001, Ensemble Studios
//
//  rg [2/17/06] - Ported from Phoenix codebase
//
//============================================================================
#include "xgameRender.h"
#include "primDraw2D.h"
#include "render.h"

//============================================================================
// BPrimDraw2D::setTransformScreenspace
//============================================================================
void BPrimDraw2D::setTransformScreenspace(void)
{
   gRenderDraw.setRenderState(D3DRS_VIEWPORTENABLE, FALSE);

   D3DXMATRIX identity;
   D3DXMatrixIdentity(&identity);
   gRenderDraw.setVertexShaderConstantF(0, (const float*)&identity, 4);
}

//============================================================================
// BPrimDraw2D::resetTransformScreenspace
//============================================================================
void BPrimDraw2D::resetTransformScreenspace(void)
{
   gRenderDraw.setRenderState(D3DRS_VIEWPORTENABLE, TRUE);
}

//============================================================================
// BPrimDraw2D::setTransformWorldspace
//============================================================================
void BPrimDraw2D::setTransformWorldspace(void)
{
   XMMATRIX worldMatrix;
   XMMATRIX worldToProj;
   
   if (GetCurrentThreadId() == gRenderThread.getWorkerThreadId())
   {
      worldMatrix = XMMatrixIdentity();
      worldToProj = gRenderDraw.getWorkerActiveMatrixTracker().getMatrix(cMTWorldToProj, false);
   }
   else
   {
      worldMatrix = gRender.getWorldXMMatrix();
      worldToProj = gRenderDraw.getMainActiveMatrixTracker().getMatrix(cMTWorldToProj, false);
   }
   
   XMMATRIX matrix = XMMatrixMultiplyTranspose(worldMatrix, worldToProj );
      
   gRenderDraw.setVertexShaderConstantF(0, reinterpret_cast<const float*>(&matrix), 4);
}

//============================================================================
// BPrimDraw2D::drawLine2D
//============================================================================
void BPrimDraw2D::drawLine2D(int x0, int y0, int x1, int y1, DWORD color1, DWORD color2)
{
   // Lock the dynamic transformed/lit vb.
   BPosDiffuseTLVertex *v=(BPosDiffuseTLVertex*)gRenderDraw.lockDynamicVB(2, sizeof(v[0]));

   // Poke in our data.
   v[0].x=float(x0);
   v[0].y=float(y0);
   v[0].z=0.0f;
   v[0].rhw=1.0f;
   v[0].diffuse=color1;

   v[1].x=float(x1);
   v[1].y=float(y1);
   v[1].z=0.0f;
   v[1].rhw=1.0f;
   v[1].diffuse=color2;

   // Set the stream source to the dynamic vb.
   gRenderDraw.setStreamSource(0, gRenderDraw.getDynamicVB(), 0, sizeof(v[0]));
   
   // We're done copying in data, so unlock.
   gRenderDraw.unlockDynamicVB();
      
   gFixedFuncShaders.set(cPosDiffuseVS, cDiffusePS);
   
   setTransformScreenspace();
      
   gRenderDraw.setVertexDeclaration(v[0].msVertexDecl);

   // Draw the line.
   gRenderDraw.drawVertices(D3DPT_LINELIST, 0, 2);

   resetTransformScreenspace();
   
   gRenderDraw.clearStreamSource(0);
}


//============================================================================
// BPrimDraw2D::drawSolidRect2D
//============================================================================
void BPrimDraw2D::drawSolidRect2D(int x0, int y0, int x1, int y1, DWORD color, DWORD specular)
{  
   drawSolidRect2D(x0, y0, x1, y1, 0.0f, 0.0f, 1.0f, 1.0f, color, specular, cPosDiffuseTex1VS, cDiffuseTex1PS);
}

//============================================================================
// BPrimDraw2D::drawSolidRect2D
//============================================================================
void BPrimDraw2D::drawSolidRect2D(
   int x0, int y0, int x1, int y1, 
   float u0, float v0, float u1, float v1, 
   DWORD color, DWORD specular,
   eFixedFuncShaderIndex vs, eFixedFuncShaderIndex ps)
{
   // Lock the dynamic transformed/lit vb.
   BTLVertex *v=(BTLVertex*)gRenderDraw.lockDynamicVB(4, sizeof(v[0]));

   // Poke in our data.
   v[0].x=x1-0.5f;
   v[0].y=y1-0.5f;
   v[0].z=0.0f;
   v[0].rhw=1.0f;
   v[0].diffuse=color;
   v[0].specular=specular;
   v[0].tu=u1;
   v[0].tv=v1;

   v[1].x=x0-0.5f;
   v[1].y=y1-0.5f;
   v[1].z=0.0f;
   v[1].rhw=1.0f;
   v[1].diffuse=color;
   v[1].specular=specular;
   v[1].tu=u0;
   v[1].tv=v1;

   v[2].x=x1-0.5f;
   v[2].y=y0-0.5f;
   v[2].z=0.0f;
   v[2].rhw=1.0f;
   v[2].diffuse=color;
   v[2].specular=specular;
   v[2].tu=u1;
   v[2].tv=v0;

   v[3].x=x0-0.5f;
   v[3].y=y0-0.5f;
   v[3].z=0.0f;
   v[3].rhw=1.0f;
   v[3].diffuse=color;
   v[3].specular=specular;
   v[3].tu=u0;
   v[3].tv=v0;

   // Set the stream source to the dynamic vb.
   gRenderDraw.setStreamSource(0, gRenderDraw.getDynamicVB(), 0, sizeof(v[0]));
   
   // We're done copying in data, so unlock.
   gRenderDraw.unlockDynamicVB();
   
   gFixedFuncShaders.set(vs, ps);
   
   setTransformScreenspace();

   gRenderDraw.setVertexDeclaration(v[0].msVertexDecl);

   gRenderDraw.drawVertices(D3DPT_TRIANGLESTRIP, 0, 4);
   
   resetTransformScreenspace();

   gRenderDraw.setRenderState(D3DRS_VIEWPORTENABLE, TRUE);
   gRenderDraw.clearStreamSource(0);
}


//============================================================================
// BPrimDraw2D::drawSolidRect2DTexCoord
//============================================================================
void BPrimDraw2D::drawSolidRect2DTexCoord(int x0, int y0, int x1, int y1, BVector texCoords[4], int texCoordSize, DWORD color, DWORD specular)
{
   // Check texCoordSize
   BDEBUG_ASSERT((texCoordSize == 2) || (texCoordSize == 3));
   
   // Lock the dynamic transformed/lit vb.
   int sizeOfVert = 0;
   IDirect3DVertexDeclaration9 *pDecl = NULL;
   
   if (texCoordSize == 2)
   {
      BTLVertex *v=(BTLVertex*)gRenderDraw.lockDynamicVB(4, sizeof(v[0]));

      sizeOfVert = sizeof(v[0]);
      pDecl = v[0].msVertexDecl;

      // Poke in our data.
      v[0].x=x1-0.5f;
      v[0].y=y1-0.5f;
      v[0].z=0.0f;
      v[0].rhw=1.0f;
      v[0].diffuse=color;
      v[0].specular=specular;
      v[0].tu=texCoords[0].x;
      v[0].tv=texCoords[0].y;

      v[1].x=x0-0.5f;
      v[1].y=y1-0.5f;
      v[1].z=0.0f;
      v[1].rhw=1.0f;
      v[1].diffuse=color;
      v[1].specular=specular;
      v[1].tu=texCoords[1].x;
      v[1].tv=texCoords[1].y;

      v[2].x=x1-0.5f;
      v[2].y=y0-0.5f;
      v[2].z=0.0f;
      v[2].rhw=1.0f;
      v[2].diffuse=color;
      v[2].specular=specular;
      v[2].tu=texCoords[2].x;
      v[2].tv=texCoords[2].y;

      v[3].x=x0-0.5f;
      v[3].y=y0-0.5f;
      v[3].z=0.0f;
      v[3].rhw=1.0f;
      v[3].diffuse=color;
      v[3].specular=specular;
      v[3].tu=texCoords[3].x;
      v[3].tv=texCoords[3].y;
   }
   else
   {
      BTLT3DVertex *v=(BTLT3DVertex*)gRenderDraw.lockDynamicVB(4, sizeof(v[0]));

      sizeOfVert = sizeof(v[0]);
      pDecl = v[0].msVertexDecl;

      // Poke in our data.
      v[0].x=x1-0.5f;
      v[0].y=y1-0.5f;
      v[0].z=0.0f;
      v[0].rhw=1.0f;
      v[0].diffuse=color;
      v[0].tu=texCoords[0].x;
      v[0].tv=texCoords[0].y;
      v[0].ts=texCoords[0].z;

      v[1].x=x0-0.5f;
      v[1].y=y1-0.5f;
      v[1].z=0.0f;
      v[1].rhw=1.0f;
      v[1].diffuse=color;
      v[1].tu=texCoords[1].x;
      v[1].tv=texCoords[1].y;
      v[1].ts=texCoords[1].z;

      v[2].x=x1-0.5f;
      v[2].y=y0-0.5f;
      v[2].z=0.0f;
      v[2].rhw=1.0f;
      v[2].diffuse=color;
      v[2].tu=texCoords[2].x;
      v[2].tv=texCoords[2].y;
      v[2].ts=texCoords[2].z;

      v[3].x=x0-0.5f;
      v[3].y=y0-0.5f;
      v[3].z=0.0f;
      v[3].rhw=1.0f;
      v[3].diffuse=color;
      v[3].tu=texCoords[3].x;
      v[3].tv=texCoords[3].y;
      v[3].ts=texCoords[3].z;
   }

   // Set the stream source to the dynamic vb.
   gRenderDraw.setStreamSource(0, gRenderDraw.getDynamicVB(), 0, sizeOfVert);
   
   // We're done copying in data, so unlock.
   gRenderDraw.unlockDynamicVB();
   
   gFixedFuncShaders.set(cPosDiffuseTex1VS, cDiffuseTex1PS);
   
   setTransformScreenspace();

   gRenderDraw.setVertexDeclaration(pDecl);

   gRenderDraw.drawVertices(D3DPT_TRIANGLESTRIP, 0, 4);

   resetTransformScreenspace();
   
   gRenderDraw.clearStreamSource(0);
}

//============================================================================
// BPrimDraw2D::drawSolidRect2D2Tex
//============================================================================
void BPrimDraw2D::drawSolidRect2D2Tex(int x0, int y0, int x1, int y1, float u0, float v0, float u1, float v1,
                                          float u0_2, float v0_2, float u1_2, float v1_2, DWORD color, DWORD specular)
{
   BTLT2Vertex *v=(BTLT2Vertex*)gRenderDraw.lockDynamicVB(4, sizeof(v[0]));

   // Poke in our data.
   v[0].x=x1-0.5f;
   v[0].y=y1-0.5f;
   v[0].z=0.0f;
   v[0].rhw=1.0f;
   v[0].diffuse=color;
   v[0].specular=specular;
   v[0].tu=u1;
   v[0].tv=v1;
   v[0].tu2=u1_2;
   v[0].tv2=v1_2;

   v[1].x=x0-0.5f;
   v[1].y=y1-0.5f;
   v[1].z=0.0f;
   v[1].rhw=1.0f;
   v[1].diffuse=color;
   v[1].specular=specular;
   v[1].tu=u0;
   v[1].tv=v1;
   v[1].tu2=u0_2;
   v[1].tv2=v1_2;

   v[2].x=x1-0.5f;
   v[2].y=y0-0.5f;
   v[2].z=0.0f;
   v[2].rhw=1.0f;
   v[2].diffuse=color;
   v[2].specular=specular;
   v[2].tu=u1;
   v[2].tv=v0;
   v[2].tu2=u1_2;
   v[2].tv2=v0_2;

   v[3].x=x0-0.5f;
   v[3].y=y0-0.5f;
   v[3].z=0.0f;
   v[3].rhw=1.0f;
   v[3].diffuse=color;
   v[3].specular=specular;
   v[3].tu=u0;
   v[3].tv=v0;
   v[3].tu2=u0_2;
   v[3].tv2=v0_2;

   // Set the stream source to the dynamic vb.
   gRenderDraw.setStreamSource(0, gRenderDraw.getDynamicVB(), 0, sizeof(v[0]));
   
   // We're done copying in data, so unlock.
   gRenderDraw.unlockDynamicVB();
   
   gFixedFuncShaders.set(cPosDiffuseTex2VS, cDiffuseTex2PS);
   
   setTransformScreenspace();

   gRenderDraw.setVertexDeclaration(v[0].msVertexDecl);

   gRenderDraw.drawVertices(D3DPT_TRIANGLESTRIP, 0, 4);
   
   resetTransformScreenspace();

   gRenderDraw.setRenderState(D3DRS_VIEWPORTENABLE, TRUE);
   gRenderDraw.clearStreamSource(0);
}


//============================================================================
// BPrimDraw2D::drawEmptyRect2D
//============================================================================
void BPrimDraw2D::drawEmptyRect2D(int x0, int y0, int x1, int y1, DWORD color)
{
   // Lock the dynamic transformed/lit vb.
   BTLVertex *v=(BTLVertex*)gRenderDraw.lockDynamicVB(4, sizeof(v[0]));

   // Poke in our data.
   v[0].x=float(x1);
   v[0].y=float(y1);
   v[0].z=0.0f;
   v[0].rhw=1.0f;
   v[0].diffuse=color;
   v[0].specular=0;
   v[0].tu=0.0f;
   v[0].tv=0.0f;

   v[1].x=float(x0);
   v[1].y=float(y1);
   v[1].z=0.0f;
   v[1].rhw=1.0f;
   v[1].diffuse=color;
   v[1].specular=0;
   v[1].tu=0.0f;
   v[1].tv=0.0f;

   v[2].x=float(x1);
   v[2].y=float(y0);
   v[2].z=0.0f;
   v[2].rhw=1.0f;
   v[2].diffuse=color;
   v[2].specular=0;
   v[2].tu=0.0f;
   v[2].tv=0.0f;

   v[3].x=float(x0);
   v[3].y=float(y0);
   v[3].z=0.0f;
   v[3].rhw=1.0f;
   v[3].diffuse=color;
   v[3].specular=0;
   v[3].tu=0.0f;
   v[3].tv=0.0f;

   // Set the stream source to the dynamic vb.
   gRenderDraw.setStreamSource(0, gRenderDraw.getDynamicVB(), 0, sizeof(v[0]));
   
   // We're done copying in data, so unlock.
   gRenderDraw.unlockDynamicVB();

   // Now lock five indices.
   DWORD startIndex;
   WORD *indices=(WORD*)gRenderDraw.lockDynamicIB(5, startIndex);

   indices[0]=0;
   indices[1]=1;
   indices[2]=3;
   indices[3]=2;
   indices[4]=0;
   
   // Set indices.
   gRenderDraw.setIndices(gRenderDraw.getDynamicIB());
         
   gRenderDraw.unlockDynamicIB();
      
   gFixedFuncShaders.set(cPosDiffuseVS, cDiffusePS);
   
   setTransformScreenspace();
   
   gRenderDraw.setVertexDeclaration(v[0].msVertexDecl);

   // Draw the line.
   gRenderDraw.drawIndexedVertices(D3DPT_LINESTRIP, 0, 0, 4);
   
   resetTransformScreenspace();
   
   gRenderDraw.clearStreamSource(0);
   gRenderDraw.setIndices(NULL);
}

//============================================================================
// BPrimDraw2D::drawSolidFan
//============================================================================
void BPrimDraw2D::drawSolidFan(BTLVertex *vertices, DWORD vertexCount)
{
   // Lock the dynamic transformed/lit vb.
   BTLVertex *v=(BTLVertex*)gRenderDraw.lockDynamicVB(vertexCount, sizeof(v[0]));

   // Copy data.
   memcpy(v, vertices, vertexCount*sizeof(v[0]));

   // Set the stream source to the dynamic vb.
   gRenderDraw.setStreamSource(0, gRenderDraw.getDynamicVB(), 0, sizeof(v[0]));
   
   // We're done copying in data, so unlock.
   gRenderDraw.unlockDynamicVB();
      
   gFixedFuncShaders.set(cPosDiffuseVS, cDiffusePS);
      
   setTransformScreenspace();

   gRenderDraw.setVertexDeclaration(v[0].msVertexDecl);
      
   // Draw the fan.
   gRenderDraw.drawVertices(D3DPT_TRIANGLEFAN, 0, vertexCount);
   
   resetTransformScreenspace();
   
   gRenderDraw.setRenderState(D3DRS_VIEWPORTENABLE, TRUE);

   gRenderDraw.clearStreamSource(0);
}


//============================================================================
// BPrimDraw2D::drawLine2D
//============================================================================
void BPrimDraw2D::drawLine2D(int x0, int y0, int x1, int y1, DWORD color)
{
   drawLine2D(x0, y0, x1, y1, color, color);
}

//============================================================================
// BPrimDraw2D::drawLine
//============================================================================
void BPrimDraw2D::drawLine(const BVector &p0, const BVector &p1, DWORD color0, DWORD color1)
{
   // Lock the dynamic transformed/lit vb.
   BPDVertex *v=(BPDVertex*)gRenderDraw.lockDynamicVB(2, sizeof(v[0]));

   // Poke in our data.
   v[0].pos.x=p0.x;
   v[0].pos.y=p0.y;
   v[0].pos.z=p0.z;
   v[0].diffuse=color0;

   v[1].pos.x=p1.x;
   v[1].pos.y=p1.y;
   v[1].pos.z=p1.z;
   v[1].diffuse=color1;

   // Set the stream source to the dynamic vb.
   gRenderDraw.setStreamSource(0, gRenderDraw.getDynamicVB(), 0, sizeof(v[0]));
   
   // We're done copying in data, so unlock.
   gRenderDraw.unlockDynamicVB();
      
   gFixedFuncShaders.set(cPosDiffuseVS, cDiffusePS);
   
   setTransformWorldspace();

   gRenderDraw.setVertexDeclaration(v[0].msVertexDecl);

   // Draw the line.
   gRenderDraw.drawVertices(D3DPT_LINELIST, 0, 2);

   gRenderDraw.clearStreamSource(0);
}

//==============================================================================
// BPrimDraw2D::drawLines
//==============================================================================
void BPrimDraw2D::drawLines(const BVector *points, int lineCount, DWORD color)
{
   if (lineCount < 1)
      return;
      
   // Lock the dynamic transformed/lit vb.
   BPDVertex *v=(BPDVertex*)gRenderDraw.lockDynamicVB(2*lineCount, sizeof(v[0]));

   int pointCount = lineCount*2;
   for(int i=0; i<pointCount; i++)
   {
      // Poke in our data.
      v->pos.x=points->x;
      v->pos.y=points->y;
      v->pos.z=points->z;
      v->diffuse=color;
      v++;
      points++;
   }

   // Set the stream source to the dynamic vb.
   gRenderDraw.setStreamSource(0, gRenderDraw.getDynamicVB(), 0, sizeof(v[0]));
      
   // We're done copying in data, so unlock.
   gRenderDraw.unlockDynamicVB();
   
   gFixedFuncShaders.set(cPosDiffuseVS, cDiffusePS);
   
   setTransformWorldspace();
      
   gRenderDraw.setVertexDeclaration(v[0].msVertexDecl);

   // Draw the line.
   gRenderDraw.drawVertices(D3DPT_LINELIST, 0, pointCount);

   gRenderDraw.clearStreamSource(0);
}

//==============================================================================
// BPrimDraw2D::drawPolygon2D
//==============================================================================
void BPrimDraw2D::drawPolygon2D(const BVec2* pPoints, uint numPoints, DWORD color)
{
   if (numPoints < 2)
      return;
      
   // Lock the dynamic transformed/lit vb.
   BPosDiffuseTLVertex *v=(BPosDiffuseTLVertex*)gRenderDraw.lockDynamicVB(numPoints, sizeof(v[0]));

   for(uint i = 0; i < numPoints; i++)
   {
      v[i].x=pPoints[i][0];
      v[i].y=pPoints[i][1];
      v[i].z=0.0f;
      v[i].rhw=1.0f;
      v[i].diffuse=color;
   }

   // Set the stream source to the dynamic vb.
   gRenderDraw.setStreamSource(0, gRenderDraw.getDynamicVB(), 0, sizeof(v[0]));

   // We're done copying in data, so unlock.
   gRenderDraw.unlockDynamicVB();

   gFixedFuncShaders.set(cPosDiffuseVS, cDiffusePS);

   setTransformScreenspace();

   gRenderDraw.setVertexDeclaration(v[0].msVertexDecl);

   // Draw the line.
   gRenderDraw.drawVertices(D3DPT_LINESTRIP, 0, numPoints);

   resetTransformScreenspace();

   gRenderDraw.clearStreamSource(0);
}

//==============================================================================
// BPrimDraw2D::drawCircle2D
//==============================================================================
void BPrimDraw2D::drawCircle2D(float x, float y, float radius, uint numSegments, DWORD color)
{
   const uint cMaxSegments = 64;
   BVec2 segments[cMaxSegments];
   BDEBUG_ASSERT(numSegments >= 3 && numSegments < cMaxSegments);

   const float radInc = Math::fTwoPi / numSegments;
   float radCur = -XM_PI;
   for (uint i = 0; i < numSegments; i++)
   {
      float sin, cos;
      XMScalarSinCosEst(&sin, &cos, radCur);
      
      segments[i] = BVec2(x + sin * radius, y + cos * radius);
            
      radCur += radInc;
   }
   
   segments[numSegments] = segments[0];
   
   drawPolygon2D(segments, numSegments + 1, color);
}

//==============================================================================
// BPrimDraw2D::drawIndexedLines
//==============================================================================
void BPrimDraw2D::drawIndexedLines(const BVector *points, int pointCount, const int *indices, int indexCount, DWORD color)
{
   // Lock the dynamic transformed/lit vb.
   BPDVertex *v=(BPDVertex*)gRenderDraw.lockDynamicVB(pointCount, sizeof(v[0]));
   
   for(int i=0; i<pointCount; i++)
   {
      // Poke in our data.
      v->pos.x=points->x;
      v->pos.y=points->y;
      v->pos.z=points->z;
      v->diffuse=color;
      v++;
      points++;
   }
   
   gRenderDraw.setStreamSource(0, gRenderDraw.getDynamicVB(), 0, sizeof(v[0]));

   // We're done copying in data, so unlock.
   gRenderDraw.unlockDynamicVB();

   // Index buffer.
   WORD *ib = (WORD*)gRenderDraw.lockDynamicIB(indexCount);
   
   // Copy indices.      
   for(int i=0; i<indexCount; i++)
   {
      *ib = (WORD)*indices;
      ib++;
      indices++;
   }
   
   gRenderDraw.setIndices(gRenderDraw.getDynamicIB());

   // Unlock.
   gRenderDraw.unlockDynamicIB();

   gFixedFuncShaders.set(cPosDiffuseVS, cDiffusePS);
   
   setTransformWorldspace();
   
   gRenderDraw.setVertexDeclaration(v[0].msVertexDecl);
         
   gRenderDraw.drawIndexedVertices(D3DPT_LINELIST, 0, 0, indexCount);   

   gRenderDraw.clearStreamSource(0);
   gRenderDraw.setIndices(NULL);
}

//==============================================================================
// BPrimDraw2D::drawDebugQuad
//==============================================================================
void BPrimDraw2D::drawDebugQuad(const BVector clockWiseQuad[4], DWORD color)
{
   gRenderDraw.setRenderState(D3DRS_CULLMODE, D3DCULL_NONE);

   // Lock the dynamic transformed/lit vb.
   BPDVertex *v=(BPDVertex*)gRenderDraw.lockDynamicVB(4, sizeof(v[0]));

   // Poke in our data.
   v[0].pos.x=clockWiseQuad[0].x;
   v[0].pos.y=clockWiseQuad[0].y;
   v[0].pos.z=clockWiseQuad[0].z;
   v[0].diffuse=color;
   v[1].pos.x=clockWiseQuad[1].x;
   v[1].pos.y=clockWiseQuad[1].y;
   v[1].pos.z=clockWiseQuad[1].z;
   v[1].diffuse=color;
   v[2].pos.x=clockWiseQuad[3].x;
   v[2].pos.y=clockWiseQuad[3].y;
   v[2].pos.z=clockWiseQuad[3].z;
   v[2].diffuse=color;
   v[3].pos.x=clockWiseQuad[2].x;
   v[3].pos.y=clockWiseQuad[2].y;
   v[3].pos.z=clockWiseQuad[2].z;
   v[3].diffuse=color;

   // Set the stream source to the dynamic vb.
   gRenderDraw.setStreamSource(0, gRenderDraw.getDynamicVB(), 0, sizeof(v[0]));

   // We're done copying in data, so unlock.
   gRenderDraw.unlockDynamicVB();

   gFixedFuncShaders.set(cPosDiffuseVS, cDiffusePS);

   setTransformWorldspace();

   gRenderDraw.setVertexDeclaration(v[0].msVertexDecl);

   // Draw the line.
   gRenderDraw.drawVertices(D3DPT_TRIANGLESTRIP, 0, 4);

   gRenderDraw.setRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
}

//==============================================================================
// BPrimDraw2D::renderQuad
// x,y,width,height     - screenspace coordinates (int)
// ofsX, ofsY           - screenspace offset (float)
// grid                 - true to use a grid of rects instead of a single rect
// uLo, uHi, vLo, vHi   - uv0 range
//==============================================================================
void BPrimDraw2D::renderQuad(int x, int y, int width, int height, float ofsX, float ofsY, bool grid, float uLo, float uHi, float vLo, float vHi)
{
#ifdef BUILD_DEBUG   
   if (gEventDispatcher.getThreadIndex() == cThreadIndexRender)
   {
      DWORD val;
      BD3D::mpDev->GetRenderState(D3DRS_HALFPIXELOFFSET, &val);
      BDEBUG_ASSERT(val == FALSE);
   }
#endif   

   BCOMPILETIMEASSERT(sizeof(BPU2Vertex) == sizeof(XMFLOAT2) * 3);
   
   const DWORD g_dwQuadGridSizeX  = 8; // 1280 / 8 = 160
   const DWORD g_dwQuadGridSizeY  = 1; //  720 / 1 = 720
            
   const uint numVerts = grid ? 3 * (g_dwQuadGridSizeX * g_dwQuadGridSizeY) : 3;
   BPU2Vertex* pVB = static_cast<BPU2Vertex*>(gRenderDraw.lockDynamicVB(numVerts, sizeof(BPU2Vertex)));
         
   if (grid)
   {
      XMFLOAT2* v = reinterpret_cast<XMFLOAT2*>(pVB);

      float fGridDimX = 1.0f / (float)g_dwQuadGridSizeX;
      float fGridDimY = 1.0f / (float)g_dwQuadGridSizeY;
      float fGridDimU = 1.0f / (float)g_dwQuadGridSizeX;
      float fGridDimV = 1.0f / (float)g_dwQuadGridSizeY;
      float T  = 0.0f;
      float V0 = 0.0f;

      for( DWORD iy=0; iy<g_dwQuadGridSizeY; iy++ )
      {
         float L  = 0.0f;
         float U0 = 0.0f;
         for( DWORD ix=0; ix<g_dwQuadGridSizeX; ix++ )
         {
            float R = L + fGridDimX;
            float B = T + fGridDimY;
            float U1 = U0 + fGridDimU;
            float V1 = V0 + fGridDimV;

            *v++ = XMFLOAT2(x + ofsX + L * width, y + ofsY + T * height); // x, y
            *v++ = XMFLOAT2(Math::Lerp(uLo, uHi, U0), Math::Lerp(vLo, vHi, V0) ); // tu, tv
            *v++ = XMFLOAT2(U0, V0);
            
            *v++ = XMFLOAT2(x + ofsX + R * width, y + ofsY + T * height); // x, y
            *v++ = XMFLOAT2(Math::Lerp(uLo, uHi, U1), Math::Lerp(vLo, vHi, V0) ); // tu, tv
            *v++ = XMFLOAT2(U1, V0); 
            
            *v++ = XMFLOAT2(x + ofsX + L * width, y + ofsY + B * height);
            *v++ = XMFLOAT2(Math::Lerp(uLo, uHi, U0), Math::Lerp(vLo, vHi, V1) ); // x, y, tu, tv
            *v++ = XMFLOAT2(U0, V1);

            L  += fGridDimX;
            U0 += fGridDimU;
         }

         T  += fGridDimY;
         V0 += fGridDimV;
      }
   }
   else
   {
      pVB->pos = XMFLOAT2(x + ofsX, y + ofsY);
      pVB->uv0 = XMFLOAT2(uLo, vLo);
      pVB->uv1 = XMFLOAT2(0.0f, 0.0f);
      pVB++;

      pVB->pos = XMFLOAT2(x + width + ofsX, y + ofsY);
      pVB->uv0 = XMFLOAT2(uHi, vLo);
      pVB->uv1 = XMFLOAT2(1.0f, 0.0f);
      pVB++;

      pVB->pos = XMFLOAT2(x + ofsX, y + height + ofsY);
      pVB->uv0 = XMFLOAT2(uLo, vHi);
      pVB->uv1 = XMFLOAT2(0.0f, 1.0f);
   }      
   
   gRenderDraw.setStreamSource(0, gRenderDraw.getDynamicVB(), 0, sizeof(BPU2Vertex));

   gRenderDraw.unlockDynamicVB();

   gRenderDraw.setVertexDeclaration(BPU2Vertex::msVertexDecl);
   
   gRenderDraw.drawVertices(D3DPT_RECTLIST, 0, numVerts);
}