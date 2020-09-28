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
// AkAttenuationMgr.cpp
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "AkAttenuationMgr.h"
#include "AkBankFloatConversion.h"
#include "AkPBI.h"

extern AkMemPoolId g_DefaultPoolId;

#ifndef AK_OPTIMIZED
CAkLock CAkAttenuation::m_optAttenuationLock;
#endif

//====================================================================================================
//====================================================================================================
// CAkAttenuation
//====================================================================================================
//====================================================================================================

CAkAttenuation* CAkAttenuation::Create(AkUniqueID in_ulID)
{
	CAkAttenuation* pAttenuation = AkNew( g_DefaultPoolId, CAkAttenuation( in_ulID ) );
	if( pAttenuation )
	{
		if( pAttenuation->Init() != AK_Success )
		{
			pAttenuation->Release();
			pAttenuation = NULL;
		}
	}
	return pAttenuation;
}

AKRESULT CAkAttenuation::SetInitialValues( AkUInt8* in_pData, AkUInt32 in_ulDataSize )
{
	AKRESULT eResult = AK_Success;

	AkUniqueID attenuationID = READBANKDATA( AkUInt32, in_pData, in_ulDataSize);

	m_bIsConeEnabled = READBANKDATA( AkUInt8, in_pData, in_ulDataSize ) != 0;
	if( m_bIsConeEnabled )
	{
		m_ConeParams.fInsideAngle	=	AkMath::ToRadians( READBANKDATA( AkReal32, in_pData, in_ulDataSize ) ) * 0.5f;
		m_ConeParams.fOutsideAngle	=	AkMath::ToRadians( READBANKDATA( AkReal32, in_pData, in_ulDataSize ) ) * 0.5f;
		m_ConeParams.fOutsideVolume	=	READBANKDATA( AkReal32, in_pData, in_ulDataSize );
		m_ConeParams.LoPass			=	READBANKDATA( AkReal32, in_pData, in_ulDataSize );
	}
#ifdef _DEBUG
	else
	{// this else could be commented out, but setting to 0 to help debugging
		m_ConeParams.fInsideAngle	=	0;// Invalid / Not required
		m_ConeParams.fOutsideAngle	=	0;// Invalid / Not required
		m_ConeParams.fOutsideVolume	=	0;// Invalid / Not required
		m_ConeParams.LoPass			=	0;// Invalid / Not required
	}
#endif
	for( int i = 0; i < AK_MAX_NUM_ATTENUATION_CURVE; ++i )
	{
		m_curveToUse[i] = READBANKDATA( AkUInt8, in_pData, in_ulDataSize );
	}
	AkUInt32 NumCurves = READBANKDATA( AkUInt32, in_pData, in_ulDataSize );

	AKASSERT( NumCurves );// at least 1 curve is required for volume dry, otherwise the curve assignment logic will be wrong.

	eResult = AK_Fail;
	for( AkUInt32 i = 0; i < NumCurves; ++i )
	{
		// Read curve scaling type (None, dB...)
		AkCurveScaling eScaling	= READBANKDATA( AkCurveScaling, in_pData, in_ulDataSize );
		// ArraySize (number of points in the RTPC curve)
		AkUInt32 ulSize	= READBANKDATA( AkUInt32, in_pData, in_ulDataSize );

		eResult = m_curves[i].Set( (AkRTPCGraphPoint*)in_pData, ulSize, eScaling );
		if( eResult != AK_Success )
			break;

		// Skip the array that was just processed above
		in_pData += ( ulSize*sizeof(AkRTPCGraphPoint) );
		in_ulDataSize -= ( ulSize*sizeof(AkRTPCGraphPoint) );
	}

	AkUInt32 ulNumRTPC = READBANKDATA( AkUInt32, in_pData, in_ulDataSize );

	for(AkUInt32 i = 0; i < ulNumRTPC; ++i)
	{
		AkPluginID l_FXID = READBANKDATA( AkUInt32, in_pData, in_ulDataSize );
		// Should not be an FX since it is a param to be used for positionning parameter on Shared positionning.
		AKASSERT( l_FXID == AK_INVALID_UNIQUE_ID );

		// Read RTPCID
		AkRtpcID l_RTPCID = READBANKDATA(AkUInt32, in_pData, in_ulDataSize);

		// Read ParameterID//ie. Volume, Pitch, LFE...
		AkRTPC_ParameterID l_ParamID = (AkRTPC_ParameterID)READBANKDATA(AkUInt32, in_pData, in_ulDataSize);

		// Read Curve ID
		const AkUniqueID rtpcCurveID = (AkUniqueID)READBANKDATA(AkUInt32, in_pData, in_ulDataSize);

		// Read curve scaling type (None, dB...)
		AkCurveScaling eScaling = READBANKDATA(AkCurveScaling, in_pData, in_ulDataSize);
		// ArraySize //i.e.:number of conversion points
		AkUInt32 ulSize = READBANKDATA(AkUInt32, in_pData, in_ulDataSize);

		eResult = SetRTPC( l_FXID, l_RTPCID, l_ParamID, rtpcCurveID, eScaling, (AkRTPCGraphPoint*)in_pData, ulSize );
		if( eResult != AK_Success )
			break;

		//Skipping the Conversion array
		in_pData += ( ulSize*sizeof(AkRTPCGraphPoint) );
		in_ulDataSize -= ( ulSize*sizeof(AkRTPCGraphPoint) );
	}

	return eResult;
}

