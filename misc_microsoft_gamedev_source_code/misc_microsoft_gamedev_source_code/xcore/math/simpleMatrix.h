//==============================================================================
// simplematrix.h
//
// Copyright (c) 1999 - 2005 Ensemble Studios
//==============================================================================
#ifndef _SIMPLE_MATRIX_H_
#define _SIMPLE_MATRIX_H_
//==============================================================================
// 
//==============================================================================
class BSimpleMatrix
{
   public:

      void setD3DXMatrix(const D3DMATRIX &d3dmatrix);
      void getD3DXMatrix(D3DMATRIX &d3dmatrix) const;
      
      inline void makeIdentity(void)
                  {
                     m[0][0] = 1.0f;   m[0][1] = 0.0f;   m[0][2] = 0.0f;   m[0][3] = 0.0f;
                     m[1][0] = 0.0f;   m[1][1] = 1.0f;   m[1][2] = 0.0f;   m[1][3] = 0.0f;
                     m[2][0] = 0.0f;   m[2][1] = 0.0f;   m[2][2] = 1.0f;   m[2][3] = 0.0f;
                  }

      inline void makeZero(void)
                  {
                     memset(m,0,12*sizeof(float));
                  } 

      inline void makeRotateX(float rads)
                  {
                     makeIdentity();
                     float sine = (float)sin(rads);
                     float cosine = (float)cos(rads);
                     m[1][1] = cosine;
                     m[1][2] = -sine;
                     m[2][1] = sine;
                     m[2][2] = cosine;
                  }

      inline void makeRotateXSinCos(float sine, float cosine)
                  {
                     makeIdentity();
                     m[1][1] = cosine;
                     m[1][2] = -sine;
                     m[2][1] = sine;
                     m[2][2] = cosine;
                  }

      inline void makeRotateY(float rads)
                  {
                     makeIdentity();
                     float sine = (float)sin(rads);
                     float cosine = (float)cos(rads);
                     m[0][0] = cosine;
                     m[0][2] = sine;
                     m[2][0] = -sine;
                     m[2][2] = cosine; 
                  }

      inline void makeRotateYSinCos(float sine, float cosine)
                  {
                     makeIdentity();
                     m[0][0] = cosine;
                     m[0][2] = sine;
                     m[2][0] = -sine;
                     m[2][2] = cosine; 
                  }

      inline void makeRotateZ(float rads)
                  {
                     makeIdentity();
                     float sine = (float)sin(rads);
                     float cosine = (float)cos(rads);
                     m[0][0] = cosine;
                     m[0][1] = -sine;
                     m[1][0] = sine;
                     m[1][1] = cosine;
                  }

      inline void makeRotateZSinCos(float sine, float cosine)
                  {
                     makeIdentity();
                     m[0][0] = cosine;
                     m[0][1] = -sine;
                     m[1][0] = sine;
                     m[1][1] = cosine;
                  }

      void makeRotateArbitrary(const float rads, const BVector &axis);
      void makeRotateYawPitchRoll(float yaw, float pitch, float roll);

      inline void makeScale(float xScale, float yScale, float zScale)
                  {
                     makeIdentity();
                     m[0][0] = xScale;
                     m[1][1] = yScale;
                     m[2][2] = zScale;
                  }

      inline void makeScale(float scale)
                  {
                     makeIdentity();
                     m[0][0] = scale;
                     m[1][1] = scale;
                     m[2][2] = scale;
                  }

      inline void makeView(const BVector &cameraLoc, const BVector &cameraDir, const BVector &cameraUp, const BVector &cameraRight)
                  {
                     m[0][0] = cameraRight.x;
                     m[0][1] = cameraRight.y;
                     m[0][2] = cameraRight.z;
                     m[0][3] = -cameraRight.x*cameraLoc.x-cameraRight.y*cameraLoc.y-cameraRight.z*cameraLoc.z;
                     m[1][0] = cameraUp.x;
                     m[1][1] = cameraUp.y;
                     m[1][2] = cameraUp.z;
                     m[1][3] = -cameraUp.x*cameraLoc.x-cameraUp.y*cameraLoc.y-cameraUp.z*cameraLoc.z;
                     m[2][0] = cameraDir.x;
                     m[2][1] = cameraDir.y;
                     m[2][2] = cameraDir.z;
                     m[2][3] = -cameraDir.x*cameraLoc.x-cameraDir.y*cameraLoc.y-cameraDir.z*cameraLoc.z;
                  }

