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
// AkInterpolation.cpp
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "AkInterpolation.h"
#include "AkInterpolationTable.h"
#include "AudioLibDefs.h"     // Pool IDs definition.
//====================================================================================================
//====================================================================================================
CAkInterpolation::CAkInterpolation()
:m_tableDBtoReal(NULL)
,m_tableRealToDB(NULL)
,m_tableExp1(NULL)
,m_tableExp2(NULL)
,m_tableExp3(NULL)
,m_tableLinearMutingTodBMuting255(NULL)
,m_tableLinearMutingTodBMuting96(NULL)
{
}
//====================================================================================================
//====================================================================================================
CAkInterpolation::~CAkInterpolation()
{
}
//====================================================================================================
//====================================================================================================
AKRESULT CAkInterpolation::Init(size_t in_wTableSize, size_t in_wdBTableSize, AkITable in_eITable)
{
	AKASSERT( AK::MemoryMgr::IsInitialized() );

	// we have at least the dB tables
	AkUInt32		ulNumTables = 4;

	// figure out how many more we have
	if(in_eITable & AkITable_Exp1)
	{
		++ulNumTables;
	}
	if(in_eITable & AkITable_Exp2)
	{
		++ulNumTables;
	}
	if(in_eITable & AkITable_Exp3)
	{
		++ulNumTables;
	}

	bool bThingsWentWrong = false;

	if(!bThingsWentWrong && (in_eITable & AkITable_Exp1))
	{
		m_tableExp1 = AkNew(g_DefaultPoolId,CAkInterpolationTable(CalculationFunction::AkExp1,0,1,in_wTableSize));
		bThingsWentWrong = bThingsWentWrong || (m_tableExp1 == NULL) || ( m_tableExp1->Init() != AK_Success );
	}
	if(!bThingsWentWrong && (in_eITable & AkITable_Exp2))
	{
		m_tableExp2 = AkNew(g_DefaultPoolId,CAkInterpolationTable(CalculationFunction::AkExp2,0,1,in_wTableSize));
		bThingsWentWrong = bThingsWentWrong || (m_tableExp2 == NULL) || ( m_tableExp2->Init() != AK_Success );
	}
	if(!bThingsWentWrong && (in_eITable & AkITable_Exp3))
	{
		m_tableExp3 = AkNew(g_DefaultPoolId,CAkInterpolationTable(CalculationFunction::AkExp3,0,1,in_wTableSize));
		bThingsWentWrong = bThingsWentWrong || (m_tableExp3 == NULL) || ( m_tableExp3->Init() != AK_Success );
	}
	m_tableDBtoReal = AkNew(g_DefaultPoolId,CAkInterpolationTable(CalculationFunction::dBToReal,AK_MINIMUM_VOLUME_LEVEL,-AK_MINIMUM_VOLUME_LEVEL,in_wdBTableSize));
	bThingsWentWrong = bThingsWentWrong || (m_tableDBtoReal == NULL) || ( m_tableDBtoReal->Init() != AK_Success );

	m_tableRealToDB = AkNew(g_DefaultPoolId,CAkInterpolationTable(CalculationFunction::realTodB,1,10,in_wdBTableSize));
	bThingsWentWrong = bThingsWentWrong || (m_tableRealToDB == NULL) || ( m_tableRealToDB->Init() != AK_Success );

	m_tableLinearMutingTodBMuting255 = AkNew(g_DefaultPoolId,CAkInterpolationTable(CalculationFunction::linearMutingTodBMuting255,0,255,256));
	bThingsWentWrong = bThingsWentWrong || (m_tableLinearMutingTodBMuting255 == NULL) || ( m_tableLinearMutingTodBMuting255->Init() != AK_Success );

	m_tableLinearMutingTodBMuting96 = AkNew(g_DefaultPoolId,CAkInterpolationTable(CalculationFunction::linearMutingTodBMuting96,AK_MINIMUM_VOLUME_LEVEL,-AK_MINIMUM_VOLUME_LEVEL,in_wdBTableSize));
	bThingsWentWrong = bThingsWentWrong || (m_tableLinearMutingTodBMuting96 == NULL) || ( m_tableLinearMutingTodBMuting96->Init() != AK_Success );

	return bThingsWentWrong ? AK_Fail : AK_Success;
}
//====================================================================================================
//====================================================================================================
void CAkInterpolation::Term()
{
	if(m_tableExp1)
	{
		m_tableExp1->Term();
		AkDelete(g_DefaultPoolId,m_tableExp1);
		m_tableExp1 = NULL;
	}
	if(m_tableExp2)
	{
		m_tableExp2->Term();
		AkDelete(g_DefaultPoolId,m_tableExp2);
		m_tableExp2 = NULL;
	}
	if(m_tableExp3)
	{
		m_tableExp3->Term();
		AkDelete(g_DefaultPoolId,m_tableExp3);
		m_tableExp3 = NULL;
	}
	if( m_tableDBtoReal )
	{
		m_tableDBtoReal->Term();
		AkDelete(g_DefaultPoolId,m_tableDBtoReal);
		m_tableDBtoReal = NULL;
	}
	if( m_tableRealToDB )
	{
		m_tableRealToDB->Term();
		AkDelete(g_DefaultPoolId,m_tableRealToDB);
		m_tableRealToDB = NULL;
	}
	if( m_tableLinearMutingTodBMuting255 )
	{
		m_tableLinearMutingTodBMuting255->Term();
		AkDelete(g_DefaultPoolId,m_tableLinearMutingTodBMuting255);
		m_tableLinearMutingTodBMuting255 = NULL;
	}
	if( m_tableLinearMutingTodBMuting96 )
	{
		m_tableLinearMutingTodBMuting96->Term();
		AkDelete(g_DefaultPoolId,m_tableLinearMutingTodBMuting96);
		m_tableLinearMutingTodBMuting96 = NULL;
	}
}
//====================================================================================================
//====================================================================================================
AkReal32 CAkInterpolation::InterpolateDB(AkReal32 in_fTimeRatio, AkReal32 in_fInitialVal, AkReal32 in_fTargetVal, AkCurveInterpolation in_eFadeCurve)
{
	AkReal32 fCorrector = 0;
	AkReal32 fNormValue = Interpolate(	in_fTimeRatio,
									in_fInitialVal,
									in_fTargetVal,
									in_eFadeCurve);
	if(fNormValue <= 0)
	{
		return AK_MINIMUM_VOLUME_LEVEL;
	}
	// The m_tableRealToDB table is scaled to 20log10(Norm) ranging from 1 to 10
	while(fNormValue > 10)
	{
		fNormValue/=10;
		fCorrector+=20;
	}
	while(fNormValue < 1)
	{
		fNormValue*=10;
		fCorrector-=20;
	}
	return m_tableRealToDB->Get(fNormValue) + fCorrector;
}

