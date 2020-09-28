//=============================================================================
// Copyright (c) 1997 - 2000 Ensemble Studios
//
// BANG Math Stuff
//=============================================================================
 
#include "xsystem.h"
#include "mathutil.h"
#include "segintersect.h"
#include "plane.h"


//=============================================================================
static float fsintable[65];
static float fcostable[65];

static float fconst = 10.0f;

static DWORD gseed;

static float frandmultiplier = 4.656612E-10f;

static const float INV_1000 = (1.0f/1000.0f);

//==============================================================================
// logBase2(long num)
//
// Returns the (rounded) base 2 log of a given number or -1 on error
//==============================================================================
long logBase2(long num)
{
   if(num < 1)
      return -1;

   long result = 0;
   while(num > 1)
   {
      result++;
      num /= 2;
   }

   return result;
}

//==============================================================================
// twoTo(long num)
//
// Returns 2 raised to the given power or -1 on error
//==============================================================================
long twoTo(long num)
{
   if(num < 1)
      return -1;

   long result = 1;
   for(long i=0; i<num; i++)
      result *= 2;

   return result;
}
//=============================================================================
// createSinTable
//=============================================================================
void createSinTable(void)
{
   DWORD i = 0;

   memset(&fsintable[0], 0, sizeof(fsintable));
   
   for (float f = 0.0f; f < cTwoPi; f += 0.10f, i++)
      fsintable[i] = (float) sin(f);


} // createSinTable

//=============================================================================
// createCosTable
//=============================================================================
void createCosTable(void)
{
   DWORD i = 0;

   memset(&fcostable[0], 0, sizeof(fcostable));
   
   for (float f = 0.0f; f < cTwoPi; f += 0.10f, i++)
      fcostable[i] = (float) cos(f);

} // createCosTable

//=============================================================================
// lookupSin
//=============================================================================
float lookupSin(float fangle)
{
   BASSERT(fangle>=0.0f-cFloatCompareEpsilon && fangle<=cTwoPi+cFloatCompareEpsilon);
   
#ifdef XBOX
   // rg [6/10/05] - FIXME
   return sin(fangle);
#else
   DWORD dwtemp;
   
   __asm
   {
      fld dword ptr [fangle]
      fmul dword ptr [fconst]
      fistp dword ptr [dwtemp]
   }
   
   if (dwtemp > 0)
      dwtemp--;

   return fsintable[dwtemp];
#endif      

} // lookupSin

//=============================================================================
// lookupCos
//=============================================================================
float lookupCos(float fangle)
{
#ifdef XBOX
   // rg [6/10/05] - Fixme
   return cos(fangle);
#else
   DWORD dwtemp;

   __asm
   {
      fld dword ptr [fangle]
      fmul dword ptr [fconst]
      fistp dword ptr [dwtemp]
   }
   
   if (dwtemp > 0)
      dwtemp--;

   return fcostable[dwtemp];
#endif
} // lookupCos

//=============================================================================
// circleTan(float x, float z)
//
// Returns a value >= 0 and < 2PI that represents the angle measurement of
// this 'X,Y' pair.
//=============================================================================
float circleTan(float x, float z)
{
   //If z is 0, return 0 if x > 0, else return PI.
   if (_fabs(z) < cFloatCompareEpsilon)
   {
      if (x > 0)
         return(0.0f);
      return(cPi);
   }
   //If x is 0, return PI/2 if z > 0, else return 3PI/2.
   if (_fabs(x) < cFloatCompareEpsilon)
   {
      if (z > 0)
         return(cPiOver2);
      return(cThreePiOver2);
   }

   float rVal=(float)atan(z/x);
   if (rVal < 0.0f)
      rVal=-rVal;
   if (x > 0.0f)
   {
      if (z > 0.0f)
         return(rVal);
      return(cTwoPi-rVal);
   }
   if (z > 0.0f)
      return(cPi-rVal);
   return(cPi+rVal);

} // circleTan

//=============================================================================
// getrandfraction
//=============================================================================
float getrandfraction(void) 
{  
  float uniform;
  long k = gseed / 54532;
  gseed = 39373 * (gseed - k * 54532) - k * 1481;

#ifdef XBOX
   // rg [6/10/05] - Fixme
   uniform = frandmultiplier * gseed;
#else
   __asm
   {
      fild dword ptr [gseed]
      fmul dword ptr [frandmultiplier]
      fstp dword ptr [uniform]
   }
#endif   

   if (uniform < 0.0f)
      uniform = - uniform;

  return uniform;

} // getrandfraction

//=============================================================================
// getirand
//=============================================================================
long getirand(void) 
{  
  long k;
  k = gseed / 54532;
  gseed = 39373 * (gseed - k * 54532) - k * 1481;

#ifdef XBOX
   // rg [6/10/05] - FIXME
   float temp = frandmultiplier * gseed;
   // Yes this is evil and dumb for xenon. Does the return value have the right distribution anyway?
   k = *reinterpret_cast<const long*>(&temp);
#else
   __asm
   {
      fild dword ptr [gseed]
      fmul dword ptr [frandmultiplier]
      fstp dword ptr [k]      ; // purposely mis-store this as a float into a DWORD
   }
#endif   

  return k;

} // getirand


//=============================================================================
// VectorFromPoints
//=============================================================================
void VectorFromPoints(const float *px1, const float *py1, const float *pz1, 
                   const float *px2, const float *py2, const float *pz2,
                   float *px, float *py, float *pz)
{
   *px = *px1 - *px2;
   *py = *py1 - *py2;
   *pz = *pz1 - *pz2;

} // Vector

//=============================================================================
// CrossProduct
//=============================================================================
void CrossProduct(const float *px1, const float *py1, const float *pz1, 
                         const float *px2, const float *py2, const float *pz2,
                         float *px, float *py, float *pz)
{
   *px = *py1 * *pz2 - *pz1 * *py2;
   *py = *pz1 * *px2 - *px1 * *pz2;
   *pz = *px1 * *py2 - *py1 * *px2;

} // CrossProduct

//=============================================================================
// VectorLength
//=============================================================================
inline float VectorLength(const float *px, const float *py, const float *pz)
{
   return (float) sqrt(*px * *px + *py * *py + *pz * *pz);
}

//=============================================================================
// normalize
//=============================================================================
void normalize(float *px, float *py, float *pz) 
{
   float len=VectorLength(px, py, pz); 
   
   *px /= len; 
   *py /= len; 
   *pz /= len;

} // normalize

//=============================================================================
// raySegmentIntersectionPlane
//
//=============================================================================
bool raySegmentIntersectionPlane(const BVector &pointOnPlane, const BVector &normal, const BVector &origin, 
   const BVector &direction, bool segment, BVector &iPoint, const float errorEpsilon)
{
   // Calculate d parameter of polygon's plane equation.
   float d = -(pointOnPlane.dot(normal));

   // ray(t) = origin + direction*t and the equation of the triangles plane is
   // normal|p + d = 0  (where p is any point on the plane).
   // Thus at the intersection of the plane and the ray, t = -(d+N|origin)/(N|direction).

   // Get the denominator.  
   float den = normal.dot(direction);
   // If the denominator is 0, then the ray is parallel to the plane so
   // there is no intersection.
   if(_fabs(den) < cFloatCompareEpsilon)
      return(false);

   // Get the numerator.
   float num = d + (normal.dot(origin));

   // Calculate t.
   float t = -num/den;

   // If t is less than 0, than the intersection is behind the origin of the ray, so 
   // there is no intersection.
   if(t < -errorEpsilon)
      return(false);

   // If this is a segment and the intersection is past the end of the segment, there
   // is no intersection.
   if(segment && (t > 1.0f+errorEpsilon))
      return(false);

   // Now we wish to see if the point of intersection is within the triangle.

   // First, get the actual point of intersection of the ray and the plane.
   //iPoint = origin + direction*t;
   iPoint.x = origin.x + direction.x*t;
   iPoint.y = origin.y + direction.y*t;
   iPoint.z = origin.z + direction.z*t;

   return(true);
}

