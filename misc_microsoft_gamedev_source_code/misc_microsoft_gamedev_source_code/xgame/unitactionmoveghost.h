//==============================================================================
// unitactionmoveghost.h
// Copyright (c) 2005-2007 Ensemble Studios
//==============================================================================

#pragma once

#include "unitactionmovewarthog.h"


//==============================================================================
// Unit move action for the physics ghost.  Instead of setting unit position
// and orientation, it sets a target position that is queried by the havok
// physics action to actually move the unit.
//==============================================================================
class BUnitActionMoveGhost : public BUnitActionMoveWarthog
{
   public:
      BUnitActionMoveGhost () { }
      virtual ~BUnitActionMoveGhost () { }

      DECLARE_FREELIST(BUnitActionMoveGhost, 5);

   protected:

      virtual void   updateSkidAnimState() {}

};
