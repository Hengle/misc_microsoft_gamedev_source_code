//==============================================================================
// uihelp.cpp
//
// Copyright (c) 2005, Ensemble Studios
//==============================================================================

//==============================================================================
// Includes
#include "common.h"
#include "uihelp.h"
#include "ability.h"
#include "alert.h"
#include "config.h"
#include "configsgame.h"
#include "FontSystem2.h"
#include "object.h"
#include "player.h"
#include "powerentry.h"
#include "protoobject.h"
#include "selectionmanager.h"
#include "techtree.h"
#include "uigame.h"
#include "unit.h"
#include "user.h"
#include "usermanager.h"
#include "world.h"
#include "uimanager.h"
#include "selectionmanager.h"
//#include "inputcontrolenum.h"
//#include "inputinterface.h"

// Constants
const DWORD cGotoItemInterval=1500;
const DWORD cGotoEventInterval=100;
const DWORD cGotoArmyInterval=1200;
const DWORD cAbilitiesInterval=200;

//==============================================================================
// BUIHelp::BUIHelp
//==============================================================================
BUIHelp::BUIHelp() :
   mInitialized(false),
   mUserMode(BUser::cUserModeNormal),
   mSubMode(BUser::cSubModeNone),
   mSelectedSquad(cInvalidObjectID),
   mHoverObject(cInvalidObjectID),
   mHoverSelected(false),
   mHoverType(-1),
   mHoverResource(-1),
   mHoverAbilityLabelID(-1),
   mHoverAbilityAvailable(false),
   mCanMove(false),
   mCircleSelecting(false),
   mAnySelected(false),
   mNumSubSelectGroups(0),
   mSubSelectMode(false),
   mSubSelectTag(false),
   mCircleMenuUp(false),
   mCurrentItemType(-1),
   mCurrentItemID(-1),
   mDequeue(false),
   mPowerAvailable(false),
   mModifier(false),
   mGotoBase(false),
   mGotoScout(false),
   mGotoNode(false),
   mGotoHero(false),
   mGotoRally(false),
   mGotoEvent(false),
   mGotoArmy(false),
   mGotoItemTime(0),
   mGotoEventTime(0),
   mGotoArmyTime(0),
   mGotoBaseCache(false),
   mGotoScoutCache(false),
   mGotoNodeCache(false),
   mGotoHeroCache(false),
   mGotoRallyCache(false),
   mGotoEventCache(false),
   mGotoArmyCache(false),
   mGroupIndex(-1),
   mpUser(NULL)
{
   for(long i=0; i<cMaxGroups; i++)
      mGroupAssigned[i]=false;
}

//==============================================================================
// BUIHelp::~BUIHelp
//==============================================================================
BUIHelp::~BUIHelp()
{
   mpUser = NULL;
}

//==============================================================================
//==============================================================================
void BUIHelp::init(BUser* pUser)
{
   mpUser = pUser;
}

//==============================================================================
// BUIHelp::cleanup
//==============================================================================
void BUIHelp::cleanup()
{
   for(long i=0; i<cNumberIcons; i++)
      mIcons[i].cleanup();
   for(long i=0; i<cNumberLabels; i++)
      mLabels[i].cleanup();

   mpUser = NULL;
}

