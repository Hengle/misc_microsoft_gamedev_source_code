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
// AkRegisteredObj.h
//
//////////////////////////////////////////////////////////////////////
#ifndef _REGISTERED_OBJ_H_
#define _REGISTERED_OBJ_H_

#include "AkKeyArray.h"
#include "AkList2.h"
#include <AK/Tools/Common/AkPlatformFuncs.h>

#define MIN_SIZE_REG_ENTRIES 8  //ListPolldID Size / 4bytes
#define MAX_SIZE_REG_ENTRIES AK_NO_MAX_LIST_SIZE

class CAkAudioNode;
class CAkPBI;

struct AkSoundPositionEntry
{
	AkSoundPosition pos;
	AkUInt32        uListenerIdx;	// if relative to listener, otherwise AK_INVALID_LISTENER_INDEX.
	AkUInt32        uListenerMask;	// bitmask of active listeners
#ifdef RVL_OS
	AkUInt32		uControllerActiveMask;	// Wiimote activation mask
#endif
};

static AkForceInline void _SetSoundPosToListener( const AkListenerPosition & in_listPos, AkSoundPosition & io_sndPos )
{
	// Take the listener orientation, as is
	io_sndPos.Orientation = in_listPos.OrientationFront;
	AkReal32 fScale = AK_LOWER_MIN_DISTANCE;

	// Position the sound AK_LOWER_MIN_DISTANCE Distance unit in front of the Listener
	io_sndPos.Position.X = in_listPos.Position.X + (io_sndPos.Orientation.X * fScale);
	io_sndPos.Position.Y = in_listPos.Position.Y + (io_sndPos.Orientation.Y * fScale);
	io_sndPos.Position.Z = in_listPos.Position.Z + (io_sndPos.Orientation.Z * fScale);

	// Orientation should be the opposite of the listener one
	io_sndPos.Orientation.X = -(io_sndPos.Orientation.X);
	io_sndPos.Orientation.Y = -(io_sndPos.Orientation.Y);
	io_sndPos.Orientation.Z = -(io_sndPos.Orientation.Z);
}

struct AkSwitchHistItem
{
	AkForceInline void IncrementPlayback( AkUInt32 in_Switch )
	{
		if( LastSwitch == in_Switch )
		{
			// If the switch is the same than last time, simply increment it
			++NumPlayBack;
		}
		else
		{
			LastSwitch = in_Switch;
			NumPlayBack = 1;
		}
	}

	AkUInt32 LastSwitch;
	AkUInt32 NumPlayBack;
};

typedef CAkKeyArray<AkUniqueID, AkSwitchHistItem> AkSwitchHist; // Switch Container to SwitchHist

//Class representing a registered Object
class CAkRegisteredObj : public CAkObject
{
	typedef CAkList2<AkUniqueID, AkUniqueID, AkAllocAndFree> AkListNode;

public:

	//Constructor
	CAkRegisteredObj( AkGameObjectID in_GameObjID );

	//Destructor
	virtual ~CAkRegisteredObj();

	AkForceInline void AddRef()
	{
		++m_refCount;
	}

	AkForceInline void Release()
	{
		AKASSERT( m_refCount > 0 );
		if ( --m_refCount <= 0 )
			AkDelete( g_DefaultPoolId, this );
	}

	AkForceInline AkGameObjectID ID() { return m_GameObjID; }

	AKRESULT Init();

	// Set the specified Node as modified (this node has an SIS that will require to be deleted)
	void SetNodeAsModified(
		CAkAudioNode* in_pNode// Node modified
		);

	// Fill the list with the modified elements.
	void FillElementList(
		AkListNode& io_rlistNode// Modified Elements list
		);

	void SetPosition( 
		const AkSoundPosition & in_Position, 
		AkUInt32 in_uListenerIndex 
		);

	void SetActiveListeners(
		AkUInt32 in_uListenerMask	///< Bitmask representing active listeners. LSB = Listener 0, set to 1 means active.
		);

#ifdef RVL_OS
	void SetActiveControllers(
		AkUInt32 in_uActiveControllerMask	///< Bitmask representing active listeners. LSB = Controller 0, set to 1 means active.
		);
#endif

	const AkSoundPositionEntry & GetPosition() 
	{
		return m_PosEntry; 
	}

	AKRESULT SetGameObjectEnvironmentsValues( 
		AkEnvironmentValue*	in_aEnvironmentValues,
		AkUInt32			in_uNumEnvValues
		);

	AKRESULT SetGameObjectDryLevelValue( 
		AkReal32			in_fControlValue
		);

	const AkEnvironmentValue* GetEnvironmentValues() 
	{ 
		return m_EnvironmentValues; 
	}

	AkReal32 GetDryLevelValue(
		) 
	{ 
		return m_fDryLevelValue; 
	}

	AKRESULT SetObjectObstructionAndOcclusion( 
		AkUInt32			in_uListener,
		AkReal32			in_fObstructionValue,
		AkReal32			in_fOcclusionValue
		);

	AkReal32 GetObjectObstructionValue( 
		AkUInt32			in_uListener
		)
	{
		return m_fObstructionValue[ in_uListener ];
	}

	AkReal32 GetObjectOcclusionValue( 
		AkUInt32			in_uListener
		)
	{
		return m_fOcclusionValue[ in_uListener ];
	}

	AkSwitchHist & GetSwitchHist() { return m_SwitchHist; }

#ifdef RVL_OS
	void IncrementGameObjectPlayCount();
	void DecrementGameObjectPlayCount();
#endif

private:

	// Function called once all the PBIs are deactivated and the object is unregistered
	// this function frees the SIS allocated and destroy this*
	void FreeModifiedNodes();

	AkListNode m_listModifiedNodes;

	AkSoundPositionEntry m_PosEntry;
	AkEnvironmentValue	 m_EnvironmentValues[AK_MAX_ENVIRONMENTS_PER_OBJ];

	AkSwitchHist m_SwitchHist;

	//for these values, we could store less bits than a float to save memory?
	AkReal32			 m_fDryLevelValue; 
	AkReal32			 m_fObstructionValue[AK_NUM_LISTENERS];
	AkReal32			 m_fOcclusionValue[AK_NUM_LISTENERS];

	AkGameObjectID      m_GameObjID;
	AkInt32				m_refCount;

#ifdef RVL_OS
	AkUInt32			m_PlayCount;
#endif
};
#endif
