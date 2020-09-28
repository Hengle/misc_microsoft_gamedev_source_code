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
// AkAttenuationMgr.h
//
// creator : alessard
//
//////////////////////////////////////////////////////////////////////
#ifndef _ATTENUATION_MGR_H_
#define _ATTENUATION_MGR_H_

#include <AK/SoundEngine/Common/AkTypes.h>
#include "AkKeyArray.h"
#include "AkConversionTable.h"
#include "AkIndexable.h"
#include "AkAudioLibIndex.h"
#include "Ak3DParams.h"
#include "AkParameterNodeBase.h"

class CAkPBI;

#define AK_MAX_NUM_ATTENUATION_CURVE 5 // TODO : Add a shared file between WWise define and SE define, so they don't get duplicated.

enum AttenuationCurveID
{
	AttenuationCurveID_VolumeDry	= 0,
	AttenuationCurveID_VolumeWet	= 1,
	AttenuationCurveID_LFE			= 2,
	AttenuationCurveID_LowPassFilter= 3,
	AttenuationCurveID_Spread		= 4,

	AttenuationCurveID_None			= 255,// was -1, but PS3 was complaining
};

struct AkWwiseConeAttenuation
{
	AkUInt8				bIsConeEnabled;		//as bool
	AkReal32			cone_fInsideAngle;
	AkReal32			cone_fOutsideAngle;
	AkReal32			cone_fOutsideVolume;
	AkLPFType			cone_LoPass;
};

struct AkWwiseGraphCurve
{
	AkCurveScaling m_eScaling;
	AkUInt32 m_ulConversionArraySize;
	AkRTPCGraphPoint* m_pArrayConversion;
};

struct AkWwiseRTPCreg
{
	AkPluginID m_FXID;
	AkRtpcID m_RTPCID;
	AkRTPC_ParameterID m_paramID;
	AkUniqueID m_RTPCCurveID;
	AkCurveScaling m_eScaling;
	AkRTPCGraphPoint* m_pArrayConversion;
	AkUInt32 m_ulConversionArraySize;
};

struct AkWwiseAttenuation
{
	AkWwiseConeAttenuation	Cone;

	AkUInt8					CurveIndexes[ AK_MAX_NUM_ATTENUATION_CURVE ];
	AkUInt32				uNumCurves;
	AkWwiseGraphCurve*		paCurves;

	AkUInt32				uNumRTPCReg;
	AkWwiseRTPCreg*			paRTPCReg;
};

class CAkAttenuation : public CAkIndexable
{

public:

	CAkAttenuation( AkUniqueID in_ulID ):CAkIndexable( in_ulID ){}
	~CAkAttenuation();

	//Thread safe version of the constructor
	static CAkAttenuation* Create( AkUniqueID in_ulID = 0 );

	AKRESULT SetInitialValues( AkUInt8* pData, AkUInt32 ulDataSize );
	AKRESULT SetAttenuationParams( AkWwiseAttenuation& in_rParams );

	///////////////////////////
	// Internal use only
	virtual AkUInt32 AddRef();
	virtual AkUInt32 Release();
	///////////////////////////

	typedef CAkConversionTable<AkRTPCGraphPoint, AkReal32> AkAttenuationCurve;

	// If no curve is available for the selected index(none) the function returns NULL.
	AkAttenuationCurve* GetCurve( AkUInt8 in_parameterIndex )
	{
		AKASSERT( in_parameterIndex < AK_MAX_NUM_ATTENUATION_CURVE );

		// Get the curve selected based on the index
		AkUInt8 l_crveToUse = m_curveToUse[in_parameterIndex];

		return ( l_crveToUse != AttenuationCurveID_None ) ? &m_curves[l_crveToUse] : NULL;
	}

	AkRTPCFXSubscriptionList* GetRTPCSubscriptionList();

#ifndef AK_OPTIMIZED
	AKRESULT AddPBI( CAkPBI* in_pPBI );
	void RemovePBI( CAkPBI* in_pPBI );
#endif

	// public members
	bool m_bIsConeEnabled;

	ConeParams m_ConeParams;

	AkUInt8				m_curveToUse[ AK_MAX_NUM_ATTENUATION_CURVE ];
	AkAttenuationCurve	m_curves[ AK_MAX_NUM_ATTENUATION_CURVE ];

protected:
	AKRESULT Init();

private:

	void ClearRTPCs();
#ifndef AK_OPTIMIZED
	void UpdatePBIs();
#endif

	// Adds the Attenuation in the General index
	void AddToIndex();

	// Removes the Attenuation from the General index
	void RemoveFromIndex();

	AKRESULT SetRTPC(
		AkPluginID					in_FXID,		// If invalid, means that the RTPC is directly on a parameter of this object
		AkRtpcID					in_RTPC_ID,
		AkRTPC_ParameterID			in_ParamID,
		AkUniqueID					in_RTPCCurveID,
		AkCurveScaling				in_eScaling,
		AkRTPCGraphPoint*			in_pArrayConversion,		// NULL if none
		AkUInt32					in_ulConversionArraySize	// 0 if none
		);

	AKRESULT UnsetRTPC(
		AkPluginID in_FXID,
		AkRTPC_ParameterID	in_ParamID,
		AkUniqueID in_RTPCCurveID
		);

	AkRTPCFXSubscriptionList m_ListFXRTPCSubscriptions;

#ifndef AK_OPTIMIZED
	typedef AkArray<CAkPBI*, CAkPBI*, ArrayPoolDefault, DEFAULT_POOL_BLOCK_SIZE/sizeof( CAkPBI* )> PBIArray;
	PBIArray m_PBIList;

	static CAkLock m_optAttenuationLock;

	// Since The lower engine is not acquiring the main global lock while performing the pipeline, 
	// we need a separate lock to handle attenuation changes from Wwise.
	// This lock is not required for banks, and not in release.
	// This is due to the fact that Attenuation curves are not copied and now act as being shared
	// objects. So the version modified by Wwise is the same memory bject than the one being used for playback.
public:
	static void AcquireAttenuationLock(){ m_optAttenuationLock.Lock(); }
	static void ReleaseAttenuationLock(){ m_optAttenuationLock.Unlock(); }
#else
public:
	static void AcquireAttenuationLock(){};
	static void ReleaseAttenuationLock(){};
#endif
};

#endif // _ATTENUATION_MGR_H_
