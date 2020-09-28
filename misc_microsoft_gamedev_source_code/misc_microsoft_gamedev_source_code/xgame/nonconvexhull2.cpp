
//==============================================================================
// nonconvexhull.cpp
//
// Copyright (c) 2004, Ensemble Studios
//==============================================================================

// Includes
#include "common.h"
#include "config.h"
//#include "configenum.h"
#include "convexhull.h"
#include "nonconvexhull2.h"
#include "obstructionmanager.h"
#include "segintersect.h"
#include "pointin.h"
#include "pathhull.h"
#include "game.h"
//#include "terrainbase.h"
#include "unit.h"
#include "world.h"
#include "mathutil.h"
#include "pather.h"
#include "grannymodel.h"
//#include "memorystack.h"
//#include "threading\workDistributor.h"

#ifndef BUILD_FINAL
#include "tracerecording.h"
#endif


#ifdef NCH_DEBUG
#include "nonconvexhull.h"
#endif

#define USE_ALTERNATE_CONVEX_HULL2



#if 0
//==============================================================================
//==============================================================================
class BPolygonMergeWorkEntry
{
   public:
      BDynamicArray<BNCHPolygon>          mPolygons;
      BNCHPolygon*                        mResult;
      BNCHBuilder*                        mBuilder;
};


static BDynamicArray<BPolygonMergeWorkEntry, 32>  sPolygonMergeWorkEntries;
static BCountDownEvent sPolygonMergeRemainingBuckets;



BThread BNonconvexHull::mWorkerThread;
BWin32Semaphore BNonconvexHull::mWorkerSemaphore;
BCountDownEvent BNonconvexHull::mWorkerCountdownEvent;

static BPolygonMergeWorkEntry sWorkEntry;


//==============================================================================
//==============================================================================
void recursiveBuildNCHWorker(BDynamicArray<BNCHPolygon> &polygons, long startIndex, long endIndex, BNCHBuilder *builder, BNCHPolygon &result)
{
   // How many do we need to process?
   long numObs = endIndex - startIndex + 1;
   
   // If more than the cutoff, subdivide.
   if(numObs >= 4)
   {
      // Decide how many go into first batch.
      long numInFirstBatch = numObs/2;

      // First batch.
      BNCHPolygon temp1;
      recursiveBuildNCHWorker(polygons, startIndex, startIndex+numInFirstBatch-1, builder, temp1);
      
      // Second half.
      BNCHPolygon temp2;
      recursiveBuildNCHWorker(polygons, startIndex+numInFirstBatch, startIndex+numObs-1, builder, temp2);
      
      // Merge.
      builder->polygonMerge(temp1, temp2, result);
   }
   else
   {
      // Just process linearly.
      result = polygons[startIndex];

      // Merge each polygon into the result.
      for(long i=startIndex+1; i<=endIndex; i++)
         builder->polygonMerge( result, polygons[i], result );
   }
}


//============================================================================
// Worker thread for assisting in building hulls
//============================================================================
void* _cdecl BNonconvexHull::buildNCHWorker(void* pVal)
{
   // Get the entry we're supposed to be using.
   BPolygonMergeWorkEntry *workEntry = (BPolygonMergeWorkEntry *)pVal;
   
   // Sanity check.
   if(!pVal)
   {
      BFAIL("null entry");
      return(NULL);
   }
   
   // Loop forever (almost)
   for(;;)
   {
      // Wait for work to appear.
      mWorkerSemaphore.wait();
      
// BTimer timer;
// timer.start();   

      // Get builder.
      BNCHBuilder *builder = workEntry->mBuilder;
      
      // If the builder is now NULL, that's our signal to exit.
      if(!builder)
      {
         // Done with our "work".
         return(NULL);
      }
      
      // Otherwise, there's real work to be done...

      // Get reference to result we're supposed to fill out.
      BNCHPolygon &result = *(workEntry->mResult);
      
      // Prime result by copying in the first hull.
      result = workEntry->mPolygons[0];
      
      // Merge each polygon into the result.
      for(uint i=1; i<workEntry->mPolygons.getSize(); i++)
         builder->polygonMerge( result, workEntry->mPolygons[i], result );
      
      // Done with our work.
      mWorkerCountdownEvent.decrement();

// timer.stop();
// double time = timer.getElapsedSeconds();
// LARGE_INTEGER currTime;
// QueryPerformanceCounter(&currTime);
// LARGE_INTEGER freq;
// QueryPerformanceFrequency(&freq);
// freq.QuadPart /= 1000;
// currTime.QuadPart /= freq.QuadPart;
// trace("buildNCHWorker %d entries, %0.4f ms, threadID=%ud  (timestamp=%ld)", workEntry->mPolygons.getNumber(), 1000.0f*time, GetCurrentThreadId(), currTime.QuadPart);
   }
 
   // Never reached.  
}

//============================================================================
//============================================================================
/*static*/ void BNonconvexHull::polygonMergeCallback(void* privateData0, uint64 privateData1, uint workBucketIndex, bool lastWorkEntryInBucket)
{
   SCOPEDSAMPLE(polygonMergeCallback);
   
BTimer timer;
timer.start();   

   // Cast parameter to the proper type.
   BPolygonMergeWorkEntry *workEntry = static_cast<BPolygonMergeWorkEntry*>(privateData0);

   // Get reference to result we're supposed to fill out.
   BNCHPolygon &result = *(workEntry->mResult);
   
   // Get reference to the builder.
   BNCHBuilder &builder = *(workEntry->mBuilder);

   // Prime result by copying in the first hull.
   result = workEntry->mPolygons[0];
   
   // Merge each polygon into the result.
   for(uint i=1; i<workEntry->mPolygons.getSize(); i++)
      builder.polygonMerge( result, workEntry->mPolygons[i], result );

   // Bucket done.
   if(lastWorkEntryInBucket)
      sPolygonMergeRemainingBuckets.decrement();
   else
      BFAIL("wtf?");
   
timer.stop();
double time = timer.getElapsedSeconds();
LARGE_INTEGER currTime;
QueryPerformanceCounter(&currTime);
LARGE_INTEGER freq;
QueryPerformanceFrequency(&freq);
freq.QuadPart /= 1000;
currTime.QuadPart /= freq.QuadPart;
trace("polygonMergeCallback %d entries, %0.4f ms, threadID=%ud  (timestamp=%ld)", workEntry->mPolygons.getNumber(), 1000.0f*time, GetCurrentThreadId(), currTime.QuadPart);
}

#endif



#if 0
float FPFENCETEMP;
static float FPFENCE(float x) 
{ 
   FPFENCETEMP = x;
   DWORD* p = reinterpret_cast<DWORD*>(&FPFENCETEMP);
   return *reinterpret_cast<const float*>(p);
}
#endif

#define DEBUG_GRAPHNONCONVEX
#define DEBUG_TIMING_STATS

const long HULL_LEFT  = 0;
const long HULL_RIGHT = 1;

const long HULL_CLIP = 0;
const long HULL_MAIN = 1;

long BNonconvexHull::moverlapsHull = 0;
long BNonconvexHull::misInside = 0;
long BNonconvexHull::mInsideConvexHull = 0;
long BNonconvexHull::minside = 0;
long BNonconvexHull::msegmentIntersects = 0;
long BNonconvexHull::mfindclosestpointonhull = 0;
long BNonconvexHull::minitialize = 0;

#undef SAME_SIGNS
static inline bool SAME_SIGNS(float a, float b)
{
   if ((a < 0.0f) && (b < 0.0f)) return true;
   if ((a >= 0.0f) && (b >= 0.0f)) return true;
   return false;
}

#undef IDENTICAL_FLOAT
static inline bool IDENTICAL_FLOAT(float a, float b)
{
   return a == b;
}

#ifdef USE_ALTERNATE_CONVEX_HULL2

#include "math/generalVector.h"

// Adapted from:
// two-dimensional convex hull
// the results should be "robust", and not return a wildly wrong hull,
//	despite using floating point
// works in O(n log n); I think a bit faster than Graham scan;
// 	somewhat like Procedure 8.2 in Edelsbrunner's "Algorithms in Combinatorial
//	Geometry", and very close to:
//	    A.M. Andrew, "Another Efficient Algorithm for Convex Hulls in Two Dimensions",
//		Info. Proc. Letters 9, 216-219 (1979)
//

// rg [3/15/08] - I'm disabling FP contraction so the ccw() function will compute the same results in debug and optimized builds.
// It is not necessary for correctness, it's only here to help debug and playtest compute the same results.
#pragma fp_contract (off) 
class BConvexHullComputer
{
public:
   BConvexHullComputer(uint maxPoints) :
      mpPoints(NULL),
      mNumPoints(0),
      mpVerts(maxPoints + 1),
      mNumVerts(0)
      
   {
   }
               
   int calculate(const BVec2* pPoints, uint n)
   {
      mpPoints = pPoints;
      BDEBUG_ASSERT(n < mpVerts.size());
               
      for (uint i = 0; i < n; i++)
         mpVerts[i] = &pPoints[i];
      
      // make lower hull 
      const int u = makeChain(&mpVerts[0], n, cmpl);		
      if (!n) 
      {
         mNumVerts = 0;
         return 0;
      }
         
      mpVerts[n] = mpVerts[0];
      
      // make upper hull
      mNumVerts = u + makeChain(&mpVerts[u], n - u + 1, cmph);	
      return mNumVerts;
   }
   
   uint numVerts(void) const { return mNumVerts; }
   uint vertex(uint i) const { return mpVerts[i] - mpPoints; }
         
private:
   const BVec2* mpPoints;
   uint mNumPoints;
   BDynamicSimArray<const BVec2*> mpVerts;
   uint mNumVerts;
   
   static int ccw(const BVec2** P, int i, int j, int k) 
   {
      // true if points i, j, k counterclockwise 
      return (P[i]->element[0] - P[j]->element[0]) * (P[k]->element[1] - P[j]->element[1]) - 
             (P[i]->element[1] - P[j]->element[1]) * (P[k]->element[0] - P[j]->element[0]) <= 0.0f;	   
   }

   static int cmpm(int c, const BVec2** A, const BVec2** B)
   {
      const float v = (*A)->element[c] - (*B)->element[c];
      if (v > 0.0f) 
         return 1;
      if (v < 0.0f) 
         return -1;
      return 0;
   }

   static int cmpl(const void *a, const void *b) 
   {
      const int compResult = cmpm(0, (const BVec2**)a, (const BVec2**)b);
      if (compResult)
         return compResult;
      return cmpm(1, (const BVec2**)b, (const BVec2**)(a));
   }

   static int cmph(const void *a, const void *b) 
   {
      return cmpl(b, a);
   }

   static int makeChain(const BVec2** V, int n, int (*cmp)(const void*, const void*)) 
   {
      int s = 1;
      
      qsort(V, n, sizeof(const BVec2**), cmp);
      
      for (int i = 2; i < n; i++) 
      {
         int j;
         for (j = s; j >= 1 && ccw(V, i, j, j - 1); j--)
            ;  
         
         s = j + 1;
         
         std::swap(V[s], V[i]);
      }
      
      return s;
   }
}; // class BConvexHullComputer
#pragma fp_contract (on) 

#endif // USE_ALTERNATE_CONVEX_HULL2

static double stableSum4(double* x)
{
#define SWAP(a, b) do { if (x[a] > x[b]) { double c = x[a]; x[a] = x[b]; x[b] = c; } } while(0)   
   SWAP(0, 1);
   SWAP(2, 3);
   SWAP(0, 2);
   SWAP(1, 3);
   SWAP(1, 2);
#undef SWAP   

   const int cNumVals = 4;

   // Sum each set from smallest to largest absolute values.
   double posSum = 0.0f;
   for (int i = 0; i < cNumVals; i++)
      if (x[i] >= 0.0f)
         posSum += x[i];

   double negSum = 0.0f;   
   for (int i = cNumVals - 1; i >= 0; --i)
      if (x[i] < 0.0f)
         negSum += x[i];

   return posSum + negSum;
}

void BNCHVector::set(const float ix, const float iz)
{
   x = ix;
   z = iz;
}

BVector BNCHVector::getBVector() const
{
   return BVector( x, 0.0f, z );
}

bool BNCHVector::compare( const BNCHVector& b, const float epsilon ) const
{
   if (fabs( x - b.x ) <= epsilon && fabs( z - b.z ) <= epsilon)
   {
      return true;
   }
   return false;
}

void BNCHVector::zero()
{
   x = z = 0.0f;
}

// Tests equality w/ epsilon.
static inline bool EQ(float a, float b)
{
   // Difference.
      
   return fabs(a - b) <= cFloatCompareEpsilon;
   
#if 0
   // rg [3/15/08] - This will cause a LHS on 360.
   float temp = a-b;
   
   // Clear sign bit to get absolute value of difference.
   INT_VAL(temp) &= 0x7FFFFFFF;
   
   // Compare with epsilon.
   return(INT_VAL(temp)<=INT_VAL(cFloatCompareEpsilon));
#endif   
}


typedef enum                        // Edge intersection classes         
{
   NUL,                              // Empty non-intersection            
   EMX,                              // External maximum                  
   ELI,                              // External left intermediate        
   TED,                              // Top edge                          
   ERI,                              // External right intermediate       
   RED,                              // Right edge                        
   IMM,                              // Internal maximum and minimum      
   IMN,                              // Internal minimum                  
   EMN,                              // External minimum                  
   EMM,                              // External maximum and minimum      
   LED,                              // Left edge                         
   ILI,                              // Internal left intermediate        
   BED,                              // Bottom edge                       
   IRI,                              // Internal right intermediate       
   IMX,                              // Internal maximum                  
   FUL                               // Full non-intersection             
} vertex_type;

// jce [1/21/2005] -- here's a rough grouping of common and uncommon cases of the enums.
// This can't be reorder to help the switch statements because they are made from bit operations so
// they're actual numeric values are relevant.  Yes, I found this the hard way.
/*
   // very common
   RED,
   LED,

   // average
   EMX,
   ELI,
   ERI,
   IMN,
   EMN,
   ILI,
   IRI,
   IMX,

   // rare
   TED,
   BED,
   FUL,
   NUL,
   IMM,
   EMM,
*/


typedef enum {
   UP,
   DOWN,
   LEFT,
   RIGHT,
   LEFT_OR_RIGHT,
   UP_OR_DOWN,
   I_AM_LOST,
   ON
};

typedef enum                        // Horizontal edge states            
{
   no_h_edge,                       // No horizontal edge                
   bottom_h_edge,                   // Bottom horizontal edge            
   top_h_edge                       // Top horizontal edge               
} h_state;

// Horizontal edge state transitions within scanbeam boundary 
static const h_state next_h_state[3][6] =
{
   //        HULL_ABOVE     HULL_BELOW     CROSS 
   //        L   R     L   R     L   R   
   {bottom_h_edge, top_h_edge,   top_h_edge, bottom_h_edge, no_h_edge,     no_h_edge      },
   {no_h_edge,     no_h_edge,    no_h_edge,  no_h_edge,     top_h_edge,    top_h_edge     },
   {no_h_edge,     no_h_edge,    no_h_edge,  no_h_edge,     bottom_h_edge, bottom_h_edge  }
};

#ifdef DEBUG_GRAPHNONCONVEX
bool screwed = false;
bool news = false;
#endif

/* Faster Line Segment Intersection   */
/* Graphic Gems III Franklin Antonio  */

long NCHsegmentIntersect(const BVector& p11, const BVector& p12, const BVector& p21, const BVector& p22, BVector& intersection, float errorEpsilon)
{
   errorEpsilon;
   float Ax,Bx,Cx,Ay,By,Cy,d,e,f,num,offset;
   float x1lo,x1hi,y1lo,y1hi;

   Ax = p12.x - p11.x;
   Bx = p21.x - p22.x;

   if(Ax<0) {						                           /* X bound box test*/
      x1lo=p12.x; x1hi=p11.x;
   } else {
      x1hi=p12.x; x1lo=p11.x;
   }
   if(Bx>0) {
      if(x1hi < p22.x || p21.x < x1lo) return cNoIntersection;
   } else {
      if(x1hi < p21.x || p22.x < x1lo) return cNoIntersection;
   }

   Ay = p12.z-p11.z;
   By = p21.z-p22.z;

   if(Ay<0) {						                           /* Y bound box test*/
      y1lo=p12.z; y1hi=p11.z;
   } else {
      y1hi=p12.z; y1lo=p11.z;
   }
   if(By>0) {
      if(y1hi < p22.z || p21.z < y1lo) return cNoIntersection;
   } else {
      if(y1hi < p21.z || p22.z < y1lo) return cNoIntersection;
   }


   Cx = p11.x-p21.x;
   Cy = p11.z-p21.z;
   d = By*Cx - Bx*Cy;					                     /* alpha numerator*/
   f = Ay*Bx - Ax*By;					                     /* both denominator*/
   if(f>0) {						                           /* alpha tests*/
      if(d<0 || d>f) return cNoIntersection;
   } else {
      if(d>0 || d<f) return cNoIntersection;
   }

   e = Ax*Cy - Ay*Cx;					                     /* beta numerator*/
   if(f>0) {						                           /* beta tests*/
      if(e<0 || e>f) return cNoIntersection;
   } else {
      if(e>0 || e<f) return cNoIntersection;
   }

   /*compute intersection coordinates*/

   if(f==0) return cCoincident;

   intersection.y = 0.0f;

   num = d*Ax;						                           /* numerator */
   offset = SAME_SIGNS(num,f) ? f/2 : -f/2;		         /* round direction*/
   intersection.x = p11.x + (num+offset) / f;				/* intersection x */

   num = d*Ay;
   offset = SAME_SIGNS(num,f) ? f/2 : -f/2;
   intersection.z = p11.z + (num+offset) / f;				/* intersection y */

   return cIntersection;
}

//==============================================================================
// pointInXZProjection
//==============================================================================
bool pointInXZProjectionNCH(const BNCHVector *vertices, const long numVertices, const BVector &v)
{
   bool result =false;
   for (long i = 0, j = numVertices-1; i < numVertices; j = i++) 
   {
      float xi = vertices[i].x;
      float zi = vertices[i].z;
      float xj = vertices[j].x;
      float zj = vertices[j].z;
      if ((((zi<=v.z) && (v.z<zj)) ||
         ((zj<=v.z) && (v.z<zi))) &&
         (v.x < (xj - xi) * (v.z - zi) / (zj - zi) + xi))
         result = !result;      
   }
   return result;
}

