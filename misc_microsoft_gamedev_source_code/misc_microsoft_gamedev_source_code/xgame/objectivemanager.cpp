//==============================================================================
// objectivemanager.h
//
// objectivemanager manages all objectives
//
// Copyright (c) 2006 Ensemble Studios
//==============================================================================

// Includes
#include "common.h"
#include "objectivemanager.h"
#include "player.h"
#include "world.h"
#include "game.h"
#include "ui.h"
#include "usermanager.h"
#include "user.h"
#include "visiblemap.h"
#include "scoremanager.h"

GFIMPLEMENTVERSION(BObjectiveManager, 1);
enum
{
   cSaveMarkerObjective=10000,
   cSaveMarkerMessage,
   cSaveMarkerArrow,
};

//==============================================================================
// Constants
//==============================================================================
const uint cNumObjectives = 10;

//==============================================================================
// BObjectiveArrow::BObjectiveArrow
//==============================================================================
BObjectiveArrow::BObjectiveArrow() :
   mPlayerID(cInvalidPlayerID),
   mOrigin(cOriginVector),
   mTarget(cOriginVector),
   mOffset(0.0f),
   mFlagVisible(false),
   mFlagUseTarget(false),
   mFlagForceTargetVisible(false),
   mFlagTargetDirty(false),
   mObjectID(cInvalidObjectID),
   mLocationObjectID(cInvalidObjectID)
{
}

//==============================================================================
// BObjectiveArrow::BObjectiveArrow
//==============================================================================
BObjectiveArrow::BObjectiveArrow(BPlayerID playerID, BVector origin, BVector target, bool visible, bool useTarget, bool forceTargetVisible) :
   mPlayerID(playerID),
   mOrigin(origin),
   mTarget(target),
   mOffset(0.0f),
   mFlagVisible(visible),
   mFlagUseTarget(useTarget),
   mFlagForceTargetVisible(forceTargetVisible),
   mFlagTargetDirty(true),
   mObjectID(cInvalidObjectID),
   mLocationObjectID(cInvalidObjectID)
   //mGroundFXObjectID(cInvalidObjectID)
{
   // Setup objective arrow
   float offsetRadius = gDatabase.getObjectiveArrowRadialOffset();
   mOrigin.y = 0.0f;
   mTarget.y = 0.0f;
   BVector forward = mTarget - mOrigin;
   forward.normalize();
   BVector offset = forward * offsetRadius;
   BVector position = mOrigin + offset;

   BObjectCreateParms parms;
   parms.mPlayerID = mPlayerID;
   parms.mProtoObjectID = gDatabase.getPOIDObjectiveArrow();
   parms.mPosition = position;
   parms.mForward = forward;
   parms.mRight.assignCrossProduct(cYAxisVector, parms.mForward);
   parms.mRight.normalize();

   BObject* pObject = gWorld->createObject(parms);
   BASSERT(pObject);
   mObjectID = pObject->getID();
   position = pObject->getPosition();   
   gTerrainSimRep.getCameraHeightRaycast(position, position.y, true);
   position.y += pObject->getObstructionRadiusY() + gDatabase.getObjectiveArrowYOffset();
   pObject->setPosition(position);
   pObject->updateObstruction();
   pObject->setFlagNoRenderDuringCinematic(true);
   pObject->setFlagNearLayer(true);
   pObject->setFlagDontInterpolate(true);   

   // Setup location arrow
   BVector locForward = mOrigin - mTarget;
   locForward.normalize();
   BVector locPosition = mTarget;

   BObjectCreateParms locParms;
   locParms.mPlayerID = 0;
   locParms.mProtoObjectID = gDatabase.getPOIDObjectiveLocArrow();
   locParms.mPosition = locPosition;
   locParms.mForward = locForward;
   locParms.mRight.assignCrossProduct(cYAxisVector, locParms.mForward);
   locParms.mRight.normalize();

   BObject* pLocObject = gWorld->createObject(locParms);
   BASSERT(pLocObject);
   mLocationObjectID = pLocObject->getID();
   locPosition = pLocObject->getPosition();   
   gTerrainSimRep.getCameraHeightRaycast(locPosition, locPosition.y, true);
   locPosition.y += pLocObject->getObstructionRadiusZ() + gDatabase.getObjectiveArrowYOffset();
   pLocObject->setPosition(locPosition);
   // Halwes - 7/22/2008 - Temporary until new art is in?
   //pLocObject->pitch(DEGREES_TO_RADIANS(90.0f));
   pLocObject->updateObstruction();
   pLocObject->setFlagNoRenderDuringCinematic(true);   
   pLocObject->setFlagDopples(true);
   pLocObject->setFlagNearLayer(true);
   pLocObject->setFlagGrayMapDopples(mFlagForceTargetVisible);

   // Setup ground fx
   //BObjectCreateParms fxParms;
   //fxParms.mPlayerID = 0;   
   //fxParms.mProtoObjectID = gDatabase.getPOIDObjectiveGroundFX();
   //fxParms.mPosition = mTarget;
   //BObject* pFXObject = gWorld->createObject(fxParms);
   //BASSERT(pFXObject);   
   //mGroundFXObjectID = pFXObject->getID();   
   //pFXObject->setFlagNoRenderDuringCinematic(true);   
   //pFXObject->setFlagDopples(true);
}