AkReal32 CAkInterpolation::Interpolate(AkReal32 in_fTimeRatio, AkReal32 in_fInitialVal, AkReal32 in_fTargetVal, AkCurveInterpolation in_eFadeCurve)
{
	// *****************************************************************************************************************
	// IMPORTANT: If you modify this function, do not forget to update CAkInterpolation::InterpolateNoCheck() as well!!!
	// *****************************************************************************************************************

	AkReal32 fEffectRatio = 0;
	switch(in_eFadeCurve)
	{
	case AkCurveInterpolation_Linear:
		fEffectRatio = in_fTimeRatio;
		break;
	case AkCurveInterpolation_Exp1:
		AKASSERT(m_tableExp1);
		fEffectRatio = m_tableExp1->Get(in_fTimeRatio);
		break;
	case AkCurveInterpolation_Exp2:
		AKASSERT(m_tableExp2);
		fEffectRatio = m_tableExp2->Get(in_fTimeRatio);
		break;
	case AkCurveInterpolation_Exp3:
		AKASSERT(m_tableExp3);
		fEffectRatio = m_tableExp3->Get(in_fTimeRatio);
		break;
	case AkCurveInterpolation_Log1:
		AKASSERT(m_tableExp1);
		fEffectRatio = 1 - m_tableExp1->Get(1-in_fTimeRatio);
		break;
	case AkCurveInterpolation_Log2:
		AKASSERT(m_tableExp2);
		fEffectRatio = 1 - m_tableExp2->Get(1-in_fTimeRatio);
		break;
	case AkCurveInterpolation_Log3:
		AKASSERT(m_tableExp3);
		fEffectRatio = 1 - m_tableExp3->Get(1-in_fTimeRatio);
		break;
	case AkCurveInterpolation_SCurve:
		AKASSERT(m_tableExp2);
		if(in_fTimeRatio < 0.5)
		{
			fEffectRatio = m_tableExp2->Get(2*in_fTimeRatio)/2;
		}
		else
		{
			fEffectRatio = ((1-m_tableExp2->Get(1.0f-(2*(in_fTimeRatio-0.5f))))/2)+0.5f;
		}
		break;
	case AkCurveInterpolation_InvSCurve:
		AKASSERT(m_tableExp2);
		if(in_fTimeRatio < 0.5)
		{
			fEffectRatio = (1 - m_tableExp2->Get(1-(2*in_fTimeRatio)))/2;
		}
		else
		{
			fEffectRatio = (m_tableExp2->Get(2*(in_fTimeRatio-0.5f))/2) + 0.5f;
		}
		break;
	default:
		AKASSERT(!"Fade curve not supported");
	}
	return (fEffectRatio * (in_fTargetVal-in_fInitialVal)) + in_fInitialVal;

	// *****************************************************************************************************************
	// IMPORTANT: If you modify this function, do not forget to update CAkInterpolation::InterpolateNoCheck() as well!!!
	// *****************************************************************************************************************
}

