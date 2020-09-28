/***********************************************************************
  The content of this file includes source code for the sound engine
  portion of the AUDIOKINETIC Wwise Technology and constitutes "Level
  Two Source Code" as defined in the Source Code Addendum attached
  with this file.  Any use of the Level Two Source Code shall be
  subject to the terms and conditions outlined in the Source Code
  Addendum and the End User License Agreement for Wwise(R).

  Version: v2008.2.1 Patch 4  Build: 2821
  Copyright (c) 2006-2008 Audiokinetic Inc.
 ***********************************************************************/

//////////////////////////////////////////////////////////////////////
//
// AkBankReader.cpp
//
//////////////////////////////////////////////////////////////////////
#include "StdAfx.h"
#include "AkBankReader.h"
#include "AkBankMgr.h"
#include "AkAudiolib.h"

extern AkMemPoolId g_DefaultPoolId;

// On the Wii, the buffer used to access IO must be aligned on 32 byted boundaries.
// this is not a requirement on others platforms up to now.
// The bank manager does allocate its own buffer to read IO, and this buffer must be aligned too.
#define AK_BANK_READER_BUFFER_ALIGNMENT (32)

//This buffer must be aligned on a multiple of AK_BANK_READER_BUFFER_ALIGNMENT
#define AK_DEFAULT_BNK_IO_BUFFER_SIZE   (32*1024)

CAkBankReader::CAkBankReader()
:m_pBuffer(NULL)
,m_pReadBufferPtr(NULL)
,m_ulRemainingBytes(0)
,m_bIsInitDone(false)
,m_pUserReadBuffer(NULL)
,m_ulDeviceBlockSize(0)
,m_pStream(NULL)
,m_pBankReaderOwner(NULL)
,m_ulBufferSize(0)
,m_pInMemoryBankReaderPtr(NULL)
{
}

CAkBankReader::~CAkBankReader()
{
}

AKRESULT CAkBankReader::Init(CAkBankMgr* pBankManager)
{
	AKASSERT(pBankManager);
	AKASSERT(!m_bIsInitDone);

	m_pBankReaderOwner = pBankManager;

	if(m_bIsInitDone)
	{
		return AK_Fail;
	}

    m_ulRemainingBytes = 0;

    m_ulBufferSize = 0;
    m_pBuffer = NULL;
    
    m_pReadBufferPtr = m_pBuffer;
    m_bIsInitDone = true;

    m_fThroughput = AK_DEFAULT_BANK_THROUGHPUT;
    m_priority = AK_DEFAULT_BANK_IO_PRIORITY;
    
    return AK_Success;
}

void CAkBankReader::Reset()
{
	AKASSERT( !m_pUserReadBuffer );
	m_ulRemainingBytes = 0;
    m_pReadBufferPtr = m_pBuffer;
	m_pInMemoryBankReaderPtr = NULL;
}

AKRESULT CAkBankReader::Term()
{
	if(m_pBuffer)
	{
		AK::MemoryMgr::Falign(g_DefaultPoolId, m_pBuffer);
		m_pBuffer = NULL;	//MUST set it to NULL
	}
	m_pReadBufferPtr = NULL;
	return AK_Success;
}

AKRESULT CAkBankReader::SetBankLoadIOSettings(AkReal32 in_fThroughput, AkPriority in_priority)
{
    if ( in_fThroughput < 0 ||
         in_priority < AK_MIN_PRIORITY ||
         in_priority > AK_MAX_PRIORITY )
    {
        AKASSERT( !"Invalid bank I/O settings" );
        return AK_InvalidParameter;
    }

    m_fThroughput = in_fThroughput;
    m_priority = in_priority;
    return AK_Success;
}

