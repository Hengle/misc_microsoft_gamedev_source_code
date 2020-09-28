//============================================================================
//
// File: generalMatrix.h
// Copyright (c) 2005-2006, Ensemble Studios
//
//============================================================================
#pragma once

#include "generalVector.h"

// Column vector: 1  x 4  times 4  x 3 = 1 x 3
//    Row vector: 4  x 3  times 3  x 1 = 4 x 1
// ColumnVector4 * Matrix44 is the usual usage.

template<int rows, int cols, typename scalarType = float>
struct BMatrixNxN
{
   enum { numRows = rows };
   enum { numCols = cols };
   enum { requiresAlignment = false };
   enum { isSquare = (numRows == numCols) };
   typedef scalarType scalarType;            
   
   // RowVec and RowVecMatrix have the same memory footprint (both are row vecs)
   typedef BVecN<cols, scalarType>             RowVec; 
   typedef BVecN<rows, scalarType>             ColVec; 
   typedef BMatrixNxN<1, cols, scalarType>     RowVecMatrix;
   typedef BMatrixNxN<rows, 1, scalarType>     ColVecMatrix;
   typedef BMatrixNxN<rows, cols, scalarType>  MatrixType;
   typedef BMatrixNxN<cols, rows, scalarType>  TransposedMatrixType;
   
   RowVec mRow[rows];
   
   BMatrixNxN()
   {
   }
   
   explicit BMatrixNxN(const scalarType* pData)
   {
      setFromPtr(pData);
   }
   
   explicit BMatrixNxN(const RowVec& row)
   {
      BCOMPILETIMEASSERT(rows == 1);
      mRow[0] = row;
   }
   
   BMatrixNxN(const RowVec& row0, const RowVec& row1)
   {
      BCOMPILETIMEASSERT(rows == 2);
      mRow[0] = row0;
      mRow[1] = row1;
   }
   
   BMatrixNxN(const RowVec& row0, const RowVec& row1, const RowVec& row2)
   {
      BCOMPILETIMEASSERT(rows == 3);
      mRow[0] = row0;
      mRow[1] = row1;
      mRow[2] = row2;
   }
   
   BMatrixNxN(const RowVec& row0, const RowVec& row1, const RowVec& row2, const RowVec& row3)
   {
      BCOMPILETIMEASSERT(rows == 4);
      mRow[0] = row0;
      mRow[1] = row1;
      mRow[2] = row2;
      mRow[3] = row3;
   }

   BMatrixNxN(const MatrixType& b)
   {
      for (int row = 0; row < rows; row++)
         mRow[row] = b.mRow[row];
   }
   
   template<int OtherRows, int OtherCols, typename OtherscalarType>
   BMatrixNxN(const BMatrixNxN<OtherRows, OtherCols, OtherscalarType>& b)
   {
      setIdentity(OtherRows, OtherCols);
      
      const int rowsToCopy = Math::Min(OtherRows, rows);
               
      for (int row = 0; row < rowsToCopy; row++)
         mRow[row] = b.mRow[row];
   }
   
   BMatrixNxN(
      scalarType e00, scalarType e01, 
      scalarType e10, scalarType e11)
   {
      setElements(
         e00, e01,  
         e10, e11,)
   }
   
   BMatrixNxN(
      scalarType e00, scalarType e01, scalarType e02, 
      scalarType e10, scalarType e11, scalarType e12, 
      scalarType e20, scalarType e21, scalarType e22)
   {
      setElements(
         e00, e01, e02,  
         e10, e11, e12,  
         e20, e21, e22);
   }
                    
   BMatrixNxN(
      scalarType e00, scalarType e01, scalarType e02, scalarType e03, 
      scalarType e10, scalarType e11, scalarType e12, scalarType e13, 
      scalarType e20, scalarType e21, scalarType e22, scalarType e23, 
      scalarType e30, scalarType e31, scalarType e32, scalarType e33)
   {
      setElements(
         e00, e01, e02, e03, 
         e10, e11, e12, e13, 
         e20, e21, e22, e23, 
         e30, e31, e32, e33);
   }

#if 0   
   BMatrixNxN(const BOptMatrix44& b)
   {
      *this = b;
   }
   
   MatrixType& operator= (const BOptMatrix44& rhs)
   {
      BCOMPILETIMEASSERT((rows == 4) && (cols == 4));
      for (int i = 0; i < 16; i++)
         reinterpret_cast<scalarType*>(this)[i] = reinterpret_cast<const float*>(&rhs)[i];                  
      return *this;
   }
   
   operator BOptMatrix44() const
   {
      BCOMPILETIMEASSERT((rows == 4) && (cols == 4));
      BOptMatrix44 ret;
      for (int i = 0; i < 16; i++)
         reinterpret_cast<float*>(&ret)[i] = reinterpret_cast<const scalarType*>(this)[i];                  
      return ret;
   }
#endif   
   
   MatrixType& setElements(
      scalarType e00, scalarType e01,
      scalarType e10, scalarType e11)
   {
      BCOMPILETIMEASSERT(rows == 2);
      BCOMPILETIMEASSERT(cols == 2);
      mRow[0][0] = e00; mRow[0][1] = e01;	
      mRow[1][0] = e10; mRow[1][1] = e11;	
      return *this;
   }

   MatrixType& setElements(
      scalarType e00, scalarType e01, scalarType e02,  
      scalarType e10, scalarType e11, scalarType e12, 
      scalarType e20, scalarType e21, scalarType e22)
   {
      BCOMPILETIMEASSERT(rows == 3);
      BCOMPILETIMEASSERT(cols == 3);
      mRow[0][0] = e00; mRow[0][1] = e01;	mRow[0][2] = e02; 
      mRow[1][0] = e10; mRow[1][1] = e11;	mRow[1][2] = e12; 
      mRow[2][0] = e20; mRow[2][1] = e21;	mRow[2][2] = e22; 
      return *this;
   }

