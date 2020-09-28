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
// AkActionExcept.cpp
//
//////////////////////////////////////////////////////////////////////
#include "stdAfx.h"
#include "AkActionExcept.h"
#include "AkBankFloatConversion.h"

CAkActionExcept::CAkActionExcept(AkActionType in_eActionType, AkUniqueID in_ulID)
: CAkAction(in_eActionType, in_ulID)
{
}

CAkActionExcept::~CAkActionExcept()
{
	m_listElementException.Term();
}

AKRESULT CAkActionExcept::AddException(const AkUniqueID in_ulElementID)
{
	if ( m_listElementException.Exists( in_ulElementID ) )
		return AK_Success;

	return m_listElementException.AddLast( in_ulElementID ) ? AK_Success : AK_Fail;
}

void CAkActionExcept::RemoveException( const AkUniqueID in_ulElementID )
{
	m_listElementException.Remove( in_ulElementID );
}

void CAkActionExcept::ClearExceptions()
{
	m_listElementException.RemoveAll();
}

AKRESULT CAkActionExcept::SetExceptParams(AkUInt8*& io_rpData, AkUInt32& io_rulDataSize )
{
	AkUInt32 ulExceptionListSize = READBANKDATA(AkUInt32, io_rpData, io_rulDataSize);

	m_listElementException.Reserve( ulExceptionListSize );

	for (AkUInt32 i = 0; i < ulExceptionListSize; ++i)
	{
		AkUInt32 ulID = READBANKDATA(AkUInt32, io_rpData, io_rulDataSize);
		if( !m_listElementException.AddLast( ulID ) )
		{
			return AK_Fail;
		}
	}
	return AK_Success;
}
