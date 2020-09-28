/***********************************************************************
  The content of this file includes source code for the sound engine
  portion of the AUDIOKINETIC Wwise Technology and constitutes "Level
  Two Source Code" as defined in the Source Code Addendum attached
  with this file.  Any use of the Level Two Source Code shall be
  subject to the terms and conditions outlined in the Source Code
  Addendum and the End User License Agreement for Wwise(R).

  Version: v2008.2.1  Build: 2821
  Copyright (c) 2006-2008 Audiokinetic Inc.
 ***********************************************************************/

//////////////////////////////////////////////////////////////////////
//
// AkRanSeqCntr.h
//
//////////////////////////////////////////////////////////////////////
#ifndef _RAN_SEQ_CNTR_H_
#define _RAN_SEQ_CNTR_H_

#include "AkContainerBase.h"
#include "AkKeyList.h"
#include "AkRanSeqBaseInfo.h"

//Forward definitions
class CAkPlayList;

// class corresponding to a Random/Sequence container
//
// Author:  alessard
class CAkRanSeqCntr : public CAkContainerBase
{
public:

	struct CoreRanSeqCntr
	{
		AkUInt16						m_wAvoidRepeatCount;			// Avoid repeat count, set to 0 if none

		RANGED_PARAMETER<AkTimeMs>	m_TransitionTime;				// Used for both crossfade and delay

		AkTransitionMode			m_eTransitionMode :TRANSITION_MODE_NUM_STORAGE_BIT;	// Transition mode(delay,crossfade, sample accurate or none)
		AkRandomMode				m_eRandomMode :2;				// Shuffle or normal (shuffle has a default avoid count of 1)
		AkContainerMode				m_eMode :3;						// Sequence or random mode
		AkUInt8						m_bIsUsingWeight :1;			// Is the container using weight (random mode only)
		AkUInt8						m_bResetPlayListAtEachPlay :1;	// Used only by sequence in continuous mode, true is the normal behavior
		AkUInt8						m_bIsRestartBackward :1;		// Does the container restart backward in sequence continuous mode
		AkUInt8						m_bIsContinuous :1;				// Is the container in continuous mode
		AkUInt8						m_bIsGlobal :1;					// Is the container Global(opposite of per object)
	};

	// Thread safe version of the constructor
	static CAkRanSeqCntr* Create(AkUniqueID in_ulID = 0, AkContainerMode in_ContainerMode = ContainerMode_Sequence);

	AKRESULT Init();
	void	 Term();

	//Return the node category
	virtual AkNodeCategory NodeCategory();

	// Notify the children that the associated object was unregistered
	virtual void Unregister(
		CAkRegisteredObj * in_pGameObj //Game object associated to the action
		);

	// Add an item in the playlist
	AKRESULT AddPlaylistItem(
		AkUniqueID in_ElementID,	// Child unique ID
		AkUInt8 in_cWeight = DEFAULT_RANDOM_WEIGHT			// Weight of the item(used for random, facultative parameter)
		);

	// Remove the specified item from the playlist
	AKRESULT RemovePlaylistItem(
		AkUniqueID in_ElementID		// Element unique ID
		);

	// Clear the content of the playlist
	AKRESULT ClearPlaylist();

	//---------------  Not to be exported to thd SDK  ---------------
	void _SetItemWeight(
		AkUniqueID in_ID, 
		AkUInt8 in_cWeight
		);

	// Set the weight of an item in the playlist
	// must work with position since the item might be more than once in the list
	void SetItemWeight(
		AkUInt16 in_Position,	// Element position in the playlist
		AkUInt8 in_cWeight		// Element weight
		);

	// Returns if the Container is restarting backward in Sequence mode
	//
	// Returns - bool -  true if the container will restart backward
	bool RestartBackward();

	// Set the RestartBackward behavior
	void RestartBackward(
		const bool in_bRestartBackward // Does the container should restart backward?
		);

	// Returns if the container is in continuous mode
	//
	// Return - bool - true if in continuous mode
	bool Continuous();

	// Set The continuous mode
	void Continuous(
		const bool in_bIsContinuous // Set the continuous mode
		);

