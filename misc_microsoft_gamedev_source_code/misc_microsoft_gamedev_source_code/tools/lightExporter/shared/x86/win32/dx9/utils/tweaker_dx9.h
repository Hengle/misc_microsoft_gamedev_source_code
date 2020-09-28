//-----------------------------------------------------------------------------
// tweaker_dx9.h
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#pragma once
#ifndef TWEAKER_DX9
#define TWEAKER_DX9

#include "common/utils/tweaker.h"

namespace gr
{
	enum EDX9Tweakers
	{
		eToneMapper = eNumCommonTweakers,
		eTilizer,
		eGlare,
		eShadows,
		eFog,
		
		eNumDX9Tweakers
	};
	
	class DX9Tweaker : public SharedTweaker
	{
	public:
		DX9Tweaker() : SharedTweaker()
		{
		}

		virtual void init(void)
		{
			SharedTweaker::init();

			createPage("Tone Mapping/Buffer Visualization", eToneMapper);
			createPage("Tilerizer/Light Boundary Visualization", eTilizer);
			createPage("Bloom/Glare", eGlare);
			createPage("Shadows", eShadows);			
			createPage("Fog", eFog);
		}
	};
	
} // namespace gr

#endif // TWEAKER_DX9
