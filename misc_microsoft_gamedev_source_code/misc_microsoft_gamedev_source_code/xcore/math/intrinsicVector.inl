//==============================================================================
// BIntrinsicVector Constructors
//==============================================================================
XMFINLINE BIntrinsicVector::BIntrinsicVector(void)
{
   // don't EVER initialize anything here
   BVECTOR_TEST_ALIGNMENT_16(this);
}

XMFINLINE BIntrinsicVector::BIntrinsicVector(XMVECTOR v)
{
   BVECTOR_TEST_ALIGNMENT_16(this);
   BVECTOR_TEST_ALIGNMENT_16(&v);

   *this = *((BIntrinsicVector *) &v);
}

XMFINLINE BIntrinsicVector::BIntrinsicVector(const float v)
{
   BVECTOR_TEST_ALIGNMENT_16(this);

   *this = XMVectorReplicate(v);
}

XMFINLINE BIntrinsicVector::BIntrinsicVector(const float nx, const float ny, const float nz)
{
   BVECTOR_TEST_ALIGNMENT_16(this);

   set(nx, ny, nz);
}

XMFINLINE BIntrinsicVector::BIntrinsicVector(const BIntrinsicShortVector &sv)
{
   BVECTOR_TEST_ALIGNMENT_16(this);

   set(sv.mx.asFloat(), sv.my.asFloat(), sv.mz.asFloat());
}

XMFINLINE BIntrinsicVector::BIntrinsicVector(const BSimpleVector &v)
{
   BVECTOR_TEST_ALIGNMENT_16(this);

   set(v.x, v.y, v.z);
}

XMFINLINE BIntrinsicVector::BIntrinsicVector(const XMDHEN3 *v)
{
   BVECTOR_TEST_ALIGNMENT_16(this);

   unpack(v);
}


//==============================================================================
// BIntrinsicVector Assignment
//==============================================================================
XMFINLINE BIntrinsicVector BIntrinsicVector::operator=(const XMVECTOR v)
{
   BVECTOR_TEST_ALIGNMENT_16(this);
   BVECTOR_TEST_ALIGNMENT_16(&v);

   *this = *((BIntrinsicVector *) &v);
   return *this;
}

XMFINLINE BIntrinsicVector BIntrinsicVector::operator=(const BIntrinsicShortVector &sv)
{
   BVECTOR_TEST_ALIGNMENT_16(this);

   set(sv.mx.asFloat(), sv.my.asFloat(), sv.mz.asFloat());
   return *this;
}

XMFINLINE BIntrinsicVector BIntrinsicVector::operator=(const BSimpleVector &v)
{
   BVECTOR_TEST_ALIGNMENT_16(this);

   set(v.x, v.y, v.z);
   return *this;
}

XMFINLINE void BIntrinsicVector::set(const float nx, const float ny, const float nz)
{
   BVECTOR_TEST_ALIGNMENT_16(this);

   *this = XMVectorSet(nx, ny, nz, 0.0f);
}

XMFINLINE void BIntrinsicVector::set(const BIntrinsicVector v)
{
   BVECTOR_TEST_ALIGNMENT_16(this);

   *this = v;
}

XMFINLINE void BIntrinsicVector::zero(void)
{
   BVECTOR_TEST_ALIGNMENT_16(this);

   *this = XMVectorZero();
}


//==============================================================================
// BIntrinsicVector Packing/Unpacking
//==============================================================================
XMFINLINE void BIntrinsicVector::pack(XMDHEN3 *output) const
{
   BVECTOR_TEST_ALIGNMENT_16(this);

   BIntrinsicVector scaleFactor(511.0f / XM_PI, 1023.0f / XM_PI, 1023.0f / XM_PI);
   XMStoreDHen3(output, XMVectorMultiply(XMVectorModAngles(*this), scaleFactor));
}

XMFINLINE void BIntrinsicVector::unpack(const XMDHEN3 *input)
{
   BVECTOR_TEST_ALIGNMENT_16(this);

   BIntrinsicVector scaleFactor(XM_PI / 511.0f, XM_PI / 1023.0f, XM_PI / 1023.0f);
   *this = XMVectorMultiply(XMLoadDHen3(input), scaleFactor);
}


//==============================================================================
// BIntrinsicVector Comparison
//==============================================================================
XMFINLINE long BIntrinsicVector::operator==(const BIntrinsicVector v) const
{
   BVECTOR_TEST_ALIGNMENT_16(this);
   BVECTOR_TEST_ALIGNMENT_16(&v);

   return XMVector3Equal(*this, v);
}

