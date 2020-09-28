//==============================================================================
// UnitOpportunity.cpp
// Copyright (c) 2005-2007 Ensemble Studios
//==============================================================================

#include "Common.h"
#include "UnitOpportunity.h"
#include "SimOrderManager.h"


//==============================================================================
//==============================================================================
IMPLEMENT_FREELIST(BUnitOpp, 5, &gSimHeap);


//==============================================================================
//==============================================================================
BUnitOppID BUnitOpp::generateID()
{
   mID=gSimOrderManager.getNextOppID();
   return (mID);
}

//==============================================================================
//==============================================================================
void BUnitOpp::setPath(const BDynamicSimVectorArray& path)
{
   mPath.resize(path.getSize());
   for (uint i=0; i < mPath.getSize(); i++)
      mPath[i]=path[i];
}

//==============================================================================
//==============================================================================
const char* BUnitOpp::getTypeName() const
{
   switch (mType)
   {
      case cTypeNone: return("None"); 
	   case cTypeMove: return("Move"); 
      case cTypeAttack: return("Attack");
      case cTypeSecondaryAttack: return("SecondaryAttack");
      case cTypeWork: return("Work");
      case cTypeMines: return("Mines");
      case cTypeCapture: return("Capture");
      case cTypeJoin: return("Join");
      case cTypeChangeMode: return("ChangeMode");
      case cTypeGarrison: return("Garrison");
      case cTypeEvade: return("Evade");
      case cTypeCheer: return("Cheer");
      case cTypeUngarrison: return("Ungarrison");
      case cTypeRetreat: return("Retreat");
      case cTypeAnimation: return("Animation");
      case cTypeReload: return("Reload");
      case cTypeGather: return("Gather");
      case cTypeRepair: return("Repair");
      case cTypeHeal: return("Heal");
      case cTypeDeath: return("Death");
      case cTypeHeroDeath: return ("HeroDeath");
      case cTypeThrown: return("Thrown");
      case cTypeDetonate: return ("Detonate");
      case cTypeJump: return ("Jump");
      case cTypeJumpGather: return ("JumpGather");
      case cTypeJumpGarrison: return ("JumpGarrison");
      case cTypeJumpAttack: return ("JumpAttack");
      case cTypePointBlankAttack: return ("PointBlankAttack");
      case cTypeEnergyShield: return ("EnergyShield");
      case cTypeJumpPull: return ("JumpPull");
      case cTypeInfantryEnergyShield: return ("InfantryEnergyShield");
   }
   return ("Unknown");
}

//==============================================================================
//==============================================================================
bool BUnitOpp::save(BStream* pStream, int saveType) const
{
   GFWRITEVECTORARRAY(pStream, mPath, uint8, 200);
   GFWRITECLASS(pStream, saveType, mTarget);
   GFWRITEVAR(pStream, BEntityID, mSource);
   GFWRITEVAR(pStream, BUnitOppID, mID);
   GFWRITEVAR(pStream, BUnitOppType, mType);
   GFWRITEVAR(pStream, uint16, mUserData);
   GFWRITEVAR(pStream, uint8, mPriority);
   GFWRITEVAR(pStream, uint8, mUserData2);
   GFWRITEVAR(pStream, ushort, mWaitCount);
   GFWRITEBITBOOL(pStream, mEvaluated);
   GFWRITEBITBOOL(pStream, mExistForOneUpdate);
   GFWRITEBITBOOL(pStream, mExistUntilEvaluated);
   GFWRITEBITBOOL(pStream, mAllowComplete);
   GFWRITEBITBOOL(pStream, mNotifySource);
   GFWRITEBITBOOL(pStream, mLeash);
   GFWRITEBITBOOL(pStream, mForceLeash);
   GFWRITEBITBOOL(pStream, mTrigger);
   GFWRITEBITBOOL(pStream, mRemoveActions);
   GFWRITEBITBOOL(pStream, mComplete);
   GFWRITEBITBOOL(pStream, mCompleteValue);
   GFWRITEBITBOOL(pStream, mPreserveDPS);
   GFWRITEBITBOOL(pStream, mMustComplete);
   GFWRITEBITBOOL(pStream, mUserDataSet);
   return true;
}

//==============================================================================
//==============================================================================
bool BUnitOpp::load(BStream* pStream, int saveType)
{
   GFREADVECTORARRAY(pStream, mPath, uint8, 200);
   GFREADCLASS(pStream, saveType, mTarget);
   GFREADVAR(pStream, BEntityID, mSource);
   GFREADVAR(pStream, BUnitOppID, mID);
   GFREADVAR(pStream, BUnitOppType, mType);
   GFREADVAR(pStream, uint16, mUserData);
   GFREADVAR(pStream, uint8, mPriority);
   GFREADVAR(pStream, uint8, mUserData2);
   GFREADVAR(pStream, ushort, mWaitCount);
   GFREADBITBOOL(pStream, mEvaluated);
   GFREADBITBOOL(pStream, mExistForOneUpdate);
   GFREADBITBOOL(pStream, mExistUntilEvaluated);
   GFREADBITBOOL(pStream, mAllowComplete);
   GFREADBITBOOL(pStream, mNotifySource);
   GFREADBITBOOL(pStream, mLeash);
   GFREADBITBOOL(pStream, mForceLeash);
   GFREADBITBOOL(pStream, mTrigger);
   GFREADBITBOOL(pStream, mRemoveActions);
   GFREADBITBOOL(pStream, mComplete);
   GFREADBITBOOL(pStream, mCompleteValue);
   GFREADBITBOOL(pStream, mPreserveDPS);
   GFREADBITBOOL(pStream, mMustComplete);
   GFREADBITBOOL(pStream, mUserDataSet);
   return true;
}
