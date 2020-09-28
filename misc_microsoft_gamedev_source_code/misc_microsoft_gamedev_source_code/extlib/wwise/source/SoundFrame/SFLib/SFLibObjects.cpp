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

#include "stdafx.h"
#include "SFLibObjects.h"

#include "SFBytesMem.h"

#include <crtdbg.h>
#include <guiddef.h>
#include <objidl.h>


// serializer.

using namespace SoundFrame;

// -----------------------------------------------------------------------------

SFSoundObject::SFSoundObject()
	: m_bHasAttenuation( false )
	, m_dblAttenuationMaxDistance( 0.0 )
{
}

SFSoundObject::~SFSoundObject()
{
}

bool SFSoundObject::HasAttenuation() const
{
	return m_bHasAttenuation;
}

double SFSoundObject::AttenuationMaxDistance() const
{
	return m_dblAttenuationMaxDistance;
}

void SFSoundObject::HasAttenuation( bool in_bHasAttenuation )
{
	m_bHasAttenuation = in_bHasAttenuation;
}

void SFSoundObject::AttenuationMaxDistance( const double& in_rAttenuationMax )
{
	m_dblAttenuationMaxDistance = in_rAttenuationMax;
}

SFSoundObject * SFSoundObject::From( IReadBytes * in_pBytes )
{
	// Version check: cannot read later versions
	BYTE bVersion = in_pBytes->Read<BYTE>();
	if ( bVersion > SF_PERSIST_SOUNDOBJECT_VERSION_CURRENT )
		return NULL;

	// Read data

	SFSoundObject * pSound = new SFSoundObject();

	if( bVersion < SF_PERSIST_SOUNDOBJECT_VERSION_UNIQUEID )
		in_pBytes->Read<GUID>();
	else
		pSound->m_id = in_pBytes->Read<AkUniqueID>();

	in_pBytes->ReadString( pSound->m_wszName, kStrSize );

	pSound->m_bHasAttenuation = in_pBytes->Read<bool>();
	if ( pSound->m_bHasAttenuation ) 
	{
		// backward compatiblity -- read and discard the radius min value.
		if( bVersion < SF_PERSIST_SOUNDOBJECT_VERSION_UNIQUEID )
			in_pBytes->Read<double>();

		pSound->m_dblAttenuationMaxDistance = in_pBytes->Read<double>();

		if ( bVersion == SF_PERSIST_SOUNDOBJECT_VERSION_FIRST )
		{
			// backward compatiblity -- read and discard transition point.
			if ( in_pBytes->Read<bool>() )
				in_pBytes->Read<double>();
		}
	}

	return pSound;
}

ISoundObject * ISoundObject::From( IReadBytes * in_pBytes )
{
	return SFSoundObject::From( in_pBytes );
}

bool SFSoundObject::To( IWriteBytes * io_pBytes )
{
	bool bSuccess = io_pBytes->Write<BYTE>( SF_PERSIST_SOUNDOBJECT_VERSION_CURRENT )
				 && io_pBytes->Write<AkUniqueID>( m_id )
				 && io_pBytes->WriteString( m_wszName )
				 && io_pBytes->Write<bool>( m_bHasAttenuation );

	if ( bSuccess && m_bHasAttenuation ) 
	{
		bSuccess = io_pBytes->Write<double>( m_dblAttenuationMaxDistance );
	}

	return bSuccess;
}

// -----------------------------------------------------------------------------

SFEvent::SFEvent()
{
}

SFEvent::~SFEvent()
{
}

IActionList * SFEvent::GetActionList()
{
	return GetChildrenList();
}

// Persistence

SFEvent * SFEvent::From( IReadBytes * in_pBytes )
{
	// Version check: cannot read later versions
	BYTE bVersion = in_pBytes->Read<BYTE>();
	if ( bVersion > SF_PERSIST_EVENT_VERSION_CURRENT )
		return NULL;

	// Read data

	SFEvent * pEvent = new SFEvent();

	// backward compatiblity -- read and discard GUID ID.
	if( bVersion == SF_PERSIST_EVENT_VERSION_FIRST )
		in_pBytes->Read<GUID>();

	pEvent->m_id = in_pBytes->Read<AkUniqueID>();
	in_pBytes->ReadString( pEvent->m_wszName, kStrSize );

	ListFrom( pEvent, bVersion, in_pBytes );

	return pEvent;
}