XMFINLINE long BIntrinsicVector::operator!=(const BIntrinsicVector v) const
{
   BVECTOR_TEST_ALIGNMENT_16(this);
   BVECTOR_TEST_ALIGNMENT_16(&v);

   return XMVector3NotEqual(*this, v);
}


//==============================================================================
// BIntrinsicVector Scaling with Floats
//==============================================================================
XMFINLINE BIntrinsicVector operator*(const float a, const BIntrinsicVector v)
{
   BVECTOR_TEST_ALIGNMENT_16(this);
   BVECTOR_TEST_ALIGNMENT_16(&v);

   return XMVectorScale(v, a);
}

XMFINLINE BIntrinsicVector operator/(const float a, const BIntrinsicVector v)
{
   BVECTOR_TEST_ALIGNMENT_16(this);
   BVECTOR_TEST_ALIGNMENT_16(&v);

   return XMVectorMultiply(v, XMVectorReciprocal(XMVectorReplicate(a)));
}

XMFINLINE BIntrinsicVector BIntrinsicVector::operator*(const float a) const
{
   BVECTOR_TEST_ALIGNMENT_16(this);

   return XMVectorScale(*this, a);
}

XMFINLINE BIntrinsicVector BIntrinsicVector::operator/(const float a) const
{
   BVECTOR_TEST_ALIGNMENT_16(this);

   return XMVectorMultiply(*this, XMVectorReciprocal(XMVectorReplicate(a)));
}

XMFINLINE BIntrinsicVector BIntrinsicVector::operator*=(const float a)
{
   BVECTOR_TEST_ALIGNMENT_16(this);

   *this = XMVectorScale(*this, a);
   return *this;
}

XMFINLINE BIntrinsicVector BIntrinsicVector::operator/=(const float a)
{
   BVECTOR_TEST_ALIGNMENT_16(this);

   *this = XMVectorMultiply(*this, XMVectorReciprocal(XMVectorReplicate(a)));
   return *this;
}

XMFINLINE void BIntrinsicVector::scale(float a)
{
   BVECTOR_TEST_ALIGNMENT_16(this);

   *this = XMVectorScale(*this, a);
}

XMFINLINE void BIntrinsicVector::assignProduct(const float a, const BIntrinsicVector v1)
{
   BVECTOR_TEST_ALIGNMENT_16(this);
   BVECTOR_TEST_ALIGNMENT_16(&v1);

   *this = XMVectorScale(v1, a);
}


//==============================================================================
// BIntrinsicVector Negation
//==============================================================================
XMFINLINE BIntrinsicVector BIntrinsicVector::operator-(void) const
{
   BVECTOR_TEST_ALIGNMENT_16(this);

   return XMVectorNegate(*this);
}

XMFINLINE void BIntrinsicVector::inverse(void)
{
   BVECTOR_TEST_ALIGNMENT_16(this);

   *this = XMVectorNegate(*this);
}


//==============================================================================
// BIntrinsicVector Vector Arithmetic
//==============================================================================
XMFINLINE BIntrinsicVector BIntrinsicVector::operator+(const BIntrinsicVector v) const
{
   BVECTOR_TEST_ALIGNMENT_16(this);
   BVECTOR_TEST_ALIGNMENT_16(&v);

   return XMVectorAdd(*this, v);
}

XMFINLINE BIntrinsicVector BIntrinsicVector::operator+=(const BIntrinsicVector v)
{
   BVECTOR_TEST_ALIGNMENT_16(this);
   BVECTOR_TEST_ALIGNMENT_16(&v);

   *this = XMVectorAdd(*this, v);
   return *this;
}

XMFINLINE BIntrinsicVector BIntrinsicVector::operator-(const BIntrinsicVector v) const
{
   BVECTOR_TEST_ALIGNMENT_16(this);
   BVECTOR_TEST_ALIGNMENT_16(&v);

   return XMVectorSubtract(*this, v);
}

XMFINLINE BIntrinsicVector BIntrinsicVector::operator-=(const BIntrinsicVector v)
{
   BVECTOR_TEST_ALIGNMENT_16(this);
   BVECTOR_TEST_ALIGNMENT_16(&v);

   *this = XMVectorSubtract(*this, v);
   return *this;
}

XMFINLINE void BIntrinsicVector::assignSum(const BIntrinsicVector v1, const BIntrinsicVector v2)
{
   BVECTOR_TEST_ALIGNMENT_16(this);
   BVECTOR_TEST_ALIGNMENT_16(&v1);
   BVECTOR_TEST_ALIGNMENT_16(&v2);

   *this = XMVectorAdd(v1, v2);
}

