//==============================================================================
// squadlosvalidator.h
//
// Copyright (c) 2005-2007 Ensemble Studios
//==============================================================================
#pragma once

//==============================================================================
//Includes.

//==============================================================================
//Forward declarations.
class BSquad;
class BProtoAction;

//==============================================================================
// BSquadLOSValidator
//==============================================================================
class BSquadLOSValidator
{
   public:

      //Constructors and Destructor.
      BSquadLOSValidator( void );
      ~BSquadLOSValidator( void );

      bool                       validateLOS(const BSquad *sourceSquad, const BSquad *targetSquad);
      bool                       validateLOS(const BSquad *sourceSquad, const BProtoAction *pSourceSquadAction, const BSquad *targetSquad);
      bool                       validateLOS(BVector sourcePos, BVector targetPos, const BSquad *sourceSquad, const BProtoAction *pSourceSquadAction, const BSquad *targetSquad, const BEntityIDArray* ignoreList = NULL);

      //Render methods.
      void                       render( void ) const;

      //Reset.
      void                       reset( void );
};

extern BSquadLOSValidator gSquadLOSValidator;