//=======================================================
// Update the objective arrow's position and orientation
//=======================================================
void BObjectiveArrow::update(BUser* pUser)
{   
   BASSERT(pUser);
   BObject* pObject = gWorld->getObject(mObjectID);
   BASSERT(pObject);   
   BObject* pLocObject = gWorld->getObject(mLocationObjectID);
   BASSERT(pLocObject);
   //BObject* pFXObject = gWorld->getObject(mGroundFXObjectID);
   //BASSERT(pFXObject);
//-- FIXING PREFIX BUG ID 2436
   const BCamera* pCamera = pUser->getCamera();
//--
   BASSERT(pCamera);
   if (!pUser || !pObject || !pCamera || !pLocObject /*|| !pFXObject*/)
   {
      return;
   }

   // See if the target is visible
   gTerrainSimRep.getHeightRaycast(mTarget, mTarget.y, true);
   bool targetVisible = pUser->isSphereVisible(mTarget, 0.1f);

   // See if the target is close enough to stop drawing the objective arrow
   bool hit = gTerrainSimRep.rayIntersectsCamera(pCamera->getCameraLoc(), pCamera->getCameraDir(), mOrigin);
   if (!hit)
   {
      mOrigin = pUser->getHoverPoint();
   }   
   mOrigin.y = 0.0f;
   mTarget.y = 0.0f;
   float offsetRadius = gDatabase.getObjectiveArrowRadialOffset();
   float distSq = mOrigin.distanceSqr(mTarget);
   float switchRadiusSq = gDatabase.getObjectiveArrowSwitchOffset();
   switchRadiusSq *= switchRadiusSq;   
   targetVisible = (targetVisible && (distSq < switchRadiusSq));
   bool drawArrow = (mFlagVisible && (!targetVisible || (!mFlagForceTargetVisible && !pLocObject->isVisibleOrDoppled(pUser->getTeamID()))));
   pObject->setFlagOccluded(!drawArrow);         
  
   // If the objective arrow is visible and the camera moved since last frame
   bool camMoved = pCamera->getMoved();
   if (drawArrow && (camMoved || mFlagTargetDirty))
   {      
      pObject->setFlagNoUpdate(false);
      BVector forward = mTarget - mOrigin;
      forward.normalize();      
      float zoomFactor = (pUser->getCameraZoom() - pUser->getCameraZoomMin())/(pUser->getCameraZoomMax() - pUser->getCameraZoomMin());
      BVector offset = forward * (offsetRadius * (0.5f + 0.5f*zoomFactor));
      BVector position = mOrigin + offset;
      BVector right = cInvalidVector;
      right.assignCrossProduct(cYAxisVector, forward);
      right.normalize();
      
      gTerrainSimRep.getCameraHeightRaycast(position, position.y, true);
      position.y += pObject->getObstructionRadiusY() + gDatabase.getObjectiveArrowYOffset();
      
      pObject->setPosition(position);
      pObject->setForward(forward);
      pObject->setRight(right);
      pObject->calcUp();
      pObject->updateObstruction();
   }
   else
   {
      pObject->setFlagNoUpdate(true);
   }   

   // Update location object based on their use flag
   if (camMoved || mFlagTargetDirty)      
   {
      pLocObject->setFlagNoUpdate(false);
      //pFXObject->setFlagNoUpdate(false);

      if (mFlagUseTarget)
      {
         // Set visibility
         if (targetVisible || mFlagForceTargetVisible)
         {
            pLocObject->setFlagOccluded(false);
            //pFXObject->setFlagOccluded(false); 
         }
         else
         {
            pLocObject->setFlagOccluded(true);
            //pFXObject->setFlagOccluded(true); 
         }

         // Dirt target so update it
         if (mFlagTargetDirty)
         {
            // Disable dopples
            if (!mFlagForceTargetVisible)
               pLocObject->setFlagDopples(false);
            //pFXObject->setFlagDopples(false);

            // Setup location arrow
            BVector locForward = mOrigin - mTarget;
            locForward.normalize();
            BVector locPosition = mTarget;
            BVector locRight;
            locRight.assignCrossProduct(cYAxisVector, locForward);
            locRight.normalize();

            // Adjust y
            gTerrainSimRep.getCameraHeightRaycast(locPosition, locPosition.y, true);
            //BVector groundFXLoc = locPosition;
            locPosition.y += + gDatabase.getObjectiveArrowYOffset();
            //groundFXLoc.y = locPosition.y;
            locPosition.y += pLocObject->getObstructionRadiusZ();
            pLocObject->setPosition(locPosition);
            pLocObject->setForward(locForward);
            pLocObject->setRight(locRight);
            pLocObject->calcUp();
            // Halwes - 7/22/2008 - Temporary until new art is in?
            //pLocObject->pitch(DEGREES_TO_RADIANS(90.0f));
            pLocObject->updateObstruction();            

            // Setup ground fx
            //pFXObject->setPosition(groundFXLoc);

            mFlagTargetDirty = false;
         }
         else if (targetVisible || mFlagForceTargetVisible)
         {
            // Enable dopples
            pLocObject->setFlagDopples(true);
            //pFXObject->setFlagDopples(true);
         }
      }
      else
      {
         pLocObject->setFlagOccluded(true);
         //pFXObject->setFlagOccluded(true); 
      }
   }
   else
   {
      pLocObject->setFlagNoUpdate(true);
      //pFXObject->setFlagNoUpdate(true);
   }
}

