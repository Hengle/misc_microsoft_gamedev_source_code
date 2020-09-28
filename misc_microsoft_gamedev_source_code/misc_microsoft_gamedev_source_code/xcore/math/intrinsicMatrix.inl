//=============================================================================
// intrinsicmatrix.inl
//
// Copyright (c) 2005 Ensemble Studios
//=============================================================================

XMFINLINE void BIntrinsicMatrix::setD3DXMatrix(const D3DMATRIX &d3dmatrix)
{
   XMemCpy(this, &d3dmatrix, sizeof(BIntrinsicMatrix));
}

XMFINLINE void BIntrinsicMatrix::getD3DXMatrix(D3DMATRIX &d3dmatrix) const
{
   XMemCpy(&d3dmatrix, this, sizeof(BIntrinsicMatrix));
}

XMFINLINE void BIntrinsicMatrix::makeRotateArbitrary(const float rads, const BVector axis)
{
   if (XMVector3Equal(axis, XMVectorZero()))
      makeIdentity();
   else
      *this = XMMatrixRotationAxis(axis, rads);
}

XMFINLINE void BIntrinsicMatrix::makeRotateYawPitchRoll(float yaw, float pitch, float roll)
{
   *this = XMMatrixRotationRollPitchYaw(pitch, yaw, roll);
}

XMFINLINE void BIntrinsicMatrix::multRotateX(float rads)
{
   *this = XMMatrixMultiply(*this, XMMatrixRotationX(rads));
}

XMFINLINE void BIntrinsicMatrix::multRotateXSinCos(float sine, float cosine)
{
   BIntrinsicMatrix temp;
   temp.makeRotateXSinCos(sine, cosine);
   mult(*this, temp);
}

XMFINLINE void BIntrinsicMatrix::multRotateY(float rads)
{
   *this = XMMatrixMultiply(*this, XMMatrixRotationY(rads));
}

XMFINLINE void BIntrinsicMatrix::multRotateYSinCos(float sine, float cosine)
{
   BIntrinsicMatrix temp;
   temp.makeRotateYSinCos(sine,cosine);
   mult(*this, temp);
}

XMFINLINE void BIntrinsicMatrix::multRotateZ(float rads)
{
   *this = XMMatrixMultiply(*this, XMMatrixRotationZ(rads));
}

XMFINLINE void BIntrinsicMatrix::multRotateZSinCos(float sine, float cosine)
{
   BIntrinsicMatrix temp;
   temp.makeRotateZSinCos(sine,cosine);
   mult(*this, temp);
}

XMFINLINE void BIntrinsicMatrix::multRotateArbitrary(const float rads, const BVector axis)
{
   *this = XMMatrixMultiply(*this, XMMatrixRotationAxis(axis, rads));
}

XMFINLINE void BIntrinsicMatrix::multInverseOrient(const BVector dir, const BVector up, const BVector right)
{
   BIntrinsicMatrix temp;
   temp.makeInverseOrient(dir, up, right);
   mult(*this, temp);
}

//=============================================================================
// Transforms the given vector by multiplying by the matrix (without the 
// translation) and returns the result in result
//=============================================================================
XMFINLINE void BIntrinsicMatrix::transformVector(const BVector vect, BVector &result) const
{
   result = XMVector3TransformNormal(vect, *this);
}

XMFINLINE void BIntrinsicMatrix::transformVectorList(const BVector *pvect, BVector *presult, DWORD nvertices) const
{
   presult = (BVector *) XMVector3TransformNormalStream((XMFLOAT3 *) presult, sizeof(BVector), (const XMFLOAT3 *) pvect, sizeof(BVector), nvertices, *this);
}

//=============================================================================
// Transforms the given vector (treating it as a point, not a vector) by multiplying 
// it by the matrix and returns the result in result
//=============================================================================
XMFINLINE void BIntrinsicMatrix::transformVectorAsPoint(const BVector vect, BVector &result) const
{
   result = XMVector3Transform(vect, *this);
}