//=============================================================================
// lineIntersectsPlane
//
//=============================================================================
bool lineIntersectsPlane(const BPlane &plane, const BVector &origin, const BVector &direction, BVector &iPoint)
{
   // line(t) = origin + direction*t and the equation of the plane is
   // normal|p + d = 0  (where p is any point on the plane).
   // Thus at the intersection of the plane and the line, t = -(d+N|origin)/(N|direction).

   // Get the denominator.  
   float den = plane.mNormal.dot(direction);
   // If the denominator is 0, then the line is parallel to the plane so
   // there is no intersection.
   if(_fabs(den) < cFloatCompareEpsilon)
      return(false);

   // Get the numerator.
   float num = plane.mDistance + (plane.mNormal.dot(origin));

   // Calculate t.
   float t = -num/den;

   // Get the actual point of intersection of the line and the plane.
   //iPoint = origin + direction*t;
   iPoint.x = origin.x + direction.x*t;
   iPoint.y = origin.y + direction.y*t;
   iPoint.z = origin.z + direction.z*t;

   return(true);
}

//=============================================================================
// lineIntersectsPlane
//
//=============================================================================
bool lineIntersectsPlane(const BVector &pointOnPlane, const BVector &normal, const BVector &origin, 
   const BVector &direction, BVector &iPoint)
{
   // Calculate d parameter of polygon's plane equation.
   float d = -(pointOnPlane.dot(normal));

   // line(t) = origin + direction*t and the equation of the plane is
   // normal|p + d = 0  (where p is any point on the plane).
   // Thus at the intersection of the plane and the line, t = -(d+N|origin)/(N|direction).

   // Get the denominator.  
   float den = normal.dot(direction);
   // If the denominator is 0, then the line is parallel to the plane so
   // there is no intersection.
   if(_fabs(den) < cFloatCompareEpsilon)
      return(false);

   // Get the numerator.
   float num = d + (normal.dot(origin));

   // Calculate t.
   float t = -num/den;

   // Get the actual point of intersection of the line and the plane.
   //iPoint = origin + direction*t;
   iPoint.x = origin.x + direction.x*t;
   iPoint.y = origin.y + direction.y*t;
   iPoint.z = origin.z + direction.z*t;

   return(true);
}

//=============================================================================
// raySegmentIntersectionTriangle
//
// Computes the intersection the ray defined by origin and vector with the triangle
// defined by the vertex array.  If an intersection is found, true is returned and
// iPoint is filled in with the point of intersection.  Otherwise, false is returned.
//=============================================================================
bool raySegmentIntersectionTriangle(const BVector *vertex, const DWORD vertexIndex[3], const BVector &normal, 
   const BVector &origin, const BVector &direction, const bool segment, BVector &iPoint, const float errorEpsilon)
{
   // Adapted from Graphic Gems I (p. 390)

   // First we calculate the intersection of the ray with the plane defined by the
   // triangle.
   bool result = raySegmentIntersectionPlane(vertex[vertexIndex[0]], normal, origin, direction, segment, iPoint, errorEpsilon);
   if(!result)
      return(false);

   result = pointInTriangle(iPoint, vertex, vertexIndex, normal, errorEpsilon);
   return(result);
}

//=============================================================================
// raySegmentIntersectionTriangle
//
// Computes the intersection the ray defined by origin and vector with the triangle
// defined by the vertex array.  If an intersection is found, true is returned and
// iPoint is filled in with the point of intersection.  Otherwise, false is returned.
//=============================================================================
bool raySegmentIntersectionTriangle(const BVector *vertex, const WORD vertexIndex[3], const BVector &normal, 
                                    const BVector &origin, const BVector &direction, const bool segment, 
                                    BVector &iPoint, const float errorEpsilon)
{
   // Adapted from Graphic Gems I (p. 390)

   // First we calculate the intersection of the ray with the plane defined by the
   // triangle.
   bool result = raySegmentIntersectionPlane(vertex[vertexIndex[0]], normal, origin, direction, segment, iPoint, errorEpsilon);
   if(!result)
      return(false);

   result = pointInTriangle(iPoint, vertex, vertexIndex, normal, errorEpsilon);
   return(result);
}

//=============================================================================
// raySegmentIntersectionTriangle
//=============================================================================
bool raySegmentIntersectionTriangle(const BVector vertex[3], const BVector &normal, 
   const BVector &origin, const BVector &direction, const bool segment, 
   BVector &iPoint, const float errorEpsilon)
{
   // This could be made faster by writing a specialized, non-indexed version...
   static const DWORD sIndexList[3]={0, 1, 2};
   bool result=raySegmentIntersectionTriangle(vertex, sIndexList, normal, origin, direction, segment, iPoint, errorEpsilon);
   return(result);
}


//=============================================================================
// pointInTriangle
//=============================================================================
bool pointInTriangle(const BVector &point, const BVector *vertex, const DWORD vertexIndex[3], const BVector &normal, 
   const float errorEpsilon)
{
   // Get pointers to each vertex.
   const BVector *vert0 = vertex+vertexIndex[0];
   const BVector *vert1 = vertex+vertexIndex[1];
   const BVector *vert2 = vertex+vertexIndex[2];

   float u[3];
   float v[3];
   // Now choose a plane to project onto base on the dominant axis of the triangle.
   float absnx = _fabs(normal.x);
   float absny = _fabs(normal.y);
   float absnz = _fabs(normal.z);
   if(absnx > absny && absnx > absnz)
   {
      // Project onto yz plane.
      u[0] = point.y - vert0->y;
      u[1] = vert1->y - vert0->y;
      u[2] = vert2->y - vert0->y;
      v[0] = point.z - vert0->z;
      v[1] = vert1->z - vert0->z;
      v[2] = vert2->z - vert0->z;
   }
   else if(absny > absnz)
   {
      // Project onto xz plane.
      u[0] = point.x - vert0->x;
      u[1] = vert1->x - vert0->x;
      u[2] = vert2->x - vert0->x;
      v[0] = point.z - vert0->z;
      v[1] = vert1->z - vert0->z;
      v[2] = vert2->z - vert0->z;
   }
   else
   {
      // Project onto xy plane.
      u[0] = point.x - vert0->x;
      u[1] = vert1->x - vert0->x;
      u[2] = vert2->x - vert0->x;
      v[0] = point.y - vert0->y;
      v[1] = vert1->y - vert0->y;
      v[2] = vert2->y - vert0->y;
   }

   float alpha, beta;
   if(_fabs(u[1]) < errorEpsilon)
   {
      beta = u[0]/u[2];
      if(beta<-cFloatCompareEpsilon || beta>1.0f+errorEpsilon)
         return(false);

      alpha = (v[0] - beta*v[2])/v[1];
   }
   else
   {
      beta = (v[0]*u[1] - u[0]*v[1])/(v[2]*u[1] - u[2]*v[1]);
      if(beta<-errorEpsilon || beta>1.0f+errorEpsilon)
         return(false);

      alpha = (u[0] - beta*u[2])/u[1];
   }

   if(alpha+errorEpsilon>0.0f && (alpha+beta)<=1.0f+errorEpsilon)
      return(true);
   else
      return(false);
}

