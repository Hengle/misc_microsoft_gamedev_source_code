//=============================================================================
// simplematrix.cpp
//
// Copyright (c) 1999-2005 Ensemble Studios
//=============================================================================

#include "xcore.h"
#include "math\matrix.h"

//==============================================================================
// BSimpleMatrix::setD3DXMatrix
//==============================================================================
void BSimpleMatrix::setD3DXMatrix(const D3DMATRIX &d3dmatrix)
{
   m[0][0] = d3dmatrix._11; m[1][0] = d3dmatrix._12; m[2][0] = d3dmatrix._13;
   m[0][1] = d3dmatrix._21; m[1][1] = d3dmatrix._22; m[2][1] = d3dmatrix._23;
   m[0][2] = d3dmatrix._31; m[1][2] = d3dmatrix._32; m[2][2] = d3dmatrix._33;
   m[0][3] = d3dmatrix._41; m[1][3] = d3dmatrix._42; m[2][3] = d3dmatrix._43;
} // D3DMatrixToBSimpleMatrix

//==============================================================================
// BSimpleMatrix::getD3DMatrix
//==============================================================================
void BSimpleMatrix::getD3DXMatrix(D3DMATRIX &d3dmatrix) const
{
   d3dmatrix._11 = m[0][0]; d3dmatrix._12 = m[1][0]; d3dmatrix._13 = m[2][0]; d3dmatrix._14 = 0;
   d3dmatrix._21 = m[0][1]; d3dmatrix._22 = m[1][1]; d3dmatrix._23 = m[2][1]; d3dmatrix._24 = 0;
   d3dmatrix._31 = m[0][2]; d3dmatrix._32 = m[1][2]; d3dmatrix._33 = m[2][2]; d3dmatrix._34 = 0;
   d3dmatrix._41 = m[0][3]; d3dmatrix._42 = m[1][3]; d3dmatrix._43 = m[2][3]; d3dmatrix._44 = 1;
} // BSimpleMatrixToD3DMatrix

//=============================================================================
// BSimpleMatrix::makeRotateArbitrary(const float rads, const BVector &axis)
//=============================================================================
void BSimpleMatrix::makeRotateArbitrary(const float rads, const BVector &axis)
{
   float c = (float)cos(rads);
   float omc = 1.0f-c;
   float s = (float)sin(rads);

   float A = axis.x;
   float B = axis.y;
   float C = axis.z;
   
   m[0][0] = A*A*omc+c;
   float AB = A*B;
   m[0][1] = AB*omc-C*s;
   float AC = A*C;
   m[0][2] = AC*omc+B*s;
   m[0][3] = 0.0f;

   m[1][0] = AB*omc+C*s;
   m[1][1] = B*B*omc+c;
   float BC = B*C;
   m[1][2] = BC*omc-A*s;
   m[1][3] = 0.0f;

   m[2][0] = AC*omc-B*s;
   m[2][1] = BC*omc+A*s;
   m[2][2] = C*C*omc+c;
   m[2][3] = 0.0f;
} // BSimpleMatrix::makeRotateArbitrary


//=============================================================================
// BSimpleMatrix::makeRotateYawPitchRoll
//=============================================================================
void BSimpleMatrix::makeRotateYawPitchRoll(float yaw, float pitch, float roll)
{
   m[0][0] = cosf(yaw)*cosf(roll)+sinf(pitch)*sinf(yaw)*sinf(roll); 
   m[0][1] = cosf(yaw)*-sinf(roll)+sinf(pitch)*sinf(yaw)*cosf(roll);
   m[0][2] = cosf(pitch)*sinf(yaw);
   m[0][3] = 0.0f;
   
   m[1][0] = cosf(pitch)*sinf(roll);
   m[1][1] = cosf(pitch)*cosf(roll);
   m[1][2] = -sinf(pitch);
   m[1][3] = 0.0f;
   
   m[2][0] = -sinf(yaw)*cosf(roll)+sinf(pitch)*cosf(yaw)*sinf(roll);
   m[2][1] = -sinf(yaw)*-sinf(roll)+sinf(pitch)*cosf(yaw)*cosf(roll);
   m[2][2] = cosf(pitch)*cosf(yaw);
   m[2][3] = 0.0f;
}



