//==============================================================================
// selectionmanager.h
//
// Copyright (c) 1999-2007 Ensemble Studios
//==============================================================================
#pragma once

// Includes
#include "bitarray.h"
#include "simtypes.h"

// Forward declarations
class BObject;
class BUser;

//==============================================================================
// BSelectionPossibility
//==============================================================================
class BSelectionPossibility
{
   public:

      enum
      {  
         cExact,
         cClose,
         cExactThroughOther
      };

      BEntityID                  mID;
      long                       mPriority;
      float                      mDistance;
      short                      mSelectionMethod;
      bool                       mOutlined;
};
typedef BDynamicSimArray<BSelectionPossibility> BSelectionPossibilityArray;


//==============================================================================
// BSelectionManager
//==============================================================================
class BSelectionManager
{
   public:
      enum
      {
         cMaxSubSelectGroups=15,
         cMaxUISubSelectGroupIndex=7,
      };

      // Init functions.
      BSelectionManager();
      ~BSelectionManager();
      void                       attachUser(BUser *pUser);
      void                       detachUser(void);

      bool                       selectUnit(BEntityID unitID);
      bool                       unselectUnit(BEntityID unitID);
      void                       clearSelections();

      bool                       selectSquad(BEntityID squadID);
      void                       unselectSquad(BEntityID squadID);
      bool                       selectSquads(BEntityIDArray& squads);

      bool                       isUnitSelected(BEntityID unitID);
      bool                       isSquadSelected(BEntityID squadID);

      long                       getNumberSelectedUnits() { return(mSelectedUnits.getNumber()); }
      long                       getNumberSelectedSquads() { return(mSelectedSquads.getNumber()); }

      const BEntityIDArray&      getSelectedSquads() const { return(mSelectedSquads); }
      const BEntityIDArray&      getSelectedUnits() const { return(mSelectedUnits); }

      void                       fixupPlayableBoundsSelections();

      // TODO:  Fix me
      BEntityID                  getSelected(long index) { if(index<0 || index>=mSelectedUnits.getNumber()) return cInvalidObjectID; else return mSelectedUnits[index]; }
      BEntityID                  getSelectedSquad(long index) { if(index<0 || index>=mSelectedSquads.getNumber()) return cInvalidObjectID; else return mSelectedSquads[index]; }
      void                       populateSelectionsRect(BVector cameraLoc, BVector point, float xSize, float ySize, bool drawMode, bool targetMode);
      void                       populateSelectionsCircle(BVector cameraLoc, BVector* points, long pointCount, float pixelError, bool drawMode);
      void                       sortPossibleSelectionList();
      BEntityID                  getNextPossibleSelection(bool startOfList);
      long                       getPossibleSelectionCount() const {return(mPossibleSelections.getNumber());}
      BEntityID                  getPossibleSelection(long index) const;
      void                       clearPossibleSelectionList() { mPossibleSelections.setNumber(0); }
      // TODO:  Fix me

      int                        getSubSelectGroupHandle() const { return mSubSelectGroupHandle; }
      int                        getSubSelectSquadHandle() const { return mSubSelectSquadHandle; }

      int                        getSubSelectGroupUIStartIndex() const { return mSubselectUIStartIndex; };
      int                        getSubSelectGroupUIEndIndex() const { return mSubselectUIEndIndex; };

      int                        subSelectNext();
      int                        subSelectPrev();
      void                       subSelectBySquad();
      void                       subSelectByType();
      bool                       isSubSelectBySquad() { return mFlagSubSelectingSquads; }
      void                       subSelectTag();
      void                       subSelectSelect();
      void                       clearCurrentSubSelect();
      bool                       getSubSelectedUnits(BEntityIDArray& unitList) const;
      bool                       getSubSelectedSquads(BEntityIDArray& squadList) const;
      uint                       getNumSubSelectGroups() const { return mNumSubSelectGroups; }
      const BEntityIDArray&      getSubSelectGroup(uint index) const { return mSubSelectGroups[index]; }
      BSelectionAbility          getSubSelectAbility(uint index) const { return mSubSelectAbility[index]; }
      uint                       getNumSubSelectSquads() const;
      BEntityID                  getSubSelectSquad(uint index) const;
      BSelectionAbility          getSubSelectAbility() const;
      int                        getSubSelectAbilityID() const { return getSubSelectAbility().mAbilityID; }
      bool                       getSubSelectAbilityValid() const;
      bool                       getSubSelectAbilityRecovering() const;
      bool                       getSubSelectAbilityPlayer() const;
      int                        getSubSelectAbilityType() const { return getSubSelectAbility().mAbilityType; }
      int                        getSubSelectAbilityTargetType() const { return getSubSelectAbility().mTargetType; }
      bool                       getSubSelectAbilityReverse() const { return getSubSelectAbility().mReverse; }
      bool                       getSubSelectAbilityTargetUnit() const { return getSubSelectAbility().mTargetUnit; }
      bool                       isSubSelected(BEntityID squadID) const;
      bool                       getSubSelectTag(uint index) const { return mSubSelectTag[index]; }
      void                       saveSubSelect();
      void                       restoreSubSelect();

      uint                       getSelectionChangeCounter() const { return mSelectionChangeCounter; }

      void                       recomputeSubSelection();

      void                       updateSubSelectAbilities(BEntityID hoverObject, BVector hoverPoint);

      // Flags
      bool                       getFlagAbilityAvailable() const { return mFlagAbilityAvailable; }

   protected:
      void                       markUnitSelected(BEntityID unitID);
      void                       markUnitUnselected(BEntityID unitID);

      void                       calculateSelectedSquads();
      void                       calculateSubSelectGroups();
      void                       calculateSubSelectAbilities();      

      int                        getSquadSortValue(BEntityID squadID);

      // TODO:  Fix me
      long                       comparePossibleSelections(const BSelectionPossibility& sp1, const BSelectionPossibility& sp2);
      long                       getSelectionScore(const BSelectionPossibility& sp);
      // TODO:  Fix me

      BEntityIDArray             mSelectedUnits;
      BBitArray                  mIsUnitSelected;

      BEntityIDArray             mSelectedSquads;
      BBitArray                  mIsSquadSelected;

      BSelectionPossibilityArray mPossibleSelections;
      BUser*                     mpUser;

      int                        mSubSelectGroupHandle;
      int                        mSubSelectSquadHandle;
      BEntityIDArray             mSubSelectGroups[cMaxSubSelectGroups];
      BSelectionAbility          mSubSelectAbility[cMaxSubSelectGroups];
      bool                       mSubSelectTag[cMaxSubSelectGroups];
      uint                       mNumSubSelectGroups;
      int                        mSaveSubSelectSort;
      int                        mSaveSubSelectGroupHandle;

      uint                       mSelectionChangeCounter;
      int                        mSubselectUIStartIndex;
      int                        mSubselectUIEndIndex;

      bool                       mFlagSubSelectingSquads:1;
      bool                       mFlagAbilityAvailable:1;
      bool                       mFlagAbilityValid:1;
      bool                       mFlagAbilityRecovering:1;
      bool                       mFlagAbilityPlayer:1;
};