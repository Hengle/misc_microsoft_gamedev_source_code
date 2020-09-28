//==============================================================================
// aitypes.cpp
//
// Copyright (c) 2008 Ensemble Studios
//==============================================================================

// xgame
#include "common.h"
#include "aidebug.h"
#include "aitypes.h"
#include "database.h"

GFIMPLEMENTVERSION(BAISquadAnalysis, 1);

//==============================================================================
//==============================================================================
void BAISquadAnalysis::reset()
{
   Utils::FastMemSet(this, 0, sizeof(BAISquadAnalysis));
   /*mCVLight = 0.0f;
   mCVMedium = 0.0f;
   mCVMediumAir = 0.0f;
   mCVHeavy = 0.0f;
   mCVBuilding = 0.0f;
   mCVTotal = 0.0f;
   mHPLight = 0.0f;
   mHPMedium = 0.0f;
   mHPMediumAir = 0.0f;
   mHPHeavy = 0.0f;
   mHPBuilding = 0.0f;
   mHPTotal = 0.0f;
   mSPLight = 0.0f;
   mSPMedium = 0.0f;
   mSPMediumAir = 0.0f;
   mSPHeavy = 0.0f;
   mSPBuilding = 0.0f;
   mSPTotal = 0.0f;
   mDPSLight = 0.0f;
   mDPSMedium = 0.0f;
   mDPSMediumAir = 0.0f;
   mDPSHeavy = 0.0f;
   mDPSBuilding = 0.0f;
   mDPSTotal = 0.0f;
   mCVPercentLight = 0.0f;
   mCVPercentMedium = 0.0f;
   mCVPercentMediumAir = 0.0f;
   mCVPercentHeavy = 0.0f;
   mCVPercentBuilding = 0.0f;
   mHPPercentLight = 0.0f;
   mHPPercentMedium = 0.0f;
   mHPPercentMediumAir = 0.0f;
   mHPPercentHeavy = 0.0f;
   mHPPercentBuilding = 0.0f;
   mCVStarsLight = 0.0f;
   mCVStarsMedium = 0.0f;
   mCVStarsMediumAir = 0.0f;
   mCVStarsHeavy = 0.0f;
   mCVStarsBuilding = 0.0f;
   mCVStarsTotal = 0.0f;
   mNormalizedStarsLight = 0.0f;
   mNormalizedStarsMedium = 0.0f;
   mNormalizedStarsMediumAir = 0.0f;
   mNormalizedStarsHeavy = 0.0f;
   mNormalizedStarsBuilding = 0.0f;
#ifdef aiCombatHack
   mNumSquads = 0;
#endif
   */
}


