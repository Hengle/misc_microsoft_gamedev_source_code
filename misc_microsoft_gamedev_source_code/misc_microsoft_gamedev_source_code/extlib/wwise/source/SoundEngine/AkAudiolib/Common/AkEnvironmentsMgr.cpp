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
// AkEnvironmentsMgr.cpp
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "AkEnvironmentsMgr.h"
#include "AkAudiolib.h"
#include "AkFXMemAlloc.h"
#include "AkEffectsMgr.h"
#include "AkLEngine.h"
#include "AkURenderer.h"
#include "AkMonitor.h"
#include "AkAudioLibIndex.h"

extern AkMemPoolId g_DefaultPoolId;

//====================================================================================================
//====================================================================================================
AKRESULT CAkEnvironmentsMgr::Init()
{
	m_bEnvironmentalBypass = false;
	
	memset( m_bCurveEnabled, 0, sizeof( m_bCurveEnabled ) );
	return m_FXParameterSetList.Init( AK_MIN_FX_PARAMETER_SET_LIST_SIZE, AK_MAX_FX_PARAMETER_SET_LIST_SIZE, g_DefaultPoolId );
}
//====================================================================================================
//====================================================================================================
AKRESULT CAkEnvironmentsMgr::Term()
{
	for( AkFXParameterSetList::Iterator iter = m_FXParameterSetList.Begin(); iter != m_FXParameterSetList.End(); ++iter )
	{
		if( (*iter).pParam )
			(*iter).pParam->Term( AkFXMemAlloc::GetUpper( ) );
	}

	m_FXParameterSetList.Term();

	for( AkRTPCEnvSubscriptionList::Iterator iter = m_RTPCSubsList.Begin(); iter != m_RTPCSubsList.End(); ++iter )
	{
		(*iter).ConversionTable.Unset();
	}
	m_RTPCSubsList.Term();

	for ( int i=0; i<MAX_CURVE_X_TYPES; ++i )
	{
		for( int j=0; j<MAX_CURVE_Y_TYPES; ++j )
		{
			ConversionTable[i][j].Unset();
		}
	}

	return AK_Success;
}
//====================================================================================================
// Add or Update FXParameterSet in the List
//====================================================================================================
AKRESULT CAkEnvironmentsMgr::AddFXParameterSet(
		AkEnvID			in_FXParameterSetID,	// Preset unique ID
		AkPluginID		in_EffectTypeID,		// Effect unique type ID. 
		void*			in_pvInitParamsBlock,	// Blob
		AkUInt32			in_ulParamBlockSize		// Blob size
		)
{
	AKRESULT l_eResult = RemoveFXParameterSet( in_FXParameterSetID );

	if( l_eResult == AK_Success )
	{
		FXParameterSetParams* l_pFXParameterSetParams = m_FXParameterSetList.AddLast();
		if( !l_pFXParameterSetParams )
		{
			return AK_Fail;
		}
		
		l_pFXParameterSetParams->FXParameterSetID = in_FXParameterSetID;
		l_pFXParameterSetParams->FXID = in_EffectTypeID;
		l_pFXParameterSetParams->bBypassed = false;
		l_pFXParameterSetParams->fVolume = 1.0f;

		l_eResult = CAkEffectsMgr::AllocParams( AkFXMemAlloc::GetUpper(), in_EffectTypeID, l_pFXParameterSetParams->pParam );

		AKASSERT( l_eResult != AK_Success || l_pFXParameterSetParams->pParam != NULL );
		if( l_eResult != AK_Success || l_pFXParameterSetParams->pParam == NULL )
		{
			RemoveFXParameterSet( in_FXParameterSetID );
			// Yes success. We don't want a bank to fail loading because an FX is not registered or not working.
			// This FX will not be available, and will be considered as no FX.
			// A warning message has already been sent to StdErr and to Wwise Log.
			return AK_Success;
		}

		l_eResult = l_pFXParameterSetParams->pParam->Init( AkFXMemAlloc::GetUpper(), in_pvInitParamsBlock, in_ulParamBlockSize );
		AKASSERT( l_eResult == AK_Success );
		if( l_eResult != AK_Success )
		{
			l_pFXParameterSetParams->pParam->Term( AkFXMemAlloc::GetUpper( ) );
			RemoveFXParameterSet( in_FXParameterSetID );
		}
	}
 
	return l_eResult;
}
//====================================================================================================
// Change a specific parameter on a specified ParameterSet
//====================================================================================================
AKRESULT CAkEnvironmentsMgr::SetFXParameterSetParam( 
		AkEnvID			in_FXParameterSetID,		// FXParameterSet unique ID
		AkPluginParamID in_ulParamID,
		void*    		in_pvParamsBlock,
		AkUInt32     	in_ulParamBlockSize
		)
{
	for( AkFXParameterSetList::Iterator iter = m_FXParameterSetList.Begin(); iter != m_FXParameterSetList.End(); ++iter )
	{
		if((*iter).FXParameterSetID == in_FXParameterSetID)
		{
			if( (*iter).pParam )
			{
#ifndef AK_OPTIMIZED
				CAkURenderer::UpdateEnvParam( in_FXParameterSetID, in_ulParamID, in_pvParamsBlock, in_ulParamBlockSize );
#endif
				return (*iter).pParam->SetParam( in_ulParamID, in_pvParamsBlock, in_ulParamBlockSize );
			}
			break;
		}
	}
	return AK_InvalidID;
}
//====================================================================================================
// Remove the specified FXParameterSet from the Preset List
//====================================================================================================
AKRESULT CAkEnvironmentsMgr::RemoveFXParameterSet(
		AkEnvID		in_FXParameterSetID		// FXParameterSet unique ID
		)
{
	for( AkFXParameterSetList::IteratorEx iter = m_FXParameterSetList.BeginEx(); iter != m_FXParameterSetList.End(); ++iter )
	{
		if((*iter).FXParameterSetID == in_FXParameterSetID)
		{
			if ( (*iter).pParam )
				(*iter).pParam->Term( AkFXMemAlloc::GetUpper( ) );
			m_FXParameterSetList.Erase(iter);
			// No notification needed, The Bus should terminate by himself.
			break;
		}
	}
	return AK_Success;
}

