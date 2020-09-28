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
// AkInterpolationTable.h
//
//////////////////////////////////////////////////////////////////////
#ifndef _INTERPOLATION_TABLE_H_
#define _INTERPOLATION_TABLE_H_

#include <AK/Tools/Common/AkObject.h>

typedef AkReal32 (*AkFillTableFunc)(AkReal64);

//These are the basic functions used to generate the interpolation tables
namespace CalculationFunction
{
	AkReal32 AkExp1(AkReal64 in_fFrom);

	AkReal32 AkExp2(AkReal64 in_fFrom);

	AkReal32 AkExp3(AkReal64 in_fFrom);

	AkReal32 dBToReal(AkReal64 in_fFrom);

	AkReal32 realTodB(AkReal64 in_fFrom);

	AkReal32 linearMutingTodBMuting255(AkReal64 in_fFrom);

	AkReal32 linearMutingTodBMuting96(AkReal64 in_fFrom);
}

class CAkInterpolationTable : public CAkObject
{
public:
	//Constructor
	CAkInterpolationTable(AkFillTableFunc in_pfCalcFunc, AkReal32 in_fMinValue, AkReal32 in_fMaxValue, size_t in_wTableSize);
	
	AKRESULT	Init();
	void		Term();

	AkReal32 Get(AkReal32 in_Key);                // Evaluate table at point
	AkReal32 GetNoCheck(AkReal32 in_Key);         // No range check
	AkReal32 GetNoCheckNoInterp(AkReal32 in_Key); // No range check, no interpolation

private:
	AkReal32*		m_ConversionTable;
	AkFillTableFunc	m_pfCalcFunction;
	AkReal32		m_fMaxValue;
	AkReal32		m_fMinValue;
	size_t			m_wTableSize;
	AkReal32		m_fOneOnScale;
};
#endif