AKRESULT CAkBankReader::SetFile( AkTChar* in_pwChar, AkUInt32 in_uFileOffset )
{
	AKASSERT( m_pStream == NULL );
    
    // Try open the file in the language specific directory.
    AkFileSystemFlags fsFlags;
    fsFlags.uCompanyID = AKCOMPANYID_AUDIOKINETIC;
    fsFlags.uCodecID = AKCODECID_BANK;
    fsFlags.bIsLanguageSpecific = true;
    fsFlags.pCustomParam = NULL;
    fsFlags.uCustomParamSize = 0;

    AKRESULT eResult =  AK::IAkStreamMgr::Get( )->CreateStd( 
                                            in_pwChar, 
                                            &fsFlags,
                                            AK_OpenModeRead,
                                            m_pStream );
    if ( eResult != AK_Success )
    {
        // Perhaps file was not found. Try open the bank in the common (non-language specific) section.
        fsFlags.bIsLanguageSpecific = false;
        eResult =  AK::IAkStreamMgr::Get( )->CreateStd( 
                                            in_pwChar, 
                                            &fsFlags,
                                            AK_OpenModeRead,
                                            m_pStream );
    }
    if ( eResult == AK_Success )
    {
        // If profiling, name the stream with the file name.
#ifndef AK_OPTIMIZED
        m_pStream->SetStreamName( in_pwChar );
#endif

        // Adapt to low-level I/O block size.
        m_ulDeviceBlockSize = m_pStream->GetBlockSize( );
        if ( m_ulDeviceBlockSize == 0 )
        {
            AKASSERT( !"Invalid IO block size" );
            return AK_Fail;
        }

        // Reallocate buffer in the extreme case where it is smaller than the device block size.
        if ( m_ulBufferSize < m_ulDeviceBlockSize )
        {
            if ( m_pBuffer )
                AK::MemoryMgr::Falign(g_DefaultPoolId, m_pBuffer);
            m_ulBufferSize = AkMax(m_ulDeviceBlockSize, AK_DEFAULT_BNK_IO_BUFFER_SIZE);
			m_pBuffer = (AkUInt8*)AK::MemoryMgr::Malign( g_DefaultPoolId, m_ulBufferSize, AK_BANK_READER_BUFFER_ALIGNMENT );// Must be aligned on 32 bytes since it will be used to stream RVL_OS I/O
        }
        if ( !m_pBuffer )
        {
            return AK_InsufficientMemory;
        }

		if( in_uFileOffset != 0 )
		{
			AkInt64 lRealOffset = 0;
			eResult = m_pStream->SetPosition( in_uFileOffset, AK_MoveBegin, &lRealOffset );
			if( eResult == AK_Success )
			{
				AkUInt32 correction = in_uFileOffset - (AkUInt32)(lRealOffset);

				AkUInt32 uSkipped;
				eResult = Skip( correction, uSkipped );

				if( uSkipped != correction )
					eResult = AK_Fail;
			}
		}
    }
    return eResult;
}

AKRESULT CAkBankReader::SetFile( void* in_pInMemoryFile, AkUInt32 in_ui32FileSize )
{
	AKASSERT( m_pStream == NULL );

	m_pInMemoryBankReaderPtr = static_cast<AkUInt8*>( in_pInMemoryFile );
	m_ulRemainingBytes = in_ui32FileSize;

	return AK_Success;
}

