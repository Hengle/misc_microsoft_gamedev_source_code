//============================================================================
//
// File: LinearAlgebra.cpp
// Copyright (c) 2005-2006, Ensemble Studios
//
//============================================================================

#include "xcore.h"

#include "math\generalvector.h"
#include "math\generalmatrix.h"

#include "LinearAlgebra.h"

namespace LinearAlgebra
{
   bool SolveLinearSystem(int n, float *m, float *r)
   {
	   float *rowNormalizer = new float[n];
	   bool result = false;
   	
	   // Calculate a normalizer for each row
	   for (int i = 0; i < n; i++)
	   {
		   const float *entry = m + i;
		   float maxvalue = 0.0F;
   		
		   for (int j = 0; j < n; j++)
		   {
			   float value = fabs(*entry);
			   if (value > maxvalue) maxvalue = value;
			   entry += n;
		   }
   		
		   if (maxvalue == 0.0F) goto exit; // Singular
		   rowNormalizer[i] = 1.0F / maxvalue;
	   }
   	
	   // Perform elimination one column at a time
	   for (int j = 0; j < n - 1; j++)
	   {
		   // Find pivot element
		   int pivotRow = -1;
		   float maxvalue = 0.0F;
		   for (int i = j; i < n; i++)
		   {
			   float p = fabs(m[j * n + i]) * rowNormalizer[i];
			   if (p > maxvalue)
			   {
				   maxvalue = p;
				   pivotRow = i;
			   }
		   }
   		
		   if (pivotRow != j)
		   {
			   if (pivotRow == -1) goto exit; // Singular
   			
			   // Exchange rows
			   for (int k = 0; k < n; k++)
			   {
				   float temp = m[k * n + j];
				   m[k * n + j] = m[k * n + pivotRow];
				   m[k * n + pivotRow] = temp;
			   }
   			
			   float temp = r[j];
			   r[j] = r[pivotRow];
			   r[pivotRow] = temp;
   			
			   rowNormalizer[pivotRow] = rowNormalizer[j];
		   }
   		
		   float denom = 1.0F / m[j * n + j];
		   for (int i = j + 1; i < n; i++)
		   {
			   float factor = m[j * n + i] * denom;
			   r[i] -= r[j] * factor;
			   for (int k = 0; k < n; k++) m[k * n + i] -= m[k * n + j] * factor;
		   }
	   }
   	
	   // Perform backward substitution
	   for (int i = n - 1; i >= 0; i--)
	   {
		   float sum = r[i];
		   for (int k = i + 1; k < n; k++) sum -= m[k * n + i] * r[k];
		   r[i] = sum / m[i * n + i];
	   }

	   result = true;
   	
	   exit:
	   delete[] rowNormalizer;
	   return (result);
   }

