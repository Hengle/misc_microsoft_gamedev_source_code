//==============================================================================
// lightVisualManager.h
//
// Copyright (c) 2006 Ensemble Studios
//==============================================================================
#pragma once

#include "xmlreader.h"

// Forward declarations
class BLightVisualData;
class BLightVisualInstance;

//==============================================================================
// BLightVisualManager
//==============================================================================
class BLightVisualManager
{
   public:
                              BLightVisualManager();
                             ~BLightVisualManager();                              

      bool                    init();
      void                    deinit();

      long                    createData(BXMLNode node);
      BLightVisualData*       getData(long index);

      BLightVisualInstance*   createInstance(long dataIndex, const BMatrix* pTransform);
      void                    releaseInstance(BLightVisualInstance* pInstance);

   protected:
      BDynamicArray<BLightVisualData*> mDataList;
};

// Globals
extern BLightVisualManager gLightVisualManager;
