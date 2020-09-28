//==============================================================================
// lightVisualData.cpp
//
// Copyright (c) 2006 Ensemble Studios
//==============================================================================

// Includes
#include "xgameRender.h"
#include "lightVisualData.h"
#include "sceneLightManager.h"
#include "xmlreader.h"
#include "xmlwriter.h"

//==============================================================================
// BLightVisualData::BLightVisualData
//==============================================================================
BLightVisualData::BLightVisualData() : 
   mDirection(0.0f, -1.0f, 0.0f),
   mRadius(20.0f), 
   mShadows(false), 
   mColor(1.0f,1.0f,1.0f), 
   mIntensity(1.0f),
   mType(cLTOmni), 
   mFarAttenStart(.8f),
   mDecayDist(120.0f), 
   mOuterAngle(Math::fDegToRad(55.0f)), 
   mInnerAngle(Math::fDegToRad(60.0f)),
   mShadowDarkness(0.0f),
   mSpecular(false),
   mLightBuffered(false)
{
}

//==============================================================================
// BLightVisualData::load
//==============================================================================
void BLightVisualData::load(BXMLNode node)
{
   long count=node.getNumberChildren();
   for(long i=0; i<count; i++)
   {
      const BXMLNode child(node.getChild(i));
      const BPackedString name(child.getName());

      if(name=="radius")
         child.getTextAsFloat(mRadius);
      else if(name=="shadows")
      {
         bool val;
         child.getTextAsBool(val);
         mShadows = val;
      }
      else if(name=="specular")
      {
         bool val;
         child.getTextAsBool(val);
         mSpecular = val;
      }
      else if(name=="color")
      {
         child.getTextAsVector(mColor);
         
         // Convert to linear light
         mColor = XMVectorPowEst(mColor * 1.0f/255.0f, XMVectorReplicate(2.2f));
      }
      else if(name=="type")
      {
         if(child.compareText("Spot") == 0)
            mType=cLTSpot;
         else if(child.compareText("Omni") == 0)
            mType=cLTOmni;
      }
      else if(name=="farAttnStart")
         child.getTextAsFloat(mFarAttenStart);
      else if(name=="intensity")
         child.getTextAsFloat(mIntensity);
      else if(name=="decayDist")
         child.getTextAsFloat(mDecayDist);
      else if(name=="outerAngle")
         child.getTextAsAngle(mOuterAngle);
      else if(name=="innerAngle")
         child.getTextAsAngle(mInnerAngle);
      else if(name=="direction")
      {
         child.getTextAsVector(mDirection);
         mDirection.safeNormalize();
      }
      else if(name=="shadowDarkness")
         child.getTextAsFloat(mShadowDarkness);
      else if(name=="lightBuffered")
      {
         bool enabled = false;
         child.getTextAsBool(enabled);
         mLightBuffered = enabled;
      }
   }
}


char *gLightTypeNames[] = {   "Omni",
                              "Spot" };

//==============================================================================
// BLightVisualData::save
//==============================================================================
void BLightVisualData::save(BXMLWriter &writer)
{
   writer.addItem("radius", mRadius);
   writer.addItem("shadows", mShadows);
   writer.addItem("specular", mSpecular);
   writer.addItem("color", mColor);
   writer.addItem("type", gLightTypeNames[mType]);
   writer.addItem("farAttnStart", mFarAttenStart);
   writer.addItem("intensity", mIntensity);
   writer.addItem("decayDist", mDecayDist);
   writer.addItem("outerAngle", mOuterAngle);
   writer.addItem("innerAngle", mInnerAngle);
   writer.addItem("direction", mDirection);
   writer.addItem("shadowDarkness", mShadowDarkness);
   writer.addItem("lightBuffered", mLightBuffered);
}