AKRESULT CAkAttenuation::SetAttenuationParams( AkWwiseAttenuation& in_rParams )
{
	AKRESULT eResult = AK_Fail;

	m_bIsConeEnabled			=   in_rParams.Cone.bIsConeEnabled ? true : false;
	m_ConeParams.fInsideAngle	=	AkMath::ToRadians( in_rParams.Cone.cone_fInsideAngle ) * 0.5f;
	m_ConeParams.fOutsideAngle	=	AkMath::ToRadians( in_rParams.Cone.cone_fOutsideAngle ) * 0.5f;
	m_ConeParams.fOutsideVolume	=	in_rParams.Cone.cone_fOutsideVolume;
	m_ConeParams.LoPass			=	in_rParams.Cone.cone_LoPass;

	for( int i = 0; i < AK_MAX_NUM_ATTENUATION_CURVE; ++i )
	{
		m_curveToUse[i] = in_rParams.CurveIndexes[i];
	}

	AKASSERT( in_rParams.uNumCurves );// at least 1 curve is required for volume dry, otherwise the curve assignment logic will be wrong.

	for( AkUInt32 i = 0; i < in_rParams.uNumCurves; ++i )
	{
		eResult = m_curves[i].Set( in_rParams.paCurves[i].m_pArrayConversion, in_rParams.paCurves[i].m_ulConversionArraySize, in_rParams.paCurves[i].m_eScaling );
		if( eResult != AK_Success )
			break;
	}

	ClearRTPCs();
	for( AkUInt32 i = 0; i < in_rParams.uNumRTPCReg; ++i )
	{
		// Should not be an FX since it is a param to be used for positionning parameter on Shared positionning.
		AKASSERT( in_rParams.paRTPCReg[i].m_FXID == AK_INVALID_UNIQUE_ID );

		eResult = SetRTPC(	in_rParams.paRTPCReg[i].m_FXID,
							in_rParams.paRTPCReg[i].m_RTPCID, 
							in_rParams.paRTPCReg[i].m_paramID, 
							in_rParams.paRTPCReg[i].m_RTPCCurveID, 
							in_rParams.paRTPCReg[i].m_eScaling, 
							in_rParams.paRTPCReg[i].m_pArrayConversion, 
							in_rParams.paRTPCReg[i].m_ulConversionArraySize );

		if( eResult != AK_Success )
			break;
	}

#ifndef AK_OPTIMIZED
	UpdatePBIs();
#endif

	return eResult;
}

AKRESULT CAkAttenuation::Init()
{ 
	for( int i = 0; i < AK_MAX_NUM_ATTENUATION_CURVE; ++i )
	{
		m_curveToUse[i] = -1;
		m_curves[i].m_pArrayGraphPoints = NULL;
	}

	AddToIndex(); 
	return AK_Success; 
}

CAkAttenuation::~CAkAttenuation()
{
	ClearRTPCs();
	m_ListFXRTPCSubscriptions.Term();

	for( int i = 0; i < AK_MAX_NUM_ATTENUATION_CURVE; ++i )
	{
		m_curves[i].Unset();
	}

#ifndef AK_OPTIMIZED
	m_PBIList.Term();
#endif
}

