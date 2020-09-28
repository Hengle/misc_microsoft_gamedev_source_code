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
// AkAudioLibIndex.h
//
// Class containing the maps allowing to make the link between 
// string to IDs and between IDs to pointers.
//
//////////////////////////////////////////////////////////////////////
#ifndef _AUDIOLIB_INDEX_H_
#define _AUDIOLIB_INDEX_H_

#include "AkHashList.h"
#include <AK/Tools/Common/AkLock.h>
#include <AK/Tools/Common/AkAutoLock.h>
#include "AkIndexable.h"
#include "AkIDStringMap.h"

class CAkAudioNode;
class CAkState;
class CAkEvent;
class CAkAction;
class CAkLayer;
class CAkAttenuation;
class CAkDynamicSequence;
class CAkDialogueEvent;

template <class U_PTR> class CAkIndexItem
{
	friend class CAkStateMgr;
	friend class CAkAudioLibIndex;
	friend class CAkActionActive;
	friend class CAkBankMgr;
	friend class AkMonitor;

public:

	//Set an ID corresponding to a pointer
	void SetIDToPtr( U_PTR in_Ptr )
	{
		AkAutoLock<CAkLock> IndexLock( m_IndexLock );
		AKASSERT( in_Ptr );
		m_mapIDToPtr.Set( in_Ptr );
	}

	//Remove an ID from the index
	void RemoveID( AkUniqueID in_ID )
	{
		AkAutoLock<CAkLock> IndexLock( m_IndexLock );
		m_mapIDToPtr.Unset( in_ID );
	}

	U_PTR GetPtrAndAddRef( AkUniqueID in_ID ) 
    { 
		AkAutoLock<CAkLock> IndexLock( m_IndexLock ); 
		CAkIndexable * pIndexable = m_mapIDToPtr.Exists( in_ID ); 
		if( pIndexable ) 
		{ 
			pIndexable->AddRefUnsafe(); 
			return static_cast<U_PTR>( pIndexable );
		} 
		else 
		{ 
			return NULL; 
		} 
    } 


	AKRESULT Init()
	{
		return m_mapIDToPtr.Init();
	}

	void Term()
	{
		//If this assert pop, that mean that a Main ref-counted element of the audiolib was not properly released
		AKASSERT( m_mapIDToPtr.Length() == 0 );
		m_mapIDToPtr.Term();
	}

	CAkLock& GetLock() { return m_IndexLock; }

	typedef AkHashListBare<AkUniqueID, CAkIndexable, 31> AkMapIDToPtr;

private:

	CAkLock			m_IndexLock;
	AkMapIDToPtr	m_mapIDToPtr;
};

class CAkIndexSiblingItem
{

public:

	AKRESULT Init();
	void Term();

	//Set an ID corresponding to a pointer
	AKRESULT SetIDToPtr( AkStateGroupID in_StateGroupID, CAkState* in_Ptr );

	//Remove an ID from the index
	void RemoveID( AkStateGroupID in_StateGroupID, AkUniqueID in_ID );

	CAkState* GetPtrAndAddRef( AkStateGroupID in_StateGroupID, AkUniqueID in_ID );

	CAkLock& GetLock() { return m_IndexLock; }

private:
	
	CAkIndexItem<CAkState*>* GetStateGroup( AkStateGroupID in_StateGroupID );
	CAkIndexItem<CAkState*>* GetOrCreateStateGroup( AkStateGroupID in_StateGroupID );

	typedef CAkKeyArray< AkStateGroupID, CAkIndexItem<CAkState*>*, 4 > AkMapSibling;

	CAkLock			m_IndexLock;
	AkMapSibling	m_ArrayStateGroups;
};

// Class containing the maps allowing to make the link between 
// string to IDs and between IDs to pointers.
//
// Author:  alessard
class  CAkAudioLibIndex : public CAkObject
{

public:
	//Singleton instanciation
	static CAkAudioLibIndex* Instance();

    static void Destroy();

	AKRESULT	Init();
	void		Term();

#ifndef AK_OPTIMIZED
	AKRESULT ResetRndSeqCntrPlaylists();
#endif

public:
	CAkIndexItem<CAkAudioNode*> m_idxAudioNode;	// AudioNodes index

	CAkIndexSiblingItem		m_idxStates;		// States index
	CAkIndexItem<CAkState*> m_idxCustomStates;	// Custom States index

	CAkIndexItem<CAkEvent*> m_idxEvents;		// Events index
	CAkIndexItem<CAkAction*> m_idxActions;		// Actions index

	CAkIndexItem<CAkLayer*> m_idxLayers;		// Layers index

	CAkIndexItem<CAkAttenuation*> m_idxAttenuations;// Actions index

	CAkIndexItem<CAkDynamicSequence*> m_idxDynamicSequences; // Dynamic Sequence index
	CAkIndexItem<CAkDialogueEvent*> m_idxDialogueEvents; // Dynamic Sequence index

private:
	static CAkAudioLibIndex* pInstance;
};

extern CAkAudioLibIndex* g_pIndex;

#endif
