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
// AkInterpolationTable.cpp
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "AkInterpolationTable.h"
#include "math.h"
#include "AudioLibDefs.h"     // Pool IDs definition.

extern AkMemPoolId g_DefaultPoolId;

#define AKROUND( in_dblValue ) \
	( in_dblValue < 0 ? ceil( in_dblValue - 0.5 ) : floor( in_dblValue + 0.5 ) )

//====================================================================================================
//====================================================================================================
CAkInterpolationTable::CAkInterpolationTable(AkFillTableFunc in_pfCalcFunc, AkReal32 in_fMinValue, AkReal32 in_fMaxValue, size_t in_wTableSize)
:m_fMaxValue(in_fMaxValue)
,m_fMinValue(in_fMinValue)
,m_pfCalcFunction(in_pfCalcFunc)
,m_wTableSize(in_wTableSize)
,m_ConversionTable(NULL)
{
}
//====================================================================================================
//====================================================================================================
AKRESULT CAkInterpolationTable::Init()
{
	AkReal64 dblScale = (AkReal64)(m_fMaxValue-m_fMinValue)/(m_wTableSize-1);
	m_fOneOnScale = (AkReal32)(1.0/dblScale);
	m_ConversionTable = (AkReal32*)AkAlloc( g_DefaultPoolId,AkUInt32(m_wTableSize*sizeof(AkReal32)) );
	if( m_ConversionTable )
	{
		for(size_t i = 0; i < m_wTableSize; ++i)
		{
			m_ConversionTable[i] = m_pfCalcFunction(m_fMinValue + (i * dblScale));
		}
		return AK_Success;
	}
	else
	{
		return AK_Fail;
	}
}
//====================================================================================================
//====================================================================================================
void CAkInterpolationTable::Term()
{
	if( m_ConversionTable )
	{
		AkFree(g_DefaultPoolId,m_ConversionTable);
		m_ConversionTable = NULL;
	}
}
//====================================================================================================
//====================================================================================================
AkReal32 CAkInterpolationTable::Get(AkReal32 in_fKey)
{
	AkReal32 fConvertedValue;

	// over the top ?
	if(in_fKey >= m_fMaxValue)
	{
		fConvertedValue = m_ConversionTable[m_wTableSize-1];
	}
	// lower than table ?
	else if(in_fKey <= m_fMinValue)
	{
		fConvertedValue = m_ConversionTable[0];
	}
	// get value from conversion table
	else
	{
		fConvertedValue = GetNoCheck( in_fKey );
	}

	return fConvertedValue;
}
//====================================================================================================
//====================================================================================================
AkReal32 CAkInterpolationTable::GetNoCheck(AkReal32 in_fKey)
{
	// This function assumes the caller already checked that in_fKey is
	// in the range of our table
	AKASSERT( in_fKey >= m_fMinValue && in_fKey <= m_fMaxValue );

	AkReal32 fPos = (in_fKey - m_fMinValue)*m_fOneOnScale;

	// Use unsigned int to avoid sign extend instruction on PS3
	AkUInt32 uLowPos = static_cast<AkUInt32> ( fPos );

	// get converted value
	AkReal32 fConvertedValue = m_ConversionTable[uLowPos];

	AkReal32 Temp1;
	AkReal32 Temp2;
	// convert to index in conversiontable
	AkReal32 fLowPos = static_cast<AkReal32> (uLowPos);

	Temp1 = fPos - fLowPos;
	Temp2 = m_ConversionTable[uLowPos+1];
	Temp2 -= fConvertedValue;
	Temp1 *= Temp2;
	fConvertedValue += Temp1;

	return fConvertedValue;
}
AkReal32 CAkInterpolationTable::GetNoCheckNoInterp(AkReal32 in_fKey)
{
	// This function assumes the caller already checked that in_fKey is
	// in the range of our table
	AKASSERT( in_fKey >= m_fMinValue && in_fKey <= m_fMaxValue );

	AkReal32 fPos = (in_fKey - m_fMinValue)*m_fOneOnScale;

	// Use unsigned int to avoid sign extend instruction on PS3
	AkUInt32 uLowPos = static_cast<AkUInt32> ( fPos );

	// get converted value
	AkReal32 fConvertedValue = m_ConversionTable[uLowPos];

	return fConvertedValue;
}
//====================================================================================================
//====================================================================================================
AkReal32 CalculationFunction::AkExp1(AkReal64 in_fFrom)
{
	return (AkReal32) pow(in_fFrom, 1.41);
}
//====================================================================================================
//====================================================================================================
AkReal32 CalculationFunction::AkExp2(AkReal64 in_fFrom)
{
	return (AkReal32) pow(in_fFrom, 2);
}
//====================================================================================================
//====================================================================================================
AkReal32 CalculationFunction::AkExp3(AkReal64 in_fFrom)
{
	return (AkReal32) pow(in_fFrom, 3);
}
//====================================================================================================
//====================================================================================================
AkReal32 CalculationFunction::dBToReal(AkReal64 in_fFrom)
{
	return (AkReal32) pow( (AkReal64) 10, in_fFrom/20);
}
//====================================================================================================
//====================================================================================================
AkReal32 CalculationFunction::realTodB(AkReal64 in_fFrom)
{
	return (AkReal32) ( 20*log10(in_fFrom) );
}
//====================================================================================================
//====================================================================================================
AkReal32 CalculationFunction::linearMutingTodBMuting255(AkReal64 in_fFrom)
{
	if ( in_fFrom <= 0.0f )
		return 0.0f;
	else if ( in_fFrom >= 255.0f )
		return 255.0f;
	else
		return (AkReal32)(( 20.0 * log10( in_fFrom / 255.0 ) + 96.3 ) / 96.3 * 255.0 );
}
//====================================================================================================
//====================================================================================================
AkReal32 CalculationFunction::linearMutingTodBMuting96(AkReal64 in_fFrom)
{
	if ( in_fFrom == 0.0f )
		return 0.0f;
	else if ( in_fFrom >= 96.3f )
		return 96.3f;
	else if ( in_fFrom <= -96.3f )
		return -96.3f;
	else if ( in_fFrom < 0 )
		return realTodB( ( in_fFrom + 96.3 ) / 96.3 );
	else // in_fFrom > 0 )
		return - realTodB( ( - in_fFrom + 96.3 ) / 96.3 );
}

