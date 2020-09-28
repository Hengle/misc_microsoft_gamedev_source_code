//==============================================================================
// matrix.inl
//
// XBOX matrix implementation.  Based on the D3DX library.
//
// Copyright (c) 2002, Ensemble Studios
//==============================================================================

// Includes
#include "physics.h"
#include "physicsquat.h"
#include "math\vector.h"

//============================================================================
// BPhysicsMatrix::makeIdentity
//============================================================================
inline void BPhysicsMatrix::makeIdentity( void )
{
   D3DXMatrixIdentity(this);
}

//============================================================================
// BPhysicsMatrix::isIdentity
//============================================================================
inline BOOL BPhysicsMatrix::isIdentity( void ) const
{
   return D3DXMatrixIsIdentity(this);
}

//============================================================================
// BPhysicsMatrix::fDeterminant
//============================================================================
inline float BPhysicsMatrix::determinant( void ) const
{
   return D3DXMatrixDeterminant(this);
}

//============================================================================
// BPhysicsMatrix::transpose
//============================================================================
inline void BPhysicsMatrix::transpose(const BPhysicsMatrix &tm)
{
   D3DXMatrixTranspose(this, &tm);
}

//============================================================================
// BPhysicsMatrix::transposeRotation
//============================================================================
inline void BPhysicsMatrix::transposeRotation()
{
   bswap(_21, _12);
   bswap(_31, _13);
   bswap(_32, _23);
}

//============================================================================
// BPhysicsMatrix
//============================================================================
inline bool BPhysicsMatrix::invert( void )
{
   if (D3DXMatrixInverse(this, NULL, this))
      return (true);
   else
      return (false);
}

//============================================================================
// BPhysicsMatrix::makeZero
//============================================================================
inline void BPhysicsMatrix::makeZero( void )
{
    memset(this, 0, sizeof(BPhysicsMatrix));
}

//============================================================================
// BPhysicsMatrix::makeScale
//============================================================================
inline void BPhysicsMatrix::makeScale( const BVector& scale )
{
   D3DXMatrixScaling(this, scale.x, scale.y, scale.z);
}

//============================================================================
// BPhysicsMatrix::makeScale
//============================================================================
inline void BPhysicsMatrix::makeScale( float xScale, float yScale, float zScale )
{
   D3DXMatrixScaling(this, xScale, yScale, zScale);
}

//============================================================================
// BPhysicsMatrix::makeScale
//============================================================================
inline void BPhysicsMatrix::makeScale( float xyzScale )
{
   D3DXMatrixScaling(this, xyzScale, xyzScale, xyzScale);
}

//============================================================================
// BPhysicsMatrix::makeTranslation
//============================================================================
inline void BPhysicsMatrix::makeTranslation( const BVector& translation )
{
   D3DXMatrixTranslation(this, translation.x, translation.y, translation.z);
}

//============================================================================
// BPhysicsMatrix::makeTranslation
//============================================================================
inline void BPhysicsMatrix::makeTranslation( float xTranslation, float yTranslation, float zTranslation )
{
   D3DXMatrixTranslation(this, xTranslation, yTranslation, zTranslation);
}

//============================================================================
// BPhysicsMatrix::makeTranslation
//============================================================================
inline void BPhysicsMatrix::makeTranslation( float xyzTranslation )
{
   D3DXMatrixTranslation(this, xyzTranslation, xyzTranslation, xyzTranslation);
}


//============================================================================
// BPhysicsMatrix::makeRotationX
//============================================================================
inline void BPhysicsMatrix::makeRotationX( float rads )
{
   D3DXMatrixRotationX(this, rads);
}

//============================================================================
// BPhysicsMatrix::makeRotationX
//============================================================================
inline void BPhysicsMatrix::makeRotationY( float rads )
{
   D3DXMatrixRotationY(this, rads);
}

//============================================================================
// BPhysicsMatrix::makeRotationX
//============================================================================
inline void BPhysicsMatrix::makeRotationZ( float rads )
{
   D3DXMatrixRotationZ(this, rads);
}

//============================================================================
// BPhysicsMatrix::makeRotateX
// Age3 Code base compliance
//============================================================================
inline void BPhysicsMatrix::makeRotateX( float rads )
{
   makeRotationX(rads);
}

//============================================================================
// BPhysicsMatrix::makeRotateY
// Age3 Code base compliance
//============================================================================
inline void BPhysicsMatrix::makeRotateY( float rads )
{
   makeRotationY(rads);
}