AKRESULT CAkEnvironmentsMgr::GetFXParameterSetParams( 
		AkEnvID		in_FXParameterSetID, 
		AkPluginID& out_rEffectTypeID, 
		AK::IAkPluginParam*& out_rpParam
		)
{
	for( AkFXParameterSetList::Iterator iter = m_FXParameterSetList.Begin(); iter != m_FXParameterSetList.End(); ++iter )
	{
		if((*iter).FXParameterSetID == in_FXParameterSetID)
		{
			out_rEffectTypeID = (*iter).FXID;
			out_rpParam = (*iter).pParam;
			return AK_Success;
		}
	}

	// Environment not found, returning no Valid FX
	out_rpParam = NULL;
	out_rEffectTypeID = AK_INVALID_PLUGINID;
	return AK_Fail;
}

AKRESULT CAkEnvironmentsMgr::SetRTPC(
		AkEnvID						in_FXParameterSetID, 
		AkRtpcID					in_RTPC_ID,
		AkRTPC_ParameterID			in_ParamID,
		AkUniqueID					in_RTPCCurveID,
		AkCurveScaling				in_eScaling,
		AkRTPCGraphPoint*			in_pArrayConversion,
		AkUInt32						in_ulConversionArraySize
		)
{
	AKRESULT eResult = AK_Success;

	UnsetRTPC( in_FXParameterSetID, in_ParamID, in_RTPCCurveID );

	AkRTPCEnvSubscription * pSubsItem = m_RTPCSubsList.AddLast();
	if ( pSubsItem )
	{
		pSubsItem->EnvID = in_FXParameterSetID;
		pSubsItem->ParamID = in_ParamID;
		pSubsItem->RTPCCurveID = in_RTPCCurveID;
		pSubsItem->RTPCID = in_RTPC_ID;

		if( in_pArrayConversion && in_ulConversionArraySize )
		{
			eResult = pSubsItem->ConversionTable.Set( in_pArrayConversion, in_ulConversionArraySize, in_eScaling );
		}

#ifndef AK_OPTIMIZED
		CAkURenderer::UpdateEnvRTPC( in_FXParameterSetID, *pSubsItem );
#endif
	}
	else
	{
		eResult = AK_Fail;
	}

	return eResult;
}