	// Returns if the container is in global mode
	//
	// Return - bool - true = Global Mode
	//				   false = Per object mode
	bool IsGlobal();

	// Set the global or per object mode
	void IsGlobal(
		bool in_bIsGlobal	//true = global or false = per object mode
		);

	// Is the playlist reseted at each play in sequence continuous mode(ignored in others situations)
	//
	// Return - bool - Is the playlist reseted at each play
	bool ResetPlayListAtEachPlay();

	// Set if the playlist is reseted at each play in sequence continuous mode(ignored in others situations)
	void ResetPlayListAtEachPlay(
		bool in_bResetPlayListAtEachPlay //Is the playlist reseted at each play
		);

	// Returns the transition mode 
	//
	// return - AkTransitionMode - (disabled, crossfade or delayed)
	AkTransitionMode TransitionMode();

	// set the transition mode 
	//(disabled, crossfade or delayed)
	void TransitionMode(
		AkTransitionMode in_eTransitionMode //Transition mode
		);

	// Gets the transitionTime
	//
	// Returns - AkTimeMs - Transition time in ms
	AkTimeMs TransitionTime();

	// Sets the transitionTime
	void TransitionTime(
		AkTimeMs in_TransitionTime,	// Transition time in ms
		AkTimeMs in_RangeMin = 0,	// Min Range
		AkTimeMs in_RangeMax = 0	// Max Range
		);

	// Get the random mode
	//
	// Return - AkRandomMode - Normal or shuffle
	AkRandomMode RandomMode();

	// Set the random mode
	void RandomMode(
		AkRandomMode in_eRandomMode	//Random mode - Normal or shuffle
		);

	// Get the avoid repeat count of the container
	//
	// Return - AkUInt16 - AvoidRepeatingCount
	AkUInt16 AvoidRepeatingCount();

	// Set the avoid repeat count of a container
	void AvoidRepeatingCount(
		AkUInt16 in_wCount	//Avoir repeating count (minimum of 1 in shuffle mode)
		);

	// Get the mode of the container
	//
	// Return - AkContainerMode - Mode Random or Sequence available
	AkContainerMode Mode();

	// Set the mode of the Container
	AKRESULT Mode(
		AkContainerMode in_eMode	//Mode Random or Sequence
		);

	//Set Sound Looping info
	void Loop(
		bool  in_bIsLoopEnabled,
		bool  in_bIsLoopInfinite,
		AkInt16 in_sLoopCount
		);

	// PlayAndContinue the specified node
	// Does the same as the Play() does, but have more parameters telling that the play
	// passed by a Continuous container and the PBI will have to launch another actions
	// at a given time.
    //
    // Return - AKRESULT - Ak_Success if succeeded
	virtual AKRESULT Play( AkPBIParams& in_rPBIParams );

	virtual CAkPBI* CreatePBI( CAkSoundBase*    in_pSound,
							   CAkSource*	    in_pSource,
                               AkPBIParams&     in_rPBIParams,
                               AkPriority       in_priority ) const;

	// This function is redefined because the current input 
	// must be removed in authoring mode when the current input is removed
	virtual AKRESULT RemoveChild(
		AkUniqueID in_ulID		// Child uniqueID
		);

#ifndef AK_OPTIMIZED
	// This function has two main uses
	// This function does nothing if not in sequence mode
	// In STEP mode:
	//		This function set the next to play
	// In Continuous mode:
	//		if the container is actually playing, is changes the current output sound to the specified position
	//		if not playing(or if in_PlayingID == 0) it sets the starting point for the next play
	void ForceNextToPlay(
		AkInt16 in_iPosition,						//Position in the playlist
		CAkRegisteredObj * in_pGameObj = NULL,		// Game object to affect, this parameter is ignored in Global mode
		AkPlayingID in_PlayingID = NO_PLAYING_ID// PlayingID (returned by the akPostEvent() ) 
												// Required to switch the actually playing sounds, 
												// If set to NO_PLAYING_ID, it simply sets the next starting point of the sequence
		);

	// Return the position of the item that will be played for the specified game object
	// This function will unconditionnally return 0 if the Container is set as random
	// or if the container is continuous and reset playlist at each play is enabled
	AkInt16 NextToPlay(
		CAkRegisteredObj * in_pGameObj = NULL
		);
#endif

