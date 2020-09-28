//============================================================================
// UITalkingHeadControl.cpp
// Ensemble Studios (c) 2008
//============================================================================
#include "common.h"
#include "UITalkingHeadControl.h"
#include "configsgame.h"
#include "soundmanager.h"
#include "world.h"
#include "objectivemanager.h"
#include "binkInterface.h"
#include "uimanager.h"
#include "cinematicManager.h"
#include "user.h"

GFIMPLEMENTVERSION(BUITalkingHeadControl, 1);

//============================================================================
//============================================================================
BUITalkingHeadControl::BUITalkingHeadControl( void ) : 
   mVideoHandle(cInvalidVideoHandle),
   mObjectiveVisible(false),
   mTalkingHeadVisible(false),
   mShowBackground(false),
   mObjectiveID(-1),
   mLastCount(-1)
{
   mControlType.set("UITalkingHeadControl");
}

//============================================================================
//============================================================================
BUITalkingHeadControl::~BUITalkingHeadControl( void )
{
}

//============================================================================
//============================================================================
bool BUITalkingHeadControl::executeInputEvent( long port, long event, long controlType, BInputEventDetail& detail, const BSimString& command, const BInputControl* pInputControl )
{
   fireUIControlEvent(command.getPtr());
   return true;
}

//============================================================================
//============================================================================
void BUITalkingHeadControl::showTalkingHead(BBinkVideoHandle videoHandle)
{
   mVideoHandle = videoHandle;

   if( !isShown() )
      gBinkInterface.setVisibleVideo( mVideoHandle, false );

   GFxValue value;
   value.SetBoolean(mShowBackground);

   invokeActionScript( "onShowTalkingHead", &value, 1);

   if( !gUIManager->isNonGameUIVisible() || gUIManager->getCurrentUser()->getUserMode() == BUser::cUserModeCinematic )
      show();
}

//============================================================================
//============================================================================
void BUITalkingHeadControl::hideTalkingHead()
{
   mVideoHandle = cInvalidVideoHandle;
   invokeActionScript("onHideTalkingHead");
}

//============================================================================
//============================================================================
void BUITalkingHeadControl::showObjectiveText(long objectiveID)
{
   BObjectiveManager* pObjManager = gWorld->getObjectiveManager();
   if (!pObjManager)
      return;

   BObjective* pObjective = pObjManager->getObjective( pObjManager->getIndexFromID(objectiveID));
   if (!pObjective)
      return;

   mLastCount = pObjective->getCurrentCount();
   mObjectiveID = objectiveID;
   GFxValue value;
   value.SetStringW(pObjective->getDescription().getPtr());

   // This is a hacky - hardcoded thing.
   gSoundManager.playCue("ui_objective_changed");

   invokeActionScript( "onShowObjectiveText", &value, 1);
   mObjectiveVisible=true;
   //show();
}

//============================================================================
//============================================================================
void BUITalkingHeadControl::show( bool force /*= false*/ )
{
   if( (!isShown() || force) && (mVideoHandle != cInvalidVideoHandle) )
      gBinkInterface.setVisibleVideo(mVideoHandle, true);

   __super::show(force);
}

//============================================================================
//============================================================================
void BUITalkingHeadControl::hide( bool force /*= false*/ )
{
   if( (isShown() || force) && (mVideoHandle != cInvalidVideoHandle) )
      gBinkInterface.setVisibleVideo(mVideoHandle, false);

   __super::hide(force);
}

//============================================================================
//============================================================================
void BUITalkingHeadControl::update( void )
{
   bool bBarsVisible = gWorld->getCinematicManager()->areBarsVisible();
   if( bBarsVisible != mShowBackground )
      setShowBackground( bBarsVisible );

   if( mObjectiveID == -1 )
      return;

   BObjectiveManager* pObjManager = gWorld->getObjectiveManager();
   if (!pObjManager)
      return;

   BObjective* pObjective = pObjManager->getObjective( pObjManager->getIndexFromID(mObjectiveID));
   if (!pObjective)
      return;

   if( pObjective->getCurrentCount() != mLastCount )
   {
      mLastCount = pObjective->getCurrentCount();
      GFxValue value;
      value.SetStringW(pObjective->getDescription().getPtr());
      invokeActionScript( "onUpdateObjectiveText", &value, 1 );
   }
}