   MatrixType& setElements(
      scalarType e00, scalarType e01, scalarType e02, scalarType e03, 
      scalarType e10, scalarType e11, scalarType e12, scalarType e13, 
      scalarType e20, scalarType e21, scalarType e22, scalarType e23, 
      scalarType e30, scalarType e31, scalarType e32, scalarType e33)
   {
      BCOMPILETIMEASSERT(rows == 4);
      BCOMPILETIMEASSERT(cols == 4);
      mRow[0][0] = e00; mRow[0][1] = e01;	mRow[0][2] = e02; mRow[0][3] = e03;
      mRow[1][0] = e10; mRow[1][1] = e11;	mRow[1][2] = e12; mRow[1][3] = e13;
      mRow[2][0] = e20; mRow[2][1] = e21;	mRow[2][2] = e22; mRow[2][3] = e23;
      mRow[3][0] = e30; mRow[3][1] = e31;	mRow[3][2] = e32; mRow[3][3] = e33;
      return *this;
   }
         
   MatrixType& operator= (const MatrixType& rhs)
   {
      for (int row = 0; row < rows; row++)
         mRow[row] = rhs.mRow[row];
      return *this;
   }

   template<int OtherRows, int OtherCols, typename OtherscalarType>      
   MatrixType& operator= (const BMatrixNxN<OtherRows, OtherCols, OtherscalarType>& rhs)
   {
      setIdentity(OtherRows, OtherCols);

      const int rowsToCopy = Math::Min(OtherRows, rows);

      for (int row = 0; row < rowsToCopy; row++)
         mRow[row] = static_cast<scalarType>(rhs.mRow[row]);

      return *this;
   }
   
   const scalarType  operator() (int r, int c) const  { return mRow[debugRangeCheck(r, rows)][c]; }
         scalarType& operator() (int r, int c)        { return mRow[debugRangeCheck(r, rows)][c]; }

   const RowVec& operator[] (int r) const      { return mRow[debugRangeCheck(r, rows)]; }
         RowVec& operator[] (int r)            { return mRow[debugRangeCheck(r, rows)]; }
        
   const RowVec& getRow(int r) const           { return mRow[debugRangeCheck(r, rows)]; }
   MatrixType& setRow(int r, const RowVec& v)  { mRow[debugRangeCheck(r, rows)] = v; return *this; }
   
   MatrixType& setFromPtr(const scalarType* pData)
   {
      memcpy(&mRow[0], pData, rows * cols * sizeof(scalarType));
      return *this;
   }
         
   BVecN<rows> getColumn(int c) const               
   {
      debugRangeCheck(c, cols);
      BVecN<rows> ret;
      for (int r = 0; r < rows; r++)
         ret[r] = (*this)(r, c);
      return ret;
   }
   
   MatrixType& setColumn(int c, const BVecN<rows>& v)
   {
      debugRangeCheck(c, cols);
      for (int r = 0; r < rows; r++)
         (*this)(r, c) = v[r];
      return *this;
   }
   
   RowVecMatrix getRowAsRowMatrix(int r) const
   {
      debugRangeCheck(r, rows);
      RowVecMatrix ret;
      for (int c = 0; c < cols; c++)
         ret(0, c) = (*this)(r, c);
      return ret;
   }
   
   ColVecMatrix getRowAsColMatrix(int r) const
   {
      BCOMPILETIMEASSERT(isSquare);
      debugRangeCheck(r, rows);
      ColVecMatrix ret;
      for (int c = 0; c < cols; c++)
         ret(c, 0) = (*this)(r, c);
      return ret;
   }
   
   ColVecMatrix getColAsColMatrix(int c) const
   {
      debugRangeCheck(c, cols);
      ColVecMatrix ret;
      for (int r = 0; r < rows; r++)
         ret(r, 0) = (*this)(r, c);
      return ret;
   }
   
   RowVecMatrix getColAsRowMatrix(int c) const
   {
      BCOMPILETIMEASSERT(isSquare);
      debugRangeCheck(c, cols);
      ColVecMatrix ret;
      for (int r = 0; r < rows; r++)
         ret(0, r) = (*this)(r, c);
         return ret;
   }
   
   const RowVec& getTranslate(void) const { return getRow(rows - 1); }
   MatrixType& setTranslate(const RowVec& t) { setRow(rows - 1, t); return *this; }
   
   MatrixType& operator+= (const MatrixType& b)
   {
      for (int r = 0; r < rows; r++)
         mRow[r] += b[r];
      return *this;
   }
   
   MatrixType& operator-= (const MatrixType& b)
   {
      for (int r = 0; r < rows; r++)
         mRow[r] -= b[r];
      return *this;
   }

   MatrixType& operator*= (const MatrixType& b)
   {
      MatrixType temp;
      *this = multiplyToDest(temp, *this, b);
      return *this;
   }

   MatrixType& operator*= (scalarType s)
   {
      for (int r = 0; r < rows; r++)
         mRow[r] *= s;
      return *this;
   }

   friend MatrixType operator+ (const MatrixType& a, const MatrixType& b)
   {
      MatrixType ret;
      for (int r = 0; r < rows; r++)
         ret[r] = a[r] + b[r];
      return ret;
   }
   
   friend MatrixType operator- (const MatrixType& a, const MatrixType& b)
   {
      MatrixType ret;
      for (int r = 0; r < rows; r++)
         ret[r] = a[r] - b[r];
      return ret;
   }
   
   friend MatrixType operator* (const MatrixType& a, const MatrixType& b)
   {
      MatrixType ret;
      return multiplyToDest(ret, a, b);         
   }
   
   friend RowVec operator* (const ColVec& a, const MatrixType& b)
   {
      return transform(a, b);
   }

#if 0   
   friend BOptVec4 operator* (const BOptVec4& a, const MatrixType& b)
   {
      return transform(a, b);
   }
#endif   
   
   friend MatrixType operator* (const MatrixType& a, scalarType s)
   {
      MatrixType ret;
      for (int r = 0; r < rows; r++)
         ret[r] = a[r] * s;
      return ret;
   }
   
   friend MatrixType operator* (scalarType s, const MatrixType& a)
   {
      MatrixType ret;
      for (int r = 0; r < rows; r++)
         ret[r] = s * a[r];
      return ret;
   }
   
   friend MatrixType operator/ (const MatrixType& a, scalarType s)
   {
      MatrixType ret;
      for (int r = 0; r < rows; r++)
         ret[r] = a[r] / s;
      return ret;
   }

   MatrixType operator+ () const
   {
      return *this;
   }
   
   MatrixType operator- () const
   {
      MatrixType ret;
      for (int r = 0; r < rows; r++)
         ret[r] = -mRow[r];
      return ret;
   }

#if 0   
   MatrixType operator- (const MatrixType& b) const
   {
      MatrixType ret;
      for (int r = 0; r < rows; r++)
         ret[r] = (*this)[r] - b[r];
      return ret;
   }
#endif   
   
