//==============================================================================
// battle.h
//
// Copyright (c) 2004-2007, Ensemble Studios
//==============================================================================
#pragma once

//==============================================================================
// Includes
#include "bitarray.h"
#include "simtypes.h"

//==============================================================================
//Forward declarations
class BUnit;
class BBattle;

//==============================================================================
class IBattleManagerListener
{
   public:
      virtual void unitAddedToBattle(void) = 0;
      virtual void unitRemovedFromBattle(void) = 0;
      virtual void battleWon(int32 battleID) = 0;
      virtual void battleLost(int32 battleID) = 0;
};

//==============================================================================
class BBattlePlayer
{
public:

   BBattlePlayer( long id );
   ~BBattlePlayer( void );

   //ID.
   long                    getID( void ) const { return(mID); }
   void                    setID( long v ) { mID=v; }

   //Units.
   long                    getNumberUnits( void ) const { return(mUnits.getNumber()); }
   BEntityID               getUnitID( long index ) const;
   bool                    addUnit( BEntityID unitID );
   bool                    removeUnit( BEntityID unitID );
   bool                    containsUnit( BEntityID unitID ) const;

   //Update.
   bool                    update( BBattle *battle );

   DWORD                   getPlayerStartTime(void) const {return mPlayerStartTime;}


protected:
   void                    setDefaultFlags( void );
   long                    getUnitIndex( long unitID );

   long                    mID;
   BEntityIDArray          mUnits;   
   DWORD                   mPlayerStartTime;

   //-- Flags
   bool                    mFlagFirstUpdate:1;
   bool                    mFlagDone:1;
};

//==============================================================================
class BBattleUnit
{
   public:      

      //Ctor/Dtor.
      BBattleUnit( void );
      ~BBattleUnit( void );

      //UnitID.
      BEntityID               getUnitID( void ) const { return(mUnitID); }
      void                    setUnitID( BEntityID v ) { mUnitID=v; }
      //TargetUnitID.
      BEntityID               getTargetUnitID( void ) const { return(mTargetUnitID); }
      void                    setTargetUnitID( BEntityID v ) { mTargetUnitID=v; }
      //PlayerID.
      long                    getPlayerID( void ) const { return(mPlayerID); }
      void                    setPlayerID( long v ) { mPlayerID=v; }
      //ExitTimer.
      DWORD                   getExitTime( void ) const { return(mExitTime); }
      void                    setExitTime( DWORD v ) { mExitTime=v; }
      void                    startExit( DWORD v );
      void                    stopExit( void );
		   
      //Reset.
      void                    reset( void );

      //Update.
      void                    update( BBattle *battle );      

      //Flags
      bool                    getFlagExiting() const { return (mFlagExiting); }
      void                    setFlagExiting(bool v) { mFlagExiting = v; }
      bool                    getFlagDestroy() const { return (mFlagDestroy); }
      void                    setFlagDestroy(bool v) { mFlagDestroy = v; }
      bool                    getFlagSplit() const { return (mFlagSplit); }
      void                    setFlagSplit(bool v) { mFlagSplit = v; }
   
   protected:
      void                    setDefaultFlags( void );

      //Data vars.
      BEntityID               mUnitID;
      BEntityID               mTargetUnitID;
      long                    mPlayerID;
      DWORD                   mExitTime;

      // Flags
      bool mFlagExiting:1;
      bool mFlagDestroy:1;
      bool mFlagCanAttack:1;
      bool mFlagSplit:1;
};

//=============================================================================

class BBattle
{
   public:

      BBattle( void );
      ~BBattle( void );

      //ID.
      long                    getID( void ) const { return(mID); }
      void                    setID( long v ) { mID=v; }
      //Create.
      bool                    create( void );
      bool                    create( const BEntityIDArray &units );

      //Position/Size.
      const BVector&          getPosition( void ) const { return(mPosition); }
      float                   getSize( void ) const { return(mSize); }
      float                   getSizeSqr( void ) const { return(mSizeSqr); }

		//Time
		DWORD							getStartTime() const {return(mStartTime);}
		DWORD							getEndTime() const {return(mEndTime);}

      //Units.
      bool                    containsUnit( BEntityID unitID ) const;
      bool                    addUnit( BUnit *unit, BEntityID targetUnitID, bool createPlayerIfNeeded=true );
      bool                    removeUnit( BEntityID unitID, DWORD exitTime, bool force );
      void                    stopUnits( bool cheer );
      long                    getNumberUnits() { return mUnits.getNumber(); }
      BEntityID               getUnitID(long index)
      { 
         if(mUnits.validIndex(index))
            return mUnits[index].getUnitID();
         else
            return cInvalidObjectID;
      }

