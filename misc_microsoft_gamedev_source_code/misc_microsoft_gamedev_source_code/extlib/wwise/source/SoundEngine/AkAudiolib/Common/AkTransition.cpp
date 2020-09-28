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
// AkTransition.cpp
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "AkTransition.h"
#include "AudiolibDefs.h"
#include "AkInterpolation.h"

extern CAkInterpolation g_Interpol;

//====================================================================================================
//====================================================================================================
CAkTransition::CAkTransition()
{
	m_iNumUsers = 0;
	m_eTargetType = AkUndefinedTargetType;
	m_bdBs = false;
	m_eState = Idle;
	m_bCurrentValueSet = false;
	m_fTimeRatio = 0.0f;
}
//====================================================================================================
//====================================================================================================
CAkTransition::~CAkTransition()
{
}
//====================================================================================================
//====================================================================================================
void CAkTransition::Term()
{
	// get rid of our user's lists
	m_UsersList.Term();
}
//====================================================================================================
//====================================================================================================
void CAkTransition::Reset()
{
	m_UsersList.RemoveAll();
	m_iNumUsers = 0;
	m_eTargetType = AkUndefinedTargetType;
	m_bdBs = false;
	m_eState = Idle;
	m_bCurrentValueSet = false;
	m_fTimeRatio = 0.0f;
}
//====================================================================================================
// steps transition returns TRUE when the transition has completed
//====================================================================================================
bool CAkTransition::ComputeTransition( AkUInt32 in_CurrentBufferTick )
{
	TransitionTarget	Parameter;
	AkReal32	fCurrentTime,
				fResult;			// will get sent to the user to update the the target parameter
	bool		bDone = false;		// tells the manager to drop completed transitions

	fCurrentTime = static_cast <AkReal32> ( in_CurrentBufferTick );

	// non zero duration means interpolate
	if(m_fDurationInBufferTick != 0.0f)
	{
		m_fTimeRatio = ( fCurrentTime - m_fStartTimeInBufferTick ) / m_fDurationInBufferTick;

		// avoid overshots and detect when done
		if(m_fTimeRatio >= 1.0)
		{
			bDone = true;
		}
        else if(m_fTimeRatio < 0.0)
        {
            m_fTimeRatio = 0.0;
            // OPTIMIZE (LX) return false;
        }
	}
	// zero duration means go to target value now
	else
	{
		bDone = true;
	}

	// handle last value properly
	if(bDone == true)
	{
		// we reached target value
		fResult = m_uTargetValue.fValue;

		// it's gotta be dBs so convert it
		if(m_bdBs == true)
		{
			fResult = g_Interpol.RealTodB(m_uTargetValue.fValue);
		}
	}
	else
	{
		// compute new value
		if(m_bdBs)
		{
			fResult = g_Interpol.InterpolateDB(m_fTimeRatio,
								m_uStartValue.fValue,
								m_uTargetValue.fValue,
								m_eFadeCurve);
		}
		else
		{
			fResult = g_Interpol.Interpolate(m_fTimeRatio,
								m_uStartValue.fValue,
								m_uTargetValue.fValue,
								m_eFadeCurve);
		}
	}

	m_fCurrentValue = fResult;
	m_bCurrentValueSet = true;

	AKASSERT( m_eTargetType != (TransitionTargetTypes)AkUndefinedType );

	switch(m_eTargetType & AkTypeMask)
	{
	case AkTypeFloat:
		Parameter.fValue = fResult;
		break;
	case AkTypeLong:
		Parameter.lValue = static_cast<AkInt32>(fResult);
		break;
	case AkTypeWord:
		Parameter.wValue = static_cast<AkUInt16>(fResult);
		break;
	case AkTypeShort:
		Parameter.sValue = static_cast<AkInt16>(fResult);
		break;
	case AkTypeByte:
		Parameter.ucValue = static_cast<AkUInt8>(fResult);
		break;
	default:
		AKASSERT(0);
		break;
	}

	// send the new stuff to the users if any
	for( AkTransitionUsersList::Iterator iter = m_UsersList.Begin(); iter != m_UsersList.End(); ++iter )
	{
		AKASSERT( *iter );
		// call this one
		(*iter)->TransUpdateValue( m_eTargetType, Parameter, bDone );
	}

	return bDone;
}
//====================================================================================================
// fill in a transition
//====================================================================================================
AKRESULT CAkTransition::InitParameters( const TransitionParameters& in_Params, AkUInt32 in_CurrentBufferTick )
{
	AKRESULT l_eResult = AK_Success;

	m_eTargetType = in_Params.eTargetType;

	m_eFadeCurve = in_Params.eFadeCurve;

	m_bdBs = in_Params.bdBs;

    TransitionTarget startValue = in_Params.uStartValue;
    TransitionTarget targetValue = in_Params.uTargetValue;

    // convert dBs to linear first, this saves two m_tableDBtoReal->Get() per transition in the process loop
	if(in_Params.bdBs)
	{
        ConvertdBs(static_cast<TransitionTypes>(in_Params.eTargetType & AkTypeMask),startValue,targetValue);
	}

	switch(in_Params.eTargetType & AkTypeMask)
	{
	case AkTypeFloat:
		m_uStartValue.fValue = startValue.fValue;
		m_fCurrentValue = m_uStartValue.fValue;
		m_uTargetValue.fValue = targetValue.fValue;
		break;
	case AkTypeLong:
		m_uStartValue.fValue = static_cast<AkReal32>(startValue.lValue);
		m_fCurrentValue = m_uStartValue.fValue;
		m_uTargetValue.fValue = static_cast<AkReal32>(targetValue.lValue);
		break;
	case AkTypeWord:
		m_uStartValue.fValue = static_cast<AkReal32>(startValue.wValue);
		m_fCurrentValue = m_uStartValue.fValue;
		m_uTargetValue.fValue = static_cast<AkReal32>(targetValue.wValue);
		break;
	case AkTypeShort:
		m_uStartValue.fValue = static_cast<AkReal32>(startValue.sValue);
		m_fCurrentValue = m_uStartValue.fValue;
		m_uTargetValue.fValue = static_cast<AkReal32>(targetValue.sValue);
		break;
	case AkTypeByte:
		m_uStartValue.fValue = static_cast<AkReal32>(startValue.ucValue);
		m_fCurrentValue = m_uStartValue.fValue;
		m_uTargetValue.fValue = static_cast<AkReal32>(targetValue.ucValue);
		break;
	default:
		l_eResult = AK_Fail;
		break;
	}

	m_fStartTimeInBufferTick = static_cast<AkReal32>( in_CurrentBufferTick );
	m_fDurationInBufferTick = static_cast<AkReal32>( CAkTransition::Convert( in_Params.lDuration ) );
	m_fTimeRatio = 0.0f;

	// as we may be called to start a sticky transition the user might already be in here
	if(!m_UsersList.Exists(in_Params.pUser))
	{
		// add the user to the list
		if( !m_UsersList.AddLast( in_Params.pUser ) )
		{
			return AK_Fail;
		}

		++m_iNumUsers;
	}
	return l_eResult;
}