IEvent * IEvent::From( IReadBytes * in_pBytes )
{
	return SFEvent::From( in_pBytes );
}

bool SFEvent::To( IWriteBytes * io_pBytes )
{
	bool bSuccess = io_pBytes->Write<BYTE>( SF_PERSIST_EVENT_VERSION_CURRENT )
				 && io_pBytes->Write<AkUniqueID>( m_id )
				 && io_pBytes->WriteString( m_wszName );

	if ( bSuccess )
	{
		bSuccess = ListTo( io_pBytes );
	}

	return bSuccess;
}


// -----------------------------------------------------------------------------

SFAction::SFAction()
	: m_soundObjectID( AK_INVALID_UNIQUE_ID )
{
}

SFAction::~SFAction()
{
}

AkUniqueID SFAction::GetSoundObjectID() const
{
	return m_soundObjectID;
}

void SFAction::SetSoundObjectID( AkUniqueID in_soundObjectID )
{
	m_soundObjectID = in_soundObjectID;
}

// Persistence

SFAction * SFAction::From( IReadBytes * in_pBytes, BYTE in_bEventVersion )
{
	SFAction * pAction = new SFAction();

	in_pBytes->ReadString( pAction->m_wszName, kStrSize );

	// backward compatiblity -- read and discard GUID ID.
	if( in_bEventVersion == SF_PERSIST_EVENT_VERSION_FIRST )
		in_pBytes->Read<GUID>();
	else	//SF_PERSIST_EVENT_VERSION_CURRENT
		pAction->m_soundObjectID = in_pBytes->Read<AkUniqueID>();

	return pAction;
}

bool SFAction::To( IWriteBytes * io_pBytes )
{
	bool bSuccess = io_pBytes->WriteString( m_wszName )
				 && io_pBytes->Write<AkUniqueID>( m_soundObjectID );

	return bSuccess;
}

// -----------------------------------------------------------------------------

SFState::SFState()
{
}

SFState::~SFState()
{
}

// Persistence

SFState * SFState::From( IReadBytes * in_pBytes, BYTE in_bStateGroupVersion )
{
	SFState * pState = new SFState();

	in_pBytes->ReadString( pState->m_wszName, kStrSize );
	pState->m_id = in_pBytes->Read<AkUniqueID>();

	return pState;
}
	
bool SFState::To( IWriteBytes * io_pBytes )
{
	bool bSuccess = io_pBytes->WriteString( m_wszName )
				 && io_pBytes->Write<AkUniqueID>( m_id );

	return bSuccess;
}

// -----------------------------------------------------------------------------

SFStateGroup::SFStateGroup()
{
}

SFStateGroup::~SFStateGroup()
{
}

IStateList * SFStateGroup::GetStateList()
{
	return GetChildrenList();
}

// Persistence
SFStateGroup * SFStateGroup::From( IReadBytes * in_pBytes )
{
	// Version check: cannot read later versions

	BYTE bVersion = in_pBytes->Read<BYTE>();
	if ( bVersion > SF_PERSIST_STATEGROUP_VERSION_CURRENT )
		return NULL;

	// Read data
	SFStateGroup * pStateGroup = new SFStateGroup();

	pStateGroup->m_id = in_pBytes->Read<AkUniqueID>();
	in_pBytes->ReadString( pStateGroup->m_wszName, kStrSize );

	ListFrom( pStateGroup, bVersion, in_pBytes );

	return pStateGroup;
}

IStateGroup * IStateGroup::From( IReadBytes * in_pBytes )
{
	return SFStateGroup::From( in_pBytes );
}

bool SFStateGroup::To( IWriteBytes * io_pBytes )
{
	bool bSuccess = io_pBytes->Write<BYTE>( SF_PERSIST_STATEGROUP_VERSION_CURRENT )
				 && io_pBytes->Write<AkUniqueID>( m_id )
				 && io_pBytes->WriteString( m_wszName );

	if ( bSuccess )
	{
		bSuccess = ListTo( io_pBytes );
	}

	return bSuccess;
}