   MatrixType& setZero(void) 
   {
      for (int r = 0; r < rows; r++)
         mRow[r].setZero();
      return *this;
   }
   
   MatrixType& setIdentity(void) 
   {
      for (int r = 0; r < rows; r++)
      {
         mRow[r].setZero();
         if (r < cols)
            mRow[r][r] = 1.0f;
      }
      return *this;
   }
   
   MatrixType& setIdentity(int rowsToSkip, int colsToSkip) 
   {
#if 0      
      if ((rowsToSkip < rows) && (colsToSkip < cols))
      {
         for (int r = rowsToSkip; r < rows; r++)
            for (int c = colsToSkip; c < cols; c++)
               mRow[r][c] = (r == c) ? 1.0f : 0.0f;
      }
#endif
      if ((rowsToSkip < rows) && (colsToSkip < cols))
      {
         for (int r = 0; r < rowsToSkip; r++)
            for (int c = colsToSkip; c < cols; c++)
               mRow[r][c] = (r == c) ? 1.0f : 0.0f;
         
         for (int r = rowsToSkip; r < rows; r++)
            for (int c = 0; c < cols; c++)
               mRow[r][c] = (r == c) ? 1.0f : 0.0f;
      }         
      
      return *this;
   }
   
   MatrixType& setZero(int rowsToSkip, int colsToSkip) 
   {
#if 0      
      if ((rowsToSkip < rows) && (colsToSkip < cols))
      {
         for (int r = rowsToSkip; r < rows; r++)
            for (int c = colsToSkip; c < cols; c++)
               mRow[r][c] = 0.0f;
      }
#endif
      if ((rowsToSkip < rows) && (colsToSkip < cols))
      {
         for (int r = 0; r < rowsToSkip; r++)
            for (int c = colsToSkip; c < cols; c++)
               mRow[r][c] = 0.0f;
            
         for (int r = rowsToSkip; r < rows; r++)
            for (int c = 0; c < cols; c++)
               mRow[r][c] = 0.0f;
      }         

      return *this;
   }
   
   // in-place transpose
   MatrixType& transpose(void)
   {
      BCOMPILETIMEASSERT(isSquare);
      MatrixType temp(*this);
      for (int r = 0; r < rows; r++)
         for (int c = 0; c < cols; c++)
            (*this)[c][r] = temp[r][c];
      return *this;               
   }

   // in-place upper 3x3 transpose
   MatrixType& transpose3x3(void)
   {
      BCOMPILETIMEASSERT(rows >= 3 && cols >= 3);
      
      BMatrixNxN<3, 3, scalarType> temp(*this);
      for (int r = 0; r < 3; r++)
         for (int c = 0; c < 3; c++)
            (*this)[c][r] = temp[r][c];
   }

   // returns transpose of matrix
   TransposedMatrixType transposed(void) const
   {
      TransposedMatrixType ret;
      for (int r = 0; r < rows; r++)
         for (int c = 0; c < cols; c++)
            ret[c][r] = (*this)[r][c];
      return ret;
   }
               
   // Slow, uses Gaussian elimination.
   // 0 on failure, 1 on success (should be the matrix det., but it's not calculated here)
   scalarType invert(void)
   {
#ifdef XBOX   
      // rg [6/18/07] - This is a hack to work around a 360 compiler optimization bug.
      if ((sizeof(scalarType) == sizeof(float)) && (rows == 4) && (cols == 4))
      {
         float det;
         D3DXMatrixInverse((D3DXMATRIX*)this, &det, (D3DXMATRIX*)this);
         return 1.0f;
      }
#endif
      BCOMPILETIMEASSERT(isSquare);
      
      MatrixType a(*this);
      MatrixType b(makeIdentity());

      for (int c = 0; c < cols; c++)
      {
         int rMax = c;
         for (int r = c + 1; r < rows; r++)
            if (fabs(a[r][c]) > fabs(a[rMax][c]))
               rMax = r;

         if (0.0f == a[rMax][c])
         {
            setIdentity();
            return 0.0f;
         }

         std::swap(a[c], a[rMax]);
         std::swap(b[c], b[rMax]);

         b[c] /= a[c][c];
         a[c] /= a[c][c];

         for (int row = 0; row < rows; row++)
         {
            if (row != c)
            {
               const RowVec temp(a[row][c]);
               a[row] -= RowVec::multiply(a[c], temp);
               b[row] -= RowVec::multiply(b[c], temp);
            }
         }
      }

      *this = b;
      return 1.0f;
   }
   
   MatrixType inverse(void) const
   {
      MatrixType ret(*this);
      if (0.0f == ret.invert())
      {
         BASSERT(false);
         return makeIdentity();
      }
      return ret;
   }
   
   MatrixType inverseSlow(void) const
   {
      MatrixType ret(*this);
      if (0.0f == ret.invert())
      {
// rg [1/27/05] disabled for now because brender inits one of its matrices to all 0's or something         
//            BASSERT(false);
         return makeIdentity();
      }
      return ret;
   }
   
   static scalarType detRecursive(const MatrixType& a, int n)
   {
      scalarType d;
      MatrixType m;

      BASSERT(n > 1);

      // TODO: Make special cases for 3x3 and 4x4
      if (2 == n)
         d = a(0, 0) * a(1, 1) - a(1, 0) * a(0, 1);
      else 
      {
         d = 0;
         for (int j1 = 1; j1 <= n; j1++)
         {
            // create minor matrix
            for (int i = 2; i <= n; i++)
            {
               int j2 = 1;
               for (int j = 1; j <= n; j++)
               {
                  if (j == j1)
                     continue;
                  m(i - 1 - 1, j2 - 1) = a(i - 1, j - 1);
                  j2++;
               }
            }
            
            const scalarType sign = ((1 + j1) & 1) ? -1.0f : 1.0f;
               
            BASSERT(Math::EqualTol<scalarType>(pow(-1.0f, 1 + j1), sign));
            
            d += sign * a(1 - 1, j1 - 1) * detRecursive(m, n - 1);
         }
      }
      
      return d;
   }
      
   // Pretty slow for large matrices!
   scalarType det(void) const
   {
      BCOMPILETIMEASSERT(isSquare);
      return detRecursive(*this, rows);
   }

