//==============================================================================
// objectivemanager.h
//
// objectivemanager manages all objectives
//
// Copyright (c) 2006 Ensemble Studios
//==============================================================================

// Includes
#include "containers\freelist.h"
#include "simtypes.h"
#include "gamefilemacros.h"

#pragma once

// Forward declarations
class BChunkReader;
class BChunkWriter;
class BUser;

typedef int BObjectiveID;

//===============================================
// BObjectiveArrow
//
// Manage objective arrow placement in the world
//===============================================
class BObjectiveArrow
{
   public:
      BObjectiveArrow();
      BObjectiveArrow(BPlayerID playerID, BVector origin, BVector target, bool visible, bool useTarget, bool forceTargetVisible);
      ~BObjectiveArrow() {};

      void update(BUser* pUser);

      BVector getTarget() const { return (mTarget); }
      void setTarget(BVector target) { mTarget = target; mFlagTargetDirty = true; }

      BVector getOrigin() const { return (mOrigin); }
      void setOrigin(BVector origin) { mOrigin = origin; }

      BPlayerID getPlayerID() { return (mPlayerID); }
      void changeOwner(BPlayerID playerID);

      bool getFlagVisible() const { return (mFlagVisible); }
      void setFlagVisible(bool visible) { mFlagVisible = visible; }
      bool getFlagUseTarget() const { return (mFlagUseTarget); }
      void setFlagUseTarget(bool useTarget) { mFlagUseTarget = useTarget; }
      bool getFlagForceTargetVisible() const { return (mFlagForceTargetVisible); }
      void setFlagForceTargetVisible(bool forceTargetVisible) { mFlagForceTargetVisible = forceTargetVisible; }

      bool save(BStream* pStream, int saveType) const;
      bool load(BStream* pStream, int saveType);

   private:      
      BVector   mOrigin;
      BVector   mTarget;
      float     mOffset;
      BEntityID mObjectID;    
      BEntityID mLocationObjectID;
      //BEntityID mGroundFXObjectID;
      BPlayerID mPlayerID;
      bool      mFlagVisible:1;
      bool      mFlagUseTarget:1;
      bool      mFlagTargetDirty:1;
      bool      mFlagForceTargetVisible:1;
};

//=================================
// BObjectiveMessage
//
// Container class.
//=================================
class BObjectiveMessage
{
public:
   BObjectiveMessage(BObjectiveID objectiveID, DWORD state, float duration);
   BObjectiveMessage();
   ~BObjectiveMessage() {}

   enum 
   {
      cObjectiveMessageNew=0,
      cObjectiveMessageComplete,
   };

   bool isNew() { return mIsNew; }
   void setIsNew(bool isNew) { mIsNew = isNew; }
   bool shouldRefresh() { return (mRefresh); }
   void setShouldRefresh(bool val) { mRefresh = val; }

   void updateTime(float elapsedTime);
   bool hasExpired() { return mbNeverExpire?false:(mTimeToDisplay < 0.0f); }

   BObjectiveID getObjectiveID() {return mObjectiveID; }

   bool isNewMessage() { return (mObjectiveMessageState == cObjectiveMessageNew); }
   bool isCompleteMessage() { return (mObjectiveMessageState == cObjectiveMessageComplete); }

   void setState(DWORD state) { mObjectiveMessageState = state; }

   bool save(BStream* pStream, int saveType) const;
   bool load(BStream* pStream, int saveType);

protected:
   DWORD         mObjectiveMessageState;
   BObjectiveID  mObjectiveID;              // get the text from the objective
   float         mTimeToDisplay;   
   bool          mIsNew:1;
   bool          mRefresh:1;
   bool			 mbNeverExpire:1;
};

//=================================
// BObjective
//
// Objective class
//=================================
class BObjective 
{
   public:

      // Flags used with objectives
      enum EObjectiveFlags
      {
         cObjectivePlayer1    = ( 1 << 0 ),
         cObjectivePlayer2    = ( 1 << 1 ),
         cObjectivePlayer3    = ( 1 << 2 ),
         cObjectivePlayer4    = ( 1 << 3 ),
         cObjectivePlayer5    = ( 1 << 4 ),
         cObjectivePlayer6    = ( 1 << 5 ),
         cObjectiveRequired   = ( 1 << 6 ),
         cObjectiveDiscovered    = ( 1 << 7 ),
         cObjectiveCompleted  = ( 1 << 8 )
      };

      BObjective( void );
      ~BObjective( void ){}

      // Assignment operator.
      BObjective& operator = ( const BObjective& srcObj );

      bool operator >  (const BObjective& o) const { return mTimeCompleted >  o.mTimeCompleted; }
      bool operator <  (const BObjective& o) const { return mTimeCompleted <  o.mTimeCompleted; }
      bool operator <= (const BObjective& o) const { return mTimeCompleted <= o.mTimeCompleted; }
      bool operator >= (const BObjective& o) const { return mTimeCompleted >= o.mTimeCompleted; }
      bool operator != (const BObjective& o) const { return mTimeCompleted != o.mTimeCompleted; }
      bool operator == (const BObjective& o) const { return mTimeCompleted == o.mTimeCompleted; }

      // Flag functions
      inline void  addFlag( EObjectiveFlags flag ){ mFlags |= flag; }
      inline void  removeFlag( EObjectiveFlags flag ){ mFlags &= ~flag; }
      inline bool  hasFlag( EObjectiveFlags flag ){ return( ( mFlags & flag ) ? true : false ); }
      inline DWORD getFlags( void ){ return( mFlags ); }

      inline bool isRequired( void ){ return( ( mFlags & cObjectiveRequired ) ? true : false ); }
      inline bool isDiscovered( void ){ return( ( mFlags & cObjectiveDiscovered ) ? true : false ); }
      inline bool isCompleted( void ){ return( ( mFlags & cObjectiveCompleted ) ? true : false ); }
      bool isPlayer(long playerID);

      inline void setRequired( bool required ){ mFlags = required ? ( mFlags | cObjectiveRequired ) : ( mFlags & ~cObjectiveRequired ); }
      inline void setDiscovered( bool displayed ){ mFlags = displayed ? ( mFlags | cObjectiveDiscovered ) : ( mFlags & ~cObjectiveDiscovered ); }
      void  setCompleted( bool completed );
      DWORD getTimeCompleted() const { return mTimeCompleted; }

      // ID
      inline BObjectiveID getID( void ){ return( mID ); }
      inline void         setID( BObjectiveID ID ){ mID = ID; }

      // Description
      inline const BUString& getDescription( void );
      inline void            setDescription( const BUString& description ){ mDescription = description; }

      // Tracker Text
      inline const BUString& getTrackerText( void );
      inline void            setTrackerText( const BUString& trackerText ){ mTrackerText = trackerText; }
      inline uint            getTrackerDuration( void ) { return mTrackerDuration; }
      inline void            setTrackerDuration( uint duration ) { mTrackerDuration = duration; }
      inline uint            getMinTrackerIncrement( void ) { return mMinTrackerIncrement; }
      inline void            setMinTrackerIncrement( uint increment ) { mMinTrackerIncrement = increment; }

      // Hint
      inline const BUString& getHint( void ){ return( mHint ); }
      inline void            setHint( const BUString& hint ){ mHint = hint; }

      // Score
      inline uint getScore( void ){ return( mScore ); }
      inline void setScore( uint score ){ mScore = score; }

      // Counter
      inline int  getFinalCount( void ){ return( mFinalCount ); }
      inline void setFinalCount( int count ){ mFinalCount = count; }
      inline int  getCurrentCount( void ){ return( mCurrentCount ); }
      inline void setCurrentCount( int count );

      // Name
      #if defined( BUILD_DEBUG )
         inline BSimUString* getName( void ){ return( &mName ); }
         inline void         setName( BSimUString name ){ mName = name; }
      #endif

