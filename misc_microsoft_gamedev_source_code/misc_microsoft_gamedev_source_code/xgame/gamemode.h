//==============================================================================
// gamemode.h
// Copyright (c) 2006 Ensemble Studios
//==============================================================================
#pragma once

#include "xmlreader.h"

//==============================================================================
// BGameMode
//==============================================================================
class BGameMode
{
   public:
      BGameMode();
      ~BGameMode() {}

      bool load(BXMLNode root);

      const BSimString& getName() const { return mName; }
      const BSimString& getWorldScript() const { return mWorldScript; }
      const BSimString& getPlayerScript() const { return mPlayerScript; }
      const BSimString& getNPC() const { return mNPC; }
      int getTechID() const { return mTechID; }
      const BUString& getDisplayName() const;
      const BUString& getDescription() const;

   private:
      BSimString  mName;
      BSimString  mWorldScript;
      BSimString  mPlayerScript;
      BSimString  mNPC;
      int         mTechID;
      int         mDisplayNameIndex;
      int         mDescriptionIndex;
};
