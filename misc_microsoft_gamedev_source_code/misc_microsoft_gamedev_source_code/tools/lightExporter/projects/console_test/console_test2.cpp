#include "stdafx.h"

#include "common/math/math.h"
#include "common/math/vector.h"
#include "common/math/quat.h"
#include "x86/core/timer_x86.h"
#include "common/math/euler_angles.h"
#include "common/math/plane.h"

using namespace gr;

int main(int argc, char* argv[])
{
	{
		Vec3 a(0,0,0);
		Vec3 b(1,0,0);
		Vec3 c(0,1,0);

		Matrix44 m;
		
		EulerAngles(Math::fDegToRad(45.0f), Math::fDegToRad(30.0f), Math::fDegToRad(99)).toMatrix(m);
		Vec3 ta(a * m);
		Vec3 tb(b * m);
		Vec3 tc(c * m);


		Plane a1(a, b, c);
		Plane a2(ta, tb, tc);
		Plane a3(a1 * m);
		
		printf("%f", a3.normal()[0]);
	}		

	{
		Quat r;
		Matrix44 k;
		Matrix44 l;
		EulerAngles(Math::fDegToRad(45.0f), Math::fDegToRad(30.0f), Math::fDegToRad(99)).toQuat(r);
		EulerAngles(Math::fDegToRad(45.0f), Math::fDegToRad(30.0f), Math::fDegToRad(99)).toMatrix(k);
		r.toMatrix(l);

		EulerAngles zz(k);
		EulerAngles kk(r);



	for (int i = 0; i < 10000000; i++)
	{
		Quat k(Quat::makeRandom());
		Assert(k.isUnit());

		Quat l(Quat::makeRandom());
		Assert(l.isUnit());

		Quat p(Quat::slerp(k, l, Math::fRand(0, 1)));
		Assert(p.isUnit());

		Quat d(Quat::makeRotation(k, l));
		Assert(d.isUnit());

		Quat z(k * d);
		Assert(Quat::equalTol(l, z));
	}
	}

	Matrix44 a(Matrix44::makeRotate(2, Math::fDegToRad(45.0f)));
	Matrix44 b(Matrix44::makeRotate(1, Math::fDegToRad(100.0f)));
	Matrix44 c(a * b);
	Matrix44 m;
	
	Quat qa, qb;
	qa.setFromAxisAngle(Vec4(0,0,1,0), Math::fDegToRad(45.0f));
	qb.setFromAxisAngle(Vec4(0,1,0,0), Math::fDegToRad(100.0f));

	Quat qc(qa * qb); //Quat::slerp(qa, qb, .5f));

	qc.toMatrix(m);
	
	Vec4 x(1, 1, 1, 0);
	Vec4 temp1 = (qc * Quat(x) * qc.conjugate()).vec;
	Vec4 temp2 = qc.rotateVec(x);

	Vec4 r0 = qc.rotateVec(Vec4::makeAxisVector(0, 1));
	Vec4 r1 = qc.rotateVec(Vec4::makeAxisVector(1, 1));
	Vec4 r2 = qc.rotateVec(Vec4::makeAxisVector(2, 1));

	Vec4 x0 = Vec4::makeAxisVector(0, 1) * m;
	Vec4 x1 = Vec4::makeAxisVector(1, 1) * m;
	Vec4 x2 = Vec4::makeAxisVector(2, 1) * m;

	Matrix44 r;
		
	uint mn = UINT_MAX;
#if DEBUG
	for (int t = 0; t < 10; t++)
#else
	for (int t = 0; t < 1000; t++)
#endif
	{
		uint s = Time::ReadCycleCounter();
	
		for (int i = 0; i < 10000; i++)
		{
			r = m;
			// 830
			r.invertSlow(); 
		}

		uint t = Time::ReadCycleCounter() - s;
		if (t < mn) mn = t;
	}
  	
	printf("%f\n", (mn - 88) / 10000.0f);
	printf("%f\n", r[0][0]);

	/*

  Vec4 a;
	a.set(1,2,3,4);
	a.setX(Vec4(99,-1,-2,-3));
	printf("%f %f %f %f\n", a[0], a[1], a[2], a[3]);

	a.set(1,2,3,4);
	a.setY(Vec4(-1,99,-2,-3));
	printf("%f %f %f %f\n", a[0], a[1], a[2], a[3]);

	a.set(1,2,3,4);
	a.setZ(Vec4(-2,-1,99,-3));
	printf("%f %f %f %f\n", a[0], a[1], a[2], a[3]);

	a.set(1,2,3,4);
	a.setW(Vec4(-2,-1,-3, 99));
	printf("%f %f %f %f\n", a[0], a[1], a[2], a[3]);

	a.set(1,2,3,4);
	a.setXY(Vec4(-2,-1,-3, 99));
	printf("%f %f %f %f\n", a[0], a[1], a[2], a[3]);

	a.set(1,2,3,4);
	a.setZW(Vec4(-2,-1,-3, 99));
	printf("%f %f %f %f\n", a[0], a[1], a[2], a[3]);

	*/


		
	return 0;
}