//==============================================================================
// BUIHelp::refresh
//==============================================================================
void BUIHelp::refresh( long configIndex )
{
   BUIElement::refresh();

   // Init
   if(!mInitialized)
   {
      BUIRect*  pos=gUIGame.getHelpPosition();
      setPosition(pos->mX1, pos->mY1);
      setSize(pos->mX2-pos->mX1+1, pos->mY2-pos->mY1+1);
      setupElements();
      mInitialized=true;
   }

   // Hide all of the UI elements
   for (long i=0; i<cNumberIcons; i++)
      mIcons[i].setHidden(true);
   for (long i=0; i<cNumberLabels; i++)
      mLabels[i].setHidden(true);

   if (gConfig.isDefined(cConfigFlashGameUI))
   {
      if (mpUser->getUIContext())
         mpUser->getUIContext()->setAllHUDLabelTextVisible(false);         
   }

   // Input interface
   BInputInterface* pInputInterface = &gInputSystem.getInputInterfaces()->get( configIndex );

   // set up the reticule label
   long reticuleLabel = -1;
   
   // Figure out which buttons are mapped to which functions
   long gameSelectLabel = getLabelForFunction(pInputInterface, BInputInterface::cInputSelection);
   long gameActionLabel = getLabelForFunction(pInputInterface, BInputInterface::cInputDoWork);
   long gameUnselectLabel = getLabelForFunction(pInputInterface, BInputInterface::cInputClear);
   long gameAbilityLabel = getLabelForFunction(pInputInterface, BInputInterface::cInputAbility);
   long gamePowersLabel = getLabelForFunction(pInputInterface, BInputInterface::cInputPowers);
   long gameConfirmLabel = getLabelForFunction(pInputInterface, BInputInterface::cInputSelection);
   long gameCancelLabel = getLabelForFunction(pInputInterface, BInputInterface::cInputClear);
   long menuOKLabel = getLabelForFunction(pInputInterface, BInputInterface::cInputSelection);
   long menuCancelLabel = getLabelForFunction(pInputInterface, BInputInterface::cInputClear);
   long menuAlternateLabel = getLabelForFunction(pInputInterface, BInputInterface::cInputDoWork);
   long menuExtraInfo = getLabelForFunction(pInputInterface, BInputInterface::cInputDisplayExtraInfo);

   // Gather state information
   mUserMode=mpUser->getUserMode();
   mSubMode=mpUser->getSubMode();
   mHoverObject=mpUser->getHoverObject();
   mHoverSelected=mpUser->getSelectionManager()->isUnitSelected(mHoverObject);
   mHoverType=mpUser->getHoverType();
   mHoverResource=mpUser->getHoverResource();
   mHoverAbilityLabelID = gameAbilityLabel;
   mHoverAbilityAvailable=mpUser->getSelectionManager()->getFlagAbilityAvailable();
   mCircleSelecting=(mUserMode==BUser::cUserModeCircleSelecting);
   mAnySelected=mpUser->getSelectionManager()->getNumberSelectedUnits()>0;
   mNumSubSelectGroups=mpUser->getSelectionManager()->getNumSubSelectGroups();

   int subSelectGroupHandle=mpUser->getSelectionManager()->getSubSelectGroupHandle();
   mSubSelectMode=(subSelectGroupHandle!=-1);
   mSubSelectTag=(subSelectGroupHandle!=-1 && mpUser->getSelectionManager()->getSubSelectTag(subSelectGroupHandle));

   mSelectedSquad=cInvalidObjectID;
//-- FIXING PREFIX BUG ID 3638
   const BObject* pSelectedObject=NULL;
//--
   if(mAnySelected)
   {
      pSelectedObject=gWorld->getObject(mpUser->getSelectionManager()->getSelected(0));
      if(pSelectedObject && pSelectedObject->getPlayerID()==mpUser->getPlayerID())
         mSelectedSquad=pSelectedObject->getParentID();
   }

//-- FIXING PREFIX BUG ID 3639
   const BObject* pHoverObject = gWorld->getObject(mHoverObject);
//--
   int selectType = (pHoverObject ? pHoverObject->getProtoObject()->getSelectType() : -1);

   mCanMove=false;
   int numSquads=mpUser->getSelectionManager()->getNumberSelectedSquads();
   for (int i=0; i<numSquads; i++)
   {
//-- FIXING PREFIX BUG ID 3634
      const BSquad* pSquad=gWorld->getSquad(mpUser->getSelectionManager()->getSelectedSquad(i));
//--
      if (pSquad && pSquad->canMove())
      {
         mCanMove=true;
         break;
      }
   }

   getCircleMenuState(mCircleMenuUp, mCurrentItemType, mCurrentItemID, mDequeue);
   getPowerState(mPowerAvailable);
   getModifierState(mModifier);
   getGotoState(mGotoBase, mGotoEvent, mGotoArmy, mGotoScout, mGotoNode, mGotoHero, mGotoRally);

   mGroupIndex=mpUser->getGroupIndex();
   for (long i=0; i<cMaxGroups; i++)
      mGroupAssigned[i]=mpUser->isGroupAssigned(i);

   if (!mCircleSelecting && !mCircleMenuUp && mUserMode==BUser::cUserModeNormal)
   {
      // If you must press and hold a button to create a group, then only use the basic "Group X" text and always display it.
      // Otherwise use the "Create Group X" for creating groups and "Group X" for selecting/going to groups.
      if(isHoldControl(pInputInterface, BInputInterface::cInputAssignGroup1))
      {
         setLableTextForFunction(pInputInterface, BInputInterface::cInputSelectGroup1, 130, true);
         setLableTextForFunction(pInputInterface, BInputInterface::cInputSelectGroup2, 131, true);
         setLableTextForFunction(pInputInterface, BInputInterface::cInputSelectGroup3, 132, true);
         setLableTextForFunction(pInputInterface, BInputInterface::cInputSelectGroup4, 133, true);
      }
      else
      {
         // Group 1
         if (mGroupAssigned[0])
         {
            setLableTextForFunction(pInputInterface, BInputInterface::cInputSelectGroup1, 130, false);
            setLableTextForFunction(pInputInterface, BInputInterface::cInputGotoGroup1, 130, false);
         }
         if (mAnySelected || mGroupAssigned[0])
            setLableTextForFunction(pInputInterface, BInputInterface::cInputAssignGroup1, mAnySelected ? 140 : 150, false);

         // Group 2
         if (mGroupAssigned[1])
         {
            setLableTextForFunction(pInputInterface, BInputInterface::cInputSelectGroup2, 131, false);
            setLableTextForFunction(pInputInterface, BInputInterface::cInputGotoGroup2, 131, false);
         }
         if (mAnySelected || mGroupAssigned[1])
            setLableTextForFunction(pInputInterface, BInputInterface::cInputAssignGroup2, mAnySelected ? 141 : 151, false);

         // Group 3
         if (mGroupAssigned[2])
         {
            setLableTextForFunction(pInputInterface, BInputInterface::cInputSelectGroup3, 132, false);
            setLableTextForFunction(pInputInterface, BInputInterface::cInputGotoGroup3, 132, false);
         }
         if (mAnySelected || mGroupAssigned[2])
            setLableTextForFunction(pInputInterface, BInputInterface::cInputAssignGroup3, mAnySelected ? 142 : 152, false);

         // Group 4
         if (mGroupAssigned[3])
         {
            setLableTextForFunction(pInputInterface, BInputInterface::cInputSelectGroup4, 133, false);
            setLableTextForFunction(pInputInterface, BInputInterface::cInputGotoGroup4, 133, false);
         }
         if (mAnySelected || mGroupAssigned[3])
            setLableTextForFunction(pInputInterface, BInputInterface::cInputAssignGroup4, mAnySelected ? 143 : 153, false);
      }

      if (mNumSubSelectGroups>0)
      {
         if (!setLableTextForFunction(pInputInterface, BInputInterface::cInputSubSelectPrev, 191, false))
            setLableTextForFunction(pInputInterface, BInputInterface::cInputSubSelectNext, 115, false);
         else
         {
            setLableTextForFunction(pInputInterface, BInputInterface::cInputSubSelectNext, 190, false);
            setLableTextForFunction(pInputInterface, BInputInterface::cInputSubSelectType, 213, false);
         }
      }

      long controlType1=-1, controlType2=-1;
      bool modifyFlag1=false, modifyFlag2=false;
      bool doubleClick1=false, doubleClick2=false;
      bool hold1=false, hold2=false;
      pInputInterface->getFunctionControl(BInputInterface::cInputGlobalSelect, controlType1, modifyFlag1, doubleClick1, hold1);
      pInputInterface->getFunctionControl(BInputInterface::cInputScreenSelect, controlType2, modifyFlag2, doubleClick2, hold2);
      if (controlType1==controlType2 && modifyFlag1==modifyFlag2)
         setLableTextForFunction(pInputInterface, BInputInterface::cInputScreenSelect, 126, false);
      else
      {
         setLableTextForFunction(pInputInterface, BInputInterface::cInputGlobalSelect, 124, false);
         setLableTextForFunction(pInputInterface, BInputInterface::cInputScreenSelect, 125, false);
      }

      setLableTextForFunction(pInputInterface, BInputInterface::cInputModeFlare, 113, false);
      setLableTextForFunction(pInputInterface, BInputInterface::cInputSetRally, 169, false);
   }

   if (!mCircleSelecting && (mUserMode==BUser::cUserModeNormal || mUserMode==BUser::cUserModePowerMenu || mUserMode==BUser::cUserModeCommandMenu))
   {
      // Goto Base
      if (mGotoBase)
         setLableTextForFunction(pInputInterface, BInputInterface::cInputGotoBase, 160, false);

      // Goto Event
      if (mGotoEvent)
         setLableTextForFunction(pInputInterface, BInputInterface::cInputGotoAlert, 161, false);

      // Goto Scout
      if (mGotoScout)
         setLableTextForFunction(pInputInterface, BInputInterface::cInputGotoScout, 163, false);

      // Goto Node
      if (mGotoNode)
         setLableTextForFunction(pInputInterface, BInputInterface::cInputGotoNode, 162, false);

      // Goto Hero
      if (mGotoHero)
         setLableTextForFunction(pInputInterface, BInputInterface::cInputGotoHero, 167, false);

      // Goto Rally
      if (mGotoRally)
         setLableTextForFunction(pInputInterface, BInputInterface::cInputGotoRally, 168, false);

      // Goto Army
      if (mGotoArmy)
         setLableTextForFunction(pInputInterface, BInputInterface::cInputGotoArmy, 164, false);

      // Goto Selected
      if (mAnySelected)
         setLableTextForFunction(pInputInterface, BInputInterface::cInputGotoSelected, 165, false);
   }

   // Turn on labels based on the current state info
   if(mCircleMenuUp)
   {
      setLabelText(menuCancelLabel, gDatabase.getLocStringFromID(104));
      setLabelText(menuExtraInfo, gDatabase.getLocStringFromID(214));

      if(mDequeue)
         setLabelText(menuAlternateLabel, gDatabase.getLocStringFromID(105));
      
      if(mCurrentItemType!=-1)
      {
         if(mUserMode==BUser::cUserModeCommandMenu)
         {
            if(mCurrentItemType==BProtoObjectCommand::cTypeTrainUnit || mCurrentItemType==BProtoObjectCommand::cTypeTrainSquad)
               setLabelText(menuOKLabel, gDatabase.getLocStringFromID(110));
            else if(mCurrentItemType==BProtoObjectCommand::cTypeResearch)
            {
               if (!mDequeue)
                  setLabelText(menuOKLabel, gDatabase.getLocStringFromID(111));
            }
            else if(mCurrentItemType==BProtoObjectCommand::cTypeBuild || mCurrentItemType==BProtoObjectCommand::cTypeBuildOther)
               setLabelText(menuOKLabel, gDatabase.getLocStringFromID(112));
            else if(mCurrentItemType==BProtoObjectCommand::cTypeCustomCommand)
            {
//-- FIXING PREFIX BUG ID 3635
               const BCustomCommand* pCustomCommand=gWorld->getCustomCommand(mCurrentItemID);
//--
               if (pCustomCommand && pCustomCommand->mHelpStringIndex!=-1)
                  setLabelText(menuOKLabel, gDatabase.getLocStringFromIndex(pCustomCommand->mHelpStringIndex));
               else
                  setLabelText(menuOKLabel, gDatabase.getLocStringFromID(112));
            }
            else
               setLabelText(menuOKLabel, gDatabase.getLocStringFromID(100));
         }
         else if(mUserMode==BUser::cUserModePowerMenu)
         {
            if (mCurrentItemType==gDatabase.getPPIDRallyPoint())
               setLabelText(menuOKLabel, gDatabase.getLocStringFromID(100));
            else
               setLabelText(menuOKLabel, gDatabase.getLocStringFromID(25530));
         }
      }

      setLabelIconForFunction(pInputInterface, BInputInterface::cInputPowers);
      setLabelText(gamePowersLabel, gDatabase.getLocStringFromIndex(mpUser->getPlayer()->getCiv()->getLeaderMenuNameIndex()));
   }
   else
   {
      if(!mCircleSelecting)
      {
         if(mUserMode!=BUser::cUserModeNormal)
         {
            /* DELETE ME
            if (mUserMode==BUser::cUserModeOptionsMainMenu || mUserMode==BUser::cUserModeOptionsGameMenu || mUserMode==BUser::cUserModeOptionsControlsMenu 
               || mUserMode==BUser::cUserModeOptionsAudioMenu || mUserMode==BUser::cUserModeOptionsGraphicsMenu || mUserMode==BUser::cUserModeOptionsMiscMenu)
            {
               setLabelText(gameCancelLabel, gDatabase.getLocStringFromID(104));
            }
            else
            */
            {
               reticuleLabel = gameConfirmLabel;
               setLabelText(gameConfirmLabel, gDatabase.getLocStringFromID(100));
               setLabelText(gameCancelLabel, gDatabase.getLocStringFromID(101));
            }
         }
         else
         {
            if(mHoverObject!=cInvalidObjectID)
            {
               if(!mHoverSelected)
               {
                  if (selectType == cSelectTypeUnit || selectType == cSelectTypeSingleType || selectType == cSelectTypeSingleUnit || selectType == cSelectTypeCommand)
                  {
                     bool canSelect = true;
                     if (selectType == cSelectTypeCommand && gConfig.isDefined(cConfigBuildingMenuOnSelect))
                     {
                        canSelect = false;
//-- FIXING PREFIX BUG ID 3637
                        const BUnit* pUnit = gWorld->getUnit(mHoverObject);
//--
                        if (!pUnit)
                        {
//-- FIXING PREFIX BUG ID 3636
                           const BDopple* pDopple = gWorld->getDopple(mHoverObject);
//--
                           if (pDopple)
                              pUnit = gWorld->getUnit(pDopple->getParentID());
                        }
                        if (pUnit)
                        {
                           bool isPlayer = (pUnit->getPlayerID() == mpUser->getPlayerID() || pUnit->getPlayerID() == mpUser->getCoopPlayerID());
                           bool isCommandableByAnyPlayer = pUnit->getProtoObject()->getFlagCommandableByAnyPlayer();
                           bool mustOwnToSelect = pUnit->getProtoObject()->getFlagMustOwnToSelect();
                           if (isPlayer || isCommandableByAnyPlayer || !mustOwnToSelect)
                              canSelect = true;
                        }
                     }
                     if (canSelect)
                     {
                        reticuleLabel = gameSelectLabel;
                        setLabelText(gameSelectLabel, gDatabase.getLocStringFromID(102));
                     }
                  }
               }
            }

            setLabelIconForFunction(pInputInterface, BInputInterface::cInputPowers);
            setLabelText(gamePowersLabel, gDatabase.getLocStringFromIndex(mpUser->getPlayer()->getCiv()->getLeaderMenuNameIndex()));

            if(mAnySelected)
            {
               if(mSubSelectMode && gConfig.isDefined(cConfigExitSubSelectOnCancel))
                  setLabelText(gameUnselectLabel, gDatabase.getLocStringFromID(194));
               else
                  setLabelText(gameUnselectLabel, gDatabase.getLocStringFromID(103));

               if(mSelectedSquad!=cInvalidObjectID)
               {
                  switch(mHoverType)
                  {
                     case BUser::cHoverTypeGather  : reticuleLabel = gameActionLabel; setLabelText(gameActionLabel, gDatabase.getLocStringFromID(170)); break;
                     case BUser::cHoverTypeBuild   : reticuleLabel = gameActionLabel; setLabelText(gameActionLabel, gDatabase.getLocStringFromID(171)); break;
                     case BUser::cHoverTypeCapture : reticuleLabel = gameActionLabel; setLabelText(gameActionLabel, gDatabase.getLocStringFromID(187)); break;
                     case BUser::cHoverTypeRepair  : reticuleLabel = gameActionLabel; setLabelText(gameActionLabel, gDatabase.getLocStringFromID(172)); break;
                     case BUser::cHoverTypeGarrison: reticuleLabel = gameActionLabel; setLabelText(gameActionLabel, gDatabase.getLocStringFromID(173)); break;
                     case BUser::cHoverTypeEnemy   : reticuleLabel = gameActionLabel; setLabelText(gameActionLabel, gDatabase.getLocStringFromID(174)); break;
                     default : 
                        if(mCanMove)
                        {
                           if (reticuleLabel == -1 || !pHoverObject)
                           {
                              // Don't show X if pointing at your own selected unit and that's the only unit selected
                              if(!mHoverSelected || mpUser->getSelectionManager()->getNumberSelectedSquads() > 1)
                                 reticuleLabel = gameActionLabel; 
                           }
                           if(mModifier)
                              setLabelText(gameActionLabel, gDatabase.getLocStringFromID(175));
                           else
                              setLabelText(gameActionLabel, gDatabase.getLocStringFromID(176));
                        }
                        break;
                  }
               }
            }
         }
      }
   }

   hideReticuleButtons();
   setReticuleButton(reticuleLabel, BFlashReticle::cReticleKeyframeActive);

   //-- don't allow the Ability (Y-Button to show if the A Button is visible)   
   updateAbilityButton(configIndex);
   updateReticuleButtons();

   setButtonStates();   

   // Turn on icons based on labels that are set
   for(long i=cLabelButtonA; i<=cLabelButtonY; i++)
   {
      if(!mLabels[i].getHidden())
      {
         mIcons[cIconButtonA].setHidden(false);
         mIcons[cIconButtonB].setHidden(false);
         mIcons[cIconButtonX].setHidden(false);
         mIcons[cIconButtonY].setHidden(false);
         break;
      }
   }

   if(!mLabels[cLabelButtonShoulderRight].getHidden())
      mIcons[cIconcButtonShoulderRight].setHidden(false);

   if(!mLabels[cLabelButtonShoulderLeft].getHidden())
      mIcons[cIconcButtonShoulderLeft].setHidden(false);

   for(long i=cLabelDpadUp; i<=cLabelDpad; i++)
   {
      if(!mLabels[i].getHidden())
      {
         mIcons[cIconDpad].setHidden(false);
         break;
      }            
   }   

   if (mpUser && !gUIManager->isNonGameUIVisible() && mpUser->getHUDItemEnabled(BUser::cHUDItemDpadHelp))
   {
      for(long i=cLabelDpadUp; i<=cLabelDpad; i++)
      {
         int controlID = -1;
         switch(i)
         {
            case cLabelDpadUp: controlID = BFlashHUD::eControlDPadUp; break;
            case cLabelDpadDown: controlID = BFlashHUD::eControlDPadDown; break;
            case cLabelDpadLeft: controlID = BFlashHUD::eControlDPadLeft; break;
            case cLabelDpadRight: controlID = BFlashHUD::eControlDPadRight; break;
         };

         if(mLabels[i].getHidden())
         {            
            mpUser->getUIContext()->setDPadButtonIcon(controlID, -1, BFlashHUD::eHUDKeyframeOff);
         }            
      }   
   }

}


