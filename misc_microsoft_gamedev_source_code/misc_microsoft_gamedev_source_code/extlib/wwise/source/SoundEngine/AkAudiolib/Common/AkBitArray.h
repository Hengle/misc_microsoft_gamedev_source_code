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
// AkBitArray.h
//
// AudioKinetic Lock base class.
//
//////////////////////////////////////////////////////////////////////

#ifndef _AK_BIT_ARRAY_H_
#define _AK_BIT_ARRAY_H_

// Do not move this define(alessard)
#define MAX_BITFIELD_NUM 32

//Allowing to set an array of max 32 bits
//this one is way more performant, and use only 32 bits of space, nothing virtual...
class CAkBitArrayMax32
{
public:
	CAkBitArrayMax32()
		: m_i32BitArray(0)
	{
	}

	void SetBit( AkUInt32 in_BitIndex )
	{
		AKASSERT( in_BitIndex < 32 );
		m_i32BitArray |= (1 << in_BitIndex);
	}

	void UnsetBit( AkUInt32 in_BitIndex )
	{
		AKASSERT( in_BitIndex < 32 );
		m_i32BitArray &= (~(1 << in_BitIndex));
	}

	void Set( AkUInt32 in_BitIndex, bool in_value )
	{
		if( in_value )
		{
			SetBit( in_BitIndex );
		}
		else
		{
			UnsetBit( in_BitIndex );
		}
	}

	bool IsSet( AkUInt32 in_BitIndex )
	{
		AKASSERT( in_BitIndex < 32 );
		return ( m_i32BitArray & (1 << in_BitIndex) )?true:false;
	}

	bool IsEmpty()
	{
		return (m_i32BitArray == 0);
	}

	void ClearBits()
	{
		m_i32BitArray = 0;
	}

private:
	AkUInt32 m_i32BitArray;
};

#endif //_AK_BIT_ARRAY_H_