AKRESULT CAkBankReader::SetFile( AkFileID in_FileID, AkUInt32 in_uFileOffset, AkUInt32 in_codecID )
{
	AKASSERT( m_pStream == NULL );

    // Try open the file in the language specific directory.
    AkFileSystemFlags fsFlags;
    fsFlags.uCompanyID = AKCOMPANYID_AUDIOKINETIC;
    fsFlags.uCodecID = in_codecID;
    fsFlags.bIsLanguageSpecific = true;
    fsFlags.pCustomParam = NULL;
    fsFlags.uCustomParamSize = 0;

    AKRESULT eResult =  AK::IAkStreamMgr::Get( )->CreateStd( 
                                            in_FileID, 
                                            &fsFlags,
                                            AK_OpenModeRead,
                                            m_pStream );
    if ( eResult != AK_Success )
    {
        // Perhaps file was not found. Try open the bank in the common (non-language-specific) section.
        fsFlags.bIsLanguageSpecific = false;
        eResult =  AK::IAkStreamMgr::Get( )->CreateStd( 
                                            in_FileID, 
                                            &fsFlags,
                                            AK_OpenModeRead,
                                            m_pStream );
    }
    if ( eResult == AK_Success )
    {
        // If profiling, compute a name for the stream.
#ifndef AK_OPTIMIZED
        AkTChar szProfileStreamName[32];
#if defined(WIN32) || defined(XBOX360)
		wsprintf( szProfileStreamName, L"FileID: %u", in_FileID );
#else
		swprintf( szProfileStreamName, 32, L"FileID: %u", in_FileID );
#endif
        m_pStream->SetStreamName( szProfileStreamName );
#endif

        // Adapt to low-level I/O block size.
        m_ulDeviceBlockSize = m_pStream->GetBlockSize( );
        if ( m_ulDeviceBlockSize == 0 )
        {
            AKASSERT( !"Invalid IO block size" );
            return AK_Fail;
        }

        // Reallocate buffer in the extreme case where it is smaller than the device block size.
        if ( m_ulBufferSize < m_ulDeviceBlockSize )
        {
            if ( m_pBuffer )
                AK::MemoryMgr::Falign(g_DefaultPoolId, m_pBuffer);
            m_ulBufferSize = AkMax(m_ulDeviceBlockSize, AK_DEFAULT_BNK_IO_BUFFER_SIZE);
            m_pBuffer = (AkUInt8*)AK::MemoryMgr::Malign(g_DefaultPoolId, m_ulBufferSize, AK_BANK_READER_BUFFER_ALIGNMENT);
        }
        if ( !m_pBuffer )
        {
            return AK_InsufficientMemory;
        }

		if( in_uFileOffset != 0 )
		{
			AkInt64 lRealOffset = 0;
			eResult = m_pStream->SetPosition( in_uFileOffset, AK_MoveBegin, &lRealOffset );
			if( eResult == AK_Success )
			{
				AkUInt32 correction = in_uFileOffset - (AkUInt32)(lRealOffset);

				AkUInt32 uSkipped;
				eResult = Skip( correction, uSkipped );

				if( uSkipped != correction )
					eResult = AK_Fail;
			}
		}
    }
    return eResult;
}

AKRESULT CAkBankReader::CloseFile()
{
    if ( m_pStream != NULL )
    {
        // Note. Unsafe close, because we are sure there is no pending IO.
        m_pStream->Destroy( );
        m_pStream = NULL;
    }

	m_pInMemoryBankReaderPtr = NULL;
	m_ulRemainingBytes = 0;

    return AK_Success;
}

void * CAkBankReader::GetData( AkUInt32 in_uSize )
{
	AKRESULT eResult = AK_Success;

	AKASSERT( m_pStream || m_pInMemoryBankReaderPtr );
	AKASSERT(m_pBankReaderOwner);
	AKASSERT( !m_pUserReadBuffer );

	if( m_pStream )
	{
		if ( in_uSize <= m_ulRemainingBytes )
		{
			void * pOut = m_pReadBufferPtr;

			m_pReadBufferPtr += in_uSize;
			m_ulRemainingBytes -= in_uSize;

			return pOut;

		}
		else
		{
			m_pUserReadBuffer = AkAlloc( g_DefaultPoolId, in_uSize );
			if( m_pUserReadBuffer )
			{
				AkUInt32 uSizeRead;
				if ( FillData( m_pUserReadBuffer, in_uSize, uSizeRead ) != AK_Success 
					|| uSizeRead != in_uSize )
				{
					AkFree( g_DefaultPoolId, m_pUserReadBuffer );
					m_pUserReadBuffer = NULL;
				}
			}

			return m_pUserReadBuffer;
		}
	}
	else //Using in-memory file
	{
		AkUInt32 uSizeRead = AkMin( in_uSize, m_ulRemainingBytes );
		m_ulRemainingBytes -= uSizeRead;

		void * pOut = m_pInMemoryBankReaderPtr;

		m_pInMemoryBankReaderPtr += uSizeRead;

		return pOut;
	}
}

void CAkBankReader::ReleaseData()
{
	if ( m_pUserReadBuffer )
	{
		AkFree( g_DefaultPoolId, m_pUserReadBuffer );
		m_pUserReadBuffer = NULL;
	}
}