      bool save(BStream* pStream, int saveType) const;
      bool load(BStream* pStream, int saveType);

   protected:
      BUString     mDescription;
      BUString     mTrackerText;
      uint         mTrackerDuration;
      uint         mMinTrackerIncrement;
      BUString     mDescriptionWithCount;
      BUString     mHint;
      DWORD        mFlags;
      DWORD        mTimeCompleted;
      BObjectiveID mID;      
      uint         mScore;
      int          mCurrentCount;      // the current value
      int          mFinalCount;        // the destination value

      #if defined( BUILD_DEBUG )
         BSimUString mName;
      #endif
};

//==========================================
// BObjectiveManager
//
// Management and access to game objectives
//==========================================
class BObjectiveManager
{
   public:

      BObjectiveManager( void );
      virtual ~BObjectiveManager( void );      

      // Management functions 
      bool init( void );
      void reset( void );

      BObjectiveID addObjective( void );
      bool deleteObjectiveByIndex( int index );
      bool deleteObjectiveByID( BObjectiveID ID );

      // Iterate objectives functions
      int getNumberObjectives( void ) const;
      uint getNumberRequiredObjectives( void );

      // ID
      BObjectiveID getObjectiveID( int index );                  
      void setObjectiveID( int index, BObjectiveID ID );
      int getIndexFromID( BObjectiveID ID );

      BObjective* getObjective(int index);

      // Description
      //const BUString& getObjectiveDescription( int index );
      void            setObjectiveDescription( int index, const BUString& description );

      // Tracker Stuff
      void            setObjectiveTrackerText( int index, const BUString& trackerText );
      void            setObjectiveTrackerDuration( int index, uint duration );
      void            setObjectiveMinTrackerIncrement( int index, uint increment );

      // Hint
      //const BUString& getObjectiveHint( int index );
      void            setObjectiveHint( int index, const BUString& hint );

      // Required
      bool getObjectiveRequired( int index );
      void setObjectiveRequired( int index, bool required );

      // Player assignment
      bool getObjectivePlayer( int index, BPlayerID playerID );
      void setObjectivePlayer( int index, BPlayerID playerID );

      // Score
      uint getObjectiveScore( int index );
      void setObjectiveScore( int index, uint score );

      // Counter
      int  getObjectiveFinalCount( BObjectiveID ID );
      void setObjectiveFinalCount( BObjectiveID ID, int count );
      int  getObjectiveCurrentCount( BObjectiveID ID );
      void setObjectiveCurrentCount( BObjectiveID ID, int count );

      // Completed     
      bool getObjectiveCompleted( int index );
      void setObjectiveCompleted( BObjectiveID ID, bool completed );

      // Displayed
      bool getObjectiveDisplayed( int index );
      void setObjectiveDisplayed( BObjectiveID ID, bool displayed, float duration = 10.0 );

      void addObjectiveNotification( BObjectiveID objectiveID, bool isNew, float duration = 10.0 );
      void removeObjectiveNotification(BObjectiveID objectiveID);
      BObjectiveMessage* getMessageNotification();

      // User Message
      void setObjectiveUserMessage( BObjectiveID ID, int index, float xPos, float yPos, int justify, float point, float alpha, BColor color, bool enabled );

      // Name
      #if defined( BUILD_DEBUG )
         BSimUString* getObjectiveName( int index );
         void         setObjectiveName( int index, BSimUString name );
      #endif

      GFDECLAREVERSION();
      bool save(BStream* pStream, int saveType) const;
      bool load(BStream* pStream, int saveType);

   protected:
      inline bool testIndex( int index ){ return( (bool)( ( index >= 0 ) && ( index < (int)getNumberObjectives() ) ) ); }

      // Objective list
      BObjectiveID          mNextID;
      BFreeList<BObjective> mObjectiveList;

      BDynamicSimArray<BObjectiveMessage*>   mObjectiveMessageList;
};