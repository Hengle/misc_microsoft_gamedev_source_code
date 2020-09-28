//==============================================================================
// uihelp.h
//
// Copyright (c) 2005-2007 Ensemble Studios
//==============================================================================
#pragma once

//==============================================================================
// Includes
#include "uielement.h"

class BUser;

//==============================================================================
// Forward declarations
class BInputInterface;

//==============================================================================
class BUIHelp : public BUIElement
{
   public:
      enum
      {
         cIconButtonA,
         cIconButtonB,
         cIconButtonX,
         cIconButtonY,
         cIconDpad,
         cIconcButtonShoulderLeft,
         cIconcButtonShoulderRight,
         cNumberIcons,
      };

      enum
      {
         cLabelButtonA,
         cLabelButtonB,
         cLabelButtonX,
         cLabelButtonY,
         cLabelDpadUp,
         cLabelDpadRight,
         cLabelDpadLeft,
         cLabelDpadDown,
         cLabelDpad,
         cLabelButtonShoulderLeft,
         cLabelButtonShoulderRight,
         cNumberLabels,
      };

                                 BUIHelp();
      virtual                    ~BUIHelp();

      void                       init(BUser* pUser);
      virtual void               refresh( long configIndex );
      virtual void               update( long configIndex );
      virtual void               render(long parentX, long parentY);
      virtual void               cleanup();

   protected:
      enum
      {
         cMaxGroups=4,
      };

      void                       setupElements();
      void                       getCircleMenuState(bool& circleMenuUp, long& currentItemType, long& currentItemID, bool& dequeue);
      void                       getPowerState(bool& powerAvailable);
      void                       getModifierState(bool& modifier);
      void                       getGotoState(bool& gotoBase, bool& gotoEvent, bool& gotoArmy, bool& gotoScout, bool& gotoNode, bool& gotoHero, bool& gotoRally);
      void                       getGroupState(bool* pGroupAssigned);
      void                       setLabelText(long id, const WCHAR* pText);

      long                       getLabelFromControlType(long controlType);
      long                       getLabelForFunction(BInputInterface* pInputInterface, long inputFunc);
      bool                       setLableTextForFunction(BInputInterface* pInputInterface, long inputFunc, long stringID, bool allowHold);
      void                       setLabelIconForFunction(BInputInterface* pInputInterface, long inputFunc);
      void                       setButtonStates();
      void                       hideReticuleButtons();
      void                       setReticuleButton(long reticuleLabel, long frameID);
      void                       updateAbilityButton(long configIndex);
      void                       updateReticuleButtons();

      bool                       isHoldControl(BInputInterface* pInputInterface, long inputFunc);

      BUser*                     mpUser;
      bool                       mInitialized;
      BUIElement                 mIcons[cNumberIcons];
      BUIElement                 mLabels[cNumberLabels];

      long                       mBaseTypeID;

      long                       mUserMode;
      long                       mSubMode;
      BEntityID                  mSelectedSquad;
      BEntityID                  mHoverObject;
      bool                       mHoverSelected;
      long                       mHoverType;
      long                       mHoverResource;
      long                       mHoverAbilityLabelID;
      bool                       mHoverAbilityAvailable;

      bool                       mCanMove;
      bool                       mCircleSelecting;
      bool                       mAnySelected;
      bool                       mSubSelectMode;
      bool                       mSubSelectTag;
      uint                       mNumSubSelectGroups;
      bool                       mCircleMenuUp;
      long                       mCurrentItemType;
      long                       mCurrentItemID;
      bool                       mDequeue;
      bool                       mPowerAvailable;
      bool                       mModifier;

      bool                       mGotoBase;
      bool                       mGotoScout;
      bool                       mGotoNode;
      bool                       mGotoHero;
      bool                       mGotoRally;
      bool                       mGotoEvent;
      bool                       mGotoArmy;

      DWORD                      mGotoItemTime;
      DWORD                      mGotoEventTime;
      DWORD                      mGotoArmyTime;

      bool                       mGotoBaseCache;
      bool                       mGotoScoutCache;
      bool                       mGotoNodeCache;
      bool                       mGotoHeroCache;
      bool                       mGotoRallyCache;
      bool                       mGotoEventCache;
      bool                       mGotoArmyCache;

      int                        mGroupIndex;
      bool                       mGroupAssigned[cMaxGroups];

};