      inline void makeInverseView(const BVector &cameraLoc, const BVector &cameraDir, const BVector &cameraUp, const BVector &cameraRight)
                  {
                     m[0][0] = cameraRight.x;
                     m[0][1] = cameraUp.x;
                     m[0][2] = cameraDir.x;
                     m[0][3] = cameraLoc.x;

                     m[1][0] = cameraRight.y;
                     m[1][1] = cameraUp.y;
                     m[1][2] = cameraDir.y;
                     m[1][3] = cameraLoc.y;

                     m[2][0] = cameraRight.z;
                     m[2][1] = cameraUp.z;
                     m[2][2] = cameraDir.z;
                     m[2][3] = cameraLoc.z;
                  }

      inline void makeOrient(const BVector &dir, const BVector &up, const BVector &right)
                  {
                     m[0][0] = right.x;
                     m[0][1] = up.x;
                     m[0][2] = dir.x;
                     m[0][3] = 0.0f;

                     m[1][0] = right.y;
                     m[1][1] = up.y;
                     m[1][2] = dir.y;
                     m[1][3] = 0.0f;

                     m[2][0] = right.z;
                     m[2][1] = up.z;
                     m[2][2] = dir.z;
                     m[2][3] = 0.0f;
                  }

      inline void makeInverseOrient(const BVector &dir, const BVector &up, const BVector &right)
                  {
                     m[0][0] = right.x;
                     m[0][1] = right.y;
                     m[0][2] = right.z;
                     m[0][3] = 0.0f;

                     m[1][0] = up.x;
                     m[1][1] = up.y;
                     m[1][2] = up.z;
                     m[1][3] = 0.0f;

                     m[2][0] = dir.x;
                     m[2][1] = dir.y;
                     m[2][2] = dir.z;
                     m[2][3] = 0.0f;
                  }

      inline void makeTranslate(const float tx, const float ty, const float tz)
                  {
                     m[0][0] = 1.0f;   m[0][1] = 0.0f;   m[0][2] = 0.0f;   m[0][3] = tx;
                     m[1][0] = 0.0f;   m[1][1] = 1.0f;   m[1][2] = 0.0f;   m[1][3] = ty;
                     m[2][0] = 0.0f;   m[2][1] = 0.0f;   m[2][2] = 1.0f;   m[2][3] = tz;
                  }

      inline void makeTranslate(const BVector& v)
      {
         m[0][0] = 1.0f;   m[0][1] = 0.0f;   m[0][2] = 0.0f;   m[0][3] = v.x;
         m[1][0] = 0.0f;   m[1][1] = 1.0f;   m[1][2] = 0.0f;   m[1][3] = v.y;
         m[2][0] = 0.0f;   m[2][1] = 0.0f;   m[2][2] = 1.0f;   m[2][3] = v.z;
      }

      void makeReflect(const BVector &point, const BVector &normal);

      inline void multTranslate(float tx, float ty, float tz)
                  {
                     m[0][3] += tx;
                     m[1][3] += ty;
                     m[2][3] += tz;
                  }

      void multRotateX(float rads);
      void multRotateXSinCos(float sine, float cosine);
      void multRotateY(float rads);
      void multRotateYSinCos(float sine, float cosine);
      void multRotateZ(float rads);
      void multRotateZSinCos(float sine, float cosine);
      void multRotateArbitrary(const float rads, const BVector &axis);

      inline void multScale(float xScale, float yScale, float zScale)
                  {
                     BSimpleMatrix matrixTemp9;
                     matrixTemp9.makeScale(xScale, yScale, zScale);
                     this->mult(*this, matrixTemp9);
                  }

      inline void multScale(float factor)
                  {
                     BSimpleMatrix matrixTemp10;
                     matrixTemp10.makeScale(factor, factor, factor);
                     this->mult(*this, matrixTemp10);
                  }

      inline void multOrient(const BVector &dir, const BVector &up, const BVector &right)
                  {
                     BSimpleMatrix matrixTemp11;
                     matrixTemp11.makeOrient(dir, up, right);
                     this->mult(*this, matrixTemp11);
                  }