//====================================================================================================
// convert dB values to real (non-log)
//====================================================================================================
void CAkTransition::ConvertdBs(TransitionTypes WhatType,TransitionTarget& Start,TransitionTarget& Target)
{
	switch(WhatType)
	{
	case AkTypeFloat:
		Start.fValue = g_Interpol.dBToReal(Start.fValue);
		Target.fValue = g_Interpol.dBToReal(Target.fValue);
		break;
	case AkTypeLong:
		Start.lValue = static_cast<AkInt32>(g_Interpol.dBToReal(static_cast<AkReal32>(Start.lValue)));
		Target.lValue = static_cast<AkInt32>(g_Interpol.dBToReal(static_cast<AkReal32>(Target.lValue)));
		break;
	case AkTypeWord:
		Start.wValue = static_cast<AkUInt16>(g_Interpol.dBToReal(static_cast<AkReal32>(Start.wValue)));
		Target.wValue = static_cast<AkUInt16>(g_Interpol.dBToReal(static_cast<AkReal32>(Target.wValue)));
		break;
	case AkTypeShort:
		Start.sValue = static_cast<AkInt16>(g_Interpol.dBToReal(static_cast<AkReal32>(Start.sValue)));
		Target.sValue = static_cast<AkInt16>(g_Interpol.dBToReal(static_cast<AkReal32>(Target.sValue)));
		break;
	case AkTypeByte:
		Start.ucValue = static_cast<AkUInt8>(g_Interpol.dBToReal(static_cast<AkReal32>(Start.ucValue)));
		Target.ucValue = static_cast<AkUInt8>(g_Interpol.dBToReal(static_cast<AkReal32>(Target.ucValue)));
		break;
	}
}
