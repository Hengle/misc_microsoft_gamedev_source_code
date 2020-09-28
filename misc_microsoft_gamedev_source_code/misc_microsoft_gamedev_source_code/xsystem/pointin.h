//==============================================================================
// Copyright (c) 1997 Ensemble Studios
//
// Point in poly functions
//==============================================================================

#ifndef _POINTIN_H_
#define _POINTIN_H_

//==============================================================================
bool     pointInTriangleXY(const BVector *p, const BVector *vert0, const BVector *vert1, 
            const BVector *vert2);
bool     pointInTriangleYZ(const BVector *p, const BVector *vert0, const BVector *vert1, 
            const BVector *vert2);
bool     pointInTriangleXZ(const BVector *p, const BVector *vert0, const BVector *vert1, 
            const BVector *vert2);
bool     pointInTriangleXZ(const BVector& p, const BVector& vert0, const BVector& vert1, 
            const BVector& vert2);

bool     pointInXZProjection(const BVector *vertices, const long numVertices, const BVector &v);
bool     pointInXZProjection(const BVector *vertices, const long *indices, const long numVertices, const BVector &v);

bool     pointInXYProjection(const BVector *vertices, const long numVertices, const BVector &v);
bool     pointInYZProjection(const BVector *vertices, const long numVertices, const BVector &v);


#endif


