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
// AkLayer.h
//
// Declaration of the CAkLayer class, which represents a Layer
// in a Layer Container (CAkLayerCntr)
//
//////////////////////////////////////////////////////////////////////

#ifndef _AKLAYER_H_
#define _AKLAYER_H_

#include "AkIndexable.h"
#include "Ak3DParams.h"
#include "AkRegisteredObj.h"
#include "AkMutedMap.h"
#include "AkAudioNode.h"
#include "AkBitArray.h"
#include "AkConversionTable.h"

class CAkParameterNode;
class CAkLayerCntr;

// Represents a Layer in a Layer Container (CAkLayerCntr)
class CAkLayer
	: public CAkIndexable
{
public:

	// Thread safe version of the constructor
	static CAkLayer* Create( AkUniqueID in_ulID = 0 );

	AKRESULT Init();

	virtual AkUInt32 AddRef();
	virtual AkUInt32 Release();

	// Read data from Soundbank
	AKRESULT SetInitialValues( AkUInt8*& io_rpData, AkUInt32& io_rulDataSize );

	// Get audio parameters for the specified associated child
	AKRESULT GetAudioParameters(
		CAkParameterNode*	in_pAssociatedChild,	// Associated child making the call
		AkSoundParams		&io_Parameters,			// Set of parameter to be filled
		AkUInt32				in_ulParamSelect,		// Bitfield of the list of parameters to retrieve
		AkMutedMap&			io_rMutedMap,			// Map of muted elements
		CAkRegisteredObj*	in_GameObjPtr			// Game object associated to the query
	);

	// Set RTPC info
	void SetRTPC(
		AkRtpcID					in_RTPC_ID,
		AkRTPC_ParameterID			in_ParamID,
		AkUniqueID					in_RTPCCurveID,
		AkCurveScaling				in_eScaling,
		AkRTPCGraphPoint*			in_pArrayConversion,		// NULL if none
		AkUInt32						in_ulConversionArraySize	// 0 if none
	);

	// Clear RTPC info
	AKRESULT UnsetRTPC( AkRTPC_ParameterID in_ParamID, AkUniqueID in_RTPCCurveID );

	// Called by the RTPCMgr when the value of a Game Parameter changes
	AKRESULT SetParamComplexFromRTPCManager( 
		void * in_pToken,
		AkUInt32 in_Param_id, 
		AkRtpcID in_RTPCid,
		AkReal32 in_value, 
		CAkRegisteredObj * in_GameObj = NULL,
		void* in_pGameObjExceptArray = NULL // Actually a GameObjExceptArray pointer
	);

	// Notify associated objects that they must RecalcNotification()
	void RecalcNotification();

	// Is any of the associated objects currently playing?
	bool IsPlaying() const;

	// Set information about an associated object
	AKRESULT SetChildAssoc(
		AkUniqueID in_ChildID,
		AkRTPCCrossfadingPoint* in_pCrossfadingCurve,	// NULL if none
		AkUInt32 in_ulCrossfadingCurveSize				// 0 if none
	);

	// Clear information about an associated object
	AKRESULT UnsetChildAssoc(
		AkUniqueID in_ChildID 
	);

	// Can the specified object be associated to this layer?
	AKRESULT CanAssociateChild( CAkAudioNode * in_pAudioNode );

	// Set this Layer's owner (NULL to clear it)
	void SetOwner( CAkLayerCntr* in_pOwner );

	// If the layer is associated to the specified child, get
	// the pointer to that child. This is called when a child
	// is added to the container, so child-layer associations can
	// be made on the fly.
	void UpdateChildPtr( AkUniqueID in_ChildID );

	// If the layer is associated to the specified child, clear
	// the pointer to the child. This is called when the child
	// is being removed.
	void ClearChildPtr( AkUniqueID in_ChildID );

	// Set the Unique ID of the Game Parameter to be used for crossfading.
	// 0 means there's no crossfading.
	AKRESULT SetCrossfadingRTPC( AkRtpcID in_rtpcID );

	// Set the default value of the crossfading game parameter. This value
	// is used when the Layer needs to evaluate a crossfading curve for
	// a game object that does not have a value for the crossfading game parameter.
	void SetCrossfadingRTPCDefaultValue( AkReal32 in_fValue );

public:

	// Called by the AkRTPCMgr() when our crossfading Game Parameter (m_crossfadingRTPCID)
	// moves. If in_GameObjPtr is NULL then it's the global value of the Game Parameter
	// that has changed, otherwise it's the value for that Game Object.
	void OnRTPCChanged( CAkRegisteredObj * in_GameObjPtr, AkReal32 in_fValue );

private:

	// Constructor/Destructor
    CAkLayer( AkUniqueID in_ulID );
    virtual ~CAkLayer(void);

	// Indexing
	void AddToIndex();
	void RemoveFromIndex();

	// Notify the Children PBIs that a VOLUME variation occured
	void VolumeNotification(
		AkVolumeValue in_Volume,									// Volume variation
		CAkRegisteredObj * in_pGameObj = NULL,	// Target Game Object
		void* in_pExceptArray = NULL
		);

	// Notify the Children PBIs that a PITCH variation occured
	void PitchNotification(
		AkPitchValue in_Pitch,										// Pitch variation
		CAkRegisteredObj * in_pGameObj = NULL,		// Target Game Object
		void* in_pExceptArray = NULL
		);

	// Notify the Children PBIs that a LPF variation occured
	void LPFNotification(
		AkLPFType in_LPF,										// LPF variation
		CAkRegisteredObj * in_pGameObj = NULL,	// Target Game Object
		void* in_pExceptArray = NULL
		);
		
	// Notify the Children PBIs that a LFE variation occured
	void LFENotification(
		AkVolumeValue in_LFE,									// LFE variation
		CAkRegisteredObj * in_pGameObj = NULL,	// Target Game Object
		void* in_pExceptArray = NULL
		);

	void ParamNotification( NotifParams& in_rParams );

	// Read RTPC data from Soundbank
	AKRESULT SetInitialRTPC( AkUInt8*& io_rpData, AkUInt32& io_rulDataSize );

	// Object representing a child-layer association
	class CAssociatedChildData
	{
	public:

		// Constructor/Destructor
		CAssociatedChildData();
		~CAssociatedChildData();

		// Init/Term
		AKRESULT Init( CAkLayer* in_pLayer, AkUniqueID in_ulAssociatedChildID );
		AKRESULT Term( CAkLayer* in_pLayer );

		// If the child pointer is NULL, try to get it and
		// associate the layer to the child.
		AKRESULT UpdateChildPtr( CAkLayer* in_pLayer );

		// If the child pointer is not NULL, dissociate the
		// layer from the child and nullify the pointer
		AKRESULT ClearChildPtr( CAkLayer* in_pLayer );

		// Data
		AkUniqueID m_ulChildID;
		CAkParameterNode* m_pChild;
		CAkConversionTable<AkRTPCCrossfadingPoint, AkReal32> m_fadeCurve;
	};

	// This layer's associations
	typedef CAkKeyArray<AkUniqueID, CAssociatedChildData> AssociatedChildMap;
	AssociatedChildMap m_assocs;

	// Parameters with RTPCs have their bit set to 1 in this array
	CAkBitArrayMax32 m_RTPCBitArrayMax32;

	// The container that owns this layer
	CAkLayerCntr* m_pOwner;

	// Game Parameter to use for crossfading
	AkRtpcID m_crossfadingRTPCID;

	// Value to use for a Game Parameter when a Game Object does not
	// have a specific value (i.e. when CAkRTPCMgr::GetRTPCValue() fails)
	AkReal32 m_crossfadingRTPCDefaultValue;
};

#endif // _AKLAYER_H_