// -----------------------------------------------------------------------------

SFSwitch::SFSwitch()
{
}

SFSwitch::~SFSwitch()
{
}

// Persistence

SFSwitch * SFSwitch::From( IReadBytes * in_pBytes, BYTE in_bSwitchGroupVersion )
{
	SFSwitch * pSwitch = new SFSwitch();

	in_pBytes->ReadString( pSwitch->m_wszName, kStrSize );
	pSwitch->m_id = in_pBytes->Read<AkUniqueID>();

	return pSwitch;
}
	
bool SFSwitch::To( IWriteBytes * io_pBytes )
{
	bool bSuccess = io_pBytes->WriteString( m_wszName )
				 && io_pBytes->Write<AkUniqueID>( m_id );

	return bSuccess;
}

// -----------------------------------------------------------------------------

SFSwitchGroup::SFSwitchGroup()
{
}

SFSwitchGroup::~SFSwitchGroup()
{
}

ISwitchList * SFSwitchGroup::GetSwitchList()
{
	return GetChildrenList();
}

// Persistence
SFSwitchGroup * SFSwitchGroup::From( IReadBytes * in_pBytes )
{
	// Version check: cannot read later versions

	BYTE bVersion = in_pBytes->Read<BYTE>();
	if ( bVersion > SF_PERSIST_SWITCHGROUP_VERSION_CURRENT )
		return NULL;

	// Read data
	SFSwitchGroup * pSwitchGroup = new SFSwitchGroup();

	pSwitchGroup->m_id = in_pBytes->Read<AkUniqueID>();
	in_pBytes->ReadString( pSwitchGroup->m_wszName, kStrSize );

	ListFrom( pSwitchGroup, bVersion, in_pBytes );

	return pSwitchGroup;
}

ISwitchGroup * ISwitchGroup::From( IReadBytes * in_pBytes )
{
	return SFSwitchGroup::From( in_pBytes );
}

bool SFSwitchGroup::To( IWriteBytes * io_pBytes )
{
	bool bSuccess = io_pBytes->Write<BYTE>( SF_PERSIST_SWITCHGROUP_VERSION_CURRENT )
				 && io_pBytes->Write<AkUniqueID>( m_id )
				 && io_pBytes->WriteString( m_wszName );

	if ( bSuccess )
	{
		bSuccess = ListTo( io_pBytes );
	}

	return bSuccess;
}

// -----------------------------------------------------------------------------

SFGameObject::SFGameObject()
{
	m_id = s_InvalidGameObject;
}
	
SFGameObject::~SFGameObject()
{}

// Persistence
SFGameObject * SFGameObject::From( IReadBytes * in_pBytes )
{
	// Version check: cannot read later versions

	BYTE bVersion = in_pBytes->Read<BYTE>();
	if ( bVersion > SF_PERSIST_GAMEOBJECT_VERSION_CURRENT )
		return NULL;

	// Read data
	SFGameObject * pGameObject = new SFGameObject();

	pGameObject->m_id = in_pBytes->Read<AkUniqueID>();
	in_pBytes->ReadString( pGameObject->m_wszName, kStrSize );

	return pGameObject;
}

IGameObject * IGameObject::From( IReadBytes * in_pBytes )
{
	return SFGameObject::From( in_pBytes );
}

bool SFGameObject::To( IWriteBytes * io_pBytes )
{
	bool bSuccess = io_pBytes->Write<BYTE>( SF_PERSIST_GAMEOBJECT_VERSION_CURRENT )
				 && io_pBytes->Write<AkUniqueID>( m_id )
				 && io_pBytes->WriteString( m_wszName );

	return bSuccess;
}

// -----------------------------------------------------------------------------

SFGameParameter::SFGameParameter()
	: m_dblRangeMin(0.0)
	, m_dblRangeMax(0.0)
{
}
	
SFGameParameter::~SFGameParameter()
{
}

double SFGameParameter::RangeMin() const
{
	return m_dblRangeMin;
}

