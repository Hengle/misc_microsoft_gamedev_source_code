//==============================================================================
// matrix.h
//
// XBOX matrix implementation.  Based on the XG library.
//
// Copyright (c) 2002, Ensemble Studios
//==============================================================================

#ifndef _PHYSICS_MATRIX
#define _PHYSICS_MATRIX

//#include "math\vector.h"
//#include "math\quat.h"
//-- Forward declarations
class BPhysicsQuat;
class hkRotation;

#ifdef BUILD_DEBUG
   //This hex value is an invalid float on the current system.  There is a chance
   //that it could be a valid float on other systems, in which case we could get a false
   //assert.  If that happens this number needs to be updated.
   #define INVALID_FLOAT 0x0012f6cc
   //This check tests to make sure the matrix has been initialized before calling setPos 
   //or setRight, etc.  Those functions do not set the 4th column, so the values were being
   //left uninitialized.  This test does not add any data variables, which is illegal in 
   //BPhysicsMatrix since we rely on it being a specific size.
#define DEBUG_MATRIX_CHECK(exp) exp
#else
   #define DEBUG_MATRIX_CHECK(exp) {}
#endif

//==============================================================================
// BPhysicsMatrix
//==============================================================================
__declspec(align(16)) class BPhysicsMatrix : public D3DXMATRIX
{
public:
   //--
   //-- Constructors
   //--
                     BPhysicsMatrix() : D3DXMATRIX()                             { DEBUG_MATRIX_CHECK(*((long *)(&_14)) = INVALID_FLOAT;) };
                     BPhysicsMatrix( const float *v )   : D3DXMATRIX ( v )       {};
                     BPhysicsMatrix( CONST D3DMATRIX& ref ): D3DXMATRIX( ref )   {};
                     
                     BPhysicsMatrix(    float _11, float _12, float _13, float _14,
                        float _21, float _22, float _23, float _24,
                        float _31, float _32, float _33, float _34,
                        float _41, float _42, float _43, float _44 ) : 
                        D3DXMATRIX( _11, _12, _13, _14, _21, _22, _23, _24,_31, _32, _33, _34, _41, _42, _43, _44 )  {};

                     BPhysicsMatrix( const BPhysicsQuat &quat )
                     {
                        makeRotationQuat(quat);
                     }; 


   //--
   //-- Identity
   //--
   inline void          makeIdentity( void ); 

   //-- using BOOL here for performance (to align with Direct3D BOOL usage)
   inline BOOL          isIdentity( void ) const;

   //--
   //-- Determinant
   //--
   inline float         determinant( void ) const;

   //--
   //-- Inverse
   //--
   inline bool          invert( void );   

   //-- 
   //-- Transposition
   //--
   inline void          transpose( const BPhysicsMatrix &tm );
   inline void          transposeRotation();

   //-- Initializers
   inline void          makeZero ( void );
   inline void          makeScale( const BVector& scale );
   inline void          makeScale( float xScale, float yScale, float zScale );
   inline void          makeScale( float xyzScale );
   inline void          makeTranslation( const BVector& translation );
   inline void          makeTranslation( float xTranslation, float yTranslation, float zTranslation );
   inline void          makeTranslation( float xyzTranslation );

   inline void          makeRotationX( float rads );
   inline void          makeRotationY( float rads );
   inline void          makeRotationZ( float rads );
   inline void          makeRotateX( float rads );   
   inline void          makeRotateY( float rads );   
   inline void          makeRotateZ( float rads );    
   inline void          makeRotationAxis( const BVector &axis, float rads );
   inline void          makeRotationQuat( const BPhysicsQuat &quat );
   inline void          makeRotationXYZ(float xRads, float yRads, float zRads);
   inline void          makeRotateArbitrary(float rads, const BVector& axis);
   inline void          makeLookAt( const BVector &eye, const BVector &at, const BVector &up ); // For left handed coordinate system
   inline void          makePerspectiveProjection(float fov, float aspect, float nearZ, float farZ); // For left handed coordinate system
   inline void          makeReflect(const BVector& point, const BVector& normal);
   inline void          makeViewportScaling(float x, float y, float width, float height, float minZ = 0.0f, float maxZ = 1.0f);
   inline void          makeTransposeRotation(const BPhysicsMatrix& r);

   //-- Multiplication
   inline void          multScale(float scale);
   inline void          multScale(const BVector& scale);
   inline void          multScale(float xScale, float yScale, float zScale);

   inline void          multRotationX(float rads);
   inline void          multRotationY(float rads);
   inline void          multRotationZ(float rads);
   inline void          multRotationAxis(const BVector &axis, float rads);
   inline void          multOrientation(const BVector& right, const BVector& up,  const BVector& forward);

   inline void          multTranslate( const BVector& translation );
   inline void          multTranslate( float translation );
   inline void          multTranslate( float xTrans, float yTrans, float zTrans );
   
   // Transformation
   inline BVector&     transformPoint(const BVector& vector) const;
   inline void          transformPoint(const BVector& vector, BVector& result) const;
   inline void          transformVectorAsPoint(const BVector& vector, BVector& result) const;

   inline BVector&     transformVector(const BVector& vector) const;
   inline void          transformVector(const BVector& vector, BVector& result) const;

 
   inline void          set           (const float* pTransform);
   inline void          set           (const BVector& right, const BVector& up, const BVector& forward, const BVector& translation);
   inline void          setTranslation(const BVector& v);
   inline void          setTranslation(float x, float y, float z);
   inline void          clearTranslation();
   inline void          setForward    (const BVector& v);
   inline void          setUp         (const BVector& v);
   inline void          setRight      (const BVector& v);

   inline const BVector&          getTranslation  (BVector& v) const;
   inline const BVector&          getForward      (BVector& v) const;
   inline const BVector&          getUp           (BVector& v) const;
   inline const BVector&          getRight        (BVector& v) const;
   inline const BVector&          getTranslation  (void) const;
   inline const BVector&          getForward      (void) const;
   inline const BVector&          getUp           (void) const;
   inline const BVector&          getRight        (void) const;
   inline void                     getAsFloatBuffer(float* pTransform) const;
   inline void                     getQuaternion(BPhysicsQuat &quat) const;

   inline void                     normalize() const;
   // Operators
   //inline BVector operator*(const BVector& vector) const;

   //-- WARNING:: DO NOT ADD DATA MEMBERS TO THIS CLASS
   //-- This class must maintain data consistency with the Direct 3D expected matrix format
protected:

   //-- static members are ok to add
   static BPhysicsMatrix       mTempMatrix1;        //-- used for all scaling operators only
   static BPhysicsMatrix       mTempMatrix2;        //-- used for all addition operators only
   static BPhysicsMatrix       mTempMatrix3;        //-- used for X rotation operators only
   static BPhysicsMatrix       mTempMatrix4;        //-- used for Y rotation operators only
   static BPhysicsMatrix       mTempMatrix5;        //-- used for Z rotation operators only
   static BVector      mTempVector3_1;
   static BVector      mTempVector3_2;
   static BVector      mTempVector3_3;
   
};

//============================================================================
// Matrix constants
//============================================================================
extern const __declspec(selectany) BPhysicsMatrix  cIdentityMatrix(1.0f, 0.0f, 0.0f, 0.0f,
                                                                   0.0f, 1.0f, 0.0f, 0.0f,
                                                                   0.0f, 0.0f, 1.0f, 0.0f,
                                                                   0.0f, 0.0f, 0.0f, 1.0f);

//============================================================================
// INLINE FUNCTIONS
//============================================================================
#include "physicsmatrix.inl"

#endif