//============================================================================
//============================================================================
void BUITalkingHeadControl::setShowBackground(bool bShowBackground)
{
   if( bShowBackground != mShowBackground )
   {
      mShowBackground = bShowBackground;
      GFxValue arg;
      arg.SetBoolean(bShowBackground);
      invokeActionScript( "showBlackBackground", &arg, 1 );
   }
}

//============================================================================
//============================================================================
void BUITalkingHeadControl::playTextShowSound()
{
   // part 2 of the hacky sound hookup.

   // play the sound cue here.(or put it on a list for calling on the next update)
   gSoundManager.playCue("ui_objective_changed_part_2");
}


//============================================================================
//============================================================================
void BUITalkingHeadControl::hideObjectiveText(bool easeOut /* = true*/ )
{
   if (!mObjectiveVisible)
      return;

   mObjectiveID = -1;
   GFxValue arg;
   arg.SetBoolean( easeOut );
   invokeActionScript("onHideObjectiveText", &arg, 1 );
   mObjectiveVisible=false;
}

//============================================================================
//============================================================================
void BUITalkingHeadControl::showTalkingHeadText(const BUString& talkingHeadText)
{
   GFxValue values[2];
   values[0].SetStringW(talkingHeadText.getPtr());
   values[1].SetBoolean(mShowBackground);

   invokeActionScript( "onShowTalkingHeadText", values, 2);

   mTalkingHeadText=talkingHeadText;
   mTalkingHeadVisible=true;

   if( !gUIManager->isNonGameUIVisible() || gUIManager->getCurrentUser()->getUserMode() == BUser::cUserModeCinematic )
      show();
}

//============================================================================
//============================================================================
void BUITalkingHeadControl::hideTalkingHeadText()
{
   if (!mTalkingHeadVisible)
      return;

   invokeActionScript("onHideTalkingHeadText");
   mTalkingHeadVisible=false;
}

//============================================================================
//============================================================================
bool BUITalkingHeadScene::init( const char* filename, const char* datafile ) 
{ 
   return __super::init( filename, datafile ); 
}

//============================================================================
//============================================================================
bool BUITalkingHeadScene::init( BXMLNode dataNode )
{
   bool bResult = __super::init( dataNode );

   if( bResult )
      bResult = mTalkingHeadControl.init( this, "mTalkingHeadWidget" );

   return bResult;
}

//============================================================================
//============================================================================
bool BUITalkingHeadControl::save(BStream* pStream, int saveType) const
{
   GFWRITESTRING(pStream, BUString, mTalkingHeadText, 1000);
   //BBinkVideoHandle mVideoHandle;
   GFWRITEVAR(pStream, long, mObjectiveID);
   GFWRITEVAR(pStream, long, mLastCount);
   GFWRITEVAR(pStream, bool, mShowBackground);
   GFWRITEVAR(pStream, bool, mObjectiveVisible);
   GFWRITEVAR(pStream, bool, mTalkingHeadVisible);
   GFWRITEVAL(pStream, bool, isShown());
   return true;
}

//============================================================================
//============================================================================
bool BUITalkingHeadControl::load(BStream* pStream, int saveType)
{
   GFREADSTRING(pStream, BUString, mTalkingHeadText, 1000);
   //BBinkVideoHandle mVideoHandle;
   GFREADVAR(pStream, long, mObjectiveID);
   GFREADVAR(pStream, long, mLastCount);
   GFREADVAR(pStream, bool, mShowBackground);
   GFREADVAR(pStream, bool, mObjectiveVisible);
   GFREADVAR(pStream, bool, mTalkingHeadVisible);

   bool shown;
   GFREADVAR(pStream, bool, shown);
   if (shown)
      show();

   if (mShowBackground)
   {
      mShowBackground = false;
      setShowBackground(true);
   }

   if (mObjectiveVisible)
   {
      mObjectiveVisible = false;
      showObjectiveText(mObjectiveID);
   }

   if (mTalkingHeadVisible)
   {
      mTalkingHeadVisible = false;
      showTalkingHeadText(mTalkingHeadText);
   }

   return true;
}


//============================================================================
//============================================================================
void BUITalkingHeadControl::renderVideo( void )
{
   if( mVideoHandle != cInvalidVideoHandle )
   {
      gBinkInterface.decompressVideo( mVideoHandle );

      if( isShown() )
         gBinkInterface.renderVideo( mVideoHandle );

      gBinkInterface.advanceVideo( mVideoHandle );
   }
}