AKRESULT CAkBankReader::FillData(void* in_pBufferToFill, AkUInt32 in_ulSizeToRead, AkUInt32& out_rulSizeRead
#ifdef RVL_OS
		,bool in_bFlushCacheLineAfterCopyingData
#endif								 
								 )
{
	AKRESULT eResult = AK_Success;

	AKASSERT( m_pStream || m_pInMemoryBankReaderPtr );
	AKASSERT(m_pBankReaderOwner);
	

	AkUInt8* pWhereToWrite = static_cast<AkUInt8*>(in_pBufferToFill);

	out_rulSizeRead = 0;
	if( m_pStream )
	{
        while(in_ulSizeToRead > 0)
		{
			if(m_pBankReaderOwner->IsCancelled())
			{
				eResult = AK_Cancelled;
				break;
			}

			if(m_ulRemainingBytes)
			{
				AKASSERT( m_pReadBufferPtr != NULL );

				AkUInt32 ulNumRead = AkMin(m_ulRemainingBytes, in_ulSizeToRead );

				AKPLATFORM::AkMemCpy(pWhereToWrite, m_pReadBufferPtr, ulNumRead);
#ifdef RVL_OS
				if( in_bFlushCacheLineAfterCopyingData )
					DCStoreRange(pWhereToWrite, ulNumRead);
#endif	
				

				m_pReadBufferPtr += ulNumRead;
				pWhereToWrite += ulNumRead;
				out_rulSizeRead += ulNumRead;
				m_ulRemainingBytes -= ulNumRead;
				in_ulSizeToRead -= ulNumRead;
			}
			else
			{
				if(in_ulSizeToRead >= m_ulBufferSize)
				{
					// Could be anything. Snap to sector size.
					AkUInt32 ulSizeToRead = AkUInt32(in_ulSizeToRead / m_ulDeviceBlockSize) * m_ulDeviceBlockSize;

					// Read into user buffer.
					AkUInt32 ulActualSizeRead;
					eResult =  m_pStream->Read( pWhereToWrite, 
                                                ulSizeToRead, 
                                                true, 
                                                m_priority,
                                                ( ulSizeToRead / m_fThroughput ), // deadline (s) = size (bytes) / throughput (bytes/s).
                                                ulActualSizeRead );
					if ( eResult != AK_Success ||
						 m_pStream->GetStatus( ) != AK_StmStatusCompleted )
					{
						AKASSERT( !"IO error" );
						return eResult;
					}

					// Update pointers and sizes.
					pWhereToWrite += ulActualSizeRead;
					out_rulSizeRead += ulActualSizeRead;
					in_ulSizeToRead -= ulActualSizeRead;

					// Check if end of file unexpectedly encountered.
					if(in_ulSizeToRead >= m_ulBufferSize)
					{
						// Still too much data to read.
						bool bReachedEof;
						m_pStream->GetPosition(&bReachedEof);
						AKASSERT( !bReachedEof || !"Unexpected end of bank file" );
						AKASSERT( bReachedEof || !"IO error" );
						return AK_Fail;
					}

				}
				else
				{
					// Read into BankReader buffer.
                    // Snap to sector size in case buffer size does not conform to IO block size.
					AkUInt32 ulSizeToRead = AkUInt32(m_ulBufferSize / m_ulDeviceBlockSize) * m_ulDeviceBlockSize;

					eResult =  m_pStream->Read( m_pBuffer, 
                                                ulSizeToRead, 
                                                true, 
                                                m_priority,
                                                ( ulSizeToRead / m_fThroughput ), // deadline (s) = size (bytes) / throughput (bytes/s).
                                                m_ulRemainingBytes );
					if ( eResult != AK_Success ||
						m_pStream->GetStatus( ) != AK_StmStatusCompleted )
					{
						AKASSERT( !"IO error" );
						return eResult;
					}

					m_pReadBufferPtr = m_pBuffer;

					// Leave if there was no more data.
					if(!m_ulRemainingBytes)
					{
						break;
					}
				}
			}
		}
	}
	else //Using in-memory file
	{
		out_rulSizeRead = AkMin( in_ulSizeToRead, m_ulRemainingBytes );
		m_ulRemainingBytes -= out_rulSizeRead;
		AKPLATFORM::AkMemCpy( in_pBufferToFill, m_pInMemoryBankReaderPtr, out_rulSizeRead );
		m_pInMemoryBankReaderPtr += out_rulSizeRead;
	}
	return eResult;
}