   bool LUDecompose(int n, float *m, unsigned short *index, float *detSign)
   {
	   float *rowNormalizer = new float[n];
	   float exchangeParity = 1.0F;
	   bool result = false;
   	
	   // Calculate a normalizer for each row
	   for (int i = 0; i < n; i++)
	   {
		   const float *entry = m + i;
		   float maxvalue = 0.0F;
   		
		   for (int j = 0; j < n; j++)
		   {
			   float value = fabs(*entry);
			   if (value > maxvalue) maxvalue = value;
			   entry += n;
		   }
   		
		   if (maxvalue == 0.0F) goto exit; // Singular
		   rowNormalizer[i] = 1.0F / maxvalue;
		   index[i] = static_cast<ushort>(i);
	   }
   	
	   // Perform decomposition
	   for (int j = 0; j < n; j++)
	   {
		   for (int i = 1; i < j; i++)
		   {
			   // Evaluate Equation (14.15)
			   float sum = m[j * n + i];
			   for (int k = 0; k < i; k++) sum -= m[k * n + i] * m[j * n + k];
			   m[j * n + i] = sum;
		   }
   		
		   // Find pivot element
		   int pivotRow = -1;
		   float maxvalue = 0.0F;
		   for (int i = j; i < n; i++)
		   {
			   // Evaluate Equation (14.17)
			   float sum = m[j * n + i];
			   for (int k = 0; k < j; k++) sum -= m[k * n + i] * m[j * n + k];
			   m[j * n + i] = sum;
   			
			   sum = fabs(sum) * rowNormalizer[i];
			   if (sum > maxvalue)
			   {
				   maxvalue = sum;
				   pivotRow = i;
			   }
		   }
   		
		   if (pivotRow != j)
		   {
			   if (pivotRow == -1) goto exit; // Singular
   			
			   // Exchange rows
			   for (int k = 0; k < n; k++)
			   {
				   float temp = m[k * n + j];
				   m[k * n + j] = m[k * n + pivotRow];
				   m[k * n + pivotRow] = temp;
			   }
   			
			   unsigned short temp = index[j];
			   index[j] = index[pivotRow];
			   index[pivotRow] = temp;
   			
			   rowNormalizer[pivotRow] = rowNormalizer[j];
			   exchangeParity = -exchangeParity;
		   }
   		
		   // Divide by pivot element
		   if (j != n - 1)
		   {
			   float denom = 1.0F / m[j * n + j];
			   for (int i = j + 1; i < n; i++) m[j * n + i] *= denom;
		   }
	   }
   	
	   if (detSign) *detSign = exchangeParity;
	   result = true;

	   exit:
	   delete[] rowNormalizer;
	   return (result);
   }

   void LUBacksubstitute(int n, const float *d, const unsigned short *index, const float *r, float *x)
   {
	   for (int i = 0; i < n; i++) x[i] = r[index[i]];
   	
	   // Perform forward substitution for Ly = r
	   for (int i = 0; i < n; i++)
	   {
		   float sum = x[i];
		   for (int k = 0; k < i; k++) sum -= d[k * n + i] * x[k];
		   x[i] = sum;
	   }
   	
	   // Perform backward substitution for Ux = y
	   for (int i = n - 1; i >= 0; i--)
	   {
		   float sum = x[i];
		   for (int k = i + 1; k < n; k++) sum -= d[k * n + i] * x[k];
		   x[i] = sum / d[i * n + i];
	   }
   }

   void LURefineSolution(int n, const float *m,
	   const float *d, const unsigned short *index,
	   const float *r, float *x)
   {
	   float *t = new float[n];
   	
	   for (int i = 0; i < n; i++)
	   {
		   double q = -r[i];
		   for (int k = 0; k < n; k++) q += m[k * n + i] * x[k];
		   t[i] = (float) q;
	   }
   	
	   LUBacksubstitute(n, d, index, t, t);
	   for (int i = 0; i < n; i++) x[i] -= t[i];
   	
	   delete[] t;
   }

   void SolveTridiagonalSystem(int n,
	   const float *a, const float *b, const float *c,
	   const float *r, float *x)
   {
	   // Allocate temporary storage for c[i]/beta[i]
	   float *t = new float[n - 1];

	   float recipBeta = 1.0F / b[0];
	   x[0] = r[0] * recipBeta;

	   for (int i = 1; i < n; i++)
	   {
		   t[i - 1] = c[i - 1] * recipBeta; 
		   recipBeta = 1.0F / (b[i] - a[i] * t[i - 1]);
		   x[i] = (r[i] - a[i] * x[i - 1]) * recipBeta;
	   }

	   for (int i = n - 2; i >= 0; i--) x[i] -= t[i] * x[i + 1];

	   delete[] t;
   }
   
   const float epsilon = 1.0e-10F;
   const int maxSweeps = 32;
   