XMFINLINE void BIntrinsicVector::assignDifference(const BIntrinsicVector v1, const BIntrinsicVector v2)
{
   BVECTOR_TEST_ALIGNMENT_16(this);
   BVECTOR_TEST_ALIGNMENT_16(&v1);
   BVECTOR_TEST_ALIGNMENT_16(&v2);

   *this = XMVectorSubtract(v1, v2);
}

XMFINLINE void BIntrinsicVector::scale(const BIntrinsicVector v)
{
   BVECTOR_TEST_ALIGNMENT_16(this);
   BVECTOR_TEST_ALIGNMENT_16(&v);

   *this = XMVectorMultiply(*this, v);
}


//==============================================================================
// BIntrinsicVector Cross Product
//==============================================================================
XMFINLINE BIntrinsicVector BIntrinsicVector::cross(const BIntrinsicVector v) const
{
   BVECTOR_TEST_ALIGNMENT_16(this);
   BVECTOR_TEST_ALIGNMENT_16(&v);

   return XMVector3Cross(*this, v);
}

XMFINLINE void BIntrinsicVector::assignCrossProduct(const BIntrinsicVector v1, const BIntrinsicVector v2)
{
   BVECTOR_TEST_ALIGNMENT_16(this);
   BVECTOR_TEST_ALIGNMENT_16(&v1);
   BVECTOR_TEST_ALIGNMENT_16(&v2);

   *this = XMVector3Cross(v1, v2);
}


//==============================================================================
// BIntrinsicVector Dot Product
//==============================================================================
XMFINLINE float BIntrinsicVector::dot(const BIntrinsicVector v) const
{
   BVECTOR_TEST_ALIGNMENT_16(this);
   BVECTOR_TEST_ALIGNMENT_16(&v);

   float retVal = 0.0f;
   const XMVECTOR result = XMVector3Dot(*this, v);
   XMStoreScalar(&retVal,result);
   return retVal;
}


//==============================================================================
// BIntrinsicVector Length and Normalize
//==============================================================================
XMFINLINE float BIntrinsicVector::length(void) const
{
   BVECTOR_TEST_ALIGNMENT_16(this);

   float retVal = 0.0f;
   const XMVECTOR result = XMVector3Length(*this);
   XMStoreScalar(&retVal,result);
   return retVal;
}

XMFINLINE float BIntrinsicVector::lengthEstimate(void) const
{
   BVECTOR_TEST_ALIGNMENT_16(this);

   float retVal = 0.0f;
   const XMVECTOR result = XMVector3LengthEst(*this);
   XMStoreScalar(&retVal,result);
   return retVal;
}

XMFINLINE float BIntrinsicVector::lengthSquared(void) const
{
   BVECTOR_TEST_ALIGNMENT_16(this);

   float retVal = 0.0f;
   const XMVECTOR result = XMVector3LengthSq(*this);
   XMStoreScalar(&retVal,result);
   return retVal;
}

XMFINLINE void BIntrinsicVector::normalize(void)
{
   BVECTOR_TEST_ALIGNMENT_16(this);

   *this = XMVector3Normalize(*this);
}

XMFINLINE void BIntrinsicVector::normalizeEstimate(void)
{
   BVECTOR_TEST_ALIGNMENT_16(this);

   *this = XMVector3NormalizeEst(*this);
}

XMFINLINE bool BIntrinsicVector::safeNormalize(void)
{
#if 0
   float len=length(); 

   if(len>cFloatCompareEpsilon) 
   {
      x/=len; 
      y/=len; 
      z/=len; 
      return(true);
   } 

   return(false);
#else
   BVECTOR_TEST_ALIGNMENT_16(this);

   XMVECTOR lengthSqr = XMVector3LengthSq(*this);
   XMVECTOR result = XMVectorGreater(lengthSqr, XMVectorReplicate(cFloatCompareEpsilon * cFloatCompareEpsilon));
   *this = XMVectorSelect(*this, XMVectorMultiply(*this, XMVectorReciprocalSqrt(lengthSqr)), result);

   return (result.x ? true : false);
#endif
}