   // Orthonormalizes a 2x2, 3x3, or 4x4 rotation matrix (partial Gram-Schimidt).
   // For 4x4 matrices, the 4th column should be (0,0,0,1).
   void orthonormalize(void)
   {
      BCOMPILETIMEASSERT(isSquare);
      BCOMPILETIMEASSERT(rows >= 2 && rows <= 4);
      
      if (2 == rows)
      {
         RowVec x(getRow(0));
         RowVec y(getRow(1));
         x.normalize();
         y = RowVec::removeCompUnit(y, x).normalize();
         setRow(0, x);
         setRow(1, y);
      }
      else
      {
         RowVec x(getRow(0));
         RowVec y(getRow(1));
         RowVec z(getRow(2));
         x.normalize();
         y = RowVec::removeCompUnit(y, x).normalize();
         RowVec new_z(x % y);
         new_z.normalize();
         if ((new_z * z) < 0.0f)
            new_z *= -1.0f;
         setRow(0, x);
         setRow(1, y);
         setRow(2, new_z);
      }
   }
   
   // QR decomposition using modified Gram-Schmidt. cols must be <= rows.
   // Columns of q will be orthogonal to each other, r will be an upper triangular matrix, q * r = matrix.
   // false on failure
   bool QRDecomposition(BMatrixNxN<rows, cols, scalarType>& q, BMatrixNxN<cols, cols>& r) const
   {
      bool success = true;
      
      r.setZero();
      
      BCOMPILETIMEASSERT(cols <= rows);
      
      double l = sqrt(getColumn(0).dotPrecise(getColumn(0)));
      r(0, 0) = static_cast<scalarType>(l);
      if (0.0f != l)
         q.setColumn(0, getColumn(0) * (1.0f / l));
      else
      {
         q.setColumn(0, getColumn(0));
         success = false;
      }

      for (int j = 1; j < cols; j++)
      {
         double tmp[rows];
         for (int x = 0; x < rows; x++)
            tmp[x] = (*this)(x, j);
         
         for (int i = 0; i <= j - 1; i++)
         {
            l = tmp[0] * q(0, i);
            for (int x = 1; x < rows; x++)
               l += tmp[x] * q(x, i);
                                          
            r(i, j) = static_cast<scalarType>(l);
            
            for (int x = 0; x < rows; x++)
               tmp[x] = tmp[x] - l * q(x, i);
         }
         
         l = tmp[0] * tmp[0];
         for (int x = 1; x < rows; x++)
            l += tmp[x] * tmp[x];
         
         l = sqrt(l);
                  
         r(j, j) = static_cast<scalarType>(l);
         
         double ool = 1.0f;
         
         if (0.0f == l)
            success = false;
         else
            ool = 1.0f / l;
         
         for (int x = 0; x < rows; x++)
            q(x, j) = tmp[x] * ool;
      }
      
      return success;
   }
   
   // false on failure
   bool invertUpperTriangular(MatrixType& x) const
   {
      BCOMPILETIMEASSERT(cols == rows);
      
      x.setZero();
      
      for (int i = cols - 1; i >= 0; i--)
      {
         if (0.0f == (*this)(i, i))
            return false;
         
         x(i, i) = 1.0f / (*this)(i, i);
            
         for (int j = i + 1; j < cols; j++)
         {
            double sum = 0.0f;
            
            for (int k = i + 1; k <= j; k++)
               sum += x(k, j) * (*this)(i, k);
         
            if (0.0f == (*this)(j, j))
               return false;
            
            // Umm, wtf?
            //x(i, j) = -sum / (*this)(j, j);
            x(i, j) = -sum / (*this)(i, i);
         }               
      }
      
      return true;
   }
   
   scalarType maxAbsElement(void) const
   {
      scalarType maxVal = -Math::fNearlyInfinite;
      for (int r = 0; r < rows; r++)
         for (int c = 0; c < cols; c++)
            if (fabs((*this)(r, c)) > maxVal)
               maxVal = fabs((*this)(r, c));
      return maxVal;
   }
   
   scalarType minAbsElement(void) const
   {
      scalarType minVal = Math::fNearlyInfinite;
      for (int r = 0; r < rows; r++)
         for (int c = 0; c < cols; c++)
            if (fabs((*this)(r, c)) < minVal)
               minVal = fabs((*this)(r, c));
      return minVal;
   }

   BStream& writeText(BStream& stream) const
   {
      for (int r = 0; r < rows; r++)
      {
         for (int c = 0; c < cols; c++)
            stream.printf("%f ", (*this)(r, c));
         stream.printf("\n");
      }
      return stream;
   }
   
   void log(BTextDispatcher& dispatcher) const
   {
      for (int r = 0; r < rows; r++)
      {
         for (int c = 0; c < cols; c++)
            dispatcher.printf("%f ", (*this)(r, c));
         dispatcher.printf("\n");
      }
   }

   const scalarType* getPtr() const      { return reinterpret_cast<const scalarType*>(this); }
         scalarType* getPtr()            { return reinterpret_cast<scalarType*>(this); }
         
#ifdef DEBUG
   void debugCheck(void) const
   {
      for (int r = 0; r < rows; r++)
         mRow[r].debugCheck();
   }
#else
   void debugCheck(void) const
   {
   }
#endif

   bool isOrthonormal3x3(void) const
   {
      BVecN<3, scalarType> a(mRow[0].toVector());
      BVecN<3, scalarType> b(mRow[1].toVector());
      BVecN<3, scalarType> c(mRow[2].toVector());

      return 
         a.isUnit() &&  b.isUnit() && c.isUnit() &&
         Math::EqualTol(0.0f, a * b) && Math::EqualTol(0.0f, a * c) && Math::EqualTol(0.0f, b * c);
   }

   bool hasNoReflection3x3(void) const
   {
      BASSERT(isOrthonormal3x3());
      return BVecN<3, scalarType>::equalTol3(BVecN<3, scalarType>(mRow[0]) % BVecN<3, scalarType>(mRow[1]), BVecN<3, scalarType>(mRow[2]));
   }

