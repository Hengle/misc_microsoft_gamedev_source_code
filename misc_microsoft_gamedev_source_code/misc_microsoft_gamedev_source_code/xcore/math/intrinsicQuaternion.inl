//==============================================================================
// BIntrinsicQuaternion Constructors
//==============================================================================
XMFINLINE BIntrinsicQuaternion::BIntrinsicQuaternion(void)
{
}

XMFINLINE BIntrinsicQuaternion::BIntrinsicQuaternion(const XMVECTOR &v)
{
   *this = *((BIntrinsicQuaternion *) &v);
}

XMFINLINE BIntrinsicQuaternion::BIntrinsicQuaternion(const float &fX, const float &fY, const float &fZ, const float &fW)
{
   *this = XMVectorSet(fX, fY, fZ, fW);
}

XMFINLINE BIntrinsicQuaternion::BIntrinsicQuaternion(const BVector &vectAxis, const float &fTheta)
{
   *this = XMQuaternionRotationAxis(vectAxis, fTheta);
}

XMFINLINE BIntrinsicQuaternion::BIntrinsicQuaternion(const BVector &dir, const BVector &up, const BVector &right)
{
   set(dir, up, right);
}

XMFINLINE BIntrinsicQuaternion::BIntrinsicQuaternion(const BVector &vEuler)
{
   *this = XMQuaternionRotationRollPitchYaw(vEuler.z, vEuler.y, vEuler.x);
}

XMFINLINE BIntrinsicQuaternion::BIntrinsicQuaternion(const BMatrix &matrix)
{
   set(matrix);
}

//==============================================================================
// BIntrinsicQuaternion Assignments
//==============================================================================
XMFINLINE void BIntrinsicQuaternion::makeIdentity(void)
{
   *this = XMQuaternionIdentity();
}

XMFINLINE void BIntrinsicQuaternion::set(const BVector &dir, const BVector &up, const BVector &right)
{
   BMatrix matrix;

   matrix.makeOrient(dir, up, right);
   *this = XMQuaternionRotationMatrix(matrix);
}

XMFINLINE void BIntrinsicQuaternion::set(const BMatrix &matrix)
{
   *this = XMQuaternionRotationMatrix(matrix);
}

XMFINLINE void BIntrinsicQuaternion::toAxisAngle(BVector *pvAxis, float *pfTheta) const
{
   XMQuaternionToAxisAngle(pvAxis, pfTheta, *this);
}

XMFINLINE void BIntrinsicQuaternion::toMatrix(BMatrix &matrix) const
{
   matrix = XMMatrixRotationQuaternion(*this);
}

XMFINLINE void BIntrinsicQuaternion::toOrient(BVector *pvDir, BVector *pvUp, BVector *pvRight) const
{
   XMMATRIX matrix = XMMatrixRotationQuaternion(*this);

   *pvRight = matrix.r[0];
   *pvUp = matrix.r[1];
   *pvDir = matrix.r[2];
   return;
}

//==============================================================================
// BIntrinsicQuaternion Math Operations
//==============================================================================
XMFINLINE BIntrinsicQuaternion BIntrinsicQuaternion::operator *(const BIntrinsicQuaternion &q) const
{
   return XMQuaternionMultiply(*this, q);
}

XMFINLINE void BIntrinsicQuaternion::normalize(void)
{
   *this = XMQuaternionNormalize(*this);
}

XMFINLINE bool BIntrinsicQuaternion::operator ==(const BIntrinsicQuaternion &q) const
{
   return (XMQuaternionEqual(*this, q)) ? true : false;
}

XMFINLINE BIntrinsicQuaternion BIntrinsicQuaternion::slerp(const BIntrinsicQuaternion &qTo, const float &fT) const
{
   return XMQuaternionSlerp(*this, qTo, fT);
}

XMFINLINE void BIntrinsicQuaternion::slerp(const BIntrinsicQuaternion &qTo, const float &fT, BIntrinsicQuaternion &result) const
{
   result = XMQuaternionSlerp(*this, qTo, fT);
}

XMFINLINE BIntrinsicQuaternion BIntrinsicQuaternion::inverse(void) const
{
   return XMQuaternionConjugate(*this);
}

XMFINLINE BIntrinsicQuaternion BIntrinsicQuaternion::squad(const BIntrinsicQuaternion &qP, const BIntrinsicQuaternion &qA, const BIntrinsicQuaternion &qB, const BIntrinsicQuaternion &qQ, const float &fT)
{
   return XMQuaternionSquad(qP, qA, qB, qQ, fT);
}

XMFINLINE BIntrinsicQuaternion BIntrinsicQuaternion::log(void) const
{
   return XMQuaternionLn(*this);
}

XMFINLINE BIntrinsicQuaternion BIntrinsicQuaternion::exp(void) const
{
   return XMQuaternionExp(*this);
}

XMFINLINE BIntrinsicQuaternion BIntrinsicQuaternion::operator *(const float &fC) const
{
   return XMVectorScale(*this, fC);
}

XMFINLINE BIntrinsicQuaternion BIntrinsicQuaternion::operator +(const BIntrinsicQuaternion &q) const
{
   return XMVectorAdd(*this, q);
}

XMFINLINE BIntrinsicQuaternion BIntrinsicQuaternion::operator -(const BIntrinsicQuaternion &q) const
{
   return XMVectorSubtract(*this, q);
}

XMFINLINE float BIntrinsicQuaternion::dot(const BIntrinsicQuaternion &q) const
{
   XMVECTOR result = XMQuaternionDot(*this, q);
   return result.x;
}

XMFINLINE float BIntrinsicQuaternion::norm(void) const
{
   XMVECTOR result = XMQuaternionLengthSq(*this);
   return result.x;
}

//==============================================================================
// BIntrinsicQuaternion Accessors
//==============================================================================
XMFINLINE float BIntrinsicQuaternion::getX(void) const
{
   return x;
}

XMFINLINE float BIntrinsicQuaternion::getY(void) const
{
   return y;
}

XMFINLINE float BIntrinsicQuaternion::getZ(void) const
{
   return z;
}

XMFINLINE float BIntrinsicQuaternion::getW(void) const
{
   return w;
}
