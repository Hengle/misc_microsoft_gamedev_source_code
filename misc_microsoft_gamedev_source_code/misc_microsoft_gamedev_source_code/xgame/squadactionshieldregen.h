//==============================================================================
// squadactionshieldregen.h
// Copyright (c) 2006-2007 Ensemble Studios
//==============================================================================

#pragma once
#include "action.h"

class BUnit;


//==============================================================================
//==============================================================================
class BSquadActionShieldRegen : public BAction
{
   public:

      BSquadActionShieldRegen() { }
      virtual ~BSquadActionShieldRegen() { }

      //Init.
      virtual bool               init();

      virtual bool               update(float elapsed);

      DECLARE_FREELIST(BSquadActionShieldRegen, 5);

   protected:


};