   // handles equal sized, square matrices only
   static MatrixType& multiplyToDest(MatrixType& dest, const MatrixType& a, const MatrixType& b)
   {
      BCOMPILETIMEASSERT(isSquare);
      for (int r = 0; r < rows; r++)
      {
         for (int c = 0; c < cols; c++)
         {
            scalarType dotSum = 0;
            for (int i = 0; i < cols; i++)
               dotSum += a(r, i) * b(i, c);
            dest(r, c) = dotSum;
         }
      }                 
      return dest;
   }
         
   static MatrixType multiply(const MatrixType& a, const MatrixType& b)
   {
      BCOMPILETIMEASSERT(isSquare);
      MatrixType ret;
      multiplyToDest(ret, a, b);
      return ret;
   }

   // col_vec * matrix = row_vec
   // 1xA * AxB = 1xB
   static RowVec transform(const ColVec& a, const MatrixType& b)
   {
      RowVec ret(b[0] * a[0]);
      // uses mads, not dots
      for (int r = 1; r < rows; r++)
         ret += b[r] * a[r];
      return ret;
   }
   
   // Like transform(), except b is transposed. Square matrices only.
   static ColVec transformTransposed(const MatrixType& b, const ColVec& a)
   {
      BCOMPILETIMEASSERT(isSquare);
      ColVec ret;
      // uses dots
      for (int r = 0; r < rows; r++)
         ret[r] = b[r] * a;
      return ret;
   }

#if 0   
   static BOptVec4 transform(const BOptVec4& a, const MatrixType& b)
   {
      BCOMPILETIMEASSERT(rows == 4 && cols == 4);
      BOptVec4 ret(b[0] * a[0]);
      // uses mads, not dots
      for (int r = 1; r < rows; r++)
         ret += BOptVec4(b[r] * a[r]);
      return ret;
   }
#endif   
   
   // row_vec * matrix = row_vec
   // (row vector) 1xA * AxB = 1xB
   template<int size>
   static RowVec transformPoint(const BVecN<size>& a, const MatrixType& b)
   {
      ColVec c(a);

      c[ColVec::numElements - 1] = 1.0f;

      return transform(c, b);
   }
   
   template<int size>
   static RowVec transformPointTransposed(const MatrixType& b, const BVecN<size>& a)
   {
      ColVec c(a);

      c[ColVec::numElements - 1] = 1.0f;

      return transformTransposed(b, c);
   }

   // row_vec * matrix = row_vec
   // (row vector) 1xA * AxB = 1xB
   template<int size>
   static RowVec transformVector(const BVecN<size>& a, const MatrixType& b)
   {
      ColVec c(a);

      c[ColVec::numElements - 1] = 0.0f;

      return transform(c, b);
   }
   
   template<int size>
   static RowVec transformVectorTransposed(const MatrixType& b, const BVecN<size>& a)
   {
      ColVec c(a);

      c[ColVec::numElements - 1] = 0.0f;

      return transformTransposed(b, c);
   }
   
   // matrix * row_vec = col_vec
   // AxB * Bx1 = Ax1
   static ColVec transform(const MatrixType& b, const RowVec& a)
   {
      ColVec ret;
      // uses dots
      for (int r = 0; r < rows; r++)
         ret[r] = b[r] * a;
      return ret;
   }

#if 0   
   static BOptVec4 transform(const MatrixType& b, const BOptVec4& a)
   {
      BOptVec4 ret;
      // uses dots
      for (int r = 0; r < rows; r++)
         ret[r] = b[r] * a;
      return ret;
   }
#endif   
   
   static MatrixType makeScale(const RowVec& s)
   {
      MatrixType ret;
      for (int r = 0; r < rows; r++)
         for (int c = 0; c < cols; c++)
            ret[r][c] = (r == c) ? s[c] : 0.0f;
      return ret;
   }
   
   static MatrixType makeScale(scalarType s)
   {
      MatrixType ret;
      for (int r = 0; r < rows; r++)
         for (int c = 0; c < cols; c++)
            ret[r][c] = (r == c) ? s : 0.0f;
      return ret;
   }
   
   static MatrixType makeScale(float x, float y, float z)
   {
      MatrixType ret;
      ret.setIdentity();
      
      ret[0][0] = x;
      if ((rows >= 2) && (cols >= 2))
         ret[1][1] = y;
      if ((rows >= 3) && (cols >= 3))
         ret[2][2] = z;
         
      return ret;
   }

   static MatrixType makeIdentity(void)
   {
      MatrixType ret;
      ret.setIdentity();
      return ret;
   }

   static MatrixType makeZero(void)
   {
      MatrixType ret;
      ret.setZero();
      return ret;
   }      
               
   static MatrixType makeTranslate(const RowVec& translate)
   {
      MatrixType ret;
      ret.setIdentity();
      ret[rows - 1] = translate;
      return ret;
   }
   
   static MatrixType makeTranslate(float x, float y, float z)
   {
      MatrixType ret;
      ret.setIdentity();
      
      ret[rows - 1][0] = x;
      if (cols >= 2)
         ret[rows - 1][1] = y;
      if (cols >= 3)
         ret[rows - 1][2] = z;
            
      return ret;
   }
   
   static MatrixType makeRotate(const BVec3& axis, scalarType ang)
   {
      BCOMPILETIMEASSERT(rows >= 3 && cols >= 3);
      
      MatrixType ret;
      ret.setIdentity();

      BVec3 nrm(axis.normalized());

      double sinA = sin(ang);
      double cosA = cos(ang);
      double invCosA = 1.0f - cosA;

      const scalarType x = nrm[0];
      const scalarType y = nrm[1];
      const scalarType z = nrm[2];

      const double xSq = nrm[0] * nrm[0];
      const double ySq = nrm[1] * nrm[1];
      const double zSq = nrm[2] * nrm[2];
      
      ret[0][0] = (invCosA * xSq) + cosA;
      ret[0][1] = (invCosA * x * y) - (sinA * z);
      ret[0][2] = (invCosA * x * z) + (sinA * y);

      ret[1][0] = (invCosA * x * y) + (sinA * z);
      ret[1][1] = (invCosA * ySq) + cosA;
      ret[1][2] = (invCosA * y * z) - (sinA * x);

      ret[2][0] = (invCosA * x * z) - (sinA * y);
      ret[2][1] = (invCosA * y * z) + (sinA * x);
      ret[2][2] = (invCosA * zSq) + cosA;
      
      return ret;
   }
   