//=======================================================
// Change the objective arrow's owner
//=======================================================
void BObjectiveArrow::changeOwner(BPlayerID playerID)
{
   BObject* pObject = gWorld->getObject(mObjectID);
   BASSERT(pObject);   

   mPlayerID = playerID;
   if (pObject)
   {
      pObject->changeOwner(mPlayerID);
   }
}

//==============================================================================
// BObjectiveMessage::BObjectiveMessage
//==============================================================================
BObjectiveMessage::BObjectiveMessage(BObjectiveID objectiveID, DWORD state, float duration) :
   mObjectiveID(objectiveID),
   mObjectiveMessageState(state),
   mTimeToDisplay(duration),
   mIsNew(true),
   mbNeverExpire(false)
{
	if (duration == 0.0f)
		mbNeverExpire = true;
}

//==============================================================================
// BObjectiveMessage::BObjectiveMessage
//==============================================================================
BObjectiveMessage::BObjectiveMessage() :
   mObjectiveID(-1),
   mObjectiveMessageState(0),
   mTimeToDisplay(0.0f),
   mIsNew(true),
   mbNeverExpire(true)
{
}

//==============================================================================
// BObjectiveMessage::BObjectiveMessage
//==============================================================================
void BObjectiveMessage::updateTime(float elapsedTime)
{
   mTimeToDisplay -= elapsedTime;
}


//==============================================================================
// BObjective::BObjective
//==============================================================================
BObjective::BObjective() :
   mID( -1 ),
   mFlags( 0 ),
   mTimeCompleted( 0 ),
   mScore( 0 ),
   mCurrentCount(-1),
   mFinalCount(-1),
   mTrackerDuration(8000),
   mMinTrackerIncrement(1)
{
   mDescription.format( L"" );
   mHint.format( L"" );

   #if defined( BUILD_DEBUG )
      mName.format( L"" );
   #endif
}

//==============================================================================
// BObjective::BObjective operator =
//
// Assignment operator
//==============================================================================
BObjective& BObjective::operator = ( const BObjective& cSrcObj )
{
   mID          = ( (BObjective)cSrcObj ).getID();
   mFlags       = ( (BObjective)cSrcObj ).getFlags();
   mDescription = ( (BObjective)cSrcObj ).getDescription();
   mTrackerText = ( (BObjective)cSrcObj ).getTrackerText();
   mTrackerDuration = ( (BObjective)cSrcObj ).getTrackerDuration();
   mMinTrackerIncrement = ( (BObjective)cSrcObj ).getMinTrackerIncrement();
   mHint        = ( (BObjective)cSrcObj ).getHint();
   mScore       = ( (BObjective)cSrcObj ).getScore();

   #if defined( BUILD_DEBUG )
      mName = *( (BObjective)cSrcObj ).getName();
   #endif

   return( *this );
}

//==============================================================================
// BObjective::isPlayer()
//==============================================================================
bool BObjective::isPlayer(BPlayerID playerID)
{
   BObjective::EObjectiveFlags flag = (BObjective::EObjectiveFlags)0;

   switch(playerID)
   {
      case 1:
         flag = BObjective::cObjectivePlayer1;
         break;
      case 2:
         flag = BObjective::cObjectivePlayer2;
         break;
      case 3:
         flag = BObjective::cObjectivePlayer3;
         break;
      case 4:
         flag = BObjective::cObjectivePlayer4;
         break;
      case 5:
         flag = BObjective::cObjectivePlayer5;
         break;
      case 6:
         flag = BObjective::cObjectivePlayer6;
         break;
   }
   return( hasFlag(flag) );
}

//==============================================================================
// BObjective::::getDescription()
//==============================================================================
const BUString& BObjective::getDescription( void )
{ 
   // if we should be using the counter
   if (mFinalCount >= 0)
   {
      // make sure current is valid
      if (mCurrentCount < 0)
         mCurrentCount = 0;

      if (mDescription.length() > 0)
      {
         mDescriptionWithCount.locFormat(L"%s (%d/%d)", mDescription.getPtr(), mCurrentCount, mFinalCount);      
         return mDescriptionWithCount;
      }
   }

   return( mDescription ); 
}

//==============================================================================
// BObjective::::getTrackerText()
//==============================================================================
const BUString& BObjective::getTrackerText( void )
{
   return mTrackerText;
}