//==============================================================================
// pointInXZProjection
// counter clock wise
//==============================================================================
bool pointInXZProjectionCCNCH(const BNCHVector *vertices, const long numVertices, const BVector &v)
{
   bool result =false;
   for (long j = 0, i = numVertices-1; j < numVertices; i = j++) 
   {
      float xi = vertices[i].x;
      float zi = vertices[i].z;
      float xj = vertices[j].x;
      float zj = vertices[j].z;
      if ((((zi<=v.z) && (v.z<zj)) ||
         ((zj<=v.z) && (v.z<zi))) &&
         (v.x < (xj - xi) * (v.z - zi) / (zj - zi) + xi))
         result = !result;      
   }
   return result;
}

//==============================================================================
// pointInXZProjection
//==============================================================================
bool pointInXZProjectionNCH(const BNCHVector *vertices, const long numVertices, const BNCHVector &v)
{
   bool result =false;
   for (long i = 0, j = numVertices-1; i < numVertices; j = i++) 
   {
      float xi = vertices[i].x;
      float zi = vertices[i].z;
      float xj = vertices[j].x;
      float zj = vertices[j].z;
      if ((((zi<=v.z) && (v.z<zj)) ||
         ((zj<=v.z) && (v.z<zi))) &&
         (v.x < (xj - xi) * (v.z - zi) / (zj - zi) + xi))
         result = !result;      
   }
   return result;
}

//=============================================================================
// comparePoints
//=============================================================================
inline bool comparePointsNCH(const void *v1, const void *v2)
{
   if(((BNCHVector *)v1)->x < ((BNCHVector *)v2)->x)
      return(false);

   if(IDENTICAL_FLOAT(((BNCHVector *)v1)->x, ((BNCHVector *)v2)->x))
   {
      if(((BNCHVector *)v1)->z < ((BNCHVector *)v2)->z)
         return(false);
   }

   return(true);
}


//=============================================================================
// comparePoints2
//=============================================================================
inline bool comparePointsNCH2(const void *v1, const void *v2)
{
   if(((BNCHVector *)v1)->x < ((BNCHVector *)v2)->x)
      return(true);

   if( IDENTICAL_FLOAT(((BNCHVector *)v1)->x, ((BNCHVector *)v2)->x))
   {
      if(((BNCHVector *)v1)->z < ((BNCHVector *)v2)->z)
         return(true);
   }

   return(false);
}

//=============================================================================
//  shellsort
//=============================================================================
/*static*/ void BNonconvexHull::shellsortNCH(BNCHVector *a, long num)
{
   long h = 1;
   // Find the largest h value possible 
   while ((h * 3 + 1) < num) 
   {
      h = 3 * h + 1;
   }

   // while h remains larger than 0 
   while( h > 0 ) 
   {
      // For each set of elements (there are h sets)
      for (int i = h - 1; i < num; i++) 
      {
         // pick the last element in the set
         BNCHVector v = a[i];
         long j = i;

         // compare the element at B to the one before it in the set
         // if they are out of order continue this loop, moving
         // elements "back" to make room for B to be inserted.
         for( j = i; (j >= h) && comparePointsNCH((void*)(a+(j-h)), (void*)&v); j -= h) 
            //for( j = i; (j >= h) && (a[j-h].x>v.x || (a[j-h].x==v.x && a[j-h].z>v.z)); j -= h) 
         {
            a[j] = a[j-h];
         }
         a[j] = v;
      }
      h = h / 3;
   }
}


//=============================================================================
// shellsort
//=============================================================================
/*static*/ void BNonconvexHull::shellsortNCH2(BNCHVector *a, long num)
{ 
   long h = 1;
   // Find the largest h value possible 
   while ((h * 3 + 1) < num) 
   {
      h = 3 * h + 1;
   }

   // while h remains larger than 0 
   while( h > 0 ) 
   {
      // For each set of elements (there are h sets)
      for (int i = h - 1; i < num; i++) 
      {
         // pick the last element in the set
         BNCHVector v = a[i];
         long j = i;

         // compare the element at B to the one before it in the set
         // if they are out of order continue this loop, moving
         // elements "back" to make room for B to be inserted.
         for( j = i; (j >= h) && comparePointsNCH2((void*)(a+(j-h)), (void*)&v); j -= h) 
         {
            a[j] = a[j-h];
         }
         a[j] = v;
      }
      h = h / 3;
   }
}

//=============================================================================
// leftTurn
//=============================================================================
/*static*/ bool BNonconvexHull::leftTurnNCH(const BNCHVector &v1, const BNCHVector &v2, const BNCHVector &v3)
{
   // Compute determinant of:
   //  v1.x   v1.z   1
   //  v2.x   v2.z   1
   //  v3.x   v3.z   1
   return ( (v1.x*(v2.z-v3.z)-v1.z*(v2.x-v3.x)+(v2.x*v3.z-v2.z*v3.x)) > 0.0f);
}

//=============================================================================
// rightTurnNCHStable
//=============================================================================
bool rightTurnNCHStable(const BNCHVector &v1, const BNCHVector &v2, const BNCHVector &v3) 
{
   // Compute determinant of:
   //  v1.x   v1.z   1
   //  v2.x   v2.z   1
   //  v3.x   v3.z   1

   const uint cNumVals = 4;
   double x[cNumVals];
   x[0] = v1.x * (v2.z - v3.z);
   x[1] = -v1.z * (v2.x - v3.x);
   x[2] = v2.x * v3.z;
   x[3] = -v2.z * v3.x;
   
   double sum = stableSum4(x);
   if (sum < -.00000125f)
      return true;

   return false;
}

//=============================================================================
// rightTurn
//=============================================================================
/*static*/ bool BNonconvexHull::rightTurnNCH(const BNCHVector &v1, const BNCHVector &v2, const BNCHVector &v3)
{
   // Compute determinant of:
   //  v1.x   v1.z   1
   //  v2.x   v2.z   1
   //  v3.x   v3.z   1

#if 1
   return rightTurnNCHStable(v1, v2, v3);
#else   
   return ( (v1.x*(v2.z-v3.z)-v1.z*(v2.x-v3.x)+(v2.x*v3.z-v2.z*v3.x)) < 0.0f);
#endif   
   
//   return false;
}

//=============================================================================
// leftScanHullNCH - Scan for counter clockwise convex hull
//=============================================================================
/*static*/ long BNonconvexHull::leftScanHullNCH(BNCHVector *points, long n)
{
   long h =1;
   for(long i =1; i<n; i++)
   {
      // While top two points on stack and points[i] don't make a left turn,
      // pop the top stack element off.
      while(h>=2 && !leftTurnNCH(points[h-2], points[h-1], points[i]))
         h--;

      // Put points[i] on the stack.
      bswap(points[i], points[h]);
      h++;
   }
   return(h);
}

//=============================================================================
// rightScanHullNCH - Scan for clockwise convex hull
//=============================================================================
/*static*/ long BNonconvexHull::rightScanHullNCH(BNCHVector *points, long n)
{
   long h =1;
   for(long i =1; i<n; i++)
   {
      // While top two points on stack and points[i] don't make a right turn,
      // pop the top stack element off.
      while(h>=2 && !rightTurnNCH(points[h-2], points[h-1], points[i]))
         h--;

      // Put points[i] on the stack.
      bswap(points[i], points[h]);
      h++;
   }
   return(h);
}


//=============================================================================
// BNonconvexHull::operator =
//=============================================================================
BNonconvexHull &BNonconvexHull::operator =(const BNonconvexHull &h)
{
   if (mPolygon)
   {
      mPolygon->dealloc();
      if (mbDynAllocPolygon && (mPolygon->mAlloc <= 0))
         delete *&mPolygon; //AMG
   }

   // Copy points.
   mPolygon = h.mPolygon;
   mbDynAllocPolygon = h.mbDynAllocPolygon;
   mPolygon->mAlloc++;
   // Copy Obstructions
   long lObs = h.mObstructionList.getNumber();
   mObstructionList.setNumber(lObs);
   if (lObs > 0)
   {
      memcpy((BOPObstructionNode**)mObstructionList.getPtr(), (const BOPObstructionNode* const*)h.mObstructionList.getPtr(), lObs * sizeof(BOPObstructionNode*));
   }

   mIsConvex = h.mIsConvex;
   mBoundingMin = h.mBoundingMin;
   mBoundingMax = h.mBoundingMax;

#ifdef NCH_DEBUG
   *(debugHull) = *(h.debugHull);
#endif
   return *this;
}


//==============================================================================
// BNonconvexHull::BNonconvexHull
//==============================================================================
BNonconvexHull::BNonconvexHull(void)
#ifdef SPLIT_DATA
: m_pRarelyAccessedData(new BNonconvexHull_RarelyAccessedData()) 
#endif
{
   mPolygon = NULL;
   mbDynAllocPolygon = false;
   reset();
#ifdef NCH_DEBUG
   debugHull = new BNonconvexHullOld;
#endif
} // BNonconvexHull::BNonconvexHull

//==============================================================================
// BNonconvexHull::BNonconvexHull
//==============================================================================
BNonconvexHull::BNonconvexHull(const BNonconvexHull &h)
#ifdef SPLIT_DATA
: m_pRarelyAccessedData(new BNonconvexHull_RarelyAccessedData()) 
#endif
{
   mPolygon = NULL;
   mbDynAllocPolygon = false;
   *this = h;
} // BNonconvexHull::BNonconvexHull

//==============================================================================
// BNonconvexHull::~BNonconvexHull
//==============================================================================
BNonconvexHull::~BNonconvexHull(void)
{
   if (mPolygon)
   {
      mPolygon->dealloc();
      if (mbDynAllocPolygon && (mPolygon->mAlloc <= 0))
         delete *&mPolygon; //AMG
   }
   mPolygon = NULL;
#ifdef NCH_DEBUG
   delete debugHull;
   debugHull = NULL;
#endif
#ifdef SPLIT_DATA
   delete m_pRarelyAccessedData;
#endif
} // BNonconvexHull::~BNonconvexHull


//==============================================================================
// BNonconvexHull::reset
//==============================================================================
void BNonconvexHull::reset()
{
   if (mPolygon)
   {
      mPolygon->dealloc();
      if (mbDynAllocPolygon && (mPolygon->mAlloc <= 0))
         delete *&mPolygon; //AMG
   }
   mBoundingMin.zero();
   mBoundingMax.zero();
   mIsConvex = false;
   mObstructionList.setNumber(0); 
   mPolygon = NULL;
   mbDynAllocPolygon = false;
   mfInitTime = 0.0f;
}


static const long CACHE_NUM = 512;
static int64 cacheCheck[CACHE_NUM];
static float cacheCheckMinX[CACHE_NUM];
static float cacheCheckMaxX[CACHE_NUM];
static float cacheCheckMinZ[CACHE_NUM];
static float cacheCheckMaxZ[CACHE_NUM];
static long  cacheCheckObjs[CACHE_NUM];
static BNCHPolygon cachePolygon[CACHE_NUM];

static long NON_CACHE_NUM = 512;
static BNCHPolygon** nonCachePolygon;

static long cHits = 0;
static long cMisses = 0;
static DWORD cFrame = 0;
static BNCHPolygon cInput;
static long cOnNonCache = 0;
static long cHighWater = 0;
static long cMaxHullsPerNch = 0;
static long cMaxVerticesPerNch = 0;

const DWORD DEADMARK = 0xDEADDEAD;

static BNCHPolygon sTempNCHPolygon;
static const long cNumBuilders = 2;
static BNCHBuilder sNCHBuilders[cNumBuilders];


//=============================================================================
// BNonconvexHull::clearCache
//=============================================================================
void BNonconvexHull::clearCache()
{
   // jce [11/17/2008] -- clear out cFrame also so that it's freshly 0 on each new scenario
   cFrame = 0;
   
   cHits = 0;
   cMisses = 0;
   cMaxHullsPerNch = 0;
   cMaxVerticesPerNch = 0;
   for(long i=0; i<CACHE_NUM; i++)
   {
      cacheCheck[i] = -1;
      cachePolygon[i].mAge = 0;
      
      // jce [11/17/2008] -- in reality, nothing should have a ref count >0 at this point or
      // else someone is holding on to something incorrectly
      BASSERT(cachePolygon[i].mAlloc == 0);
      
      cachePolygon[i].mAlloc = 0;
   }
}   


//=============================================================================
// startup
//=============================================================================
void BNonconvexHull::startup()
{
   cHits = 0;
   cMisses = 0;
   cMaxVerticesPerNch = 0;
   cMaxHullsPerNch = 0;
   for(long i=0; i<CACHE_NUM; i++)
   {
      cacheCheck[i] = -1;
      cachePolygon[i].mAge = 0;
      cachePolygon[i].mAlloc = 0;
      cachePolygon[i].hull.setNumber(4);
      for(long j=0; j<4; j++)
      {
         cachePolygon[i].hull[j].vertex.setNumber(64);
      }
   }
   nonCachePolygon = new BNCHPolygon*[NON_CACHE_NUM];

   for(long i=0; i<NON_CACHE_NUM; i++)
   {
      nonCachePolygon[i] = new BNCHPolygon;
      nonCachePolygon[i]->mAge = DEADMARK;
      nonCachePolygon[i]->mAlloc = 0;
      nonCachePolygon[i]->hull.setNumber(1);
      nonCachePolygon[i]->hull[0].vertex.setNumber(4);
   }
   
   /*
   // Create worker thread, put it on sim helper core.
   mWorkerThread.createThread(buildNCHWorker, &sWorkEntry, 0, false);
   
   // jce [11/11/2008] -- Putting this on thread 4 since sim helper (thread 1) is really a hyper-thread of 0 where the
   // sim is running and won't give us maximum speedup.  Four mostly does things like IO and is mostly not used, except when
   // work distributor is really busy.
   mWorkerThread.setThreadProcessor(4);
   */
}

//=============================================================================
// resetNonCached
//=============================================================================
void BNonconvexHull::resetNonCached()
{
   for(long i=0; i<NON_CACHE_NUM; i++)
   {
      nonCachePolygon[i]->mAge = DEADMARK;
      nonCachePolygon[i]->mAlloc = 0;
   }

   if (cHighWater < cOnNonCache)
      cHighWater = cOnNonCache;

   cOnNonCache = 0;
}

//=============================================================================
// oldestPolygon
//=============================================================================
long BNonconvexHull::oldestPolygon() const
{
   long onCache = -1;
   DWORD oldest = cFrame+1;

   for( long i=0; i<CACHE_NUM; i++)
   {
      if (cachePolygon[i].mAlloc == 0 && cachePolygon[i].mAge < oldest)
      {
         onCache = i;
         oldest = cachePolygon[i].mAge;
      }
   }

   BASSERT( onCache != -1 );

   return onCache;
}

//=============================================================================
// oldestPolygon
//=============================================================================
long BNonconvexHull::getNoncachePolygon() const
{
   long bas = 0;
   long onCache = 0;
   do {
      onCache = cOnNonCache;
      cOnNonCache++;
      if (cOnNonCache == NON_CACHE_NUM)
      {
         cOnNonCache = 0;
         bas++;
         if (bas == 2)
         {
            long oldCN = NON_CACHE_NUM;
            long newCN = NON_CACHE_NUM*2;

            BNCHPolygon **newList = new BNCHPolygon*[newCN];
            for(long i=0; i<oldCN; i++)
            {
               newList[i] = nonCachePolygon[i];
            }

            for(long i=NON_CACHE_NUM; i<newCN; i++)
            {
               newList[i] = new BNCHPolygon;
               newList[i]->mAge = DEADMARK;
               newList[i]->mAlloc = 0;
               newList[i]->hull.setNumber(1);
               newList[i]->hull[0].vertex.setNumber(4);
            }
            NON_CACHE_NUM = newCN;
            cOnNonCache = oldCN;
            delete [] nonCachePolygon;
            nonCachePolygon = newList;
            return oldCN;
         }
         BASSERT( bas < 2 );
      }
   } while (nonCachePolygon[onCache]->mAlloc != 0);

   return onCache;
}

//=============================================================================
// shutdown
//=============================================================================
void BNonconvexHull::shutdown()
{
   resetNonCached();
   for(long i=0; i<NON_CACHE_NUM; i++)
   {
      delete nonCachePolygon[i];
      nonCachePolygon[i] = NULL;
   }
   delete [] nonCachePolygon;
   nonCachePolygon = NULL;

   // jce [10/7/2008] -- adding more stuff to be cleaned up here for inter-scenario cleanliness.  This stuff 
   // will all get realloced the next scenario, however.

   // jce [11/17/2008] -- clear out the cache data too
   clearCache();

   for(long i=0; i<CACHE_NUM; i++)
   {
      cachePolygon[i].hull.clear();
   }

   cInput.hull.clear();
   
   
   /*
   // Kill worker thread.
   // Null builder is going to tell thread to go away.
   sWorkEntry.mBuilder = NULL;
   // Release semaphore to tell it "work" is ready.
   mWorkerSemaphore.release();
   // Now wait for it to die.
   mWorkerThread.waitForThread();
   */
}


//==============================================================================
//==============================================================================
void BNonconvexHull::localBuildNCH(BDynamicSimArray<BOPObstructionNode*> &obs, long startIndex, long endIndex, BNCHPolygon &result)
{
// BTimer timer;
// timer.start();

   // Prime with first obstruction of the batch
   result.hull.setNumber(1);
   result.hull[0].vertex.setNumber(4);
   const BOPQuadHull *pHull = gObsManager.getExpandedHull(obs[startIndex]);
   for (long i =0; i<4; i++)
   {
      result.hull[0].vertex[i].set(pHull->mPoint[i].mX, pHull->mPoint[i].mZ);
   }
   
   for(long i=startIndex+1; i<=endIndex; i++)
   {
      // Get next obs hull.
      pHull = gObsManager.getExpandedHull(obs[i]);
      
      // Convert to nch poly.
      for (long k =0; k<4; k++)
      {
         cInput.hull[0].vertex[k].set(pHull->mPoint[k].mX, pHull->mPoint[k].mZ);
      }
      
      // Merge into what we have so far.
      sNCHBuilders[0].polygonMerge(result, cInput, result);
   }
// timer.stop();
// double time = timer.getElapsedSeconds();
// LARGE_INTEGER currTime;
// QueryPerformanceCounter(&currTime);
// LARGE_INTEGER freq;
// QueryPerformanceFrequency(&freq);
// freq.QuadPart /= 1000;
// currTime.QuadPart /= freq.QuadPart;
// trace("local build %d entries, %0.4f ms, threadID=%ud  (timestamp=%ld)", endIndex-startIndex+1, 1000.0f*time, GetCurrentThreadId(), currTime.QuadPart);
}