//=============================================================================
// pointInTriangle
//=============================================================================
bool pointInTriangle(const BVector &point, const BVector *vertex, const WORD vertexIndex[3], const BVector &normal, 
   const float errorEpsilon)
{
   // Get pointers to each vertex.
   const BVector *vert0 = vertex+vertexIndex[0];
   const BVector *vert1 = vertex+vertexIndex[1];
   const BVector *vert2 = vertex+vertexIndex[2];

   float u[3];
   float v[3];
   // Now choose a plane to project onto base on the dominant axis of the triangle.
   float absnx = _fabs(normal.x);
   float absny = _fabs(normal.y);
   float absnz = _fabs(normal.z);
   if(absnx > absny && absnx > absnz)
   {
      // Project onto yz plane.
      u[0] = point.y - vert0->y;
      u[1] = vert1->y - vert0->y;
      u[2] = vert2->y - vert0->y;
      v[0] = point.z - vert0->z;
      v[1] = vert1->z - vert0->z;
      v[2] = vert2->z - vert0->z;
   }
   else if(absny > absnz)
   {
      // Project onto xz plane.
      u[0] = point.x - vert0->x;
      u[1] = vert1->x - vert0->x;
      u[2] = vert2->x - vert0->x;
      v[0] = point.z - vert0->z;
      v[1] = vert1->z - vert0->z;
      v[2] = vert2->z - vert0->z;
   }
   else
   {
      // Project onto xy plane.
      u[0] = point.x - vert0->x;
      u[1] = vert1->x - vert0->x;
      u[2] = vert2->x - vert0->x;
      v[0] = point.y - vert0->y;
      v[1] = vert1->y - vert0->y;
      v[2] = vert2->y - vert0->y;
   }

   float alpha, beta;
   if(_fabs(u[1]) < errorEpsilon)
   {
      beta = u[0]/u[2];
      if(beta<-cFloatCompareEpsilon || beta>1.0f+errorEpsilon)
         return(false);

      alpha = (v[0] - beta*v[2])/v[1];
   }
   else
   {
      beta = (v[0]*u[1] - u[0]*v[1])/(v[2]*u[1] - u[2]*v[1]);
      if(beta<-errorEpsilon || beta>1.0f+errorEpsilon)
         return(false);

      alpha = (u[0] - beta*u[2])/u[1];
   }

   if(alpha+errorEpsilon>0.0f && (alpha+beta)<=1.0f+errorEpsilon)
      return(true);
   else
      return(false);
}

//=============================================================================
// pointInTriangle
//=============================================================================
bool pointInTriangle(const BVector point, const BVector v1, const BVector v2, const BVector v3, const BVector normal, const float errorEpsilon)
{
   float u[3];
   float v[3];
   // Now choose a plane to project onto base on the dominant axis of the triangle.
   float absnx = _fabs(normal.x);
   float absny = _fabs(normal.y);
   float absnz = _fabs(normal.z);
   if(absnx > absny && absnx > absnz)
   {
      // Project onto yz plane.
      u[0] = point.y - v1.y;
      u[1] = v2.y - v1.y;
      u[2] = v3.y - v1.y;
      v[0] = point.z - v1.z;
      v[1] = v2.z - v1.z;
      v[2] = v3.z - v1.z;
   }
   else if(absny > absnz)
   {
      // Project onto xz plane.
      u[0] = point.x - v1.x;
      u[1] = v2.x - v1.x;
      u[2] = v3.x - v1.x;
      v[0] = point.z - v1.z;
      v[1] = v2.z - v1.z;
      v[2] = v3.z - v1.z;
   }
   else
   {
      // Project onto xy plane.
      u[0] = point.x - v1.x;
      u[1] = v2.x - v1.x;
      u[2] = v3.x - v1.x;
      v[0] = point.y - v1.y;
      v[1] = v2.y - v1.y;
      v[2] = v3.y - v1.y;
   }

   float alpha, beta;
   if(_fabs(u[1]) < errorEpsilon)
   {
      beta = u[0]/u[2];
      if(beta<-cFloatCompareEpsilon || beta>1.0f+errorEpsilon)
         return(false);

      alpha = (v[0] - beta*v[2])/v[1];
   }
   else
   {
      beta = (v[0]*u[1] - u[0]*v[1])/(v[2]*u[1] - u[2]*v[1]);
      if(beta<-errorEpsilon || beta>1.0f+errorEpsilon)
         return(false);

      alpha = (u[0] - beta*u[2])/u[1];
   }

   if(alpha+errorEpsilon>0.0f && (alpha+beta)<=1.0f+errorEpsilon)
      return(true);
   else
      return(false);
}

//=============================================================================
// simple helper class
//=============================================================================
class BXZCWPair
{
   public:
      long index;
      float cosine;
};
//const long cMaxPairs=1024;
const long cMaxPairs=10000;
static BXZCWPair sPairs[cMaxPairs];

//=============================================================================
// sortCompareXZCW
//
// qsort callback
//=============================================================================
int __cdecl sortCompareXZCW(const void* o1, const void* o2)
{
   BXZCWPair* v1=(BXZCWPair*)o1;
   BXZCWPair* v2=(BXZCWPair*)o2;
   if (v1->cosine < v2->cosine)
      return -1;
   else
      return 1;
}

//=============================================================================
// orderXZCW
//
// Makes a sorted list of indices to the points in clockwise order as viewed 
// looking down on the xz plane
//=============================================================================
bool orderXZCW(const BVector *vertices, const long num, long *sorted)
{
   // Check for bogus parameters.
   if(!vertices || !sorted)
   {
      BASSERT(0);
      return(false);
   }

   if(num > cMaxPairs)
   {
      BASSERT(0);
      return(false);
   }

   // With less than 3 vertices, we have a trivial/degenerate case.
   if(num < 3)
   {
      for(long i=0; i<num; i++)
         sorted[i] = i;
      return(true);
   }

   // Get the vertex with the minimum z coordinate
   float minZ = vertices[0].z;
   long minZVertex = 0;
   for(long i=1; i<num; i++)
   {
      if(vertices[i].z < minZ)
      {
         minZ = vertices[i].z;
         minZVertex = i;
      }
   }

   // Make it vertex 0 (if it isn't already)
   sorted[0] = minZVertex;

   // Compute the cosine of the angle between the x-axis at vertex 0 and each of the
   // points.
   long listIndex=1;
   for(long i=0; i<num; i++)
   {
      if(i==sorted[0])
         continue;
         
      sPairs[listIndex].index = i;
      float px = vertices[i].x-vertices[sorted[0]].x;
      float pz = vertices[i].z-vertices[sorted[0]].z;
      float dsqr = px*px + pz*pz;
      if(dsqr < cFloatCompareEpsilon)
         sPairs[listIndex].cosine = 0.0f;
      else
         sPairs[listIndex].cosine = px/(float)sqrt(dsqr);

      listIndex++;
   }

   // Sort the points by increasing cosine
   qsort((void*)(&sPairs[1]), num-1, sizeof(BXZCWPair), sortCompareXZCW);

   // Copy indices.
   for(long i=1; i<num; i++)
      sorted[i] = sPairs[i].index;

   return(true);
}


//=============================================================================
// distanceToLineSqr
//=============================================================================
float distanceToLineSqr(float x, float y, float px, float py, float dx, float dy)
{
   // Get vector length.
   float lenSqr = dx*dx + dy*dy;

   // If 0, just return distance to the point.
   if(lenSqr < cFloatCompareEpsilon)
   {
      dx = x-px;
      dy = y-py;
      return(dx*dx + dy*dy);
   }

   // Get parameter.
   float t = ((x-px)*dx + (y-py)*dy)/lenSqr;

   float newx = px+t*dx;
   float newy = py+t*dy;

   dx = x-newx;
   dy = y-newy;

   return(dx*dx + dy*dy);
}

//=============================================================================
// distanceToLine
//=============================================================================
float distanceToLine(float x, float y, float px, float py, float dx, float dy)
{
   return((float)sqrt(distanceToLineSqr(x, y, px, py, dx, dy)));
}


