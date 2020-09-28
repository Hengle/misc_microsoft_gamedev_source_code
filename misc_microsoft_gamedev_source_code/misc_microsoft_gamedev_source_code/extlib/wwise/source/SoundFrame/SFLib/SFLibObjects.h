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

#pragma once

#include <crtdbg.h>

#include <AK/IBytes.h>
#include <AK/SoundFrame/SFObjects.h>

#include "SFArray.h"
#include "SFPrivate.h"

using namespace AK;
using namespace SoundFrame;

//-------------------------------------------------------------------------------------------------
// 

template <class TRefCountInterface> 
class CRefCountBase
	: public TRefCountInterface
{
public:
	CRefCountBase()
		: m_cRef( 1 )
	{
	}

	virtual ~CRefCountBase() 
	{
	}

    virtual long AddRef() 
    { 
        return ++m_cRef; 
    }

	virtual long Release()
    {
		_ASSERT( m_cRef );
        if (!--m_cRef)
        {
            delete this;
			return 0L;
        }

        return m_cRef;
    }

private:
	long m_cRef;
};

template <class TYPE, class TPublicInterface> 
class SFList
	: public CRefCountBase<TPublicInterface>
	, public SFArray<TYPE, TYPE>
{
public:
	typedef typename TYPE				SFObject_type;
	typedef typename TPublicInterface	SFList_type;

	SFList()
		: m_lPos( 0 )
	{
	}

	virtual ~SFList()
	{
		long cItems = GetSize();
		for ( int i = 0; i < cItems; i++ )
		{
			GetAt( i )->Release();
		}
	}

	virtual long GetCount() const
	{
		return GetSize();
	}

	virtual void Reset()
	{
		m_lPos = 0;
	}

	virtual TYPE Next()
	{
		return ( m_lPos < GetSize() ) ? GetAt( m_lPos++ ) : NULL;
	}

private:
	long m_lPos;
};


typedef SFList<IEvent *, IEventList> EventList;
typedef SFList<IAction *, IActionList> ActionList;
typedef SFList<ISoundObject *, ISoundObjectList> SoundObjectList;
typedef SFList<IStateGroup *, IStateGroupList> StateGroupList;
typedef SFList<IState *, IStateList> StateList;
typedef SFList<ISwitchGroup *, ISwitchGroupList> SwitchGroupList;
typedef SFList<ISwitch *, ISwitchList> SwitchList;
typedef SFList<IGameObject *, IGameObjectList> GameObjectList;
typedef SFList<IGameParameter *, IGameParameterList> GameParameterList;
typedef SFList<ITrigger *, ITriggerList> TriggerList;
typedef SFList<IEnvironment *, IEnvironmentList> EnvironmentList;

template <class TInterface> 
class SFNamedObject
	: public CRefCountBase<TInterface>
{
public:
	SFNamedObject()
	{
		m_wszName[ 0 ] = 0;
	}

	virtual ~SFNamedObject() 
	{
	}

	virtual const WCHAR * GetName() const 
	{
		return m_wszName;
	}

	void SetName( const WCHAR* in_szName )
	{
		::_tcsncpy( m_wszName, in_szName, kStrSize );
	}

protected:
	WCHAR        m_wszName[ kStrSize ];
};

template <class TInterface> 
class SFObjectWithID
	: public SFNamedObject<TInterface>
{
public:
	SFObjectWithID()
		: m_id(AK_INVALID_UNIQUE_ID)
	{
	}

	virtual ~SFObjectWithID() 
	{
	}

	virtual AkUniqueID  GetID() const
	{
		return m_id;
	}
	
	void SetID( AkUniqueID in_id )
	{
		m_id = in_id;
	}

protected:
	AkUniqueID         m_id;
};

template <class TInterface, class TChild, class TList>
class SFObjectWithChildList
	: public SFObjectWithID<TInterface>
{
public:
	SFObjectWithChildList()
		:	m_pChildrens( NULL )
	{}

	virtual ~SFObjectWithChildList()
	{
		if( m_pChildrens )
			m_pChildrens->Release();
	}

	typename TList::SFList_type * GetChildrenList()
	{
		if ( m_pChildrens ) 
		{
			m_pChildrens->Reset();
		}

		return m_pChildrens;
	}

	void AddChild( TChild* in_pChild )
	{
		if( !in_pChild )
			return;

		if( !m_pChildrens )
			m_pChildrens = new TList;

		m_pChildrens->Add( in_pChild );

		in_pChild->AddRef();
	}

	// Persistence
	static void ListFrom( SFObjectWithChildList* in_pObject, BYTE in_bVersion, IReadBytes * in_pBytes )
	{
		long cChildrens = in_pBytes->Read<long>();

		if ( cChildrens )
		{
			in_pObject->m_pChildrens = new TList;
			in_pObject->m_pChildrens->SetSize( cChildrens );
			
			long i = 0;
			for (; cChildrens > 0; --cChildrens )
			{
				TChild* pChild = TChild::From( in_pBytes, in_bVersion );
				if( pChild )
				{
					in_pObject->m_pChildrens->SetAt( i, pChild );
					++i;
				}
			}
			
			in_pObject->m_pChildrens->SetSize( i );
		}
	}

	bool ListTo( IWriteBytes * io_pBytes )
	{
		long cChildrens = m_pChildrens ? m_pChildrens->GetCount() : 0;

		bool bSuccess = io_pBytes->Write<long>( cChildrens );

		for ( long i = 0; i < cChildrens && bSuccess; ++i )
		{
			TChild * pChild = (TChild *) m_pChildrens->GetAt( i );

			bSuccess = pChild->To( io_pBytes );
		}

		return bSuccess;
	}

private:

	TList * m_pChildrens;
};