//============================================================================
// BPhysicsMatrix::makeRotateZ
// Age3 Code base compliance
//============================================================================
inline void BPhysicsMatrix::makeRotateZ( float rads )
{
   makeRotationZ(rads);
}

//============================================================================
// BPhysicsMatrix::makeRotationAxis
//============================================================================
inline void BPhysicsMatrix::makeRotationAxis( const BVector &axis, float rads )
{
   D3DXMatrixRotationAxis(this, (D3DXVECTOR3*)&axis, rads);
}

//============================================================================
// BPhysicsMatrix::makeRotateArbitrary
//============================================================================
inline void BPhysicsMatrix::makeRotateArbitrary(float rads, const BVector& axis)
{
   makeRotationAxis(axis, rads);
}

//============================================================================
// BPhysicsMatrix::makeRotationQuat
//============================================================================
inline void BPhysicsMatrix::makeRotationQuat( const BPhysicsQuat &quat )
{
   D3DXMatrixRotationQuaternion(this, &quat);
}

//============================================================================
// BPhysicsMatrix::makeRotationXYZ
//============================================================================
inline void BPhysicsMatrix::makeRotationXYZ(float xRads, float yRads, float zRads)
{
   D3DXMatrixRotationYawPitchRoll(this, yRads, xRads, zRads);
}

//============================================================================
// BPhysicsMatrix::makeLookAt  (left handed only)
//============================================================================
inline void BPhysicsMatrix::makeLookAt( const BVector &eye, const BVector &at, const BVector &up )
{
   D3DXMatrixLookAtLH(this, (D3DXVECTOR3*)&eye, (D3DXVECTOR3*)&at, (D3DXVECTOR3*)&up);
}

//============================================================================
// BPhysicsMatrix::makePerspectiveProjection (left handed only)
//============================================================================
inline void BPhysicsMatrix::makePerspectiveProjection(float fov, float aspect, float nearZ, float farZ)
{
   D3DXMatrixPerspectiveFovLH(this, fov, aspect, nearZ, farZ);
}

//============================================================================
// BPhysicsMatrix::makeReflect
// This is an adapted funtion based on the one in the core Matrix class.
//============================================================================
inline void BPhysicsMatrix::makeReflect(const BVector& point, const BVector& normal)
{
/*
   //-- Function is adapted from the BPhysicsMatrix code.  The BPhysicsMatrix from core relates to our 
   //-- Matrix in the following way.
   
 	d3dmatrix._11 = BPhysicsMatrix.m[0][0]; d3dmatrix._12 = BPhysicsMatrix.m[1][0]; d3dmatrix._13 = BPhysicsMatrix.m[2][0]; d3dmatrix._14 = 0;
   d3dmatrix._21 = BPhysicsMatrix.m[0][1]; d3dmatrix._22 = BPhysicsMatrix.m[1][1]; d3dmatrix._23 = BPhysicsMatrix.m[2][1]; d3dmatrix._24 = 0;
   d3dmatrix._31 = BPhysicsMatrix.m[0][2]; d3dmatrix._32 = BPhysicsMatrix.m[1][2]; d3dmatrix._33 = BPhysicsMatrix.m[2][2]; d3dmatrix._34 = 0;
   d3dmatrix._41 = BPhysicsMatrix.m[0][3]; d3dmatrix._42 = BPhysicsMatrix.m[1][3]; d3dmatrix._43 = BPhysicsMatrix.m[2][3]; d3dmatrix._44 = 1;
   
   //-- original code from the core BPhysicsMatrix function
   float a=normal.x;
   float b=normal.y;
   float c=normal.z;

   m[0][0] = 1.0f-2.0f*a*a;
   m[0][1] = -2.0f*a*b;
   m[0][2] = -2.0f*a*c;
   m[0][3] = -point.x*m[0][0]-point.y*m[0][1]-point.z*m[0][2]+point.x;

   m[1][0] = -2.0f*a*b;
   m[1][1] = 1.0f-2.0f*b*b;
   m[1][2] = -2.0f*b*c;
   m[1][3] = -point.x*m[1][0]-point.y*m[1][1]-point.z*m[1][2]+point.y;

   m[2][0] = -2.0f*a*c;
   m[2][1] = -2.0f*b*c;
   m[2][2] = 1.0f-2.0f*c*c;
   m[2][3] = -point.x*m[2][0]-point.y*m[2][1]-point.z*m[2][2]+point.z;
 */

   float a=normal.x;
   float b=normal.y;
   float c=normal.z;

   _11 =  1.0f-2.0f*a*a;
   _21 = -2.0f*a*b;
   _31 = -2.0f*a*c;
   _41 = -point.x*_11-point.y*_21-point.z*_31+point.x;

   _12 = -2.0f*a*b;
   _22 =  1.0f-2.0f*b*b;
   _32 = -2.0f*b*c;
   _42 = -point.x*_12-point.y*_22-point.z*_32+point.y;

   _13 = -2.0f*a*c;
   _23 = -2.0f*b*c;
   _33 =  1.0f-2.0f*c*c;
   _43 = -point.x*_13-point.y*_23-point.z*_33+point.z;

   _14 = 0.0f;
   _24 = 0.0f;
   _34 = 0.0f;
   _44 = 1.0f;     
}


