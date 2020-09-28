//==============================================================================
// textvisualmanager.h
//
// Copyright (c) 2005-2007, Ensemble Studios
//==============================================================================
#pragma once

//==============================================================================
// Includes

//==============================================================================
// Forward declarations
class BTextVisual;
class BTextVisualDef;


//==============================================================================
// Defines.

//==============================================================================
class BTextVisualManager;
extern BTextVisualManager gTextVisualManager;

class BTextVisualManager
{
   public:
                              BTextVisualManager();
                              ~BTextVisualManager();
                              
      void                    update(DWORD elapsedTime);
      void                    render(int viewportIndex, bool bSplitScreen);
      void                    reset();
      
      long                    createVisual(void);
      BTextVisual             *getVisual(long id);
      void                    destroyVisual(long id);
      
      long                    getOrCreateVisualDef(const BCHAR_T *filename);
      BTextVisualDef          *getVisualDef(long id);

      void                    renderReset(void);
      void                    reload(BCHAR_T *filename);

      void                    create(long defID, long playerID, const BUString &text, const BVector &anchorPos, BDynamicSimLongArray *resultIDs=NULL);

      // jce [11/15/2005] -- hacky template access w/ fun string lookups.
      void                    create(const BCHAR_T *defName, long playerID, const BUString &text, const BVector &anchorPos, BDynamicSimLongArray *resultIDs=NULL);
      
   protected:
      void                    cleanup(void);
      long                    createDef(const BCHAR_T *filename);


      BDynamicSimArray<BTextVisual*>            mTextVisuals;
      
      BDynamicSimArray<BTextVisualDef*>         mTextVisualDefs;
      BStringTable<long>                        mDefNameTable;
      
      BDynamicSimLongArray                      mUnusedIndices;
};