//-------------------------------------------------------------------------------------------------
//  
class SFAction
	: public SFNamedObject<IAction>
{
public:
	SFAction();
	virtual ~SFAction();

	virtual AkUniqueID GetSoundObjectID() const;

	void SetSoundObjectID( AkUniqueID in_soundObjectID );

	// Persistence
	static SFAction * From( IReadBytes * in_pBytes, BYTE in_bEventVersion );
	bool To( IWriteBytes * io_pBytes );

private:
	AkUniqueID   m_soundObjectID;
};

//-------------------------------------------------------------------------------------------------
//  
class SFSoundObject
	: public SFObjectWithID<ISoundObject>
{
public:
	SFSoundObject();
	virtual ~SFSoundObject();

	virtual bool HasAttenuation() const;
	virtual double AttenuationMaxDistance() const;

	void HasAttenuation( bool in_bHasRadius );
	void AttenuationMaxDistance( const double& in_rAttenuationMax );

	// Persistence
	static SFSoundObject * From( IReadBytes * in_pBytes );
	bool To( IWriteBytes * io_pBytes );

private:
	double m_dblAttenuationMaxDistance;

	bool   m_bHasAttenuation;
};

//-------------------------------------------------------------------------------------------------
//  
class SFEvent
	: public SFObjectWithChildList<IEvent, SFAction, ActionList>
{
public:
	SFEvent();
	virtual ~SFEvent();

    virtual IActionList * GetActionList();

	// Persistence
	static SFEvent * From( IReadBytes * in_pBytes );
	bool To( IWriteBytes * io_pBytes );
};

//-------------------------------------------------------------------------------------------------
//  
class SFState
	: public SFObjectWithID<IState>
{
public:
	SFState();
	virtual ~SFState();

	// Persistence
	static SFState * From( IReadBytes * in_pBytes, BYTE in_bStateGroupVersion );
	bool To( IWriteBytes * io_pBytes );
};

//-------------------------------------------------------------------------------------------------
//  
class SFStateGroup
	: public SFObjectWithChildList<IStateGroup, SFState, StateList>
{
public:
	SFStateGroup();
	virtual ~SFStateGroup();

    virtual IStateList * GetStateList();

	// Persistence
	static SFStateGroup * From( IReadBytes * in_pBytes );
	bool To( IWriteBytes * io_pBytes );
};

//-------------------------------------------------------------------------------------------------
//  
class SFSwitch
	: public SFObjectWithID<ISwitch>
{
public:
	SFSwitch();
	virtual ~SFSwitch();

	// Persistence
	static SFSwitch * From( IReadBytes * in_pBytes, BYTE in_bSwitchGroupVersion );
	bool To( IWriteBytes * io_pBytes );
};

//-------------------------------------------------------------------------------------------------
//  
class SFSwitchGroup
	: public SFObjectWithChildList<ISwitchGroup, SFSwitch, SwitchList>
{
public:
	SFSwitchGroup();
	virtual ~SFSwitchGroup();

    virtual ISwitchList * GetSwitchList();

	// Persistence
	static SFSwitchGroup * From( IReadBytes * in_pBytes );
	bool To( IWriteBytes * io_pBytes );
};

//-------------------------------------------------------------------------------------------------
//  
class SFGameObject
	: public SFObjectWithID<IGameObject>
{
public:
	SFGameObject();
	virtual ~SFGameObject();

	// Persistence
	static SFGameObject * From( IReadBytes * in_pBytes );
	bool To( IWriteBytes * io_pBytes );
};

//-------------------------------------------------------------------------------------------------
//  
class SFGameParameter
	: public SFObjectWithID<IGameParameter>
{
public:
	SFGameParameter();
	virtual ~SFGameParameter();

	virtual double RangeMin() const;
	virtual double RangeMax() const;

	void SetRange( const double& in_rMin, const double& in_rMax );

	// Persistence
	static SFGameParameter * From( IReadBytes * in_pBytes );
	bool To( IWriteBytes * io_pBytes );

private:
	double m_dblRangeMin;
	double m_dblRangeMax;
};

//-------------------------------------------------------------------------------------------------
//  
class SFTrigger
	: public SFObjectWithID<ITrigger>
{
public:
	SFTrigger();
	virtual ~SFTrigger();

	// Persistence
	static SFTrigger * From( IReadBytes * in_pBytes );
	bool To( IWriteBytes * io_pBytes );
};

//-------------------------------------------------------------------------------------------------
//  
class SFEnvironment
	: public SFObjectWithID<IEnvironment>
{
public:
	SFEnvironment();
	virtual ~SFEnvironment();

	// Persistence
	static SFEnvironment * From( IReadBytes * in_pBytes );
	bool To( IWriteBytes * io_pBytes );
};