	AKRESULT SetInitialValues(AkUInt8* in_pData, AkUInt32 in_ulDataSize);

	// Returns the next Audionode to be played
	// Same as GetNextToPlay but this one is called instead if the 
	// Container is in continuous mode
	//
	// Returns - CAkAudioNode* - Next audionode to be played
	virtual CAkAudioNode* GetNextToPlayContinuous(
		CAkRegisteredObj * in_pGameObj,					// Game object associated
		AkUInt16&		out_rwPositionSelected,
		CAkContainerBaseInfoPtr& pContainerInfo,	// Container information specific in the sequence
		AkLoop&		io_rLoopInfo					// Looping information reference
		);

protected:

	// Constructors
    CAkRanSeqCntr(AkUniqueID in_ulID, AkContainerMode in_ContainerMode = ContainerMode_Sequence);

	// Destructor
    virtual ~CAkRanSeqCntr(void);

	// Returns the next Audionode to be played
	//
	// NOTE : plenty of process is done in this function, 
	// do not interpret is as a simple get function
	//
	// Returns - CAkAudioNode* - Next audionode to be played
	virtual CAkAudioNode* GetNextToPlay(
		CAkRegisteredObj * in_pGameObj,	//Game object pointer
		AkUInt16&		out_rwPositionSelected
		);

public:
	// Resets the information concerning the playlist 
	// and continuity(does not mean reseting the playlist)
	void ResetSpecificInfo();

private:

	AKRESULT _Play( AkPBIParams& in_rPBIParams );
	AKRESULT _PlayContinuous( AkPBIParams& in_rPBIParams );

	// Select Randomly a sound to play from the specified set of parameters
	//
	// Return - AkUInt16 - Position in the playlist to play
	AkUInt16 SelectRandomly(
		CAkRandomInfo* in_pRandomInfo,	// Container set of parameters
		bool& out_bIsAnswerValid,		// out value, is the result valid
		AkLoop* io_pLoopCount = NULL    // Looping information
		);

	// Select SelectSequentially a sound to play from the specified set of parameters
	//
	// Return - AkUInt16 - Position in the playlist to play
	AkUInt16 SelectSequentially(
		CAkSequenceInfo* in_pSeqInfo,	// Container set of parameters
		bool& out_bIsAnswerValid,		// out value, is the result valid
		AkLoop* io_pLoopCount = NULL	// Looping information
		);

	// Function called once a complete loop is completed.
	// 
	// Return - bool - true if the loop must continue, false if nothing else has to be played
	bool CanContinueAfterCompleteLoop(
		AkLoop* io_pLoopingInfo			// Looping information (not const)
		);

	// Gets if the playlist position can be played(already played and avoid repeat)
	//
	// Return - bool - true if can play it, false otherwise
	bool CanPlayPosition(
		CAkRandomInfo* in_pRandomInfo,	// Container set of parameters
		AkUInt16 in_wPosition				// Position in the list
		);

	// Calculate the total weight of the playlist
	//
	// Return - AkUInt32 - Total weight of the playlist
	AkUInt32 CalculateTotalWeight();

	// Function called in sequence AND continuous AND do not reset playlist mode
	// It updates the content that will be used by the next play on the container
	void UpdateResetPlayListSetup(
		CAkSequenceInfo* in_pSeqInfo,	// Container set of parameters
		CAkRegisteredObj * in_pGameObj	// Game object (Null in global mode)
		);

	CAkPlayList*				m_pPlayList;					// Playlist of the container

	typedef AkHashListBare<CAkRegisteredObj *, CAkContainerBaseInfo, 31> AkMapObjectCntrInfo;
	AkMapObjectCntrInfo m_mapObjectCntrInfo;	// Per object playlist inforamtion

	CAkContainerBaseInfo*		m_pGlobalContainerInfo;			// Shared information about the content to play

	CoreRanSeqCntr				m_RanSeqParams;

	AkInt16						m_Loop;

	bool	m_bContainerBeenPlayed;	// Initially set to false
									// it is set tu true once it has been playing, 
									// telling to stop childs if the playlist is affected
	
};
#endif