//==============================================================================
//==============================================================================
const long cNumRecursiveTemps = 16;
static BNCHPolygon sRecursiveTemps[cNumRecursiveTemps];
static long sRecursiveTempIndex = 0;
void BNonconvexHull::recursiveBuildNCH(BDynamicSimArray<BOPObstructionNode*> &obs, long startIndex, long endIndex, BNCHPolygon &result)
{
   // How many do we need to process?
   long numObs = endIndex - startIndex + 1;
   
   // If more than the cutoff, subdivide.
   if(numObs >= 6)
   {
      // Decide how many go into first batch.
      long numInFirstBatch = numObs/2;
      
      // Only enough temps to recurse 16 deep here, which takes >128k obstructions(!)
      BASSERT(sRecursiveTempIndex <= cNumRecursiveTemps-2);

      // Get temp hull space.
      BNCHPolygon &temp1 = sRecursiveTemps[sRecursiveTempIndex];
      sRecursiveTempIndex++;
      temp1.reset();

      BNCHPolygon &temp2 = sRecursiveTemps[sRecursiveTempIndex];
      sRecursiveTempIndex++;
      temp2.reset();

      // First batch.
      recursiveBuildNCH(obs, startIndex, startIndex+numInFirstBatch-1, temp1);

      // Second half.
      recursiveBuildNCH(obs, startIndex+numInFirstBatch, startIndex+numObs-1, temp2);
      
      // Merge.
      sNCHBuilders[0].polygonMerge(temp1, temp2, result);
      
      // Return temps.
      sRecursiveTempIndex -= 2;
   }
   else
   {
      // Just process linearly.
      localBuildNCH(obs, startIndex, endIndex, result);
   }
}

//==============================================================================
// BNonconvexHull::initialize
// DLM: Note:  Currently there is one way, and one way only to create a valid nonconvex
// DLM: hull, and that is through this function.  
// DLM: We take as parameters a long array of obstruction id's, and an obsruction
// DLM: manager from which to pull them.
// DLM: Takes an arbitray number of overlapping convex hulls, and generates
// DLM: the concaveHull that is the precise union of those hulls.  
// DLM: It's harder than it looks...
// DLM: Either we can create a concaveHull, or we can't.  Return the truth..
//==============================================================================
bool BNonconvexHull::initialize(BDynamicSimArray<BOPObstructionNode*> &obs, BObstructionManager *obManager,
                                BOPObstructionNode *startOb, long lStartSegmentIdx, const BVector *pvStartPoint)
{
   // DLM: Sometimes we have to build a nuclear power plant in order to figure out
   // DLM: how a watermill works.  In any event, this much simpler routine throws 
   // DLM: the callipers and bridge building to hell and just walks the hulls.
   // DLM: Note, it was in the process of debugging the calliper routine that
   // DLM: I figured out you could successfully walk the exterior hulls. dlm.

   // DLM: Takes the array of Obstructions and creates a concave hull from the exterior.
   // DLM: The *assumption* is that all of these obstructions intersect.

   // I'm sure I should do something with these
   lStartSegmentIdx; startOb; pvStartPoint;

   cFrame++;

   // Clear everything out
   reset();

   long lNumObs = obs.getNumber();
   if (lNumObs== 0)
      return false;

   // If we only have a single obstruction, just copy it's points, and be done.
   if (lNumObs== 1)
   {
      // Use *expanded* hulls to generate the path.. this put's us slightly
      // away from the hull's themselves.
      const BOPQuadHull *pHull = obManager->getExpandedHull(obs[0]);
      if(!pHull)
      {
         BFAIL("null hull in obstruction manager.");
         return(false);
      }

      long onCache = getNoncachePolygon();
      mPolygon = nonCachePolygon[onCache];
      mPolygon->mAlloc++;

      mPolygon->hull.setNumber(1);
      mPolygon->hull[0].vertex.setNumber(4);
      mPolygon->hull[0].hole = false;

      for (long i =0; i<4; i++)
      {
         mPolygon->hull[0].vertex[i].set(pHull->mPoint[i].mX, pHull->mPoint[i].mZ);
      }

      mObstructionList.add(obs[0]);

      mIsConvex = true;
      computeBoundingBox();

#ifdef NCH_DEBUG
      debugHull->initialize( obs, obManager, startOb, lStartSegmentIdx, pvStartPoint );
#endif
      return true;
   }

#ifdef DEBUG_TIMING_STATS
   LARGE_INTEGER QPFreq;
   LARGE_INTEGER startTime;
   LARGE_INTEGER endTime;
   QueryPerformanceFrequency(&QPFreq);
   int64 delta;
   QueryPerformanceCounter(&startTime);
#endif

   int64 ca = lNumObs;
   
   float minx = FLT_MAX;
   float minz = FLT_MAX;
   float maxx = -FLT_MAX;
   float maxz = -FLT_MAX;

   for( long i=0; i<lNumObs; i++)
   {
      const BOPQuadHull *pHull = obManager->getExpandedHull(obs[i]);
      BASSERTM(pHull, "Somehow a null pointer got into the obstruction manager.");     // jce [6/7/2005] -- this shouldn't happen, so for speed reasons not doing the if-check 
      ca += INT_VAL( pHull->mRadius );
      ca += pHull->mRotation;
      for (long k =0; k<4; k++)
      {
         ca += INT_VAL( pHull->mPoint[k].mX ) ^ INT_VAL( pHull->mPoint[k].mZ );
         if (maxx < pHull->mPoint[k].mX)
            maxx = pHull->mPoint[k].mX;
         if (minx > pHull->mPoint[k].mX)
            minx = pHull->mPoint[k].mX;
         if (maxz < pHull->mPoint[k].mZ)
            maxz = pHull->mPoint[k].mZ;
         if (minz > pHull->mPoint[k].mZ)
            minz = pHull->mPoint[k].mZ;
      }
      mObstructionList.add(obs[i]);
      ca = ca << 1;
   }

   minitialize++;

   bool found = false;
   // DLM 11/5/08 - Commment this out if you don't want to use the cache..   
   for( long i=0; i<CACHE_NUM; i++)
   {
      // check to see if we've had this before
      if (cacheCheck[i] == ca && cacheCheckObjs[i] == lNumObs && cacheCheckMinX[i] == minx && cacheCheckMaxX[i] == maxx && cacheCheckMinZ[i] == minz && cacheCheckMaxZ[i] == maxz)
      {
         mPolygon = &cachePolygon[i];
         mPolygon->mAlloc++;
         mPolygon->mAge = cFrame;
         found = true;
         cHits++;
         break;
      }
   }   
   if (!found)
   {
      long onCache = oldestPolygon();
      
      // jce [11/17/2008] -- If onCache is -1, it's actually pretty catastrophic.  It will have asserted already, but
      // adding a clause to bail out here and not overwrite memory at least.  In reality, this will probably mean pathing
      // will cease to get any results forever more.
      if(onCache < 0 || onCache >= CACHE_NUM)
         return(false);

      cMisses++;
      cInput.hull.setNumber(1);
      cInput.hull[0].vertex.setNumber(4);
      cInput.hull[0].hole = false;

      mPolygon = &cachePolygon[onCache];

      mPolygon->mAlloc++;
      mPolygon->mAge = cFrame;

      mPolygon->hull.setNumber(1);
      mPolygon->hull[0].vertex.setNumber(4);
      mPolygon->hull[0].hole = false;

      cacheCheck[onCache] = ca;
      cacheCheckObjs[onCache] = lNumObs;
      cacheCheckMinX[onCache] = minx;
      cacheCheckMaxX[onCache] = maxx;
      cacheCheckMinZ[onCache] = minz;
      cacheCheckMaxZ[onCache] = maxz;
      
      // jce [11/11/2008] -- We're going to pick among three versions of doing this based on the number of obstructions
      // we need to process.
      
      if(lNumObs < 6)
      {
         // jce [11/11/2008] -- This is the original method.  It just merges every input hull one by one into the result.
         localBuildNCH(obs, 0, lNumObs-1, cachePolygon[onCache]);
      }
      //else if(lNumObs < 50)
      else
      {
         // jce [11/11/2008] -- This version divides & conquers using a single thread.
         /*
         // Decide how many go into first batch.
         long numInFirstBatch = lNumObs/2;

         // Build first half into a temp hull.
         sTempNCHPolygon.reset();
         localBuildNCH(obs, 0, numInFirstBatch-1, sTempNCHPolygon);
         
         // Build second half.
         localBuildNCH(obs, numInFirstBatch, lNumObs-1, cachePolygon[onCache]);
         
         // Merge the halves.
         sNCHBuilders[0].polygonMerge(cachePolygon[onCache], sTempNCHPolygon, cachePolygon[onCache]);
         */

         // jce [11/13/2008] -- Newever version recursively subdivides.
         recursiveBuildNCH(obs, 0, lNumObs-1, cachePolygon[onCache]);
      }
      // jce [11/13/2008] -- Taking this out because the single-threaded recursive version is beating it right now.
      // I had a multi-thread + recursive version and that wasn't helping either for some reason.  Maybe on a future project
      // we'll revisit this :)
      /*
      else
      {
         // jce [11/11/2008] -- This version divides & conquers using two threads.

         // Decide how many go into first batch.
         //long numInFirstBatch = lNumObs/2;

         // Set up for worker thread.
         sTempNCHPolygon.reset();
         sWorkEntry.mPolygons.setNumber(numInFirstBatch);
         sWorkEntry.mResult = &sTempNCHPolygon;
         sWorkEntry.mBuilder = &sNCHBuilders[1];
         
         // Fill in polygons.
         for(long i=0; i<numInFirstBatch; i++)
         {
            BNCHPolygon &poly = sWorkEntry.mPolygons[i];
            poly.hull.setNumber(1);
            BNCHVertexList &polyHull = poly.hull[0];
            polyHull.vertex.setNumber(4);
            polyHull.hole = false;
            
            for (long k =0; k<4; k++)
            {
               const BOPQuadHull *pHull = obManager->getExpandedHull(obs[i]);
               polyHull.vertex[k].set(pHull->mPoint[k].mX, pHull->mPoint[k].mZ);
            }
         }

         // One work item for helper thread to process.
         mWorkerCountdownEvent.set(1);

         // Signal the worker thread that it has stuff to do.
         mWorkerSemaphore.release();

         // Build second half on this thread.
         BNCHPolygon test1;
         localBuildNCH(obs, numInFirstBatch, lNumObs-1, cachePolygon[onCache]);
         
         // Now wait for the other thread to be done.
         mWorkerCountdownEvent.getEvent().wait();

         // Merge the halves.
         sNCHBuilders[0].polygonMerge(cachePolygon[onCache], sTempNCHPolygon, cachePolygon[onCache]);
      }
      */
   }


/*
#ifdef DEBUG_GRAPHNONCONVEX
   if (screwed && !news)
   {
      news = true;
      addDebugLines(BDebugPrimitives::cCategoryPatherNonConvex, 0xffff0000);
   }
   else if (!screwed && !news)
   {
      addDebugLines(BDebugPrimitives::cCategoryPatherNonConvex, 0xffff0000);
   }
#endif
*/

   mBoundingMin.x = minx;
   mBoundingMin.z = minz;

   mBoundingMax.x = maxx;
   mBoundingMax.z = maxz;

#ifdef DEBUG_TIMING_STATS
   QueryPerformanceCounter(&endTime);
   delta = (long)(endTime.QuadPart - startTime.QuadPart);
   mfInitTime = 1000.0f*(float)delta/(float)(QPFreq.QuadPart);
#endif

#ifdef NCH_DEBUG
   debugHull->initialize( obs, obManager, startOb, lStartSegmentIdx, pvStartPoint );
#endif
   return true;
}

/*//==============================================================================
// BNonconvexHull::initialize
//==============================================================================
bool BNonconvexHull::initialize(BGrannyModel *model)
{
   if (model == NULL)
      return false;

   reset();

   // Set up the hull
   mPolygon = new BNCHPolygon;
   mbDynAllocPolygon = true;
   mPolygon->mAlloc++;
   mPolygon->mAge = cFrame;

   // Add all meshes to the hull
   long meshIndx;
   long numMeshes = model->getMeshCount();
   for (meshIndx = 0; meshIndx < numMeshes; meshIndx++)
   {
      granny_mesh *mesh = model->getMesh(meshIndx);

      // Make sure mesh not in optimized format because won't be able to copy verts this way otherwise
      if (model->isOptimizedVertexFormatMesh(meshIndx))
      {
         setBlogError(0);
         blogerrortrace("BNonconvexHull::initialize(BGrannyModel*) : Mesh %d of file \"%s\" is a non-rigid mesh and isn't supported.", meshIndx, BStrConv::toA(model->getFilename()));
         continue;
      }

      long numTriGroups = GrannyGetMeshTriangleGroupCount(mesh);
      granny_tri_material_group *triGroup = GrannyGetMeshTriangleGroups(mesh);
      long bytesPerIndex = GrannyGetMeshBytesPerIndex(mesh);

      BYTE *vertices = (BYTE*) GrannyGetMeshVertices(mesh);
      long vertexSize = GrannyGetTotalObjectSize(GrannyGetMeshVertexType(mesh));

      // Add all tri groups within the mesh
      long triGroupIndx;
      for (triGroupIndx = 0; triGroupIndx < numTriGroups; triGroupIndx++)
      {
         BYTE *indices = (BYTE*) GrannyGetMeshIndices(mesh) + (triGroup[triGroupIndx].TriFirst * 3 * bytesPerIndex);
         addTris(triGroup[triGroupIndx].TriCount, indices, bytesPerIndex, vertices, vertexSize);
      }
   }

   computeBoundingBox();

   return true;
}*/

//==============================================================================
// BNonconvexHull::initializeConvex
//==============================================================================
bool BNonconvexHull::initializeConvex(const BNCHVector *points, const long num)
{
   reset();
   
   // Sanity.
   if(num<=0)
   {
      BFAIL("Bad point count.");
      return(false);
   }

   long onCache = getNoncachePolygon();
   mPolygon = nonCachePolygon[onCache];
   mPolygon->mAlloc++;

   mPolygon->hull.setNumber(1);
   mPolygon->hull[0].vertex.setNumber(num);
   mPolygon->hull[0].hole = false;
   // set the normal space coordinates
   for (long l = 0; l < num; l++)
   {
      mPolygon->hull[0].vertex[l].set( points[l].x, points[l].z );
   }

   mIsConvex = true;

   computeBoundingBox();

#ifdef NCH_DEBUG
   debugHull->initializeConvex( points, num );
#endif
   return true;
}

//==============================================================================
// BNonconvexHull::initializeConvex
//==============================================================================
bool BNonconvexHull::initializeConvex(const BVector *points, const long num)
{
   reset();

   // Sanity.
   if(num<=0)
   {
      BFAIL("Bad point count.");
      return(false);
   }

   long onCache = getNoncachePolygon();
   mPolygon = nonCachePolygon[onCache];
   mPolygon->mAlloc++;

   mPolygon->hull.setNumber(1);
   mPolygon->hull[0].vertex.setNumber(num);
   mPolygon->hull[0].hole = false;
   // set the normal space coordinates
   for (long l = 0; l < num; l++)
   {
      mPolygon->hull[0].vertex[l].set( points[l].x, points[l].z );
   }

   mIsConvex = true;

   computeBoundingBox();

#ifdef NCH_DEBUG
   debugHull->initializeConvex( points, num );
#endif
   return true;
}

//==============================================================================
// BNonconvexHull::initializeNonConvex
//==============================================================================
bool BNonconvexHull::initializeNonConvex(const BVector *points, const long num)
{
   //initializeConvex( points, num );
   //mIsConvex = false;

   reset();

   // Sanity.
   if(num<=0)
   {
      BFAIL("Bad point count.");
      return(false);
   }

   mPolygon = new BNCHPolygon;
   mbDynAllocPolygon = true;
   mPolygon->mAlloc++;
   mPolygon->mAge = cFrame;

   mPolygon->hull.setNumber(1);
   mPolygon->hull[0].vertex.setNumber(num);
   mPolygon->hull[0].hole = false;

   // set the normal space coordinates
   for (long l = 0; l < num; l++)
   {
      mPolygon->hull[0].vertex[l].set( points[l].x, points[l].z );
   }

   mIsConvex = false;

   computeBoundingBox();
/*
#ifdef DEBUG_GRAPHNONCONVEX
   addDebugLines(BDebugPrimitives::cCategoryPatherNonConvex, 0xffff0000);
#endif
*/
   return true;
}

//==============================================================================
// BNonconvexHull::initializeNonConvex
//==============================================================================
bool BNonconvexHull::initializeNonConvex(const BNCHVector *points, const long num)
{
   //initializeConvex( points, num );
   //mIsConvex = false;

   reset();

   // Sanity.
   if(num<=0)
   {
      BFAIL("Bad point count.");
      return(false);
   }

   mPolygon = new BNCHPolygon;
   mbDynAllocPolygon = true;
   mPolygon->mAlloc++;
   mPolygon->mAge = cFrame;

   mPolygon->hull.setNumber(1);
   mPolygon->hull[0].vertex.setNumber(num);
   mPolygon->hull[0].hole = false;

   // set the normal space coordinates
   for (long l = 0; l < num; l++)
   {
      mPolygon->hull[0].vertex[l].set( points[l].x, points[l].z );
   }

   mIsConvex = false;

   computeBoundingBox();
   /*
   #ifdef DEBUG_GRAPHNONCONVEX
   addDebugLines(BDebugPrimitives::cCategoryPatherNonConvex, 0xffff0000);
   #endif
   */
   return true;
}

//==============================================================================
// BNonconvexHull::initializeFromSubHull
//==============================================================================
bool BNonconvexHull::initializeFromSubhull(const BNonconvexHull &masterHull, const long nHullNum)
{
   reset();

   const BDynamicArray<BNCHVector> &pointArray = masterHull.getPoints(nHullNum);
   long nPointCount = masterHull.getPointCount(nHullNum);
   return initializeNonConvex((BNCHVector *)&pointArray, nPointCount);
}

//==============================================================================
// BNonconvexHull::initializeQuadHull
//==============================================================================
bool BNonconvexHull::initializeQuadHull(const BOPQuadHull *quadhull)
{
   reset();

   long onCache = getNoncachePolygon();
   mPolygon = nonCachePolygon[onCache];
   mPolygon->mAlloc++;

   long num = 4;

   mPolygon->hull.setNumber(1);
   mPolygon->hull[0].vertex.setNumber(num);
   mPolygon->hull[0].hole = false;
   // set the normal space coordinates
   for (long l = 0; l < num; l++)
   {
      mPolygon->hull[0].vertex[l].set(quadhull->mPoint[l].mX, quadhull->mPoint[l].mZ);
   }

   mIsConvex = true;

   computeBoundingBox();

#ifdef NCH_DEBUG
   debugHull->initializeQuadHull( quadhull );
#endif
   return true;
}

