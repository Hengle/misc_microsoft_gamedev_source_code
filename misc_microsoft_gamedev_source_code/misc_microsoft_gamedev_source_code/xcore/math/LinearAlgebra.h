//============================================================================
//
// File: LinearAlgebra.h
// Copyright (c) 2005-2006, Ensemble Studios
//
//============================================================================
#pragma once

namespace LinearAlgebra
{   
   // Functions from "Mathematics for 3D Game Programming and Computer Graphics" 2nd Edition.
   bool SolveLinearSystem(int n, float *m, float *r);
   bool LUDecompose(int n, float *m, unsigned short *index, float *detSign);
   void LUBacksubstitute(int n, const float *d, const unsigned short *index, const float *r, float *x);
   void LURefineSolution(int n, const float *m, const float *d, const unsigned short *index, const float *r, float *x);
   void SolveTridiagonalSystem(int n, const float *a, const float *b, const float *c, const float *r, float *x);
   
   struct Matrix3D
   {
      float n[3][3];

      float& operator()(int i, int j)
      {
         return (n[j][i]);
      }

      const float& operator()(int i, int j) const
      {
         return (n[j][i]);
      }

      void SetIdentity(void)
      {
         n[0][0] = n[1][1] = n[2][2] = 1.0F;
         n[0][1] = n[0][2] = n[1][0] = n[1][2] = n[2][0] = n[2][1] = 0.0F;
      }
   };
   
   void CalculateEigensystem(const Matrix3D& m, float *lambda, Matrix3D& r);
   
   float* newVector(uint nl, uint nh);
   void freeVector(float* p, uint nl, uint nh);
   
   float** newMatrix(uint r, uint c);
   void freeMatrix(float** p, uint r, uint c);
   
   template<class T>
   void setMatrix(float** p, const T& mat)
   {
      for (uint r = 0; r < T::numRows; r++)
         for (uint c = 0; c < T::numCols; c++)
            p[r+1][c+1] = mat[r][c];   
   }
   
   template<class T>
   void getMatrix(T& mat, const float* const * p)
   {
      for (uint r = 0; r < T::numRows; r++)
         for (uint c = 0; c < T::numCols; c++)
            mat[r][c] = p[r+1][c+1];
   }
   
   // Functions from "Numerical Recipes in C".
   
   // Given a matrix a[1..m][1..n], this routine computes its singular value decomposition, A =
   // U·W·V T. The matrix U replaces a on output. The diagonal matrix of singular values W is output
   // as a vector w[1..n]. The matrix V (not the transpose V T ) is output as v[1..n][1..n].
   bool svdcmp(float **a, int m, int n, float w[], float **v);
   
   // Solves A·X = B for a vector X, where A is specified by the arrays u[1..m][1..n], w[1..n],
   //   v[1..n][1..n] as returned by svdcmp. m and n are the dimensions of a, and will be equal for
   //   square matrices. b[1..m] is the input right-hand side. x[1..n] is the output solution vector.
   //   No input quantities are destroyed, so the routine may be called sequentially with different b’s.
   void svbksb(float **u, float w[], float **v, int m, int n, float b[], float x[]);
   
