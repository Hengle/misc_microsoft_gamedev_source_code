//==============================================================================
// techtree.h
//
// Copyright (c) 2006-2007 Ensemble Studios
//==============================================================================
#pragma once

#include "gamefilemacros.h"

// Forward declarations
class BPlayer;
class BTechUnique;

//==============================================================================
// BTechNode
//==============================================================================
class BTechNode
{
   public:
      BTechNode() :
         mUnique(false),
         mStatus(0),
         mResearchPoints(0.0f),
         mResearchBuilding(-1),
         mpUniqueNodes(NULL)
      {
      }
      BDynamicSimArray<BTechUnique>* mpUniqueNodes;
      float                      mResearchPoints;
      long                       mResearchBuilding;
      BYTE                       mStatus;
      bool                       mUnique;
};

//==============================================================================
// BTechUnique
//==============================================================================
class BTechUnique : public BTechNode
{
   public:
      BTechUnique() :
         mUnitID(-1)
      {
      }
      long                       mUnitID;
};

//==============================================================================
// BTechTree
//==============================================================================
class BTechTree
{
   public:
      enum
      {
         cStatusUnobtainable,
         cStatusObtainable,
         cStatusAvailable,
         cStatusResearching,
         cStatusActive,
         cStatusDisabled,
         cStatusCoopResearching,
      };

                                 BTechTree();
                                 ~BTechTree();

      bool                       init(BPlayer*  pPlayer, bool fromSave);

      BPlayer*                   getPlayer() { return mpPlayer; }
      void                       setPlayer(BPlayer* pPlayer) { mpPlayer=pPlayer; }

      long                       getTechCount() const { return mTechCount; }
      BTechNode*                 getTechNodes() { return mpTechNodes; }

      const BTechNode*           getTechNodeConst(long techID, long unitID) const;
      long                       getTechStatus(long techID, long unitID) const;

      void                       addUnitRef(long techID, long unitID);

      bool                       researchTech(long techID, long unitID, bool fromCoop=false);
      bool                       unresearchTech(long techID, long unitID);

      bool                       activateTech(long techID, long unitID, bool noCost=false, bool fromCoop=false);
      bool                       deactivateTech(long techID, long unitID);

      bool                       makeObtainable(long techID, long unitID);

      void                       setResearchPoints(long techID, long unitID, float points);
      float                      getResearchPercent(long techID, long unitID) const;
      long                       getResearchBuilding(long techID, long unitID) const;

      void                       checkUnitPrereq(long protoUnitID);

      GFDECLAREVERSION();
      bool save(BStream* pStream, int saveType) const;
      bool load(BStream* pStream, int saveType);

   protected:
      BTechNode*                 getTechNode(long techID, long unitID, bool forceAdd=false);
      bool                       checkTech(long techID, long unitID, BTechNode* pTechNode, bool fromCoop=false);

      BPlayer*                   mpPlayer;
      long                       mTechCount;
      BTechNode*                 mpTechNodes;
      int                        mInitTechID;

};