//==============================================================================
// BUIHelp::hideReticuleButtons
//==============================================================================
void BUIHelp::hideReticuleButtons()
{   
   if (!mpUser)
      return;

   BUIContext* pUIContext = mpUser->getUIContext();
   if (!pUIContext)
      return;

   pUIContext->setReticleHelp(BFlashReticle::cButtonA, BFlashReticle::cReticleKeyframeOff);
   pUIContext->setReticleHelp(BFlashReticle::cButtonB, BFlashReticle::cReticleKeyframeOff);
   pUIContext->setReticleHelp(BFlashReticle::cButtonX, BFlashReticle::cReticleKeyframeOff);
   pUIContext->setReticleHelp(BFlashReticle::cButtonY, BFlashReticle::cReticleKeyframeOff);
}

//==============================================================================
// BUIHelp::updateReticuleButtons
//==============================================================================
void BUIHelp::updateReticuleButtons()
{
   if (!mpUser)
      return;

   BUIContext* pUIContext = mpUser->getUIContext();
   if (pUIContext)
      pUIContext->updateReticleButtons();
}

//==============================================================================
// BUIHelp::setReticuleButton
//==============================================================================
void BUIHelp::setReticuleButton(long reticuleLabel, long frameID)
{
   if (!mpUser)
      return;

   BUIContext* pUIContext = mpUser->getUIContext();
   if (!pUIContext)
      return;

   int helpButton = -1;
   switch (reticuleLabel)
   {
      case cLabelButtonA:
         helpButton = BFlashReticle::cButtonA;
         break;

      case cLabelButtonB:
         helpButton = BFlashReticle::cButtonB;
         break;

      case cLabelButtonX:
         helpButton = BFlashReticle::cButtonX;
         break;

      case cLabelButtonY:
         helpButton = BFlashReticle::cButtonY;
         break;
   }

   if (helpButton != -1)
      pUIContext->setReticleHelp(helpButton, frameID);   
}

