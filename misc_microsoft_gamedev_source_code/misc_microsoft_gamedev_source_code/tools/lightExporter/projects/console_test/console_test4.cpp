//************************************************************************** 
//* console_test.cpp
//* 
//* By Rich Geldreich
//*
//* June 12, 2003 RG Initial coding
//*
//* Copyright (c) 2003, All Rights Reserved. 
//***************************************************************************

#include "stdafx.h"

#include "common/math/math.h"
#include "common/math/vector.h"
#include "common/math/quat.h"
#include "x86/core/timer_x86.h"
#include "common/math/euler_angles.h"
#include "common/math/plane.h"
#include "common/utils/unifier.h"
#include "common/geom/univert.h"
//#include "common/geom/indexed_tri.h"
#include "common/utils/stream.h"
#include "common/geom/unigeom.h"

using namespace gr;
using namespace gr::Math;

int main(int argc, char* argv[])
{

	Univert k(Vec4(0,0,0,0));
	Univert l(Vec4(0,0,0,1));
	Univert m(Vec4(0,0,0,2));

	Unifier<Univert> z;
	printf("%i\n", z.insert(k));
	printf("%i\n", z.insert(l));
	printf("%i\n", z.insert(m));
	printf("%i\n", z.insert(k));
	printf("%i\n", z.insert(m));
	printf("%i\n", z.insert(l));
	printf("%i\n", z.insert(Univert(Vec4(1,1,1,1))));

	for (int i = 0; i < z.size(); i++)
	{
		printf("%f %f %f %f\n", z[i].p[0], z[i].p[1], z[i].p[2], z[i].p[3]);
	}
	

	for ( ; ; )
	{
		Matrix44 r(Matrix44::makeReflection(Vec4(0,1,0), Vec4(0,0,0)));
		Matrix44 ir(r.inverse());
		
		Quat qx(Quat::makeRandom());
		Quat qy(Quat::makeRandom());
		Quat qz(Quat::makeRandom());

		Matrix44 x(qx);
		Matrix44 y(qy);
		Matrix44 z(qz);
			
		// GTS->View = GTS->Model * Model->View		tspace=rows
		// View->PTS = (View->Model * Model->GTS) * GTS->PTS   tspace=cols

		Matrix44 qq(z.inverse() * r * z);
		Matrix44 t(x * y * z * qq);

	    	
		//y.setColumn(0, -y.getColumn(0));
		Matrix44 j = x * y * r * z;
		    
		Assert(Matrix44::equalTol3x3(t, j));
	}

	//printf("reflection: %i\n", z.hasNoReflection3x3());
	
	return 0;
}


/*

	x * y * z * qq = x * y * r * z

	y * z * qq = ix * x * y * r * z
	z * qq = iy * ix * x * y * r * z
	qq = iz * iy * (ix * x) * y * r * z
	qq = iz * (iy * y) * r * z
	qq = iz * r * z
	qq = r * z
  
*/