   void CalculateEigensystem(const Matrix3D& m, float *lambda, Matrix3D& r)
   {
	   float m11 = m(0,0);
	   float m12 = m(0,1);
	   float m13 = m(0,2);
	   float m22 = m(1,1);
	   float m23 = m(1,2);
	   float m33 = m(2,2);
   	
	   r.SetIdentity();
	   for (int a = 0; a < maxSweeps; a++)
	   {
		   // Exit if off-diagonal entries small enough
		   if ((fabs(m12) < epsilon) && (fabs(m13) < epsilon) &&
			   (fabs(m23) < epsilon)) break;
   		
		   // Annihilate (1,2) entry
		   if (m12 != 0.0F)
		   {
			   float u = (m22 - m11) * 0.5F / m12;
			   float u2 = u * u;
			   float u2p1 = u2 + 1.0F;
			   
			   // rg - 11/25/05 - This doesn't look right!
			   //float t = (u2p1 != u2) ? ((u < 0.0F) ? -1.0F : 1.0F) * (sqrt(u2p1) - fabs(u)) : 0.5F / u;
			               
            float t = 1.0f/(fabs(u)+sqrt(u2p1));
            if (u < 0.0f)
               t = -t;
				   
			   float c = 1.0F / sqrt(t * t + 1.0F);
			   float s = c * t;
   			
			   m11 -= t * m12;
			   m22 += t * m12;
			   m12 = 0.0F;
   			
			   float temp = c * m13 - s * m23;
			   m23 = s * m13 + c * m23;
			   m13 = temp;
   			
			   for (int i = 0; i < 3; i++)
			   {
				   float temp = c * r(i,0) - s * r(i,1);
				   r(i,1) = s * r(i,0) + c * r(i,1);
				   r(i,0) = temp;
			   }
		   }
   		
		   // Annihilate (1,3) entry
		   if (m13 != 0.0F)
		   {
			   float u = (m33 - m11) * 0.5F / m13;
			   float u2 = u * u;
			   float u2p1 = u2 + 1.0F;
			   // rg - 11/25/05 - This doesn't look right!
			   //float t = (u2p1 != u2) ? ((u < 0.0F) ? -1.0F : 1.0F) * (sqrt(u2p1) - fabs(u)) : 0.5F / u;
            
            float t = 1.0f/(fabs(u)+sqrt(u2p1));
            if (u < 0.0f)
               t = -t;
               
			   float c = 1.0F / sqrt(t * t + 1.0F);
			   float s = c * t;
   			
			   m11 -= t * m13;
			   m33 += t * m13;
			   m13 = 0.0F;
   			
			   float temp = c * m12 - s * m23;
			   m23 = s * m12 + c * m23;
			   m12 = temp;
   			
			   for (int i = 0; i < 3; i++)
			   {
				   float temp = c * r(i,0) - s * r(i,2);
				   r(i,2) = s * r(i,0) + c * r(i,2);
				   r(i,0) = temp;
			   }
		   }
   		
		   // Annihilate (2,3) entry
		   if (m23 != 0.0F)
		   {
			   float u = (m33 - m22) * 0.5F / m23;
			   float u2 = u * u;
			   float u2p1 = u2 + 1.0F;
			   // rg - 11/25/05 - This doesn't look right!
			   //float t = (u2p1 != u2) ? ((u < 0.0F) ? -1.0F : 1.0F) * (sqrt(u2p1) - fabs(u)) : 0.5F / u;
            float t = 1.0f/(fabs(u)+sqrt(u2p1));
            if (u < 0.0f)
               t = -t;
				   
			   float c = 1.0F / sqrt(t * t + 1.0F);
			   float s = c * t;
   			
			   m22 -= t * m23;
			   m33 += t * m23;
			   m23 = 0.0F;
   			
			   float temp = c * m12 - s * m13;
			   m13 = s * m12 + c * m13;
			   m12 = temp;
   			
			   for (int i = 0; i < 3; i++)
			   {
				   float temp = c * r(i,1) - s * r(i,2);
				   r(i,2) = s * r(i,1) + c * r(i,2);
				   r(i,1) = temp;
			   }
		   }
	   }
   	
	   lambda[0] = m11;
	   lambda[1] = m22;
	   lambda[2] = m33;
   }

   float SIGN(float a, float b)
   {
      return fabs(a) * Math::Sign(b);
   }

   float FMAX(float a, float b)
   {
      return Math::Max(a, b);
   }