//==============================================================================
//==============================================================================
void BAISquadAnalysis::add(const BAISquadAnalysis& a)
{
   mCVLight += a.mCVLight;
   mCVLightArmored += a.mCVLightArmored;
   mCVMedium += a.mCVMedium;
   mCVMediumAir += a.mCVMediumAir;
   mCVHeavy += a.mCVHeavy;
   mCVBuilding += a.mCVBuilding;
   mCVTotal += a.mCVTotal;

   mHPLight += a.mHPLight;
   mHPLightArmored += a.mHPLightArmored;
   mHPMedium += a.mHPMedium;
   mHPMediumAir += a.mHPMediumAir;
   mHPHeavy += a.mHPHeavy;
   mHPBuilding += a.mHPBuilding;
   mHPTotal += a.mHPTotal;

   mSPLight += a.mSPLight;
   mSPLightArmored += a.mSPLightArmored;
   mSPMedium += a.mSPMedium;
   mSPMediumAir += a.mSPMediumAir;
   mSPHeavy += a.mSPHeavy;
   mSPBuilding += a.mSPBuilding;
   mSPTotal += a.mSPTotal;

   mDPSLight += a.mDPSLight;
   mDPSLightArmored += a.mDPSLightArmored;
   mDPSMedium += a.mDPSMedium;
   mDPSMediumAir += a.mDPSMediumAir;
   mDPSHeavy += a.mDPSHeavy;
   mDPSBuilding += a.mDPSBuilding;
   mDPSTotal += a.mDPSTotal;

   if (mCVTotal > 0.0f)
   {
      mCVPercentLight = mCVLight / mCVTotal;
      mCVPercentLightArmored = mCVLightArmored / mCVTotal;
      mCVPercentMedium = mCVMedium / mCVTotal;
      mCVPercentMediumAir = mCVMediumAir / mCVTotal;
      mCVPercentHeavy = mCVHeavy / mCVTotal;
      mCVPercentBuilding = mCVBuilding / mCVTotal;
   }
   else
   {
      mCVPercentLight = 0.0f;
      mCVPercentLightArmored = 0.0f;
      mCVPercentMedium = 0.0f;
      mCVPercentMediumAir = 0.0f;
      mCVPercentHeavy = 0.0f;
      mCVPercentBuilding = 0.0f;
   }

   if (mHPTotal > 0.0f)
   {
      mHPPercentLight = mHPLight / mHPTotal;
      mHPPercentLightArmored = mHPLightArmored / mHPTotal;
      mHPPercentMedium = mHPMedium / mHPTotal;
      mHPPercentMediumAir = mHPMediumAir / mHPTotal;
      mHPPercentHeavy = mHPHeavy / mHPTotal;
      mHPPercentBuilding = mHPBuilding / mHPTotal;
   }
   else
   {
      mHPPercentLight = 0.0f;
      mHPPercentLightArmored = 0.0f;
      mHPPercentMedium = 0.0f;
      mHPPercentMediumAir = 0.0f;
      mHPPercentHeavy = 0.0f;
      mHPPercentBuilding = 0.0f;
   }

   mCVStarsLight += a.mCVStarsLight;
   mCVStarsLightArmored += a.mCVStarsLightArmored;
   mCVStarsMedium += a.mCVStarsMedium;
   mCVStarsMediumAir += a.mCVStarsMediumAir;
   mCVStarsHeavy += a.mCVStarsHeavy;
   mCVStarsBuilding += a.mCVStarsBuilding;
   mCVStarsTotal += a.mCVStarsTotal;

   if (mCVStarsTotal > 0.0f)
   {
      float starsMax = mCVStarsLight;
      starsMax = static_cast<float>(Math::fSelectMax(starsMax, mCVStarsLightArmored));
      starsMax = static_cast<float>(Math::fSelectMax(starsMax, mCVStarsMedium));
      starsMax = static_cast<float>(Math::fSelectMax(starsMax, mCVStarsMediumAir));
      starsMax = static_cast<float>(Math::fSelectMax(starsMax, mCVStarsHeavy));
      starsMax = static_cast<float>(Math::fSelectMax(starsMax, mCVStarsBuilding));
      mNormalizedStarsLight = mCVStarsLight / starsMax;
      mNormalizedStarsLightArmored = mCVStarsLightArmored / starsMax;
      mNormalizedStarsMedium = mCVStarsMedium / starsMax;
      mNormalizedStarsMediumAir = mCVStarsMediumAir / starsMax;
      mNormalizedStarsHeavy = mCVStarsHeavy / starsMax;
      mNormalizedStarsBuilding = mCVStarsBuilding / starsMax;         
   }
   else
   {
      mNormalizedStarsLight = 0.0f;
      mNormalizedStarsLightArmored = 0.0f;
      mNormalizedStarsMedium = 0.0f;
      mNormalizedStarsMediumAir = 0.0f;
      mNormalizedStarsHeavy = 0.0f;
      mNormalizedStarsBuilding = 0.0f;
   }
#ifdef aiCombatHack
   mNumSquads += a.mNumSquads;
#endif
}