//=============================================================================
// distanceToSegmentSqr
//=============================================================================
float distanceToSegmentSqr(float x, float y, float sx1, float sy1, float sx2, float sy2)
{
   float dx, dy;

   float s1s2x = sx2-sx1;
   float s1s2y = sy2-sy1;

   //If the dot of p1p and p1p2 is negative, the point is not on the segment.
   float s1px = x-sx1;
   float s1py = y-sy1;
   float dP=s1s2x*s1px + s1s2y*s1py;
   if(dP < 0.0f)
   {
      // Get distance from s1.
      dx = sx1-x;
      dy = sy1-y;
      return(dx*dx + dy*dy);
   }

   //If the dot of p2p and the negative of p1p2 (to reverse dir) is negative,
   //the point is not on the segment.
   float ps2x = sx2-x;
   float ps2y = sy2-y;
   dP=s1s2x*ps2x + s1s2y*ps2y;
   if(dP < 0.0f)
   {
      // Get distance from s2.
      dx = sx2-x;
      dy = sy2-y;
      return(dx*dx + dy*dy);
   }

   return(distanceToLineSqr(x, y, sx1, sy1, s1s2x, s1s2y));
}

//=============================================================================
// closestPointOnSegment
//=============================================================================
float closestPointOnSegment(float x, float y, float sx1, float sy1, float sx2, float sy2, float &res_x, float &res_y)
{
   float s1s2x = sx2-sx1;
   float s1s2y = sy2-sy1;

   //If the dot of p1p and p1p2 is negative, the point is not on the segment.
   float s1px = x-sx1;
   float s1py = y-sy1;
   float dP=s1s2x*s1px + s1s2y*s1py;
   if(dP < 0.0f)
   {
      // return normalized distance to s1
      res_x = sx1;
      res_y = sy1;
      return(0.0f);
   }

   //If the dot of p2p and the negative of p1p2 (to reverse dir) is negative,
   //the point is not on the segment.
   float ps2x = sx2-x;
   float ps2y = sy2-y;
   dP=s1s2x*ps2x + s1s2y*ps2y;
   if(dP < 0.0f)
   {
      // return normalized distance to s2
      res_x = sx2;
      res_y = sy2;
      return(1.0f);
   }

   // We know the closest point in the segment, compute it using one of the last
   // dop product.  Either one is fine.
   float length_s1s2 = sqrt((s1s2x * s1s2x) + (s1s2y * s1s2y));

   float distance = dP / length_s1s2;
   distance = 1.0f - distance / length_s1s2;       // normalize and invert

   res_x = sx1 + (distance * s1s2x);
   res_y = sy1 + (distance * s1s2y);

   return(distance);
}

//=============================================================================
// distanceToSegment
//=============================================================================
float distanceToSegment(float x, float y, float sx1, float sy1, float sx2, float sy2)
{
   return((float)sqrt(distanceToSegmentSqr(x, y, sx1, sy1, sx2, sy2)));
}


//=============================================================================
// distanceBetweenSegments
//=============================================================================
float distanceBetweenSegmentsSqr(float x11, float y11, float x12, float y12, float x21, float y21, float x22, float y22)
{
   float r, s;

   // If they intersect, distance is 0.
   if(segIntersect(x11, y11, x12, y12, x21, y21, x22, y22, r, s) == cIntersection)
      return(0.0f);

   // Otherwise, distance of each endpoint to the other segment.
   float bestDistSqr = distanceToSegmentSqr(x11, y11, x21, y21, x22, y22);

   float thisDistSqr = distanceToSegmentSqr(x12, y12, x21, y21, x22, y22);
   bestDistSqr = min(bestDistSqr, thisDistSqr);

   thisDistSqr = distanceToSegmentSqr(x21, y21, x11, y11, x12, y12);
   bestDistSqr = min(bestDistSqr, thisDistSqr);

   thisDistSqr = distanceToSegmentSqr(x22, y22, x11, y11, x12, y12);
   bestDistSqr = min(bestDistSqr, thisDistSqr);

   return(bestDistSqr);
}


//=============================================================================
// distanceBetweenSegments
//=============================================================================
float distanceBetweenSegments(float x11, float y11, float x12, float y12,
   float x21, float y21, float x22, float y22)
{
   return((float)sqrt(distanceBetweenSegmentsSqr(x11, y11, x12, y12, x21, y21, x22, y22)));
}

//=============================================================================
// factorial
//=============================================================================
long factorial(long n)
{
   long rVal=n;
   while (n > 1)
   {
      n--;
      rVal*=n;
   }
   return(rVal);
}



//=============================================================================
// vectorToRGBA
//=============================================================================
DWORD vectorToRGBA(const BVector &vector)
{
	DWORD red = min(255, (DWORD)((vector.x+1.0f)*127.5f));
	DWORD green = min(255, (DWORD)((vector.y+1.0f)*127.5f));
	DWORD blue  = min(255, (DWORD)((vector.z+1.0f)*127.5f));
#define VARGBToDWORD(a, r, g, b) DWORD((a<<24)|(r<<16)|(g<<8)|(b))
	return(VARGBToDWORD(255, red, green, blue));
}


//==============================================================================
// logBase2WithError(WORD val)
// 
// Returns the base 2 log of val if it is an even power of 2, otherwise returns
// negative closest log base 2
//==============================================================================
template <class Type> long logBase2WithErrorTemplate(Type val)
{
   long firstOnePos = -1;
   long ones = 0;

   #pragma warning(disable: 4244)

   for(long i=0;i<sizeof(WORD)*8;i++)
   {
      if(val & 0x1)
      {
         firstOnePos = i;
         ones++;
      }
      val = val >> 1;
   }
   
   #pragma warning(default: 4244)

   // power of two can only have one '1' in its binary representation
   if(ones == 1)
      return firstOnePos;

   firstOnePos++;
   return(-firstOnePos);
} // logBase2WithError


//=============================================================================
// logBase2WithError
//=============================================================================
long logBase2WithError(WORD val)
{
   return(logBase2WithErrorTemplate(val));
}


//=============================================================================
// logBase2WithError
//=============================================================================
long logBase2WithError(DWORD val)
{
   return(logBase2WithErrorTemplate(val));
}


//=============================================================================
// intersectSphere - Intersect a ray with a sphere
//=============================================================================
float intersectSphere(const BVector &rayOrigin, const BVector &rayVector, const BVector &sphereOrigin, float radius)
{
   BVector Q = sphereOrigin - rayOrigin;
   float c = Q.length();
   float v = Q.dot(rayVector);
   float d = (radius * radius) - ((c * c) - (v * v));

   // If there was no intersection, return -1
   if (d < 0.0f) 
      return -1.0f;

   // Return the distance to the [first] intersecting point
   return (float)(v - sqrt(d));
} 

//=============================================================================
// closestPointOnTriangle - Calculate the closest point on a triangle
//=============================================================================
BVector closestPointOnTriangle(const BVector *vertex, const WORD vertexIndex[3], const BVector &point)
{
   BVector closestPoint[3];
   closestPoint[0] = closestPointOnLine(vertex[vertexIndex[0]], vertex[vertexIndex[1]], point);
   closestPoint[1] = closestPointOnLine(vertex[vertexIndex[1]], vertex[vertexIndex[2]], point);
   closestPoint[2] = closestPointOnLine(vertex[vertexIndex[2]], vertex[vertexIndex[0]], point);

   long closestIndex = 0;
   float closestDistance = 0.0f;
   for (long i=0; i<3; i++)
   {
      BVector v = closestPoint[i] - point;
      float distance = v.lengthSquared();
      if (i == 0 || distance < closestDistance)
      {
         closestIndex = i;
         closestDistance = distance;
      }
   }

   return closestPoint[closestIndex];
}