   float FMIN(float a, float b)
   {
      return Math::Min(a, b);
   }

   int IMIN(int a, int b)
   {
      return Math::Min(a, b);
   }
   
   float SQR(float s)
   {
      return s * s;
   }
   
   //Computes (a2 + b2)1/2 without destructive underflow or overflow.
   float pythag(float a, float b)
   {
      float absa,absb;
      absa=fabs(a);
      absb=fabs(b);
      if (absa > absb) return absa*sqrt(1.0f+SQR(absb/absa));
      else return (absb == 0.0f ? 0.0f : absb*sqrt(1.0f+SQR(absa/absb)));
   }
   
   float* newVector(uint nl, uint nh)
   {  
      float* p = new float[nl + nh];
      std::fill(p, p + nl + nh, 0.0f);
      return p;
   }   
   
   void freeVector(float* p, uint nl, uint nh)
   {
      nl;
      nh;
      
      delete [] p;
   }
   
   float** newMatrix(uint r, uint c)
   {
      float** p = new float* [r+1];
      
      for (uint i = 0; i < r+1; i++)
      {
         p[i] = new float [c+1];
         std::fill(p[i], p[i] + c+1, 0.0f);
      }
      
      return p;
   }
   
   void freeMatrix(float** p, uint r, uint c)
   {
      c;
      
      if (!p)
         return;
      
      for (uint i = 0; i < r+1; i++)
         delete [] p[i];
      
      delete [] p;
   }

   // Here is the algorithm for constructing the singular value decomposition of any
   // matrix. See §11.2–§11.3, and also [4-5], for discussion relating to the underlying
   // method.

