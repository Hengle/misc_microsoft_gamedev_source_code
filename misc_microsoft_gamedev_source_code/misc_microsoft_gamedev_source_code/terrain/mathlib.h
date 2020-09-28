//============================================================================
//
//  mathlib.h
//  
//  Copyright (c) 2006, Ensemble Studios
//
//============================================================================
#pragma once

//-----------------------------------------------------

bool raySegmentIntersectionTriangle(const D3DXVECTOR3 vertex[3], const D3DXVECTOR3 &origin, const D3DXVECTOR3 &direction, 
                                    const bool segment, D3DXVECTOR3 &iPoint);

bool ray3AABB(D3DXVECTOR3& coord,
              float& t,
              const D3DXVECTOR3& rayOrig,
              const D3DXVECTOR3& rayDir,
              const D3DXVECTOR3& boxmin,
              const D3DXVECTOR3& boxmax);

bool spheresIntersect(const D3DXVECTOR3 &center1,const float radius1, const D3DXVECTOR3 &center2,const float radius2);
bool aabbsIntersect(const D3DXVECTOR3 &aMin,const D3DXVECTOR3 &aMax,const D3DXVECTOR3 &bMin,const D3DXVECTOR3 &bMax);

bool pointBoxIntersect(const D3DXVECTOR3 &tA,const D3DXVECTOR3 &tB,const D3DXVECTOR3 &tC,const D3DXVECTOR3 &tD,
                       const D3DXVECTOR3 &bA,const D3DXVECTOR3 &bB,const D3DXVECTOR3 &bC,const D3DXVECTOR3 &bD,
                       const D3DXVECTOR3 &pt);