   // Jacobi rotation method to calculate EVD of symmetric matrix src
   template<int N>
   void JacobiEVD(const BMatrixNxN<N,N>& src, BVecN<N>& lambda, BMatrixNxN<N,N>& r)
   {
      BMatrixNxN<N,N> a(src);

      r.setIdentity();

      BVecN<N> b, d, z;
      for (uint i = 0; i < N; i++)
         b[i] = src[i][i];

      d = b;      
      z.setZero();

      const double epsilon = 1.0e-10F;
      const int maxSweeps = 50;

      for (int s = 0; s < maxSweeps; s++)
      {
         uint i;
         for (i = 0; i < N; i++)
         {
            if (fabs(d[i]) >= epsilon)
               break;
         }

         if (i == N)
            break;

         double sum = 0.0f;
         for (uint x = 0; x < N - 1; x++)
            for (uint y = x + 1; y < N; y++)
               sum += fabs(a[x][y]);
         if (sum == 0.0f)
            break;

         double thresh;
         if (i < 3)
            thresh = 0.2f * sum / (N * N);
         else
            thresh = 0.0f; 

         for (uint p = 0; p < N - 1; p++)
         {
            for (uint q = p + 1; q < N; q++)   
            {
               double g = 100.0f * fabs(a[p][q]);

               if ( (s > 3) && (fabs(d[p])+g == fabs(d[p])) && (fabs(d[q])+g == fabs(d[q])) )
               {
                  a[p][q] = 0.0f;
               }
               else if (fabs(a[p][q]) > thresh)
               {
                  // p,p = 1,2 or 0,1
                  // p=p, p=p

                  double h = d[q] - d[p];
                  double t;
                  if (fabs(h)+g == fabs(h))
                  {
                     t = a[p][q]/h;
                  }
                  else
                  {
                     //double u = .5f * h / a[p][q];
                     //double u2 = u * u;
                     //double u2p1 = u2 + 1.0F;
                     //t = (u2p1 != u2) ? ((u < 0.0F) ? -1.0f : 1.0f) * (fabs(u) + sqrt(u2p1)) : 0.5f / u;

                     //theta = 0.5_sp*h/a(ip,iq)
                     //   t = 1.0_sp/(ABS(theta)+SQRT(1.0_sp+theta**2))
                     //   IF(theta < 0.0) t = -t

                     double theta = .5f * h / a[p][q];

                     t = 1.0f/(fabs(theta)+sqrt(1.0f+theta*theta));
                     if (theta < 0.0f)
                        t = -t;
                  }

                  double c = 1.0f / sqrt(1.0f + t * t);
                  double s = c * t;
                  double tau = s / (1.0f + c);

                  h = t * a[p][q];

                  z[p] = z[p] - h;
                  z[q] = z[q] + h;
                  d[p] = d[p] - h;
                  d[q] = d[q] + h;

                  a[p][q] = 0.0f;

                  //float a1, a2;
                  for (uint i = 0; i < p; i++)
                  {
                     //a1 = a[i][p] - s * (a[i][q] + a[i][p] * tau);
                     //a2 = a[i][q] + s * (a[i][p] - a[i][q] * tau);

                     double temp = c * a[i][p] - s * a[i][q];
                     a[i][q] = s * a[i][p] + c * a[i][q];
                     a[i][p] = temp;
                  }

                  for (uint i = p + 1; i < q; i++)
                  {
                     double temp = c * a[p][i] - s * a[i][q];
                     a[i][q] = s * a[p][i] + c * a[i][q];
                     a[p][i] = temp;
                  }

                  for (uint i = q + 1; i < N; i++)
                  {
                     double temp = c * a[p][i] - s * a[q][i];
                     a[q][i] = s * a[p][i] + c * a[q][i];
                     a[p][i] = temp;
                  }

                  for (uint i = 0; i < N; i++)
                  {
                     double temp = c * r[i][p] - s * r[i][q];
                     r[i][q] = s * r[i][p] + c * r[i][q];
                     r[i][p] = temp;
                  }  
               }               
            }
         }

         b = b + z;
         d = b;
         z.setZero();
      }

      for (uint i = 0; i < N; i++)
         lambda[i] = d[i];
   }

   template<typename T, typename U, typename V>
   void CalculatePCA(const T& vecs, U& eigenvectors, V& eigenvalues, V& mean)
   {
      const uint N = V::numElements;
      typedef U MatrixType;
      typedef V VecType;

      eigenvalues.set(1.0f);
      eigenvectors.setIdentity();

      const uint numVecs = vecs.size();   
      if (!numVecs)
         return;

      mean.setZero();

      for (uint i = 0; i < numVecs; i++)
         mean += vecs[i];

      mean /= float(numVecs);

      //for (uint i = 0; i < N; i++)
      //   mean[i] = 128.0f;

      // Create covariance matrix
      
      MatrixType covar;
      covar.setZero();

      for (uint i = 0; i < numVecs; i++)
      {
         const V v(vecs[i] - mean);

         for (uint x = 0; x < N; x++)
            for (uint y = x; y < N; y++)
               covar[x][y] = covar[x][y] + v[x] * v[y];
      }

      for (uint x = 0; x < N - 1; x++)
         for (uint y = x + 1; y < N; y++)
            covar[y][x] = covar[x][y];

      for (uint x = 0; x < N; x++)
         for (uint y = 0; y < N; y++)         
            assert(covar[x][y]==covar[y][x]);

      covar *= 1.0f/float(numVecs);

      MatrixType temp(covar);

      // Now find eigenvectors of symmetric covariance matrix
      JacobiEVD<N>(covar, eigenvalues, eigenvectors);

#if 0
      // z = diagnol matrix of the eigenvalues
      BMatrixNxN<N,N> z(eigenvectors.transposed() * temp * eigenvectors);
#endif   

#if 0   
      //  A = V * D * VT, where A is the covar matrix, V is the eigenvector matrix, D is the diag eigenvalue matrix
      MatrixType d;
      d.setZero();
      for (uint i = 0; i < N; i++)
         d[i][i] = eigenvalues[i];
      MatrixType z(eigenvectors * d * eigenvectors.transposed());
      z.transpose();
#endif   

#if 0   
      float q;
      V k;
      V ev;
      for (uint i = 0; i < N; i++)
      {
         ev = eigenvectors.getColumn(i);
         k = ev * temp;
         q = k.len() / ev.len();
         k *= 1.0f/q;
      }

      for (uint i = 0; i < N - 1; i++)
      {
         for (uint j = i + 1; j < N; j++)
         {
            q = eigenvectors.getColumn(i) * eigenvectors.getColumn(j);
         }
      }
#endif   
   }
}   