//==============================================================================
// BUIHelp::update
//==============================================================================
void BUIHelp::update( long configIndex )
{   
   // Update goto cache values
   DWORD time=gWorld->getGametime();
   if(time-mGotoItemTime>cGotoItemInterval)
   {
      mGotoBaseCache=mpUser->gotoItem(cGotoTypeBase, true);
      mGotoScoutCache=mpUser->gotoItem(cGotoTypeScout, true);
      mGotoNodeCache=mpUser->gotoItem(cGotoTypeNode, true);
      mGotoHeroCache=mpUser->gotoItem(cGotoTypeHero, true);
      mGotoRallyCache=mpUser->getPlayer()->haveRallyPoint();
      mGotoItemTime=time;
   }

   if(time-mGotoEventTime>cGotoEventInterval)
   {
      mGotoEventCache = (mpUser->getPlayer()->getAlertManager()->getNextGotoAlertID() != cInvalidAlertID);
      mGotoEventTime=time;
   }

   if(time-mGotoArmyTime>cGotoArmyInterval)
   {
      mGotoArmyCache=mpUser->findCrowd(BUser::cFindCrowdMilitary, true, true, NULL);
      mGotoArmyTime=time;
   }

   // Check circle selecting state
   bool circleSelecting=(mpUser->getUserMode()==BUser::cUserModeCircleSelecting);
   if(mCircleSelecting!=circleSelecting)
   {
      refresh( configIndex );
      return;
   }

   if(mCircleSelecting)
      return;

   // Check circle menu state
   bool circleMenuUp, dequeue;
   long currentItemType, currentItemID;
   getCircleMenuState(circleMenuUp, currentItemType, currentItemID, dequeue);

   if(mCircleMenuUp!=circleMenuUp)
   {
      refresh( configIndex );
      return;
   }

   // Check modifier
   bool modifier;
   getModifierState(modifier);
   if(mModifier!=modifier)
   {
      refresh( configIndex );
      return;
   }

   // Check user mode
   if(mUserMode!=mpUser->getUserMode())
   {
      refresh( configIndex );
      return;
   }

   // Check sub mode
   if(mSubMode!=mpUser->getSubMode())
   {
      refresh( configIndex );
      return;
   }

   // Circle menu item
   if(circleMenuUp)
   {
      if(mCurrentItemType!=currentItemType || mCurrentItemID!=currentItemID || mDequeue!=dequeue)
         refresh( configIndex );
      return;
   }

   // Check power state
   bool powerAvailable;
   getPowerState(powerAvailable);
   if(mPowerAvailable!=powerAvailable)
   {
      refresh( configIndex );
      return;
   }

   // Check goto state
   bool gotoBase, gotoEvent, gotoArmy, gotoScout, gotoNode, gotoHero, gotoRally;
   getGotoState(gotoBase, gotoEvent, gotoArmy, gotoScout, gotoNode, gotoHero, gotoRally);
   if(mGotoBase!=gotoBase || mGotoEvent!=gotoEvent || mGotoArmy!=gotoArmy || mGotoScout!=gotoScout || mGotoNode!=gotoNode || mGotoHero!=gotoHero || mGotoRally!=gotoRally)
   {
      refresh( configIndex );
      return;
   }

   // Groups
   if(mGroupIndex!=mpUser->getGroupIndex())
   {
      refresh(configIndex);
      return;
   }
   for(long i=0; i<cMaxGroups; i++)
   {
      if(mGroupAssigned[i]!=mpUser->isGroupAssigned(i))
      {
         refresh(configIndex);
         return;
      }
   }

   // Sub-select
   int subSelectGroupHandle=mpUser->getSelectionManager()->getSubSelectGroupHandle();
   bool subSelectMode=(subSelectGroupHandle!=-1);
   bool subSelectTag=(subSelectGroupHandle==-1 ? false : mpUser->getSelectionManager()->getSubSelectTag(subSelectGroupHandle));
   if (mSubSelectMode!=subSelectMode || mSubSelectTag!=subSelectTag)
   {
      refresh(configIndex);
      return;
   }

   // Check other state info
   if(mHoverObject!=mpUser->getHoverObject() ||
      mHoverSelected!=mpUser->getSelectionManager()->isUnitSelected(mHoverObject) ||
      mHoverType!=mpUser->getHoverType() ||
      mHoverResource!=mpUser->getHoverResource() ||
      mHoverAbilityAvailable!=mpUser->getSelectionManager()->getFlagAbilityAvailable() ||
      mAnySelected!=(mpUser->getSelectionManager()->getNumberSelectedUnits()>0) ||
      mNumSubSelectGroups!=mpUser->getSelectionManager()->getNumSubSelectGroups())
   {
      refresh(configIndex);
      return;
   }

   bool canMove=false;
   int numSquads=mpUser->getSelectionManager()->getNumberSelectedSquads();
   for (int i=0; i<numSquads; i++)
   {
//-- FIXING PREFIX BUG ID 3640
      const BSquad* pSquad=gWorld->getSquad(mpUser->getSelectionManager()->getSelectedSquad(i));
//--
      if (pSquad && pSquad->canMove())
      {
         canMove=true;
         break;
      }
   }
   if (canMove != mCanMove)
   {
      refresh(configIndex);
      return;
   }

   if (mHoverAbilityAvailable)
   {
      updateAbilityButton(configIndex);      
      updateReticuleButtons();
   }
}

