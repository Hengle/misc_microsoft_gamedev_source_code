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
// AkGen3DParams.h
//
// alessard
//
//////////////////////////////////////////////////////////////////////
#ifndef _GEN_3D_PARAMS_H_
#define _GEN_3D_PARAMS_H_

#include "Ak3DParams.h"
#include "AkRTPC.h"
#include "AkBitArray.h"
#include <AK/Tools/Common/AkLock.h>
#include "AkConversionTable.h"
#include "AkAudioLibIndex.h"
#include "AkAttenuationMgr.h"

enum AkPositioningType;

struct AkPathState;
struct AkPathVertex;
struct AkPathListItem;
struct AkPathListItemOffset;
class CAkPath;
class CAkAttenuation;

// IDs of the AudioLib known RTPC capable parameters
enum AkPositioning_ParameterID
{
	POSID_Positioning_Divergence_Center_PCT			= RTPC_Positioning_Divergence_Center_PCT,
	POSID_Positioning_Cone_Attenuation_ON_OFF		= RTPC_Positioning_Cone_Attenuation_ON_OFF,
	POSID_Positioning_Cone_Attenuation				= RTPC_Positioning_Cone_Attenuation,
	POSID_Positioning_Cone_LPF						= RTPC_Positioning_Cone_LPF,
	POSID_Position_PAN_RL							= RTPC_Position_PAN_RL,
	POSID_Position_PAN_FR							= RTPC_Position_PAN_FR,

//ASSERT that  RTPC_MaxNumRTPC == 32 == MAX_BITFIELD_NUM

	POSID_PositioningType							= MAX_BITFIELD_NUM,
	POSID_2DPannerEnabled,
	POSID_ConeInsideAngle,
	POSID_ConeOutsideAngle,
	POSID_IsPositionDynamic,
	POSID_IsLooping,
	POSID_Transition,
	POSID_PathMode,
};

struct BaseGenParams
{
	AkReal32			m_fPAN_RL;
	AkReal32			m_fPAN_FR;
	AkReal32			m_fCenterPct; // 0..1
	bool				bIsPannerEnabled;
	bool operator ==(const BaseGenParams& in_Op)
	{
		return ( 
			   ( m_fPAN_RL		== in_Op.m_fPAN_RL ) 
			&& ( m_fPAN_FR		== in_Op.m_fPAN_FR ) 
			&& ( m_fCenterPct	== in_Op.m_fCenterPct ) 
			&& ( bIsPannerEnabled	== in_Op.bIsPannerEnabled ) 
			);
	}
};

struct Gen3DParams
{
public:
	Gen3DParams();
	~Gen3DParams();

public:
	AkUniqueID			m_ID;					// Id of the owner

	// Global members
	AkPositioningType	m_eType;				// real 3D or not
	AkUniqueID			m_uAttenuationID;		// Attenuation short ID
	bool				m_bIsSpatialized;		// Use spatialization

	// Shared members
	bool				m_bIsConeEnabled;		// cone checkbox
	ConeParams			m_ConeParams;			// cone parameters (needed for rtpc)

	AkVector			m_Position;				// position parameters

	bool				m_bIsPanningFromRTPC;

	// In-game specific params 
	bool				m_bIsDynamic;			// set position continuously

	// Pre-defined specific params 
	bool				m_bIsLooping;			// 
	AkPathMode			m_ePathMode;			// sequence/random & continuous/step
	AkTimeMs			m_TransitionTime;		// 

	// for the paths
	AkPathVertex*		m_pArrayVertex;			// the current path being used
	AkUInt32			m_ulNumVertices;		// how many vertices in m_pArrayVertex
	AkPathListItem*		m_pArrayPlaylist;		// the play list
	AkUInt32            m_ulNumPlaylistItem;	// own many paths in the play list

friend class CAkPBI;
friend class CAkListener;

private:
	// Only CAkPBI and CAkListener are allowed to access the attenuation using GetAttenuation() function.
	//Inlined
	CAkAttenuation*		GetAttenuation()
	{
		if( !m_pAttenuation )
			m_pAttenuation = g_pIndex->m_idxAttenuations.GetPtrAndAddRef( m_uAttenuationID );

		return m_pAttenuation;
	}

private:
	// private member, refcounted pointer
	CAkAttenuation*		m_pAttenuation;
};


class CAkGen3DParams : public CAkObject
{
	//friend class CAkParameterNode;	// To access Term() function only
	// No one else should access the Term() function
	//friend class CAkPBI;			// because this one reads in here to fill in something else

public:
	//Constructor and destructor
	CAkGen3DParams();
	~CAkGen3DParams();

	void Term();

	// real 3D or not
	AKRESULT SetPositioningType(
		AkPositioningType in_ePosType		// how the sound should be processed
		);

	// use the cone attenuation or not
	AKRESULT SetConeUsage(
		bool in_bIsConeEnabled
		);

	// use the spatialization or not
	AKRESULT SetSpatializationEnabled(
		bool in_bIsSpatializationEnabled
		);

	// Set the attenuation to use
	AKRESULT SetAttenuationID(
		AkUniqueID in_AttenuationID
		);

	// position related things
	AKRESULT SetPosition(const AkVector & in_rPosition);

	/*Setting cone params*/
	AKRESULT SetConeInsideAngle( AkReal32 in_fInsideAngle );
	AKRESULT SetConeOutsideAngle( AkReal32 in_fOutsideAngle );
	AKRESULT SetConeOutsideVolume( AkReal32 in_fOutsideVolume );
	AKRESULT SetConeLPF( AkLPFType in_LPF );

	AKRESULT SetIsPanningFromRTPC( bool in_bIsPanningFromRTPC, BaseGenParams& in_rBasePosParams );

	AKRESULT SetIsPositionDynamic( bool in_bIsDynamic );

	AKRESULT SetIsLooping( bool in_bIsLooping );
	AKRESULT SetTransition( AkTimeMs in_TransitionTime );

	AKRESULT SetPathMode( AkPathMode in_ePathMode );
	AKRESULT SetPathPlayList(
		CAkPath*		in_pPath,
		AkPathState*	in_pState
		);
	AKRESULT StartPath();
	AKRESULT StopPath();

	AKRESULT SetPath(
		AkPathVertex*           in_pArrayVertex, 
		AkUInt32                 in_ulNumVertices, 
		AkPathListItemOffset*   in_pArrayPlaylist, 
		AkUInt32                 in_ulNumPlaylistItem 
		);

	void SetPathOwner(AkUniqueID in_PathOwner) { m_Params.m_ID = in_PathOwner; }

	AkUniqueID GetPathOwner() { return m_Params.m_ID; }

	AKRESULT UpdatePathPoint(
		AkUInt32 in_ulPathIndex,
		AkUInt32 in_ulVertexIndex,
		AkVector in_newPosition,
		AkTimeMs in_DelayToNext
		);

	Gen3DParams *		GetParams() { return &m_Params; }

#ifndef AK_OPTIMIZED
	void				Lock() { m_csLock.Lock(); }
	void				Unlock() { m_csLock.Unlock(); }
#else
	void				Lock() { }
	void				Unlock() { }
#endif

#ifndef AK_OPTIMIZED
	void InvalidatePaths();
#endif

protected:
	Gen3DParams			m_Params;

private:

	void				UpdateTransitionTimeInVertex();
	void				ClearPaths();

#ifndef AK_OPTIMIZED
	static CAkLock		m_csLock;
#endif
};

#endif //_GEN_3D_PARAMS_H_