//=============================================================================
// Transforms the given vector (treating it as a point, not a vector) by multiplying 
// it by the matrix and returns the result in result
//=============================================================================
XMFINLINE void BIntrinsicMatrix::transformVectorAsPoint(const BVector vect, BVector4 &result) const
{
   result = XMVector3Transform(vect, *this);
}

//=============================================================================
// Transforms the given vector (treating it as a point, not a vector) by multiplying 
// it by the matrix and returns the result in result
//=============================================================================
XMFINLINE void BIntrinsicMatrix::transformVectorAsPoint(const BVector2 vect, BVector &result) const
{
   result = XMVector2Transform(vect, *this);
}

XMFINLINE void BIntrinsicMatrix::transformVectorListAsPoint(const BVector *vect, BVector4 *result, const long num) const
{
   result = (BVector4 *) XMVector3TransformStream((XMFLOAT4 *) result, sizeof(BVector), (const XMFLOAT3 *) vect, sizeof(BVector), num, *this);
   for(long i = 0; i < num; i++, result++)
      result->w = 1.0f / result->z;
}

XMFINLINE void BIntrinsicMatrix::transformVectorListAsPoint(const BVector *vect, BVector *result, const long num) const
{
   result = (BVector *) XMVector3TransformStream((XMFLOAT4 *) result, sizeof(BVector), (const XMFLOAT3 *) vect, sizeof(BVector), num, *this);
}

XMFINLINE bool BIntrinsicMatrix::invert(void)
{
   XMVECTOR determinant;
   *this = XMMatrixInverse(&determinant, *this);
   return true;
}

//=============================================================================
// Normal is expected to be normalized!
//=============================================================================
XMFINLINE void BIntrinsicMatrix::makeReflect(const BVector point, const BVector normal)
{
   *this = XMMatrixReflect(XMPlaneFromPointNormal(point, normal));
}

XMFINLINE void BIntrinsicMatrix::multReflect(const BVector point, const BVector normal)
{
   *this = XMMatrixMultiply(*this, XMMatrixReflect(XMPlaneFromPointNormal(point, normal)));
}

XMFINLINE void BIntrinsicMatrix::makeOrient(const BVector dir, const BVector up, const BVector right)
{
   r[0] = XMVectorZero();
   r[1] = XMVectorZero();
   r[2] = XMVectorZero();
   r[0] = XMVectorInsert(r[0], right, 0, 1, 1, 1, 0);
   r[1] = XMVectorInsert(r[1], up, 0, 1, 1, 1, 0);
   r[2] = XMVectorInsert(r[2], dir, 0, 1, 1, 1, 0);
   r[3] = __vpermwi(__vupkd3d(XMVectorZero(), VPACK_NORMSHORT2), 0xAB);
}

XMFINLINE void BIntrinsicMatrix::makeInverseOrient(const BVector dir, const BVector up, const BVector right)
{
   makeOrient(dir, up, right);
   *this = XMMatrixTranspose(*this);
}

XMFINLINE BIntrinsicMatrix::BIntrinsicMatrix(void)
{
}

XMFINLINE BIntrinsicMatrix::BIntrinsicMatrix(const XMMATRIX m)
{
   *this = *((BIntrinsicMatrix *) &m);
}

XMFINLINE BVector BIntrinsicMatrix::getRow(const int row) const
{
   return r[row];
}

XMFINLINE float BIntrinsicMatrix::getElement(const int row, const int column) const
{
   return m[row][column];
}

XMFINLINE void BIntrinsicMatrix::setElement(const int row, const int column, const float value)
{
   m[row][column] = value;
}

XMFINLINE void BIntrinsicMatrix::makeIdentity(void)
{
   *this = XMMatrixIdentity();
}

XMFINLINE void BIntrinsicMatrix::makeZero(void)
{
   r[0] = XMVectorZero();
   r[1] = XMVectorZero();
   r[2] = XMVectorZero();
   r[3] = XMVectorZero();
} 

XMFINLINE void BIntrinsicMatrix::makeRotateX(float rads)
{
   *this = XMMatrixRotationX(rads);
}

