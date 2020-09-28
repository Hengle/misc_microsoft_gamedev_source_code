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
// AkGen3DParams.cpp
//
// alessard
//
//////////////////////////////////////////////////////////////////////
#include "StdAfx.h"
#include <AK/Tools/Common/AkAutoLock.h>
#include "AkGen3DParams.h"
#include "AkMath.h"
#include "AkPathManager.h"
#include "AkDefault3DParams.h"
#include <AK/Tools/Common/AkPlatformFuncs.h>

extern AkMemPoolId g_DefaultPoolId;

#ifndef AK_OPTIMIZED
CAkLock	CAkGen3DParams::m_csLock;		// Static lock required because this class is copied.
#endif

Gen3DParams::Gen3DParams()
	:m_pAttenuation( NULL )
{
}

Gen3DParams::~Gen3DParams()
{
	if( m_pAttenuation ) 
		m_pAttenuation->Release();
}

CAkGen3DParams::CAkGen3DParams()
{
	m_Params.m_ID						= AK_INVALID_UNIQUE_ID;
	m_Params.m_eType					= AkUndefined;
	m_Params.m_uAttenuationID			= AK_INVALID_UNIQUE_ID;
	m_Params.m_bIsSpatialized			= false;

	m_Params.m_bIsDynamic				= true;

	m_Params.m_ePathMode				= AkStepSequence;
	m_Params.m_bIsLooping				= false;
	m_Params.m_TransitionTime			= 0;

	m_Params.m_Position					= g_DefaultSoundPosition.Position;
	m_Params.m_bIsPanningFromRTPC		= false;
	m_Params.m_pArrayVertex				= NULL;
	m_Params.m_ulNumVertices			= 0;
	m_Params.m_pArrayPlaylist			= NULL;
	m_Params.m_ulNumPlaylistItem		= 0;
}

CAkGen3DParams::~CAkGen3DParams()
{
}

void CAkGen3DParams::Term()
{
	ClearPaths();
}

void CAkGen3DParams::ClearPaths()
{
	Lock();
	if(m_Params.m_pArrayVertex)
	{
		AkFree( g_DefaultPoolId, m_Params.m_pArrayVertex );
		m_Params.m_pArrayVertex = NULL;
	}
	if(m_Params.m_pArrayPlaylist)
	{
		AkFree( g_DefaultPoolId, m_Params.m_pArrayPlaylist );
		m_Params.m_pArrayPlaylist = NULL;
	}
	m_Params.m_ulNumVertices = 0;
	m_Params.m_ulNumPlaylistItem = 0;
	Unlock();
}

AKRESULT CAkGen3DParams::SetPositioningType( AkPositioningType in_ePosType )
{
	Lock();
	m_Params.m_eType = in_ePosType;
	Unlock();
	return AK_Success;
}

AKRESULT CAkGen3DParams::SetPosition(const AkVector &in_rPosition)
{
	Lock();
	m_Params.m_Position = in_rPosition;
	Unlock();
	return AK_Success;
}
AKRESULT CAkGen3DParams::SetConeUsage( bool in_bIsConeEnabled )
{
	Lock();
	m_Params.m_bIsConeEnabled = in_bIsConeEnabled;
	Unlock();
	return AK_Success;
}

AKRESULT CAkGen3DParams::SetSpatializationEnabled( bool in_bIsSpatializationEnabled )
{
	Lock();
	m_Params.m_bIsSpatialized = in_bIsSpatializationEnabled;
	Unlock();
	return AK_Success;
}

AKRESULT CAkGen3DParams::SetAttenuationID( AkUniqueID in_AttenuationID )
{
	Lock();
	m_Params.m_uAttenuationID = in_AttenuationID;
	Unlock();
	return AK_Success;
}

/*Setting cone params*/
AKRESULT CAkGen3DParams::SetConeInsideAngle( AkReal32 in_fInsideAngle )
{
	Lock();
	m_Params.m_ConeParams.fInsideAngle = AkMath::ToRadians(in_fInsideAngle) * 0.5f;
	Unlock();
	return AK_Success;
}

AKRESULT CAkGen3DParams::SetConeOutsideAngle( AkReal32 in_fOutsideAngle )
{
	Lock();
    m_Params.m_ConeParams.fOutsideAngle = AkMath::ToRadians(in_fOutsideAngle) * 0.5f;
	Unlock();
	return AK_Success;
}

AKRESULT CAkGen3DParams::SetConeOutsideVolume( AkReal32 in_fOutsideVolume )
{
	Lock();
	m_Params.m_ConeParams.fOutsideVolume = in_fOutsideVolume;
	Unlock();
	return AK_Success;
}

AKRESULT CAkGen3DParams::SetConeLPF( AkLPFType in_LPF )
{
	Lock();
	m_Params.m_ConeParams.LoPass = in_LPF;
	Unlock();
	return AK_Success;
}

AKRESULT CAkGen3DParams::SetIsPanningFromRTPC( bool in_bIsPanningFromRTPC, BaseGenParams& in_rBasePosParams )
{
	Lock();
	m_Params.m_bIsPanningFromRTPC = in_bIsPanningFromRTPC;
	Unlock();
	return AK_Success;
}

AKRESULT CAkGen3DParams::SetIsPositionDynamic( bool in_bIsDynamic )
{
	Lock();
	m_Params.m_bIsDynamic = in_bIsDynamic;
	Unlock();
	return AK_Success;
}

AKRESULT CAkGen3DParams::SetPathMode( AkPathMode in_ePathMode )
{
	Lock();
	m_Params.m_ePathMode = in_ePathMode;
	Unlock();
	return AK_Success;
}

AKRESULT CAkGen3DParams::SetIsLooping( bool in_bIsLooping )
{
	Lock();
	m_Params.m_bIsLooping = in_bIsLooping;
	Unlock();
	return AK_Success;
}