//==============================================================================
// BObjective::::setCurrentCount()
//==============================================================================
void BObjective::setCurrentCount( int count )
{ 
   mCurrentCount = count;

   if (mCurrentCount >= 0 && mFinalCount >= 0 && mDescription.length() > 0)
      mDescriptionWithCount.format(L"%s (%d/%d)", mDescription.getPtr(), mCurrentCount, mFinalCount);
   else
      mDescriptionWithCount.empty();
}

//==============================================================================
// BObjective::::setCompleted()
//==============================================================================
void BObjective::setCompleted( bool completed )
{ 
   mFlags = completed ? ( mFlags | cObjectiveCompleted ) : ( mFlags & ~cObjectiveCompleted );
   if (completed)
      mTimeCompleted = gWorld->getGametime();
   else
      mTimeCompleted = 0;
}

//==============================================================================
// BObjectiveManager::BObjectiveManager()
//==============================================================================
BObjectiveManager::BObjectiveManager() :  
   mNextID( 1 )
{
}

//==============================================================================
// BObjectiveManager::~BObjectiveManager()
//==============================================================================
BObjectiveManager::~BObjectiveManager( void )
{
}

//==============================================================================
// BObjectiveManager::init
//==============================================================================
bool BObjectiveManager::init( void )
{
   mObjectiveMessageList.setNumber(0);

   bool success = mObjectiveList.init( cNumObjectives );
   if( !success )
   {
      BASSERT(success);
      return( false );
   }

   mNextID = 1;

   return( true );
}

//==============================================================================
// BObjectiveManager::reset
//==============================================================================
void BObjectiveManager::reset( void )
{
   mObjectiveList.clear();
   uint numObjectiveMessages = mObjectiveMessageList.getSize();
   for (uint i = 0; i < numObjectiveMessages; i++)
   {
      BObjectiveMessage* pMessage = mObjectiveMessageList[i];
      if (!pMessage)
         continue;

      delete pMessage;
      mObjectiveMessageList[i] = NULL;
   }
   mObjectiveMessageList.clear();

   mNextID = 1;
}

//==============================================================================
// BObjectiveManager::addObjective
//==============================================================================
BObjectiveID BObjectiveManager::addObjective( void )
{
   // add a new default entry
   uint index = 0;
   mObjectiveList.acquire( index );   
   if( index < 0 )
   {
      return( -1 );
   }

   // assign the next ID
   mObjectiveList.get( index )->setID( mNextID );

   // return and increment the ID
   return( mNextID++ );
}

//==============================================================================
// BObjectiveManager::deleteObjectiveByIndex
//==============================================================================
bool BObjectiveManager::deleteObjectiveByIndex( int index )
{
   if( !testIndex( index ) )
   {
      return( false );
   }

   mObjectiveList.release( index );
   return( true );
}

//==============================================================================
// BObjectiveManager::deleteObjectiveByID
//==============================================================================
bool BObjectiveManager::deleteObjectiveByID( BObjectiveID ID )
{
   return( deleteObjectiveByIndex( getIndexFromID( ID ) ) );
}

//==============================================================================
// BObjectiveManager::getNumberObjectives
//==============================================================================
int BObjectiveManager::getNumberObjectives( void ) const
{      
   return( mObjectiveList.getNumberAllocated() );
}

//==============================================================================
// BObjectiveManager::getNumberRequiredObjectives
//==============================================================================
uint BObjectiveManager::getNumberRequiredObjectives( void )
{
   uint count         = 0;
   uint numObjectives = getNumberObjectives();

   for( uint i = 0; i < numObjectives; i++ )
   {
      BObjective* pObjective = mObjectiveList.get( i );
      if( pObjective && pObjective->isRequired() )
      {
         count++;
      }
   }

   return( count );
}

//==============================================================================
// BObjectiveManager::getObjective
//==============================================================================
BObjective* BObjectiveManager::getObjective(int index)
{
   if( !testIndex( index ) )
   {
      return NULL;
   }

   return( mObjectiveList.get(index) );
}


//==============================================================================
// BObjectiveManager::getObjectiveID
//==============================================================================
BObjectiveID BObjectiveManager::getObjectiveID( int index )
{
   if( !testIndex( index ) )
   {
      return( -1 );
   }

   return( mObjectiveList.get( index )->getID() );
}

//==============================================================================
// BObjectiveManager::setObjectiveID
//==============================================================================
void BObjectiveManager::setObjectiveID( int index, BObjectiveID ID )
{
   if( !testIndex( index ) )
   {
      BASSERTM( false, "Setting objective ID with invalid index!" );
      return;
   }

   mObjectiveList.get( index )->setID( ID );
}

////==============================================================================
//// BObjectiveManager::getObjectiveDescription
////==============================================================================
//const BUString& BObjectiveManager::getObjectiveDescription( int index )
//{
//   if( !testIndex( index ) )
//   {
//      return gDatabase.getLocStringFromIndex(-1);
//   }
//
//   return( mObjectiveList.get( index )->getDescription() );
//}

