//==============================================================================
// triangle.cpp
//
// Copyright (c) 2003, Ensemble Studios
//==============================================================================

// Includes
#include "common.h"
#include "triangle.h"

//==============================================================================
// BTriangle::getCenter
//==============================================================================
BVector BTriangle::getCenter() const
{
   BVector minPoint(min(mPoints[0].x, min(mPoints[1].x, mPoints[2].x)), min(mPoints[0].y, min(mPoints[1].y, mPoints[2].y)), min(mPoints[0].z, min(mPoints[1].z, mPoints[2].z)));
   BVector maxPoint(max(mPoints[0].x, max(mPoints[1].x, mPoints[2].x)), max(mPoints[0].y, max(mPoints[1].y, mPoints[2].y)), max(mPoints[0].z, max(mPoints[1].z, mPoints[2].z)));

   BVector halfExtents=(maxPoint-minPoint)*0.5f;

   BVector center(minPoint.x+halfExtents.x, minPoint.y+halfExtents.y, minPoint.z+halfExtents.z);
   return center;
}

//==============================================================================
// BTriangle::getPlane
//==============================================================================
BPlane BTriangle::getPlane() const
{
   BPlane plane(mPoints[0], mPoints[1], mPoints[2]);
   return plane;
}

//==============================================================================
// BTriangle::getNormal
//==============================================================================
BVector BTriangle::getNormal() const
{
   BVector normal;
   normal.assignCrossProduct(mPoints[1]-mPoints[0], mPoints[2]-mPoints[0]);
   normal.normalize();
   return normal;
}

//==============================================================================
// BTriangle::getRadius
//==============================================================================
float BTriangle::getRadius() const
{
   BVector center;

   BVector minPoint(min(mPoints[0].x, min(mPoints[1].x, mPoints[2].x)), min(mPoints[0].y, min(mPoints[1].y, mPoints[2].y)), min(mPoints[0].z, min(mPoints[1].z, mPoints[2].z)));
   BVector maxPoint(max(mPoints[0].x, max(mPoints[1].x, mPoints[2].x)), max(mPoints[0].y, max(mPoints[1].y, mPoints[2].y)), max(mPoints[0].z, max(mPoints[1].z, mPoints[2].z)));

   BVector halfExtents=(maxPoint-minPoint)*0.5f;

   center.set(minPoint.x+halfExtents.x, minPoint.y+halfExtents.y, minPoint.z+halfExtents.z);

   float distFromMin = center.distance(minPoint);
   float distFromMax = center.distance(maxPoint); 
   float radius=max(distFromMin, distFromMax);

   return radius;
}

//==============================================================================
// BTriangle::getCenter
//==============================================================================
void BTriangle::getCenter(BVector& center) const
{
   BVector minPoint(min(mPoints[0].x, min(mPoints[1].x, mPoints[2].x)), min(mPoints[0].y, min(mPoints[1].y, mPoints[2].y)), min(mPoints[0].z, min(mPoints[1].z, mPoints[2].z)));
   BVector maxPoint(max(mPoints[0].x, max(mPoints[1].x, mPoints[2].x)), max(mPoints[0].y, max(mPoints[1].y, mPoints[2].y)), max(mPoints[0].z, max(mPoints[1].z, mPoints[2].z)));

   BVector halfExtents=(maxPoint-minPoint)*0.5f;

   center.set(minPoint.x+halfExtents.x, minPoint.y+halfExtents.y, minPoint.z+halfExtents.z);
}

//==============================================================================
// BTriangle::getPlane
//==============================================================================
void BTriangle::getPlane(BPlane& plane) const
{
   plane.assign(mPoints[0], mPoints[1], mPoints[2]);
}

//==============================================================================
// BTriangle::getNormal
//==============================================================================
void BTriangle::getNormal(BVector& normal) const
{
   normal.assignCrossProduct(mPoints[1]-mPoints[0], mPoints[2]-mPoints[0]);
   normal.normalize();
}
