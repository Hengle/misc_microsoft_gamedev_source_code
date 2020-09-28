//==============================================================================
// protoimpacteffect.h
//
// Copyright (c) 2006-2007 Ensemble Studios
//==============================================================================

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
class BProtoImpactEffect
{
   public: 
      BProtoImpactEffect() : mName(), mTerrainEffectIndex(-1), mLifespan(1.0f), mLimit(1), mBoundingRadius(5.0f) {};   
     ~BProtoImpactEffect() {};

     BSimString mName;

     int   mTerrainEffectIndex;
     float mLifespan;

     float mBoundingRadius;
     int   mLimit;
};