   // Rotation about the specified axis.
   // For 3x3 or larger matrices.
   // Sets to identity, then fills upper 3x3 with rotation.
   static MatrixType makeRotate(int axis, scalarType ang)
   {
      BCOMPILETIMEASSERT(rows >= 3 && cols >= 3);
      
      MatrixType ret;
      ret.setIdentity();
      
      const scalarType sinA = (scalarType)sin(ang);
      const scalarType cosA = (scalarType)cos(ang);

      switch (axis)
      {
         case 0:
            ret[1][1] =  cosA; ret[1][2] = -sinA;
            ret[2][1] =  sinA; ret[2][2] =  cosA;
            break;
         case 1:
            ret[0][0] =  cosA; ret[0][2] =  sinA;
            ret[2][0] = -sinA; ret[2][2] =  cosA;
            break;
         case 2:
            ret[0][0] =  cosA; ret[0][1] = -sinA; 
            ret[1][0] =  sinA; ret[1][1] =  cosA; 
            break;
         default:
            BASSERT(false);
            __assume(0);
      }
      
      return ret;
   }
   
   static MatrixType makeRotate(const BVec3& from, const BVec3& to)
   {
      BCOMPILETIMEASSERT(rows >= 3 && cols >= 3);
      
      BASSERT(from.isUnit());
      BASSERT(to.isUnit());

      MatrixType res;
      
      res.setIdentity();

      scalarType ang = from * to;
      if (Math::EqualTol<scalarType>(ang, 1.0f, Math::fTinyEpsilon))
      {
      }
      else if (Math::EqualTol<scalarType>(ang, -1.0f, Math::fTinyEpsilon))
      {
         BVec3 side(0.0f, from[2], -from[1]);

         if (Math::EqualTol<scalarType>(side * side, 0.0f, Math::fTinyEpsilon))
         {
            side[0] = -from[2];
            side[1] = 0.f;
            side[2] = from[0];
         }
         
         if (side.tryNormalize() == 0.0f)
         {
            BASSERT(false);
         }

         BVec3 up((side % from).normalize());
         
         res[0][0] = (up[0] * up[0]) - (from[0] * from[0]) - (side[0] * side[0]);
         res[0][1] = (up[0] * up[1]) - (from[0] * from[1]) - (side[0] * side[1]);
         res[0][2] = (up[0] * up[2]) - (from[0] * from[2]) - (side[0] * side[2]);

         res[1][0] = (up[0] * up[1]) - (from[0] * from[1]) - (side[0] * side[1]);
         res[1][1] = (up[1] * up[1]) - (from[1] * from[1]) - (side[1] * side[1]);
         res[1][2] = (up[1] * up[2]) - (from[1] * from[2]) - (side[1] * side[2]);

         res[2][0] = (up[0] * up[2]) - (from[0] * from[2]) - (side[0] * side[2]);
         res[2][1] = (up[1] * up[2]) - (from[1] * from[2]) - (side[1] * side[2]);
         res[2][2] = (up[2] * up[2]) - (from[2] * from[2]) - (side[2] * side[2]);
      }
      else
      {
         BVec3 v(from % to);
      
         const scalarType temp = (1.0f - ang) / v.norm();

         res[0][0] = ang + temp * v[0] * v[0];
         res[0][1] = temp * v[0] * v[1] + v[2];
         res[0][2] = temp * v[0] * v[2] - v[1];
         
         res[1][0] = temp * v[0] * v[1] - v[2];
         res[1][1] = ang + temp * v[1] * v[1];
         res[1][2] = temp * v[1] * v[2] + v[0];
         
         res[2][0] = temp * v[0] * v[2] + v[1];
         res[2][1] = temp * v[1] * v[2] - v[0];
         res[2][2] = ang + temp * v[2] * v[2];
      }

      return res;
   }
   
   // For 2x2 or larger matrices
   static MatrixType makeRotate(scalarType ang)
   {
      BCOMPILETIMEASSERT(rows >= 2 && cols >= 2);

      MatrixType ret;
      ret.setIdentity(2, 2);

      const scalarType sinA = (scalarType)sin(ang);
      const scalarType cosA = (scalarType)cos(ang);
      
      ret[0][0] =  cosA; ret[0][1] = -sinA; 
      ret[1][0] =  sinA; ret[1][1] =  cosA; 
         
      return ret;
   }
   
   static MatrixType makeRightToLeft(void)
   {
      BCOMPILETIMEASSERT(rows >= 3 && cols >= 3);
      MatrixType ret(makeIdentity());
      ret[2][2] = -1.0f;
      return ret;
    }
   
   static MatrixType makeCamera(const BVec4& pos, const BVec4& at, const BVec4& up, scalarType roll = 0.0f)
   {
      BCOMPILETIMEASSERT(rows == 4 && cols == 4);

      MatrixType rot;

      BASSERT(pos.isPoint());
      BASSERT(at.isPoint());

      BVec4 z(at - pos);
      if (z.tryNormalize() == 0.0f)
         z = BVec4(0,0,1,0);//gVec4ZOne;

      BVec4 y(up);

      if (!z[0] && !z[2])
         y = BVec4(-1.0f, 0.0f, 0.0f, 0.0f);

      if ((y * z) >= (1.0f - Math::fTinyEpsilon))
         y = BVec4(0,1,0,0);

      BVec4 x((y % z).normalize());

      y = (z % x).normalize();

      rot.setColumn(0, x);
      rot.setColumn(1, y);
      rot.setColumn(2, z);
      rot.setColumn(3, BVec4(0.0f, 0.0f, 0.0f, 1.0f));
      return BMatrixNxN<4, 4>::makeTranslate((-pos).toPoint()) * rot * BMatrixNxN<4, 4>::makeRotate(2, roll);
   }
   
   static MatrixType makeTensorProduct(const RowVec& v, const RowVec& w)
   {
      MatrixType ret;
      for (int r = 0; r < rows; r++)
        ret[r] = RowVec::multiply(v.broadcast(r), w);
      return ret;
   } 
 
   static MatrixType makeCrossProduct(const BVec3& w)
   {
      BCOMPILETIMEASSERT(rows >= 3 && cols >= 3);

      MatrixType ret;
      ret.setZero();
           
      ret[0][1] =  w[2];
      ret[0][2] = -w[1];

      ret[1][0] = -w[2];
      ret[1][2] =  w[0];

      ret[2][0] =  w[1];
      ret[2][1] = -w[0];

      return ret;
   }
 
