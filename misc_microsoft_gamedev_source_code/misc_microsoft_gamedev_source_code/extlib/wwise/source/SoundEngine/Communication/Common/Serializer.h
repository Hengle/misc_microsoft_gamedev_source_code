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


#pragma once

#ifdef AK_WWISE_APP
#include "BytesMem.h"
#else
#include "ALBytesMem.h"
#endif

#include "AkPrivateTypes.h"

class Serializer
{
public:
	Serializer( bool in_bSwapEndian );
	~Serializer();

	AkUInt8* GetWrittenBytes() const;
	AkInt32 GetWrittenSize() const;

	void Deserializing( const AkUInt8* in_pData );
	const AkUInt8* GetReadBytes() const;

	void Reset();

	class AutoSetDataPeeking
	{
	public:
		AutoSetDataPeeking( Serializer& in_rSerializer )
			: m_rSerializer( in_rSerializer )
		{
			m_rSerializer.SetDataPeeking( true );
		}

		~AutoSetDataPeeking()
		{
			m_rSerializer.SetDataPeeking( false );
		}

	private:
		Serializer& m_rSerializer;
	};

	// Template, catch all
	template <class T>
	bool Put( const T& in_rValue );

	template <class T>
	bool Get( T& out_rValue );

	// Basic, known size types.
	bool Put( AkUInt8 in_value );
	bool Get( AkUInt8& out_rValue );
	bool Put( AkInt16 in_value );
	bool Get( AkInt16& out_rValue );
	bool Put( AkInt32 in_value );
	bool Get( AkInt32& out_rValue );
	bool Put( AkUInt16 in_value );
	bool Get( AkUInt16& out_rValue );
	bool Put( AkUInt32 in_value );
	bool Get( AkUInt32& out_rValue );
	bool Put( AkInt64 in_value );
	bool Get( AkInt64& out_rValue );
	bool Put( AkReal32 in_value );
	bool Get( AkReal32& out_rValue );

	// Variable length types
	bool Put( const void* in_pvData, AkInt32 in_size );
	bool Get( void*& out_rpData, AkInt32& out_rSize );
	bool Put( const char* in_pszData );
	bool Get( char*& out_rpszData, AkInt32& out_rSize );

private:
	friend class AutoSetDataPeeking;
	void SetDataPeeking( bool in_bPeekData );

	AkUInt8 Swap( const AkUInt8& in_rValue ) const;
	AkInt16 Swap( const AkInt16& in_rValue ) const;
	AkUInt16 Swap( const AkUInt16& in_rValue ) const;
	AkInt32 Swap( const AkInt32& in_rValue ) const;
	AkUInt32 Swap( const AkUInt32& in_rValue ) const;
	AkInt64 Swap( const AkInt64& in_rValue ) const;
	AkReal32 Swap( const AkReal32& in_rValue ) const;

	WriteBytesMem m_writer;

	const AkUInt8* m_pReadBytes;
	AkUInt32 m_readPos;
	AkUInt32 m_readPosBeforePeeking;

	const bool m_bSwapEndian;
};

template <class T>
bool Serializer::Put( const T& in_rValue )
{
	return in_rValue.Serialize( *this );
}

template <class T>
bool Serializer::Get( T& out_rValue )
{
	return out_rValue.Deserialize( *this );
}