XMFINLINE void BIntrinsicMatrix::makeRotateXSinCos(float sine, float cosine)
{
   r[0] = XMVectorZero();
   r[1] = XMVectorSet(0.0f, cosine, sine, 0.0f);
   r[2] = XMVectorSet(0.0f, -sine, cosine, 0.0f);
   clearTranslation();
}

XMFINLINE void BIntrinsicMatrix::makeRotateY(float rads)
{
   *this = XMMatrixRotationY(rads);
}

XMFINLINE void BIntrinsicMatrix::makeRotateYSinCos(float sine, float cosine)
{
   r[0] = XMVectorSet(cosine, 0.0f, -sine, 0.0f);
   r[1] = XMVectorZero();
   r[2] = XMVectorSet(sine, 0.0f, cosine, 0.0f);
   clearTranslation();
}

XMFINLINE void BIntrinsicMatrix::makeRotateZ(float rads)
{
   *this = XMMatrixRotationZ(rads);
}

XMFINLINE void BIntrinsicMatrix::makeRotateZSinCos(float sine, float cosine)
{
   r[0] = XMVectorSet(cosine, sine, 0.0f, 0.0f);
   r[1] = XMVectorSet(-sine, cosine, 0.0f, 0.0f);
   r[2] = XMVectorZero();
   clearTranslation();
}

XMFINLINE void BIntrinsicMatrix::makeScale(float xScale, float yScale, float zScale)
{
   *this = XMMatrixScaling(xScale, yScale, zScale);
}

XMFINLINE void BIntrinsicMatrix::makeScale(float scale)
{
   *this = XMMatrixScalingFromVector(XMVectorReplicate(scale));
}

XMFINLINE void BIntrinsicMatrix::makeTranslate(const float tx, const float ty, const float tz)
{
   *this = XMMatrixTranslation(tx, ty, tz);
}

XMFINLINE void BIntrinsicMatrix::makeTranslate(const BVector v)
{
   *this = XMMatrixTranslation(v.x, v.y, v.z);
}

XMFINLINE void BIntrinsicMatrix::multTranslate(float tx, float ty, float tz)
{
   r[3] = XMVectorAdd(r[3], XMVectorSet(tx, ty, tz, 0.0f));
}

XMFINLINE void BIntrinsicMatrix::multScale(float xScale, float yScale, float zScale)
{
   *this = XMMatrixMultiply(*this, XMMatrixScaling(xScale, yScale, zScale));
}

XMFINLINE void BIntrinsicMatrix::multScale(float factor)
{
   *this = XMMatrixMultiply(*this, XMMatrixScalingFromVector(XMVectorReplicate(factor)));
}

XMFINLINE void BIntrinsicMatrix::multOrient(const BVector dir, const BVector up, const BVector right)
{
   BIntrinsicMatrix  temp;
   temp.makeOrient(dir, up, right);
   mult(*this, temp);
}

XMFINLINE BVector BIntrinsicMatrix::operator*(const BVector vec) const
{
   return XMVector3TransformNormal(vec, *this);
}

XMFINLINE void BIntrinsicMatrix::mult(BIntrinsicMatrix matrix1, BIntrinsicMatrix matrix2)
{
   *this = XMMatrixMultiply(matrix1, matrix2);
}

XMFINLINE long BIntrinsicMatrix::operator==(const BIntrinsicMatrix &mat) const
{
   if (XMVector3Equal(r[0], mat.r[0]) &&
       XMVector3Equal(r[1], mat.r[1]) &&
       XMVector3Equal(r[2], mat.r[2]) &&
       XMVector3Equal(r[3], mat.r[3]))
      return true;
   else
      return false;
} 

XMFINLINE long BIntrinsicMatrix::operator!=(const BIntrinsicMatrix &mat) const
{
   if (XMVector3Equal(r[0], mat.r[0]) &&
       XMVector3Equal(r[1], mat.r[1]) &&
       XMVector3Equal(r[2], mat.r[2]) &&
       XMVector3Equal(r[3], mat.r[3]))
      return false;
   else
      return true;
}

