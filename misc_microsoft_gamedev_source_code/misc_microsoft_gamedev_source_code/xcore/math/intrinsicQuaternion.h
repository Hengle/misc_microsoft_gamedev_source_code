//==============================================================================
// Copyright (c) 1997-2005 Ensemble Studios
//
// Quaternion class
//==============================================================================

#if !defined(_INTRINSICQUATERNION_H_)
#define _INTRINSICQUATERNION_H_

//==============================================================================
//
//==============================================================================

__declspec(intrin_type) _DECLSPEC_ALIGN_16_  class BIntrinsicQuaternion : public XMVECTOR
{
   public:
      XMFINLINE BIntrinsicQuaternion(void);
	   XMFINLINE BIntrinsicQuaternion(const XMVECTOR &v);
	   XMFINLINE BIntrinsicQuaternion(const float &fX, const float &fY, const float &fZ, const float &fW = 1.0f);
	   XMFINLINE BIntrinsicQuaternion(const BVector &vect, const float &fAngle);	                           // Creation from Axis/Angle information
	   XMFINLINE BIntrinsicQuaternion(const BVector &vEuler);				                                    // Creation from Euler angles
      XMFINLINE BIntrinsicQuaternion(const BMatrix &matrix);                                                // Creation from Rotation Matrix
      XMFINLINE BIntrinsicQuaternion(const BVector &dir, const BVector &up, const BVector &right);          // Creation from dir/up/right
      
      XMFINLINE void set(const BVector &dir, const BVector &up, const BVector &right);                      // Creation from dir/up/right
      XMFINLINE void set(const BMatrix &matrix);

      XMFINLINE void makeIdentity(void);                                                                    // Set to identity

	   XMFINLINE void toAxisAngle(BVector *pvAxis, float *pfTheta) const;                                    // Convert to Axis Angle
      XMFINLINE void toMatrix(BMatrix &matrix) const;                                                       // Convert to Matrix
      XMFINLINE void toOrient(BVector *pvDir, BVector *pvUp, BVector *pvRight) const;                       // Convert to Orientation Vectors

	   XMFINLINE void normalize(void);

	   XMFINLINE BIntrinsicQuaternion operator +(const BIntrinsicQuaternion &q) const;    // Addition
	   XMFINLINE BIntrinsicQuaternion operator -(const BIntrinsicQuaternion &q) const;    // Subtraction
	   XMFINLINE BIntrinsicQuaternion operator *(const BIntrinsicQuaternion &q) const;    // Multiply
      XMFINLINE BIntrinsicQuaternion operator *(const float &fC) const;                  // Mult by a constant
      XMFINLINE bool operator ==(const BIntrinsicQuaternion &q) const;                    // comparison
      XMFINLINE BIntrinsicQuaternion slerp(const BIntrinsicQuaternion &qTo, const float &fT) const;              // slerp from this quat to To quat given T      
      XMFINLINE void slerp(const BIntrinsicQuaternion &qTo, const float &fT, BIntrinsicQuaternion &result) const; // slerp from this quat to To quat given T      

      static XMFINLINE BIntrinsicQuaternion squad(const BIntrinsicQuaternion &qP, const BIntrinsicQuaternion &qA, const BIntrinsicQuaternion &qB, const BIntrinsicQuaternion &qQ, const float &fT);
      XMFINLINE BIntrinsicQuaternion inverse(void) const;                    // Inverse of the quat.  Assumes it's a unit quat.
      XMFINLINE BIntrinsicQuaternion log(void) const;                        // log
      XMFINLINE BIntrinsicQuaternion exp(void) const;                        // exp
      XMFINLINE float dot(const BIntrinsicQuaternion &q) const;              // dot product
      XMFINLINE float norm(void) const;                                      // Return the quat's norm

      XMFINLINE float getX(void) const;
      XMFINLINE float getY(void) const;
      XMFINLINE float getZ(void) const;
      XMFINLINE float getW(void) const;

      
};

#include "intrinsicquaternion.inl"

#endif // _INTRINSICQUATERNION_H_


//==============================================================================
// eof: intrinsicquaternion.h
//==============================================================================