      void multInverseOrient(const BVector &dir, const BVector &up, const BVector &right);
      void multReflect(const BVector &point, const BVector &normal);

      inline void mult(const BSimpleMatrix &matrix1, const BSimpleMatrix &matrix2)
                  {
                     // Note: this is not a standard matrix mult because we assume that the last
                     // row of the 4x4 matrix is 0 0 0 1 and therefore don't store it or waste time
                     // doing computations using it.   

                     /*for(long i=0; i<3; i++)
                        for(long j=0; j<3; j++)
                           m[i][j] = matrix1.m[i][0]*matrix2.m[0][j] + matrix1.m[i][1]*matrix2.m[1][j] + matrix1.m[i][2]*matrix2.m[2][j];

                     for(i=0; i<3; i++)
                        m[i][3] = matrix1.m[i][0]*matrix2.m[0][3] + matrix1.m[i][1]*matrix2.m[1][3] + matrix1.m[i][2]*matrix2.m[2][3] + matrix1.m[i][3];*/
                     
                     BSimpleMatrix result;  
                     
                     result.m[0][0] = matrix2.m[0][0]*matrix1.m[0][0] + matrix2.m[0][1]*matrix1.m[1][0] + matrix2.m[0][2]*matrix1.m[2][0];
                     result.m[0][1] = matrix2.m[0][0]*matrix1.m[0][1] + matrix2.m[0][1]*matrix1.m[1][1] + matrix2.m[0][2]*matrix1.m[2][1];
                     result.m[0][2] = matrix2.m[0][0]*matrix1.m[0][2] + matrix2.m[0][1]*matrix1.m[1][2] + matrix2.m[0][2]*matrix1.m[2][2];
                     result.m[0][3] = matrix2.m[0][0]*matrix1.m[0][3] + matrix2.m[0][1]*matrix1.m[1][3] + matrix2.m[0][2]*matrix1.m[2][3] + matrix2.m[0][3];

                     result.m[1][0] = matrix2.m[1][0]*matrix1.m[0][0] + matrix2.m[1][1]*matrix1.m[1][0] + matrix2.m[1][2]*matrix1.m[2][0];
                     result.m[1][1] = matrix2.m[1][0]*matrix1.m[0][1] + matrix2.m[1][1]*matrix1.m[1][1] + matrix2.m[1][2]*matrix1.m[2][1];
                     result.m[1][2] = matrix2.m[1][0]*matrix1.m[0][2] + matrix2.m[1][1]*matrix1.m[1][2] + matrix2.m[1][2]*matrix1.m[2][2];
                     result.m[1][3] = matrix2.m[1][0]*matrix1.m[0][3] + matrix2.m[1][1]*matrix1.m[1][3] + matrix2.m[1][2]*matrix1.m[2][3] + matrix2.m[1][3];

                     result.m[2][0] = matrix2.m[2][0]*matrix1.m[0][0] + matrix2.m[2][1]*matrix1.m[1][0] + matrix2.m[2][2]*matrix1.m[2][0];
                     result.m[2][1] = matrix2.m[2][0]*matrix1.m[0][1] + matrix2.m[2][1]*matrix1.m[1][1] + matrix2.m[2][2]*matrix1.m[2][1];
                     result.m[2][2] = matrix2.m[2][0]*matrix1.m[0][2] + matrix2.m[2][1]*matrix1.m[1][2] + matrix2.m[2][2]*matrix1.m[2][2];
                     result.m[2][3] = matrix2.m[2][0]*matrix1.m[0][3] + matrix2.m[2][1]*matrix1.m[1][3] + matrix2.m[2][2]*matrix1.m[2][3] + matrix2.m[2][3];
                     
                     *this = result;
                  }