//==============================================================================
// BNonconvexHull::addPointsConvex
//==============================================================================
bool BNonconvexHull::addPointsConvex(const BNCHVector *points, const long num)
{
   // Check params for validity.
   if(!points || num<=0)
   {
      BASSERT(0);
      return(false);
   }
   
#ifdef USE_ALTERNATE_CONVEX_HULL2
   {
      BDynamicArray<BNCHVector>& resultVerts = mPolygon->hull[0].vertex;
      
      BDynamicArray<BNCHVector> tempVerts;
      tempVerts.reserve(resultVerts.getSize() + num);
      
      tempVerts += resultVerts;
      tempVerts.pushBack(points, num);
                  
      BConvexHullComputer convexHull(tempVerts.getSize());
      uint numHullVerts = convexHull.calculate(reinterpret_cast<BVec2*>(tempVerts.getPtr()), tempVerts.getSize());
      
      resultVerts.resize(numHullVerts);
      
      for (uint i = 0; i < numHullVerts; i++)
         resultVerts[i] = tempVerts[convexHull.vertex(numHullVerts - 1 - i)];
   }   
#else
   {
      // Add new points to the list.
      long origNum =mPolygon->hull[0].vertex.getNumber();
      mPolygon->hull[0].vertex.setNumber(origNum+num);
      for( long l =0; l<num; l++){
         mPolygon->hull[0].vertex[l+origNum].set( points[l].x, points[l].z );
      }

      // Sort.
      shellsortNCH(mPolygon->hull[0].vertex.getPtr(), mPolygon->hull[0].vertex.getNumber());

      // Build upper hull.
      long f = rightScanHullNCH(mPolygon->hull[0].vertex.getPtr(), mPolygon->hull[0].vertex.getNumber());

      for(long i =0; i<f-1; i++)
         bswap(mPolygon->hull[0].vertex[i], mPolygon->hull[0].vertex[i+1]);

      // Sort for lower hull.
      shellsortNCH2(mPolygon->hull[0].vertex.getPtr()+(f-2), mPolygon->hull[0].vertex.getNumber()-f+2);

      // Build lower hull.
      long g = rightScanHullNCH(&mPolygon->hull[0].vertex[f-2], mPolygon->hull[0].vertex.getNumber()-f+2);

      long lnum = f+g-2;
      // Update point count.
      mPolygon->hull[0].vertex.setNumber(lnum);
   } 
#endif        

   computeBoundingBox();

#ifdef NCH_DEBUG
   debugHull->addPointsConvex( points, num );
#endif
   return true;
}

//==============================================================================
// BNonconvexHull::copyTriIntoPolygon - Helper function for addTris.  Must be
// above addTris so it can be inlined.
//==============================================================================
/*static*/ inline void BNonconvexHull::copyTriIntoPolygon(BNCHPolygon &poly, BYTE *indexArray, long indexSize, BYTE *vertexArray, long vertexSize)
{
   D3DXVECTOR3 *vert;
   long vertIndx;
   BASSERT(vertexSize >= 0);

   // Access the index array based on the size of the indices
   if (indexSize == 4)
   {
      // 4 byte indices
      unsigned long *indices = (unsigned long*) indexArray;
      for (vertIndx = 0; vertIndx < 3; vertIndx++)
      {
         vert = (D3DXVECTOR3*) (vertexArray + vertexSize * indices[vertIndx]);
         poly.hull[0].vertex[vertIndx].set(vert->x, vert->z);
      }
   }
   else
   {
      // 2 byte indices
      unsigned short *indices = (unsigned short*) indexArray;
      for (vertIndx = 0; vertIndx < 3; vertIndx++)
      {
         vert = (D3DXVECTOR3*) (vertexArray + vertexSize * indices[vertIndx]);
         poly.hull[0].vertex[vertIndx].set(vert->x, vert->z);
      }
   }
}

//==============================================================================
// BNonconvexHull::addTris
//==============================================================================
bool BNonconvexHull::addTris(long numTris, BYTE *indexArray, long indexSize, BYTE *vertexArray, long vertexSize)
{
   long triIndx = 0;

   if ((numTris <= 0) || (mPolygon == NULL))
      return false;

   // If this is the first try to add, just set it.  Otherwise it should be merged.
   if ((getHullCount() == 0) || (getPointCount(0) == 0))
   {
      // Create polygon and add first tri
      mPolygon->hull.setNumber(1);
      mPolygon->hull[0].vertex.setNumber(3);
      mPolygon->hull[0].hole = false;
      BNonconvexHull::copyTriIntoPolygon(*mPolygon, indexArray, indexSize, vertexArray, vertexSize);

      // Move to next tri in list
      indexArray += (indexSize * 3);
      triIndx++;
   }

   // Initialize input polygon
   cInput.hull.setNumber(1);
   cInput.hull[0].vertex.setNumber(3);
   cInput.hull[0].hole = false;

   // Merge with remaining tris.  Tri index initialized above.
   for ( ; triIndx < numTris; triIndx++)
   {
      // Set tri for input polygon
      BNonconvexHull::copyTriIntoPolygon(cInput, indexArray, indexSize, vertexArray, vertexSize);

      // Merge polygon
      sNCHBuilders[0].polygonMerge(*mPolygon, cInput, *mPolygon);

      // Move to next tri in list
      indexArray += (indexSize * 3);
   }

   return true;
}

//==============================================================================
// BNonconvexHull::getPointCount
//==============================================================================
const long BNonconvexHull::getPointCount(long h) const
{
   return mPolygon->hull[h].vertex.getNumber();
}

//==============================================================================
// BNonconvexHull::getHullCount
//==============================================================================
const long BNonconvexHull::getHullCount() const
{
   return mPolygon->hull.getNumber();
}

//==============================================================================
// BNonconvexHull::getPoint
//==============================================================================
const BVector& BNonconvexHull::getPoint(long index, long h) const
{
   static BVector temp;
   BASSERT( h>=0 && h<mPolygon->hull.getNumber());
   BASSERT(index>=0 && index<mPolygon->hull[h].vertex.getNumber()); 
   temp.set(mPolygon->hull[h].vertex[index].x, 0.0f, mPolygon->hull[h].vertex[index].z);
   return temp;
}

//==============================================================================
// BNonconvexHull::getPoints
//==============================================================================
const BDynamicArray<BNCHVector>& BNonconvexHull::getPoints(long h) const
{
   BASSERT( h>=0 && h<mPolygon->hull.getNumber());
   return mPolygon->hull[h].vertex;
}

//==============================================================================
// isVertex
//==============================================================================
const bool BNonconvexHull::isVertex(const BVector& point, long *vertexIndex, long *hullIndex) const
{
   BVector vTest;

   for( long hull = 0; hull<getHullCount(); hull++)
   {
      long numPoints = getPointCount(hull);
      const BNCHVector *points = getPoints(hull).getPtr();
      for( long vertex=0; vertex < numPoints; vertex++ )
      {
         if (_fabs(points[vertex].x - point.x) < cFloatCompareEpsilon && _fabs(points[vertex].z - point.z) < cFloatCompareEpsilon)
         {
            if (vertexIndex)
               *vertexIndex = vertex;
            if (hullIndex)
               *hullIndex = hull;
            return true;
         }
      }
   }
   return false;
}


//==============================================================================
// BNonconvexHull::getHole
//==============================================================================
const bool BNonconvexHull::getHole(const long h) const
{
   BASSERT( h>=0 && h<mPolygon->hull.getNumber());
   return mPolygon->hull[h].hole;
}

//==============================================================================
// BNonconvexHull::getInitTime
//==============================================================================
float BNonconvexHull::getInitTime()
{
   return mfInitTime;
}

//==============================================================================
// BNonconvexHull::addDebugLines
//==============================================================================
void BNonconvexHull::addDebugLines(int category, DWORD color, float *height) const
{
   #ifdef DEBUG_GRAPHNONCONVEX

      gpDebugPrimitives->clear(category);
      for( long hull = 0; hull<mPolygon->hull.getNumber(); hull++)
      {
         long numPoints = mPolygon->hull[hull].vertex.getNumber();
         long startPoint = numPoints - 1;
         for( long endPoint =0; endPoint<numPoints; endPoint++ )
         {
            if (height == NULL)
            {
               gTerrainSimRep.addDebugLineOverTerrain(mPolygon->hull[hull].vertex[startPoint].getBVector(), mPolygon->hull[hull].vertex[endPoint].getBVector(), (DWORD)0xff00ff00, color, 0.5f, category);
            }
            else
            {
               BVector p1(mPolygon->hull[hull].vertex[startPoint].getBVector());
               BVector p2(mPolygon->hull[hull].vertex[endPoint].getBVector());
               p1.y = *height;
               p2.y = *height;
               gpDebugPrimitives->addDebugLine(p1, p2, (DWORD)0xff00ff00, color, category);
            }
            startPoint =endPoint;
         }
      }

   #else
      category;
      color;
      height;
   #endif
}

#ifdef DEBUG_GRAPHNONCONVEX
//==============================================================================
// BNonconvexHull::debugAddHull
//==============================================================================
void BNonconvexHull::debugAddHull(int category, const BOPQuadHull *pHull, DWORD color)
{
   static float fDepth = 0.2f;
   fDepth += 0.2f;
   if (fDepth > 1.4f)
      fDepth = 0.2f;
   
   long lIdx1 = 3;
   
   BVector vP1(cOriginVector);
   BVector vP2(cOriginVector);
   
   for (long lIdx2 = 0; lIdx2 < 4; lIdx2++)
   {
      vP1.x = pHull->mPoint[lIdx1].mX;
      vP1.y = 0.0f;
      vP1.z = pHull->mPoint[lIdx1].mZ;
      vP2.x = pHull->mPoint[lIdx2].mX;
      vP2.y = 0.0f;
      vP2.z = pHull->mPoint[lIdx2].mZ;
      gTerrainSimRep.addDebugLineOverTerrain(vP1, vP2, color, color, fDepth, category);
      lIdx1 = lIdx2;
   }
}
#endif

//==============================================================================
// BNonconvexHull::computePerpToSegment
//==============================================================================
void BNonconvexHull::computePerpToSegment(long segIndex, float &x, float &z, long h)
{
   long numPoints = mPolygon->hull[h].vertex.getNumber();
   // Check for valid segindex.
   if(segIndex<0 || segIndex>=numPoints)
   {
      BASSERT(0);
      x =0.0f;
      z =0.0f;
      return;
   }

   // Get next index.
   long nxtIndex =segIndex+1;
   if(nxtIndex>=numPoints)
      nxtIndex =0;

   // Get perp.
   x =mPolygon->hull[h].vertex[segIndex].z-mPolygon->hull[h].vertex[nxtIndex].z;
   z =mPolygon->hull[h].vertex[nxtIndex].x-mPolygon->hull[h].vertex[segIndex].x;

   // Normalize.
   float recipLen =1.0f/float(sqrt(x*x+z*z));
   x*=recipLen;
   z*=recipLen;
}


//==============================================================================
// BNonconvexHull::computePerpAtPoint
//==============================================================================
void BNonconvexHull::computePerpAtPoint(long segIndex, float &x, float &z, long h)
{
   long numPoints = mPolygon->hull[h].vertex.getNumber();
   // Check for valid segindex.
   if(segIndex<0 || segIndex>=numPoints)
   {
      BASSERT(0);
      x =0.0f;
      z =0.0f;
      return;
   }

   // Get previous index.
   long prevIndex = segIndex - 1;
   if (prevIndex < 0)
      prevIndex = numPoints - 1;

   // Get next index.
   long nxtIndex =segIndex+1;
   if(nxtIndex>=numPoints)
      nxtIndex =0;

   // Get perp.
   x = mPolygon->hull[h].vertex[segIndex].z-mPolygon->hull[h].vertex[nxtIndex].z;
   z = mPolygon->hull[h].vertex[nxtIndex].x-mPolygon->hull[h].vertex[segIndex].x;

   // Normalize.
   float recipLen =1.0f/float(sqrt(x*x+z*z));
   x*=recipLen;
   z*=recipLen;

   // Get perp.
   float x2 = mPolygon->hull[h].vertex[prevIndex].z-mPolygon->hull[h].vertex[segIndex].z;
   float z2 = mPolygon->hull[h].vertex[segIndex].x-mPolygon->hull[h].vertex[prevIndex].x;

   // Normalize.
   recipLen =1.0f/float(sqrt(x2*x2+z2*z2));
   x2*=recipLen;
   z2*=recipLen;

   // Average and normalize the normals
   x = (x + x2) / 2;
   z = (z + z2) / 2;
   recipLen = 1.0f/float(sqrt(x*x+z*z));
   x*=recipLen;
   z*=recipLen;
}


//==============================================================================
// BNonconvexHull::findClosestPointOnHull
// DLM: Finds the closest point on the nonconvex hull to the point in question.
// DLM: the vector returned is that point.  The distancSqr value is set with the
// DLM: distance squared between that point and the referenced point.
//==============================================================================
BVector BNonconvexHull::findClosestPointOnHull(const BVector& vStart, long *plSegmentIndex, long *plHullIndex , float *pfClosestDistSqr)
{
   mfindclosestpointonhull++;
   // Find closest point on the concave hull to this location.
   float fBestDistSqr = cMaximumFloat;
   BVector vClosest(0.0f, 0.0f, 0.0f);

   long lBestIdx = -1L;
   long lBestHull = -1L;

   BVector vTest;

   for( long hull = 0; hull<getHullCount(); hull++)
   {
      if (!getHole(hull))
      {
         long numPoints = getPointCount(hull);
         const BNCHVector *points = getPoints(hull).getPtr();
         long startPoint = numPoints - 1;
         for( long endPoint =0; endPoint<numPoints; startPoint = endPoint, endPoint++ )
         {
            BVector vTest(0.0f, 0.0f, 0.0f);
            float fDistSqr = vStart.xzDistanceToLineSegmentSqr(points[startPoint].getBVector(), points[endPoint].getBVector(), &vTest);
            if (fDistSqr <= fBestDistSqr)
            {
               fBestDistSqr = fDistSqr;
               vClosest = vTest;
               lBestIdx = startPoint;
               lBestHull = hull;
            }
         }
      }
   }

   if (plSegmentIndex)
      *plSegmentIndex = lBestIdx;

   if (pfClosestDistSqr)
      *pfClosestDistSqr = fBestDistSqr;

   if (plHullIndex)
      *plHullIndex = lBestHull;

   vClosest.y = 0.0f;

#ifdef NCH_DEBUG
   BVector vTemp;
   long vf1, vf2;
   float vf3;
   vTemp = debugHull->findClosestPointOnHull( vStart, &vf1, &vf2, &vf3 );
   BVector foo;
   foo = vTemp - vClosest;
   if (foo.length()>=0.1f)
   {
      screwed = true;
      news = true;
      game->getWorld()->getPather()->debugClearRenderLines("nonconvex");
      game->getWorld()->getPather()->debugClearRenderLines("foobar2");
      addDebugLines("foobar2", cDWORDBlue );
      debugHull->addDebugLines("foobar2", cDWORDYellow );
      game->getWorld()->getPather()->debugAddPoint("foobar2", vStart, cDWORDBlue);
      game->getWorld()->getPather()->debugAddPoint("foobar2", vClosest, cDWORDRed);
      game->getWorld()->getPather()->debugAddPoint("foobar2", vTemp, cDWORDYellow);
   }
   BASSERT( foo.length()<0.1f );
#endif
   return vClosest;
}

//==============================================================================
// BNonconvexHull::findClosestPointOnHull
// DLM: Finds the closest point on the nonconvex hull to the point in question.
// DLM: the vector returned is that point.  The distancSqr value is set with the
// DLM: distance squared between that point and the referenced point.
//==============================================================================
BVector BNonconvexHull::findClosestPointOnHull(const long hull, const BVector& vStart, long *plSegmentIndex , float *pfClosestDistSqr)
{
   mfindclosestpointonhull++;
   // Find closest point on the concave hull to this location.
   float fBestDistSqr = cMaximumFloat;
   BVector vClosest(0.0f, 0.0f, 0.0f);

   long lBestIdx = -1L;
   long lBestHull = -1L;

   BVector vTest;

   long numPoints = getPointCount(hull);
   const BNCHVector *points = getPoints(hull).getPtr();
   long startPoint = numPoints - 1;
   for( long endPoint =0; endPoint<numPoints; endPoint++ )
   {
      BVector vTest(0.0f, 0.0f, 0.0f);
      float fDistSqr = vStart.xzDistanceToLineSegmentSqr(points[startPoint].getBVector(), points[endPoint].getBVector(), &vTest);
      if (fDistSqr <= fBestDistSqr)
      {
         fBestDistSqr = fDistSqr;
         vClosest = vTest;
         lBestIdx = startPoint;
         lBestHull = hull;
      }
      startPoint =endPoint;
   }

   if (plSegmentIndex)
      *plSegmentIndex = lBestIdx;

   if (pfClosestDistSqr)
      *pfClosestDistSqr = fBestDistSqr;

   vClosest.y = 0.0f;
#ifdef NCH_DEBUG
   BVector vTemp;
   long vf1, vf2;
   float vf3;
   if (debugHull->mbValid)
   {
      vTemp = debugHull->findClosestPointOnHull( vStart, &vf1, &vf2, &vf3 );
      BVector foo;
      foo = vTemp - vClosest;
      if (foo.length()>=0.1f)
      {
         screwed = true;
         news = true;
         game->getWorld()->getPather()->debugClearRenderLines("nonconvex");
         game->getWorld()->getPather()->debugClearRenderLines("foobar2");
         addDebugLines("foobar2", cDWORDBlue );
         debugHull->addDebugLines("foobar2", cDWORDYellow );
         game->getWorld()->getPather()->debugAddPoint("foobar2", vStart, cDWORDBlue);
         game->getWorld()->getPather()->debugAddPoint("foobar2", vClosest, cDWORDRed);
         game->getWorld()->getPather()->debugAddPoint("foobar2", vTemp, cDWORDYellow);
      }
      BASSERT( foo.length()<0.1f );
   }
#endif
   return vClosest;
}

