// Quaternion.cpp: implementation of the BSimpleQuaternion class.
//
//////////////////////////////////////////////////////////////////////

#include "xsystem.h"
#include "chunker.h"
#include "Quaternion.h"
#include "math\Vector.h"

//////////////////////////////////////////////////////////////////////
// Default Constructor
//////////////////////////////////////////////////////////////////////
BSimpleQuaternion::BSimpleQuaternion() : mQuaternion()
{
}


//////////////////////////////////////////////////////////////////////
// Constructor with floats
//////////////////////////////////////////////////////////////////////
BSimpleQuaternion::BSimpleQuaternion(float fX, float fY, float fZ, float fW) : mQuaternion(fX,fY,fZ,fW)
{

}

//////////////////////////////////////////////////////////////////////
// Axis Angle Constructor
//////////////////////////////////////////////////////////////////////
BSimpleQuaternion::BSimpleQuaternion(const BVector &vectAxis, float fTheta)
{
   D3DXVECTOR3 vect;
   vect.x = vectAxis.x;
   vect.y = vectAxis.y;
   vect.z = vectAxis.z;

   D3DXQuaternionRotationAxis(&mQuaternion, &vect, fTheta);
}

//////////////////////////////////////////////////////////////////////
// Constructor with dir/up/right
//////////////////////////////////////////////////////////////////////
BSimpleQuaternion::BSimpleQuaternion(const BVector &dir, const BVector &up, const BVector &right)
{
   set(dir, up, right);
}

void BSimpleQuaternion::set(const BVector &dir, const BVector &up, const BVector &right)
{
   D3DXMATRIX matrix;

   matrix._11 = right.x; matrix._12 = right.y;  matrix._13 = right.z; matrix._14 = 0;
   matrix._21 = up.x;    matrix._22 = up.y;     matrix._23 = up.z; matrix._24 = 0;
   matrix._31 = dir.x;   matrix._32 = dir.y;    matrix._33 = dir.z; matrix._34 = 0;
   matrix._41 = 0; matrix._42 = 0;  matrix._43 = 0; matrix._44 = 1;

   D3DXQuaternionRotationMatrix(&mQuaternion, &matrix);
}

void BSimpleQuaternion::set(const BMatrix &matrix)
{
   D3DXMATRIX d3dMatrix;
   matrix.getD3DXMatrix(d3dMatrix);
   D3DXQuaternionRotationMatrix(&mQuaternion, &d3dMatrix);
}


//////////////////////////////////////////////////////////////////////
// Euler Angle Constructor
//////////////////////////////////////////////////////////////////////
BSimpleQuaternion::BSimpleQuaternion(const BVector &vEuler)
{
   D3DXQuaternionRotationYawPitchRoll(&mQuaternion, vEuler.x, vEuler.y, vEuler.z);
}

//////////////////////////////////////////////////////////////////////
// Rotation Matrix Constructor
//////////////////////////////////////////////////////////////////////
BSimpleQuaternion::BSimpleQuaternion(const BMatrix &matrix)
{
   D3DXMATRIX d3dMatrix;
   matrix.getD3DXMatrix(d3dMatrix);
   D3DXQuaternionRotationMatrix(&mQuaternion, &d3dMatrix);
}



//////////////////////////////////////////////////////////////////////
// Destructor
//////////////////////////////////////////////////////////////////////
BSimpleQuaternion::~BSimpleQuaternion()
{

}


BSimpleQuaternion& BSimpleQuaternion::operator =(const BSimpleQuaternion &q)
{
   mQuaternion = q.mQuaternion;
   return(*this);
}

BSimpleQuaternion  BSimpleQuaternion::operator *(const BSimpleQuaternion &qArg) const
{
	// This is defined as *this multiplied by the Arg
	BSimpleQuaternion qSolution;
   qSolution.mQuaternion = mQuaternion*qArg.mQuaternion;
	return qSolution;
}

void BSimpleQuaternion::normalize()
{
   D3DXQuaternionNormalize(&mQuaternion, &mQuaternion);
}

void BSimpleQuaternion::toAxisAngle(BVector *pvAxis, float *pfTheta) const
{
   D3DXVECTOR3 vect;
   D3DXQuaternionToAxisAngle(&mQuaternion, &vect, pfTheta);
   pvAxis->x = vect.x;
   pvAxis->y = vect.y;
   pvAxis->z = vect.z;
}
	

//////////////////////////////////////////////////////////////////////
// BSimpleQuaternion::toMatrix(BMatrix &matrix)
//////////////////////////////////////////////////////////////////////
void BSimpleQuaternion::toMatrix(BMatrix &matrix) const
{
   D3DXMATRIX d3dMatrix;

   D3DXMatrixRotationQuaternion(&d3dMatrix, &mQuaternion);
   matrix.setD3DXMatrix(d3dMatrix);
}


//////////////////////////////////////////////////////////////////////
// BSimpleQuaternion::operator ==
//////////////////////////////////////////////////////////////////////
bool BSimpleQuaternion::operator ==(const BSimpleQuaternion &qArg) const
{
   return (mQuaternion == qArg.mQuaternion?true:false);
}

//////////////////////////////////////////////////////////////////////
// BSimpleQuaternion::toOrient
//////////////////////////////////////////////////////////////////////
void BSimpleQuaternion::toOrient(BVector *pvDir, BVector *pvUp, BVector *pvRight) const
{
   BMatrix matrix;
   toMatrix(matrix);

   *pvRight = matrix.getRow(0);
   *pvUp = matrix.getRow(1);
   *pvDir = matrix.getRow(2);

   return;
}

