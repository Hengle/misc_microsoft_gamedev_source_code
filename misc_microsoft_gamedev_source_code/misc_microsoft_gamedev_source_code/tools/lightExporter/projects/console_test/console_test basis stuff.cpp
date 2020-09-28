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
#include "common/math/texture_space_basis.h"

using namespace gr;
using namespace gr::Math;

namespace gr
{
	

	void CreateBasisVectors(const Unigeom::TriVec& tris)
	{
		UnivertAttributes pnAttributes;
		pnAttributes.pos = true;
		pnAttributes.norm = true;
		
		typedef Unifier<Univert> UnivertUnifier;
		UnivertUnifier pnUnifier;

		IntVec pnIndices(tris.size() * 3);

		for (int i = 0; i < tris.size(); i++)
			for (int j = 0; j < 3; j++)
				pnIndices[i * 3 + j] = pnUnifier.insert(tris[i][j].select(pnAttributes)).first;

	}

} // namespace gr

int main(int argc, char* argv[])
{
	Vec3 bvec[3];
	Vec2 tv[3];
	Vec3 v[3];

	Utils::ClearObj(bvec);
	Utils::ClearObj(tv);
	Utils::ClearObj(v);

	v[0].set(0.0f, 0.0f, 0.0f);
	v[1].set(1.0f, 0.2f, 0.0f);
	v[2].set(1.0f, 1.0f, 0.0f);

	float x = 2.0f;
	tv[0].set(0.0f, 0.0f, 0.0f);
	tv[1].set(x, 0.0f, 0.0f);
	tv[2].set(x, x, 0.0f);
	
	MakeTextureSpaceBasisAlt(bvec, tv, v);

	bvec[2] = (bvec[0] % bvec[1]).normalize();

	Matrix44 k(Matrix44::I);
	k.setRow(0, bvec[0]);
	k.setRow(1, bvec[1]);
	k.setRow(2, bvec[2]);
			
	k.invert();

	Vec4 p = Vec4(v[2]) * k;

	k.transpose();
  
	for (int i = 0; i < 3; i++)
		printf("%f %f %f\n", bvec[i][0], bvec[i][1], bvec[i][2]);

	printf("\n");

	for (int i = 0; i < 4; i++)
		printf("%f %f %f %f\n", k[i][0], k[i][1], k[i][2], k[i][3]);

	return 0;
}