//NOTE: easy optimization!  Change all of the following mult* functions to 
//only affect the parts of the matrix that are going to change instead of doing
//a full multiply.  Can also get away with only saving the parts of the matrix 
//that are going to change instead of making a whole copy of the matrix to multiply.


//=============================================================================
// BSimpleMatrix::multRotateX(float rads)
//=============================================================================
void BSimpleMatrix::multRotateX(float rads)
{
   BSimpleMatrix matrixTemp2;
   matrixTemp2.makeRotateX(rads);
   this->mult(*this, matrixTemp2);
} // BSimpleMatrix::multRotateX

//=============================================================================
// BSimpleMatrix::multRotateXSinCos(float sine, float cosine)
//=============================================================================
void BSimpleMatrix::multRotateXSinCos(float sine, float cosine)
{
   BSimpleMatrix matrixTemp3;
   matrixTemp3.makeRotateXSinCos(sine, cosine);
   this->mult(*this, matrixTemp3);
} // BSimpleMatrix::multRotateXSinCos

//=============================================================================
// BSimpleMatrix::multRotateY(float rads)
//=============================================================================
void BSimpleMatrix::multRotateY(float rads)
{
   BSimpleMatrix matrixTemp4;
   matrixTemp4.makeRotateY(rads);
   this->mult(*this, matrixTemp4);
} // BSimpleMatrix::multRotateY

//=============================================================================
// BSimpleMatrix::multRotateYSinCos(float sine, float cosine)
//=============================================================================
void BSimpleMatrix::multRotateYSinCos(float sine, float cosine)
{
   BSimpleMatrix matrixTemp5;
   matrixTemp5.makeRotateYSinCos(sine,cosine);
   this->mult(*this, matrixTemp5);
} // BSimpleMatrix::multRotateYSinCos

//=============================================================================
// BSimpleMatrix::multRotateZ(float rads)
//=============================================================================
void BSimpleMatrix::multRotateZ(float rads)
{
   BSimpleMatrix matrixTemp6;
   matrixTemp6.makeRotateZ(rads);
   this->mult(*this, matrixTemp6);
} // BSimpleMatrix::multRotateZ

//=============================================================================
// BSimpleMatrix::multRotateZSinCos(float sine, float cosine)
//=============================================================================
void BSimpleMatrix::multRotateZSinCos(float sine, float cosine)
{
   BSimpleMatrix matrixTemp7;
   matrixTemp7.makeRotateZSinCos(sine,cosine);
   this->mult(*this, matrixTemp7);
} // BSimpleMatrix::multRotateZSinCos


//=============================================================================
// BSimpleMatrix::multRotateArbitrary(const float rads, const BVector &axis)
//=============================================================================
void BSimpleMatrix::multRotateArbitrary(const float rads, const BVector &axis)
{
   BSimpleMatrix matrixTemp8;
   matrixTemp8.makeRotateArbitrary(rads, axis);
   this->mult(*this, matrixTemp8);
} // BSimpleMatrix::multRotateArbitrary

//=============================================================================
// BSimpleMatrix::multInverseOrient(const BVector &dir, const BVector &up, const BVector &right)
//=============================================================================
void BSimpleMatrix::multInverseOrient(const BVector &dir, const BVector &up, const BVector &right)
{
   BSimpleMatrix matrixTemp11;
   matrixTemp11.makeInverseOrient(dir, up, right);
   this->mult(*this, matrixTemp11);
} // BSimpleMatrix::multInverseOrient


//=============================================================================
// BSimpleMatrix::transformVector(const BVector &vect, BVector &result) const
//
// Transforms the given vector by multiplying by the matrix (without the 
// translation) and returns the result in result
//=============================================================================
void BSimpleMatrix::transformVector(const BVector &vect, BVector &result) const
{
   const float *pfloat = &m[0][0];
   const float *psrc = &vect.x;

   // transform x

   result.x = *pfloat * *psrc + 
              *(pfloat + 1) * *(psrc + 1) + 
              *(pfloat + 2) * *(psrc + 2);

   // transform y

   result.y = *(pfloat + 4)  * *psrc + 
              *(pfloat + 5)  * *(psrc + 1) + 
              *(pfloat + 6)  * *(psrc + 2);

   // transform z

   result.z = *(pfloat + 8)  * *psrc + 
              *(pfloat + 9)  * *(psrc + 1) + 
              *(pfloat + 10)  * *(psrc + 2);

} // BSimpleMatrix::transformVector

