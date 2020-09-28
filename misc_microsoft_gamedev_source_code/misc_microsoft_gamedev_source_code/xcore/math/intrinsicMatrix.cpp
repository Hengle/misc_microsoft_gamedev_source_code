//=============================================================================
// intrinsicmatrix.cpp
//
// Copyright (c) 2005 Ensemble Studios
//=============================================================================

#include "xcore.h"
#include "matrix.h"

//=============================================================================
// BIntrinsicMatrix::transformShortVectorListAsPoint
//=============================================================================
void BIntrinsicMatrix::transformShortVectorListAsPoint(const BVector *vect, BShortVector *result, const long num) const
{
   // SLB: optimize me
   for(long i=num-1; i>=0; i--, vect++, result++)
   {
      result->mx = m[0][0]*vect->x + m[1][0]*vect->y + m[2][0]*vect->z + m[3][0];
      result->my = m[0][1]*vect->x + m[1][1]*vect->y + m[2][1]*vect->z + m[3][1];
      result->mz = m[0][2]*vect->x + m[1][2]*vect->y + m[2][2]*vect->z + m[3][2];
   }
}

//=============================================================================
// BIntrinsicMatrix::transformShortVectorListAsPoint
//=============================================================================
void BIntrinsicMatrix::transformShortVectorListAsPoint(const BShortVector *vect, BShortVector *result, const long num) const
{
   // SLB: optimize me
   for(long i=num-1; i>=0; i--, vect++, result++)
   {
      result->mx = m[0][0]*vect->mx.asFloat() + m[1][0]*vect->my.asFloat() + m[2][0]*vect->mz.asFloat() + m[3][0];
      result->my = m[0][1]*vect->mx.asFloat() + m[1][1]*vect->my.asFloat() + m[2][1]*vect->mz.asFloat() + m[3][1];
      result->mz = m[0][2]*vect->mx.asFloat() + m[1][2]*vect->my.asFloat() + m[2][2]*vect->mz.asFloat() + m[3][2];
   }
}

//=============================================================================
// BIntrinsicMatrix::transformShortVectorListAsPoint
//=============================================================================
void BIntrinsicMatrix::transformShortVectorListAsPoint(const BShortVector *vect, BVector *result, const long num) const
{
   // SLB: optimize me
   for(long i=num-1; i>=0; i--, vect++, result++)
   {
      result->x = m[0][0]*vect->mx.asFloat() + m[1][0]*vect->my.asFloat() + m[2][0]*vect->mz.asFloat() + m[3][0];
      result->y = m[0][1]*vect->mx.asFloat() + m[1][1]*vect->my.asFloat() + m[2][1]*vect->mz.asFloat() + m[3][1];
      result->z = m[0][2]*vect->mx.asFloat() + m[1][2]*vect->my.asFloat() + m[2][2]*vect->mz.asFloat() + m[3][2];
   }
}

//=============================================================================
// BIntrinsicMatrix::transformOverlappingVectorListAsPoint
//
// This version is safe when vect==result.
//=============================================================================
void BIntrinsicMatrix::transformOverlappingVectorListAsPoint(const BVector *vect, BVector *result, const long num) const
{
   // SLB: optimize me
   float x,y,z;
   for(long i=num-1; i>=0; i--, vect++, result++)
   {
      x=vect->x;
      y=vect->y;
      z=vect->z;

      result->x = m[0][0]*x + m[1][0]*y + m[2][0]*z + m[3][0];
      result->y = m[0][1]*x + m[1][1]*y + m[2][1]*z + m[3][1];
      result->z = m[0][2]*x + m[1][2]*y + m[2][2]*z + m[3][2];
   }
}

//=============================================================================
// BIntrinsicMatrix::transformOverlappingShortVectorListAsPoint
//
// This version is safe when vect==result.
//=============================================================================
void BIntrinsicMatrix::transformOverlappingShortVectorListAsPoint(const BShortVector *vect, BShortVector *result, const long num) const
{
   // SLB: optimize me
   float x,y,z;
   for(long i=num-1; i>=0; i--, vect++, result++)
   {
      x=vect->mx.asFloat();
      y=vect->my.asFloat();
      z=vect->mz.asFloat();

      result->mx = m[0][0]*x + m[1][0]*y + m[2][0]*z + m[3][0];
      result->my = m[0][1]*x + m[1][1]*y + m[2][1]*z + m[3][1];
      result->mz = m[0][2]*x + m[1][2]*y + m[2][2]*z + m[3][2];
   }
}

//=============================================================================
// eof: intrinsicmatrix.cpp
//=============================================================================