//==============================================================================
// BNonconvexHull::segmentIntersects
//==============================================================================
const bool BNonconvexHull::segmentIntersects(const BVector &point1, const BVector &point2, const long ignorePoint,
                                             BVector &iPoint, long &segmentIndex, long &hullIndex, float &distanceSqr, 
                                             long &lNumIntersections, bool checkBBox, bool bCheckInside, float ignoreDistSqr,
                                             float errorEpsilon, bool ignoreHoles) const
{
   msegmentIntersects++;
   // If the segment can't possibly intersect the bounding box, there can't
   // be any intersection.
   lNumIntersections = 0L;
   errorEpsilon;

   if(checkBBox)
   {
      if ((point1.x>mBoundingMax.x) && (point2.x>mBoundingMax.x))
         return(false);
      if ((point1.x<mBoundingMin.x) && (point2.x<mBoundingMin.x))
         return(false);
      if ((point1.z>mBoundingMax.z) && (point2.z>mBoundingMax.z))
         return(false);
      if ((point1.z<mBoundingMin.z) && (point2.z<mBoundingMin.z))
         return(false);
   }

   BVector thisIPoint(cOriginVector);
   segmentIndex = -1;
   distanceSqr = cMaximumFloat;

   hullIndex = 0;
   // Check each segment
   for( long hull = 0; hull<getHullCount(); hull++)
   {
      if (!ignoreHoles || !getHole(hull))
      {
         long numPoints = getPointCount(hull);
         long startPoint = numPoints - 1;
         for( long endPoint =0; endPoint<numPoints; startPoint=endPoint, endPoint++ )
         {
            // Skip segment if it contains the ignore point (if any)
            if((ignorePoint>=0) && ((ignorePoint==endPoint) || (ignorePoint==startPoint)))
               continue;

            // Check this hull segment.
            long result = segmentIntersect(point1, point2, mPolygon->hull[hull].vertex[startPoint].getBVector(), mPolygon->hull[hull].vertex[endPoint].getBVector(), thisIPoint, errorEpsilon);
            if(result==cIntersection)
            {
               // Check if this is the closest to point1 so far.
               float dx = point1.x-thisIPoint.x;
               float dz = point1.z-thisIPoint.z;
               float thisDistSqr = dx*dx + dz*dz;
               if(thisDistSqr>=ignoreDistSqr)
               {
                  ++lNumIntersections;
                  if(thisDistSqr < distanceSqr)
                  {
                     distanceSqr = thisDistSqr;
                     iPoint = thisIPoint;
                     segmentIndex = startPoint;
                     hullIndex = hull;
                  } 
               }
            }
         }
      }
   }

   // If no intersection found, maybe the whole damn thing is inside the hull?
   if (bCheckInside && segmentIndex== -1)
   {
      if(inside(point1, hullIndex))
      {
         ++lNumIntersections;
         segmentIndex =-1;
         hullIndex = 0;
         distanceSqr =0.0f;
         iPoint = point1;
         return(true);
      }
   }

#ifdef NCH_DEBUG
/*
   bool result2;
   BVector pnt;
   long vf1, vf2, vf4;
   float vf3;
   if (!screwed)
   {
      result2 = debugHull->segmentIntersects( point1, point2, ignorePoint, pnt, vf1, vf2, vf3, vf4, checkBBox, bCheckInside, ignoreDistSqr, errorEpsilon, ignoreHoles );
      if (result2 != (segmentIndex>=0))
      {
         screwed = true;
         news = true;
         addDebugLines("foobar2", cDWORDBlue );
         debugHull->addDebugLines("foobar2", cDWORDYellow );
         game->getWorld()->getTerrain()->addDebugLineOverTerrain("foobar2", 0, point1, point2, cDWORDBlue, cDWORDGreen);
         game->getWorld()->getPather()->debugAddPoint("foobar2", pnt, cDWORDBlue);
      }
      BASSERT( result2 == (segmentIndex>=0) );
   }
   */
#endif
   return(segmentIndex>=0);
}

//==============================================================================
// BNonconvexHull::segmentIntersectsFast
//==============================================================================
const bool BNonconvexHull::segmentIntersectsFast(const BVector &point1, const BVector &point2, const long ignorePoint,
                                             BVector &iPoint, long &segmentIndex, long &hullIndex, float &distanceSqr,
                                             bool checkBBox, bool bCheckInside, float errorEpsilon, bool ignoreHoles) const
{
   msegmentIntersects++;
   // If the segment can't possibly intersect the bounding box, there can't
   // be any intersection.
   if(checkBBox)
   {
      if ((point1.x>mBoundingMax.x) && (point2.x>mBoundingMax.x))
         return(false);
      if ((point1.x<mBoundingMin.x) && (point2.x<mBoundingMin.x))
         return(false);
      if ((point1.z>mBoundingMax.z) && (point2.z>mBoundingMax.z))
         return(false);
      if ((point1.z<mBoundingMin.z) && (point2.z<mBoundingMin.z))
         return(false);
   }

   segmentIndex = -1;
   hullIndex = 0;
   float bestR = cMaximumFloat;
   
   // Check each segment
   for( long hull = 0; hull<getHullCount(); hull++)
   {
      if (!ignoreHoles || !getHole(hull))
      {
         long numPoints = getPointCount(hull);
         long startPoint = numPoints - 1;
         for( long endPoint =0; endPoint<numPoints; startPoint=endPoint, endPoint++ )
         {
            // Skip segment if it contains the ignore point (if any)
            if((ignorePoint>=0) && ((ignorePoint==endPoint) || (ignorePoint==startPoint)))
               continue;

            // Check this hull segment.
            const BNCHVector &start = mPolygon->hull[hull].vertex[startPoint];
            const BNCHVector &end = mPolygon->hull[hull].vertex[endPoint];
            float r, s;
            long result = segIntersect(point1.x, point1.z, point2.x, point2.z, start.x, start.z, end.x, end.z, r, s, errorEpsilon);
            if(result==cIntersection)
            {
               // Check if this is the closest to point1 so far.
               if(r < bestR)
               {
                  bestR = r;
                  segmentIndex = startPoint;
                  hullIndex = hull;
               } 
            }
         }
      }
   }
   
   // Compute point and distance.
   if(segmentIndex >= 0)
   {
      iPoint.x = point1.x+(point2.x-point1.x)*bestR;
      iPoint.y = 0.0f;
      iPoint.z = point1.z+(point2.z-point1.z)*bestR;
      
      float dx = iPoint.x-point1.x;
      float dz = iPoint.z-point1.z;
      distanceSqr = dx*dx + dz*dz;
   }
   else if (bCheckInside)
   {
      // If no intersection found, maybe the whole damn thing is inside the hull?
      if(inside(point1, hullIndex))
      {
         segmentIndex =-1;
         hullIndex = 0;
         distanceSqr =0.0f;
         iPoint = point1;
         return(true);
      }
   }

#ifdef NCH_DEBUG
   /*
   bool result2;
   BVector pnt;
   long vf1, vf2, vf4;
   float vf3;
   if (!screwed)
   {
      result2 = debugHull->segmentIntersects( point1, point2, ignorePoint, pnt, vf1, vf2, vf3, vf4, checkBBox, bCheckInside, ignoreDistSqr, errorEpsilon, ignoreHoles );
      if (result2 != (segmentIndex>=0))
      {
         screwed = true;
         news = true;
         addDebugLines("foobar2", cDWORDBlue );
         debugHull->addDebugLines("foobar2", cDWORDYellow );
         game->getWorld()->getTerrain()->addDebugLineOverTerrain("foobar2", 0, point1, point2, cDWORDBlue, cDWORDGreen);
         game->getWorld()->getPather()->debugAddPoint("foobar2", pnt, cDWORDBlue);
      }
      BASSERT( result2 == (segmentIndex>=0) );
   }
   */
#endif
   return(segmentIndex>=0);
}

//==============================================================================
// BNonconvexHull::segmentIntersectsFast_3
//==============================================================================
const bool BNonconvexHull::segmentIntersectsFast_3(const BVector &point1, const BVector &point2, const long ignoreHull, const long ignorePoint,
                                                 BVector &iPoint, long &segmentIndex, long &hullIndex, float &distanceSqr,
                                                 bool checkBBox, bool bCheckInside, float errorEpsilon, bool ignoreHoles) const
{
   msegmentIntersects++;
   // If the segment can't possibly intersect the bounding box, there can't
   // be any intersection.
   if(checkBBox)
   {
      if ((point1.x>mBoundingMax.x) && (point2.x>mBoundingMax.x))
         return(false);
      if ((point1.x<mBoundingMin.x) && (point2.x<mBoundingMin.x))
         return(false);
      if ((point1.z>mBoundingMax.z) && (point2.z>mBoundingMax.z))
         return(false);
      if ((point1.z<mBoundingMin.z) && (point2.z<mBoundingMin.z))
         return(false);
   }

   segmentIndex = -1;
   hullIndex = 0;
   float bestR = cMaximumFloat;

   // Check each segment
   for( long hull = 0; hull<getHullCount(); hull++)
   {
      if (!ignoreHoles || !getHole(hull))
      {
         long numPoints = getPointCount(hull);
         long startPoint = numPoints - 1;
         for( long endPoint =0; endPoint<numPoints; startPoint=endPoint, endPoint++ )
         {
            // Skip segment if it contains the ignore point (if any) ((But only for the correct hull..)
            if (ignoreHull >= 0 && ignoreHull == hull)
            {
               if((ignorePoint>=0) && ((ignorePoint==endPoint) || (ignorePoint==startPoint)))
                  continue;
            }

            // Check this hull segment.
            const BNCHVector &start = mPolygon->hull[hull].vertex[startPoint];
            const BNCHVector &end = mPolygon->hull[hull].vertex[endPoint];
            float r, s;
            long result = segIntersect(point1.x, point1.z, point2.x, point2.z, start.x, start.z, end.x, end.z, r, s, errorEpsilon);
            if(result==cIntersection)
            {
               // Check if this is the closest to point1 so far.
               if(r < bestR)
               {
                  bestR = r;
                  segmentIndex = startPoint;
                  hullIndex = hull;
               } 
            }
         }
      }
   }

   // Compute point and distance.
   if(segmentIndex >= 0)
   {
      iPoint.x = point1.x+(point2.x-point1.x)*bestR;
      iPoint.y = 0.0f;
      iPoint.z = point1.z+(point2.z-point1.z)*bestR;

      float dx = iPoint.x-point1.x;
      float dz = iPoint.z-point1.z;
      distanceSqr = dx*dx + dz*dz;
   }
   else if (bCheckInside)
   {
      // If no intersection found, maybe the whole damn thing is inside the hull?
      if(inside(point1, hullIndex))
      {
         segmentIndex =-1;
         hullIndex = 0;
         distanceSqr =0.0f;
         iPoint = point1;
         return(true);
      }
   }

#ifdef NCH_DEBUG
   /*
   bool result2;
   BVector pnt;
   long vf1, vf2, vf4;
   float vf3;
   if (!screwed)
   {
   result2 = debugHull->segmentIntersects( point1, point2, ignorePoint, pnt, vf1, vf2, vf3, vf4, checkBBox, bCheckInside, ignoreDistSqr, errorEpsilon, ignoreHoles );
   if (result2 != (segmentIndex>=0))
   {
   screwed = true;
   news = true;
   addDebugLines("foobar2", cDWORDBlue );
   debugHull->addDebugLines("foobar2", cDWORDYellow );
   game->getWorld()->getTerrain()->addDebugLineOverTerrain("foobar2", 0, point1, point2, cDWORDBlue, cDWORDGreen);
   game->getWorld()->getPather()->debugAddPoint("foobar2", pnt, cDWORDBlue);
   }
   BASSERT( result2 == (segmentIndex>=0) );
   }
   */
#endif
   return(segmentIndex>=0);
}

//==============================================================================
// BNonconvexHull::getObstructions
//==============================================================================
BDynamicSimArray<BOPObstructionNode*>& BNonconvexHull::getObstructions()
{
   return mObstructionList;
}

//==============================================================================
// BNonconvexHull::getObstruction
//==============================================================================
const BOPObstructionNode* BNonconvexHull::getObstruction(long lIndex) const
{
   return mObstructionList[lIndex];
}

//==============================================================================
// BNonconvexHull::getObstructionCount
//==============================================================================
const long BNonconvexHull::getObstructionCount()
{
   return mObstructionList.getNumber();
}

//==============================================================================
// BNonconvexHull::inside
//==============================================================================
//==============================================================================
// BNonconvexHull::inside
//==============================================================================
void BNonconvexHull::convertHoleToHull(const long hull)
{
   // Get new poly.
   long newPoly = getNoncachePolygon();
   
   BASSERTM( !mbDynAllocPolygon, "NCH calling convertHoleToHull with a dynamic polygon" );  //This function will screw up memory if called on a convex hull with a dynamically allocated polygon.

   // Copy it.
   *nonCachePolygon[newPoly] = *mPolygon;
   
   // De-refcount the old one.
   mPolygon->dealloc();
   
   // Assign in the new one.
   mPolygon = nonCachePolygon[newPoly];
   mPolygon->mAge = cFrame;
   
   // jce [11/10/2004] -- this was alloc++, but that seemed wrong, so I changed it.  Let's see if the world explodes. 
   mPolygon->mAlloc = 1;
   
   
   long pc = getPointCount(hull);
   //
   // need to reverse the direction of the hull so it now points the right way around
   //
   if (hull == 0)
   {
      //BMemoryStackFrame stackFrame;
      //BNCHVector *temp = (BNCHVector*)stackFrame.getMemory(pc*sizeof(BNCHVector));
      
      BNCHVector *temp = (BNCHVector*)gPrimaryHeap.New(pc*sizeof(BNCHVector));
      
      if(!temp)
      {
         BFAIL("mem alloc error");
         return;
      }
      for(long i=0; i<pc; i++)
      {
         temp[i] = mPolygon->hull[0].vertex[pc-1-i];
      }
      for(long i=0; i<pc; i++)
      {
         mPolygon->hull[0].vertex[i] = temp[i];
      }
      
      gPrimaryHeap.Delete(temp);
   }
   else
   {
      mPolygon->hull[0].vertex.setNumber( pc );
      for(long i=0; i<pc; i++)
      {
         mPolygon->hull[0].vertex[i] = mPolygon->hull[hull].vertex[pc-1-i];
      }
   }
   mPolygon->hull[0].hole = false;
   mPolygon->hull.setNumber(1);
   computeBoundingBox();
/*
#ifdef DEBUG_GRAPHNONCONVEX
   screwed = true;
   news = true;
   gPather.debugClearRenderLines(BDebugPrimitives::cCategoryPatherNonConvex);
   gPather.debugClearRenderLines(BDebugPrimitives::cCategoryPatherNonConvex2);
   addDebugLines(BDebugPrimitives::cCategoryPatherNonConvex2, cDWORDBlue);
#ifdef NCH_DEBUG
   debugHull->addDebugLines("foobar2", cDWORDYellow );
#endif
#endif
*/
}

//==============================================================================
// BNonconvexHull::convertHullToConvex
//==============================================================================
void BNonconvexHull::convertHullToConvex(long hull)
{
   BASSERT((hull >= 0) && (hull <mPolygon->hull.getNumber()));

   // Sort.
   BNonconvexHull::shellsortNCH(mPolygon->hull[hull].vertex.getPtr(), mPolygon->hull[hull].vertex.getNumber());

   // Build upper hull.
   long f;
   if (getHole(hull))
      f = BNonconvexHull::leftScanHullNCH(mPolygon->hull[hull].vertex.getPtr(), mPolygon->hull[hull].vertex.getNumber());
   else
      f = BNonconvexHull::rightScanHullNCH(mPolygon->hull[hull].vertex.getPtr(), mPolygon->hull[hull].vertex.getNumber());

   for(long i =0; i<f-1; i++)
      bswap(mPolygon->hull[hull].vertex[i], mPolygon->hull[hull].vertex[i+1]);

   // Sort for lower hull.
   BNonconvexHull::shellsortNCH2(mPolygon->hull[hull].vertex.getPtr()+(f-2), mPolygon->hull[hull].vertex.getNumber()-f+2);

   // Build lower hull.
   long g;
   if (getHole(hull))
      g = BNonconvexHull::leftScanHullNCH(&mPolygon->hull[hull].vertex[f-2], mPolygon->hull[hull].vertex.getNumber()-f+2);
   else
      g = BNonconvexHull::rightScanHullNCH(&mPolygon->hull[hull].vertex[f-2], mPolygon->hull[hull].vertex.getNumber()-f+2);

   long lnum = f+g-2;
   // Update point count.
   mPolygon->hull[hull].vertex.setNumber(lnum);

   computeBoundingBox();

}

//==============================================================================
// BNonconvexHull::inside
//==============================================================================
const bool BNonconvexHull::inside(const BVector &point, long &ihull) const
{
   minside++;
   // First check bounding box.
   if(point.x < mBoundingMin.x)
      return(false);
   if(point.z < mBoundingMin.z)
      return(false);
   if(point.x > mBoundingMax.x)
      return(false);
   if(point.z > mBoundingMax.z)
      return(false);

   // check the exterior (the outlines) hulls
   for( long i =0; i<getHullCount(); i++ )
   {
      if (!getHole(i))
      {
         ihull = i;
         bool iaminside = (pointInXZProjectionNCH( getPoints(i).getPtr(), getPointCount(i), point));
         if (iaminside)
            return(true);
      }
   }
   // If we return false, make sure iHull is set to an invalid value. 
   ihull = -1;
   return(false);
}

//==============================================================================
// BNonconvexHull::inside
//==============================================================================
const bool BNonconvexHull::insideHole(const BVector &point, long &ihull) const
{
   // First check bounding box.
   if(point.x < mBoundingMin.x)
      return(false);
   if(point.z < mBoundingMin.z)
      return(false);
   if(point.x > mBoundingMax.x)
      return(false);
   if(point.z > mBoundingMax.z)
      return(false);

   // check the interior hulls (the holes)
   for( long i =0; i<getHullCount(); i++ )
   {
      if (getHole(i))
      {
         ihull = i;
         bool iaminside = (pointInXZProjectionCCNCH( getPoints(i).getPtr(), getPointCount(i), point));
         if (iaminside)
         {
            return(true);
         }
      }
   }
   return(false);
}

//==============================================================================
// BNonconvexHull::insideConvexHull
// The original version of this is a lie.  I'm going to change it to be true.
// We'll go through the hulls in this hull.  For each one, we'll make a copy
// of it, convert it to it's convex form, and then test for being inside.  
//==============================================================================
const bool BNonconvexHull::insideConvexHull(const BVector &point, long &iHull) const
{
   mInsideConvexHull++;
   // if I'm built convex, then just return the easy check. 
   if (mIsConvex)
      return pointInXZProjectionNCH( getPoints(0).getPtr(), getPointCount(0), point);

   // check the exterior (the outlines) hulls
   for( long i =0; i<getHullCount(); i++ )
   {
      if (!getHole(i))
      {
         BNonconvexHull temp;
         const BDynamicArray<BNCHVector> &points = getPoints(i);
         temp.initializeNonConvex((BNCHVector *)&points, getPointCount(i));
         temp.convertHullToConvex(0);
         long tempHull = 0;
         if (temp.inside(point, tempHull))
         {
            iHull = i;
            return true;
         }
      }
   }
   return(false);
}

