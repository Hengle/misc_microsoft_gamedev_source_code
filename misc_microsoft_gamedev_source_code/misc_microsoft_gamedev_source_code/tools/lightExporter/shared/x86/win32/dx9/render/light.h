// File: Light.h
#ifndef LIGHT_H
#define LIGHT_H

#include "common/math/vector.h"
#include "x86/win32/dx9/render/texture_manager.h"

namespace gr
{
	enum ELightType
	{
		eInvalidLightType = -1,

		eOmnidirectional,
		eSpot,
		eDirectional,
		
		eNumLightTypes
	};
	
	enum EDecayType
	{
		eDecayNone,
		eDecayLinear,
		eDecayInverseSquare
	};

	struct Light
	{
		ELightType mType;
		Vec4 mPos;
		Vec4 mDir;
		Vec4 mUp;
		Vec4 mRight;
		float mRadius;
		
		float mSpotInner;
		float mSpotOuter;
		
		float mFarAttenStart;
		
		float mDecayDist;
		EDecayType mDecayType;
		
		Vec4 mColor0;
		Vec4 mColor1;
		bool mShadows;
		bool mFog;
		bool mFogShadows;
		float mFogDensity;
		
		TextureProxy* mMask;

		void clear(void)
		{
			mType = eInvalidLightType;
			mPos = 0;
			mDir = 0;
			mUp = 0;
			mRight = 0;
			mRadius = 0; 
			mSpotInner = 0; 
			mSpotOuter = 0;
			mFarAttenStart = 0;
			mDecayDist = 1;
			mDecayType = eDecayInverseSquare;
			mColor0 = 0;
			mColor1 = 0; 
			mShadows = 0;
			mMask = NULL;
			mFog = false;
			mFogShadows = false;
			mFogDensity = 1.0f;
		}
		
		Light()
		{
			clear();
		}
	};
	
} // namespace gr
	
#endif // LIGHT_H