AKRESULT CAkBankReader::FillDataEx(void* in_pBufferToFill, AkUInt32 in_ulSizeToRead)
{
	AkUInt32 ulSizeRead = 0;

	AKRESULT eResult = FillData(in_pBufferToFill, in_ulSizeToRead, ulSizeRead);
	if(eResult == AK_Success && in_ulSizeToRead != ulSizeRead)
	{
		eResult = AK_BankReadError;
	}
	return eResult;
}

AKRESULT CAkBankReader::Skip(AkUInt32 in_ulNumBytesToSkip, AkUInt32& out_rulSizeSkipped)
{
	AKRESULT eResult = AK_Success;
	AKASSERT(m_pStream || m_pInMemoryBankReaderPtr );

	out_rulSizeSkipped = 0;
	if( m_pStream )
	{
        while(in_ulNumBytesToSkip > 0)
		{
			if(m_ulRemainingBytes)
			{
				AkUInt32 ulNumSkipped = AkMin( m_ulRemainingBytes, in_ulNumBytesToSkip );

				m_pReadBufferPtr += ulNumSkipped;
				out_rulSizeSkipped += ulNumSkipped;
				m_ulRemainingBytes -= ulNumSkipped;
				in_ulNumBytesToSkip -= ulNumSkipped;
			}
			else
			{
				if ( in_ulNumBytesToSkip > m_ulBufferSize )
				{
					// Big amount of bytes to skip. We might as well move the file pointer.
					// Since we opened the stream with unbuffered IO flag, we need to
					// monitor the number of bytes actually skipped (might have snapped to
					// a sector boundary).
					AkInt64 lRealOffset;
					AkInt64 lNumBytesMove = {in_ulNumBytesToSkip};
					eResult = m_pStream->SetPosition( lNumBytesMove,
												  	  AK_MoveCurrent,
													  &lRealOffset );
					if ( eResult != AK_Success )
					{
						AKASSERT( !"Bank load: End of file unexpectedly reached" );

						break;
					}

					out_rulSizeSkipped += (AkUInt32)lRealOffset;
					in_ulNumBytesToSkip -= (AkUInt32)lRealOffset;
				}
				else
				{
                    // Size to skip is smaller than the internal buffer.
                    // Read into buffer, and move pointers.
					// Snap to sector size in case buffer size does not conform to IO block size.
					AkUInt32 ulSizeToRead = AkUInt32(m_ulBufferSize / m_ulDeviceBlockSize) * m_ulDeviceBlockSize;
					eResult =  m_pStream->Read( m_pBuffer, 
                                                ulSizeToRead, 
                                                true, 
                                                m_priority,
                                                ( ulSizeToRead / m_fThroughput ), // deadline (s) = size (bytes) / throughput (bytes/s).
                                                m_ulRemainingBytes );
					if ( eResult != AK_Success ||
						m_pStream->GetStatus( ) != AK_StmStatusCompleted )
					{
						AKASSERT( !"IO error" );
						return eResult;
					}

					// Fail if stream unexpectedly finished before the skip size.
					bool bEndOfStream;
					m_pStream->GetPosition( &bEndOfStream );
					if ( bEndOfStream && 
						 m_ulRemainingBytes < in_ulNumBytesToSkip )
					{
						AKASSERT( !"Bank load: End of file unexpectedly reached" );
						return AK_Fail;
					}

                    m_pReadBufferPtr = m_pBuffer + in_ulNumBytesToSkip;
                    out_rulSizeSkipped += in_ulNumBytesToSkip;
                    m_ulRemainingBytes -= in_ulNumBytesToSkip;
				    in_ulNumBytesToSkip = 0;

				}
			}
		}
	}
	else //Using in-memory file
	{
		out_rulSizeSkipped = AkMin( in_ulNumBytesToSkip, m_ulRemainingBytes );
		m_ulRemainingBytes -= out_rulSizeSkipped;
		m_pInMemoryBankReaderPtr += out_rulSizeSkipped;
	}
	return eResult;
}
