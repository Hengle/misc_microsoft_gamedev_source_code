//==============================================================================
// squadactionSpiritBond.h
// Copyright (c) 2006-2007 Ensemble Studios
//==============================================================================

#pragma once
#include "Action.h"

class BGrannyInstance;
class BBitArray;

//==============================================================================
//==============================================================================
class BSquadActionSpiritBond : public BAction
{
public:
   BSquadActionSpiritBond() { }
   virtual ~BSquadActionSpiritBond() { }

   //Connect/disconnect.
   virtual bool               connect(BEntity* pOwner, BSimOrder* pOrder);
   virtual void               disconnect();
   //Init.
   virtual bool               init();

   virtual bool               setState(BActionState state);
   virtual bool               update(float elapsed);

   DECLARE_FREELIST(BSquadActionSpiritBond, 4);

   virtual bool save(BStream* pStream, int saveType) const;
   virtual bool load(BStream* pStream, int saveType);

protected:
   void createBeam();
   void updateBeam();
   void destroyBeam();

   void buffSquad();
   void unbuffSquad();

   BEntityID   mBeamID;
};