//=============================================================================
void BSimpleMatrix::transformVectorList(const BVector *pvect, BVector *presult, DWORD nvertices) const
{
   DWORD i = 0;
   const float *pfloat = &m[0][0];
   const float *psrc;

   do
   {
      psrc = &pvect->x;

      // transform x

      presult->x = *pfloat * *psrc + 
                 *(pfloat + 1) * *(psrc + 1) + 
                 *(pfloat + 2) * *(psrc + 2);

      // transform y

      presult->y = *(pfloat + 4)  * *psrc + 
                 *(pfloat + 5)  * *(psrc + 1) + 
                 *(pfloat + 6)  * *(psrc + 2);

      // transform z

      presult->z = *(pfloat + 8)  * *psrc + 
                 *(pfloat + 9)  * *(psrc + 1) + 
                 *(pfloat + 10)  * *(psrc + 2);

      pvect++;
      presult++;

      i++;

   } while (i < nvertices);

} // BSimpleMatrix::transformVectorList

//=============================================================================
// BSimpleMatrix::transformVectorAsPoint(const BVector &vect, BVector &result) const
//
// Transforms the given vector (treating it as a point, not a vector) by multiplying 
// it by the matrix and returns the result in result
//=============================================================================
void BSimpleMatrix::transformVectorAsPoint(const BVector &vect, BVector &result) const
{
   BASSERT(&vect != &result);
   result.x = m[0][0]*vect.x + m[0][1]*vect.y + m[0][2]*vect.z + m[0][3];
   result.y = m[1][0]*vect.x + m[1][1]*vect.y + m[1][2]*vect.z + m[1][3];
   result.z = m[2][0]*vect.x + m[2][1]*vect.y + m[2][2]*vect.z + m[2][3];

} // BSimpleMatrix::transformVectorAsPoint


//=============================================================================
// BSimpleMatrix::transformVectorAsPoint
//
// Transforms the given vector (treating it as a point, not a vector) by multiplying 
// it by the matrix and returns the result in result
//=============================================================================
void BSimpleMatrix::transformVectorAsPoint(const BVector &vect, BVector4 &result) const
{
   result.x = m[0][0]*vect.x + m[0][1]*vect.y + m[0][2]*vect.z + m[0][3];
   result.y = m[1][0]*vect.x + m[1][1]*vect.y + m[1][2]*vect.z + m[1][3];
   result.z = m[2][0]*vect.x + m[2][1]*vect.y + m[2][2]*vect.z + m[2][3];
}


//=============================================================================
// BSimpleMatrix::transformVectorAsPoint
//
// Transforms the given vector (treating it as a point, not a vector) by multiplying 
// it by the matrix and returns the result in result
//=============================================================================
void BSimpleMatrix::transformVectorAsPoint(const BVector2 &vect, BVector &result) const
{
   result.x = m[0][0]*vect.x + m[0][1]*vect.y + m[0][3];
   result.y = m[1][0]*vect.x + m[1][1]*vect.y + m[1][3];
   result.z = m[2][0]*vect.x + m[2][1]*vect.y + m[2][3];
}

//=============================================================================
// BSimpleMatrix::transformVectorListAsPoint
//=============================================================================
void BSimpleMatrix::transformVectorListAsPoint(const BVector *vect, BVector4 *result, const long num) const
{
   for(long i=0; i<num; i++, vect++, result++)
   {
      result->z = m[2][0]*vect->x + m[2][1]*vect->y + m[2][2]*vect->z + m[2][3];
      result->y = m[1][0]*vect->x + m[1][1]*vect->y + m[1][2]*vect->z + m[1][3];
      result->x = m[0][0]*vect->x + m[0][1]*vect->y + m[0][2]*vect->z + m[0][3];
      result->w = 1.0f/result->z;
   }
}