//=============================================================================
// closestPointOnTriangle - Calculate the closest point on a triangle
//=============================================================================
BVector closestPointOnTriangle(const BVector vert1, const BVector vert2, const BVector vert3, const BVector point)
{
   BVector closestPoint[3];
   closestPoint[0] = closestPointOnLine(vert1, vert2, point);
   closestPoint[1] = closestPointOnLine(vert2, vert3, point);
   closestPoint[2] = closestPointOnLine(vert3, vert1, point);

   long closestIndex = 0;
   float closestDistance = 0.0f;
   for (long i=0; i<3; i++)
   {
      BVector v = closestPoint[i] - point;
      float distance = v.lengthSquared();
      if (i == 0 || distance < closestDistance)
      {
         closestIndex = i;
         closestDistance = distance;
      }
   }

   return closestPoint[closestIndex];
}

//=============================================================================
// closestPointOnLine - Calculate the closest point on a line
//=============================================================================
BVector closestPointOnLine(const BVector &a, const BVector &b, const BVector &point)
{
   // Determine t (the length of the vector from ‘a’ to ‘point’)
   BVector c = point - a;
   BVector V = b - a;
   float d = V.length();
   V.normalize();
   float t = V.dot(c);

   // Check to see if ‘t’ is beyond the extents of the line segment
   if (t < 0.0f)
      return a;

   if (t > d)
      return b;

   // Return the point between ‘a’ and ‘b’
   V = V * t;
   return a + V;
}

//=============================================================================
// expandSeries: Used to get the 0.5, 0.25, 0.125, etc. "logarithmic" decay.
//=============================================================================
float expandSeries(float base, float factor, long number)
{
   float rVal=base;
   for (long i=0; i < number; i++)
   {
      base*=factor;
      rVal+=base;
   }

   return(rVal);
}

//=============================================================================
// checkSide
//=============================================================================
bool checkSide(const BVector *points, long count, float dx, float dz, float vx, float vz, float errorEpsilon)
{
   // Vertices are projected to the form v+t*d.
   // Return value is +1 if all t > 0, -1 if all t < 0, 0 otherwise, in
   // which case the line splits the polygon.
   for (long i = count-1; i>=0; i--)
   {
      float x=points[i].x-vx;
      float z=points[i].z-vz;
      float t = x*dx+z*dz;
      if(t<errorEpsilon)
         return(false);
   }
   return(true);
}

//=============================================================================
// hullsOverlapXZ
//=============================================================================
bool hullsOverlapXZ(const BVector *hull1, long hull1Count, const BVector *hull2, long hull2Count, float errorEpsilon)
{
   long i0, i1;
   for (i0 = 0, i1 = hull1Count-1; i0 < hull1Count; i1 = i0, i0++)
   {
      float nx=hull1[i1].z-hull1[i0].z;
      float nz=hull1[i0].x-hull1[i1].x;
      if (checkSide(hull2, hull2Count, nx, nz, hull1[i0].x, hull1[i0].z, errorEpsilon))
      { // C1 is entirely on ‘positive’ side of line C0.V(i0)+t*D
         return false;
      }
   }
   // Test edges of C1 for separation. Because of the clockwise ordering,
   // the projection interval for C1 is [m,0] where m <= 0. Only try to determine
   // if C0 is on the ‘positive’ side of the line.
   for (i0 = 0, i1 = hull2Count-1; i0 < hull2Count; i1 = i0, i0++)
   {
      float nx=hull2[i1].z-hull2[i0].z;
      float nz=hull2[i0].x - hull2[i1].x;
      if (checkSide(hull1, hull1Count, nx, nz, hull2[i0].x, hull2[i0].z, errorEpsilon))
      { // C0 is entirely on ‘positive’ side of line C1.V(i0)+t*D
         return false;
      }
   }

   return(true);
}

//=============================================================================
// raySegmentIntersectionTriangle
//=============================================================================
bool raySegmentIntersectionTriangle(const BVector vertex[3], const BVector &origin, const BVector &direction, 
                                    const bool segment, BVector &iPoint, const float errorEpsilon)
{
#ifdef XBOX
   
   //CLM [05.05.08] Moved to VMX to reduce lots of int->vector->float LHS problems..
   const float cOne = 1.0f;
   const XMVECTOR errEps = XMVectorReplicate(errorEpsilon);
   const XMVECTOR errEpsNeg = XMVectorNegate(errEps);
   const XMVECTOR errEpsOne = XMVectorAdd(XMVectorReplicate(cOne),errEps);


   const XMVECTOR edge1 = XMVectorSubtract(vertex[1],vertex[0]);
   const XMVECTOR edge2 = XMVectorSubtract(vertex[2],vertex[0]);

   const XMVECTOR pvec = XMVector3Cross(direction,edge2);
   const XMVECTOR pvecLen = XMVector3Dot(edge1,pvec);
   const XMVECTOR rcpDet = XMVectorReciprocal(pvecLen);
   
   if(XMVector3Greater(pvecLen,errEpsNeg) && XMVector3Less(pvecLen,errEps))
   {
      return false;
   }
  
   
   const XMVECTOR Uvec = XMVectorSubtract(origin,vertex[0]);
   const XMVECTOR UvecdotRes = XMVector3Dot(Uvec,pvec);
   const XMVECTOR UvecdotScaled = XMVectorMultiply(UvecdotRes,rcpDet);

   if(XMVector3Less(UvecdotScaled,errEpsNeg) || XMVector3Greater(UvecdotScaled,errEpsOne))
   {
      return false;
   }
   
   
   const XMVECTOR qvec = XMVector3Cross(Uvec,edge1);
   const XMVECTOR qvecdotRes = XMVector3Dot(direction,qvec);
   const XMVECTOR qvecdotScaled = XMVectorMultiply(qvecdotRes,rcpDet);

   if(XMVector3Less(qvecdotScaled,errEpsNeg) || XMVector3Greater(XMVectorAdd(UvecdotScaled,qvecdotScaled),errEpsOne))
   {
      return false;
   }
   
   

   const XMVECTOR Tvec = XMVector3Dot(edge2,qvec);
   const XMVECTOR TvecScaled = XMVectorMultiply(Tvec,rcpDet);

   // See if we're off the ray/segment.
   if(XMVector3Less(TvecScaled,errEpsNeg) || (segment && XMVector3Greater(TvecScaled,errEpsOne)))
   {
      return false;
   }
   
   
   // Get intersection point.
   iPoint = XMVectorAdd(origin,XMVectorMultiply(direction,TvecScaled));

   return true;


#else
//   if (segment)
   {
      // Moller/Trumbore method -- supposedly quick without requiring the normal.
      // Also computes the barycentric coordinates of the intersection point if we wanted to give 
      // those back for some reason.

      // Get vectors along the two triangle edges sharing vertex 0.
      BVector edge1;
      edge1.assignDifference(vertex[1], vertex[0]);
      BVector edge2;
      edge2.assignDifference(vertex[2], vertex[0]);

      // Calc determinant.
      BVector pvec;
      pvec.assignCrossProduct(direction, edge2);
      float det=edge1.dot(pvec);

      // If determinant is near, the ray is in the plane of the triangle.  In that case there is no intersection.
      if((det > -errorEpsilon) && (det < errorEpsilon))
         return(false);

      // Get reciprocal.
      const float recipDet= 1.0f/det;

      // Calc dist from vertex 0 to origin
      BVector tvec;
      tvec.assignDifference(origin, vertex[0]);

      // Get u param of barycentric coords.
      float u=(tvec.dot(pvec))*recipDet;

      // See if it's inside triangle.
      if(u<-errorEpsilon || u>1.0f+errorEpsilon)
         return(false);

      // Calc vector for v portion.
      BVector qvec;
      qvec.assignCrossProduct(tvec, edge1);

      // Calc v.
      float v=(direction.dot(qvec))*recipDet;

      // See if it's inside triangle.
      if(v<-errorEpsilon || u+v>1.0f+errorEpsilon)
         return(false);

      // Compute t.
      float t=(edge2.dot(qvec))*recipDet;

      // See if we're off the ray/segment.
      if(t<-errorEpsilon || (segment && t>1.0f+errorEpsilon))
         return(false);

      // Get intersection point.
      iPoint.assignSum(origin, t*direction);

      return(true);
   }


#endif
/*
   BPluecker   tri[3];
   BPluecker   ray;

   ray.setFromDirection( direction, origin );
   tri[0].set( vertex[2], vertex[1] );
   tri[1].set( vertex[0], vertex[2] );
   tri[2].set( vertex[1], vertex[0] );

   float d[3];
   d[0] = tri[0].dotProduct( ray );
   if (d[0]<0.0f) return (false);

   d[1] = tri[1].dotProduct( ray );
   if (d[1]<0.0f) return (false);

   d[2] = tri[2].dotProduct( ray );
   if (d[2]<0.0f) return (false);

   const float detr = 1.0f / (d[0]+d[1]+d[2]);
   d[0] *= detr;
   d[1] *= detr;
   d[2] *= detr;
   iPoint.x = vertex[0].x * d[0] + vertex[1].x * d[1] + vertex[2].x * d[2];
   iPoint.y = vertex[0].y * d[0] + vertex[1].y * d[1] + vertex[2].y * d[2];
   iPoint.z = vertex[0].z * d[0] + vertex[1].z * d[1] + vertex[2].z * d[2];

   return (true);
*/
}


