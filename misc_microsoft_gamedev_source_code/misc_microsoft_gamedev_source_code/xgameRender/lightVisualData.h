//==============================================================================
// lightVisualData.h
//
// Copyright (c) 2006 Ensemble Studios
//==============================================================================
#pragma once

// Includes
#include "sceneLightManager.h"

// Forward declarations
class BXMLWriter;

//==============================================================================
// BLightVisualData
//==============================================================================
class BLightVisualData
{
   public:
                        BLightVisualData();

      void              load(BXMLNode node);
      void              save(BXMLWriter &writer);

      BVector			   mDirection;
      BVector           mColor;
      float             mRadius;
      float             mIntensity;
      
      float             mFarAttenStart;
      float             mDecayDist;
      float             mOuterAngle;
      float             mInnerAngle;
      float             mShadowDarkness;
	   
	   eLocalLightType   mType;
	   
	   bool              mShadows : 1;
	   bool              mSpecular : 1;
	   bool              mLightBuffered : 1;
};