//=============================================================================
// BSimpleMatrix::transformVectorListAsPoint
//=============================================================================
void BSimpleMatrix::transformVectorListAsPoint(const BVector *vect, BVector *result, const long num) const
{
   BASSERT(vect != result);
   for(long i=num-1; i>=0; i--, vect++, result++)
   {
      result->x = m[0][0]*vect->x + m[0][1]*vect->y + m[0][2]*vect->z + m[0][3];
      result->y = m[1][0]*vect->x + m[1][1]*vect->y + m[1][2]*vect->z + m[1][3];
      result->z = m[2][0]*vect->x + m[2][1]*vect->y + m[2][2]*vect->z + m[2][3];
   }
}

//=============================================================================
// BSimpleMatrix::transformShortVectorListAsPoint
//=============================================================================
void BSimpleMatrix::transformShortVectorListAsPoint(const BVector *vect, BShortVector *result, const long num) const
{
   for(long i=num-1; i>=0; i--, vect++, result++)
   {
      result->mx = m[0][0]*vect->x + m[0][1]*vect->y + m[0][2]*vect->z + m[0][3];
      result->my = m[1][0]*vect->x + m[1][1]*vect->y + m[1][2]*vect->z + m[1][3];
      result->mz = m[2][0]*vect->x + m[2][1]*vect->y + m[2][2]*vect->z + m[2][3];
   }
}

//=============================================================================
// BSimpleMatrix::transformShortVectorListAsPoint
//=============================================================================
void BSimpleMatrix::transformShortVectorListAsPoint(const BShortVector *vect, BShortVector *result, const long num) const
{
   BASSERT(vect != result);
   for(long i=num-1; i>=0; i--, vect++, result++)
   {
      result->mx = m[0][0]*vect->mx.asFloat() + m[0][1]*vect->my.asFloat() + m[0][2]*vect->mz.asFloat() + m[0][3];
      result->my = m[1][0]*vect->mx.asFloat() + m[1][1]*vect->my.asFloat() + m[1][2]*vect->mz.asFloat() + m[1][3];
      result->mz = m[2][0]*vect->mx.asFloat() + m[2][1]*vect->my.asFloat() + m[2][2]*vect->mz.asFloat() + m[2][3];
   }
}

//=============================================================================
// BSimpleMatrix::transformShortVectorListAsPoint
//=============================================================================
void BSimpleMatrix::transformShortVectorListAsPoint(const BShortVector *vect, BVector *result, const long num) const
{
   for(long i=num-1; i>=0; i--, vect++, result++)
   {
      result->x = m[0][0]*vect->mx.asFloat() + m[0][1]*vect->my.asFloat() + m[0][2]*vect->mz.asFloat() + m[0][3];
      result->y = m[1][0]*vect->mx.asFloat() + m[1][1]*vect->my.asFloat() + m[1][2]*vect->mz.asFloat() + m[1][3];
      result->z = m[2][0]*vect->mx.asFloat() + m[2][1]*vect->my.asFloat() + m[2][2]*vect->mz.asFloat() + m[2][3];
   }
}

//=============================================================================
// BSimpleMatrix::transformOverlappingVectorListAsPoint
//
// This version is safe when vect==result.
//=============================================================================
void BSimpleMatrix::transformOverlappingVectorListAsPoint(const BVector *vect, BVector *result, const long num) const
{
   float x,y,z;
   for(long i=num-1; i>=0; i--, vect++, result++)
   {
      x=vect->x;
      y=vect->y;
      z=vect->z;

      result->x = m[0][0]*x + m[0][1]*y + m[0][2]*z + m[0][3];
      result->y = m[1][0]*x + m[1][1]*y + m[1][2]*z + m[1][3];
      result->z = m[2][0]*x + m[2][1]*y + m[2][2]*z + m[2][3];
   }
}

//=============================================================================
// BSimpleMatrix::transformOverlappingShortVectorListAsPoint
//
// This version is safe when vect==result.
//=============================================================================
void BSimpleMatrix::transformOverlappingShortVectorListAsPoint(const BShortVector *vect, BShortVector *result, const long num) const
{
   float x,y,z;
   for(long i=num-1; i>=0; i--, vect++, result++)
   {
      x=vect->mx.asFloat();
      y=vect->my.asFloat();
      z=vect->mz.asFloat();

      result->mx = m[0][0]*x + m[0][1]*y + m[0][2]*z + m[0][3];
      result->my = m[1][0]*x + m[1][1]*y + m[1][2]*z + m[1][3];
      result->mz = m[2][0]*x + m[2][1]*y + m[2][2]*z + m[2][3];
   }
}