//=============================================================================
// raySegmentIntersectionTriangle
//=============================================================================
/*
bool raySegmentIntersectionTriangle(const BVector vertex[3], const BVector &origin, const BVector &direction, 
   const bool segment, BVector &iPoint, const float errorEpsilon)
{
   // Moller/Trumbore method -- supposedly quick without requiring the normal.
   // Also computes the barycentric coordinates of the intersection point if we wanted to give 
   // those back for some reason.

   // Get vectors along the two triangle edges sharing vertex 0.
   BVector edge1=vertex[1]-vertex[0];
   BVector edge2=vertex[2]-vertex[0];

   // Calc determinant.
   BVector pvec=direction.cross(edge2);
   float det=edge1.dot(pvec);

   // If determinant is near, the ray is in the plane of the triangle.  In that case there is no intersection.
   if((det > -errorEpsilon) && (det < errorEpsilon))
      return(false);

   // Get reciprocal.
   float recipDet=1.0f/det;

   // Calc dist from vertex 0 to origin
   BVector tvec=origin-vertex[0];

   // Get u param of barycentric coords.
   float u=(tvec.dot(pvec))*recipDet;

   // See if it's inside triangle.
   if(u<-errorEpsilon || u>1.0f+errorEpsilon)
      return(false);

   // Calc vector for v portion.
   BVector qvec=tvec.cross(edge1);

   // Calc v.
   float v=(direction.dot(qvec))*recipDet;

   // See if it's inside triangle.
   if(v<-errorEpsilon || u+v>1.0f+errorEpsilon)
      return(false);

   // Compute t.
   float t=(edge2.dot(qvec))*recipDet;

   // See if we're off the ray/segment.
   if(t<-errorEpsilon || (segment && t>1.0f+errorEpsilon))
      return(false);
   
   // Get intersection point.
   iPoint=origin+t*direction;

   return(true);
}
*/

//=============================================================================
// raySegmentIntersectionTriangle
// 
// Indexed version, otherwise identical to above
//=============================================================================
bool raySegmentIntersectionTriangle(const BVector vertex[3], const WORD index[3], const BVector &origin, const BVector &direction, 
   const bool segment, BVector &iPoint, const float errorEpsilon)
{
   // Moller/Trumbore method -- supposedly quick without requiring the normal.
   // Also computes the barycentric coordinates of the intersection point if we wanted to give 
   // those back for some reason.

   // Get vectors along the two triangle edges sharing vertex 0.
   BVector edge1;
   edge1.assignDifference(vertex[index[1]], vertex[index[0]]);
   BVector edge2;
   edge2.assignDifference(vertex[index[2]], vertex[index[0]]);

   // Calc determinant.
   BVector pvec;
   pvec.assignCrossProduct(direction, edge2);
   float det=edge1.dot(pvec);

   // If determinant is near, the ray is in the plane of the triangle.  In that case there is no intersection.
   if((det > -errorEpsilon) && (det < errorEpsilon))
      return(false);

   // Get reciprocal.
   float recipDet=1.0f/det;

   // Calc dist from vertex 0 to origin
   BVector tvec;
   tvec.assignDifference(origin, vertex[index[0]]);

   // Get u param of barycentric coords.
   float u=(tvec.dot(pvec))*recipDet;

   // See if it's inside triangle.
   if(u<-errorEpsilon || u>1.0f+errorEpsilon)
      return(false);

   // Calc vector for v portion.
   BVector qvec;
   qvec.assignCrossProduct(tvec, edge1);

   // Calc v.
   float v=(direction.dot(qvec))*recipDet;

   // See if it's inside triangle.
   if(v<-errorEpsilon || u+v>1.0f+errorEpsilon)
      return(false);

   // Compute t.
   float t=(edge2.dot(qvec))*recipDet;

   // See if we're off the ray/segment.
   if(t<-errorEpsilon || (segment && t>1.0f+errorEpsilon))
      return(false);
   
   // Get intersection point.
   iPoint.assignSum(origin, t*direction);

   return(true);
}


//=============================================================================
// raySegmentIntersectionPlane
// 
// This version only looks at the plane defined by the triangle
//=============================================================================
bool raySegmentIntersectionPlane(const BVector vertex[3], const WORD index[3], const BVector &origin, const BVector &direction, 
   const bool segment, BVector &iPoint, const float errorEpsilon)
{
   // Moller/Trumbore method -- supposedly quick without requiring the normal.
   // Also computes the barycentric coordinates of the intersection point if we wanted to give 
   // those back for some reason.

   // Get vectors along the two triangle edges sharing vertex 0.
   BVector edge1=vertex[index[1]]-vertex[index[0]];
   BVector edge2=vertex[index[2]]-vertex[index[0]];

   // Calc determinant.
   BVector pvec=direction.cross(edge2);
   float det=edge1.dot(pvec);

   // If determinant is near, the ray is in the plane of the triangle.  In that case there is no intersection.
   if((det > -errorEpsilon) && (det < errorEpsilon))
      return(false);

   // Get reciprocal.
   float recipDet=1.0f/det;

   // Calc dist from vertex 0 to origin
   BVector tvec=origin-vertex[index[0]];

   // Get u param of barycentric coords.
   float u=(tvec.dot(pvec))*recipDet;

   // See if it's inside triangle.
   if(u<-errorEpsilon || u>1.0f+errorEpsilon)
      return(false);

   // Calc vector for v portion.
   BVector qvec=tvec.cross(edge1);

   /*
   // Calc v.
   float v=(direction|qvec)*recipDet;

   // See if it's inside triangle.
   if(v<-errorEpsilon || u+v>1.0f+errorEpsilon)
      return(false);
   */

   // Compute t.
   float t=(edge2.dot(qvec))*recipDet;

   // See if we're off the ray/segment.
   if(t<-errorEpsilon || (segment && t>1.0f+errorEpsilon))
      return(false);
   
   // Get intersection point.
   iPoint=origin+t*direction;

   return(true);
}

//=============================================================================
// powerOfTwoGreaterOrEqual(long num)
//
// Returns the smallest integer that is a power of 2 and >= num (or -1 on error)
//=============================================================================
long powerOfTwoGreaterOrEqual(long num)
{
   if(num < 1)
      return -1;

   long result = 1;
   while(result<num)
      result *= 2;

   return result;
}


