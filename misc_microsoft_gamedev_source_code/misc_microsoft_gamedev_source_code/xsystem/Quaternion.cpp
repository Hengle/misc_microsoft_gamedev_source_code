// Quaternion.cpp: implementation of the BQuaternion class.
//
//////////////////////////////////////////////////////////////////////

#include "xsystem.h"
#include "chunker.h"
#include "Quaternion.h"
#include "math\Vector.h"

//////////////////////////////////////////////////////////////////////
// Default Constructor
//////////////////////////////////////////////////////////////////////
BQuaternion::BQuaternion() : mQuaternion()
{
}


//////////////////////////////////////////////////////////////////////
// Constructor with floats
//////////////////////////////////////////////////////////////////////
BQuaternion::BQuaternion(float fX, float fY, float fZ, float fW) : mQuaternion(fX,fY,fZ,fW)
{

}

//////////////////////////////////////////////////////////////////////
// Axis Angle Constructor
//////////////////////////////////////////////////////////////////////
BQuaternion::BQuaternion(BVector &vectAxis, float fTheta)
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
BQuaternion::BQuaternion(const BVector &dir, const BVector &up, const BVector &right)
{
   set(dir, up, right);
}

void BQuaternion::set(BVector &dir, BVector &up, BVector &right)
{
   D3DXMATRIX matrix;

   matrix._11 = right.x; matrix._12 = right.y;  matrix._13 = right.z; matrix._14 = 0;
   matrix._21 = up.x;    matrix._22 = up.y;     matrix._23 = up.z; matrix._24 = 0;
   matrix._31 = dir.x;   matrix._32 = dir.y;    matrix._33 = dir.z; matrix._34 = 0;
   matrix._41 = 0; matrix._42 = 0;  matrix._43 = 0; matrix._44 = 1;

   D3DXQuaternionRotationMatrix(&mQuaternion, &matrix);
}

void BQuaternion::set(const BVector &dir, const BVector &up, const BVector &right)
{
   D3DXMATRIX matrix;

   matrix._11 = right.x; matrix._12 = right.y;  matrix._13 = right.z; matrix._14 = 0;
   matrix._21 = up.x;    matrix._22 = up.y;     matrix._23 = up.z; matrix._24 = 0;
   matrix._31 = dir.x;   matrix._32 = dir.y;    matrix._33 = dir.z; matrix._34 = 0;
   matrix._41 = 0; matrix._42 = 0;  matrix._43 = 0; matrix._44 = 1;

   D3DXQuaternionRotationMatrix(&mQuaternion, &matrix);
}

void BQuaternion::set(const BMatrix &matrix)
{
   D3DXMATRIX d3dMatrix;
   matrix.getD3DXMatrix(d3dMatrix);
   D3DXQuaternionRotationMatrix(&mQuaternion, &d3dMatrix);
}


//////////////////////////////////////////////////////////////////////
// Euler Angle Constructor
//////////////////////////////////////////////////////////////////////
BQuaternion::BQuaternion(BVector &vEuler)
{
   D3DXQuaternionRotationYawPitchRoll(&mQuaternion, vEuler.x, vEuler.y, vEuler.z);
}

//////////////////////////////////////////////////////////////////////
// Rotation Matrix Constructor
//////////////////////////////////////////////////////////////////////
BQuaternion::BQuaternion(BMatrix &matrix)
{
   D3DXMATRIX d3dMatrix;
   matrix.getD3DXMatrix(d3dMatrix);
   D3DXQuaternionRotationMatrix(&mQuaternion, &d3dMatrix);
}



//////////////////////////////////////////////////////////////////////
// Destructor
//////////////////////////////////////////////////////////////////////
BQuaternion::~BQuaternion()
{

}


BQuaternion& BQuaternion::operator =(const BQuaternion &q)
{
   mQuaternion = q.mQuaternion;
   return(*this);
}

BQuaternion  BQuaternion::operator *(BQuaternion &qArg)
{
	// This is defined as *this multiplied by the Arg
	BQuaternion qSolution;
   qSolution.mQuaternion = mQuaternion*qArg.mQuaternion;
	return qSolution;
}

void BQuaternion::normalize()
{
   D3DXQuaternionNormalize(&mQuaternion, &mQuaternion);
}

void BQuaternion::toAxisAngle(BVector *pvAxis, float *pfTheta)
{
   D3DXVECTOR3 vect;
   D3DXQuaternionToAxisAngle(&mQuaternion, &vect, pfTheta);
   pvAxis->x = vect.x;
   pvAxis->y = vect.y;
   pvAxis->z = vect.z;
}
	

//////////////////////////////////////////////////////////////////////
// BQuaternion::toMatrix(BMatrix &matrix)
//////////////////////////////////////////////////////////////////////
void BQuaternion::toMatrix(BMatrix &matrix) const
{
   D3DXMATRIX d3dMatrix;

   D3DXMatrixRotationQuaternion(&d3dMatrix, &mQuaternion);
   matrix.setD3DXMatrix(d3dMatrix);
}