//==============================================================================
// BUIHelp::setupElements
//==============================================================================
void BUIHelp::setupElements()
{
   int ox=13;
   int oy=513;
   mIcons[cIconButtonY].setPosition    (272+ox, 24+oy);  mIcons[cIconButtonY].setSize     (24,  24);
   mIcons[cIconButtonB].setPosition    (292+ox, 44+oy);  mIcons[cIconButtonB].setSize     (24,  24);
   mIcons[cIconButtonX].setPosition    (252+ox, 44+oy);  mIcons[cIconButtonX].setSize     (24,  24);
   mIcons[cIconButtonA].setPosition    (272+ox, 64+oy);  mIcons[cIconButtonA].setSize     (24,  24);

   mLabels[cLabelButtonY].setPosition  (230+ox, 0+oy);   mLabels[cLabelButtonY].setSize   (108, 24);
   mLabels[cLabelButtonB].setPosition  (316+ox, 44+oy);  mLabels[cLabelButtonB].setSize   (84,  24);
   mLabels[cLabelButtonX].setPosition  (142+ox, 44+oy);  mLabels[cLabelButtonX].setSize   (108, 24);
   mLabels[cLabelButtonA].setPosition  (230+ox, 88+oy);  mLabels[cLabelButtonA].setSize   (108, 24);

   ox=-680;
   oy=534;
   mIcons[cIconDpad].setPosition       (62+ox,  15+oy);  mIcons[cIconDpad].setSize        (42,  42);
   mLabels[cLabelDpadUp].setPosition   (43+ox,  -9+oy);  mLabels[cLabelDpadUp].setSize    (80,  24);
   mLabels[cLabelDpadRight].setPosition(104+ox, 24+oy);  mLabels[cLabelDpadRight].setSize (80,  24);
   mLabels[cLabelDpadLeft].setPosition (-20+ox, 24+oy);  mLabels[cLabelDpadLeft].setSize  (80,  24);
   mLabels[cLabelDpadDown].setPosition (43+ox,  57+oy);  mLabels[cLabelDpadDown].setSize  (80,  24);
   mLabels[cLabelDpad].setPosition     (62+ox,  15+oy);  mLabels[cLabelDpad].setSize      (42,  42);

   ox=17;
   oy=353;
   mIcons[cIconcButtonShoulderRight].setPosition (240+ox, 122+oy);  mIcons[cIconcButtonShoulderRight].setSize (48,  24);
   mLabels[cLabelButtonShoulderRight].setPosition(286+ox, 122+oy);  mLabels[cLabelButtonShoulderRight].setSize(84,  24);

   ox=-913;
   oy=329;
   mIcons[cIconcButtonShoulderLeft].setPosition  (240+ox, 146+oy);  mIcons[cIconcButtonShoulderLeft].setSize  (48,  24);
   mLabels[cLabelButtonShoulderLeft].setPosition (308+ox, 146+oy);  mLabels[cLabelButtonShoulderLeft].setSize (108, 24);

   mIcons[cIconButtonY].setTexture("ui\\game\\help\\controller_buttons_y", true);
   mIcons[cIconButtonB].setTexture("ui\\game\\help\\controller_buttons_b", true);
   mIcons[cIconButtonX].setTexture("ui\\game\\help\\controller_buttons_x", true);
   mIcons[cIconButtonA].setTexture("ui\\game\\help\\controller_buttons_a", true);
   mIcons[cIconDpad].setTexture("ui\\game\\help\\controller_dpad", true);
   mIcons[cIconcButtonShoulderRight].setTexture("ui\\game\\help\\controller_buttons_shoulder_right", true);
   mIcons[cIconcButtonShoulderLeft].setTexture("ui\\game\\help\\controller_buttons_shoulder_left", true);

   for(long i=0; i<cNumberLabels; i++)
      mLabels[i].setFont(gUIGame.getHelpFont());

   mLabels[cLabelButtonX].setTextJust(BFontManager2::cJustifyRight);
   mLabels[cLabelDpadLeft].setTextJust(BFontManager2::cJustifyRight);

   mLabels[cLabelButtonB].setTextJust(BFontManager2::cJustifyLeft);
   mLabels[cLabelDpadRight].setTextJust(BFontManager2::cJustifyLeft);
   mLabels[cLabelButtonShoulderRight].setTextJust(BFontManager2::cJustifyLeft);
}