//==============================================================================
//==============================================================================
float BAISquadAnalysis::getComponent(BAISquadAnalysisComponent c) const
{
   switch (c)
   {
   case AISquadAnalysisComponent::cCVLight:
      return (mCVLight);
   case AISquadAnalysisComponent::cCVLightArmored:
      return (mCVLightArmored);
   case AISquadAnalysisComponent::cCVMedium:
      return (mCVMedium);
   case AISquadAnalysisComponent::cCVMediumAir:
      return (mCVMediumAir);
   case AISquadAnalysisComponent::cCVHeavy:
      return (mCVHeavy);
   case AISquadAnalysisComponent::cCVBuilding:
      return (mCVBuilding);
   case AISquadAnalysisComponent::cCVTotal:
      return (mCVTotal); 
   case AISquadAnalysisComponent::cCVPercentLight:
      return (mCVPercentLight);
   case AISquadAnalysisComponent::cCVPercentLightArmored:
      return (mCVPercentLightArmored);
   case AISquadAnalysisComponent::cCVPercentMedium:
      return (mCVPercentMedium);
   case AISquadAnalysisComponent::cCVPercentMediumAir:
      return (mCVPercentMediumAir);
   case AISquadAnalysisComponent::cCVPercentHeavy:
      return (mCVPercentHeavy);
   case AISquadAnalysisComponent::cCVPercentBuilding:
      return (mCVPercentBuilding);
   case AISquadAnalysisComponent::cHPPercentLight:
      return (mHPPercentLight);   
   case AISquadAnalysisComponent::cHPPercentLightArmored:
      return (mHPPercentLightArmored);
   case AISquadAnalysisComponent::cHPPercentMedium:
      return (mHPPercentMedium);
   case AISquadAnalysisComponent::cHPPercentMediumAir:
      return (mHPPercentMediumAir);
   case AISquadAnalysisComponent::cHPPercentHeavy:
      return (mHPPercentHeavy);
   case AISquadAnalysisComponent::cHPPercentBuilding:
      return (mHPPercentBuilding);
   case AISquadAnalysisComponent::cCVStarsLight:
      return (mCVStarsLight);
   case AISquadAnalysisComponent::cCVStarsLightArmored:
      return (mCVStarsLightArmored);
   case AISquadAnalysisComponent::cCVStarsMedium:
      return (mCVStarsMedium);
   case AISquadAnalysisComponent::cCVStarsMediumAir:
      return (mCVStarsMediumAir);
   case AISquadAnalysisComponent::cCVStarsHeavy:
      return (mCVStarsHeavy);
   case AISquadAnalysisComponent::cCVStarsBuilding:
      return (mCVStarsBuilding);
   case AISquadAnalysisComponent::cCVStarsTotal:
      return (mCVStarsTotal);
   case AISquadAnalysisComponent::cNormalizedStarsLight:
      return (mNormalizedStarsLight);   
   case AISquadAnalysisComponent::cNormalizedStarsLightArmored:
      return (mNormalizedStarsLightArmored);
   case AISquadAnalysisComponent::cNormalizedStarsMedium:
      return (mNormalizedStarsMedium);
   case AISquadAnalysisComponent::cNormalizedStarsMediumAir:
      return (mNormalizedStarsMediumAir);
   case AISquadAnalysisComponent::cNormalizedStarsHeavy:
      return (mNormalizedStarsHeavy);
   case AISquadAnalysisComponent::cNormalizedStarsBuilding:
      return (mNormalizedStarsBuilding);
   default:
      return (0.0f);
   }
}


//==============================================================================
//==============================================================================
float BAISquadAnalysis::getNormalizedStars(BDamageTypeID damageTypeID) const
{
   if (damageTypeID == gDatabase.getDamageTypeLight())
      return (mNormalizedStarsLight);   
   else if (damageTypeID == gDatabase.getDamageTypeLightArmored())
      return (mNormalizedStarsLightArmored);
   else if (damageTypeID == gDatabase.getDamageTypeLightArmored())
      return (mNormalizedStarsLightArmored);
   else if (damageTypeID == gDatabase.getDamageTypeMedium())
      return (mNormalizedStarsMedium);
   else if (damageTypeID == gDatabase.getDamageTypeMediumAir())
      return (mNormalizedStarsMediumAir);
   else if (damageTypeID == gDatabase.getDamageTypeHeavy())
      return (mNormalizedStarsHeavy);
   else if (damageTypeID == gDatabase.getDamageTypeBuilding())
      return (mNormalizedStarsBuilding);
   else
      return (0.0f);
}

//==============================================================================
//==============================================================================
bool BAISquadAnalysis::save(BStream* pStream, int saveType) const
{
   GFWRITEVAR(pStream, float, mCVLight);
   GFWRITEVAR(pStream, float, mCVMedium);
   GFWRITEVAR(pStream, float, mCVMediumAir);
   GFWRITEVAR(pStream, float, mCVHeavy);
   GFWRITEVAR(pStream, float, mCVBuilding);
   GFWRITEVAR(pStream, float, mCVTotal);
   GFWRITEVAR(pStream, float, mHPLight);
   GFWRITEVAR(pStream, float, mHPMedium);
   GFWRITEVAR(pStream, float, mHPMediumAir);
   GFWRITEVAR(pStream, float, mHPHeavy);
   GFWRITEVAR(pStream, float, mHPBuilding);
   GFWRITEVAR(pStream, float, mHPTotal);
   GFWRITEVAR(pStream, float, mSPLight);
   GFWRITEVAR(pStream, float, mSPMedium);
   GFWRITEVAR(pStream, float, mSPMediumAir);
   GFWRITEVAR(pStream, float, mSPHeavy);
   GFWRITEVAR(pStream, float, mSPBuilding);
   GFWRITEVAR(pStream, float, mSPTotal);
   GFWRITEVAR(pStream, float, mDPSLight);
   GFWRITEVAR(pStream, float, mDPSMedium);
   GFWRITEVAR(pStream, float, mDPSMediumAir);
   GFWRITEVAR(pStream, float, mDPSHeavy);
   GFWRITEVAR(pStream, float, mDPSBuilding);
   GFWRITEVAR(pStream, float, mDPSTotal);
   GFWRITEVAR(pStream, float, mCVPercentLight);
   GFWRITEVAR(pStream, float, mCVPercentMedium);
   GFWRITEVAR(pStream, float, mCVPercentMediumAir);
   GFWRITEVAR(pStream, float, mCVPercentHeavy);
   GFWRITEVAR(pStream, float, mCVPercentBuilding);
   GFWRITEVAR(pStream, float, mHPPercentLight);
   GFWRITEVAR(pStream, float, mHPPercentMedium);
   GFWRITEVAR(pStream, float, mHPPercentMediumAir);
   GFWRITEVAR(pStream, float, mHPPercentHeavy);
   GFWRITEVAR(pStream, float, mHPPercentBuilding);
   GFWRITEVAR(pStream, float, mCVStarsLight);
   GFWRITEVAR(pStream, float, mCVStarsMedium);
   GFWRITEVAR(pStream, float, mCVStarsMediumAir);
   GFWRITEVAR(pStream, float, mCVStarsHeavy);
   GFWRITEVAR(pStream, float, mCVStarsBuilding);
   GFWRITEVAR(pStream, float, mCVStarsTotal);
   GFWRITEVAR(pStream, float, mNormalizedStarsLight);
   GFWRITEVAR(pStream, float, mNormalizedStarsMedium);
   GFWRITEVAR(pStream, float, mNormalizedStarsMediumAir);
   GFWRITEVAR(pStream, float, mNormalizedStarsHeavy);
   GFWRITEVAR(pStream, float, mNormalizedStarsBuilding);

#ifdef aiCombatHack
   GFWRITEVAR(pStream, uint, mNumSquads);
#else
   GFWRITEVAL(pStream, uint, 0);
#endif

   return true;
}