//==============================================================================
// BNonconvexHull::isInside
// Return true if the passed in hull is inside of me
//==============================================================================
const bool BNonconvexHull::isInside(const BNonconvexHull &hull) const
{
   misInside++;
   if(mBoundingMin.x > hull.mBoundingMax.x)
      return(false);
   if(mBoundingMin.z > hull.mBoundingMax.z)
      return(false);
   if(mBoundingMax.x < hull.mBoundingMin.x)
      return(false);
   if(mBoundingMax.z < hull.mBoundingMin.z)
      return(false);

   long hc = hull.getHullCount();

   for( long j =0; j<hc; j++)
   {
      if (!hull.getHole(j))
      {
         // Check if any of the other hulls points are within this hull.
         // they must all be inside for isInside to return true.
         long count = hull.getPointCount(j);
         const BNCHVector *points = hull.getPoints(j).getPtr();

         bool allIn = true;
         for(long i =0; i<count; i++)
         {
            long nTempHull = 0;
            bool result = ( inside(points[i].getBVector(), nTempHull) );
            if(!result)
            {
               allIn = false;
               break;
            }
         }
         // if all the points were inside one of our hulls, then we're covered
         if (allIn)
         {
#ifdef NCH_DEBUG
            BASSERT( debugHull->isInside(*(hull.debugHull)) == true );
#endif
            return(true);
         }
      }
   }
   return(false);
}

//=============================================================================
// BNonconvexHull::checkConvexSide
//=============================================================================
bool BNonconvexHull::checkConvexSide(const long h, const float dx, const float dz, const BNCHVector &v) const
{
   // Vertices are projected to the form v+t*d.
   // Return value is +1 if all t > 0, -1 if all t < 0, 0 otherwise, in
   // which case the line splits the polygon.
   long numPoints = mPolygon->hull[h].vertex.getNumber();

   for (long i = numPoints-1; i>=0; i--)
   {
      float x =mPolygon->hull[h].vertex[i].x-v.x;
      float z =mPolygon->hull[h].vertex[i].z-v.z;
      float t = (x*dx+z*dz);
      if(t<0)
         return(false);
   }
   return(true);
}

//==============================================================================
// BNonconvexHull::overlapsBox
//==============================================================================
const bool BNonconvexHull::overlapsBox(const float minX, const float minZ, const float maxX, const float maxZ) const
{
   // First check if bounding boxes overlap.
   if(minX > mBoundingMax.x)
      return(false);
   if(minZ > mBoundingMax.z)
      return(false);
   if(maxX < mBoundingMin.x)
      return(false);
   if(maxZ < mBoundingMin.z)
      return(false);

   // Now check if the actual hull overlaps the box.
   // First set up points of box.
   BVector points[4] = {BVector(minX, 0.0f, minZ), BVector(minX, 0.0f, maxZ),
      BVector(maxX, 0.0f, maxZ), BVector(maxX, 0.0f, minZ)};

   // Check if hulls points are inside box.
   long numPoints = mPolygon->hull[0].vertex.getNumber();
   for(long i=0; i<numPoints; i++)
   {
      float nx =mPolygon->hull[0].vertex[i].x;
      float nz =mPolygon->hull[0].vertex[i].z;
      if(nx>=minX && nx<=maxX && nz>=minZ && nz<=maxZ)
         return(true);
   }

   // Check if points are inside the hull.
   for(long i=0; i<4; i++)
   {
      // jce 12/6/2000 -- instead of calling the inside function, we just do a check ourselves here.
      // This makes sense because we've already checked the bounding box, etc.
      bool result = pointInXZProjectionNCH( getPoints(0).getPtr(), getPointCount(0), points[i]);
      if(result)
         return(true);
   }

   // Check segments of box against our segments.
   long startPoint = 3;
   for(long endPoint=0; endPoint<4; startPoint=endPoint, endPoint++)
   {
      bool result = segmentIntersects(points[startPoint], points[endPoint], false);
      if(result)
         return(true);
   }

   // If we got here, the two don't overlap.
   return(false);
}

//==============================================================================
// BNonconvexHull::overlapsHull
//==============================================================================
const bool BNonconvexHull::overlapsHull(const BNonconvexHull &hull) const
{
   // Check if bounding boxes overlap, fail now if they don't.
   if(mBoundingMin.x > hull.mBoundingMax.x)
      return(false);
   if(mBoundingMin.z > hull.mBoundingMax.z)
      return(false);
   if(mBoundingMax.x < hull.mBoundingMin.x)
      return(false);
   if(mBoundingMax.z < hull.mBoundingMin.z)
      return(false);

   // If both are convex, do the faster specialized check -- ripped from convex hull.
   // assumes convex hulls are single hulls
   if(mIsConvex && hull.isBuiltConvex())
   {
      // Test edges of C0 for separation. Because of the counterclockwise ordering,
      // the projection interval for C0 is [m,0] where m <= 0. Only try to determine
      // if C1 is on the ‘positive’ side of the line.
      long i0, i1;
      long numPoints = mPolygon->hull[0].vertex.getNumber();
      for (i0 = 0, i1 = numPoints-1; i0 < numPoints; i1 = i0, i0++)
      {
         float nx =mPolygon->hull[0].vertex[i1].z-mPolygon->hull[0].vertex[i0].z;
         float nz =mPolygon->hull[0].vertex[i0].x-mPolygon->hull[0].vertex[i1].x;
         if (hull.checkConvexSide(0, nx, nz, mPolygon->hull[0].vertex[i0]))
         { // C1 is entirely on ‘positive’ side of line C0.V(i0)+t*D
            return(false);
         }
      }
      // Test edges of C1 for separation. Because of the counterclockwise ordering,
      // the projection interval for C1 is [m,0] where m <= 0. Only try to determine
      // if C0 is on the ‘positive’ side of the line.
      numPoints = hull.mPolygon->hull[0].vertex.getNumber();
      for (i0 = 0, i1 = numPoints-1; i0 < numPoints; i1 = i0, i0++)
      {
         float nx =hull.mPolygon->hull[0].vertex[i1].z - hull.mPolygon->hull[0].vertex[i0].z;
         float nz =hull.mPolygon->hull[0].vertex[i0].x - hull.mPolygon->hull[0].vertex[i1].x;
         if (checkConvexSide(0, nx, nz, hull.mPolygon->hull[0].vertex[i0]))
         { // C0 is entirely on ‘positive’ side of line C1.V(i0)+t*D
            return(false);
         }
      }
      return(true);
   }

   moverlapsHull++;

   for( long h = 0; h<getHullCount(); h++)
   {
      if (!getHole(h))
      {
         long numPoints = getPointCount(h);
         // Check if any of our points are in the other hull.
         const BNCHVector *points = getPoints(h).getPtr();
         for(long i =0; i<numPoints; i++)
         {
            long nTempHull;
            bool result = hull.inside( points[i].getBVector(), nTempHull );
            if(result)
               return(true);
         }
      }
   }

   for( long h = 0; h<hull.getHullCount(); h++)
   {
      if (!hull.getHole(h))
      {
         long numPoints = hull.getPointCount(h);
         // Check if any of the other hulls points are within this hull.
         const BNCHVector *points = hull.getPoints(h).getPtr();
         for(long i =0; i<numPoints; i++)
         {
            long nTempHull = 0;
            bool result = inside(points[i].getBVector(), nTempHull);
            if(result)
               return(true);
         }
      }
   }

   // Check if any of our segments intersect the segments of the other hull.
   for( long h = 0; h<getHullCount(); h++)
   {
      if (!getHole(h))
      {
         long numPoints = getPointCount(h);
         long startPoint = numPoints - 1;
         const BNCHVector *points = getPoints(h).getPtr();
         for( long endPoint =0; endPoint<numPoints; endPoint++ )
         {
            bool result = hull.segmentIntersects(points[startPoint].getBVector(), points[endPoint].getBVector());
            if(result)
               return(true);
            startPoint =endPoint;
         }
      }
   }

   // If we got here, we don't overlap.
   return(false);
}

//==============================================================================
// BNonconvexHull::isBuiltConvex
//==============================================================================
const bool BNonconvexHull::isBuiltConvex(void) const
{
   return mIsConvex;
}

//==============================================================================
// BNonconvexHull::computeBoundingBox
//==============================================================================
void BNonconvexHull::computeBoundingBox(void)
{
   // Init bounding box.
   mBoundingMin.z = cMaximumFloat;
   mBoundingMin.x = cMaximumFloat;
   mBoundingMax.x = -cMaximumFloat;
   mBoundingMax.z = -cMaximumFloat;

   for( long hull = 0; hull<getHullCount(); hull++)
   {
      long numPoints = getPointCount( hull );
      for( long i =0; i<numPoints; i++ )
      {
         const BNCHVector& pnt = mPolygon->hull[hull].vertex[i];
         // Update bounding box.
         if (pnt.x < mBoundingMin.x)
            mBoundingMin.x = pnt.x;
         if (pnt.x > mBoundingMax.x)
            mBoundingMax.x = pnt.x;

         if (pnt.z < mBoundingMin.z)
            mBoundingMin.z = pnt.z;
         if (pnt.z > mBoundingMax.z)
            mBoundingMax.z = pnt.z;
      }
   }
}

//==============================================================================
// BNonconvexHull::segmentIntersects
//==============================================================================
const bool BNonconvexHull::segmentIntersects(const BVector &point1, const BVector &point2, float errorEpsilon, bool ignoreHoles) const
{
   static BVector iPoint(cOriginVector);

   // Check each segment
   for( long hull = 0; hull<getHullCount(); hull++)
   {
      if (!ignoreHoles || !getHole(hull))
      {
         long numPoints = getPointCount(hull);
         long startPoint = numPoints - 1;
         for( long endPoint =0; endPoint<numPoints; endPoint++ )
         {
            // Check this hull segment.
            long result = segmentIntersect(point1, point2, mPolygon->hull[hull].vertex[startPoint].getBVector(), mPolygon->hull[hull].vertex[endPoint].getBVector(), iPoint, errorEpsilon);
            if(result==cIntersection)
               return(true);
            startPoint =endPoint;
         } 
      }
   }


   return(false);
}

//==============================================================================
//
//==============================================================================
void BNCHBuilder::resetST()
{
   mOnSTNode = 0;
}

//==============================================================================
//
//==============================================================================
void BNCHBuilder::resetIT(it_node **it)
{
   mOnITNode = 0;
   *it = NULL;
}

//==============================================================================
//
//==============================================================================
void BNCHBuilder::resetLMT(lmt_node **lmt)
{
   mOnLNode = 0;
   *lmt = NULL;
}

//==============================================================================
//
//==============================================================================
void BNCHBuilder::resetSBT(sb_tree **sbtree)
{
   mOnSBTree = 0;
   *sbtree = NULL;
}

//==============================================================================
//
//==============================================================================
void BNCHBuilder::insertBound(edge_node **b, edge_node *e)
{
   edge_node *existing_bound;

   if (!*b)
   {
      // Link node e to the tail of the list 
      *b = e;
   }
   else
   {
      // Do primary sort on the x field 
      if (e[0].vertex[1].x < (*b)[0].vertex[1].x)
      {
         // Insert a new node mid-list 
         existing_bound = *b;
         *b = e;
         (*b)->ptrs[ENEXTBOUND] = existing_bound;
      }
      else
      {
         if (e[0].vertex[1].x== (*b)[0].vertex[1].x)
         {
            // Do secondary sort on the dx field 
            if (e[0].dx < (*b)[0].dx)
            {
               // Insert a new node mid-list 
               existing_bound = *b;
               *b = e;
               (*b)->ptrs[ENEXTBOUND] = existing_bound;
            }
            else
            {
               // Head further down the list 
               insertBound(&((*b)->ptrs[ENEXTBOUND]), e);
            }
         }
         else
         {
            // Head further down the list 
            insertBound(&((*b)->ptrs[ENEXTBOUND]), e);
         }
      }
   }
}

//==============================================================================
//
//==============================================================================
edge_node **BNCHBuilder::boundList(lmt_node **ilmt, NCHfloat z)
{
   lmt_node **lmt = ilmt;
   do {
      if (!*lmt)
      {
         // Add node onto the tail end of the LMT 
         *lmt = &mLNodes[mOnLNode++]; BASSERT( mOnLNode < cMaxLNodes );
         (*lmt)->z = z;
         (*lmt)->first_bound = NULL;
         (*lmt)->next = NULL;
         return &((*lmt)->first_bound);
      }
      else
      {
         if (z < (*lmt)->z)
         {
            // Insert a new LMT node before the current node 
            lmt_node *existing_node = *lmt;
            *lmt = &mLNodes[mOnLNode++]; BASSERT( mOnLNode < cMaxLNodes );
            (*lmt)->z = z;
            (*lmt)->first_bound = NULL;
            (*lmt)->next = existing_node;
            return &((*lmt)->first_bound);
         }
         else
         {
            if (z > (*lmt)->z)
            {
               // Head further up the LMT 
               lmt = &((*lmt)->next);
            }
            else
            {
               // Use this existing LMT node 
               return &((*lmt)->first_bound);
            }
         }
      }
   } while (true);
}

//==============================================================================
//
//==============================================================================
void BNCHBuilder::addToSBT(int *entries, sb_tree **tree, NCHfloat z)
{
   if (IDENTICAL_FLOAT(mLastZ, z))
      return;

   sb_tree **sbtree = tree;
   while (*sbtree)
   {
      const NCHfloat za = (*sbtree)->z;
      if (za > z)
      {
         // Head into the 'less' sub-tree 
         sbtree = &((*sbtree)->less);
      }
      else if (za < z)
      {
         // Head into the 'more' sub-tree 
         sbtree = &((*sbtree)->more);
      }
      else
      {
         // Already had this entry
         return;
      }
   }
   // Add a new tree node here 
   *sbtree = &mSBNodes[mOnSBTree++]; BASSERT( mOnSBTree < cMaxSBNodes );
   (*sbtree)->z = z;
   (*sbtree)->less = NULL;
   (*sbtree)->more = NULL;
   (*entries)++;
   mLastZ = z;
}

//==============================================================================
//
//==============================================================================
void BNCHBuilder::buildLMT(lmt_node **lmt, sb_tree **sbtree, int *sbt_entries, const BNCHPolygon& p, int type)
{
   int          c, i, min, max, num_edges, v, num_vertices;
   int          e_index =0;
   edge_node   *e, *edge_table;

   // Create the entire input polygon edge table in one go 
   edge_table = &mENodes[mOnENode];

   for ( c = 0; c < p.hull.getNumber(); c++)
   {
      // Perform hull optimization
      const BNCHVector *currVert = &p.hull[c].vertex[0];
      edge_node *edgeNode = &edge_table[0];
      for (num_vertices = 0; num_vertices < p.hull[c].vertex.getNumber(); num_vertices++)
      {
         BNCHVector *edgeVert = &edgeNode->vertex[0];
         *edgeVert = *currVert;
         
         // Record vertex in the scanbeam table 
         addToSBT(sbt_entries, sbtree, edgeVert->z);

         currVert++;
         edgeNode++;
         mOnENode++;
         BASSERT(mOnENode < cMaxENodes);
      }

      long nextIndex = 1;
      long prevIndex = (num_vertices)-1;
      // Do the hull forward pass 
      for (min = 0; min < num_vertices; prevIndex = min, min++)
      {
         // If a forward local minimum... 
         if ((edge_table[prevIndex].vertex[0].z >= edge_table[min].vertex[0].z) && (edge_table[nextIndex].vertex[0].z > edge_table[min].vertex[0].z))
         {
            // Search for the next local maximum... 
            num_edges = 1;
            max = nextIndex;
            long nextMax = max+1;
            if (nextMax == num_vertices)
               nextMax = 0;
            while (edge_table[nextMax].vertex[0].z > edge_table[max].vertex[0].z)
            {
               num_edges++;
               max = nextMax;
               nextMax++;
               if (nextMax == num_vertices)
                  nextMax = 0;
            }

            // Build the next edge list 
            e = &edge_table[e_index];
            e_index += num_edges;
            v = min;
            e[0].scratch[3][1] = UNBUNDLED;
            e[0].scratch[2][HULL_CLIP] = false;
            e[0].scratch[2][HULL_MAIN] = false;
            for (i = 0; i < num_edges; i++)
            {
               e[i].xb = edge_table[v].vertex[0].x;
               e[i].vertex[1].x = edge_table[v].vertex[0].x;
               e[i].vertex[1].z = edge_table[v].vertex[0].z;

               v++;
               if (v == num_vertices)
                  v = 0;

               e[i].vertex[2].x = edge_table[v].vertex[0].x;
               e[i].vertex[2].z = edge_table[v].vertex[0].z;
               e[i].dx = (edge_table[v].vertex[0].x - e[i].vertex[1].x) / (e[i].vertex[2].z - e[i].vertex[1].z);
               e[i].type = type;
               e[i].outp[0] = NULL;
               e[i].outp[1] = NULL;
               e[i].ptrs[EPREV] = NULL;
               e[i].ptrs[ENEXT] = NULL;
               e[i].ptrs[EPRED] = ((num_edges > 1) && (i > 0)) ? &(e[i - 1]) : NULL;
               e[i].ptrs[ESUCC] = ((num_edges > 1) && (i < (num_edges - 1))) ? &(e[i + 1]) : NULL;
               e[i].ptrs[ENEXTBOUND] = NULL;
               e[i].scratch[0][HULL_CLIP] = HULL_LEFT;
               e[i].scratch[0][HULL_MAIN] = HULL_LEFT;
            }
            insertBound(boundList(lmt, edge_table[min].vertex[0].z), e);
         }
         nextIndex++;
         if (nextIndex == num_vertices)
            nextIndex = 0;
      }

      nextIndex = 1;
      prevIndex = (num_vertices)-1;
      // Do the hull reverse pass 
      for (min = 0; min < num_vertices; prevIndex = min, min++)
      {
         // If a reverse local minimum... 
         if ((edge_table[prevIndex].vertex[0].z > edge_table[min].vertex[0].z) && (edge_table[nextIndex].vertex[0].z >= edge_table[min].vertex[0].z))
         {
            // Search for the previous local maximum... 
            num_edges = 1;
            max = prevIndex;
            long prevMax = max - 1;
            if (prevMax == -1)
               prevMax = (num_vertices)-1;
            while (edge_table[prevMax].vertex[0].z > edge_table[max].vertex[0].z)
            {
               num_edges++;
               max = prevMax;
               prevMax--;
               if (prevMax == -1)
                  prevMax = (num_vertices)-1;
            }

            // Build the previous edge list 
            e = &edge_table[e_index];
            e_index+= num_edges;
            v = min;
            e[0].scratch[3][1] = UNBUNDLED;
            e[0].scratch[2][HULL_CLIP] = false;
            e[0].scratch[2][HULL_MAIN] = false;
            for (i = 0; i < num_edges; i++)
            {
               e[i].xb = edge_table[v].vertex[0].x;
               e[i].vertex[1].x = edge_table[v].vertex[0].x;
               e[i].vertex[1].z = edge_table[v].vertex[0].z;

               v--;
               if (v == -1)
                  v = (num_vertices)-1;;

               e[i].vertex[2].x = edge_table[v].vertex[0].x;
               e[i].vertex[2].z = edge_table[v].vertex[0].z;
               e[i].dx = (edge_table[v].vertex[0].x - e[i].vertex[1].x) / (e[i].vertex[2].z - e[i].vertex[1].z);
               e[i].type = type;
               e[i].outp[0] = NULL;
               e[i].outp[1] = NULL;
               e[i].ptrs[EPREV] = NULL;
               e[i].ptrs[ENEXT] = NULL;
               e[i].ptrs[EPRED] = ((num_edges > 1) && (i > 0)) ? &(e[i - 1]) : NULL;
               e[i].ptrs[ESUCC] = ((num_edges > 1) && (i < (num_edges - 1))) ? &(e[i + 1]) : NULL;
               e[i].ptrs[ENEXTBOUND] = NULL;
               e[i].scratch[0][HULL_CLIP] = HULL_LEFT;
               e[i].scratch[0][HULL_MAIN] = HULL_LEFT;
            }
            insertBound(boundList(lmt, edge_table[min].vertex[0].z), e);
         }
         nextIndex++;
         if (nextIndex == num_vertices)
            nextIndex = 0;
      }
   }
}