//////////////////////////////////////////////////////////////////////
// BQuaternion::operator ==
//////////////////////////////////////////////////////////////////////
bool BQuaternion::operator ==(const BQuaternion &qArg)
{
   return (mQuaternion == qArg.mQuaternion?true:false);
}

//////////////////////////////////////////////////////////////////////
// BQuaternion::toOrient
//////////////////////////////////////////////////////////////////////
void BQuaternion::toOrient(BVector *pvDir, BVector *pvUp, BVector *pvRight) const
{
   BMatrix matrix;
   toMatrix(matrix);

   *pvRight = matrix.getRow(0);
   *pvUp = matrix.getRow(1);
   *pvDir = matrix.getRow(2);

   return;
}

//////////////////////////////////////////////////////////////////////
// BQuaternion::slerp
//////////////////////////////////////////////////////////////////////
BQuaternion BQuaternion::slerp(const BQuaternion &qTo, float fT) const
{
   BQuaternion result;   
   D3DXQuaternionSlerp(&result.mQuaternion, &mQuaternion, &qTo.mQuaternion, fT);
   return(result);
}

//////////////////////////////////////////////////////////////////////
// BQuaternion::slerp
//////////////////////////////////////////////////////////////////////
void BQuaternion::slerp(const BQuaternion &qTo, float fT, BQuaternion &result) const
{
   D3DXQuaternionSlerp(&result.mQuaternion, &mQuaternion, &qTo.mQuaternion, fT);
}

//////////////////////////////////////////////////////////////////////
// BQuaternion::inverse
//////////////////////////////////////////////////////////////////////
BQuaternion BQuaternion::inverse()
{   
   BQuaternion result;
   D3DXQuaternionInverse(&result.mQuaternion, &mQuaternion);
   return(result);
}

   

//////////////////////////////////////////////////////////////////////
// BQuaternion::squad
// Calculate a quadratic interpolation between 4 quaternions.
//////////////////////////////////////////////////////////////////////
BQuaternion BQuaternion::squad(BQuaternion &qP, BQuaternion &qA,
                               BQuaternion &qB, BQuaternion &qQ, float fT)
{
   BQuaternion result;
   D3DXQuaternionSquad(&result.mQuaternion, &qP.mQuaternion, &qA.mQuaternion, &qB.mQuaternion, &qQ.mQuaternion, fT);
   return(result);
}

//////////////////////////////////////////////////////////////////////
// BQuaternion::log
// Calculate log of a quaternion
//////////////////////////////////////////////////////////////////////
BQuaternion BQuaternion::log()
{
   BQuaternion result;
   D3DXQuaternionLn(&result.mQuaternion, &mQuaternion);
   return result;
}

//////////////////////////////////////////////////////////////////////
// BQuaternion::exp
// Calculate exp of a quaternion
//////////////////////////////////////////////////////////////////////
BQuaternion BQuaternion::exp () 
{
   BQuaternion result;
   D3DXQuaternionExp(&result.mQuaternion, &mQuaternion);
   return result;
}

//////////////////////////////////////////////////////////////////////
// BQuaternion::operator *(float fC)
// Mult the quaternion by a constant
//////////////////////////////////////////////////////////////////////
BQuaternion BQuaternion::operator *(float fC) const
{
   BQuaternion result;
   result.mQuaternion = mQuaternion*fC;
   return(result);
}

//////////////////////////////////////////////////////////////////////
// BQuaternion::operator +(const BQuaternion &)
// Addition
//////////////////////////////////////////////////////////////////////
BQuaternion BQuaternion::operator +(const BQuaternion& q) const
{
   BQuaternion result;
   result.mQuaternion = mQuaternion+q.mQuaternion;
   return(result);
}

//////////////////////////////////////////////////////////////////////
// BQuaternion::operator -(const BQuaternion &)
// Addition
//////////////////////////////////////////////////////////////////////
BQuaternion BQuaternion::operator -(const BQuaternion& q) const
{
   BQuaternion result;
   result.mQuaternion = mQuaternion-q.mQuaternion;
   return(result);
}

//////////////////////////////////////////////////////////////////////
// BQuaternion::dot
// Basically the sum of the squares
//////////////////////////////////////////////////////////////////////
float BQuaternion::dot(const BQuaternion& q) const
{
   return(D3DXQuaternionDot(&mQuaternion, &q.mQuaternion));
}

//////////////////////////////////////////////////////////////////////
// BQuaternion::norm
// Also Basically the sum of the squares
//////////////////////////////////////////////////////////////////////
float BQuaternion::norm()const
{
   return(D3DXQuaternionLengthSq(&mQuaternion));
}


//////////////////////////////////////////////////////////////////////
// BQuaternion::load
//////////////////////////////////////////////////////////////////////
bool BQuaternion::load(BChunkReader* pChunkReader)
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
// BQuaternion::save
//////////////////////////////////////////////////////////////////////
bool BQuaternion::save(BChunkWriter* pChunkWriter)
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

