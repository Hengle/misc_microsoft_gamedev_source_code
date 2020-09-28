//============================================================================
//
//  physicsrenderadapter.cpp
//  
//  Copyright (c) 2003, Ensemble Studios
//
//============================================================================

#include "common.h"



#include "debugprimitives.h"
#include "boundingbox.h"
#include "physicsrenderadapter.h"
#include "physicsmatrix.h"
#include "render.h"



//============================================================================
// BPhysicsRenderAdapater::BPhysicsRenderAdapter
//============================================================================
BPhysicsRenderAdapter::BPhysicsRenderAdapter()
{
}


//============================================================================
// BPhysicsRenderAdapter::~BPhysicsRenderAdapter
//============================================================================
BPhysicsRenderAdapter::~BPhysicsRenderAdapter()
{
}

//============================================================================
// BPhysicsRenderAdapter::addDebugLine
//============================================================================
void BPhysicsRenderAdapter::addDebugLine(const char *szName, long group, const BVector &start, const BVector &end, DWORD color1, DWORD color2)
{
   //gpDebugPrimitives->addDebugLine()
}

//============================================================================
// BPhysicsRenderAdapter::drawDebugLine
//============================================================================
void BPhysicsRenderAdapter::drawDebugLine(const BVector &start, const BVector &end, DWORD color1, DWORD color2)
{
   gpDebugPrimitives->addDebugLine(start, end, color1, color2);
}

//============================================================================
// BPhysicsRenderAdapter::drawDebugSphere
//============================================================================
void BPhysicsRenderAdapter::drawDebugSphere(float radius, DWORD color)
{
   BVector pos;
   gRender.getWorldBMatrix().getTranslation(pos);

   gpDebugPrimitives->addDebugSphere(pos, radius, color);
}

//============================================================================
// BPhysicsRenderAdapter::drawDebugBox
//============================================================================
void BPhysicsRenderAdapter::drawDebugBox(const BVector& halfExtents, DWORD color)
{
   BBoundingBox box;
   box.initialize(-halfExtents,halfExtents);
   
   box.draw(color, true);
 
}

//============================================================================
// BPhysicsRenderAdapter::removeDebugLines
//============================================================================
void BPhysicsRenderAdapter::removeDebugLines(const char* szName)
{
   //gpDebugPrimitives->clear()
}

//============================================================================
// BPhysicsRenderAdapter::setWorldMatrix
//============================================================================
void BPhysicsRenderAdapter::setWorldMatrix(const BPhysicsMatrix &matrix)
{
   BMatrix renderMatrix;
   renderMatrix.makeOrient(matrix.getForward(), matrix.getUp(), matrix.getRight());
   renderMatrix.setTranslation(matrix.getTranslation());
   gRender.setWorldMatrix(renderMatrix);
}

//============================================================================
// BPhysicsRenderAdapter::getWorldMatrix
//============================================================================
void BPhysicsRenderAdapter::getWorldMatrix(BPhysicsMatrix &matrix)
{
   //memcpy(&matrix, &BRenderDevice::getWorldD3DMatrix(), sizeof(D3DMATRIX));
}




//============================================================================
// eof: physicsrenderadapter.cpp
//============================================================================