//=============================================================================
// calculatePolyArea
//=============================================================================
#pragma warning(disable: 4701)
float calculatePolyArea(BVector* points, long numberPoints)
{
   if(numberPoints<3)
      return(0.0f);

   //Calculate the area of this poly (assuming we have at least three points).
   //Take half of the surface normal dotted with the sum of the triangle fan
   //cross products (out of the CGA FAQ).
   BVector cPTotal(0.0f, 0.0f, 0.0f);
   BVector surfaceNormal;
   for (long b=0; b < numberPoints-2; b++)
   {
      BVector vec1=points[b+1]-points[0];
      BVector vec2=points[b+2]-points[0];
      BVector cross=vec1.cross(vec2);
      if (b == 0)
         surfaceNormal=cross;
      cPTotal+=cross;
   }
   surfaceNormal.normalize();
   return((float)fabs((surfaceNormal.dot(cPTotal))*0.5f));
}
#pragma warning(default: 4701)


//=============================================================================
// findXZConvexHull
//
// jce 9/29/99 -- Note: returns points in CCW order.
//=============================================================================
void findXZConvexHull(BVector *points, long numberPoints, BDynamicSimVectorArray& result)
{
   result.clear(); // zeroNumber();

   if(!points || numberPoints<=0)
      return;

   //Find the point with the smallest Z value and add that as our first point.
   //If two points have the smallest Z, we'll take the one with the smallest X.
   long startIndex=0;
   float startZ=points[0].z;
   for (long i=1; i < numberPoints; i++)
   {
      if ((points[i].z < startZ) ||
         ((points[i].z == startZ) && (points[i].x < points[startIndex].x)))
      {
         startZ=points[i].z;
         startIndex=i;
      }
   }
   result.uniqueAdd(points[startIndex]);

   // Create array of indices.
   static BDynamicSimLongArray indices;
   indices.setNumber(numberPoints);
   for(long i=0; i<numberPoints; i++)
      indices[i] = i;
   // Remove startindex from list.
   indices[startIndex] = indices[numberPoints-1];
   indices.setNumber(numberPoints-1);

   //We set our current circle tan value to 0 (since we're starting with the
   //smallest Z (and X if Z ties) valued point).
   float currentCT=0.0f;
   long currentIndex=startIndex;

   //Do the wrap.  Run through the points (until we get back to the start point)
   //and add in the points with the smallest theta from the current point.

   #pragma warning(disable: 4127)

   do 
   {
      long bestIndex=-1;
      float bestCT=0.0f;
      float bestDistance=0.0f;
      long bestIndicesIndex=-1;
      for (long i=0; i < indices.getNumber(); i++)
      {
         long checkIndex = indices[i];

         //Calc the circle tan between this point and the current point.
         float dX=points[checkIndex].x-points[currentIndex].x;
         float dZ=points[checkIndex].z-points[currentIndex].z;
         float cT=circleTan(dX, dZ);
         //If the circleTan is less than our currentCT, then it's not a point we want (since
         //it would wrap past the circle if it were on the hull).  We also don't want the
         //point if its cT is over 2PI (though that should never happen) for the same reason.
         if ((cT < currentCT) || (cT > cTwoPi))
            continue;
         //Calc the distance between the two points (because we really want the farthest
         //point if any points are colinear).
         float distance=(float)sqrt(dX*dX+dZ*dZ);

         //If this is the first pass through, take the point.  Otherwise, take this point
         //if its cT is less than our bestCT or the circle tans are colinear and this point
         //is farther from the last hull point than the best one.
         if ((bestIndex == -1) ||
            (cT < bestCT) ||
            ((fabs(cT-bestCT) < cFloatCompareEpsilon) && (distance > bestDistance)))
         {
            bestIndex=checkIndex;
            bestCT=cT;
            bestDistance=distance;
            bestIndicesIndex=i;
         }
      }

      //Calc the circle tan between the current point and the start point.  If
      //that is less than the best new point we could find, then we're done because
      //we've looped back to where the start point should be the next point on the
      //convex hull.
      float dX=points[startIndex].x-points[currentIndex].x;
      float dZ=points[startIndex].z-points[currentIndex].z;
      float startCT=circleTan(dX, dZ);
      float startDistance=(float)sqrt(dX*dX+dZ*dZ);
      if ((startCT < bestCT) ||
         ((fabs(startCT-bestCT) < cFloatCompareEpsilon) && (startDistance > bestDistance)))
         break;

      //If we don't have a best point, we're done.
      if (bestIndex == -1)
         break;

      // Add result.
      result.uniqueAdd(points[bestIndex]);

      // Remove from index list.
      long num = indices.getNumber();
      indices[bestIndicesIndex] = indices[num-1];
      indices.setNumber(num-1);

      currentCT=bestCT;
      currentIndex=bestIndex;

   } while (1) ;

   #pragma warning(default: 4127)
}


//=============================================================================
// computeHullMajorAxisEdge
//
// Computes the major axis of the convex hull -- the axis returned goes from
// the midpoint of one edge to the midpoint of another.
//=============================================================================
bool computeHullMajorAxisEdge(BDynamicSimVectorArray &hull, BVector &point1, BVector &point2)
{
   long numHullPoints = hull.getNumber();

   // If fewer than 3 points in "hull", then we don't have a hull.
   if(numHullPoints < 3)
      return(false);

   float maxDistanceSqr = 0.0f;
   for(long startVert1=0; startVert1<numHullPoints-2; startVert1++)
   {
      long endVert1 = startVert1+1;
      if(endVert1>=numHullPoints)
         endVert1 = 0;

      // Get midpoint of first segment.
      float midPoint1X = 0.5f*(hull[startVert1].x+hull[endVert1].x);
      float midPoint1Z = 0.5f*(hull[startVert1].z+hull[endVert1].z);

      for(long startVert2=endVert1; startVert2<numHullPoints; startVert2++)
      {
         long endVert2 = startVert2+1;
         if(endVert2>=numHullPoints)
            endVert2 = 0;

         // Get midpoint of second segment.
         float midPoint2X = 0.5f*(hull[startVert2].x+hull[endVert2].x);
         float midPoint2Z = 0.5f*(hull[startVert2].z+hull[endVert2].z);

         // Get distance from midpoint to midpoint.
         float dx = midPoint2X-midPoint1X;
         float dz = midPoint2Z-midPoint1Z;
         float distanceSqr = dx*dx + dz*dz;

         // If the distance is bigger than the best so far, remember this pair of points.
         if(distanceSqr>maxDistanceSqr)
         {
            maxDistanceSqr = distanceSqr;
            point1.x = midPoint1X;
            point1.z = midPoint1Z;
            point2.x = midPoint2X;
            point2.z = midPoint2Z;
         }
      }
   }

   return(true);
}


//=============================================================================
// pointBetweenLines
//
// Returns true if the given point (x,z) lies between the lines defined by
// (the xz projection of) p1 and p2 and the normal.
//=============================================================================
bool pointBetweenLines(const float x, const float z, const BVector &p1, const BVector &p2, 
   const BVector &normal)
{
   float a1 = normal.x;
   float b1 = normal.z;
   float c1 = -(a1*p1.x + b1*p1.z);

   float lineResult = a1*p2.x + b1*p2.z + c1;
   float a2,b2,c2;
   if(lineResult < 0.0f)
   {
      a1 = -normal.x;
      b1 = -normal.z;
      c1 = -(a1*p1.x + b1*p1.z);
   }

   a2 = normal.x;
   b2 = normal.z;
   c2 = -(a2*p2.x + b2*p2.z);
   lineResult = a2*p1.x + b2*p1.z + c2;
   if(lineResult < 0.0f)
   {
      a2 = -normal.x;
      b2 = -normal.z;
      c2 = -(a2*p2.x + b2*p2.z);
   }
   
   float result1 = a1*x+b1*z+c1;
   float result2 = a2*x+b2*z+c2;

   if(result1>0.0f && result2>0.0f)
      return(true);

   return(false);
}


