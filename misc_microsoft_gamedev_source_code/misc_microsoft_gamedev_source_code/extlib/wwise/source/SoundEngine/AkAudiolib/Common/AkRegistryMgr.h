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
// AkRegistryMgr.h
//
//////////////////////////////////////////////////////////////////////
#ifndef _REGISTRY_MGR_H_
#define _REGISTRY_MGR_H_

#include "AkHashList.h"
#include <AK/Tools/Common/AkLock.h>
#include "AkRegisteredObj.h"

class CAkAudioNode;
class CAkPBI;
struct AkSoundPositionEntry;

//CAkRegistryMgr Class
//Unique container per platform containing the registered objects info
class CAkRegistryMgr : public CAkObject
{
	friend class CAkRegisteredObj;

public:

	typedef CAkList2<AkUniqueID, AkUniqueID, AkAllocAndFree> AkListNode;
	// Constructor
	CAkRegistryMgr();

	// Destructor
	virtual ~CAkRegistryMgr();

	AKRESULT	Init();
	void		Term();

	// Register a game object
	CAkRegisteredObj* RegisterObject(
		AkGameObjectID in_GameObjectID,	//GameObject (Ptr) to register
		void * in_pMonitorData = NULL
		);

	// Unregister the specified game object
	void UnregisterObject(
		AkGameObjectID in_GameObjectID	//Game Object to unregister
		);

	// Set the current position of a game object
	AKRESULT SetPosition( 
		AkGameObjectID in_GameObjectID, 
		const AkSoundPosition & in_Position,
		AkUInt32 in_ulListenerIndex
		);

	AKRESULT SetActiveListeners(
		AkGameObjectID in_GameObjectID,	///< Game object.
		AkUInt32 in_uListenerMask		///< Bitmask representing active listeners. LSB = Listener 0, set to 1 means active.
		);

#ifdef RVL_OS
	AKRESULT SetActiveControllers(
		AkGameObjectID in_GameObjectID,			///< Game object.
		AkUInt32 in_uActiveControllerMask		///< Bitmask representing active controllers. LSB = Controller 0, set to 1 means active.
		);
#endif

	AKRESULT GetPosition( 
		AkGameObjectID in_GameObjectID, 
		AkSoundPositionEntry & out_Position
		);

	AKRESULT SetGameObjectEnvironmentsValues( 
		AkGameObjectID		in_GameObjectID,
		AkEnvironmentValue*	in_aEnvironmentValues,
		AkUInt32			in_uNumEnvValues
		);

	AKRESULT SetGameObjectDryLevelValue( 
		AkGameObjectID		in_GameObj,
		AkReal32			in_fControlValue
		);

	AKRESULT SetObjectObstructionAndOcclusion(  
		AkGameObjectID in_GameObjectID,     ///< Game object ID.
		AkUInt32 in_uListener,             ///< Listener ID.
		AkReal32 in_fObstructionLevel,		///< ObstructionLevel : [0.0f..1.0f]
		AkReal32 in_fOcclusionLevel			///< OcclusionLevel   : [0.0f..1.0f]
		);

	AkSwitchHistItem GetSwitchHistItem( 
		CAkRegisteredObj *	in_pGameObj,
		AkUniqueID          in_SwitchContID
		);

	AKRESULT SetSwitchHistItem( 
		CAkRegisteredObj *	in_pGameObj,
		AkUniqueID          in_SwitchContID,
		const AkSwitchHistItem &  in_SwitchHistItem
		);

	AKRESULT ClearSwitchHist( 
		AkUniqueID          in_SwitchContID,
		CAkRegisteredObj *	in_pGameObj = NULL
		);

    CAkRegisteredObj * GetObjAndAddref( AkGameObjectID in_GameObjectID );

	CAkRegisteredObj* ActivatePBI(
		CAkPBI * in_pPBI,			// PBI to set as active
		AkGameObjectID in_GameObjectID	//Game object associated to the PBI
		);

	// Signify to the registry that the specified AudioNode is containing specific information
	void SetNodeIDAsModified(
		CAkAudioNode* in_pNode		//Audionode Modified
		);

	// Unregister all the objects registered in the registry.
	// It also removes all the specific memory allocated for specific parameters.
	void UnregisterAll();

	// Utility returning all the Modified Elements existing for object specific
	void GetAllObjectModifiedElements(
		AkListNode& io_rlistNode
		);

	// Utility returning all the Modified Elements existing
	void GetAllModifiedElements(
		AkListNode& io_rlistNode
		);

	void PostEnvironmentStats();
	void PostObsOccStats();
	void PostListenerStats();
	void PostfeedbackGameObjStats();
#ifdef RVL_OS
	void PostControllerStats();
#endif

	//inline, for profiling purpose only, no lock required since this information is not time critical
	AkUInt32 NumRegisteredObject(){ return m_mapRegisteredObj.Length(); }
	
private:
	AkListNode m_listModifiedNodes;

	typedef AkHashList< AkGameObjectID, CAkRegisteredObj*, 31 > AkMapRegisteredObj;
	AkMapRegisteredObj m_mapRegisteredObj; //map of all actually registered objects
};

extern CAkRegistryMgr* g_pRegistryMgr;

#endif