//============================================================================
// BPhysicsMatrix::makeViewportScaling  
//============================================================================
inline void BPhysicsMatrix::makeViewportScaling(float x, float y, float width, float height, float minZ, float maxZ)
{
   _11 = width / 2.0f;
   _12 = 0.0f;
   _13 = 0.0f;
   _14 = 0.0f;
   
   _21 = 0.0f;
   _22 = -height / 2.0f;
   _23 = 0.0f;
   _24 = 0.0f;
   
   _31 = 0.0f;
   _32 = 0.0f;
   _33 = maxZ - minZ;
   _34 = 0.0f;

   _41 = x + width / 2.0f;
   _42 = height / 2.0f + y;
   _43 = minZ;
   _44 = 1.0f;

}

//============================================================================
// BPhysicsMatrix::makeTransposeRotation  
//============================================================================
inline void BPhysicsMatrix::makeTransposeRotation(const BPhysicsMatrix& r)
{
   _11 = r._11;
   _12 = r._21;
   _13 = r._31;
   _14 = 0.0f;

   _21 = r._12;
   _22 = r._22;
   _23 = r._32;
   _24 = 0.0f;

   _31 = r._13;
   _32 = r._23;
   _33 = r._33;
   _34 = 0.0f;

   _41 = 0.0f;
   _42 = 0.0f;
   _43 = 0.0f;
   _44 = 1.0f;
}

//============================================================================
// BPhysicsMatrix::multScale  
//============================================================================
inline void BPhysicsMatrix::multScale(const BVector& scale)
{
   mTempMatrix1.makeScale(scale.x, scale.y, scale.z);
   *this *= mTempMatrix1;
}

//============================================================================
// BPhysicsMatrix::multScale  
//============================================================================
inline void BPhysicsMatrix::multScale( float scale )
{
   mTempMatrix1.makeScale(scale);
   *this *= mTempMatrix1;
}

//============================================================================
// BPhysicsMatrix::multScale  
//============================================================================
inline void BPhysicsMatrix::multScale(float xScale, float yScale, float zScale)
{
   mTempMatrix1.makeScale(xScale, yScale, zScale);
   *this *= mTempMatrix1;
}

//=============================================================================
// BPhysicsMatrix::multRotationX
//=============================================================================
void BPhysicsMatrix::multRotationX(float rads)
{
   mTempMatrix3.makeRotationX(rads);
   *this *= mTempMatrix3 ;
}

//=============================================================================
// BPhysicsMatrix::multRotationY
//=============================================================================
void BPhysicsMatrix::multRotationY(float rads)
{
   mTempMatrix4.makeRotationY(rads);
   *this *=  mTempMatrix4 ;
}

//=============================================================================
// BPhysicsMatrix::multRotationZ
//=============================================================================
void BPhysicsMatrix::multRotationZ(float rads)
{
   mTempMatrix5.makeRotationZ(rads);
   *this *= mTempMatrix5 ;
}

//=============================================================================
// BPhysicsMatrix::multRotationAxis
//=============================================================================
void BPhysicsMatrix::multRotationAxis(const BVector &axis, float rads)
{
   mTempMatrix3.makeRotationAxis(axis, rads);
   *this *= mTempMatrix3 ;
}

//=============================================================================
// BPhysicsMatrix::multOrientation
//=============================================================================
void BPhysicsMatrix::multOrientation(const BVector& right, const BVector& up,  const BVector& forward)
{
   mTempMatrix5.set(right, up, forward, cOriginVector);
   *this *= mTempMatrix5;
}