double SFGameParameter::RangeMax() const
{
	return m_dblRangeMax;
}

void SFGameParameter::SetRange( const double& in_rMin, const double& in_rMax )
{
	m_dblRangeMin = in_rMin;
	m_dblRangeMax = in_rMax;
}

// Persistence

SFGameParameter * SFGameParameter::From( IReadBytes * in_pBytes )
{
	// Version check: cannot read later versions
	BYTE bVersion = in_pBytes->Read<BYTE>();
	if ( bVersion > SF_PERSIST_GAMEPARAMETER_VERSION_CURRENT )
		return NULL;

	SFGameParameter * pGameParameter = new SFGameParameter();

	in_pBytes->ReadString( pGameParameter->m_wszName, kStrSize );
	pGameParameter->m_id = in_pBytes->Read<AkUniqueID>();

	pGameParameter->m_dblRangeMin = in_pBytes->Read<double>();
	pGameParameter->m_dblRangeMax = in_pBytes->Read<double>();

	return pGameParameter;
}

IGameParameter * IGameParameter::From( IReadBytes * in_pBytes )
{
	return SFGameParameter::From( in_pBytes );
}

bool SFGameParameter::To( IWriteBytes * io_pBytes )
{
	bool bSuccess = io_pBytes->Write<BYTE>( SF_PERSIST_GAMEPARAMETER_VERSION_CURRENT )
				 && io_pBytes->WriteString( m_wszName )
				 && io_pBytes->Write<AkUniqueID>( m_id )
				 && io_pBytes->Write<double>( m_dblRangeMin )
				 && io_pBytes->Write<double>( m_dblRangeMax );

	return bSuccess;
}

// -----------------------------------------------------------------------------

SFTrigger::SFTrigger()
{
}

SFTrigger::~SFTrigger()
{
}

// Persistence

SFTrigger * SFTrigger::From( IReadBytes * in_pBytes )
{
	// Version check: cannot read later versions
	BYTE bVersion = in_pBytes->Read<BYTE>();
	if ( bVersion > SF_PERSIST_TRIGGER_VERSION_CURRENT )
		return NULL;

	SFTrigger * pTrigger = new SFTrigger();

	in_pBytes->ReadString( pTrigger->m_wszName, kStrSize );
	pTrigger->m_id = in_pBytes->Read<AkUniqueID>();

	return pTrigger;
}

ITrigger * ITrigger::From( IReadBytes * in_pBytes )
{
	return SFTrigger::From( in_pBytes );
}
	
bool SFTrigger::To( IWriteBytes * io_pBytes )
{
	bool bSuccess = io_pBytes->Write<BYTE>( SF_PERSIST_TRIGGER_VERSION_CURRENT )
				 && io_pBytes->WriteString( m_wszName )
				 && io_pBytes->Write<AkUniqueID>( m_id );

	return bSuccess;
}

// -----------------------------------------------------------------------------

SFEnvironment::SFEnvironment()
{
}

SFEnvironment::~SFEnvironment()
{
}

// Persistence

SFEnvironment * SFEnvironment::From( IReadBytes * in_pBytes )
{
	// Version check: cannot read later versions
	BYTE bVersion = in_pBytes->Read<BYTE>();
	if ( bVersion > SF_PERSIST_ENVIRONMENT_VERSION_CURRENT )
		return NULL;

	SFEnvironment * pEnvironment = new SFEnvironment();

	in_pBytes->ReadString( pEnvironment->m_wszName, kStrSize );
	pEnvironment->m_id = in_pBytes->Read<AkUniqueID>();

	return pEnvironment;
}

IEnvironment * IEnvironment::From( IReadBytes * in_pBytes )
{
	return SFEnvironment::From( in_pBytes );
}
	
bool SFEnvironment::To( IWriteBytes * io_pBytes )
{
	bool bSuccess = io_pBytes->Write<BYTE>( SF_PERSIST_ENVIRONMENT_VERSION_CURRENT )
				 && io_pBytes->WriteString( m_wszName )
				 && io_pBytes->Write<AkUniqueID>( m_id );

	return bSuccess;
}