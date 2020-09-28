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
// AkBankReader.h
//
//////////////////////////////////////////////////////////////////////
#ifndef _DOUBLE_BUFFER_H_
#define _DOUBLE_BUFFER_H_

#include <AK/SoundEngine/Common/IAkStreamMgr.h>

class CAkBankMgr;

//////////////////////////////////////////////////////////////////////
//
// CAkBankReader
//
//////////////////////////////////////////////////////////////////////
class CAkBankReader
{
public:
	//Constructor and destructor
	CAkBankReader();
	~CAkBankReader();

	// Init and term
	// Use init only the first time, then only use reset, but when term was called
	AKRESULT Init(CAkBankMgr* pBankManager);
	AKRESULT Term();

	//Use init only the first time, then only use reset, but when term was called
	void Reset();

    // Bank loading I/O settings.
	AKRESULT SetBankLoadIOSettings(AkReal32 in_fThroughput, AkPriority in_priority);

	//Dont change the file while in process, please!
	AKRESULT SetFile( AkTChar* in_pwChar, AkUInt32 in_uFileOffset );
	AKRESULT SetFile( void* in_dInMemoryFile, AkUInt32 in_ui32FileSize );
	AKRESULT SetFile( AkFileID in_FileID, AkUInt32 in_uFileOffset, AkUInt32 in_codecID );
	AKRESULT CloseFile();

	
//Utilities

	// Return - AKRESULT - 
	// IMPORTANT NOTE, the result may be Ak_Success even if the read is incomplete, please rely on out_rulSizeRead
	// To ensure the the read is complete
	AKRESULT FillData(
		void* in_pBufferToFill,	// Pointer to the buffer to fill, must be at least in_ulSizeToRead bytes long
		AkUInt32 in_ulSizeToRead,	// Number of bytes to read
		AkUInt32& out_rulSizeRead	// Number of bytes successfully read
#ifdef RVL_OS
		,bool in_bFlushCacheLineAfterCopyingData = false
#endif
		);

	// Return - AKRESULT - 
	// IMPORTANT NOTE, Same as FillData(), but consider as an error if out_rulSizeRead != in_ulSizeToRead
	AKRESULT FillDataEx(
		void* in_pBufferToFill,
		AkUInt32 in_ulSizeToRead
		);

	// Return - AKRESULT - 
	// IMPORTANT NOTE, the result may be Ak_Success even if the read is incomplete, please rely on out_rulSizeRead
	// To ensure the the read is complete
	AKRESULT Skip(
		AkUInt32 in_ulNumBytesToSkip,	//Number of bytes to skip
		AkUInt32& out_rulSizeSkipped		//Number of byte succesfuly skipped
		);

	void * GetData( AkUInt32 in_uSize );
	void ReleaseData();

private:


//members
	AkUInt8* m_pBuffer;
	AkUInt8* m_pReadBufferPtr;
	AkUInt32 m_ulRemainingBytes;

	AkUInt32 m_ulBufferSize;
    AkUInt32 m_ulDeviceBlockSize;

	void * m_pUserReadBuffer; // Buffer allocated by GetData and released by ReleaseData

	AkUInt8* m_pInMemoryBankReaderPtr;

    AK::IAkStdStream * m_pStream;
    AkReal32 m_fThroughput;
    AkPriority m_priority;

	bool m_bIsInitDone;
	
	CAkBankMgr* m_pBankReaderOwner;
};

#endif //_DOUBLE_BUFFER_H_

