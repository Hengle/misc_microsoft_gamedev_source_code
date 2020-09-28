// File: wmvFile.h
#pragma once

#include <wmsdk.h>
#include <wmsysprf.h>

#ifndef WMFORMAT_SDK_VERSION
#define WMFORMAT_SDK_VERSION WMT_VER_9_0
#endif

class CwmvFile
{
	IWMProfile			*m_pWMProfile;
	IWMWriter			*m_pWMWriter;
	IWMInputMediaProps	*m_pVideoProps;
	IWMProfileManager	*m_pWMProfileManager;
	HDC					m_hwmvDC;
	TCHAR				m_szErrMsg[MAX_PATH];
	DWORD				m_dwVideoInput;
	DWORD				m_dwCurrentVideoSample;
	QWORD				m_msVideoTime;
	float				m_FrameRate;				// Frames Per Second Rate (FPS)

	int					m_nAppendFuncSelector;		//0=Dummy	1=FirstTime	2=Usual

	HRESULT	AppendFrameFirstTime(int, int, LPVOID,int, double);
	HRESULT	AppendFrameUsual(int, int, LPVOID,int, double );
	HRESULT	AppendDummy(int, int, LPVOID,int, double );
	HRESULT	(CwmvFile::*pAppendFrameBits[3])(int, int, LPVOID,int, double );

	/// Takes care of creating the memory, streams, compression options etc. required for the movie
	HRESULT InitMovieCreation(int nFrameWidth, int nFrameHeight, int nBitsPerPixel);

	/// Takes care of releasing the memory and movie related handles
	void ReleaseMemory();

	/// Sets the Error Message
	void SetErrorMessage(LPCTSTR lpszErrMsg);

public:
	/// <Summary>
	/// Constructor accepts the filename, ProfileGUID and frame rate settings
	/// as parameters.
	/// lpszFileName: Name of the output movie file to create
	/// guidProfileID: GIUD of the Video Profile to be used for compression and other Settings
	/// dwFrameRate: The Frames Per Second (FPS) setting to be used for the movie
	/// </Summary>
	CwmvFile(LPCTSTR lpszFileName = ("Output.wmv"),
			const GUID& guidProfileID = WMProfile_V80_384Video,
			float frameRate = 1,
			IWMProfile* pWMProfile = NULL);

	/// <Summary> 
	/// Destructor closes the movie file and flushes all the frames
	/// </Summary>
	~CwmvFile(void);
	
	/// </Summary>
	/// Inserts the given bitmap bits into the movie as a new Frame at the end.
	/// The width, height and nBitsPerPixel are the width, height and bits per pixel
	/// of the bitmap pointed to by the input pBits.
	/// </Summary>
	HRESULT	AppendNewFrame(int nWidth, int nHeight, LPVOID pBits,int nBitsPerPixel=32, double frameTime = -1.0f);

	/// <Summary>
	/// Returns the last error message, if any.
	/// </Summary>
	LPCTSTR GetLastErrorMessage() const {	return m_szErrMsg;	}
};
