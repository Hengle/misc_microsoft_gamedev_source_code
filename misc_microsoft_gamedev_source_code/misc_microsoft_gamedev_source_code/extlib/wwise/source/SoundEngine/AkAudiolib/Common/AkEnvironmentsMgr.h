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
// AkEnvironmentsMgr.h
//
//////////////////////////////////////////////////////////////////////
#ifndef _ENVIRONMENTS_MGR_H_
#define _ENVIRONMENTS_MGR_H_

#include <AK/SoundEngine/Common/AkTypes.h>
#include <AK/SoundEngine/Common/IAkPlugin.h>

#include "AkList2.h"
#include <AK/Tools/Common/AkLock.h>
#include "AkRTPC.h"
#include "AkConversionTable.h"
#include <AK/Tools/Common/AkArray.h>
//====================================================================================================
//====================================================================================================
// defines for the list of objects the manager knows about
#define	AK_MIN_OBJECTS_LIST_SIZE				(32)
#define	AK_MAX_OBJECTS_LIST_SIZE				(AK_NO_MAX_LIST_SIZE)

// define for the list of environments the manager knows about
#define	AK_MIN_ENVIRONMENTS_LIST_SIZE			(16)
#define	AK_MAX_ENVIRONMENTS_LIST_SIZE			(AK_NO_MAX_LIST_SIZE)

// define for the list of FXParameterSet the manager knows about
#define	AK_MIN_FX_PARAMETER_SET_LIST_SIZE				(16)
#define	AK_MAX_FX_PARAMETER_SET_LIST_SIZE				(AK_NO_MAX_LIST_SIZE)

//====================================================================================================
// FXParameterSet list
//====================================================================================================
struct FXParameterSetParams
{
	AkEnvID			FXParameterSetID;	// Preset unique ID
	AkPluginID		FXID;				// Effect unique ID. 
    AK::IAkPluginParam*	pParam;				// Parameters.
	bool			bBypassed;
	AkReal32		fVolume;
};

struct AkRTPCEnvSubscription
{
	AkEnvID				EnvID;
	AkRtpcID			RTPCID;
	AkUniqueID			RTPCCurveID;
	AkUInt32				ParamID;		//# of the param that must be notified on change
	CAkConversionTable<AkRTPCGraphPoint, AkReal32>	ConversionTable;
};

typedef AkArray<AkRTPCEnvSubscription, const AkRTPCEnvSubscription&, ArrayPoolDefault, 2> AkRTPCEnvSubscriptionList;

//====================================================================================================
// environments manager
//====================================================================================================
class CAkEnvironmentsMgr : public CAkObject
{
public:
	
	// also defined in IWProject.h
	enum eCurveXType { CurveObs, CurveOcc, MAX_CURVE_X_TYPES };
	enum eCurveYType { CurveVol, CurveLPF, MAX_CURVE_Y_TYPES };

	
public:
	AKRESULT Init();
	AKRESULT Term();

//----------------------------------------------------------------------------------------------------
// FXParameterSet list
//----------------------------------------------------------------------------------------------------

	AKRESULT AddFXParameterSet(
		AkEnvID			in_FXParameterSetID,	// FXParameterSet unique ID
		AkPluginID		in_EffectTypeID,		// Effect unique type ID. 
		void*			in_pvInitParamsBlock, 
		AkUInt32			in_ulParamBlockSize
		);

	AKRESULT SetFXParameterSetParam( 
		AkEnvID			in_FXParameterSetID,	// FXParameterSet unique ID
		AkPluginParamID in_ulParamID,
		void*    	 	in_pvParamsBlock,
		AkUInt32    	 	in_ulParamBlockSize
		);

	AKRESULT RemoveFXParameterSet(
		AkEnvID			in_FXParameterSetID		// FXParameterSet unique ID
		);

	AKRESULT GetFXParameterSetParams( 
		AkEnvID			in_FXParameterSetID,	// FXParameterSet unique ID
		AkPluginID& out_rEffectTypeID, 
		AK::IAkPluginParam*& out_rpParam
		);

	AKRESULT SetRTPC(
		AkEnvID						in_FXParameterSetID, 
		AkRtpcID					in_RTPC_ID,
		AkRTPC_ParameterID			in_ParamID,
		AkUniqueID					in_RTPCCurveID,
		AkCurveScaling				in_eScaling,
		AkRTPCGraphPoint*			in_pArrayConversion,
		AkUInt32						in_ulConversionArraySize
		);

	AKRESULT UnsetRTPC(
		AkEnvID				in_FXParameterSetID, 
		AkRTPC_ParameterID	in_ParamID,
		AkUniqueID			in_RTPCCurveID
		);

	AkRTPCEnvSubscriptionList* GetEnvRTPCSubscriptionList(){ return &m_RTPCSubsList; }

	AKRESULT SetEnvironmentVolume( 
		AkEnvID				in_FXParameterSetID,	
		AkReal32			in_fVolume				
		);

	AkReal32 GetEnvironmentVolume( AkEnvID	in_FXParameterSetID );

	AKRESULT BypassEnvironment(
		AkEnvID	in_FXParameterSetID, 
		bool	in_bIsBypassed
		);

	void BypassAllEnv( bool in_bBypass ){ m_bEnvironmentalBypass = in_bBypass; }

	bool IsBypassed( AkEnvID in_FXParameterSetID );

	// Obstruction/Occlusion curves methods
	AKRESULT SetObsOccCurve( eCurveXType in_x, eCurveYType in_y, unsigned long in_ulNbPoints, AkRTPCGraphPoint in_paPoints[], AkCurveScaling in_eScaling );
	AkReal32 GetCurveValue( eCurveXType in_x, eCurveYType in_y, AkReal32 in_value );
	void	 SetCurveEnabled( eCurveXType in_x, eCurveYType in_y, bool in_bEnable ) { m_bCurveEnabled[in_x][in_y] = in_bEnable; };
	bool	 IsCurveEnabled( eCurveXType in_x, eCurveYType in_y ) { return m_bCurveEnabled[in_x][in_y]; };


private:

	// the list of presets we manage
	typedef CAkList2<FXParameterSetParams, const FXParameterSetParams&, AkAllocAndFree>		AkFXParameterSetList;
	AkFXParameterSetList		m_FXParameterSetList;

	AkRTPCEnvSubscriptionList	m_RTPCSubsList;

	bool m_bEnvironmentalBypass;

	bool m_bCurveEnabled[MAX_CURVE_X_TYPES][MAX_CURVE_Y_TYPES];
	CAkConversionTable<AkRTPCGraphPoint, AkReal32>	ConversionTable[MAX_CURVE_X_TYPES][MAX_CURVE_Y_TYPES]; //we use this for our obs/occ curves!
};

extern CAkEnvironmentsMgr* g_pEnvironmentMgr;

#endif
