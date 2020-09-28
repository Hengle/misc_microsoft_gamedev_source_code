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
// AkTransitionManager.cpp
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include <AK/Tools/Common/AkAutoLock.h>
#include "AkTransitionManager.h"
#include "AkList2.h"
#include "AkMath.h"
#include "AudiolibDefs.h"
#include "AkTransition.h"
#include "AkInterpolation.h"
#include "AkAudioMgr.h"

extern CAkInterpolation g_Interpol;

//====================================================================================================
//====================================================================================================
CAkTransitionManager::CAkTransitionManager()
{
	m_uMaxNumTransitions = 0;
}
//====================================================================================================
//====================================================================================================
CAkTransitionManager::~CAkTransitionManager()
{
}
//====================================================================================================
//====================================================================================================
AKRESULT CAkTransitionManager::Init( AkUInt32 in_uMaxNumTransitions )
{
	m_uMaxNumTransitions = in_uMaxNumTransitions ? in_uMaxNumTransitions : DEFAULT_MAX_NUM_TRANSITIONS;

	AKRESULT eResult = m_ActiveTransitionsList_Fade.Reserve( m_uMaxNumTransitions );
	if( eResult == AK_Success )
		eResult = m_ActiveTransitionsList_State.Reserve( m_uMaxNumTransitions );

	return eResult;
}
//====================================================================================================
//====================================================================================================
void CAkTransitionManager::Term()
{
	// anyone got lost in here ?
	TermList( m_ActiveTransitionsList_Fade );
	TermList( m_ActiveTransitionsList_State );
}

void CAkTransitionManager::TermList( AkTransitionList& in_rTransitionList )
{
	if(!in_rTransitionList.IsEmpty())
	{
		for( AkTransitionList::Iterator iter = in_rTransitionList.Begin(); iter != in_rTransitionList.End(); ++iter )
		{
			CAkTransition* pTransition = *iter;
			// get rid of the transition
			pTransition->Term();
			AkDelete( g_DefaultPoolId, pTransition );
		}
	}
	in_rTransitionList.Term();
}
//====================================================================================================
// add a new one to the list of those to be processed
//====================================================================================================
CAkTransition* CAkTransitionManager::AddTransitionToList(const TransitionParameters& in_Params,bool in_bStart,AkTransitionCategory in_eTransitionCategory)
{	
	AkTransitionList* pTransitionList = &m_ActiveTransitionsList_Fade;
	if( in_eTransitionCategory == TC_State )
	{
		pTransitionList = &m_ActiveTransitionsList_State;
	}
	else
	{
		pTransitionList = &m_ActiveTransitionsList_Fade;
	}

	AKASSERT(in_Params.eTargetType != AkUndefinedTargetType);

	CAkTransition* pThisTransition = NULL;

	// can we add a new one ?
	if(pTransitionList->Length() < m_uMaxNumTransitions)
	{
		// get a new one
		pThisTransition = AkNew( g_DefaultPoolId, CAkTransition );
		if( pThisTransition )
		{
			pThisTransition->Init();
		}
	}
	// find a transition that can be stopped
	if( !pThisTransition )
	{
		// look for the one closest to completion
		AkReal32 fBiggestTimeRatio = 0.0f;
		AkTransitionList::Iterator iter = pTransitionList->Begin();
		while(iter != pTransitionList->End())
		{
			CAkTransition* pTransition = *iter;
			// is this one closer to completion ?
			if(pTransition->m_fTimeRatio > fBiggestTimeRatio)
			{
				pThisTransition = pTransition;
				fBiggestTimeRatio = pTransition->m_fTimeRatio;
			}
			++iter;
		}
		// if we've got one then stop it and clean it up
		if(pThisTransition != NULL)
		{
			// force transition to end
			pThisTransition->m_fDurationInBufferTick = 0.0f;
			pThisTransition->ComputeTransition(0);
			// clean it up to be re-used
			pThisTransition->Reset();
			pTransitionList->RemoveSwap( pThisTransition );
		}
	}

	// have we got one ?
	if(pThisTransition != NULL)
	{
		// fill it in and check that it was ok
		if(pThisTransition->InitParameters( in_Params, g_pAudioMgr->GetBufferTick() ) != AK_Fail)
		{
			// add it to the active list
			if( pTransitionList->AddLast( pThisTransition ) )
			{
				if(in_bStart)
				{
					// start it
					pThisTransition->m_eState = CAkTransition::Running;
				}
			}
			else
			{
				pThisTransition->Term();
				AkDelete( g_DefaultPoolId, pThisTransition );
				pThisTransition = NULL;
			}
		}
		// couldn't fill it in for some reason, get rid of it
		else
		{
			pThisTransition->Term();
			AkDelete( g_DefaultPoolId, pThisTransition );
			pThisTransition = NULL;
		}
	}
	return pThisTransition;
}
//====================================================================================================
// adds a given user to a given multi user transition
//====================================================================================================
AKRESULT CAkTransitionManager::AddTransitionUser(CAkTransition* in_pTransition,ITransitionable* in_pUser)
{
	AKASSERT( m_ActiveTransitionsList_Fade.Exists(in_pTransition) || m_ActiveTransitionsList_State.Exists(in_pTransition) );

	// assume already in list
	AKRESULT eResult = AK_UserAlreadyInList;

	if(!in_pTransition->m_UsersList.Exists(in_pUser))
	{
		eResult = AK_UsersListFull;

		// can we add a new one ?
		if(in_pTransition->m_iNumUsers < AK_TRANSITION_USERS_LIST_SIZE
			&& in_pTransition->m_UsersList.AddLast(in_pUser) )
		{
			eResult = AK_Success;

			// we have one more
			++in_pTransition->m_iNumUsers;
		}
	}

	return(eResult);
}