    static MatrixType makeReflection(const BVec4& n, const BVec4& q)
    {
      BCOMPILETIMEASSERT(rows == 4 && cols == 4);
      MatrixType ret;
      BASSERT(n.isVector());
      BASSERT(q.isVector());
      ret = makeIdentity() - 2.0f * makeTensorProduct(n, n);
      ret.setTranslate(2.0f * (q * n) * n, 1.0f);
      return ret;
    }
        
    static MatrixType makeUniformScaling(const BVec4& q, scalarType c)
    {
       BCOMPILETIMEASSERT(rows == 4 && cols == 4);
       MatrixType ret;
       BASSERT(q.isVector());
       ret = c * makeIdentity();
       ret.setTranslate((1.0f - c) * q, 1.0f);
       return ret;
    }

   static MatrixType makeNonuniformScaling(const BVec4& q, scalarType c, const BVec4& w)
   {
      BCOMPILETIMEASSERT(rows == 4 && cols == 4);
      MatrixType ret;
      BASSERT(q.isVector());
      BASSERT(w.isVector());
      ret = makeIdentity() - (1.0f - c) * makeTensorProduct(w, w);
      ret.setTranslate((1.0f - c) * (q * w) * w, 1.0f);
      return ret;
   }

   // n = normal of plane, q = point on plane
   static MatrixType makeOrthoProjection(const BVec4& n, const BVec4& q)
   {
      MatrixType ret;
      BASSERT(n.isVector());
      BASSERT(q.isVector());
      ret = makeIdentity() - makeTensorProduct(n, n);
      ret.setTranslate((q * n) * n, 1.0f);
      return ret;
   }
  
   static MatrixType makeParallelProjection(const BVec4& n, const BVec4& q, const BVec4& w)
   {
      MatrixType ret;
      BASSERT(n.isVector());
      BASSERT(q.isVector());
      BASSERT(w.isVector());
      ret = makeIdentity() - (makeTensorProduct(n, w) / (w * n));
      ret.setTranslate(((q * n) / (w * n)) * w, 1.0f);
      return ret;
   }
      
   static MatrixType makeProjToScreen(int x, int y, int width, int height, scalarType minZ = 0.0f, scalarType maxZ = 1.0f)
   {
      BCOMPILETIMEASSERT(rows == 4 && cols == 4);
      
      MatrixType projToScreen;
      projToScreen.setColumn(0, BVec4(width * .5f, 0, 0, x + width * .5f));
      projToScreen.setColumn(1, BVec4(0.0f, height * -.5f, 0, y + height * .5f));
      projToScreen.setColumn(2, BVec4(0.0f, 0.0f, maxZ - minZ, minZ));
      projToScreen.setColumn(3, BVec4(0,0,0,1));
      
      return projToScreen;
   }  
   
   static MatrixType makePerspectiveFovLH(
      scalarType FovAngleY,
      scalarType AspectHByW,
      scalarType NearZ,
      scalarType FarZ)
   {
      BCOMPILETIMEASSERT(rows == 4 && cols == 4);
      
      BASSERT(!Math::EqualTol<scalarType>(FovAngleY, 0.0f, Math::fMinuteEpsilon));
      BASSERT(!Math::EqualTol<scalarType>(AspectHByW, 0.0f, Math::fMinuteEpsilon));
      BASSERT(!Math::EqualTol<scalarType>(FarZ, NearZ, Math::fMinuteEpsilon));
      BASSERT(NearZ > 0.0f);

      scalarType SinFov = sin(0.5f * FovAngleY);
      scalarType CosFov = cos(0.5f * FovAngleY);

      scalarType Height = CosFov / SinFov;
      scalarType Width = Height / AspectHByW;

      MatrixType viewToProj;
      viewToProj[0] = BVec4(Width, 0.0f, 0.0f, 0.0f);
      viewToProj[1] = BVec4(0.0f, Height, 0.0f, 0.0f);
      viewToProj[2] = BVec4(0.0f, 0.0f, FarZ / (FarZ - NearZ), 1.0f);
      viewToProj[3] = BVec4(0.0f, 0.0f, -viewToProj[2][2] * NearZ, 0.0f);       
      
      return viewToProj;
   }         
   
   static MatrixType makePerspectiveOffCenterLH(
      scalarType ViewLeft,
      scalarType ViewRight,
      scalarType ViewBottom,
      scalarType ViewTop,
      scalarType NearZ,
      scalarType FarZ)
   {
      BCOMPILETIMEASSERT(rows == 4 && cols == 4);
        
      BASSERT(!Math::EqualTol<scalarType>(ViewRight, ViewLeft, Math::fMinuteEpsilon));
      BASSERT(!Math::EqualTol<scalarType>(ViewTop, ViewBottom, Math::fMinuteEpsilon));
      BASSERT(!Math::EqualTol<scalarType>(FarZ, NearZ, Math::fMinuteEpsilon));
      BASSERT(NearZ > 0.0f);

      scalarType TwoNearZ = NearZ + NearZ;
      scalarType ReciprocalWidth = 1.0f / (ViewRight - ViewLeft);
      scalarType ReciprocalHeight = 1.0f / (ViewTop - ViewBottom);
      
      MatrixType viewToProj;

      viewToProj[0] = BVec4(TwoNearZ * ReciprocalWidth, 0.0f, 0.0f, 0.0f);
      viewToProj[1] = BVec4(0.0f, TwoNearZ * ReciprocalHeight, 0.0f, 0.0f);
      viewToProj[2] = BVec4(-(ViewLeft + ViewRight) * ReciprocalWidth, 
         -(ViewTop + ViewBottom) * ReciprocalHeight,
         FarZ / (FarZ - NearZ),
         1.0f);
      viewToProj[3] = BVec4(0.0f, 0.0f, -viewToProj[2][2] * NearZ, 0.0f);
      
      return viewToProj;
   }         
   
