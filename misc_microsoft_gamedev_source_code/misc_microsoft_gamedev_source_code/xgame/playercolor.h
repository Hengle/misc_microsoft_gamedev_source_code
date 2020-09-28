//==============================================================================
// playercolor.h
//
// Copyright (c) 2003-2007 Ensemble Studios
//==============================================================================
#pragma once

// Includes
#include "maximumsupportedplayers.h"
#include "xmlreader.h"

// Forward declarations

//==============================================================================
// BPlayerColor
//==============================================================================
class BPlayerColor
{
   public:
                           BPlayerColor() : mObjects(cDWORDWhite), mCorpse(cDWORDWhite), mMinimap(cDWORDWhite), mSelection(cDWORDWhite), mUI(cDWORDWhite) { }
      
      bool                 load(BXMLNode node);

      DWORD                mObjects;
      DWORD                mCorpse;
      DWORD                mSelection;
      DWORD                mMinimap;
      DWORD                mUI;
};