//============================================================================
// BPhysicsMatrix::multTranslate  
//============================================================================
inline void BPhysicsMatrix::multTranslate( const BVector& translation )
{
   mTempMatrix2.makeTranslation(translation.x, translation.y, translation.z);
   *this *= mTempMatrix2;
}

//============================================================================
// BPhysicsMatrix::multTranslate  
//============================================================================
inline void BPhysicsMatrix::multTranslate( float xTrans, float yTrans, float zTrans )
{
   mTempMatrix2.makeTranslation(xTrans, yTrans, zTrans);
   *this *= mTempMatrix2;
}

//============================================================================
// BPhysicsMatrix::multTranslate  
//============================================================================
inline void BPhysicsMatrix::multTranslate( float translation )
{
   mTempMatrix2.makeTranslation(translation);
   *this *= mTempMatrix2;
}


//============================================================================
// BPhysicsMatrix::transformCoord
//============================================================================
inline BVector& BPhysicsMatrix::transformPoint(const BVector& vector) const
{
   D3DXVec3TransformCoord((D3DXVECTOR3*)&mTempVector3_2, (D3DXVECTOR3*)&vector, this);
   return mTempVector3_2;
}

//============================================================================
// BPhysicsMatrix::transformCoord
//============================================================================
inline void BPhysicsMatrix::transformPoint(const BVector& vector, BVector& result) const
{
   D3DXVec3TransformCoord((D3DXVECTOR3*)&result, (D3DXVECTOR3*)&vector, this);
}

//============================================================================
// BPhysicsMatrix::transformVectorAsPoint
// Age 3 compatibility
//============================================================================
inline void BPhysicsMatrix::transformVectorAsPoint(const BVector& vector, BVector& result) const
{
   transformPoint(vector, result);
}

//============================================================================
// BPhysicsMatrix::transformNormal
//============================================================================
inline BVector& BPhysicsMatrix::transformVector(const BVector& vector) const
{
   D3DXVec3TransformNormal((D3DXVECTOR3*)&mTempVector3_3, (D3DXVECTOR3*)&vector, this);
   return mTempVector3_3;
}

//============================================================================
// BPhysicsMatrix::transformNormal
//============================================================================
inline void BPhysicsMatrix::transformVector(const BVector& vector, BVector& result) const
{
   D3DXVec3TransformNormal((D3DXVECTOR3*)&result, (D3DXVECTOR3*)&vector, this);
}



//============================================================================
// BPhysicsMatrix::set
// NOTE: This assumes that the passed in float array is a 16 float representation
// of a matrix.
//============================================================================
inline void BPhysicsMatrix::set(const float* pTransform)
{
   if (!pTransform)
      return;

   _11 = pTransform[0];
   _12 = pTransform[1];
   _13 = pTransform[2];
   _14 = pTransform[3];

   _21 = pTransform[4];
   _22 = pTransform[5];
   _23 = pTransform[6];
   _24 = pTransform[7];

   _31 = pTransform[8];
   _32 = pTransform[9];
   _33 = pTransform[10];
   _34 = pTransform[11];
   
   _41 = pTransform[12];
   _42 = pTransform[13];
   _43 = pTransform[14];
   _44 = pTransform[15];   
}

//============================================================================
// BPhysicsMatrix::set
//============================================================================
inline void BPhysicsMatrix::set(const BVector& right, const BVector& up, const BVector& forward, const BVector& translation)
{
   _14 = 0.0f;
   _24 = 0.0f;
   _34 = 0.0f;
   _44 = 1.0f;
   // makeIdentity();
   setRight(right);
   setUp(up);
   setForward(forward);
   setTranslation(translation);
}

//============================================================================
// BPhysicsMatrix::setTranslation
//============================================================================
inline void BPhysicsMatrix::setTranslation(const BVector& v)
{   
   DEBUG_MATRIX_CHECK(BASSERT(*((long *)&_14) != INVALID_FLOAT));
   _41 = v.x;   
   _42 = v.y;         
   _43 = v.z;
}

//============================================================================
// BPhysicsMatrix::setTranslation
//============================================================================
inline void BPhysicsMatrix::setTranslation(float x, float y, float z)
{
   DEBUG_MATRIX_CHECK(BASSERT(*((long *)&_14) != INVALID_FLOAT));
   _41 = x;   
   _42 = y;         
   _43 = z;
}

