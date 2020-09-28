//==============================================================================
// physicsrenderadapter.h
//
// Copyright (c) 2004-2007 Ensemble Studios
//==============================================================================
#pragma once 

//==============================================================================
// Includes
#include "physics.h"

//==============================================================================
// Forward declarations


//==============================================================================
// Const declarations


//==============================================================================
// BPhysicsRenderAdapter
//==============================================================================
class BPhysicsRenderAdapter : public BPhysicsRenderInterface
{
public:
   BPhysicsRenderAdapter();
   ~BPhysicsRenderAdapter();

   //-- implementation of BPhysicsRenderInterface 
   void                                      addDebugLine(const char *szName, long group, const BVector &start, const BVector &end, DWORD color1, DWORD color2);
   void                                      drawDebugLine(const BVector &start, const BVector &end, DWORD color1, DWORD color2);
   void                                      drawDebugSphere(float radius, DWORD color);
   void                                      drawDebugBox(const BVector& halfExtents, DWORD color);
   void                                      removeDebugLines(const char* szName);
   void                                      setWorldMatrix(const BPhysicsMatrix &matrix);
   void                                      getWorldMatrix(BPhysicsMatrix &matrix );

  
};