//==============================================================================
// BObjectiveManager::setObjectiveDescription
//==============================================================================
void BObjectiveManager::setObjectiveDescription( int index, const BUString& description )
{
   if( !testIndex( index ) )
   {
      BASSERTM( false, "Setting objective description with invalid index!" );
      return;
   }

   mObjectiveList.get( index )->setDescription( description );
}

//==============================================================================
// BObjectiveManager::setObjectiveTrackerText
//==============================================================================
void BObjectiveManager::setObjectiveTrackerText( int index, const BUString& trackerText )
{
   if( !testIndex( index ) )
   {
      BASSERTM( false, "Setting objective tracker text with invalid index!" );
      return;
   }

   mObjectiveList.get( index )->setTrackerText( trackerText );
}

//==============================================================================
// BObjectiveManager::setObjectiveTrackerDuration
//==============================================================================
void BObjectiveManager::setObjectiveTrackerDuration( int index, uint duration )
{
   if( !testIndex( index ) )
   {
      BASSERTM( false, "Setting objective tracker duration with invalid index!" );
      return;
   }

   mObjectiveList.get( index )->setTrackerDuration( duration );
}

//==============================================================================
// BObjectiveManager::setObjectiveMinTrackerIncrement
//==============================================================================
void BObjectiveManager::setObjectiveMinTrackerIncrement( int index, uint increment )
{
   if( !testIndex( index ) )
   {
      BASSERTM( false, "Setting objective min tracker increment with invalid index!" );
      return;
   }
   
   mObjectiveList.get(index)->setMinTrackerIncrement( increment );
}

////==============================================================================
//// BObjectiveManager::getObjectiveHint
////==============================================================================
//const BUString& BObjectiveManager::getObjectiveHint( int index )
//{
//   if( !testIndex( index ) )
//   {
//      return gDatabase.getLocStringFromIndex(-1);
//   }
//
//   return( mObjectiveList.get( index )->getHint() );
//}

//==============================================================================
// BObjectiveManager::setObjectiveHint
//==============================================================================
void BObjectiveManager::setObjectiveHint( int index, const BUString& hint )
{
   if( !testIndex( index ) )
   {
      BASSERTM( false, "Setting objective hint with invalid index!" );
      return;
   }

   mObjectiveList.get( index )->setHint( hint );
}

//==============================================================================
// BObjectiveManager::getObjectiveRequired
//==============================================================================
bool BObjectiveManager::getObjectiveRequired( int index )
{
   if( !testIndex( index ) )
   {
      BASSERTM( false, "Invalid objective index!" );
      return( false );
   }

   return( mObjectiveList.get( index )->isRequired() );
}

//==============================================================================
// BObjectiveManager::setObjectiveRequired
//==============================================================================
void BObjectiveManager::setObjectiveRequired( int index, bool required )
{
   if( !testIndex( index ) )
   {
      BASSERTM( false, "Setting objective required flag with invalid index!" );
      return;
   }

   mObjectiveList.get( index )->setRequired( required );
}

//==============================================================================
// BObjectiveManager::getObjectivePlayer
//==============================================================================
bool BObjectiveManager::getObjectivePlayer( int index, BPlayerID playerID )
{
   if( !testIndex( index ) )
   {
      BASSERTM( false, "Invalid objective index!" );
      return( false );
   }
   
   BObjective::EObjectiveFlags flag = (BObjective::EObjectiveFlags)0;

   switch( playerID )
   {
      case 1:
         flag = BObjective::cObjectivePlayer1;
         break;

      case 2:
         flag = BObjective::cObjectivePlayer2;
         break;

      case 3:
         flag = BObjective::cObjectivePlayer3;
         break;

      case 4:
         flag = BObjective::cObjectivePlayer4;
         break;

      case 5:
         flag = BObjective::cObjectivePlayer5;
         break;

      case 6:
         flag = BObjective::cObjectivePlayer6;
         break;
   }

   return( mObjectiveList.get( index )->hasFlag( flag ) );
}

//==============================================================================
// BObjectiveManager::setObjectivePlayer
//==============================================================================
void BObjectiveManager::setObjectivePlayer( int index, BPlayerID playerID )
{
   if( !testIndex( index ) )
   {
      BASSERTM( false, "Setting objective player with invalid index!" );
      return;
   }

   BObjective::EObjectiveFlags flag = BObjective::cObjectivePlayer1;

   switch( playerID )
   {
      case 1:
         flag = BObjective::cObjectivePlayer1;
         break;

      case 2:
         flag = BObjective::cObjectivePlayer2;
         break;

      case 3:
         flag = BObjective::cObjectivePlayer3;
         break;

      case 4:
         flag = BObjective::cObjectivePlayer4;
         break;

      case 5:
         flag = BObjective::cObjectivePlayer5;
         break;

      case 6:
         flag = BObjective::cObjectivePlayer6;
         break;
   }

   mObjectiveList.get( index )->addFlag( flag );
}