bool CAkTransitionManager::IsTerminated( CAkTransition* in_pTransition )
{
	AKASSERT( m_ActiveTransitionsList_Fade.Exists(in_pTransition) || m_ActiveTransitionsList_State.Exists(in_pTransition) );

	return ( in_pTransition->m_eState == CAkTransition::Done );
}

//====================================================================================================
// removes a given user from a given multi user transition
//====================================================================================================
AKRESULT CAkTransitionManager::RemoveTransitionUser(CAkTransition* in_pTransition,ITransitionable* in_pUser)
{
	AKASSERT( m_ActiveTransitionsList_Fade.Exists(in_pTransition) || m_ActiveTransitionsList_State.Exists(in_pTransition) );

	// assume we don't have this user
	AKRESULT eResult = AK_UserNotInList;

	// look for our user in the list
	CAkTransition::AkTransitionUsersList::Iterator iter = in_pTransition->m_UsersList.Begin();
	while( iter != in_pTransition->m_UsersList.End() )
	{
		ITransitionable* pITransitionable = *iter;
		// got it ?
		if(pITransitionable == in_pUser)
		{
			eResult = AK_Success;

			// remove it
			iter = in_pTransition->m_UsersList.EraseSwap( iter );

			// we have one less
			--in_pTransition->m_iNumUsers;
			
			if( in_pTransition->m_iNumUsers == 0 )
			{
				RemoveTransitionFromList( in_pTransition );
			}

			break;
		}
		else
		{
			++iter;
		}
	}

	return eResult;
}
//====================================================================================================
// flags a transition for removal
//====================================================================================================
void CAkTransitionManager::RemoveTransitionFromList(CAkTransition* in_pTransition)
{
	AKASSERT( m_ActiveTransitionsList_Fade.Exists(in_pTransition) || m_ActiveTransitionsList_State.Exists(in_pTransition) );

	// then remove it
	in_pTransition->m_eState = CAkTransition::ToRemove;
}
//====================================================================================================
// flags a transition for pause
//====================================================================================================
void CAkTransitionManager::Pause(CAkTransition* in_pTransition)
{
	AKASSERT( m_ActiveTransitionsList_Fade.Exists(in_pTransition) || m_ActiveTransitionsList_State.Exists(in_pTransition) );

	AKASSERT(in_pTransition != NULL);

	// is it pause-able ?
	if(in_pTransition->m_eState == CAkTransition::Running)
	{
		// then pause it
		in_pTransition->m_eState = CAkTransition::ToPause;
	}
}
//====================================================================================================
// flags a transition for un-pause
//====================================================================================================
void CAkTransitionManager::Resume(CAkTransition* in_pTransition)
{
	AKASSERT( m_ActiveTransitionsList_Fade.Exists(in_pTransition) || m_ActiveTransitionsList_State.Exists(in_pTransition) );

	AKASSERT(in_pTransition != NULL);

	// is it resume-able
	if(in_pTransition->m_eState == CAkTransition::Paused)
	{
		// then resume it
		in_pTransition->m_eState = CAkTransition::ToResume;
	}
}
//====================================================================================================
// changes the target and duration values
//====================================================================================================
void CAkTransitionManager::ChangeParameter(CAkTransition*		in_pTransition,
											TransitionTargets	in_eTransitionType,
											TransitionTarget	in_NewTarget,
											AkTimeMs			in_NewDuration,
											AkValueMeaning		in_eValueMeaning)
{
	AKASSERT( m_ActiveTransitionsList_Fade.Exists(in_pTransition) || m_ActiveTransitionsList_State.Exists(in_pTransition) );

	TransitionTypes	TransitionType;

	AKASSERT(in_pTransition != NULL);
	AKASSERT(in_pTransition->m_eTargetType != AkUndefinedTargetType);
	
	TransitionTargets l_oldTarget = (TransitionTargets)( in_pTransition->m_eTargetType & TransTarget_TargetMask );

	// set the new target type
	in_pTransition->m_eTargetType = static_cast<TransitionTargetTypes>((in_pTransition->m_eTargetType & (~TransTarget_TargetMask))|(in_eTransitionType & TransTarget_TargetMask));

	TransitionType = static_cast<TransitionTypes>(in_pTransition->m_eTargetType & AkTypeMask);
//----------------------------------------------------------------------------------------------------
// process dB values
//----------------------------------------------------------------------------------------------------
	if(in_pTransition->m_bdBs)
	{
		// did it get processed at least once ?
		if(in_pTransition->m_bCurrentValueSet)
		{
			// this is our new starting point
			in_pTransition->m_uStartValue.fValue = in_pTransition->m_fCurrentValue;

			// convert dBs to linear
            CAkTransition::ConvertdBs(TransitionType,in_pTransition->m_uStartValue,in_NewTarget);
		}
		// nope, keep the same start value
		else
		{
			// we have no start value to convert so we pass a dummy
			TransitionTarget Dummy;
			Dummy.lValue = 0;

			// convert dBs to linear
			CAkTransition::ConvertdBs(TransitionType,Dummy,in_NewTarget);
		}

		// is our new ending point an offset ?
		if(in_eValueMeaning == AkValueMeaning_Offset)
		{
			// * linears is + dBs
			switch(TransitionType)
			{
			case AkTypeFloat:
				in_pTransition->m_uTargetValue.fValue *= in_NewTarget.fValue;
				break;
			case AkTypeLong:
				in_pTransition->m_uTargetValue.fValue *= static_cast<AkReal32>(in_NewTarget.lValue);
				break;
			case AkTypeWord:
				in_pTransition->m_uTargetValue.fValue *= static_cast<AkReal32>(in_NewTarget.wValue);
				break;
			case AkTypeShort:
				in_pTransition->m_uTargetValue.fValue *= static_cast<AkReal32>(in_NewTarget.sValue);
				break;
			case AkTypeByte:
				in_pTransition->m_uTargetValue.fValue *= static_cast<AkReal32>(in_NewTarget.ucValue);
				break;
			}
		}
		// nope it's not, fill in the union with whatever we got
		else
		{
			switch(TransitionType)
			{
			case AkTypeFloat:
				in_pTransition->m_uTargetValue.fValue = in_NewTarget.fValue;
				break;
			case AkTypeLong:
				in_pTransition->m_uTargetValue.fValue = static_cast<AkReal32>(in_NewTarget.lValue);
				break;
			case AkTypeWord:
				in_pTransition->m_uTargetValue.fValue = static_cast<AkReal32>(in_NewTarget.wValue);
				break;
			case AkTypeShort:
				in_pTransition->m_uTargetValue.fValue = static_cast<AkReal32>(in_NewTarget.sValue);
				break;
			case AkTypeByte:
				in_pTransition->m_uTargetValue.fValue = static_cast<AkReal32>(in_NewTarget.ucValue);
				break;
			}
		}
	}
//----------------------------------------------------------------------------------------------------
// process linear values
//----------------------------------------------------------------------------------------------------
	else
	{
		// did it get processed at least once ?
		if(in_pTransition->m_bCurrentValueSet)
		{
			// this is our new starting point
			in_pTransition->m_uStartValue.fValue = in_pTransition->m_fCurrentValue;
		}

		// is our new ending point an offset ?
		if(in_eValueMeaning == AkValueMeaning_Offset)
		{
			switch(TransitionType)
			{
			case AkTypeFloat:
				in_pTransition->m_uTargetValue.fValue += in_NewTarget.fValue;
				break;
			case AkTypeLong:
				in_pTransition->m_uTargetValue.fValue += static_cast<AkReal32>(in_NewTarget.lValue);
				break;
			case AkTypeWord:
				in_pTransition->m_uTargetValue.fValue += static_cast<AkReal32>(in_NewTarget.wValue);
				break;
			case AkTypeShort:
				in_pTransition->m_uTargetValue.fValue += static_cast<AkReal32>(in_NewTarget.sValue);
				break;
			case AkTypeByte:
				in_pTransition->m_uTargetValue.fValue += static_cast<AkReal32>(in_NewTarget.ucValue);
				break;
			}
		}
		// nope it's not, fill in the union with whatever we got
		else
		{
			switch(TransitionType)
			{
			case AkTypeFloat:
				in_pTransition->m_uTargetValue.fValue = in_NewTarget.fValue;
				break;
			case AkTypeLong:
				in_pTransition->m_uTargetValue.fValue = static_cast<AkReal32>(in_NewTarget.lValue);
				break;
			case AkTypeWord:
				in_pTransition->m_uTargetValue.fValue = static_cast<AkReal32>(in_NewTarget.wValue);
				break;
			case AkTypeShort:
				in_pTransition->m_uTargetValue.fValue = static_cast<AkReal32>(in_NewTarget.sValue);
				break;
			case AkTypeByte:
				in_pTransition->m_uTargetValue.fValue = static_cast<AkReal32>(in_NewTarget.ucValue);
				break;
			}
		}
	}

	AkReal32 fLastNumBufferTickSeen = (AkReal32) g_pAudioMgr->GetBufferTick();

	// Same target in a Play/Stop/Pause/Resume transition ? 
	if ( ( l_oldTarget == (TransitionTargets)( in_pTransition->m_eTargetType & TransTarget_TargetMask ) )
		&& ( in_eTransitionType & ( TransTarget_Play | TransTarget_Stop | TransTarget_Pause | TransTarget_Resume ) ) )
	{
		in_pTransition->m_fDurationInBufferTick = AkMath::Min( (AkReal32)CAkTransition::Convert( in_NewDuration ), in_pTransition->m_fDurationInBufferTick - ( fLastNumBufferTickSeen - in_pTransition->m_fStartTimeInBufferTick ) );
	}
	else
	{
		in_pTransition->m_fDurationInBufferTick = (AkReal32) CAkTransition::Convert( in_NewDuration );
	}

	// this is our new start time
	in_pTransition->m_fStartTimeInBufferTick = fLastNumBufferTickSeen;
}
//====================================================================================================
// does what it takes to get things moving
//====================================================================================================
void CAkTransitionManager::ProcessTransitionsList( AkUInt32 in_CurrentBufferTick )
{
	ProcessTransitionsList( in_CurrentBufferTick, m_ActiveTransitionsList_Fade );
	ProcessTransitionsList( in_CurrentBufferTick, m_ActiveTransitionsList_State );
}
//====================================================================================================
// Helper, called for each transition list
//====================================================================================================
void CAkTransitionManager::ProcessTransitionsList( AkUInt32 in_CurrentBufferTick, AkTransitionList& in_rTransitionList )
{
	CAkTransition*	pThisTransition;
//----------------------------------------------------------------------------------------------------
// process the active transitions
//----------------------------------------------------------------------------------------------------

	AkTransitionList::Iterator iter = in_rTransitionList.Begin();
	while( iter != in_rTransitionList.End() )
	{
		pThisTransition = *iter;

//----------------------------------------------------------------------------------------------------
// should be removed ? if yes then it should not be processed
//----------------------------------------------------------------------------------------------------
		if(pThisTransition->m_eState == CAkTransition::ToRemove)
		{
			// get rid of the transition
			pThisTransition->Term();
			AkDelete( g_DefaultPoolId, pThisTransition );

			// remove the list entry
			iter = in_rTransitionList.EraseSwap( iter );
		}
		else
		{
			switch(pThisTransition->m_eState)
			{
//----------------------------------------------------------------------------------------------------
// this one needs to be paused
//----------------------------------------------------------------------------------------------------
			case CAkTransition::ToPause:
				// remember when this happened
				pThisTransition->m_uLastBufferTickUpdated = in_CurrentBufferTick;
				pThisTransition->m_eState = CAkTransition::Paused;
			break;
//----------------------------------------------------------------------------------------------------
// This one needs to be resumed
//----------------------------------------------------------------------------------------------------
			case CAkTransition::ToResume:
				// erase the time spent in pause mode
				pThisTransition->m_fStartTimeInBufferTick += static_cast<AkReal32>( in_CurrentBufferTick - pThisTransition->m_uLastBufferTickUpdated );

				pThisTransition->m_eState = CAkTransition::Running;
			break;
			}
//----------------------------------------------------------------------------------------------------
// now let's update those who need to
//----------------------------------------------------------------------------------------------------
			if(pThisTransition->m_eState == CAkTransition::Running)
			{
				// step it and let me know what to do
				if(pThisTransition->ComputeTransition( in_CurrentBufferTick )) // TRUE == transition completed 
				{
					// this one is done
					pThisTransition->Term();
					iter = in_rTransitionList.EraseSwap( iter );
					AkDelete( g_DefaultPoolId, pThisTransition );
				}
				else
				{
					++iter;
				}
			}
			else
			{
				++iter;
			}
		}
	}
}

// gets the current and maximum number of transitions
void CAkTransitionManager::GetTransitionsUsage( 
		AkUInt16& out_ulNumFadeTransitionsUsed, 
		AkUInt16& out_ulMaxFadeNumTransitions,
		AkUInt16& out_ulNumStateTransitionsUsed, 
		AkUInt16& out_ulMaxStateFadeNumTransitions 
		)
{
	out_ulNumFadeTransitionsUsed = (AkUInt16)m_ActiveTransitionsList_Fade.Length();
	out_ulMaxFadeNumTransitions = (AkUInt16)m_uMaxNumTransitions;
	out_ulNumStateTransitionsUsed = (AkUInt16)m_ActiveTransitionsList_State.Length();
	out_ulMaxStateFadeNumTransitions = (AkUInt16)m_uMaxNumTransitions;
}