//==============================================================================
// BUIHelp::getCircleMenuState
//==============================================================================
void BUIHelp::getCircleMenuState(bool& circleMenuUp, long& currentItemType, long& currentItemID, bool& dequeue)
{   
   circleMenuUp=(mpUser->getUserMode()==BUser::cUserModeCommandMenu || mpUser->getUserMode()==BUser::cUserModePowerMenu);
   currentItemType=-1;
   currentItemID=-1;
   dequeue=false;

   if(circleMenuUp)
   {
      const BUICircleMenu & circleMenu=mpUser->getCircleMenu();
      long itemIndex=circleMenu.getCurrentItemIndex();
      if(itemIndex!=-1 && !circleMenu.getItemUnavailable(itemIndex))
      {
         if(mpUser->getUserMode()==BUser::cUserModeCommandMenu)
         {
            BProtoObjectCommand command;
            command.set(circleMenu.getCurrentItemID());
            long type=command.getType();
            long id=command.getID();
            if(type==BProtoObjectCommand::cTypeTrainUnit || type==BProtoObjectCommand::cTypeTrainSquad)
            {
//-- FIXING PREFIX BUG ID 3641
               const BUnit* pUnit=gWorld->getUnit(mpUser->getCommandObject());
//--
               if(pUnit && pUnit->getTrainCount(mpUser->getPlayerID(), id)>0)
                  dequeue=true;
            }
            else if(type==BProtoObjectCommand::cTypeResearch)
            {
               if(mpUser->getPlayer()->getTechTree()->getTechStatus(id, mpUser->getCommandObject().asLong())==BTechTree::cStatusResearching)
                  dequeue=true;
            }
            else if(type==BProtoObjectCommand::cTypeCustomCommand)
            {
//-- FIXING PREFIX BUG ID 3642
               const BCustomCommand* pCustomCommand = gWorld->getCustomCommand(id);
//--
               if (pCustomCommand && pCustomCommand->mQueuedCount > 0)
                  dequeue=true;
            }
            currentItemType=type;
            currentItemID=id;
         }
         else if(mpUser->getUserMode()==BUser::cUserModePowerMenu)
         {
            currentItemType=circleMenu.getCurrentItemID();
         }
      }
   }
}

//==============================================================================
// BUIHelp::getPowerState
//==============================================================================
void BUIHelp::getPowerState(bool& powerAvailable)
{
   powerAvailable=false;
   BPlayer* pPlayer=gUserManager.getUser(BUserManager::cPrimaryUser)->getPlayer();
   if(pPlayer)
   {
      long powerEntryCount=pPlayer->getNumPowerEntries();
      for(long i=0; i<powerEntryCount && !powerAvailable; i++)
      {
         BPowerEntry* pPowerEntry=pPlayer->getPowerEntryAtIndex(i);
         if(!pPowerEntry)
            continue;
         for(long j=0; j<pPowerEntry->mItems.getNumber(); j++)
         {
//-- FIXING PREFIX BUG ID 3629
            const BPowerEntryItem& item=pPowerEntry->mItems[j];
//--
            if(item.mInfiniteUses || item.mUsesRemaining>0)
            {
               powerAvailable=true;
               break;
            }
         }
      }
   }
}

//==============================================================================
// BUIHelp::getModifierState
//==============================================================================
void BUIHelp::getModifierState(bool& modifier)
{
   modifier=gUserManager.getUser(BUserManager::cPrimaryUser)->getFlagModifierAction();
}

//==============================================================================
// BUIHelp::getGotoState
//==============================================================================
void BUIHelp::getGotoState(bool& gotoBase, bool& gotoEvent, bool& gotoArmy, bool& gotoScout, bool& gotoNode, bool& gotoHero, bool& gotoRally)
{
   gotoBase=mGotoBaseCache;
   gotoEvent=mGotoEventCache;
   gotoArmy=mGotoArmyCache;
   gotoScout=mGotoScoutCache;
   gotoNode=mGotoNodeCache;
   gotoHero=mGotoHeroCache;
   gotoRally=mGotoRallyCache;
}