//==============================================================================
// Get the objective's score
//==============================================================================
uint BObjectiveManager::getObjectiveScore(int index)
{
   if (!testIndex(index))
   {
      BASSERTM(false, "Getting objective score with invalid index!");
      return (0);
   }

   return (mObjectiveList.get(index)->getScore());   
}

//==============================================================================
// Set the objective's score
//==============================================================================
void BObjectiveManager::setObjectiveScore(int index, uint score)
{
   if (!testIndex(index))
   {
      BASSERTM(false, "Setting objective score with invalid index!");
      return;
   }

   mObjectiveList.get(index)->setScore(score);
}

//==============================================================================
int BObjectiveManager::getObjectiveFinalCount( BObjectiveID ID)
{
   int index = getIndexFromID(ID);
   if (!testIndex(index))
   {
      BASSERTM(false, "Getting objective final count with invalid index!");
      return (0);
   }

   return (mObjectiveList.get(index)->getFinalCount());   
}

//==============================================================================
void BObjectiveManager::setObjectiveFinalCount( BObjectiveID ID, int count )
{
   int index = getIndexFromID(ID);
   if (!testIndex(index))
   {
      BASSERTM(false, "Setting objective final count with invalid index!");
      return;
   }

   mObjectiveList.get(index)->setFinalCount(count);
}

//==============================================================================
int BObjectiveManager::getObjectiveCurrentCount( BObjectiveID ID )
{
   int index = getIndexFromID(ID);
   if (!testIndex(index))
   {
      BASSERTM(false, "Getting objective current count with invalid index!");
      return (0);
   }

   return (mObjectiveList.get(index)->getCurrentCount());   
}

//==============================================================================
void BObjectiveManager::setObjectiveCurrentCount( BObjectiveID ID, int count )
{
   int index = getIndexFromID(ID);
   if (!testIndex(index))
   {
      BASSERTM(false, "Setting objective current count with invalid index!");
      return;
   }

   mObjectiveList.get(index)->setCurrentCount(count);

/*
   BObjectiveMessage *pMessage = getMessageNotification();
   if (pMessage == NULL)
      addObjectiveNotification( index, true);
   else
      pMessage->setShouldRefresh(true);
*/
}

#if defined( BUILD_DEBUG )
//==============================================================================
// BObjectiveManager::getObjectiveName
//==============================================================================
BSimUString* BObjectiveManager::getObjectiveName( int index )
{
   if( !testIndex( index ) )
   {
      return( NULL );
   }

   return( mObjectiveList.get( index )->getName() );
}

//==============================================================================
// BObjectiveManager::setObjectiveName
//==============================================================================
void BObjectiveManager::setObjectiveName( int index, BSimUString name )
{
   if( !testIndex( index ) )
   {
      BASSERTM( false, "Setting objective name with invalid index!" );
      return;
   }

   mObjectiveList.get( index )->setName( name );
}
#endif

//==============================================================================
// BObjectiveManager::setObjectiveDisplayed
//==============================================================================
void BObjectiveManager::setObjectiveDisplayed( BObjectiveID ID, bool displayed, float duration )
{
   int index = getIndexFromID( ID );
   if( index < 0 )
   {
      BASSERTM( false, "Invalid objective ID!" );
      return;
   }

   mObjectiveList.get( index )->setDiscovered( displayed );

   addObjectiveNotification( ID, displayed, duration);

   gUI.playRumbleEvent(BRumbleEvent::cTypeNewObjective);
}

//==============================================================================
// BObjectiveManager::setObjectiveCompleted
//==============================================================================
void BObjectiveManager::setObjectiveCompleted( BObjectiveID ID, bool completed )
{
   int index = getIndexFromID( ID );
   if( index < 0 )
   {
      BASSERTM( false, "Invalid objective ID!" );
      return;
   }

   // [10/23/2008 xemu] check if we've already completed this, and if so, just bail.
   if (completed != mObjectiveList.get(index)->isCompleted())
   {
      mObjectiveList.get( index )->setCompleted( completed );
      gScoreManager.reportObjectiveCompleted( completed, mObjectiveList.get(index)->getScore() );

      addObjectiveNotification( ID, !completed);

      gUI.playRumbleEvent(BRumbleEvent::cTypeObjectiveComplete);
   }
}

//==============================================================================
// BObjectiveManager::addObjectiveNotification
//==============================================================================
void BObjectiveManager::addObjectiveNotification( BObjectiveID objectiveID, bool isNew, float duration )
{
   if (!isNew)
      return;
   
   DWORD state = 0;
   if (isNew)
      state = BObjectiveMessage::cObjectiveMessageNew;
   else
      state = BObjectiveMessage::cObjectiveMessageComplete;

   // if this is a completed objective, then check to see if we are currently displaying it.
   uint numObjectiveMessages = mObjectiveMessageList.getSize();
   for (uint i = 0; i < numObjectiveMessages; i++)
   {
      BObjectiveMessage* pMessage = mObjectiveMessageList[i];
      if (!pMessage)
         continue;

      if (objectiveID != pMessage->getObjectiveID())
         continue;

      // just set the state and continue;
      pMessage->setState(state);
      pMessage->setIsNew(isNew);
      return;
   }

   // add a new message
   BObjectiveMessage* pMessage = new BObjectiveMessage(objectiveID, state, duration);
   mObjectiveMessageList.add(pMessage);

   // Delegate this to the talking head UI control
   //gSoundManager.playSoundCueByEnum(BSoundManager::cSoundObjectiveOnDisplay);   
}

