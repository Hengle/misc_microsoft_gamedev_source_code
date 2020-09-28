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
// ALBytesMem.h
//
// IReadBytes / IWriteBytes implementation on a growing memory buffer.
// This version uses the AudioLib memory pools.
//
////////////////////////////////////////////////////////////////////////////////
#ifndef _AL_BYTES_MEM_H_
#define _AL_BYTES_MEM_H_

#include <AK/IBytes.h>
#include "AkPrivateTypes.h"

class WriteBytesMem
	: public AK::IWriteBytes
{
public:

	WriteBytesMem();
	virtual ~WriteBytesMem();

	// IWriteBytes implementation

	virtual bool WriteBytes( const void * in_pData, AkInt32 in_cBytes, AkInt32 & out_cWritten );

	// Public methods

	bool		Reserve( AkInt32 in_cBytes );
	AkInt32		Count() const;
	AkUInt8 *	Bytes() const;
	AkUInt8 *	Detach();

	void   Clear();

	static void SetMemPool( AkMemPoolId in_pool );

private:
	AkInt32		m_cBytes;
	AkUInt8 *	m_pBytes;

	AkInt32	m_cPos;

	static AkMemPoolId s_pool;
};
#endif