//==============================================================================
// BUIHelp::setLabelText
//==============================================================================
void BUIHelp::setLabelText(long id, const WCHAR* pText)
{
   if(id==-1)
      return;

   if (gConfig.isDefined(cConfigFlashGameUI))
   {      
      int buttonLabelID = -1;      
      switch (id)
      {
         case cLabelButtonX   : buttonLabelID = BFlashHUD::eControlLabelButtonX;   break;

         //disabled for PHX-14572 -- leaving this here in case we want to reenable
         //case cLabelButtonY   : buttonLabelID = BFlashHUD::eControlLabelButtonY;   break; 

         case cLabelButtonA   : buttonLabelID = BFlashHUD::eControlLabelButtonA;   break;
         case cLabelButtonB   : buttonLabelID = BFlashHUD::eControlLabelButtonB;   break;
         case cLabelDpadLeft  : buttonLabelID = BFlashHUD::eControlLabelDPadLeft;  break;
         case cLabelDpadUp    : buttonLabelID = BFlashHUD::eControlLabelDPadUp;    break;
         case cLabelDpadDown  : buttonLabelID = BFlashHUD::eControlLabelDPadDown;  break;
         case cLabelDpadRight : buttonLabelID = BFlashHUD::eControlLabelDPadRight; break;
         case cLabelDpad      : break; //AJL FIXME 5/13/07 - Need something added to the Flash Hud for this
         case cLabelButtonShoulderRight: buttonLabelID = BFlashHUD::eControlLabelButtonShoulderRight; break;
         case cLabelButtonShoulderLeft: buttonLabelID = BFlashHUD::eControlLabelButtonShoulderLeft; break;
      };

      if (mpUser)
      {
         if (buttonLabelID != -1)
         {
            mpUser->getUIContext()->setHUDLabelTextVisible(buttonLabelID, true);
            mpUser->getUIContext()->setHUDLabelText(buttonLabelID, pText);
         }                
      }
   }

   mLabels[id].setText(pText);
   mLabels[id].setHidden(false);
}

//==============================================================================
// BUIHelp::setButtonStates()
//==============================================================================
void BUIHelp::setButtonStates()
{
   if (!mpUser)
      return;

   BUIContext* pUIContext = mpUser->getUIContext();
   if (!pUIContext)
      return;

   if (pUIContext->isButtonPanelVisible())
   {
      pUIContext->setButtonState(BFlashHUD::eControlButtonA,  mLabels[cLabelButtonA].getHidden() ? BFlashHUD::eHUDKeyframeOff : BFlashHUD::eHUDKeyframeActive);
      pUIContext->setButtonState(BFlashHUD::eControlButtonB,  mLabels[cLabelButtonB].getHidden() ? BFlashHUD::eHUDKeyframeOff : BFlashHUD::eHUDKeyframeActive);
      pUIContext->setButtonState(BFlashHUD::eControlButtonX,  mLabels[cLabelButtonX].getHidden() ? BFlashHUD::eHUDKeyframeOff : BFlashHUD::eHUDKeyframeActive);
      
      //disabled for PHX-14572 -- leaving this here in case we want to reenable
      //pUIContext->setButtonState(BFlashHUD::eControlButtonY,  mLabels[cLabelButtonY].getHidden() ? BFlashHUD::eHUDKeyframeOff : BFlashHUD::eHUDKeyframeActive);

      pUIContext->setButtonState(BFlashHUD::eControlButtonLB, mLabels[cLabelButtonShoulderLeft].getHidden() ? BFlashHUD::eHUDKeyframeOff : BFlashHUD::eHUDKeyframeActive);
      pUIContext->setButtonState(BFlashHUD::eControlButtonRB, mLabels[cLabelButtonShoulderRight].getHidden() ? BFlashHUD::eHUDKeyframeOff : BFlashHUD::eHUDKeyframeActive);   
   }

   if (pUIContext->isDPadPanelVisible())
   {
      //-- BTK Disabled because we took text off the DPad Panel
#if 0
      bool bDPadHidden = (mLabels[cLabelDpadLeft].getHidden()) &&
                         (mLabels[cLabelDpadRight].getHidden()) &&
                         (mLabels[cLabelDpadUp].getHidden()) &&
                         (mLabels[cLabelDpadDown].getHidden()) &&
                         (mLabels[cLabelDpad].getHidden());

      pUIContext->setButtonState(BFlashHUD::eControlDPad, bDPadHidden ? BFlashHUD::eHUDKeyframeOn : BFlashHUD::eHUDKeyframeActive);
#endif
   }
}

//==============================================================================
// BUIHelp::getLabelFromControlType
//==============================================================================
long BUIHelp::getLabelFromControlType( long controlType )
{
   long result = -1;

   switch( controlType )
   {
      case cStickLeft:
         break;

      case cStickRight:
         break;

      case cDpadUp:
         result = cLabelDpadUp;
         break;

      case cDpadDown:
         result = cLabelDpadDown;
         break;

      case cDpadLeft:
         result = cLabelDpadLeft;
         break;

      case cDpadRight:
         result = cLabelDpadRight;
         break;

      case cButtonA:
         result = cLabelButtonA;
         break;

      case cButtonB:
         result = cLabelButtonB;
         break;

      case cButtonX:
         result = cLabelButtonX;
         break;

      case cButtonY:
         result = cLabelButtonY;
         break;

      case cButtonStart:
         break;

      case cButtonBack:
         break;

      case cButtonShoulderRight:
         result = cLabelButtonShoulderRight;
         break;

      case cButtonShoulderLeft:
         result = cLabelButtonShoulderLeft;
         break;

      case cButtonThumbLeft:
         break;

      case cButtonThumbRight:
         break;

      case cTriggerLeft:
         break;

      case cTriggerRight:
         break;
   }

   return( result );
}

//==============================================================================
// BUIHelp::getLabelForFunction
//==============================================================================
long BUIHelp::getLabelForFunction(BInputInterface* pInputInterface, long inputFunc)
{
   long controlType;
   bool modifyFlag, doubleClick, hold;
   pInputInterface->getFunctionControl((BInputInterface::BInputFunctions)inputFunc, controlType, modifyFlag, doubleClick, hold);
   return getLabelFromControlType(controlType);
}

//==============================================================================
// BUIHelp::setLableTextForFunction
//==============================================================================
bool BUIHelp::setLableTextForFunction(BInputInterface* pInputInterface, long inputFunc, long stringID, bool allowHold)
{
   //-- set any dpad icons if necessary.
   setLabelIconForFunction(pInputInterface, inputFunc);

   long controlType=-1;
   bool modifyFlag=false;
   bool doubleClick=false;
   bool hold=false;
   pInputInterface->getFunctionControl((BInputInterface::BInputFunctions)inputFunc, controlType, modifyFlag, doubleClick, hold);
   if (controlType==-1)
      return false;
   int label=getLabelFromControlType(controlType);
   if (label==-1)
      return false;
   if ((mModifier == modifyFlag || !pInputInterface->controlhasModifierFunc(controlType)) && !doubleClick && (allowHold || !hold))
   {
      setLabelText(label, gDatabase.getLocStringFromID(stringID));
      return true;
   }
   return false;
}