   // Given a matrix a[1..m][1..n], this routine computes its singular value decomposition, A =
   // U·W·V T. The matrix U replaces a on output. The diagonal matrix of singular values W is output
   // as a vector w[1..n]. The matrix V (not the transpose V T ) is output as v[1..n][1..n].
   bool svdcmp(float **a, int m, int n, float w[], float **v)
   {
      //float pythag(float a, float b);
      int flag,i,its,j,jj,k,l=0,nm=0;
      float anorm,c,f,g,h,s,scale,x,y,z,*rv1;
      rv1=newVector(1,n);
      g=scale=anorm=0.0; // Householder reduction to bidiagonal form.
         for (i=1;i<=n;i++) {
            l=i+1;
            rv1[i]=scale*g;
            g=s=scale=0.0;
            if (i <= m) {
               for (k=i;k<=m;k++) scale += fabs(a[k][i]);
               if (scale) {
                  for (k=i;k<=m;k++) {
                     a[k][i] /= scale;
                     s += a[k][i]*a[k][i];
                  }
                  f=a[i][i];
                  g = -SIGN(sqrt(s),f);
                  h=f*g-s;
                  a[i][i]=f-g;
                  for (j=l;j<=n;j++) {
                     for (s=0.0,k=i;k<=m;k++) s += a[k][i]*a[k][j];
                     f=s/h;
                     for (k=i;k<=m;k++) a[k][j] += f*a[k][i];
                  }
                  for (k=i;k<=m;k++) a[k][i] *= scale;
               }
            }
            w[i]=scale *g;
            g=s=scale=0.0;
            if (i <= m && i != n) {
               for (k=l;k<=n;k++) scale += fabs(a[i][k]);
               if (scale) {
                  
                  for (k=l;k<=n;k++) {
                     a[i][k] /= scale;
                     s += a[i][k]*a[i][k];
                  }
                  f=a[i][l];
                  g = -SIGN(sqrt(s),f);
                  h=f*g-s;
                  a[i][l]=f-g;
                  for (k=l;k<=n;k++) rv1[k]=a[i][k]/h;
                  for (j=l;j<=m;j++) {
                     for (s=0.0,k=l;k<=n;k++) s += a[j][k]*a[i][k];
                     for (k=l;k<=n;k++) a[j][k] += s*rv1[k];
                  }
                  for (k=l;k<=n;k++) a[i][k] *= scale;
               }
            }
            anorm=FMAX(anorm,(fabs(w[i])+fabs(rv1[i])));
         }
         for (i=n;i>=1;i--) { // Accumulation of right-hand transformations.
            if (i < n) {
               if (g) {
                  for (j=l;j<=n;j++) // Double division to avoid possible underflow.
                     v[j][i]=(a[i][j]/a[i][l])/g;
                  for (j=l;j<=n;j++) {
                     for (s=0.0,k=l;k<=n;k++) s += a[i][k]*v[k][j];
                     for (k=l;k<=n;k++) v[k][j] += s*v[k][i];
                  }
               }
               for (j=l;j<=n;j++) v[i][j]=v[j][i]=0.0;
            }
            v[i][i]=1.0;
            g=rv1[i];
            l=i;
         }
         for (i=IMIN(m,n);i>=1;i--) { // Accumulation of left-hand transformations.
            l=i+1;
         g=w[i];
         for (j=l;j<=n;j++) a[i][j]=0.0;
         if (g) {
            g=1.0f/g;
            for (j=l;j<=n;j++) {
               for (s=0.0,k=l;k<=m;k++) s += a[k][i]*a[k][j];
               f=(s/a[i][i])*g;
               for (k=i;k<=m;k++) a[k][j] += f*a[k][i];
            }
            for (j=i;j<=m;j++) a[j][i] *= g;
         } else for (j=i;j<=m;j++) a[j][i]=0.0;
         ++a[i][i];
         }
         for (k=n;k>=1;k--) { //Diagonalization of the bidiagonal form: Loop over
                              //singular values, and over allowed iterations. 
            for (its=1;its<=30;its++) {
               flag=1;
               for (l=k;l>=1;l--) { //Test for splitting.
                  nm=l-1; //Note that rv1[1] is always zero.
                  if ((float)(fabs(rv1[l])+anorm) == anorm) {
                     flag=0;
                     break;
                  }
                  if ((float)(fabs(w[nm])+anorm) == anorm) break;
               }
               if (flag) {
                  c=0.0; // Cancellation of rv1[l], if l > 1.
                     s=1.0;
                  for (i=l;i<=k;i++) {
                     
                     f=s*rv1[i];
                     rv1[i]=c*rv1[i];
                     if ((float)(fabs(f)+anorm) == anorm) break;
                     g=w[i];
                     h=pythag(f,g);
                     w[i]=h;
                     h=1.0f/h;
                     c=g*h;
                     s = -f*h;
                     for (j=1;j<=m;j++) {
                        y=a[j][nm];
                        z=a[j][i];
                        a[j][nm]=y*c+z*s;
                        a[j][i]=z*c-y*s;
                     }
                  }
               }
               z=w[k];
               if (l == k) { //Convergence.
                  if (z < 0.0) { //Singular value is made nonnegative.
                     w[k] = -z;
                     for (j=1;j<=n;j++) v[j][k] = -v[j][k];
                  }
                  break;
               }
               
               if (its == 30) {
                  //nrerror("no convergence in 30 svdcmp iterations");
                  freeVector(rv1,1,n);
                  return false;
               }
               
               x=w[l]; //Shift from bottom 2-by-2 minor.
                  nm=k-1;
               y=w[nm];
               g=rv1[nm];
               h=rv1[k];
               f=((y-z)*(y+z)+(g-h)*(g+h))/(2.0f*h*y);
               g=pythag(f,1.0);
               f=((x-z)*(x+z)+h*((y/(f+SIGN(g,f)))-h))/x;
               c=s=1.0; //Next QR transformation:
               for (j=l;j<=nm;j++) {
                  i=j+1;
                  g=rv1[i];
                  y=w[i];
                  h=s*g;
                  g=c*g;
                  z=pythag(f,h);
                  rv1[j]=z;
                  c=f/z;
                  s=h/z;
                  f=x*c+g*s;
                  g = g*c-x*s;
                  h=y*s;
                  y *= c;
                  for (jj=1;jj<=n;jj++) {
                     x=v[jj][j];
                     z=v[jj][i];
                     v[jj][j]=x*c+z*s;
                     v[jj][i]=z*c-x*s;
                  }
                  z=pythag(f,h);
                  w[j]=z; // Rotation can be arbitrary if z = 0.
                     if (z) {
                        z=1.0f/z;
                        c=f*z;
                        s=h*z;
                     }
                     f=c*g+s*y;
                     x=c*y-s*g;
                     
                     for (jj=1;jj<=m;jj++) {
                        y=a[jj][j];
                        z=a[jj][i];
                        a[jj][j]=y*c+z*s;
                        a[jj][i]=z*c-y*s;
                     }
               }
               rv1[l]=0.0;
               rv1[k]=f;
               w[k]=x;
            }
         }
         freeVector(rv1,1,n);
         
         return true;
   }
   