//============================================================================
// BPhysicsMatrix::clearTranslation
//============================================================================
inline void BPhysicsMatrix::clearTranslation()
{
   setTranslation(0.0f, 0.0f, 0.0f);
}

//============================================================================
// BPhysicsMatrix::setForward
//============================================================================
inline void BPhysicsMatrix::setForward(const BVector& v)
{
   DEBUG_MATRIX_CHECK(BASSERT(*((long *)&_14) != INVALID_FLOAT));
   _31 = v.x;
   _32 = v.y;
   _33 = v.z;
}

//============================================================================
// BPhysicsMatrix::setUp
//============================================================================
inline void BPhysicsMatrix::setUp(const BVector& v)
{   
   DEBUG_MATRIX_CHECK(BASSERT(*((long *)&_14) != INVALID_FLOAT));
   _21 = v.x;
   _22 = v.y;
   _23 = v.z;
}

//============================================================================
// BPhysicsMatrix::setRight
//============================================================================
inline void BPhysicsMatrix::setRight(const BVector& v)
{
   DEBUG_MATRIX_CHECK(BASSERT(*((long *)&_14) != INVALID_FLOAT));
   _11 = v.x;
   _12 = v.y;
   _13 = v.z;
}

//============================================================================
// BPhysicsMatrix::getTranslation
//============================================================================
inline const BVector& BPhysicsMatrix::getTranslation(BVector& v) const
{
   v.x = _41;
   v.y = _42;
   v.z = _43;

   return v;
}

//============================================================================
// BPhysicsMatrix::getForward
//============================================================================
inline const BVector& BPhysicsMatrix::getForward(BVector& v) const
{
   v.x = _31;
   v.y = _32;
   v.z = _33;

   return v;
}

//============================================================================
// BPhysicsMatrix::getUp
//============================================================================
inline const BVector& BPhysicsMatrix::getUp(BVector& v) const
{
   v.x = _21;
   v.y = _22;
   v.z = _23;

   return v;
}

//============================================================================
// BPhysicsMatrix::getRight
//============================================================================
inline const BVector& BPhysicsMatrix::getRight(BVector& v) const
{
   v.x = _11;
   v.y = _12;
   v.z = _13;

   return v;
}


//============================================================================
// BPhysicsMatrix::getTranslation
//============================================================================
inline const BVector& BPhysicsMatrix::getTranslation(void) const
{
 return *((BVector*) &_41);
}

//============================================================================
// BPhysicsMatrix::getForward
//============================================================================
inline const BVector& BPhysicsMatrix::getForward(void) const
{
 return *((BVector*) &_31);
}

//============================================================================
// BPhysicsMatrix::getUp
//============================================================================
inline const BVector& BPhysicsMatrix::getUp(void) const
{
    return *((BVector*) &_21);
}

//============================================================================
// BPhysicsMatrix::getRight
//============================================================================
inline const BVector& BPhysicsMatrix::getRight(void) const
{
   return *((BVector*) &_11);
}

//============================================================================
// BPhysicsMatrix::getAsFloatBuffer
// This function assumes that the passed in float* pointer is a pointer to
// an array of 16 floats.
//============================================================================
inline void BPhysicsMatrix::getAsFloatBuffer(float* pTransform) const
{
   if (!pTransform)
      return;
   
   pTransform[0] = _11;
   pTransform[1] = _12;
   pTransform[2] = _13;
   pTransform[3] = _14;
                  
   pTransform[4] = _21;
   pTransform[5] = _22;
   pTransform[6] = _23;
   pTransform[7] = _24;

   pTransform[8] = _31;
   pTransform[9] = _32;
   pTransform[10]= _33;
   pTransform[11]= _34;
                  
   pTransform[12]= _41;
   pTransform[13]= _42;
   pTransform[14]= _43;
   pTransform[15]= _44;   
}


//============================================================================
//  BPhysicsMatrix::getQuaternion
//============================================================================
inline void BPhysicsMatrix::getQuaternion(BPhysicsQuat &quat) const
{
   D3DXQuaternionRotationMatrix(&quat, this);
}

//============================================================================
//  BPhysicsMatrix::normalize
//============================================================================
inline void  BPhysicsMatrix::normalize() const
{
   ((BVector*) &_11)->normalize();
   ((BVector*) &_21)->normalize();
   ((BVector*) &_31)->normalize();
}


