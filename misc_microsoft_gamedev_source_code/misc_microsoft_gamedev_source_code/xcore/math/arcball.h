//-----------------------------------------------------------------------------
// File: arcball.h
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#pragma once

#include "quat.h"
#include "plane.h"
#include "intersection.h"

BQuat ArcBall(
   const BVec3& projPos,
   const BVec3& objPos, 
   const BVec3& prevDir,
   const BVec3& newDir,
   const float fudgeFactor = 4.0f)
{
   BDEBUG_ASSERT(prevDir.isUnit());
   BDEBUG_ASSERT(newDir.isUnit());

   BVec3 normal(objPos - projPos);
   
   const float ooDist = normal.oneOverLen();
   BDEBUG_ASSERT(ooDist > 0.0f);
   
   normal *= ooDist;

   const Plane plane(normal, normal * objPos);
   
   float ut, vt;
   eResult ures = Intersection::ray3Plane(ut, plane, Ray3(projPos, prevDir));
   eResult vres = Intersection::ray3Plane(vt, plane, Ray3(projPos, newDir));

   if ((ures != cSUCCESS) || (vres != cSUCCESS))
      return BQuat::I;
      
   BVec3 u(projPos + prevDir * ut);
   BVec3 v(projPos + newDir * vt);        

   u = (u - objPos) * ooDist * fudgeFactor;
   v = (v - objPos) * ooDist * fudgeFactor;
   
   const float uLen = u.len();
   if (uLen > 1.0f)
      u /= uLen;
   else
      u -= plane.n * sqrt(1.0f - Math::Sqr(uLen));

   const float vLen = v.len();
   if (vLen > 1.0f)
      v /= vLen;
   else
      v -= plane.n * sqrt(1.0f - Math::Sqr(vLen));

   return BQuat::makeRotationArc(BVec4(u), BVec4(v));
}