   // Solves A·X = B for a vector X, where A is specified by the arrays u[1..m][1..n], w[1..n],
   //   v[1..n][1..n] as returned by svdcmp. m and n are the dimensions of a, and will be equal for
   //   square matrices. b[1..m] is the input right-hand side. x[1..n] is the output solution vector.
   //   No input quantities are destroyed, so the routine may be called sequentially with different b’s.
   void svbksb(float **u, float w[], float **v, int m, int n, float b[], float x[])
   {
      int jj,j,i;
      float s,*tmp;
      tmp=newVector(1,n);
      for (j=1;j<=n;j++) 
      { // Calculate UTB.
         s=0.0f;
         if (w[j]) 
         { 
            // Nonzero result only if wj is nonzero.
            for (i=1;i<=m;i++) 
               s += u[i][j]*b[i];
               
            s /= w[j]; //This is the divide by wj .
         }
         tmp[j]=s;
      }
      
      for (j=1;j<=n;j++) 
      { 
         //Matrix multiply by V to get answer.
         s=0.0f;
         for (jj=1;jj<=n;jj++) 
            s += v[j][jj]*tmp[jj];
         x[j]=s;
      }
      
      freeVector(tmp,1,n);
   }
   
#if 0
   // How to solve a linear system, from Numerical Recipes
   #define N ..
   float wmax,wmin,**a,**u,*w,**v,*b,*x;
   int i,j;
   
      for(i=1;i<=N;i++) // Copy a into u if you don’t want it to be destroyed.
         for j=1;j<=N;j++)
            u[i][j]=a[i][j];
   svdcmp(u,N,N,w,v); // SVD the square matrix a.
      wmax=0.0; // Will be the maximum singular value obtained.
      for(j=1;j<=N;j++) if (w[j] > wmax) wmax=w[j];
   //This is where we set the threshold for singular values allowed to be nonzero. The constant
   //   is typical, but not universal. You have to experiment with your own application.
   //   wmin=wmax*1.0e-6;
   for(j=1;j<=N;j++) if (w[j] < wmin) w[j]=0.0;
   svbksb(u,w,v,N,N,b,x); // Now we can backsubstitute.
#endif   


#if 0      
   // Hacky SVD test code
   {
      BMatrix44 rot(BMatrix44::makeRotate(Math::fDegToRad(45.0f)));

      float** m = LinearAlgebra::newMatrix(4,4);
      float** v = LinearAlgebra::newMatrix(4,4);

      LinearAlgebra::setMatrix(m, rot);

      float w[5];
      std::fill(w, w + 5, 0.0f);
      bool success = LinearAlgebra::svdcmp(m, 4, 4, w, v);

      BMatrix44 um;
      LinearAlgebra::getMatrix(um, m);

      BMatrix44 vm;
      LinearAlgebra::getMatrix(vm, v);

      vm.transpose();

      BMatrix44 wm;
      wm.setZero();
      for (uint i = 0; i < 4; i++)
         wm[i][i] = w[i+1];

      BMatrix44 k(um * wm * vm);            

      LinearAlgebra::freeMatrix(m, 4, 4);
      LinearAlgebra::freeMatrix(v, 4, 4);
   }         
#endif      

} // namespace LinearAlgebra