AkReal32 CAkInterpolation::InterpolateNoCheck(AkReal32 in_fTimeRatio, AkReal32 in_fInitialVal, AkReal32 in_fTargetVal, AkCurveInterpolation in_eFadeCurve)
{
	// **********************************************************************************************************
	// IMPORTANT: If you modify this function, do not forget to update CAkInterpolation::Interpolate() as well!!!
	// **********************************************************************************************************
	
	AkReal32 fEffectRatio = 0;
	switch(in_eFadeCurve)
	{
	case AkCurveInterpolation_Linear:
		fEffectRatio = in_fTimeRatio;
		break;
	case AkCurveInterpolation_Exp1:
		AKASSERT(m_tableExp1);
		fEffectRatio = m_tableExp1->GetNoCheck(in_fTimeRatio);
		break;
	case AkCurveInterpolation_Exp2:
		AKASSERT(m_tableExp2);
		fEffectRatio = m_tableExp2->GetNoCheck(in_fTimeRatio);
		break;
	case AkCurveInterpolation_Exp3:
		AKASSERT(m_tableExp3);
		fEffectRatio = m_tableExp3->GetNoCheck(in_fTimeRatio);
		break;
	case AkCurveInterpolation_Log1:
		AKASSERT(m_tableExp1);
		fEffectRatio = 1 - m_tableExp1->GetNoCheck(1-in_fTimeRatio);
		break;
	case AkCurveInterpolation_Log2:
		AKASSERT(m_tableExp2);
		fEffectRatio = 1 - m_tableExp2->GetNoCheck(1-in_fTimeRatio);
		break;
	case AkCurveInterpolation_Log3:
		AKASSERT(m_tableExp3);
		fEffectRatio = 1 - m_tableExp3->GetNoCheck(1-in_fTimeRatio);
		break;
	case AkCurveInterpolation_SCurve:
		AKASSERT(m_tableExp2);
		if(in_fTimeRatio < 0.5)
		{
			fEffectRatio = m_tableExp2->GetNoCheck(2*in_fTimeRatio)/2;
		}
		else
		{
			fEffectRatio = ((1-m_tableExp2->GetNoCheck(1.0f-(2*(in_fTimeRatio-0.5f))))/2)+0.5f;
		}
		break;
	case AkCurveInterpolation_InvSCurve:
		AKASSERT(m_tableExp2);
		if(in_fTimeRatio < 0.5)
		{
			fEffectRatio = (1 - m_tableExp2->GetNoCheck(1-(2*in_fTimeRatio)))/2;
		}
		else
		{
			fEffectRatio = (m_tableExp2->GetNoCheck(2*(in_fTimeRatio-0.5f))/2) + 0.5f;
		}
		break;
	default:
		AKASSERT(!"Fade curve not supported");
	}
	return (fEffectRatio * (in_fTargetVal-in_fInitialVal)) + in_fInitialVal;

	// **********************************************************************************************************
	// IMPORTANT: If you modify this function, do not forget to update CAkInterpolation::Interpolate() as well!!!
	// **********************************************************************************************************
}

AkReal32 CAkInterpolation::RealTodB(AkReal32 in_fReal)
{
	if(in_fReal <= 0)
	{
		return AK_MINIMUM_VOLUME_LEVEL;
	}
	AkReal32 fCorrector = 0.0f;
 
	while(in_fReal > 10)
	{
		in_fReal/=10;
		fCorrector+=20;
	}
	while(in_fReal < 1)
	{
		in_fReal*=10;
		fCorrector-=20;
	}

	return m_tableRealToDB->Get(in_fReal) + fCorrector;
}
