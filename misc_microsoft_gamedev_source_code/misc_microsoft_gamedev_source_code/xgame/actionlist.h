//==============================================================================
// actionlist.h
// Copyright (c) 2005-2007 Ensemble Studios
//==============================================================================

#pragma once
#include "SimTypes.h"
#include "gamefilemacros.h"

class BAction;
class BEntity;
class BSimOrder;
class BUnitOpp;


//==============================================================================
//==============================================================================
class BActionList
{
   public:
      BActionList() { }
      virtual ~BActionList() { }

      bool                       addAction(BAction* pAction, BEntity* pEntity, BSimOrder* pOrder);
      bool                       removeAction(BAction *pAction);
      bool                       removeActionByIndex(uint index);
      bool                       removeActionByID(BActionID id);
      bool                       removeAllActionsOfType(BActionType type);
      bool                       pauseAllActionsForOrder(BSimOrder* pOrder);
      bool                       unpauseAllActionsForOrder(BSimOrder* pOrder);
      bool                       removeAllActionsForOrder(BSimOrder* pOrder);
      bool                       changeOrderForAllActions(BSimOrder* pOldOrder, BSimOrder* pNewOrder);
      bool                       removeAllActionsForOpp(BUnitOpp* pOpp);
      bool                       removeAllConflicts(BAction* pAction);
      void                       clearActions();
      void                       clearNonPersistentActions();
      void                       clearPersistentTacticActions();

      long                       getNumberActions() const { return ((long)mActions.getSize()); }
      BAction*                   getActionByID(BActionID id) const;
      BAction*                   getAction(uint index) { return (mActions[index]); }
      const BAction*             getAction(uint index) const { return (mActions[index]); }
      BAction*                   getActionByType(BActionType type, bool ignoreDisabled = false);
      const BAction*             getActionByType(BActionType type, bool ignoreDisabled = false) const;

      void                       update(float elapsed);
      void                       notify(DWORD eventType, BEntityID senderID, DWORD data, DWORD data2);
      bool                       hasConflictsWithType(BActionType type) const;

      void                       debugRender();

      bool save(BStream* pStream, int saveType) const;
      bool load(BStream* pStream, int saveType);
      bool postLoad(int saveType);

 protected:

      BDynamicSimArray<BAction*> mActions;
};