void CAkAttenuation::ClearRTPCs()
{
	for( AkRTPCFXSubscriptionList::Iterator iter = m_ListFXRTPCSubscriptions.Begin(); iter != m_ListFXRTPCSubscriptions.End(); ++iter )
	{
		(*iter).ConversionTable.Unset();
	}
	m_ListFXRTPCSubscriptions.RemoveAll();
}

AkRTPCFXSubscriptionList* CAkAttenuation::GetRTPCSubscriptionList()
{
	return &m_ListFXRTPCSubscriptions;
}

#ifndef AK_OPTIMIZED

AKRESULT CAkAttenuation::AddPBI( CAkPBI* in_pPBI )
{
	return m_PBIList.AddLast( in_pPBI ) ? AK_Success : AK_Fail;
}

void CAkAttenuation::RemovePBI( CAkPBI* in_pPBI )
{
	m_PBIList.Remove( in_pPBI );
}

void CAkAttenuation::UpdatePBIs()
{
	for( PBIArray::Iterator iter = m_PBIList.Begin(); iter != m_PBIList.End(); ++iter )
	{
		CAkPBI* pPBI = (*iter);
		pPBI->UpdateAttenuationInfo();
	}
}
#endif

void CAkAttenuation::AddToIndex()
{
	AKASSERT(g_pIndex);
	g_pIndex->m_idxAttenuations.SetIDToPtr( this );
}

void CAkAttenuation::RemoveFromIndex()
{
	AKASSERT(g_pIndex);
	g_pIndex->m_idxAttenuations.RemoveID( ID() );
}

AkUInt32 CAkAttenuation::AddRef() 
{ 
	AkAutoLock<CAkLock> IndexLock( g_pIndex->m_idxAttenuations.GetLock() ); 
    return ++m_lRef; 
} 

AkUInt32 CAkAttenuation::Release() 
{ 
	AkAutoLock<CAkLock> IndexLock( g_pIndex->m_idxAttenuations.GetLock() ); 
    AkInt32 lRef = --m_lRef; 
    AKASSERT( lRef >= 0 ); 
    if ( !lRef ) 
    { 
        RemoveFromIndex();
        AkDelete( g_DefaultPoolId, this ); 
    } 
    return lRef; 
}

AKRESULT CAkAttenuation::SetRTPC(
		AkPluginID					in_FXID,
		AkRtpcID					in_RTPC_ID,
		AkRTPC_ParameterID			in_ParamID,
		AkUniqueID					in_RTPCCurveID,
		AkCurveScaling				in_eScaling,
		AkRTPCGraphPoint*			in_pArrayConversion,
		AkUInt32					in_ulConversionArraySize
		)
{
	AKRESULT eResult = AK_Success;

	AKASSERT( in_FXID == AK_INVALID_UNIQUE_ID );
	UnsetRTPC( in_FXID, in_ParamID, in_RTPCCurveID );

	AkRTPCFXSubscription * pSubsItem = m_ListFXRTPCSubscriptions.AddLast();
	if ( pSubsItem )
	{
		pSubsItem->FXID = in_FXID;
		pSubsItem->ParamID = in_ParamID;
		pSubsItem->RTPCCurveID = in_RTPCCurveID;
		pSubsItem->RTPCID = in_RTPC_ID;

		if( in_pArrayConversion && in_ulConversionArraySize )
		{
			eResult = pSubsItem->ConversionTable.Set( in_pArrayConversion, in_ulConversionArraySize, in_eScaling );
		}
	}
	else
	{
		eResult = AK_Fail;
	}


	return eResult;
}

AKRESULT CAkAttenuation::UnsetRTPC(
	AkPluginID in_FXID,
	AkRTPC_ParameterID	in_ParamID,
	AkUniqueID in_RTPCCurveID
	)
{
	AKASSERT( in_FXID == AK_INVALID_UNIQUE_ID );
	AkRTPCFXSubscriptionList::Iterator iter = m_ListFXRTPCSubscriptions.Begin();
	while( iter != m_ListFXRTPCSubscriptions.End() )
	{
		if( (*iter).FXID == in_FXID && (*iter).ParamID == in_ParamID && (*iter).RTPCCurveID == in_RTPCCurveID )
		{
			(*iter).ConversionTable.Unset();
			iter = m_ListFXRTPCSubscriptions.Erase( iter );
		}
		else
		{
			++iter;
		}
	}

	return AK_Success;
}