//==============================================================================
//
//==============================================================================
void BNCHBuilder::buildSBT(int *entries, NCHfloat *sbt, sb_tree *sbtree)
{
   if (sbtree->less)
      buildSBT(entries, sbt, sbtree->less);
   sbt[*entries] = sbtree->z;
   (*entries)++;
   if (sbtree->more)
      buildSBT(entries, sbt, sbtree->more);
}

//==============================================================================
//
//==============================================================================
void BNCHBuilder::addEdgeToAET(edge_node **aet, edge_node *edge, edge_node *prev)
{
   if (!*aet)
   {
      // Append edge onto the tail end of the AET 
      *aet = edge;
      edge->ptrs[EPREV] = prev;
      edge->ptrs[ENEXT] = NULL;
   }
   else
   {
      // Do primary sort on the xb field 
      if (edge->xb < (*aet)->xb)
      {
         // Insert edge here (before the AET edge) 
         edge->ptrs[EPREV] = prev;
         edge->ptrs[ENEXT] = *aet;
         (*aet)->ptrs[EPREV] = edge;
         *aet = edge;
      }
      else
      {
         if (edge->xb== (*aet)->xb)
         {
            // Do secondary sort on the dx field 
            if (edge->dx < (*aet)->dx)
            {
               // Insert edge here (before the AET edge) 
               edge->ptrs[EPREV] = prev;
               edge->ptrs[ENEXT] = *aet;
               (*aet)->ptrs[EPREV] = edge;
               *aet = edge;
            }
            else
            {
               // Head further into the AET 
               addEdgeToAET(&((*aet)->ptrs[ENEXT]), edge, *aet);
            }
         }
         else
         {
            // Head further into the AET 
            addEdgeToAET(&((*aet)->ptrs[ENEXT]), edge, *aet);
         }
      }
   }
}

//==============================================================================
//
//==============================================================================
void BNCHBuilder::addIntersection(it_node **it, edge_node *edge0, edge_node *edge1, NCHfloat x, NCHfloat z)
{
   it_node *existing_node;

   if (!*it)
   {
      // Append a new node to the tail of the list 
      *it = &mITNodes[mOnITNode++]; BASSERT( mOnITNode<cMaxITNodes );
      (*it)->ie[0] = edge0;
      (*it)->ie[1] = edge1;
      (*it)->point.x = (float)x;
      (*it)->point.z = (float)z;
      (*it)->next = NULL;
   }
   else
   {
      if ((*it)->point.z > z)
      {
         // Insert a new node mid-list 
         existing_node = *it;
         *it = &mITNodes[mOnITNode++]; BASSERT( mOnITNode<cMaxITNodes );
         (*it)->ie[0] = edge0;
         (*it)->ie[1] = edge1;
         (*it)->point.x = (float)x;
         (*it)->point.z = (float)z;
         (*it)->next = existing_node;
      }
      else
         // Head further down the list 
         addIntersection(&((*it)->next), edge0, edge1, x, z);
   }
}


//==============================================================================
//
//==============================================================================
void BNCHBuilder::addSTEdge(st_node **st, it_node **it, edge_node *edge, NCHfloat dz)
{
   st_node *existing_node;
   NCHfloat   den, r, x, z;

   if (!*st)
   {
      // Append edge onto the tail end of the ST 
      *st = &mSTNodes[mOnSTNode++];
      BASSERT( mOnSTNode < cMaxSTNodes );
      (*st)->edge = edge;
      (*st)->xb = edge->xb;
      (*st)->xt = edge->xt;
      (*st)->dx = edge->dx;
      (*st)->prev = NULL;
   }
   else
   {
      den = ((*st)->xt - (*st)->xb) - (edge->xt - edge->xb);

      // If new edge and ST edge don't cross 
      if ((edge->xt >= (*st)->xt) || (IDENTICAL_FLOAT(edge->dx, (*st)->dx)) || 
//         ((INT_VAL(den)&0x7FFFFFFF)<=INT_VAL(NCHEpsilon)))
         (fabs(den)<=NCHEpsilon))
      {
         // No intersection - insert edge here (before the ST edge)
         existing_node = *st;
         *st = &mSTNodes[mOnSTNode++];
         BASSERT( mOnSTNode < cMaxSTNodes );
         (*st)->edge = edge;
         (*st)->xb = edge->xb;
         (*st)->xt = edge->xt;
         (*st)->dx = edge->dx;
         (*st)->prev = existing_node;
      }
      else
      {
         // Compute intersection between new edge and ST edge 
         r = (edge->xb - (*st)->xb) / den;
         x = (*st)->xb + r * ((*st)->xt - (*st)->xb);
         z = r * dz;

         // Insert the edge pointers and the intersection point in the IT 
         addIntersection(it, (*st)->edge, edge, x, z);

         // Head further into the ST 
         addSTEdge(&((*st)->prev), it, edge, dz);
      }
   }
}


//==============================================================================
//
//==============================================================================
void BNCHBuilder::buildIntersectionTable(it_node **it, edge_node *aet, NCHfloat dz)
{
   st_node   *st;
   edge_node *edge;

   // Build intersection table for the current scanbeam 
   resetIT(it);
   st = NULL;

   // Process each AET edge 
   for (edge = aet; edge; edge = edge->ptrs[ENEXT])
   {
      if ((edge->scratch[3][0] == BUNDLE_HEAD) ||
         edge->scratch[1][HULL_CLIP] || edge->scratch[1][HULL_MAIN])
         addSTEdge(&st, it, edge, dz);
   }

   resetST();
}

//==============================================================================
//
//==============================================================================
int BNCHBuilder::countHulls(polygon_node *polygon)
{
   int          nc, nv;
   vertex_node *v, *nextv;

   for (nc = 0; polygon; polygon = polygon->next)
      if (polygon->active)
      {
         // Count the vertices in the current hull 
         nv = 0;
         for (v = polygon->proxy->v[HULL_LEFT]; v; v = v->next)
            nv++;

         // Record valid vertex counts in the active field 
         if (nv > 2)
         {
            polygon->active = nv;
            nc++;
         }
         else
         {
            // Invalid hull: just free the heap 
            for (v = polygon->proxy->v[HULL_LEFT]; v; v = nextv)
            {
               nextv = v->next;
               v = NULL;
            }
            polygon->active = 0;
         }
      }
      return nc;
}


//==============================================================================
//
//==============================================================================
void BNCHBuilder::addLeft(polygon_node *p, NCHfloat x, NCHfloat z)
{
   BASSERTM(p,"bad param to addLeft");

   // DLM 6/16/08 - Crash proofing
   if (!p)
      return;

   vertex_node *nv;

   // Create a new vertex node and set its fields 
   nv = &mVNodes[mOnVNode++]; BASSERT( mOnVNode < cMaxVNodes );
   nv->x = x;
   nv->z = z;

   // Add vertex nv to the left end of the polygon's vertex list 
   nv->next = p->proxy->v[HULL_LEFT];

   // Update proxy->[HULL_LEFT] to point to nv 
   p->proxy->v[HULL_LEFT] = nv;
}


//==============================================================================
//
//==============================================================================
void BNCHBuilder::mergeLeft(polygon_node *p, polygon_node *q, polygon_node *list)
{
   BASSERTM(p&&q,"bad param to mergeLeft");

   polygon_node *target;

   // Label hull as a hole 
   q->proxy->hole = true;

   if (p->proxy != q->proxy)
   {
      // Assign p's vertex list to the left end of q's list 
      p->proxy->v[HULL_RIGHT]->next = q->proxy->v[HULL_LEFT];
      q->proxy->v[HULL_LEFT] = p->proxy->v[HULL_LEFT];

      // Redirect any p->proxy references to q->proxy 

      for (target = p->proxy; list; list = list->next)
      {
         if (list->proxy== target)
         {
            list->active = false;
            list->proxy = q->proxy;
         }
      }
   }
}


//==============================================================================
//
//==============================================================================
void BNCHBuilder::addRight(polygon_node *p, NCHfloat x, NCHfloat z)
{
   BASSERTM(p,"bad param to addRight");

   vertex_node *nv;

   // Create a new vertex node and set its fields 
   nv = &mVNodes[mOnVNode++]; BASSERT( mOnVNode < cMaxVNodes );
   nv->x = x;
   nv->z = z;
   nv->next = NULL;

   // Add vertex nv to the right end of the polygon's vertex list 
   p->proxy->v[HULL_RIGHT]->next = nv;

   // Update proxy->v[HULL_RIGHT] to point to nv 
   p->proxy->v[HULL_RIGHT] = nv;
}


//==============================================================================
//
//==============================================================================
void BNCHBuilder::mergeRight(polygon_node *p, polygon_node *q, polygon_node *list)
{
   BASSERTM(p&&q,"bad param to mergeRight");

   polygon_node *target;

   // Label hull as external 
   q->proxy->hole = false;

   if (p->proxy != q->proxy)
   {
      // Assign p's vertex list to the right end of q's list 
      q->proxy->v[HULL_RIGHT]->next = p->proxy->v[HULL_LEFT];
      q->proxy->v[HULL_RIGHT] = p->proxy->v[HULL_RIGHT];

      // Redirect any p->proxy references to q->proxy 
      for (target = p->proxy; list; list = list->next)
      {
         if (list->proxy== target)
         {
            list->active = false;
            list->proxy = q->proxy;
         }
      }
   }
}


//==============================================================================
//
//==============================================================================
void BNCHBuilder::addLocalMin(polygon_node **p, edge_node *edge, NCHfloat x, NCHfloat z)
{
   polygon_node *existing_min;
   vertex_node  *nv;

   existing_min = *p;

   *p = &mPNodes[mOnPNode++]; BASSERT( mOnPNode < cMaxPNodes );

   // Create a new vertex node and set its fields 
   nv = &mVNodes[mOnVNode++]; BASSERT( mOnVNode < cMaxVNodes );
   nv->x = x;
   nv->z = z;
   nv->next = NULL;

   // Initialise proxy to point to p itself 
   (*p)->proxy = (*p);
   (*p)->active = true;
   (*p)->next = existing_min;

   // Make v[HULL_LEFT] and v[HULL_RIGHT] point to new vertex nv 
   (*p)->v[HULL_LEFT] = nv;
   (*p)->v[HULL_RIGHT] = nv;

   // Assign polygon p to the edge 
   edge->outp[0] = *p;
}

//==============================================================================
//
//==============================================================================
void BNCHBuilder::addVertex(vertex_node **t, NCHfloat x, NCHfloat z)
{
   if (!(*t))
   {
      *t = &mVNodes[mOnVNode++]; BASSERT( mOnVNode < cMaxVNodes );
      (*t)->x = x;
      (*t)->z = z;
      (*t)->next = NULL;
   }
   else
      // Head further down the list 
      addVertex(&((*t)->next), x, z);
}

//==============================================================================
//
//==============================================================================
BNCHPolygon::BNCHPolygon() :
   mAlloc(0)
{
   reset();
}

//==============================================================================
//
//==============================================================================
void BNCHPolygon::dealloc()
{
   if (mAge != DEADMARK)
   {
      mAlloc--;
      BASSERTM(mAlloc>=0, "NCH polygon ref count busted in dealloc.");
   }
}

//==============================================================================
//
//==============================================================================
BNCHPolygon::~BNCHPolygon()
{
   reset();
}

//==============================================================================
//
//==============================================================================
void BNCHPolygon::reset()
{
   BASSERTM(mAlloc <= 0, "Resetting a NCHpolygon with an outstanding reference.");
   hull.setNumber(0);
   mAlloc = 0;
   mAge = DEADMARK;
}

//=============================================================================
// BNonconvexHull::operator =
//=============================================================================
BNCHPolygon &BNCHPolygon::operator =(const BNCHPolygon &h)
{
   hull.setNumber( h.hull.getNumber() );
   for( long j =0; j<hull.getNumber(); j++ )
   {
      long numPoints = h.hull[j].vertex.getNumber();
      hull[j].vertex.setNumber( numPoints );
      hull[j].hole = h.hull[j].hole;
      for( long i =0; i<numPoints; i++ )
      {
         hull[j].vertex[i] = h.hull[j].vertex[i];
      }
   }
   // copy the alloc/age even though they will need to change
   mAge   = h.mAge;
   mAlloc = h.mAlloc;

   return *this;
}