      inline BVector operator*(const BVector &vec) const
                  {
                  //   BVector ret(m[0][0]*vec.x + m[0][1]*vec.y + m[0][2]*vec.z,
                  //               m[1][0]*vec.x + m[1][1]*vec.y + m[1][2]*vec.z,
                  //               m[2][0]*vec.x + m[2][1]*vec.y + m[2][2]*vec.z);

                  // ret.x = m[0][0]*vec.x + m[0][1]*vec.y + m[0][2]*vec.z;
                  //   ret.y = m[1][0]*vec.x + m[1][1]*vec.y + m[1][2]*vec.z;
                  // ret.z = m[2][0]*vec.x + m[2][1]*vec.y + m[2][2]*vec.z;

                     BSimpleVector vectorTemp;
                     vectorTemp.x = m[0][0]*vec.x + m[0][1]*vec.y + m[0][2]*vec.z;
                     vectorTemp.y = m[1][0]*vec.x + m[1][1]*vec.y + m[1][2]*vec.z;
                     vectorTemp.z = m[2][0]*vec.x + m[2][1]*vec.y + m[2][2]*vec.z;

                     return vectorTemp;

                  } // BSimpleMatrix::operator*

      inline long operator==(const BSimpleMatrix &mat) const
                  {
                     return (memcmp(m,mat.m,12*sizeof(float)) == 0);
                  } 

      inline long operator!=(const BSimpleMatrix &mat) const
                  {
                     return (memcmp(m,mat.m,12*sizeof(float)) != 0);
                  }

      void transformVector(const BVector &vect, BVector &result) const;
      void transformVectorAsPoint(const BVector &vect, BVector &result) const;
      void transformVectorAsPoint(const BVector &vect, BVector4 &result) const;
      void transformVectorAsPoint(const BVector2 &vect, BVector &result) const;
      
      void transformVectorListAsPoint(const BVector *vect, BVector4 *result, const long num) const;
      void transformVectorListAsPoint(const BVector *vect, BVector *result, const long num) const;

      void transformShortVectorListAsPoint(const BShortVector *vect, BShortVector *result, const long num) const;
      void transformShortVectorListAsPoint(const BVector *vect, BShortVector *result, const long num) const;
      void transformShortVectorListAsPoint(const BShortVector *vect, BVector *result, const long num) const;

      void transformOverlappingVectorListAsPoint(const BVector *vect, BVector *result, const long num) const;
      void transformOverlappingShortVectorListAsPoint(const BShortVector *vect, BShortVector *result, const long num) const;

      void transformVectorList(const BVector *pvect, BVector *presult, DWORD nvertices) const;

      // calculates the inverse of this matrix
      bool invert(void);

      void transposeRotation()
                  {
                     bswap(m[1][0], m[0][1]);
                     bswap(m[2][0], m[0][2]);
                     bswap(m[2][1], m[1][2]);
                  }

      void clearTranslation()
                  {
                     m[0][3] = 0.0f;
                     m[1][3] = 0.0f;
                     m[2][3] = 0.0f;
                  }

      void setTranslation(const BVector& v)
                  {
                     m[0][3] = v.x;
                     m[1][3] = v.y;
                     m[2][3] = v.z;
                  }

      void setTranslation(float x, float y, float z)
                  {
                     m[0][3] = x;
                     m[1][3] = y;
                     m[2][3] = z;
                  }

      void getTranslation(BVector& v) const
                  {
                     v.x = m[0][3];
                     v.y = m[1][3];
                     v.z = m[2][3];
                  }

      void getForward(BVector& v) const
                  {
                     v.x = m[0][2];
                     v.y = m[1][2];
                     v.z = m[2][2];
                  }

      void getUp(BVector& v) const
                  {
                     v.x = m[0][1];
                     v.y = m[1][1];
                     v.z = m[2][1];
                  }

      void getRight(BVector& v) const
                  {
                     v.x = m[0][0];
                     v.y = m[1][0];
                     v.z = m[2][0];
                  }

      void getScale(float& x, float& y, float& z) const
                  {
                     x = m[0][0];
                     y = m[1][1];
                     z = m[2][2];
                  }

      BVector getRow(const int row) const
                  {
                     BSimpleVector vectorTemp;
                     vectorTemp.set(m[0][row], m[1][row], m[2][row]);
                     return vectorTemp;
                  }

      float getElement(const int row, const int column) const
                  {
                     return m[column][row];
                  }

      void setElement(const int row, const int column, const float value)
                  {
                     m[column][row] = value;
                  }

   private:
 
      void SwapRows(float *pmat, DWORD i, DWORD row);

      float m[3][4];

}; // BSimpleMatrix

#endif //_SIMPLE_MATRIX_H_
//==============================================================================
// eof: simplematrix.h
//==============================================================================
