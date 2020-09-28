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
// AkBankFloatConversion.h
//
//////////////////////////////////////////////////////////////////////
#ifndef _AKFLOAT_CONVERSION_H_
#define _AKFLOAT_CONVERSION_H_

template< typename T > 
inline T ReadBankData( 
						AkUInt8*& in_rptr 
#ifndef AK_OPTIMIZED
						,AkUInt32& in_rSize
#endif
						)
{
	T l_Value = *( ( T* )in_rptr );
	in_rptr += sizeof( T );
#ifndef AK_OPTIMIZED
	in_rSize -= sizeof( T );
#endif
	return l_Value;
}

#ifdef XBOX360
inline AkReal32 AlignFloat( AkReal32* __unaligned ptr )
{
	return *ptr;
}

template<> 
inline AkReal32 ReadBankData<AkReal32>( 
									   AkUInt8*& in_rptr
#ifndef AK_OPTIMIZED
									   ,AkUInt32& in_rSize
#endif
									   )
{
	AkReal32 l_Value = AlignFloat( ( AkReal32* )in_rptr );
	in_rptr += sizeof( AkReal32 );
#ifndef AK_OPTIMIZED
	in_rSize -= sizeof( AkReal32 );
#endif
	return l_Value;
}

inline AkReal64 AlignFloat( AkReal64* __unaligned ptr )
{
	return *ptr;
}

template<> 
inline AkReal64 ReadBankData<AkReal64>( 
									   AkUInt8*& in_rptr
#ifndef AK_OPTIMIZED
									   ,AkUInt32& in_rSize
#endif
									   )
{
	AkReal64 l_Value = AlignFloat( ( AkReal64* )in_rptr );
	in_rptr += sizeof( AkReal64 );
#ifndef AK_OPTIMIZED
	in_rSize -= sizeof( AkReal64 );
#endif
	return l_Value;
}

#endif //XBOX360

#if defined (__PPU__) || defined(RVL_OS)
template < typename TO, typename FROM >
inline TO union_cast( FROM value )
{
    union { FROM from; TO to; } convert;
    convert.from = value;
    return convert.to;
}

inline AkReal32 AlignFloat( AkReal32* ptr )
{
	AkUInt32 *puint = reinterpret_cast<AkUInt32 *>( ptr );
    volatile AkUInt32 uint = *puint;
    return union_cast<AkReal32>( uint );
}

template<> 
inline AkReal32 ReadBankData<AkReal32>( 
									   AkUInt8*& in_rptr
#ifndef AK_OPTIMIZED
									   ,AkUInt32& in_rSize
#endif
									   )
{
	AkReal32 l_Value = AlignFloat( ( AkReal32* )in_rptr );
	in_rptr += sizeof( AkReal32 );
#ifndef AK_OPTIMIZED
	in_rSize -= sizeof( AkReal32 );
#endif
	return l_Value;
}

inline AkReal64 AlignFloat( AkReal64* ptr )
{
	AkUInt64 *puint = reinterpret_cast<AkUInt64 *>( ptr );
    volatile AkUInt64 uint = *puint;
    return union_cast<AkReal64>( uint );
}

template<> 
inline AkReal64 ReadBankData<AkReal64>( 
									   AkUInt8*& in_rptr
#ifndef AK_OPTIMIZED
									   ,AkUInt32& in_rSize
#endif
									   )
{
	AkReal64 l_Value = AlignFloat( ( AkReal64* )in_rptr );
	in_rptr += sizeof( AkReal64 );
#ifndef AK_OPTIMIZED
	in_rSize -= sizeof( AkReal64 );
#endif
	return l_Value;
}
#endif // __PPU__ || RVL_OS


#ifndef AK_OPTIMIZED

#define READBANKDATA( _Type, _Ptr, _Size )		\
		ReadBankData<_Type>( _Ptr, _Size )

#define SKIPBANKDATA( _Type, _Ptr, _Size )		\
		( _Ptr ) += sizeof( _Type );			\
		( _Size ) -= sizeof( _Type )

#define SKIPBANKBYTES( _NumBytes, _Ptr, _Size )	\
		( _Ptr ) += _NumBytes;					\
		( _Size ) -= _NumBytes

#else

#define READBANKDATA( _Type, _Ptr, _Size )		\
		ReadBankData<_Type>( _Ptr )

#define SKIPBANKDATA( _Type, _Ptr, _Size )		\
		( _Ptr ) += sizeof( _Type )

#define SKIPBANKBYTES( _NumBytes, _Ptr, _Size )	\
		( _Ptr ) += _NumBytes;

#endif


#ifndef AK_OPTIMIZED

#define CHECKBANKDATASIZE( _DATASIZE_, _ERESULT_ )		\
	if( _DATASIZE_ != 0 && _ERESULT_ == AK_Success )	\
	{													\
		AKASSERT( _DATASIZE_ == 0 );					\
		_ERESULT_ = AK_Fail;							\
	}

#else
#define CHECKBANKDATASIZE(_DATASIZE_, _ERESULT_ )
#endif


#endif //_AKFLOAT_CONVERSION_H_