//==============================================================================
//
//==============================================================================
/*static*/ void BNCHBuilder::polygonMerge( const BNCHPolygon& subj, const BNCHPolygon& clip, BNCHPolygon& result)
{
   sb_tree       *sbtree = NULL;
   it_node       *it = NULL, *intersect;
   edge_node     *edge, *prev_edge, *next_edge, *succ_edge, *e0, *e1;
   edge_node     *aet = NULL;
   lmt_node      *lmt = NULL, *local_min;
   polygon_node  *out_poly = NULL, *p, *q, *poly, *npoly, *cf = NULL;
   vertex_node   *vtx, *nv;
   unsigned char            states[4][2];  // mjubran
   int            contributing, scanbeam = 0, sbt_entries = 0;
   int            vclass, bl, br, tl, tr;
   NCHfloat      *sbt = NULL, xb, px, zb, zt, dz, ix, iz;
   bool           search;

   // parity states
   states[3][0] = HULL_LEFT;
   states[3][1] = HULL_LEFT;

   mOnLNode = 0;
   mOnSBTree = 0;
   mOnITNode = 0;
   mOnSTNode = 0;
   mOnVNode = 0;
   mOnPNode = 0;
   mOnENode = 0;

   zt = 0;
   dz = 0;

   mLastZ = (NCHfloat)cMaximumFloat;
   // Build LMT 
   buildLMT(&lmt, &sbtree, &sbt_entries, subj, HULL_MAIN);
   buildLMT(&lmt, &sbtree, &sbt_entries, clip, HULL_CLIP);
   
   // Sanity.
   BASSERTM(sbtree, "sbtree failed to build.");

   // Build scanbeam table from scanbeam tree 
   //BMemoryStackFrame stackFrame;
   //sbt = (NCHfloat*)stackFrame.getMemory(sbt_entries * sizeof(NCHfloat));
   //sbt = (NCHfloat*)gPrimaryHeap.New(sbt_entries * sizeof(NCHfloat));
   mSBTBuffer.setNumber(sbt_entries);
   sbt = mSBTBuffer.getPtr();
   if(!sbt)
   {
      BFAIL("mem alloc error");
      return;
   }

   //NCHfloat* sbtStart = sbt;

   buildSBT(&scanbeam, sbt, sbtree);
   scanbeam = 0;
   resetSBT(&sbtree);

   local_min = lmt;

   // Process each scanbeam
   while (scanbeam < sbt_entries)
   {
      // Set zb and zt to the bottom and top of the scanbeam 
      zb = *sbt;
      sbt++;
      scanbeam++;
      if (scanbeam < sbt_entries)
      {
         zt = *sbt;
         dz = zt - zb;
      }

      //=== SCANBEAM BOUNDARY PROCESSING================================ 

      // If LMT node corresponding to zb exists
      if (local_min)
      {
         if (IDENTICAL_FLOAT(local_min->z, zb))
         {
            // Add edges starting at this local minimum to the AET 
            for (edge = local_min->first_bound; edge; edge = edge->ptrs[ENEXTBOUND])
               addEdgeToAET(&aet, edge, NULL);

            local_min = local_min->next;
         }
      }

      // Sanity.
      BASSERTM(aet, "aet failed to build.");

      // Set dummy previous x value 
      px = -FLT_MAX;

      // Create bundles within AET 
      e0 = aet;
      e1 = aet;

      // Set up bundle fields of first edge 
      aet->scratch[1][ aet->type] = !IDENTICAL_FLOAT(aet->vertex[2].z, zb);
      aet->scratch[1][1-aet->type] = false; // mjubran
      aet->scratch[3][0] = UNBUNDLED;

      for (next_edge = aet->ptrs[ENEXT]; next_edge; next_edge = next_edge->ptrs[ENEXT])
      {
		 bool fEqxb = EQ(e0->xb, next_edge->xb); // mjubran
         // Set up bundle fields of next edge 
         next_edge->scratch[1][ next_edge->type] = !IDENTICAL_FLOAT(next_edge->vertex[2].z, zb);
		 BASSERT(next_edge->type > 0 || next_edge->type < 1);
		 next_edge->scratch[1][1-next_edge->type] = false;
         next_edge->scratch[3][0] = UNBUNDLED;

         // Bundle edges above the scanbeam boundary if they coincide 
         if (next_edge->scratch[1][next_edge->type])
         {
            if ((!IDENTICAL_FLOAT(e0->vertex[2].z, zb)) &&  fEqxb && EQ(e0->dx, next_edge->dx))
            {
               next_edge->scratch[1][ next_edge->type] ^= e0->scratch[1][ next_edge->type];
			   next_edge->scratch[1][1-next_edge->type] = e0->scratch[1][1-next_edge->type]; // mjubran
               next_edge->scratch[3][0] = BUNDLE_HEAD;
               e0->scratch[1][HULL_CLIP] = false;
               e0->scratch[1][HULL_MAIN] = false;
               e0->scratch[3][0] = BUNDLE_TAIL;
            }
            e0 = next_edge;
         }
      }

      states[0][HULL_CLIP] = no_h_edge;
      states[0][HULL_MAIN] = no_h_edge;

	  static const bool lookupTbl[4][4] = { 0,0,0,0, 0,1,1,1, 0,1,1,1, 0,1,1,1 };
      // Process each edge at this scanbeam boundary 
      for (edge = aet; edge; edge = edge->ptrs[ENEXT])
      {
         states[2][HULL_CLIP] = edge->scratch[1][HULL_CLIP] + (edge->scratch[2][HULL_CLIP] << 1);
         states[2][HULL_MAIN] = edge->scratch[1][HULL_MAIN] + (edge->scratch[2][HULL_MAIN] << 1);

         if (states[2][HULL_CLIP] | states[2][HULL_MAIN])
         {
            // Set bundle side 
            edge->scratch[0][HULL_CLIP] = states[3][HULL_CLIP];
            edge->scratch[0][HULL_MAIN] = states[3][HULL_MAIN];

            // Determine contributing status and quadrant occupancies 
			contributing = lookupTbl[states[2][HULL_CLIP]][(!states[3][HULL_MAIN]) | states[0][HULL_MAIN]]         
			   | lookupTbl[states[2][HULL_MAIN]][(!states[3][HULL_CLIP]) | states[0][HULL_CLIP]]	   
			   | (lookupTbl[states[2][HULL_CLIP]][states[2][HULL_MAIN]]
               & (states[3][HULL_CLIP] == states[3][HULL_MAIN]));

#ifdef VERIFY
            contributingOld = (states[2][HULL_CLIP] && (!states[3][HULL_MAIN] || states[0][HULL_MAIN]))
               || (states[2][HULL_MAIN] && (!states[3][HULL_CLIP] || states[0][HULL_CLIP]))
               || (states[2][HULL_CLIP] && states[2][HULL_MAIN]
               && (states[3][HULL_CLIP]== states[3][HULL_MAIN]));
			   
			BASSERT(contributing == contributingOld);
#endif			   
               br = (states[3][HULL_CLIP]) | (states[3][HULL_MAIN]);
               bl = (states[3][HULL_CLIP] ^ edge->scratch[1][HULL_CLIP]) | (states[3][HULL_MAIN] ^ edge->scratch[1][HULL_MAIN]);
               tr = (states[3][HULL_CLIP] ^ (states[0][HULL_CLIP]!=no_h_edge)) | (states[3][HULL_MAIN] ^ (states[0][HULL_MAIN]!=no_h_edge));
               tl = (states[3][HULL_CLIP] ^ (states[0][HULL_CLIP]!=no_h_edge) ^ edge->scratch[2][HULL_CLIP]) | (states[3][HULL_MAIN] ^ (states[0][HULL_MAIN]!=no_h_edge) ^ edge->scratch[2][HULL_MAIN]);

               // Update parity
               states[3][HULL_CLIP] ^= edge->scratch[1][HULL_CLIP];
               states[3][HULL_MAIN] ^= edge->scratch[1][HULL_MAIN];

               // Update horizontal state 
               if (states[2][HULL_CLIP])         
                  states[0][HULL_CLIP] =(unsigned char)next_h_state[states[0][HULL_CLIP]][((states[2][HULL_CLIP] - 1) << 1) + states[3][HULL_CLIP]];
               if (states[2][HULL_MAIN])         
                  states[0][HULL_MAIN] =(unsigned char)next_h_state[states[0][HULL_MAIN]][((states[2][HULL_MAIN] - 1) << 1) + states[3][HULL_MAIN]];

               vclass = tr + (tl << 1) + (br << 2) + (bl << 3);

               if (contributing)
               {
                  xb = edge->xb;
                  
                  switch (vclass)
                  {
                  case LED:
                     if (IDENTICAL_FLOAT(edge->vertex[1].z, zb))
                        addLeft(edge->outp[1], xb, zb);
                     edge->outp[0] = edge->outp[1];
                     px = xb;
                     break;
                  case RED:
                     if (IDENTICAL_FLOAT(edge->vertex[1].z, zb))
                        addRight(edge->outp[1], xb, zb);
                     edge->outp[0] = edge->outp[1];
                     px = xb;
                     break;
                  case EMN:
                  case IMN:
                     addLocalMin(&out_poly, edge, xb, zb);
                     px = xb;
                     cf = edge->outp[0];
                     break;
                  case ERI:
                     if (!IDENTICAL_FLOAT(xb, px))
                     {
                        addRight(cf, xb, zb);
                        px = xb;
                     }
                     edge->outp[0] = cf;
                     cf = NULL;
                     break;
                  case ELI:
                     addLeft(edge->outp[1], xb, zb);
                     px = xb;
                     cf = edge->outp[1];
                     break;
                  case EMX:
                     if (!IDENTICAL_FLOAT(xb, px))
                     {
                        addLeft(cf, xb, zb);
                        px = xb;
                     }
                     mergeRight(cf, edge->outp[1], out_poly);
                     cf = NULL;
                     break;
                  case ILI:
                     if (!IDENTICAL_FLOAT(xb, px))
                     {
                        addLeft(cf, xb, zb);
                        px = xb;
                     }
                     edge->outp[0] = cf;
                     cf = NULL;
                     break;
                  case IRI:
                     addRight(edge->outp[1], xb, zb);
                     px = xb;
                     cf = edge->outp[1];
                     edge->outp[1] = NULL;
                     break;
                  case IMX:
                     if (!IDENTICAL_FLOAT(xb, px))
                     {
                        addRight(cf, xb, zb);
                        px = xb;
                     }
                     mergeLeft(cf, edge->outp[1], out_poly);
                     cf = NULL;
                     edge->outp[1] = NULL;
                     break;
                  case IMM:
                     if (!IDENTICAL_FLOAT(xb, px))
                     {
                        addRight(cf, xb, zb);
                        px = xb;
                     }
                     mergeLeft(cf, edge->outp[1], out_poly);
                     edge->outp[1] = NULL;
                     addLocalMin(&out_poly, edge, xb, zb);
                     cf = edge->outp[0];
                     break;
                  case EMM:
                     if (!IDENTICAL_FLOAT(xb, px))
                     {
                        addLeft(cf, xb, zb);
                        px = xb;
                     }
                     mergeRight(cf, edge->outp[1], out_poly);
                     edge->outp[1] = NULL;
                     addLocalMin(&out_poly, edge, xb, zb);
                     cf = edge->outp[0];
                     break;
                  default:
                     break;
                  } // End of switch 
               } // End of contributing conditional 
         } // End of edge states[2] conditional 
      } // End of AET loop 

      // Delete terminating edges from the AET, otherwise compute xt 
      for (edge = aet; edge; edge = edge->ptrs[ENEXT])
      {
         if (IDENTICAL_FLOAT(edge->vertex[2].z, zb))
         {
            prev_edge = edge->ptrs[EPREV];
            next_edge = edge->ptrs[ENEXT];
            if (prev_edge)
               prev_edge->ptrs[ENEXT] = next_edge;
            else
               aet = next_edge;
            if (next_edge)
               next_edge->ptrs[EPREV] = prev_edge;

            // Copy bundle head state to the adjacent tail edge if required 
            if ((edge->scratch[3][1]== BUNDLE_HEAD) && prev_edge)
            {
               if (prev_edge->scratch[3][1]== BUNDLE_TAIL)
               {
                  prev_edge->outp[1] = edge->outp[1];
                  prev_edge->scratch[3][1] = UNBUNDLED;
                  if (prev_edge->ptrs[EPREV])
                     if (prev_edge->ptrs[EPREV]->scratch[3][1]== BUNDLE_TAIL)
                        prev_edge->scratch[3][1] = BUNDLE_HEAD;
               }
            }
         }
         else
         {
            if (IDENTICAL_FLOAT(edge->vertex[2].z, zt))
               edge->xt = edge->vertex[2].x;
            else
               edge->xt = edge->vertex[1].x + edge->dx * (zt - edge->vertex[1].z);
         }
      }

      if (scanbeam < sbt_entries)
      {
         //=== SCANBEAM INTERIOR PROCESSING============================== 

         buildIntersectionTable(&it, aet, dz);

         // Process each node in the intersection table 
         for (intersect = it; intersect; intersect = intersect->next)
         {
            e0 = intersect->ie[0];
            e1 = intersect->ie[1];

            // Only generate output for contributing intersections 
            if ((e0->scratch[1][HULL_CLIP] || e0->scratch[1][HULL_MAIN]) && (e1->scratch[1][HULL_CLIP] || e1->scratch[1][HULL_MAIN]))
            {
               p = e0->outp[0];
               q = e1->outp[0];
               ix = intersect->point.x;
               iz = intersect->point.z + zb;

               states[1][HULL_CLIP] = ( e0->scratch[1][HULL_CLIP] && !e0->scratch[0][HULL_CLIP]) | ( e1->scratch[1][HULL_CLIP] &&  e1->scratch[0][HULL_CLIP]) | (!e0->scratch[1][HULL_CLIP] && !e1->scratch[1][HULL_CLIP] && e0->scratch[0][HULL_CLIP] && e1->scratch[0][HULL_CLIP]);
               states[1][HULL_MAIN] = ( e0->scratch[1][HULL_MAIN] && !e0->scratch[0][HULL_MAIN]) | ( e1->scratch[1][HULL_MAIN] &&  e1->scratch[0][HULL_MAIN]) | (!e0->scratch[1][HULL_MAIN] && !e1->scratch[1][HULL_MAIN] && e0->scratch[0][HULL_MAIN] && e1->scratch[0][HULL_MAIN]);

               // Determine quadrant occupancies 
               tr = (states[1][HULL_CLIP]) | (states[1][HULL_MAIN]);
               tl = (states[1][HULL_CLIP] ^ e1->scratch[1][HULL_CLIP]) | (states[1][HULL_MAIN] ^ e1->scratch[1][HULL_MAIN]);
               br = (states[1][HULL_CLIP] ^ e0->scratch[1][HULL_CLIP]) | (states[1][HULL_MAIN] ^ e0->scratch[1][HULL_MAIN]);
               bl = (states[1][HULL_CLIP] ^ e1->scratch[1][HULL_CLIP] ^ e0->scratch[1][HULL_CLIP]) | (states[1][HULL_MAIN] ^ e1->scratch[1][HULL_MAIN] ^ e0->scratch[1][HULL_MAIN]);

               vclass = tr + (tl << 1) + (br << 2) + (bl << 3);

               switch (vclass)
               {
               case EMN:
                  addLocalMin(&out_poly, e0, ix, iz);
                  e1->outp[0] = e0->outp[0];
                  break;
               case ERI:
                  if (p)
                  {
                     addRight(p, ix, iz);
                     e1->outp[0] = p;
                     e0->outp[0] = NULL;
                  }
                  break;
               case ELI:
                  if (q)
                  {
                     addLeft(q, ix, iz);
                     e0->outp[0] = q;
                     e1->outp[0] = NULL;
                  }
                  break;
               case EMX:
                  if (p && q)
                  {
                     addLeft(p, ix, iz);
                     mergeRight(p, q, out_poly);
                     e0->outp[0] = NULL;
                     e1->outp[0] = NULL;
                  }
                  break;
               case IMN:
                  addLocalMin(&out_poly, e0, ix, iz);
                  e1->outp[0] = e0->outp[0];
                  break;
               case ILI:
                  if (p)
                  {
                     addLeft(p, ix, iz);
                     e1->outp[0] = p;
                     e0->outp[0] = NULL;
                  }
                  break;
               case IRI:
                  if (q)
                  {
                     addRight(q, ix, iz);
                     e0->outp[0] = q;
                     e1->outp[0] = NULL;
                  }
                  break;
               case IMX:
                  if (p && q)
                  {
                     addRight(p, ix, iz);
                     mergeLeft(p, q, out_poly);
                     e0->outp[0] = NULL;
                     e1->outp[0] = NULL;
                  }
                  break;
               case IMM:
                  if (p && q)
                  {
                     addRight(p, ix, iz);
                     mergeLeft(p, q, out_poly);
                     addLocalMin(&out_poly, e0, ix, iz);
                     e1->outp[0] = e0->outp[0];
                  }
                  break;
               case EMM:
                  if (p && q)
                  {
                     addLeft(p, ix, iz);
                     mergeRight(p, q, out_poly);
                     addLocalMin(&out_poly, e0, ix, iz);
                     e1->outp[0] = e0->outp[0];
                  }
                  break;
               default:
                  break;
               } // End of switch 
            } // End of contributing intersection conditional 

            // Swap bundle sides states[1] response to edge crossing 
            if (e0->scratch[1][HULL_CLIP])
               e1->scratch[0][HULL_CLIP] = !e1->scratch[0][HULL_CLIP];
            if (e1->scratch[1][HULL_CLIP])
               e0->scratch[0][HULL_CLIP] = !e0->scratch[0][HULL_CLIP];
            if (e0->scratch[1][HULL_MAIN])
               e1->scratch[0][HULL_MAIN] = !e1->scratch[0][HULL_MAIN];
            if (e1->scratch[1][HULL_MAIN])
               e0->scratch[0][HULL_MAIN] = !e0->scratch[0][HULL_MAIN];

            // Swap e0 and e1 bundles in the AET 
            prev_edge = e0->ptrs[EPREV];
            next_edge = e1->ptrs[ENEXT];
            if (next_edge)
               next_edge->ptrs[EPREV] = e0;

            if (e0->scratch[3][0] == BUNDLE_HEAD)
            {
               search = true;
               while (search)
               {
                  prev_edge = prev_edge->ptrs[EPREV];
                  if (prev_edge)
                  {
                     if (prev_edge->scratch[3][0] != BUNDLE_TAIL)
                        search = false;
                  }
                  else
                     search = false;
               }
            }
            if (!prev_edge)
            {
               BASSERTM(aet,"null aet");
               aet->ptrs[EPREV] = e1;
               e1->ptrs[ENEXT] = aet;
               aet = e0->ptrs[ENEXT];
            }
            else
            {
               prev_edge->ptrs[ENEXT]->ptrs[EPREV] = e1;
               e1->ptrs[ENEXT] = prev_edge->ptrs[ENEXT];
               prev_edge->ptrs[ENEXT] = e0->ptrs[ENEXT];
            }
            e0->ptrs[ENEXT]->ptrs[EPREV] = prev_edge;
            e1->ptrs[ENEXT]->ptrs[EPREV] = e1;
            e0->ptrs[ENEXT] = next_edge;
         } // End of IT loop

         // Prepare for next scanbeam 
         for (edge = aet; edge; edge = next_edge)
         {
            next_edge = edge->ptrs[ENEXT];
            succ_edge = edge->ptrs[ESUCC];

            if (IDENTICAL_FLOAT(edge->vertex[2].z, zt) && succ_edge)
            {
               // Replace AET edge by its successor 
               succ_edge->outp[1] = edge->outp[0];
               succ_edge->scratch[3][1] = edge->scratch[3][0];
               succ_edge->scratch[2][HULL_CLIP] = edge->scratch[1][HULL_CLIP];
               succ_edge->scratch[2][HULL_MAIN] = edge->scratch[1][HULL_MAIN];
               prev_edge = edge->ptrs[EPREV];
               if (prev_edge)
                  prev_edge->ptrs[ENEXT] = succ_edge;
               else
                  aet = succ_edge;
               if (next_edge)
                  next_edge->ptrs[EPREV] = succ_edge;
               succ_edge->ptrs[EPREV] = prev_edge;
               succ_edge->ptrs[ENEXT] = next_edge;
            }
            else
            {
               // Update this edge 
               edge->outp[1] = edge->outp[0];
               edge->scratch[3][1] = edge->scratch[3][0];
               edge->scratch[2][HULL_CLIP] = edge->scratch[1][HULL_CLIP];
               edge->scratch[2][HULL_MAIN] = edge->scratch[1][HULL_MAIN];
               edge->xb = edge->xt;
            }
            edge->outp[0] = NULL;
         }
      }
   } //=== END OF SCANBEAM PROCESSING================================== 

   // Generate result polygon from out_poly 
   long nHulls = countHulls(out_poly);
   if (nHulls > cMaxHullsPerNch)
      cMaxHullsPerNch = nHulls;
   result.hull.setNumber( nHulls );

   if (countHulls(out_poly))
   if (result.hull.getNumber() > 0)
   {
      BNCHVertexList *hull = result.hull.getData();
      for (poly = out_poly; poly; poly = npoly)
      {
         npoly = poly->next;
         if (poly->active)
         {
            if (poly->active > cMaxVerticesPerNch)
               cMaxVerticesPerNch = poly->active;
            hull->vertex.setNumber( poly->active );
            hull->hole = poly->hole;
            
            BNCHVector *vert = &hull->vertex[poly->active - 1];
            for (vtx = poly->proxy->v[HULL_LEFT]; vtx; vtx = nv)
            {
               nv = vtx->next;
               
               vert->x = (float)vtx->x;
               vert->z = (float)vtx->z;

               vert--;
            }
            hull++;
         }
      }
   }

   // Tidy up 
   resetIT(&it);
   resetLMT(&lmt);
   
   //gPrimaryHeap.Delete(sbtStart);
}

void BNonconvexHull::dumpStats()
{
   gConsole.output(cChannelSim, "overlapsHull:           %d", moverlapsHull);
   gConsole.output(cChannelSim, "isInside:               %d", misInside);
   gConsole.output(cChannelSim, "insideConvexHull:       %d", mInsideConvexHull);
   gConsole.output(cChannelSim, "inside:                 %d", minside);
   gConsole.output(cChannelSim, "segmentIntersects:      %d", msegmentIntersects);
   gConsole.output(cChannelSim, "findclosestpointonhull: %d", mfindclosestpointonhull);
   gConsole.output(cChannelSim, "initialize:             %d", minitialize);
   gConsole.output(cChannelSim, "cache hits:             %d", cHits);
   gConsole.output(cChannelSim, "cache misses:           %d", cMisses);
   gConsole.output(cChannelSim, "high water:             %d", cHighWater);
   gConsole.output(cChannelSim, "Max Hulls Per Nch:      %d", cMaxHullsPerNch);
   gConsole.output(cChannelSim, "Max Vertices Per Nch:   %d", cMaxVerticesPerNch);
}
//==============================================================================
// eof: nonconvexhull2.cpp
//==============================================================================
