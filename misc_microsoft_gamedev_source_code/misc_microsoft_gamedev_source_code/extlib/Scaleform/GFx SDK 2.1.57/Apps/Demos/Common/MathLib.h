/**********************************************************************

Filename    :   MathLib.h
Content     :   Simple Matrix math
Created     :   
Authors     :   Andrew Reisse
Copyright   :   (c) 2007 Scaleform Corp. All Rights Reserved.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef INC_MATHLIB_H
#define INC_MATHLIB_H

void MatrixMult(float *po, const float *pa, const float *pb);
void VectorMult(float *po, const float *pa, float x, float y, float z=0, float w=1);
void VectorMult(float *po, const float *pa, const float *pv);
void VectorInvHomog(float *pv);
void MakePerspective(float *pp, float fov, float aspect, float z0, float z1);
void MakeIdentity(float *pp);
void MakeRotateY(float *pp, float angle);
void MakeRotateX(float *pp, float angle);
void MakeRotateZ(float *pp, float angle);
void Translate(float *pp, float x, float y, float z);
void MatrixTranspose(float *po, float *pa);
void MatrixInverse(float *po, const float *pa);

#endif