      //Battle Players.
      long                    getNumberPlayers( void ) const { return(mPlayers.getNumber()); }
      BBattlePlayer*          createPlayer( long playerID );
      BBattlePlayer*          getPlayerByID( long id ) const;
      BBattlePlayer*          getPlayerByIndex( long index ) const;
      BBattlePlayer*          getEnemyPlayer( long playerID ) const;

      //Update.
      bool                    update();
      void                    updateParameters( void );
     
		//Chat
		void							sendChatStatement(const char* promptName, BRelationType playerRelation, long specificTargetID, const BVector* pLocation);

      //Render.
      bool                    render( void );

      //Debug.
      void                    debug( char* v, ... );
		void							getText(BSimString& text) const;
      
      // Flags
      bool                    getFlagDone() const { return (mFlagDone); }
      void                    setFlagDone(bool v) { mFlagDone = v; }

      bool                    getFlagFirstUpdate() const { return (mFlagFirstUpdate); }
      void                    setFlagFirstUpdate(bool v) { mFlagFirstUpdate = v; }

   protected:
      bool                    firstUpdate( void );
      long                    getUnitIndexConst( BEntityID unitID ) const;
      long                    getUnitIndex( BEntityID unitID );
      void                    setDefaultFlags( void );

      //Basic data.
      long                    mID;      
      BVector                 mPosition;
      float                   mSize;
      float                   mSizeSqr;     

      //Player data.
      BDynamicSimArray<BBattlePlayer*> mPlayers;
      //Units.
      BDynamicSimArray<BBattleUnit>  mUnits;
      BDynamicSimArray<BEntityID>    mUnitHistory; //-- List of squads who have ever joined this battle
      
		DWORD							mEndTime;
		DWORD							mStartTime;      

      bool                    mFlagDone:1;
      bool                    mFlagFirstUpdate:1;      
};

//==============================================================================
class BBattleManager
{
public:
   BBattleManager( void );
   ~BBattleManager( void );

   enum 
   {
      cMaxNumPlayers=6
   };

   //Init.
   bool                    init( void );

   //Battle Control Vars.
   DWORD                   getBattleExitTime( void ) const { return(mBattleExitTime); }
   float                   getBattleSize( void ) const { return(mBattleSize); }
   float                   getBattleSizeSqr( void ) const { return(mBattleSizeSqr); }   

   //Battles.
   BBattle*                createBattle( void );
   BBattle*                createBattle( const BEntityIDArray &units );
   long                    getNumberBattles( void ) const { return(mBattles.getNumber()); }
   BBattle*                getBattleByID( long id ) const;
   BBattle*                getBattleByIndex( long index ) const;
   bool                    destroyBattle( long id );
   void                    destroyAllBattles( void );
   BBattle*                findBattle( const BUnit *unit, const BUnit *targetUnit );
   BBattle*                findNearestBattle( BVector location);

   //CompletedBattles
   BBattle*						getLastBattleFromPvP(long playerA, long playerB);

   //Units.
   bool                    removeUnitFromBattle( BUnit *unit, long battleID );
   void                    addUnitToSplit( BEntityID unitID);

   //Update battles.
   void                    updateBattles();
   //Render.
   void                    render( void );
   //Debug.
   void                    debug( char* v, ... );

   bool                    readXML( const BSimString &filename, long directoryID );
   
   void                    unitEnteredCombat(BEntityID unitID, BEntityID targetID);

   void                    trackAddUnitInBattle(const BUnit *pUnit);
   void                    trackRemoveUnitInBattle(const BUnit *pUnit);

   void                    setListener(IBattleManagerListener* pListener) {mpBattleManagerListener = pListener;}
   IBattleManagerListener* getListener() { return mpBattleManagerListener; }

protected:
   void                    playBattleStatusSound(BBattle *battle);
   void                    cleanUp( void );

   long                    mNextBattleID;
   BDynamicSimArray<BBattle*>  mBattles;
   BEntityIDArray              mUnitsToSplit;

   BSimString              mFilename;
   DWORD                   mBattleExitTime;
   float                   mBattleSize;
   float                   mBattleSizeSqr;

   DWORD                   mMinBattleConclusionLength;   

   IBattleManagerListener* mpBattleManagerListener; //-- Make this into a list if it needs more than one listener

};