//=============================================================================
// 
//=============================================================================
void BSimpleMatrix::SwapRows(float *pmat, DWORD i, DWORD row)
{
   float ftemp[8];

   memcpy(&ftemp[0], &pmat[8 * i], sizeof(float) * 8);

   memcpy(&pmat[8 * i], &pmat[8 * row], sizeof(float) * 8);

   memcpy(&pmat[8 * row], &ftemp[0], sizeof(float) * 8);

} // BSimpleMatrix::SwapRows

//=============================================================================
bool BSimpleMatrix::invert(void)
{
   float matrix[4][8];

   memset(matrix, 0, sizeof(matrix));

   // copy current matrix

   matrix[0][0] = m[0][0];
   matrix[0][1] = m[0][1];
   matrix[0][2] = m[0][2];
   matrix[0][3] = m[0][3];

   matrix[1][0] = m[1][0];
   matrix[1][1] = m[1][1];
   matrix[1][2] = m[1][2];
   matrix[1][3] = m[1][3];

   matrix[2][0] = m[2][0];
   matrix[2][1] = m[2][1];
   matrix[2][2] = m[2][2];
   matrix[2][3] = m[2][3];

   // fill in missing row to make 4x4

   matrix[3][3] = 1.0f;

   // fill in identity in b

   matrix[0][4] = 1.0f;
   matrix[1][5] = 1.0f;
   matrix[2][6] = 1.0f;
   matrix[3][7] = 1.0f;

   // step through each row

   long i, j, row;
   float multiple, divisor;

   for (row = 0; row < 4; row++)
   {

      // make sure that matrix[row][row] != 0

      if (matrix[row][row] == 0.0f)
      {
         for (i = row + 1; i < 4; i++)
         {
            if (matrix[i][row] != 0.0f)
            {
               SwapRows(&matrix[0][0], i, row);
               break;
            }
         }
      
      } // if (matrix[row][row] == 0.0f)

      // divide this row by a constant so that matrix[row][row] = 1

      divisor = matrix[row][row];
      if (divisor == 0.0f)
         return false;

      for (j = 0; j < 8; j++)
      {
         matrix[row][j] = matrix[row][j] / divisor;

      } // for (j = 0; j < 8; j++)

      // make all other elements in column "row" a 0

      for (i = 0; i < 3; i++)
      {
         if (i != row)
         {
            multiple = matrix[i][row];

            for (j = 0; j < 8; j++)
            {
               matrix[i][j] = matrix[i][j] - multiple * matrix[row][j];

            } // for (j = 0; j < 8; j++)

         }

      } // for (i = 0; i < 3; i++)

   } // for (row = 0; row < 3; row++)

   // is it okay?

   // assign b out to m

   m[0][0] = matrix[0][4];
   m[0][1] = matrix[0][5];
   m[0][2] = matrix[0][6];
   m[0][3] = matrix[0][7];

   m[1][0] = matrix[1][4];
   m[1][1] = matrix[1][5];
   m[1][2] = matrix[1][6];
   m[1][3] = matrix[1][7];

   m[2][0] = matrix[2][4];
   m[2][1] = matrix[2][5];
   m[2][2] = matrix[2][6];
   m[2][3] = matrix[2][7];

   return true;

} // BSimpleMatrix::invert


//=============================================================================
// BSimpleMatrix::makeReflect
//
// Normal is expected to be normalized!
//=============================================================================
void BSimpleMatrix::makeReflect(const BVector &point, const BVector &normal)
{
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
}


//=============================================================================
// BSimpleMatrix::multReflect
//=============================================================================
void BSimpleMatrix::multReflect(const BVector &point, const BVector &normal)
{
   BSimpleMatrix matrixTemp12;
   matrixTemp12.makeReflect(point, normal);
   this->mult(*this, matrixTemp12);
}


//=============================================================================
// eof: simplematrix.cpp
//=============================================================================