   static MatrixType makeOrthoOffCenterLH(scalarType l, scalarType r, scalarType b, scalarType t, scalarType zn, scalarType zf)
   {
      BCOMPILETIMEASSERT(rows == 4 && cols == 4);
      BASSERT(fabs(l - r) > Math::fMinuteEpsilon);
      BASSERT(fabs(b - t) > Math::fMinuteEpsilon);
      BASSERT(fabs(zn - zf) > Math::fMinuteEpsilon);
      
      MatrixType viewToProj;         
      viewToProj[0] = BVec4(2.0f / (r - l), 0.0f, 0.0f, 0.0f);
      viewToProj[1] = BVec4(0.0f, 2.0f / (t - b), 0.0f, 0.0f);
      viewToProj[2] = BVec4(0.0f, 0.0f, 1.0f / (zf - zn), 0.0f);
      viewToProj[3] = BVec4((l + r) / (l - r), (t + b) / (b - t), zn / (zn - zf), 1.0f);
               
      return viewToProj;
   }
   
   static bool equalTol(const MatrixType& a, const MatrixType& b, scalarType tol = Math::fSmallEpsilon)
   {
      for (int r = 0; r < rows; r++)
         for (int c = 0; c < cols; c++)
            if (!Math::EqualTol<scalarType>(a(r, c), b(r, c), tol))
               return false;
      return true;
   }
   
   static bool equalTol3x3(const MatrixType& a, const MatrixType& b, scalarType tol = Math::fSmallEpsilon)
   {
      for (int r = 0; r < Math::Min(3, rows); r++)
         for (int c = 0; c < Math::Min(3, cols); c++)
            if (!Math::EqualTol<scalarType>(a(r, c), b(r, c), tol))
               return false;
      return true;
   }
}; // class BMatrixNxN

// for multiplying nonequal/nonsquare sized matrices
// [A,B] * [C,D] = [A,D], B==C      
template<class A, class B, class C> C& BMatrixNxNMultiplyToDest(C& dest, const A& a, const B& b)
{
   BCOMPILETIMEASSERT(A::numCols == B::numRows);
        
   const int DstRows = A::numRows;
   const int DstCols = B::numCols;
   
   BCOMPILETIMEASSERT(C::numRows == DstRows);
   BCOMPILETIMEASSERT(C::numCols == DstCols);
   
   for (int r = 0; r < DstRows; r++)
   {
      for (int c = 0; c < DstCols; c++)
      {
         C::scalarType dotSum = 0;
         for (int i = 0; i < A::numCols; i++)
            dotSum += a(r, i) * b(i, c);
         dest(r, c) = dotSum;
      }
   }                 

   return dest;   
}

// for multiplying nonequal/nonsquare sized matrices
// [A,B] * [C,D] = [A,D], B==C   
template<class A, class B, class C> C BMatrixNxNMultiply(const A& a, const B& b)
{
   C ret;
   return BMatrixNxNMultiplyToDest(ret, a, b);
}

typedef BMatrixNxN<2, 2, float> BMatrix22;
typedef BMatrixNxN<3, 3, float> BMatrix33;
typedef BMatrixNxN<4, 3, float> BMatrix43;
typedef BMatrixNxN<3, 4, float> BMatrix34;
typedef BMatrixNxN<4, 4, float> BMatrix44;

typedef BMatrixNxN<2, 2, double> BMatrix22D;
typedef BMatrixNxN<3, 3, double> BMatrix33D;
typedef BMatrixNxN<4, 3, double> BMatrix43D;
typedef BMatrixNxN<3, 4, double> BMatrix34D;
typedef BMatrixNxN<4, 4, double> BMatrix44D;

template<class R> float BMatrixNxNInnerProduct(const R& a, const R& b)
{
   float dot = 0.0f;
   for (int r = 0; r < R::numRows; r++)
      for (int c = 0; c < R::numCols; c++)
         dot += a(r, c) * b(r, c);
   return dot;
}

// A is implicitly transposed
template <class R, class A, class B> R BMatrixNxNMultiplyATransposed(const A& aT, const B& b)
{
   BASSERT(A::numRows == B::numRows);

   R ret;
   const int dst_rows = A::numCols;
   const int dst_cols = B::numCols;

   BASSERT(R::numRows == dst_rows);
   BASSERT(R::numCols == dst_cols);

   const int dot_len = A::numRows;

   for (int r = 0; r < dst_rows; r++)
      for (int c = 0; c < dst_cols; c++)
      {
         float sum = 0;

         for (int k = 0; k < dot_len; k++)
            sum += aT(k, r) * b(k, c);

         ret(r, c) = sum;
      }

      return ret;
}

// B is implicitly transposed
template <class R, class A, class B> R BMatrixNxNMultiplyBTransposed(const A& a, const B& bT)
{
   BASSERT(A::numCols == B::numCols);

   R ret;
   const int dst_rows = A::numRows;
   const int dst_cols = B::numRows;

   BASSERT(R::numRows == dst_rows);
   BASSERT(R::numCols == dst_cols);

   const int dot_len = A::numCols;
   
   for (int r = 0; r < dst_rows; r++)
   {
      for (int c = 0; c < dst_cols; c++)
      {
         float sum = 0;

         for (int k = 0; k < dot_len; k++)
            sum += a(r, k) * bT(c, k);

         ret(r, c) = sum;
      }
   }

   return ret;
}

// For an M-by-M matrix A, T*A is an M-by-M matrix whose columns contain the 
// one-dimensional DCT of the columns of A. 
// The two-dimensional DCT of A can be computed as B=T*A*T'. 
// Since T is a real orthonormal matrix, its inverse is the same as its transpose. 
// Therefore, the inverse two-dimensional DCT of B is given by T'*B*T.
template<class R> R BMatrixNxNCreateDCT(void)
{
   R ret;
   BASSERT(R::numRows == R::numCols);

   const int N = R::numCols;

   for (int k = 0; k < N; k++)
   {
      for (int n = 0; n < N; n++)
      {
         double v;
         if (k == 0)
            v = 1.0f / sqrt(float(N));
         else
            v = sqrt(2.0f / float(N)) * cos((3.14159265358979f * (2.0f * float(n) + 1.0f) * float(k)) / (2.0f * float(N)));
         ret(k, n) = static_cast<float>(v);
      }
   }

   return ret;
}

template<class R> R BMatrixNxNDCT(const R& a, const R& dct)
{
   R temp(BMatrixNxNMultiply<R, R, R>(dct, a));
   return BMatrixNxNMultiplyBTransposed<R, R, R>(temp, dct);
}

template<class R> R BMatrixNxNIDCT(const R& b, const R& dct)
{
   R temp(BMatrixNxNMultiplyATransposed<R, R, R>(dct, b));
   return BMatrixNxNMultiply<R, R, R>(temp, dct);
}