//==============================================================================
//==============================================================================
bool BAISquadAnalysis::load(BStream* pStream, int saveType)
{
   GFREADVAR(pStream, float, mCVLight);
   GFREADVAR(pStream, float, mCVMedium);
   GFREADVAR(pStream, float, mCVMediumAir);
   GFREADVAR(pStream, float, mCVHeavy);
   GFREADVAR(pStream, float, mCVBuilding);
   GFREADVAR(pStream, float, mCVTotal);
   GFREADVAR(pStream, float, mHPLight);
   GFREADVAR(pStream, float, mHPMedium);
   GFREADVAR(pStream, float, mHPMediumAir);
   GFREADVAR(pStream, float, mHPHeavy);
   GFREADVAR(pStream, float, mHPBuilding);
   GFREADVAR(pStream, float, mHPTotal);
   GFREADVAR(pStream, float, mSPLight);
   GFREADVAR(pStream, float, mSPMedium);
   GFREADVAR(pStream, float, mSPMediumAir);
   GFREADVAR(pStream, float, mSPHeavy);
   GFREADVAR(pStream, float, mSPBuilding);
   GFREADVAR(pStream, float, mSPTotal);
   GFREADVAR(pStream, float, mDPSLight);
   GFREADVAR(pStream, float, mDPSMedium);
   GFREADVAR(pStream, float, mDPSMediumAir);
   GFREADVAR(pStream, float, mDPSHeavy);
   GFREADVAR(pStream, float, mDPSBuilding);
   GFREADVAR(pStream, float, mDPSTotal);
   GFREADVAR(pStream, float, mCVPercentLight);
   GFREADVAR(pStream, float, mCVPercentMedium);
   GFREADVAR(pStream, float, mCVPercentMediumAir);
   GFREADVAR(pStream, float, mCVPercentHeavy);
   GFREADVAR(pStream, float, mCVPercentBuilding);
   GFREADVAR(pStream, float, mHPPercentLight);
   GFREADVAR(pStream, float, mHPPercentMedium);
   GFREADVAR(pStream, float, mHPPercentMediumAir);
   GFREADVAR(pStream, float, mHPPercentHeavy);
   GFREADVAR(pStream, float, mHPPercentBuilding);
   GFREADVAR(pStream, float, mCVStarsLight);
   GFREADVAR(pStream, float, mCVStarsMedium);
   GFREADVAR(pStream, float, mCVStarsMediumAir);
   GFREADVAR(pStream, float, mCVStarsHeavy);
   GFREADVAR(pStream, float, mCVStarsBuilding);
   GFREADVAR(pStream, float, mCVStarsTotal);
   GFREADVAR(pStream, float, mNormalizedStarsLight);
   GFREADVAR(pStream, float, mNormalizedStarsMedium);
   GFREADVAR(pStream, float, mNormalizedStarsMediumAir);
   GFREADVAR(pStream, float, mNormalizedStarsHeavy);
   GFREADVAR(pStream, float, mNormalizedStarsBuilding);

#ifdef aiCombatHack
   GFREADVAR(pStream, uint, mNumSquads);
#else
   uint numSquads;
   GFREADVAR(pStream, uint, numSquads);
#endif

   return true;
}
