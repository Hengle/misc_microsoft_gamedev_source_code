// texture_space_basis.h
#pragma once
#ifndef TEXTURE_SPACE_BASIS_H
#define TEXTURE_SPACE_BASIS_H

#include "common/math/vector.h"

namespace gr
{

	// From Max SDK:
	// Returns first two rows of UVW->Model matrix or GTS->model.
	inline void MakeTextureSpaceBasis(Vec3* pBasisVecs, const Vec2* pT, const Vec3* pV) 
	{
		const float uva = pT[1][0] - pT[0][0];
		const float uvb = pT[2][0] - pT[0][0];

		const float uvc = pT[1][1] - pT[0][1];
		const float uvd = pT[2][1] - pT[0][1];

		const float uvk = uvb * uvc - uva * uvd;

		const Vec3 v1(pV[1] - pV[0]);
		const Vec3 v2(pV[2] - pV[0]);

		if (uvk != 0.0f) 
		{
  		pBasisVecs[0] = (uvc * v2 - uvd * v1) / uvk;
			pBasisVecs[1] = (uva * v2 - uvb * v1) / uvk;
		}
		else 
		{
			if (uva != 0.0f)
				pBasisVecs[0] = v1 / uva;
			else if (uvb != 0.0f)
				pBasisVecs[0] = v2 / uvb;
			else
				pBasisVecs[0] = Vec3(0.0f, 0.0f, 0.0f);

			if (uvc != 0.0f)
				pBasisVecs[1] = v1 / uvc;
			else if (uvd != 0.0f)
				pBasisVecs[1] = v2 / uvd;
			else
				pBasisVecs[1] = Vec3(0.0f, 0.0f, 0.0f);
		}

		pBasisVecs[1] *= -1.0f;
	}

	inline bool MakeTextureSpaceBasisAlt(Vec3* pBasisVecs, const Vec2* pT, const Vec3* pV) 
	{
		bool valid = true;

		Vec3 edge01( pV[1][0] - pV[0][0], pT[1][0] - pT[0][0], pT[1][1] - pT[0][1] );
		Vec3 edge02( pV[2][0] - pV[0][0], pT[2][0] - pT[0][0], pT[2][1] - pT[0][1] );
		Vec3 cp(edge01 % edge02);
		if( fabs(cp[0]) > 1e-8 )
		{
			pBasisVecs[0][0] = -cp[1] / cp[0];        
			pBasisVecs[1][0] = -cp[2] / cp[0];
			valid = false;
		}

		edge01 = Vec3( pV[1][1] - pV[0][1], pT[1][0] - pT[0][0], pT[1][1] - pT[0][1] );
		edge02 = Vec3( pV[2][1] - pV[0][1], pT[2][0] - pT[0][0], pT[2][1] - pT[0][1] );
		cp = edge01 % edge02;
		if( fabs(cp[0]) > 1e-8 )
		{
			pBasisVecs[0][1] = -cp[1] / cp[0];
			pBasisVecs[1][1] = -cp[2] / cp[0];
			valid = false;
		}

		edge01 = Vec3( pV[1][2] - pV[0][2], pT[1][0] - pT[0][0], pT[1][1] - pT[0][1] );
		edge02 = Vec3( pV[2][2] - pV[0][2], pT[2][0] - pT[0][0], pT[2][1] - pT[0][1] );
		cp = edge01 % edge02;
		if( fabs(cp[0]) > 1e-8 )
		{
			pBasisVecs[0][2] = -cp[1] / cp[0];
			pBasisVecs[1][2] = -cp[2] / cp[0];
			valid = false;
		}
		return valid;
	}

} // namespace gr

#endif // TEXTURE_SPACE_BASIS_H