AKRESULT CAkEnvironmentsMgr::UnsetRTPC(
	AkEnvID				in_FXParameterSetID, 
	AkRTPC_ParameterID	in_ParamID,
	AkUniqueID			in_RTPCCurveID
	)
{
	AkRTPCEnvSubscriptionList::Iterator iter = m_RTPCSubsList.Begin();
	while( iter != m_RTPCSubsList.End() )
	{
		if( (*iter).EnvID == in_FXParameterSetID && (*iter).ParamID == in_ParamID && (*iter).RTPCCurveID == in_RTPCCurveID )
		{
			(*iter).ConversionTable.Unset();
			m_RTPCSubsList.Erase( iter );
			break;
		}
		else
		{
			++iter;
		}
	}

#ifndef AK_OPTIMIZED
	CAkURenderer::NotifEnvUnsetRTPC( in_FXParameterSetID, in_ParamID, in_RTPCCurveID );
#endif

	return AK_Success;
}

AKRESULT CAkEnvironmentsMgr::SetEnvironmentVolume( 
	AkEnvID		in_FXParameterSetID,	
	AkReal32	in_fVolume				
	)
{
	for( AkFXParameterSetList::Iterator iter = m_FXParameterSetList.Begin(); iter != m_FXParameterSetList.End(); ++iter )
	{
		if((*iter).FXParameterSetID == in_FXParameterSetID)
		{
			(*iter).fVolume = in_fVolume;
			return AK_Success;
		}
	}

	return AK_Fail;
}

AkReal32 CAkEnvironmentsMgr::GetEnvironmentVolume( AkEnvID	in_FXParameterSetID )
{
	for( AkFXParameterSetList::Iterator iter = m_FXParameterSetList.Begin(); iter != m_FXParameterSetList.End(); ++iter )
	{
		if((*iter).FXParameterSetID == in_FXParameterSetID)
		{
			return (*iter).fVolume;
		}
	}

	return 0.0f; //not found???
}

AKRESULT CAkEnvironmentsMgr::BypassEnvironment(
	AkEnvID	in_FXParameterSetID, 
	bool	in_bIsBypassed
	)
{
	for( AkFXParameterSetList::Iterator iter = m_FXParameterSetList.Begin(); iter != m_FXParameterSetList.End(); ++iter )
	{
		if((*iter).FXParameterSetID == in_FXParameterSetID)
		{
			(*iter).bBypassed = in_bIsBypassed;
			CAkLEngine::BypassEnvFx( in_FXParameterSetID, in_bIsBypassed );
			return AK_Success;
		}
	}

	return AK_Fail;
}

bool CAkEnvironmentsMgr::IsBypassed( AkEnvID in_FXParameterSetID )
{
	if( !m_bEnvironmentalBypass )
	{
		for( AkFXParameterSetList::Iterator iter = m_FXParameterSetList.Begin(); iter != m_FXParameterSetList.End(); ++iter )
		{
			if((*iter).FXParameterSetID == in_FXParameterSetID)
			{
				return (*iter).bBypassed;
			}
		}
	}

	// returning true if if m_bEnvironmentalBypass == true or if Env not found
	return true;
}

AKRESULT CAkEnvironmentsMgr::SetObsOccCurve( eCurveXType in_x, eCurveYType in_y, unsigned long in_ulNbPoints, AkRTPCGraphPoint in_paPoints[], AkCurveScaling in_eScaling )
{
	AKASSERT( in_x < MAX_CURVE_X_TYPES );
	AKASSERT( in_y < MAX_CURVE_Y_TYPES );
	return ConversionTable[in_x][in_y].Set( in_paPoints, in_ulNbPoints, in_eScaling );
}

AkReal32 CAkEnvironmentsMgr::GetCurveValue( eCurveXType in_x, eCurveYType in_y, AkReal32 in_value )
{
	AKASSERT( in_x < MAX_CURVE_X_TYPES );
	AKASSERT( in_y < MAX_CURVE_Y_TYPES );
	if( IsCurveEnabled( in_x, in_y ) )
		return ConversionTable[in_x][in_y].Convert( in_value );
	else
	{
		//return default value
		if( in_y == CurveVol )
			return AK_MAXIMUM_VOLUME_LEVEL;
		else if( in_y == CurveLPF )
			return 0.0f;
		else
		{
			AKASSERT(!"Unknown curve type!");
			return 0.0f;
		}
	}
}

