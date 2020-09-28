//////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006-2008 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

/// \file
/// AK::IReadBytes, AK::IWriteBytes simple serialization interfaces.

#ifndef _AK_IBYTES_H
#define _AK_IBYTES_H

#include <wchar.h>

namespace AK
{
	/// Generic binary input interface.
	/// \warning The functions in this interface are not thread-safe, unless stated otherwise.
	class IReadBytes
	{
	public:
		////////////////////////////////////////////////////////////////////////
		/// @name Interface
		//@{

		/// Reads some bytes into a buffer.
		/// \return	True if the operation was successful, False otherwise
		virtual bool ReadBytes( 
			void * in_pData,		///< Pointer to a buffer
			long in_cBytes,			///< Size of the buffer (in bytes)
			long & out_cRead 		///< Returned number of read bytes
			) = 0;

		//@}

		////////////////////////////////////////////////////////////////////////
		/// @name Helpers
		//@{

		/// Reads a simple type or structure.
		/// \warning Not for object serialization.
		/// \return	True if the operation was successful, False otherwise.
		template<class T>
		bool Read( 
			T & out_data )	///< Data to be read
		{
			long cRead;
			return ReadBytes( &out_data, sizeof( T ), cRead );
		}

		/// Reads a simple type or structure.
		/// \warning This method does not allow for error checking. Use other methods when error cases need to be handled.
		/// \warning Not for object serialization.
		/// \return	Read data
		template<class T>
		T Read()
		{
			T value;

			long cRead;
			ReadBytes( &value, sizeof( T ), cRead );

			return value;
		}

		/// Reads a unicode string into a fixed-size buffer. 
		/// \return	True if the operation was successful, False otherwise. An insufficient buffer size does not cause failure.
		bool ReadString( 
			wchar_t * out_pszString,	///< Pointer to a fixed-size buffer
			long in_nMax )			///< Maximum number of characters to be read in out_pszString, including the terminating NULL character
		{
			long cChars;
			if ( !Read<long>( cChars ) ) 
				return false;

			bool bRet = true;

			if ( cChars > 0 )
			{
				long cRead;

				if ( cChars < in_nMax )
				{
					ReadBytes( out_pszString, cChars * sizeof( wchar_t ), cRead );
					out_pszString[ cChars ] = 0;

					bRet = cRead == ( cChars * sizeof( wchar_t ) );
				}
				else
				{
					ReadBytes( out_pszString, in_nMax * sizeof( wchar_t ), cRead );
					out_pszString[ in_nMax - 1 ] = 0;

					bRet = cRead == ( cChars * sizeof( wchar_t ) );

					if ( bRet )
					{
						// Read extra characters in temp buffer.
						long cRemaining = cChars - in_nMax;

						wchar_t * pTemp = new wchar_t[ cRemaining ];

						ReadBytes( pTemp, cRemaining * sizeof( wchar_t ), cRead );

						bRet = cRemaining == ( cChars * sizeof( wchar_t ) );

						delete [] pTemp;
					}
				}
			}
			else
			{
				out_pszString[ 0 ] = 0;
			}

			return bRet;
		}

		//@}
	};

	/// Generic binary output interface.
	/// \warning The functions in this interface are not thread-safe, unless stated otherwise.
	class IWriteBytes
	{
	public:
		////////////////////////////////////////////////////////////////////////
		/// @name Interface
		//@{

		/// Writes some bytes from a buffer.
		/// \return	True if the operation was successful, False otherwise
		virtual bool WriteBytes( 
			const void * in_pData,	///< Pointer to a buffer
			long in_cBytes, 		///< Size of the buffer (in bytes)
			long & out_cWritten		///< Returned number of written bytes
			) = 0;

		//@}

		////////////////////////////////////////////////////////////////////////
		/// @name Helpers
		//@{

		/// Writes a simple type or struct.
		/// \warning Not for object serialization.
		/// \return	True if the operation was successful, False otherwise
		template<class T>
		bool Write( 
			const T & in_data )		///< Data to be written
		{
			long cWritten;
			return WriteBytes( &in_data, sizeof( T ), cWritten );
		}

		/// Writes a unicode string. 
		/// \return	True if the operation was successful, False otherwise
		bool WriteString( 
			const wchar_t * in_pszString )	///< String to be written
		{
			long cChars = (long) wcslen( in_pszString );
			if ( !Write<long>( cChars ) )
				return false;

			long cWritten = 0;

			if ( cChars > 0 )
			{
				WriteBytes( in_pszString, cChars * sizeof( wchar_t ), cWritten );
			}

			return cWritten == ( cChars * sizeof( wchar_t ) );
		}

		//@}
	};
}

#endif // _AK_IBYTES_H
