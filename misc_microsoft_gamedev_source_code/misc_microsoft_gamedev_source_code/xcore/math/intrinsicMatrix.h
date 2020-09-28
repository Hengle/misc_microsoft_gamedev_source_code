//==============================================================================
// intrinsicmatrix.h
//
// Copyright (c) 2005 Ensemble Studios
//==============================================================================


#ifndef _INTRINSIC_MATRIX_
#define _INTRINSIC_MATRIX_

//==============================================================================
// 
//==============================================================================
_DECLSPEC_ALIGN_16_ class BIntrinsicMatrix : public XMMATRIX
{
   public:

      // Constructors
      XMFINLINE BIntrinsicMatrix(void);
      XMFINLINE BIntrinsicMatrix(const XMMATRIX m);

      // Operators
      XMFINLINE BVector operator*(const BVector vec) const;
      XMFINLINE long operator==(const BIntrinsicMatrix &mat) const;
      XMFINLINE long operator!=(const BIntrinsicMatrix &mat) const;

      // Accessors
      XMFINLINE BVector getRow(const int row) const;
      XMFINLINE float getElement(const int row, const int column) const;
      XMFINLINE void setElement(const int row, const int column, const float value);
      XMFINLINE void clearTranslation();
      XMFINLINE void clearOrientation();
      XMFINLINE void setTranslation(const BVector v);
      XMFINLINE void setTranslation(float x, float y, float z);
      XMFINLINE void getTranslation(BVector &v) const;
      XMFINLINE void getForward(BVector &v) const;
      XMFINLINE void getUp(BVector &v) const;
      XMFINLINE void getRight(BVector &v) const;
      XMFINLINE void getScale(float& x, float& y, float& z) const;

      // Conversion
      XMFINLINE void setD3DXMatrix(const D3DMATRIX &d3dmatrix);
      XMFINLINE void getD3DXMatrix(D3DMATRIX &d3dmatrix) const;

      // Matrix Creation
      XMFINLINE void makeIdentity(void);
      XMFINLINE void makeZero(void);
      XMFINLINE void makeRotateX(float rads);
      XMFINLINE void makeRotateXSinCos(float sine, float cosine);
      XMFINLINE void makeRotateY(float rads);
      XMFINLINE void makeRotateYSinCos(float sine, float cosine);
      XMFINLINE void makeRotateZ(float rads);
      XMFINLINE void makeRotateZSinCos(float sine, float cosine);
      XMFINLINE void makeRotateArbitrary(const float rads, const BVector axis);
      XMFINLINE void makeRotateYawPitchRoll(float yaw, float pitch, float roll);
      XMFINLINE void makeScale(float xScale, float yScale, float zScale);
      XMFINLINE void makeScale(float scale);
      XMFINLINE void makeView(const BVector cameraLoc, const BVector cameraDir, const BVector cameraUp, const BVector cameraRight);
      XMFINLINE void makeInverseView(const BVector cameraLoc, const BVector cameraDir, const BVector cameraUp, const BVector cameraRight);
      XMFINLINE void makeOrient(const BVector dir, const BVector up, const BVector right);
      XMFINLINE void makeInverseOrient(const BVector dir, const BVector up, const BVector right);
      XMFINLINE void makeTranslate(const float tx, const float ty, const float tz);
      XMFINLINE void makeTranslate(const BVector v);
      XMFINLINE void makeReflect(const BVector point, const BVector normal);

      // Matrix Math
      XMFINLINE void multTranslate(float tx, float ty, float tz);
      XMFINLINE void multRotateX(float rads);
      XMFINLINE void multRotateXSinCos(float sine, float cosine);
      XMFINLINE void multRotateY(float rads);
      XMFINLINE void multRotateYSinCos(float sine, float cosine);
      XMFINLINE void multRotateZ(float rads);
      XMFINLINE void multRotateZSinCos(float sine, float cosine);
      XMFINLINE void multRotateArbitrary(const float rads, const BVector axis);
      XMFINLINE void multScale(float xScale, float yScale, float zScale);
      XMFINLINE void multScale(float factor);
      XMFINLINE void multOrient(const BVector dir, const BVector up, const BVector right);
      XMFINLINE void multInverseOrient(const BVector dir, const BVector up, const BVector right);
      XMFINLINE void multReflect(const BVector point, const BVector normal);
      XMFINLINE void mult(BIntrinsicMatrix matrix1, BIntrinsicMatrix matrix2);
      XMFINLINE bool invert(void);
      XMFINLINE void transposeRotation();
      XMFINLINE void transpose();

      // Vector transformations
      XMFINLINE void transformVector(const BVector vect, BVector &result) const;
      XMFINLINE void transformVectorAsPoint(const BVector vect, BVector &result) const;
      XMFINLINE void transformVectorAsPoint(const BVector vect, BVector4 &result) const;
      XMFINLINE void transformVectorAsPoint(const BVector2 vect, BVector &result) const;
      XMFINLINE void transformVectorListAsPoint(const BVector *vect, BVector4 *result, const long num) const;
      XMFINLINE void transformVectorListAsPoint(const BVector *vect, BVector *result, const long num) const;
      void transformShortVectorListAsPoint(const BShortVector *vect, BShortVector *result, const long num) const;
      void transformShortVectorListAsPoint(const BVector *vect, BShortVector *result, const long num) const;
      void transformShortVectorListAsPoint(const BShortVector *vect, BVector *result, const long num) const;
      void transformOverlappingVectorListAsPoint(const BVector *vect, BVector *result, const long num) const;
      void transformOverlappingShortVectorListAsPoint(const BShortVector *vect, BShortVector *result, const long num) const;
      XMFINLINE void transformVectorList(const BVector *pvect, BVector *presult, DWORD nvertices) const;
}; // BIntrinsicMatrix

#include "intrinsicmatrix.inl"

#endif // _INTRINSIC_MATRIX_

//==============================================================================
// eof: intrinsicmatrix.h
//==============================================================================
