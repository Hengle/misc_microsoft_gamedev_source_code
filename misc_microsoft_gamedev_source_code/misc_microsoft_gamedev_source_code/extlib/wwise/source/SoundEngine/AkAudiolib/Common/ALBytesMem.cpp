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

////////////////////////////////////////////////////////////////////////////////
//
// ALBytesMem.cpp
//
// IReadBytes / IWriteBytes implementation on a memory buffer.
//
////////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#include <AK/Tools/Common/AkObject.h>
#include "ALBytesMem.h"
#include <AK/Tools/Common/AkPlatformFuncs.h>

AkMemPoolId WriteBytesMem::s_pool = AK_INVALID_POOL_ID;

void WriteBytesMem::SetMemPool( AkMemPoolId in_pool )
{
	AKASSERT( s_pool == in_pool || s_pool == AK_INVALID_POOL_ID );
	s_pool = in_pool;
}

//------------------------------------------------------------------------------
// WriteBytesMem

WriteBytesMem::WriteBytesMem()
	: m_cBytes( 0 )
	, m_pBytes( NULL )
	, m_cPos( 0 )
{
	AKASSERT( s_pool != AK_INVALID_POOL_ID );
}

WriteBytesMem::~WriteBytesMem()
{
	if ( m_pBytes )
		AkFree( s_pool, m_pBytes );
}

bool WriteBytesMem::WriteBytes( const void * in_pData, AkInt32 in_cBytes, AkInt32& out_cWritten )
{
	Reserve( m_cPos + in_cBytes );

	AKPLATFORM::AkMemCpy( m_pBytes + m_cPos, (void *)in_pData, in_cBytes );

	m_cPos += in_cBytes;
	out_cWritten = in_cBytes;

	return true;
}

bool WriteBytesMem::Reserve( AkInt32 in_cBytes )
{
	static const AkInt kGrowBy = 1024;

	if ( m_cBytes < in_cBytes )
	{
		AkInt32 cBytesOld = m_cBytes;

		AkInt cGrowBlocks = ( in_cBytes + kGrowBy-1 ) / kGrowBy;
		m_cBytes = cGrowBlocks * kGrowBy;

		if ( m_pBytes )
		{
			AkUInt8 * m_pBytesOld = m_pBytes;

			m_pBytes = (AkUInt8 *) AkAlloc( s_pool, m_cBytes );

			AKPLATFORM::AkMemCpy( m_pBytes, m_pBytesOld, cBytesOld );
			AkFree( s_pool, m_pBytesOld );
		}
		else
		{
			m_pBytes = (AkUInt8 *) AkAlloc( s_pool, m_cBytes );
		}
	}

	return true;
}

AkInt32 WriteBytesMem::Count() const
{
	return m_cPos;
}

AkUInt8 * WriteBytesMem::Bytes() const
{
	return m_pBytes;
}

AkUInt8 * WriteBytesMem::Detach()
{
    AkUInt8* pByte = m_pBytes;

    m_pBytes = NULL;
    m_cBytes = 0;
    m_cPos = 0;

    return pByte;
}

void WriteBytesMem::Clear()
{
    m_cPos = 0;

}
