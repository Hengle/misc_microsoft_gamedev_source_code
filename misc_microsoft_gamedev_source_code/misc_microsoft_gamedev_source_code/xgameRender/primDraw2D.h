//============================================================================
//
//  primDraw2D.h
//
//  Copyright (c) 1997-2001, Ensemble Studios
//
//  rg [2/17/06] - Ported from Phoenix codebase
//
//============================================================================
#pragma once

#include "vertexTypes.h"
#include "fixedFuncShaders.h"

//============================================================================
// class BPrimDraw2D
// This class is usable from the sim or render threads.
// It's pretty slow - primarily intended for development.
// Note: These methods assume the HALFPIXELOFFSET render state is set to FALSE!
//============================================================================
class BPrimDraw2D
{
public:
   static void drawLine2D(int x0, int y0, int x1, int y1, DWORD color1, DWORD color2);
   
   // (x0,y0-x1,y1) must be a D3DRECT style rectangle, i.e. (0,0)-(640,480) to fill an entire 640x480 render target.
   static void drawSolidRect2D(int x0, int y0, int x1, int y1, DWORD color, DWORD specular);

   static void drawSolidRect2D(int x0, int y0, int x1, int y1, float u0, float v0, float u1, float v1, DWORD color, DWORD specular, eFixedFuncShaderIndex vs = cPosDiffuseTex1VS, eFixedFuncShaderIndex ps = cDiffuseTex1PS);

   static void drawSolidRect2DTexCoord(int x0, int y0, int x1, int y1, BVector texCoords[4], int texCoordSize, DWORD color, DWORD specular);

   static void drawSolidRect2D2Tex(int x0, int y0, int x1, int y1, float u0, float v0, float u1, float v1,
                                             float u0_2, float v0_2, float u1_2, float v1_2, DWORD color, DWORD specular);

   static void drawEmptyRect2D(int x0, int y0, int x1, int y1, DWORD color);
   
   static void drawLine2D(int x0, int y0, int x1, int y1, DWORD color);

   static void drawPolygon2D(const BVec2* pPoints, uint numPoints, DWORD color);
   
   static void drawCircle2D(float x, float y, float radius, uint numSegments, DWORD color);

   // rg [3/18/06] - FIXME This class is for 2D drawing, but the following methods are 3D.

   static void drawSolidFan(BTLVertex *vertices, DWORD vertexCount);   
   
   static void drawLine(const BVector &p0, const BVector &p1, DWORD color0, DWORD color1);

   static void drawLines(const BVector *points, int lineCount, DWORD color);
      
   static void drawIndexedLines(const BVector *points, int pointCount, const int *indices, int indexCount, DWORD color);
   
   static void drawDebugQuad(const BVector clockWiseQuad[4], DWORD color);
      
   static void renderQuad(int x, int y, int width, int height, float ofsX = -.5f, float ofsY = -.5f, bool grid = false, float uLo = 0.0f, float uHi = 1.0f, float vLo = 0.0f, float vHi = 1.0f);
      
private:
   // Helpers, for use with "fixed function" shaders.
   static void setTransformScreenspace(void);
   static void resetTransformScreenspace(void);
   static void setTransformWorldspace(void);   
};   