//////////////////////////////////////////////////////////////////////
// BSimpleQuaternion::slerp
//////////////////////////////////////////////////////////////////////
BSimpleQuaternion BSimpleQuaternion::slerp(const BSimpleQuaternion &qTo, float fT) const
{
   BSimpleQuaternion result;   
   D3DXQuaternionSlerp(&result.mQuaternion, &mQuaternion, &qTo.mQuaternion, fT);
   return(result);
}

//////////////////////////////////////////////////////////////////////
// BSimpleQuaternion::slerp
//////////////////////////////////////////////////////////////////////
void BSimpleQuaternion::slerp(const BSimpleQuaternion &qTo, float fT, BSimpleQuaternion &result) const
{
   D3DXQuaternionSlerp(&result.mQuaternion, &mQuaternion, &qTo.mQuaternion, fT);
}

//////////////////////////////////////////////////////////////////////
// BSimpleQuaternion::inverse
//////////////////////////////////////////////////////////////////////
BSimpleQuaternion BSimpleQuaternion::inverse() const
{   
   BSimpleQuaternion result;
   D3DXQuaternionInverse(&result.mQuaternion, &mQuaternion);
   return(result);
}

   

//////////////////////////////////////////////////////////////////////
// BSimpleQuaternion::squad
// Calculate a quadratic interpolation between 4 quaternions.
//////////////////////////////////////////////////////////////////////
BSimpleQuaternion BSimpleQuaternion::squad(const BSimpleQuaternion &qP, const BSimpleQuaternion &qA,
                               const BSimpleQuaternion &qB, const BSimpleQuaternion &qQ, float fT)
{
   BSimpleQuaternion result;
   D3DXQuaternionSquad(&result.mQuaternion, &qP.mQuaternion, &qA.mQuaternion, &qB.mQuaternion, &qQ.mQuaternion, fT);
   return(result);
}

//////////////////////////////////////////////////////////////////////
// BSimpleQuaternion::log
// Calculate log of a quaternion
//////////////////////////////////////////////////////////////////////
BSimpleQuaternion BSimpleQuaternion::log() const
{
   BSimpleQuaternion result;
   D3DXQuaternionLn(&result.mQuaternion, &mQuaternion);
   return result;
}

//////////////////////////////////////////////////////////////////////
// BSimpleQuaternion::exp
// Calculate exp of a quaternion
//////////////////////////////////////////////////////////////////////
BSimpleQuaternion BSimpleQuaternion::exp () const
{
   BSimpleQuaternion result;
   D3DXQuaternionExp(&result.mQuaternion, &mQuaternion);
   return result;
}

//////////////////////////////////////////////////////////////////////
// BSimpleQuaternion::operator *(float fC)
// Mult the quaternion by a constant
//////////////////////////////////////////////////////////////////////
BSimpleQuaternion BSimpleQuaternion::operator *(float fC) const
{
   BSimpleQuaternion result;
   result.mQuaternion = mQuaternion*fC;
   return(result);
}

//////////////////////////////////////////////////////////////////////
// BSimpleQuaternion::operator +(const BSimpleQuaternion &)
// Addition
//////////////////////////////////////////////////////////////////////
BSimpleQuaternion BSimpleQuaternion::operator +(const BSimpleQuaternion& q) const
{
   BSimpleQuaternion result;
   result.mQuaternion = mQuaternion+q.mQuaternion;
   return(result);
}

//////////////////////////////////////////////////////////////////////
// BSimpleQuaternion::operator -(const BSimpleQuaternion &)
// Addition
//////////////////////////////////////////////////////////////////////
BSimpleQuaternion BSimpleQuaternion::operator -(const BSimpleQuaternion& q) const
{
   BSimpleQuaternion result;
   result.mQuaternion = mQuaternion-q.mQuaternion;
   return(result);
}

//////////////////////////////////////////////////////////////////////
// BSimpleQuaternion::dot
// Basically the sum of the squares
//////////////////////////////////////////////////////////////////////
float BSimpleQuaternion::dot(const BSimpleQuaternion& q) const
{
   return(D3DXQuaternionDot(&mQuaternion, &q.mQuaternion));
}

//////////////////////////////////////////////////////////////////////
// BSimpleQuaternion::norm
// Also Basically the sum of the squares
//////////////////////////////////////////////////////////////////////
float BSimpleQuaternion::norm()const
{
   return(D3DXQuaternionLengthSq(&mQuaternion));
}


//////////////////////////////////////////////////////////////////////
// BSimpleQuaternion::load
//////////////////////////////////////////////////////////////////////
bool BSimpleQuaternion::load(BChunkReader* pChunkReader)
{
   if (!pChunkReader)
      return false;

   long result;
   CHUNKREADSAFE(pChunkReader, Float, mQuaternion.x);
   CHUNKREADSAFE(pChunkReader, Float, mQuaternion.y);
   CHUNKREADSAFE(pChunkReader, Float, mQuaternion.z);
   CHUNKREADSAFE(pChunkReader, Float, mQuaternion.w);

   return true;
}

//////////////////////////////////////////////////////////////////////
// BSimpleQuaternion::save
//////////////////////////////////////////////////////////////////////
bool BSimpleQuaternion::save(BChunkWriter* pChunkWriter) const
{
   if (!pChunkWriter)
      return false;

   long result;
   CHUNKWRITESAFE(pChunkWriter, Float, mQuaternion.x);
   CHUNKWRITESAFE(pChunkWriter, Float, mQuaternion.y);
   CHUNKWRITESAFE(pChunkWriter, Float, mQuaternion.z);
   CHUNKWRITESAFE(pChunkWriter, Float, mQuaternion.w);

   return true;
}