XMFINLINE void BIntrinsicMatrix::transposeRotation()
{
   bswap(m[1][0], m[0][1]);
   bswap(m[2][0], m[0][2]);
   bswap(m[2][1], m[1][2]);
}

XMFINLINE void BIntrinsicMatrix::transpose()
{
   *this = XMMatrixTranspose(*this);
}

XMFINLINE void BIntrinsicMatrix::clearTranslation()
{
   r[3] = __vpermwi(__vupkd3d(XMVectorZero(), VPACK_NORMSHORT2), 0xAB);
}

XMFINLINE void BIntrinsicMatrix::clearOrientation()
{
   XMVECTOR ZO;

   XMDUMMY_INITIALIZE_VECTOR(ZO);

   ZO = __vupkd3d(ZO, VPACK_NORMSHORT2);

   r[0] = __vpermwi(ZO, 0xEA);
   r[1] = __vpermwi(ZO, 0xBA);
   r[2] = __vpermwi(ZO, 0xAE);
}

XMFINLINE void BIntrinsicMatrix::setTranslation(const BVector v)
{
   r[3] = __vupkd3d(XMVectorZero(), VPACK_NORMSHORT2);
   r[3] = XMVectorInsert(r[3], v, 0, 1, 1, 1, 0);
}

XMFINLINE void BIntrinsicMatrix::setTranslation(float x, float y, float z)
{
   r[3] = XMVectorSet(x, y, z, 1.0f);
}

XMFINLINE void BIntrinsicMatrix::getTranslation(BVector& v) const
{
   v = r[3];
}

XMFINLINE void BIntrinsicMatrix::getForward(BVector& v) const
{
   v = r[2];
}

XMFINLINE void BIntrinsicMatrix::getUp(BVector& v) const
{
   v = r[1];
}

XMFINLINE void BIntrinsicMatrix::getRight(BVector& v) const
{
   v = r[0];
}

XMFINLINE void BIntrinsicMatrix::getScale(float& x, float& y, float& z) const
{
   x = m[0][0];
   y = m[1][1];
   z = m[2][2];
}

XMFINLINE void BIntrinsicMatrix::makeView(const BVector cameraLoc, const BVector cameraDir, const BVector cameraUp, const BVector cameraRight)
{
   XMMATRIX tempMatrix;
   XMVECTOR negLoc = XMVectorNegate(cameraLoc);
   tempMatrix.r[0] = cameraRight;
   tempMatrix.r[1] = cameraUp;
   tempMatrix.r[2] = cameraDir;
   tempMatrix.r[3] = __vpermwi(__vupkd3d(XMVectorZero(), VPACK_NORMSHORT2), 0xAB);
   *this = XMMatrixTranspose(tempMatrix);
   r[3] = XMVectorInsert(r[3], XMVector3Dot(negLoc, cameraRight), 0, 1, 0, 0, 0);
   r[3] = XMVectorInsert(r[3], XMVector3Dot(negLoc, cameraUp), 0, 0, 1, 0, 0);
   r[3] = XMVectorInsert(r[3], XMVector3Dot(negLoc, cameraDir), 0, 0, 0, 1, 0);
}

XMFINLINE void BIntrinsicMatrix::makeInverseView(const BVector cameraLoc, const BVector cameraDir, const BVector cameraUp, const BVector cameraRight)
{
   r[0] = XMVectorZero();
   r[1] = XMVectorZero();
   r[2] = XMVectorZero();
   r[3] = __vpermwi(__vupkd3d(XMVectorZero(), VPACK_NORMSHORT2), 0xAB);
   r[0] = XMVectorInsert(r[0], cameraRight, 0, 1, 1, 1, 0);
   r[1] = XMVectorInsert(r[1], cameraUp, 0, 1, 1, 1, 0);
   r[2] = XMVectorInsert(r[2], cameraDir, 0, 1, 1, 1, 0);
   r[3] = XMVectorInsert(r[3], cameraLoc, 0, 1, 1, 1, 0);
}

//=============================================================================
// eof: intrinsicmatrix.inl
//=============================================================================
