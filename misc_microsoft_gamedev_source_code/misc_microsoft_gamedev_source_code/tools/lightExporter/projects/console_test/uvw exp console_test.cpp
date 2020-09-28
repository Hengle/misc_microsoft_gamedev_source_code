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
#include "common/utils/stream.h"
#include "common/geom/unigeom.h"
#include "common/math/intersection.h"
#include "common/math/arcball.h"

using namespace gr;
using namespace gr::Math;

// returns first two rows of UVW->Model matrix or GTS->model
void MakeTextureSpaceBasis(Vec3 bvec[3], const Vec3 tv[3], const Vec3 v[3]) 
{
	float uva = tv[1][0] - tv[0][0];
	float uvb = tv[2][0] - tv[0][0];

	float uvc = tv[1][1] - tv[0][1];
	float uvd = tv[2][1] - tv[0][1];

	float uvk = uvb * uvc - uva * uvd;

	printf("%f\n", uvk);

	Vec3 v1(v[1] - v[0]);
	Vec3 v2(v[2] - v[0]);

	if (uvk != 0) 
  {
  	bvec[0] = (uvc * v2 - uvd * v1) / uvk;
	  bvec[1] = (uva * v2 - uvb * v1) / uvk;
	}
	else 
  {
		if (uva != 0)
			bvec[0] = v1 / uva;
		else if (uvb != 0)
			bvec[0] = v2 / uvb;
		else
			bvec[0] = Vec3(0.0f, 0.0f, 0.0f);

    if (uvc != 0)
			bvec[1] = v1 / uvc;
		else if (uvd != 0)
			bvec[1] = v2 / uvd;
		else
			bvec[1] = Vec3(0.0f, 0.0f, 0.0f);
	}
	
	// This is invalid; either cross S and T or use vertex normal!
	bvec[2] = Vec3(0.0f, 0.0f, 1.0f);  
}

int main(int argc, char* argv[])
{
	Vec3 bvec[3];
	Vec3 tv[3];
	Vec3 v[3];

	Utils::ClearObj(bvec);
	Utils::ClearObj(tv);
	Utils::ClearObj(v);

	v[0].set(0.0f, 0.0f, 0.0f);
	v[1].set(10.0f, 0.0f, 0.0f);
	v[2].set(10.0f, 10.0f, 0.0f);

	float x = 1.0f;
	tv[0].set(0.0f, 0.0f, 0.0f);
	tv[1].set(x, 0.0f, 0.0f);
	tv[2].set(x, x, 0.0f);
	
	MakeTextureSpaceBasis(bvec, tv, v);

	bvec[1] *= -1.0f;
	bvec[2] = (bvec[0] % bvec[1]).normalize();

	// create normal transform matrix UVW->Model
	Matrix44 k(Matrix44::I);
	k.setRow(0, bvec[0]);
	k.setRow(1, bvec[1]);
	k.setRow(2, bvec[2]);
	k.invert();
	k.transpose();
  
	for (int i = 0; i < 3; i++)
		printf("%f %f %f\n", bvec[i][0], bvec[i][1], bvec[i][2]);

	printf("\n");

	for (int i = 0; i < 4; i++)
		printf("%f %f %f %f\n", k[i][0], k[i][1], k[i][2], k[i][3]);

	return 0;
}