//==============================================================================
// BObjectiveManager::removeObjectiveNotification
//==============================================================================
void BObjectiveManager::removeObjectiveNotification(BObjectiveID objectiveID)
{
   uint numObjectiveMessages = mObjectiveMessageList.getSize();
   for (uint i = 0; i < numObjectiveMessages; i++)
   {
      BObjectiveMessage* pMessage = mObjectiveMessageList[i];
      if (!pMessage)
         continue;

      if (pMessage->getObjectiveID() == objectiveID)
      {
         delete pMessage;
         mObjectiveMessageList[i] = NULL;
         mObjectiveMessageList.removeIndex(i);
         break;
      }
   }
}

//==============================================================================
// BObjectiveManager::setObjectiveUserMessage
//==============================================================================
BObjectiveMessage* BObjectiveManager::getMessageNotification()
{
   if (mObjectiveMessageList.getNumber() == 0)
      return NULL;

   return mObjectiveMessageList[0];
}


//==============================================================================
// BObjectiveManager::setObjectiveUserMessage
//==============================================================================
void BObjectiveManager::setObjectiveUserMessage( BObjectiveID ID, int index, float xPos, float yPos, int justify, float point, float alpha, BColor color, bool enabled )
{
   int objIndex = getIndexFromID( ID );
   if( objIndex < 0 )
   {
      BASSERTM( false, "Invalid objective ID!" );
      return;
   }

   BObjective* pObj = getObjective(objIndex);
   if (!pObj)
   {
      BASSERTM( false, "Invalid objective id");
      return;
   }

   BPlayerIDArray playerList;

   BPlayerID playerID = gUserManager.getUser(BUserManager::cPrimaryUser)->getPlayerID();
   if( getObjectivePlayer( objIndex, playerID ) )
      playerList.uniqueAdd( playerID );

   BSimUString temp;
   temp.set(pObj->getDescription().getPtr());
   // This user needs this objectives message
   BUser::setUserMessage( index,
                          playerList,
                          &temp,
                          xPos,
                          yPos,
                          (BUser::USER_MESSAGE_JUSTIFY)justify,
                          point,
                          alpha,
                          color,
                          enabled );
}

//==============================================================================
// BObjectiveManager::getObjectiveDisplayed
//==============================================================================
bool BObjectiveManager::getObjectiveDisplayed( int index )
{   
   if( !testIndex( index ) )
   {
      BASSERTM( false, "Invalid objective index!" );
      return( false );
   }

   return( mObjectiveList.get( index )->isDiscovered() );
}

//==============================================================================
// BObjectiveManager::getObjectiveCompleted
//==============================================================================
bool BObjectiveManager::getObjectiveCompleted( int index )
{
   if( !testIndex( index ) )
   {
      BASSERTM( false, "Invalid objective index!" );
      return( false );
   }

   return( mObjectiveList.get( index )->isCompleted() );
}

//==============================================================================
// BObjectiveManager::getIndexFromID
//==============================================================================
int BObjectiveManager::getIndexFromID( BObjectiveID ID )
{
   uint count = getNumberObjectives();
   for( uint idx = 0; idx < count; idx++ )
   {
      if( mObjectiveList.get( idx )->getID() == ID )
      {
         return( idx );
      }
   }

   return( -1 );
}

//==============================================================================
// BObjectiveManager::save
//==============================================================================
bool BObjectiveManager::save(BStream* pStream, int saveType) const
{  
   GFWRITEVAR(pStream, BObjectiveID, mNextID);
   GFWRITEFREELIST(pStream, saveType, BObjective, mObjectiveList, uint16, 100);
   GFWRITEMARKER(pStream, cSaveMarkerObjective);
   GFWRITECLASSPTRARRAY(pStream, saveType, BObjectiveMessage, mObjectiveMessageList, uint16, 200);
   GFWRITEMARKER(pStream, cSaveMarkerMessage);
   return true;
}

//==============================================================================
// BObjectiveManager::load
//==============================================================================
bool BObjectiveManager::load(BStream* pStream, int saveType)
{  
   GFREADVAR(pStream, BObjectiveID, mNextID);
   GFREADFREELIST(pStream, saveType, BObjective, mObjectiveList, uint16, 100);
   GFREADMARKER(pStream, cSaveMarkerObjective);
   GFREADCLASSPTRARRAY(pStream, saveType, BObjectiveMessage, mObjectiveMessageList, uint16, 200);
   GFREADMARKER(pStream, cSaveMarkerMessage);
   return true;
}