XMFINLINE bool BIntrinsicVector::safeNormalizeEstimate(void)
{
#if 0
   float len=length(); 

   if(len>cFloatCompareEpsilon) 
   {
      x/=len; 
      y/=len; 
      z/=len; 
      return(true);
   } 

   return(false);
#else
   BVECTOR_TEST_ALIGNMENT_16(this);

   XMVECTOR lengthSqr = XMVector3LengthSq(*this);
   XMVECTOR result = XMVectorGreater(lengthSqr, XMVectorReplicate(cFloatCompareEpsilon * cFloatCompareEpsilon));
   *this = XMVectorSelect(*this, XMVectorMultiply(*this, XMVectorReciprocalSqrtEst(lengthSqr)), result);

   return (result.u[0] ? true : false);
#endif
}

//==============================================================================
// BIntrinsicVector Other Useful Functions
//==============================================================================
XMFINLINE void BIntrinsicVector::makePositive(void)
{
   BVECTOR_TEST_ALIGNMENT_16(this);

   *this = XMVectorAbs(*this);
}

XMFINLINE void BIntrinsicVector::round(void)
{
   BVECTOR_TEST_ALIGNMENT_16(this);

   *this = XMVectorRound(*this);
}

XMFINLINE void BIntrinsicVector::clamp(const BIntrinsicVector minVec, const BIntrinsicVector maxVec)
{
   BVECTOR_TEST_ALIGNMENT_16(this);
   BVECTOR_TEST_ALIGNMENT_16(&minVec);
   BVECTOR_TEST_ALIGNMENT_16(&maxVec);

   *this = XMVectorClamp(*this, minVec, maxVec);
}

XMFINLINE void BIntrinsicVector::lerpPosition(const float t, const BIntrinsicVector p0, const BIntrinsicVector p1)
{
   BVECTOR_TEST_ALIGNMENT_16(this);
   BVECTOR_TEST_ALIGNMENT_16(&p0);
   BVECTOR_TEST_ALIGNMENT_16(&p1);

   float temp = max(0.0f, min(1.0f, t));
   *this = XMVectorLerp(p0, p1, temp);
}

XMFINLINE float BIntrinsicVector::distance(const BIntrinsicVector point) const
{
   BVECTOR_TEST_ALIGNMENT_16(this);
   BVECTOR_TEST_ALIGNMENT_16(&point);

   XMVECTOR temp = XMVector3Length(XMVectorSubtract(*this, point));
   return temp.x;
}

XMFINLINE float BIntrinsicVector::distanceEstimate(const BIntrinsicVector point) const
{
   BVECTOR_TEST_ALIGNMENT_16(this);
   BVECTOR_TEST_ALIGNMENT_16(&point);

   XMVECTOR temp = XMVector3LengthEst(XMVectorSubtract(*this, point));
   return temp.x;
}

XMFINLINE float BIntrinsicVector::distanceSqr(const BIntrinsicVector point) const
{
   BVECTOR_TEST_ALIGNMENT_16(this);
   BVECTOR_TEST_ALIGNMENT_16(&point);

   XMVECTOR temp = XMVector3LengthSq(XMVectorSubtract(*this, point));
   return temp.x;
}

XMFINLINE float BIntrinsicVector::xzDistance(const BIntrinsicVector point) const
{
   BVECTOR_TEST_ALIGNMENT_16(this);
   BVECTOR_TEST_ALIGNMENT_16(&point);

   XMVECTOR temp = XMVectorSubtract(*this, point);
   temp = XMVectorMultiply(temp, temp);
   return (float) sqrt(temp.x + temp.z);
}

XMFINLINE float BIntrinsicVector::xzDistanceSqr(const BIntrinsicVector point) const
{
   BVECTOR_TEST_ALIGNMENT_16(this);
   BVECTOR_TEST_ALIGNMENT_16(&point);

   XMVECTOR temp = XMVectorSubtract(*this, point);
   temp = XMVectorMultiply(temp, temp);
   return (temp.x + temp.z);
}

XMFINLINE float BIntrinsicVector::xyDistance(const BIntrinsicVector point) const
{
   BVECTOR_TEST_ALIGNMENT_16(this);
   BVECTOR_TEST_ALIGNMENT_16(&point);

   XMVECTOR temp = XMVectorSubtract(*this, point);
   temp = XMVectorMultiply(temp, temp);
   return (float) sqrt(temp.x + temp.y);
}

XMFINLINE float BIntrinsicVector::xyDistanceSqr(const BIntrinsicVector point) const
{
   BVECTOR_TEST_ALIGNMENT_16(this);
   BVECTOR_TEST_ALIGNMENT_16(&point);

   XMVECTOR temp = XMVectorSubtract(*this, point);
   temp = XMVectorMultiply(temp, temp);
   return (temp.x + temp.y);
}


