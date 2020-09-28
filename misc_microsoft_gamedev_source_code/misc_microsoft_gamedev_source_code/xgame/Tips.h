//=============================================================================
// tips.h
//
// Copyright (c) 2007 Ensemble Studios
//=============================================================================
#pragma once

// Includes
#include "xmlreader.h"


//=============================================================================
//=============================================================================
class BTips
{
   public:
      BTips();
      ~BTips();

      bool                 load();
      void                 reset();

      int                  getTipCount() const { return mTipStringIndexes.getNumber(); }
      const BUString*      getRandomTip() const;

      void                 getRandomLoadingScreenTips(BSmallDynamicArray<const BUString *>& randomTips);

      const BUString*      getTip(int i) const;

   protected:
      BSmallDynamicSimArray<long> mTipStringIndexes;
      BSmallDynamicSimArray<long> mLoadingScreenTips;
};