//==============================================================================
// BObjective::save
//==============================================================================
bool BObjective::save(BStream* pStream, int saveType) const
{  
   GFWRITESTRING(pStream, BUString, mDescription, 1000);
   GFWRITESTRING(pStream, BUString, mDescriptionWithCount, 1000);
   GFWRITEVAR(pStream, uint, mTrackerDuration);
   GFWRITEVAR(pStream, uint, mMinTrackerIncrement);
   GFWRITESTRING(pStream, BUString, mTrackerText, 1000);
   GFWRITESTRING(pStream, BUString, mHint, 1000);
   GFWRITEVAR(pStream, DWORD, mFlags);
   GFWRITEVAR(pStream, DWORD, mTimeCompleted);
   GFWRITEVAR(pStream, BObjectiveID, mID);
   GFWRITEVAR(pStream, uint, mScore);
   GFWRITEVAR(pStream, int, mCurrentCount);
   GFWRITEVAR(pStream, int, mFinalCount);
   GFWRITEMARKER(pStream, cSaveMarkerObjective);
   return true;
}

//==============================================================================
// BObjective::load
//==============================================================================
bool BObjective::load(BStream* pStream, int saveType)
{  
   GFREADSTRING(pStream, BUString, mDescription, 1000);
   GFREADSTRING(pStream, BUString, mDescriptionWithCount, 1000);
   GFREADVAR(pStream, uint, mTrackerDuration);
   GFREADVAR(pStream, uint, mMinTrackerIncrement);
   GFREADSTRING(pStream, BUString, mTrackerText, 1000);
   GFREADSTRING(pStream, BUString, mHint, 1000);
   GFREADVAR(pStream, DWORD, mFlags);
   GFREADVAR(pStream, DWORD, mTimeCompleted);
   GFREADVAR(pStream, BObjectiveID, mID);
   GFREADVAR(pStream, uint, mScore);
   GFREADVAR(pStream, int, mCurrentCount);
   GFREADVAR(pStream, int, mFinalCount);
   GFREADMARKER(pStream, cSaveMarkerObjective);
   return true;
}

//==============================================================================
// BObjectiveMessage::save
//==============================================================================
bool BObjectiveMessage::save(BStream* pStream, int saveType) const
{  
   GFWRITEVAR(pStream, DWORD, mObjectiveMessageState);
   GFWRITEVAR(pStream, BObjectiveID, mObjectiveID);
   GFWRITEVAR(pStream, float, mTimeToDisplay);
   GFWRITEBITBOOL(pStream, mIsNew);
   GFWRITEBITBOOL(pStream, mRefresh);
   GFWRITEBITBOOL(pStream, mbNeverExpire);
   GFWRITEMARKER(pStream, cSaveMarkerMessage);
   return true;
}

//==============================================================================
// BObjectiveMessage::load
//==============================================================================
bool BObjectiveMessage::load(BStream* pStream, int saveType)
{  
   GFREADVAR(pStream, DWORD, mObjectiveMessageState);
   GFREADVAR(pStream, BObjectiveID, mObjectiveID);
   GFREADVAR(pStream, float, mTimeToDisplay);
   GFREADBITBOOL(pStream, mIsNew);
   GFREADBITBOOL(pStream, mRefresh);
   GFREADBITBOOL(pStream, mbNeverExpire);
   GFREADMARKER(pStream, cSaveMarkerMessage);
   return true;
}

//==============================================================================
// BObjectiveArrow::save
//==============================================================================
bool BObjectiveArrow::save(BStream* pStream, int saveType) const
{  
   GFWRITEVECTOR(pStream, mOrigin);
   GFWRITEVECTOR(pStream, mTarget);
   GFWRITEVAR(pStream, float, mOffset);
   GFWRITEVAR(pStream, BEntityID, mObjectID);
   GFWRITEVAR(pStream, BEntityID, mLocationObjectID);
   GFWRITEVAR(pStream, BPlayerID, mPlayerID);
   GFWRITEBITBOOL(pStream, mFlagVisible);
   GFWRITEBITBOOL(pStream, mFlagUseTarget);
   GFWRITEBITBOOL(pStream, mFlagTargetDirty);
   GFWRITEBITBOOL(pStream, mFlagForceTargetVisible);
   GFWRITEMARKER(pStream, cSaveMarkerArrow);
   return true;
}

//==============================================================================
// BObjectiveArrow::load
//==============================================================================
bool BObjectiveArrow::load(BStream* pStream, int saveType)
{  
   GFREADVECTOR(pStream, mOrigin);
   GFREADVECTOR(pStream, mTarget);
   GFREADVAR(pStream, float, mOffset);
   GFREADVAR(pStream, BEntityID, mObjectID);
   GFREADVAR(pStream, BEntityID, mLocationObjectID);
   GFREADVAR(pStream, BPlayerID, mPlayerID);
   GFREADBITBOOL(pStream, mFlagVisible);
   GFREADBITBOOL(pStream, mFlagUseTarget);
   GFREADBITBOOL(pStream, mFlagTargetDirty);
   GFREADBITBOOL(pStream, mFlagForceTargetVisible);
   GFREADMARKER(pStream, cSaveMarkerArrow);
   return true;
}