//==============================================================================
// BIntrinsicVector2 Constructors
//==============================================================================
XMFINLINE BIntrinsicVector2::BIntrinsicVector2(void)
{
   // don't EVER initialize anything here
   BVECTOR_TEST_ALIGNMENT_16(this);
}

XMFINLINE BIntrinsicVector2::BIntrinsicVector2(XMVECTOR v)
{
   BVECTOR_TEST_ALIGNMENT_16(this);
   BVECTOR_TEST_ALIGNMENT_16(&v);

   *this = *((BIntrinsicVector2 *) &v);
}

XMFINLINE BIntrinsicVector2::BIntrinsicVector2(const float nx, const float ny)
{
   BVECTOR_TEST_ALIGNMENT_16(this);

   *this = XMVectorSet(nx, ny, 0.0f, 0.0f);
}


//==============================================================================
// BIntrinsicVector2 Functions
//==============================================================================
XMFINLINE BIntrinsicVector2 BIntrinsicVector2::operator+(const BIntrinsicVector2 v) const
{
   BVECTOR_TEST_ALIGNMENT_16(this);
   BVECTOR_TEST_ALIGNMENT_16(&v);

   return XMVectorAdd(*this, v);
}

XMFINLINE BIntrinsicVector2 BIntrinsicVector2::operator*(const float a) const
{
   BVECTOR_TEST_ALIGNMENT_16(this);

   return XMVectorScale(*this, a);
}

//==============================================================================
// BIntrinsicVector4 Constructors
//==============================================================================
XMFINLINE BIntrinsicVector4::BIntrinsicVector4(void)
{
   // don't EVER initialize anything here
   BVECTOR_TEST_ALIGNMENT_16(this);
}

XMFINLINE BIntrinsicVector4::BIntrinsicVector4(XMVECTOR v)
{
   BVECTOR_TEST_ALIGNMENT_16(this);
   BVECTOR_TEST_ALIGNMENT_16(&v);

   *this = *((BIntrinsicVector4 *) &v);
}

XMFINLINE BIntrinsicVector4::BIntrinsicVector4(const float nx, const float ny, const float nz, const float nw)
{
   BVECTOR_TEST_ALIGNMENT_16(this);

   *this = XMVectorSet(nx, ny, nz, nw);
}


//==============================================================================
// BIntrinsicVector4 Functions
//==============================================================================
XMFINLINE BIntrinsicVector4 BIntrinsicVector4::operator+(const BIntrinsicVector4 v) const
{
   BVECTOR_TEST_ALIGNMENT_16(this);
   BVECTOR_TEST_ALIGNMENT_16(&v);

   return XMVectorAdd(*this, v);
}

XMFINLINE BIntrinsicVector4 BIntrinsicVector4::operator*(const float a) const
{
   BVECTOR_TEST_ALIGNMENT_16(this);

   return XMVectorScale(*this, a);
}

XMFINLINE void BIntrinsicVector4::lerp(const float t, const BIntrinsicVector4 p0, const BIntrinsicVector4 p1)
{
   BVECTOR_TEST_ALIGNMENT_16(this);
   BVECTOR_TEST_ALIGNMENT_16(&p0);
   BVECTOR_TEST_ALIGNMENT_16(&p1);

   float temp = max(0.0f, min(1.0f, t));
   *this = XMVectorLerp(p0, p1, temp);
}


//==============================================================================
// BIntrinsicShortVector Constructors
//==============================================================================
XMFINLINE BIntrinsicShortVector::BIntrinsicShortVector(void)
{
   // don't EVER initialize anything here
}

XMFINLINE BIntrinsicShortVector::BIntrinsicShortVector(const BIntrinsicVector v)
{
   BVECTOR_TEST_ALIGNMENT_16(&v);

   set(v.x, v.y, v.z);
}

XMFINLINE BIntrinsicShortVector::BIntrinsicShortVector(const float nx, const float ny, const float nz)
{
   set(nx, ny, nz);
}


//==============================================================================
// BIntrinsicShortVector Assignments
//==============================================================================
XMFINLINE BIntrinsicShortVector &BIntrinsicShortVector::operator=(const BIntrinsicVector v)
{
   BVECTOR_TEST_ALIGNMENT_16(&v);

   set(v.x, v.y, v.z);
   return *this;
}

XMFINLINE void BIntrinsicShortVector::set(const float nx, const float ny, const float nz)
{
   mx = nx;
   my = ny;
   mz = nz;
}

XMFINLINE void BIntrinsicShortVector::zero(void)
{
   set(0.0f, 0.0f, 0.0f);
}
