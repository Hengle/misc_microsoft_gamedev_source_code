//==============================================================================
// squadactionwander.h
// Copyright (c) 2006-2008 Ensemble Studios
//==============================================================================

#pragma once
#include "action.h"

//==============================================================================
//==============================================================================
class BSquadActionWander : public BAction
{
   public:
      BSquadActionWander() { }
      virtual ~BSquadActionWander() { }

      //Connect/disconnect.
      virtual bool               connect(BEntity* pOwner, BSimOrder* pOrder);
      virtual void               disconnect();
      //Init.
      virtual bool               init();

      virtual bool               setState(BActionState state);
      virtual bool               update(float elapsed);

      virtual void               notify(DWORD eventType, BEntityID senderID, DWORD data1, DWORD data2);

      DECLARE_FREELIST(BSquadActionWander, 4);

      virtual bool save(BStream* pStream, int saveType) const;
      virtual bool load(BStream* pStream, int saveType);

   protected:
      void                       removeChildActions();

      BUnitOppID                 mOppID;
      BActionID                  mChildActionID;
      BVector                    mOrigin;
      float                      mWaitTime;
      BVector                    mForward;

      BVector                    mTarget; // This is just for debugging
};