//=============================================================================
// pointOnPlusSideOfLine
//=============================================================================
bool pointOnPlusSideOfLine(const float x, const float z, const BVector &p1, const BVector &normal)
{
   float a1 = normal.x;
   float b1 = normal.z;
   float c1 = -(a1*p1.x + b1*p1.z);

   float result = a1*x+b1*z+c1;

   if(result > 0.0f)
      return(true);

   return(false);
}


//=============================================================================
// makeAngleBetweenZeroAndTwoPi
//=============================================================================
float makeAngleBetweenZeroAndTwoPi(float angle)
{
   float ratio = angle/cTwoPi;
   return(cTwoPi*(ratio-(float)floor(ratio)));
}


//=============================================================================
// xyAngle
//=============================================================================
float xyAngle(float x, float y)
{
   BVector vector(x, 0.0f, y);
   vector.normalize();
   float angle;
   if (vector.x > cFloatCompareEpsilon)
      angle = (float)acos(vector.z);
   else if (vector.x < -cFloatCompareEpsilon)
      angle = cPi + (cPi - (float)acos(vector.z));
   else
   {
      if (vector.z < 0.0f)
         angle = cPi;
      else
         angle = 0.0f;
   }
   return angle;
}


//=============================================================================
// intersectCircles
//=============================================================================
long intersectCircles(float center0x, float center0y, float radius0, float center1x, float center1y, float radius1,
   float &ix0, float &iy0, float &ix1, float &iy1)
{
   // Delta between centers.
   float dx = center1x-center0x;
   float dy = center1y-center0y;

   // Distance between centers.
   float d = sqrtf(dx*dx + dy*dy);

   // If the circles don't overlap then there is no intersection.
   if(d > radius0 + radius1)
      return(cIntersectCirclesNone);

   // Check for containment.
   if(d<fabs(radius0-radius1))
      return(cIntersectCirclesContained);

   float a = (radius0*radius0 - radius1*radius1 + d*d)/(2.0f*d);
   float h = sqrtf(radius0*radius0 - a*a);

   float aOverD = a/d;
   float p2x = center0x + aOverD*dx;
   float p2y = center0y + aOverD*dy;

   float hOverD = h/d;
   ix0 = p2x + hOverD * dy;
   iy0 = p2y - hOverD * dx;
   ix1 = p2x - hOverD * dy;
   iy1 = p2y + hOverD * dx;

   // Normal intersection.
   return(cIntersectCirclesNormal);
}


//=============================================================================
// leftTurn
//=============================================================================
bool leftTurn(const BVector &v1, const BVector &v2, const BVector &v3)
{
   // Compute determinant of:
   //  v1.x   v1.z   1
   //  v2.x   v2.z   1
   //  v3.x   v3.z   1
   float det=v1.x*(v2.z-v3.z)-v1.z*(v2.x-v3.x)+(v2.x*v3.z-v2.z*v3.x);
   if(det>0.0f)
      return(true);
   return(false);
}


//=============================================================================
// rightTurn
//=============================================================================
bool rightTurn(const BVector &v1, const BVector &v2, const BVector &v3)
{
   // Compute determinant of:
   //  v1.x   v1.z   1
   //  v2.x   v2.z   1
   //  v3.x   v3.z   1
   float det=v1.x*(v2.z-v3.z)-v1.z*(v2.x-v3.x)+(v2.x*v3.z-v2.z*v3.x);
   if(det<0.0f)
      return(true);
   return(false);
}

//==============================================================================
// quadratic equation solver
// 
// solves: 0 = ax^2 + bx + c
// 
// places solutions in x1 and x2, and returns the number of solutions
//==============================================================================
long quadratic(float a, float b, float c, float& x1, float& x2)
{
   // if a is zero, see if we can solve the linear equation
   if(a == 0.0f)
   {
      if(b == 0.0f)
      {
         return 0;
      }
      else
      {
         x1 = x2 = (-c / b);
         return 1;
      }
   }

   // calculate the discriminant; if negative, bail
   float discriminant = b*b - 4*a*c;
   if(discriminant < 0.0f)
      return 0;

   // calculate the solutions
   x1 = (-b + sqrt(discriminant)) / (2*a);
   x2 = (-b - sqrt(discriminant)) / (2*a);

   // return the number of solutions
   if(discriminant == 0.0f)
      return 1;
   else
      return 2;
}

//=============================================================================
// intersect2Planes(): the 3D intersect of two planes
//=============================================================================
bool intersect2Planes( const BPlane& Pn1, const BPlane& Pn2, BVector& lineP1, BVector& lineP2 )
{
   BVector   u = Pn1.mNormal.cross(Pn2.mNormal);
   float    ax = (u.x >= 0 ? u.x : -u.x);
   float    ay = (u.y >= 0 ? u.y : -u.y);
   float    az = (u.z >= 0 ? u.z : -u.z);

   // Parallel?
   if ((ax+ay+az) < cFloatCompareEpsilon)
         return false; // no intersection if parallel

   // Pn1 and Pn2 intersect in a line!
   // Determine max coordinate of cross product
   long maxc = 0;
   if (ax > ay)
   {
      if (ax > az)
         maxc = 1;
      else
         maxc = 3;
   }
   else
   {
      if (ay > az)
         maxc = 2;
      else
         maxc = 3;
   }

   // next, to get a point on the intersect line
   // zero the max coord, and solve for the other two
   BVector iP;                // intersection

   switch (maxc) {            // select max coordinate
    case 1:                   // intersect with x=0
       iP.x = 0;
       iP.y = (Pn2.mDistance*Pn1.mNormal.z - Pn1.mDistance*Pn2.mNormal.z) / u.x;
       iP.z = (Pn1.mDistance*Pn2.mNormal.y - Pn2.mDistance*Pn1.mNormal.y) / u.x;
       break;
    case 2:                    // intersect with y=0
       iP.x = (Pn1.mDistance*Pn2.mNormal.z - Pn2.mDistance*Pn1.mNormal.z) / u.y;
       iP.y = 0;
       iP.z = (Pn2.mDistance*Pn1.mNormal.x - Pn1.mDistance*Pn2.mNormal.x) / u.y;
       break;
    case 3:                    // intersect with z=0
       iP.x = (Pn2.mDistance*Pn1.mNormal.y - Pn1.mDistance*Pn2.mNormal.y) / u.z;
       iP.y = (Pn1.mDistance*Pn2.mNormal.x - Pn2.mDistance*Pn1.mNormal.x) / u.z;
       iP.z = 0;
   }

   lineP1 = iP;
   u.normalize();
   lineP2 = lineP1 + u;

   return true;
}


//=============================================================================
// spheresIntersect
//=============================================================================
bool spheresIntersect(const BVector &center1, float radius1, const BVector &center2, float radius2)
{
   //-- Get distance between the centers.
   float dx          = center1.x - center2.x;
   float dy          = center1.y - center2.y;
   float dz          = center1.z - center2.z;
   float distanceSqr = (dx * dx) + (dy * dy) + (dz * dz);

   //-- Check the distance.
   float combinedRadius = radius1 + radius2;
   return (distanceSqr <= (combinedRadius * combinedRadius));
}


//=============================================================================
// rayIntersectsSphere
//=============================================================================
bool rayIntersectsSphere(const BVector& origin, const BVector& direction, const BVector &center, float radius)
{
   //-- Normalize the direction.
   BVector n = direction;
   if (!n.safeNormalize())
      return false;

   //-- From Graphics Gems I
   BVector originToCenter;
   originToCenter.assignDifference(center, origin);

   float v    = originToCenter.dot(n);
   float disc = (radius * radius) - ((originToCenter.dot(originToCenter)) - (v * v));
   return (disc >= 0);
}


//=============================================================================
// eof: math.cpp
//=============================================================================
