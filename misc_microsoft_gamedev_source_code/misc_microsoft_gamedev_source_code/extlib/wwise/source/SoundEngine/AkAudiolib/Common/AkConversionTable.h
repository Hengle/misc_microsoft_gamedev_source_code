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
// AkConversionTable.h
//
//////////////////////////////////////////////////////////////////////
#ifndef _CONVERSION_TABLE_H_
#define _CONVERSION_TABLE_H_

#include "AkRTPC.h"
#include "AkMath.h"
#include <AK/Tools/Common/AkPlatformFuncs.h>
#include "AkInterpolation.h"

extern AkMemPoolId g_DefaultPoolId;
extern CAkInterpolation g_Interpol;

template <class T_GraphPointType, class Y_Type>
class CAkConversionTable
{
public:

	//Constructor
	CAkConversionTable()
	:m_pArrayGraphPoints( NULL )
	,m_ulArraySize( 0 )
	,m_eScaling( AkCurveScaling_None )
	{
	}

	//Convert the input value into the output value, applying conversion if the converter is available
	AkNoInline Y_Type Convert( AkReal32 in_valueToConvert )
	{
		Y_Type l_returnedValue;
		if( m_pArrayGraphPoints && m_ulArraySize )
		{
			if(m_ulArraySize == 1)
			{
				l_returnedValue = m_pArrayGraphPoints[0].To;
			}
			else
			{
				for( AkUInt32 i = 0; i < m_ulArraySize; ++i )
				{
					if( in_valueToConvert <= m_pArrayGraphPoints[i].From)
					{
						// You will get here if:
						//    * You evaluate before the first point
						//    * or you evaluate exactly on a point
						l_returnedValue = m_pArrayGraphPoints[i].To;
						break;
					}
					else if( ( i < m_ulArraySize - 1 ) && ( in_valueToConvert < m_pArrayGraphPoints[i+1].From ) )
					{
						// You will get here if:
						//    * You evaluate between 2 points (in the "if" we check i first to
						//      make sure there *is* a next point)

						const T_GraphPointType& firstPoint( m_pArrayGraphPoints[i] );
						const T_GraphPointType& secondPoint( m_pArrayGraphPoints[i+1] );
						
						if ( m_pArrayGraphPoints[i].Interp == AkCurveInterpolation_Linear )
						{
							// Linear interpolation between the two points
							l_returnedValue = static_cast<Y_Type>( AkMath::InterpolateNoCheck(
								firstPoint.From,
								static_cast<AkReal32>( firstPoint.To ),
								secondPoint.From,
								static_cast<AkReal32>( secondPoint.To ),
								in_valueToConvert
								) );
						}
						else if ( m_pArrayGraphPoints[i].Interp == AkCurveInterpolation_Constant )
						{
							// Constant interpolation -> Value of the first point
							l_returnedValue = firstPoint.To;
						}
						else
						{
							l_returnedValue = static_cast<Y_Type>( g_Interpol.InterpolateNoCheck(
								( in_valueToConvert - static_cast<AkReal32>( firstPoint.From ) )
									 / ( static_cast<AkReal32>( secondPoint.From ) - static_cast<AkReal32>( firstPoint.From ) ),
								static_cast<AkReal32>( firstPoint.To ),
								static_cast<AkReal32>( secondPoint.To ),
								firstPoint.Interp ) );
						}

						break;
					}
					else if( i == m_ulArraySize - 1 )
					{
						// You will get here if:
						//    * You evaluate after the last point

						//Then we take the last available value
						l_returnedValue = m_pArrayGraphPoints[i].To;

						break;
					}
				}
			}
		}

		switch( m_eScaling )
		{
			case AkCurveScaling_None:
				break;

			case AkCurveScaling_db_255:
				l_returnedValue = static_cast<Y_Type>( g_Interpol.LinearMutingTodBMuting255( static_cast<AkReal32>( l_returnedValue ) ) );
				break;

			case AkCurveScaling_dB_96_3:
				l_returnedValue = static_cast<Y_Type>( g_Interpol.LinearMutingTodBMuting96( static_cast<AkReal32>( l_returnedValue ) ) );
				break;

			default:
				AKASSERT( ! "Unknown scaling mode!" );
		}

		return l_returnedValue;
	}

	AKRESULT Set(
		T_GraphPointType*			in_pArrayConversion,
		AkUInt32						in_ulConversionArraySize,
		AkCurveScaling				in_eScaling
		)
	{
		AKRESULT eResult = AK_InvalidParameter;
		Unset();
		AKASSERT( !m_pArrayGraphPoints );

		if(in_pArrayConversion && in_ulConversionArraySize)
		{
			m_pArrayGraphPoints = (T_GraphPointType*)AkAlloc( g_DefaultPoolId, in_ulConversionArraySize*sizeof(T_GraphPointType) );
			if(m_pArrayGraphPoints)
			{
				AKPLATFORM::AkMemCpy( m_pArrayGraphPoints, in_pArrayConversion, in_ulConversionArraySize*sizeof(T_GraphPointType) );
				m_ulArraySize = in_ulConversionArraySize;
				m_eScaling = in_eScaling;
				eResult = AK_Success;
			}
			else
			{
				eResult = AK_InsufficientMemory;
				m_ulArraySize = 0;
			}
		}
		
		return eResult;
	}

	AKRESULT Unset()
	{
		if(m_pArrayGraphPoints)
		{
			AkFree( g_DefaultPoolId, m_pArrayGraphPoints );
			m_pArrayGraphPoints = NULL;
		}

		m_ulArraySize = 0;
		m_eScaling = AkCurveScaling_None;

		return AK_Success;
	}

	AkReal32 GetMidValue()
	{
		AkReal32 l_DefaultVal = 0;
		if( m_pArrayGraphPoints && m_ulArraySize)
		{
			AkReal32 l_LowerBound = m_pArrayGraphPoints[0].From;
			AkReal32 l_HigherBound = m_pArrayGraphPoints[m_ulArraySize-1].From;
			l_DefaultVal = ( l_LowerBound + l_HigherBound )* 0.5f;
		}
		return l_DefaultVal;
	}

public://public to allow direct access
	T_GraphPointType*			m_pArrayGraphPoints;
	AkUInt32						m_ulArraySize;		//The number of sets of points in the array
	AkCurveScaling				m_eScaling;
};

#endif //_CONVERSION_TABLE_H_
