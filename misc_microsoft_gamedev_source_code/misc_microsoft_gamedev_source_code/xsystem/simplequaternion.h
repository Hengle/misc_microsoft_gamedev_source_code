// Quaternion.h: interface for the BSimpleQuaternion class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(_SIMPLEQUATERNION_H_)
#define _SIMPLEQUATERNION_H_

//#define DEGREES_TO_RADIANS(a)		(a * 0.017453f)

class BChunkReader;
class BChunkWriter;

class BSimpleQuaternion  
{
   public:
	   BSimpleQuaternion();
	   BSimpleQuaternion(float fX, float fY, float fZ, float fW = 1.0f);
	   BSimpleQuaternion(const BVector &vect, float fAngle);	// Creation from Axis/Angle information
	   BSimpleQuaternion(const BVector &vEuler);				   // Creation from Euler angles
      BSimpleQuaternion(const BMatrix &matrix);             // Creation from Rotation Matrix
      BSimpleQuaternion(const BVector &dir, const BVector &up, const BVector &right);             // Creation from dir/up/right
      
      void set        (const BVector &dir, const BVector &up, const BVector &right);             // Creation from dir/up/right
      void set        (const BMatrix &matrix);


      void makeIdentity( void ) { mQuaternion.x =0.0f; mQuaternion.y = 0.0f; mQuaternion.z = 0.0f; mQuaternion.w = 1.0f; }
	   void toAxisAngle(BVector *pvAxis, float *pfTheta) const;  // Convert to Axis Angle
      void toMatrix(BMatrix &matrix)const;             // Convert to Matrix
      void toOrient(BVector *pvDir, BVector *pvUp, BVector *pvRight)const;// Convert to Orientation Vectors

	   void normalize();

      BSimpleQuaternion& operator =(const BSimpleQuaternion &);
	   BSimpleQuaternion operator +(const BSimpleQuaternion &) const;    // Addition
	   BSimpleQuaternion operator -(const BSimpleQuaternion &) const;    // Subtraction
	   BSimpleQuaternion operator *(const BSimpleQuaternion &) const;    // Multiply
      BSimpleQuaternion operator *(float fC) const;   // Mult by a constant
      bool operator ==(const BSimpleQuaternion &) const;    // comparison
      BSimpleQuaternion slerp(const BSimpleQuaternion &qTo, float fT) const;  // slerp from this quat to To quat given T      
      void slerp(const BSimpleQuaternion &qTo, float fT, BSimpleQuaternion &result) const;  // slerp from this quat to To quat given T      

      static BSimpleQuaternion squad(const BSimpleQuaternion &qP, const BSimpleQuaternion &qA, const BSimpleQuaternion &qB, const BSimpleQuaternion &qQ, float fT);
      BSimpleQuaternion inverse() const;                    // Inverse of the quat.  Assumes it's a unit quat.
      BSimpleQuaternion log() const;                        // log
      BSimpleQuaternion exp() const;                        // exp
      float dot(const BSimpleQuaternion &q) const;    // dot product
      float norm() const;                       // Return the quat's norm

	   virtual ~BSimpleQuaternion();
      
      bool load(BChunkReader* pChunkReader);
      bool save(BChunkWriter* pChunkWriter) const;

      float getX(void) const { return mQuaternion.x; }
      float getY(void) const { return mQuaternion.y; }
      float getZ(void) const { return mQuaternion.z; }
      float getW(void) const { return mQuaternion.w; }

      D3DXQUATERNION          mQuaternion;
/*
      
	   float x;									// Vector Portions of Quat.
	   float y;
	   float z;
	   float w;									// Scalar portion of Quat.
*/

};

#endif // _SIMPLEQUATERNION_H_