AKRESULT CAkGen3DParams::SetTransition( AkTimeMs in_TransitionTime )
{
	Lock();
	m_Params.m_TransitionTime = in_TransitionTime;
	UpdateTransitionTimeInVertex();
	Unlock();
	return AK_Success;
}

AKRESULT CAkGen3DParams::SetPathPlayList( CAkPath* in_pPath, AkPathState* in_pState)
{
	return g_pPathManager->SetPathsList(in_pPath, 
		m_Params.m_pArrayPlaylist,
		m_Params.m_ulNumPlaylistItem,
		m_Params.m_ePathMode,
		m_Params.m_bIsLooping,
		in_pState);
}

AKRESULT CAkGen3DParams::SetPath(
	AkPathVertex*           in_pArrayVertex, 
	AkUInt32                 in_ulNumVertices, 
	AkPathListItemOffset*   in_pArrayPlaylist, 
	AkUInt32                 in_ulNumPlaylistItem 
	)
{
	AKRESULT eResult = AK_Success;

	Lock();
	ClearPaths();

	// If there is something valid
	if( ( in_ulNumVertices != 0 ) 
		&& ( in_ulNumPlaylistItem != 0 ) 
		&& ( in_pArrayVertex != NULL ) 
		&& ( in_pArrayPlaylist != NULL ) 
		)
	{
		AkUInt32 ulnumBytesVertexArray = in_ulNumVertices * sizeof( AkPathVertex );
		m_Params.m_pArrayVertex = (AkPathVertex*)AkAlloc( g_DefaultPoolId, ulnumBytesVertexArray );

		if(m_Params.m_pArrayVertex)
		{
			AKPLATFORM::AkMemCpy( m_Params.m_pArrayVertex, in_pArrayVertex, ulnumBytesVertexArray );
			m_Params.m_ulNumVertices = in_ulNumVertices;

			AkUInt32 ulnumBytesPlayList = in_ulNumPlaylistItem * sizeof( AkPathListItem );
			m_Params.m_pArrayPlaylist = (AkPathListItem*)AkAlloc( g_DefaultPoolId, ulnumBytesPlayList );

			if(m_Params.m_pArrayPlaylist)
			{
				m_Params.m_ulNumPlaylistItem = in_ulNumPlaylistItem;

				for( AkUInt32 i = 0; i < in_ulNumPlaylistItem; ++i )
				{
					m_Params.m_pArrayPlaylist[i].iNumVertices = in_pArrayPlaylist[i].iNumVertices;

					//Applying the offset to the initial value
					if(in_pArrayPlaylist[i].ulVerticesOffset < in_ulNumVertices )
					{
						m_Params.m_pArrayPlaylist[i].pVertices = m_Params.m_pArrayVertex + in_pArrayPlaylist[i].ulVerticesOffset;
					}
					else
					{
						AKASSERT( !"Trying to access out of range vertex" );
						eResult = AK_Fail;
						break;
					}
				}
			}
			else
			{
				eResult = AK_InsufficientMemory;
			}
		}
		else
		{
			eResult = AK_InsufficientMemory;
		}
	}
	else
	{
		eResult = AK_InvalidParameter;
	}

	UpdateTransitionTimeInVertex();

	Unlock();
	return eResult;
}

AKRESULT CAkGen3DParams::UpdatePathPoint(
		AkUInt32 in_ulPathIndex,
		AkUInt32 in_ulVertexIndex,
		AkVector in_newPosition,
		AkTimeMs in_DelayToNext
		)
{
	AKRESULT eResult = AK_Success;

	Lock();
	AKASSERT( m_Params.m_pArrayVertex != NULL );
	AKASSERT( m_Params.m_pArrayPlaylist != NULL );
	if( ( m_Params.m_pArrayVertex != NULL )
		&& ( m_Params.m_pArrayPlaylist != NULL ) 
		&& ( in_ulPathIndex < m_Params.m_ulNumPlaylistItem )
		&& ( m_Params.m_pArrayPlaylist[in_ulPathIndex].iNumVertices > 0 )// Not useless, done to validate the signed unsigned cast that will be performed afterward
		&& ( in_ulVertexIndex < (AkUInt32)(m_Params.m_pArrayPlaylist[in_ulPathIndex].iNumVertices) ) )
	{
		AkPathVertex& l_rPathVertex = m_Params.m_pArrayPlaylist[in_ulPathIndex].pVertices[in_ulVertexIndex];
		l_rPathVertex.Duration = in_DelayToNext;
		l_rPathVertex.Vertex = in_newPosition;
		UpdateTransitionTimeInVertex();
	}
	else
	{
		eResult = AK_InvalidParameter;
		AKASSERT(!"It is useless to call UpdatePoints() on 3D Parameters if no points are set yet");
	}
	Unlock();
	return eResult;
}

void CAkGen3DParams::UpdateTransitionTimeInVertex()
{
	for( AkUInt32 uli = 0; uli < m_Params.m_ulNumPlaylistItem; ++uli )
	{
		if(m_Params.m_pArrayPlaylist[uli].iNumVertices > 0)
		{
			m_Params.m_pArrayPlaylist[uli].pVertices[m_Params.m_pArrayPlaylist[uli].iNumVertices - 1].Duration = m_Params.m_TransitionTime;
		}
	}
}

#ifndef AK_OPTIMIZED
void CAkGen3DParams::InvalidatePaths()
{
	AkAutoLock<CAkGen3DParams> autoLock( *this );

	m_Params.m_pArrayVertex = NULL;
	m_Params.m_ulNumVertices = 0;
	m_Params.m_pArrayPlaylist = NULL;
	m_Params.m_ulNumPlaylistItem = 0;
}
#endif