//==============================================================================
//==============================================================================
void BUIHelp::setLabelIconForFunction(BInputInterface* pInputInterface, long inputFunc)
{
   if (!mpUser)
      return;

   long controlType = -1;
   bool modifyFlag = false;
   bool doubleClick = false;
   bool hold = false;
   pInputInterface->getFunctionControl((BInputInterface::BInputFunctions)inputFunc, controlType, modifyFlag, doubleClick, hold);

   //-- we only handle the dpad currently.
   if (controlType < cDpadUp || controlType > cDpadRight)
      return;

   int controlID = -1;
   switch(controlType)
   {
      case cDpadUp    : controlID = BFlashHUD::eControlDPadUp; break;
      case cDpadLeft  : controlID = BFlashHUD::eControlDPadLeft; break;
      case cDpadDown  : controlID = BFlashHUD::eControlDPadDown; break;
      case cDpadRight : controlID = BFlashHUD::eControlDPadRight; break;
   };

   if (controlID == -1)
      return;

   int frameID = BFlashHUD::eHUDKeyframeOn;
   int iconID = mpUser->getUIContext()->getInputFunctionIconID(inputFunc);
   if (iconID == -1)
      frameID = BFlashHUD::eHUDKeyframeOff;

   mpUser->getUIContext()->setDPadButtonIcon(controlID, iconID, frameID);
}

//==============================================================================
// BUIHelp::isHoldControl
//==============================================================================
bool BUIHelp::isHoldControl(BInputInterface* pInputInterface, long inputFunc)
{
   long controlType=-1;
   bool modifyFlag=false;
   bool doubleClick=false;
   bool hold=false;
   pInputInterface->getFunctionControl((BInputInterface::BInputFunctions)inputFunc, controlType, modifyFlag, doubleClick, hold);
   return hold;
}

//==============================================================================
// BUIHelp::render
//==============================================================================
void BUIHelp::render(long parentX, long parentY)
{
   if (gConfig.isDefined(cConfigFlashGameUI))
   {
      // AJL FIXME 5/31/07 - Hack to render the DPAD text until it's added to the Flash Hud
      mLabels[cLabelDpad].render(parentX+mX, parentY+mY);
      return;
   }

   BUIElement::render(parentX, parentY);
   long x=parentX+mX;
   long y=parentY+mY;
   for(long i=0; i<cNumberIcons; i++)
      mIcons[i].render(x, y);
   for(long i=0; i<cNumberLabels; i++)
      mLabels[i].render(x, y);
}

//==============================================================================
// BUIHelp::updateAbilityButton
//==============================================================================
void BUIHelp::updateAbilityButton(long configIndex)
{
   // don't bother doing anything with the reticle if we are in the command menu or power menu
   if ((mUserMode == BUser::cUserModeCommandMenu) || (mUserMode == BUser::cUserModePowerMenu))
      return;

   if (!mHoverAbilityAvailable || (mUserMode != BUser::cUserModeNormal))
   {
      setLabelText(mHoverAbilityLabelID, L"");
      setReticuleButton(mHoverAbilityLabelID, BFlashReticle::cReticleKeyframeOff);  
      return;
   }

   // Don't show the Ability Button State if the A Button is visible.  PHX 4564
   bool hideAifHaveY = false;
   
   BDEBUG_ASSERT(mpUser);
   BUIContext* pUIContext = mpUser->getUIContext();
   if (pUIContext)
   {
      const BReticleHelpButtonState* pButtonState = pUIContext->getReticleButtonState(BFlashReticle::cButtonA);
      if (pButtonState && pButtonState->mVisible)
      {
         // ajl 3/24/08 - Go ahead and try adding the Y button if we are pointing at an enemy. That way the warthog
         // will show Y when it can ram but not attack with X.
         bool skipAbilityCheck = true;
         if (mHoverObject != cInvalidObjectID)
         {
//-- FIXING PREFIX BUG ID 3632
            const BObject* pObject = NULL;
//--
            if (mHoverObject.getType() == BEntity::cClassTypeDopple)
            {
//-- FIXING PREFIX BUG ID 3631
               const BDopple* pDopple = gWorld->getDopple(mHoverObject);
//--
               if (pDopple)
                  pObject = gWorld->getObject(pDopple->getParentID());
            }
            else
               pObject = gWorld->getObject(mHoverObject);
            if (pObject)
            {
               skipAbilityCheck = false;
               hideAifHaveY = true;
            }
         }
         if (skipAbilityCheck)
         {
            setLabelText(mHoverAbilityLabelID, L"");
            setReticuleButton(mHoverAbilityLabelID, BFlashReticle::cReticleKeyframeOff);  
            return;
         }
      }
   }

   long abilityFrameID = BFlashReticle::cReticleKeyframeOff;
   
   if (mpUser->getSelectionManager()->getSubSelectAbilityValid())
   {
      // [8/27/2008 JRuediger] Special Stasis hack:  If you are hovering over a non flying object and your Y ability has the Hide Y Special (currently stasis only), then don't
      // show the Y icon on the reticle.
      if(mpUser->getSelectionManager()->getSubSelectAbility().mHideYSpecial)
      {
         if (mHoverObject != cInvalidObjectID)
         {
            BObject* pObject = NULL;
            if (mHoverObject.getType() == BEntity::cClassTypeDopple)
            {
               BDopple* pDopple = gWorld->getDopple(mHoverObject);
               if (pDopple)
                  pObject = gWorld->getObject(pDopple->getParentID());
            }
            else
               pObject = gWorld->getObject(mHoverObject);

            
            if (pObject && pObject->getFlagFlying() && pObject->getPlayer()->isEnemy(mpUser->getPlayer()))
            {
               abilityFrameID = BFlashReticle::cReticleKeyframeActive;
            }
         }
      }
      else
      {
         if (mpUser->getSelectionManager()->getSubSelectAbilityRecovering())
            abilityFrameID = BFlashReticle::cReticleKeyframePending;
         else
            abilityFrameID = BFlashReticle::cReticleKeyframeActive;
      
      }
   }

   setLabelText(mHoverAbilityLabelID, gDatabase.getLocStringFromID(122));

   setReticuleButton(mHoverAbilityLabelID, abilityFrameID);  

   if (hideAifHaveY && abilityFrameID != BFlashReticle::cReticleKeyframeOff)
      pUIContext->setReticleHelp(BFlashReticle::cButtonA, BFlashReticle::cReticleKeyframeOff);
}
