// File: wmvFile.cpp
// Originally from: http://www.codeproject.com/KB/graphics/createmovie.aspx
// IMPORTANT: Requires the Windows Media 11 SDK to build: 
// \\esfile\phoenix\Programming\Windows Media 11 SDK

#include "xcore.h"
#include "wmvfile.h"
#include <mbstring.h>

#pragma comment(lib, "wmvcore.lib")

#ifndef __countof
#define __countof(x)	((sizeof(x)/sizeof(x[0])))
#endif

CwmvFile::	CwmvFile(LPCTSTR lpszFileName /*= _T("Output.wmv")*/,
			const GUID& guidProfileID /*= WMProfile_V80_384Video*/,
			float frameRate /*= 1*/,
			IWMProfile* pWMProfile)

{
	CoInitialize(NULL);

	HRESULT	hr=E_FAIL;
    GUID    guidInputType;
	DWORD	dwInputCount=0;
    IWMInputMediaProps* pInputProps = NULL;
	IWMProfileManager2	*pProfileManager2=NULL;

	m_hwmvDC				= NULL;
	m_pWMWriter				= NULL;
	m_dwVideoInput			= 0;
	m_msVideoTime			= 0;
	m_pWMProfile			= NULL;
	m_pVideoProps			= NULL;
	m_pWMProfileManager		= NULL;
	m_dwCurrentVideoSample	= 0;
	m_FrameRate			= frameRate;

	strcpy_s(m_szErrMsg, sizeof(m_szErrMsg), "Method Succeeded");
	m_szErrMsg[__countof(m_szErrMsg)-1] = ('\0');

	pAppendFrameBits[0]	= &CwmvFile::AppendDummy;
	pAppendFrameBits[1]	= &CwmvFile::AppendFrameFirstTime;
	pAppendFrameBits[2]	= &CwmvFile::AppendFrameUsual;

	m_nAppendFuncSelector=0;		//Point to DummyFunction

	if(FAILED(WMCreateProfileManager(&m_pWMProfileManager)))
	{
		SetErrorMessage("Unable to Create WindowsMedia Profile Manager");
		goto TerminateConstructor;
	}

	if(FAILED(m_pWMProfileManager->QueryInterface(IID_IWMProfileManager2,(void**)&pProfileManager2)))
	{
		SetErrorMessage("Unable to Query Interface for ProfileManager2");
		goto TerminateConstructor;
	}

	hr=pProfileManager2->SetSystemProfileVersion(WMFORMAT_SDK_VERSION);

	pProfileManager2->Release();

	if(FAILED(hr))
	{
		SetErrorMessage("Unable to Set System Profile Version");
		goto TerminateConstructor;
	}

   if (pWMProfile)
      m_pWMProfile = pWMProfile;
   else if(FAILED(m_pWMProfileManager->LoadProfileByID(guidProfileID,&m_pWMProfile)))
	{
		SetErrorMessage("Unable to Load System Profile");
		goto TerminateConstructor;
	}

	if(FAILED(WMCreateWriter(NULL,&m_pWMWriter)))
	{
		SetErrorMessage("Unable to Create Media Writer Object");
		goto TerminateConstructor;
	}

	if(FAILED(m_pWMWriter->SetProfile(m_pWMProfile)))
	{
		SetErrorMessage("Unable to Set System Profile");
		goto TerminateConstructor;
	}

	if(FAILED(m_pWMWriter->GetInputCount(&dwInputCount)))
	{
		SetErrorMessage("Unable to Get input count For Profile");
		goto TerminateConstructor;
	}
    
	for(DWORD	i=0;i<dwInputCount;i++)
	{
		if(FAILED(m_pWMWriter->GetInputProps(i,&pInputProps)))
		{
			SetErrorMessage("Unable to GetInput Properties");
			goto TerminateConstructor;
		}
		if(FAILED(pInputProps->GetType(&guidInputType)))
		{
			SetErrorMessage("Unable to Get Input Property Type");
			goto TerminateConstructor;
		}
		if(guidInputType==WMMEDIATYPE_Video)
		{
			m_pVideoProps=pInputProps;
			m_dwVideoInput=i;
			break;
		}
		else
		{
			pInputProps->Release();
			pInputProps=NULL;
		}
	}

	if(m_pVideoProps==NULL)
	{
		SetErrorMessage("Profile Does not Accept Video input");
		goto TerminateConstructor;
	}

#ifndef UNICODE
        WCHAR       pwszOutFile[1024];
        if( 0 == MultiByteToWideChar( CP_ACP, 0, lpszFileName,-1, pwszOutFile, sizeof( pwszOutFile ) ) )
        {
			SetErrorMessage("Unable to Convert Output Filename");
			goto TerminateConstructor;
        }
        if(FAILED(m_pWMWriter->SetOutputFilename( pwszOutFile )))
		{
			SetErrorMessage("Unable to Set Output Filename");
			goto TerminateConstructor;
		}
#else
	if(FAILED(m_pWMWriter->SetOutputFilename(lpszFileName)))
	{
		SetErrorMessage("Unable to Set Output Filename");
		goto TerminateConstructor;
	}
#endif	//UNICODE

	m_nAppendFuncSelector=1;		//0=Dummy	1=FirstTime	2=Usual

	return;

TerminateConstructor:

	ReleaseMemory();

	return;
}

CwmvFile::~CwmvFile(void)
{
	ReleaseMemory();

	CoUninitialize();
}

void CwmvFile::ReleaseMemory()
{
	if(m_nAppendFuncSelector==2)	//If we are Currently Writing 
	{
		if(m_pWMWriter)
			m_pWMWriter->EndWriting();	
	}
	
	m_nAppendFuncSelector=0;		//Point to DummyFunction

	if(m_pVideoProps)
	{
		m_pVideoProps->Release();
		m_pVideoProps=NULL;
	}
	if(m_pWMWriter)
	{
		m_pWMWriter->Release();
		m_pWMWriter=NULL;
	}
	if(m_pWMProfile)
	{
		m_pWMProfile->Release();
		m_pWMProfile=NULL;
	}
	if(m_pWMProfileManager)
	{
		m_pWMProfileManager->Release();
		m_pWMProfileManager=NULL;
	}
	if(m_hwmvDC)
	{
		DeleteDC(m_hwmvDC);
		m_hwmvDC=NULL;
	}
}

void CwmvFile::SetErrorMessage(LPCTSTR lpszErrorMessage)
{
	strncpy_s(m_szErrMsg, lpszErrorMessage, __countof(m_szErrMsg)-1);
}

HRESULT CwmvFile::InitMovieCreation(int nFrameWidth, int nFrameHeight, int nBitsPerPixel)
{
	int	nMaxWidth=GetSystemMetrics(SM_CXSCREEN), nMaxHeight=GetSystemMetrics(SM_CYSCREEN);

	BITMAPINFO	bmpInfo;

	m_hwmvDC=CreateCompatibleDC(NULL);
	if(m_hwmvDC==NULL)	
	{
		SetErrorMessage("Unable to Create Device Context");
		return E_FAIL;
	}

	ZeroMemory(&bmpInfo,sizeof(BITMAPINFO));
	bmpInfo.bmiHeader.biSize		= sizeof(BITMAPINFOHEADER);
	bmpInfo.bmiHeader.biBitCount	= (WORD)nBitsPerPixel;	
	bmpInfo.bmiHeader.biWidth		= nFrameWidth;
	bmpInfo.bmiHeader.biHeight		= nFrameHeight;
	bmpInfo.bmiHeader.biPlanes		= 1;
	bmpInfo.bmiHeader.biSizeImage	= nFrameWidth*nFrameHeight*nBitsPerPixel/8;
	bmpInfo.bmiHeader.biCompression	= BI_RGB;

	if(bmpInfo.bmiHeader.biHeight>nMaxHeight)	nMaxHeight=bmpInfo.bmiHeader.biHeight;
	if(bmpInfo.bmiHeader.biWidth>nMaxWidth)	nMaxWidth=bmpInfo.bmiHeader.biWidth;

	WMVIDEOINFOHEADER	videoInfo;
	videoInfo.rcSource.left		= 0;
	videoInfo.rcSource.top		= 0;
	videoInfo.rcSource.right	= bmpInfo.bmiHeader.biWidth;
	videoInfo.rcSource.bottom	= bmpInfo.bmiHeader.biHeight;
	videoInfo.rcTarget			= videoInfo.rcSource;
	videoInfo.rcTarget.right	= videoInfo.rcSource.right;
	videoInfo.rcTarget.bottom	= videoInfo.rcSource.bottom;
	videoInfo.dwBitRate= (DWORD)(nMaxWidth*nMaxHeight*bmpInfo.bmiHeader.biBitCount* m_FrameRate);
	videoInfo.dwBitErrorRate	= 0;
	videoInfo.AvgTimePerFrame	= (LONGLONG)(((QWORD)1) * 10000 * 1000 / m_FrameRate);
	memcpy(&(videoInfo.bmiHeader),&bmpInfo.bmiHeader,sizeof(BITMAPINFOHEADER));

	WM_MEDIA_TYPE mt;
	mt.majortype = WMMEDIATYPE_Video;
    if( bmpInfo.bmiHeader.biCompression == BI_RGB )
    {
        if( bmpInfo.bmiHeader.biBitCount == 32 )
        {
			mt.subtype = WMMEDIASUBTYPE_RGB32;
        } 
        else if( bmpInfo.bmiHeader.biBitCount == 24 )
        {
            mt.subtype = WMMEDIASUBTYPE_RGB24;
        }
        else if( bmpInfo.bmiHeader.biBitCount == 16 )
        {
            mt.subtype = WMMEDIASUBTYPE_RGB555;
        }
        else if( bmpInfo.bmiHeader.biBitCount == 8 )
        {
            mt.subtype = WMMEDIASUBTYPE_RGB8;
        }
        else
        {
            mt.subtype = GUID_NULL;
        }
    }
	mt.bFixedSizeSamples	= false;
	mt.bTemporalCompression	= false;
	mt.lSampleSize			= 0;
	mt.formattype			= WMFORMAT_VideoInfo;
	mt.pUnk					= NULL;
	mt.cbFormat				= sizeof(WMVIDEOINFOHEADER);
	mt.pbFormat				= (BYTE*)&videoInfo;

	if(FAILED(m_pVideoProps->SetMediaType(&mt)))
	{
		SetErrorMessage("Unable to Set Media Type");
		return E_FAIL;
	}

    if(FAILED(m_pWMWriter->SetInputProps(m_dwVideoInput,m_pVideoProps)))
	{
		SetErrorMessage("Unable to Set Input Properties for Media Writer");
		return E_FAIL;
	}

	if(FAILED(m_pWMWriter->BeginWriting()))
	{
		SetErrorMessage("Unable to Initialize Writing");
		return E_FAIL;
	}
	
	return S_OK;
}

HRESULT CwmvFile::AppendNewFrame(int nWidth, int nHeight, LPVOID pBits,int nBitsPerPixel, double frameTime)
{
	return (this->*pAppendFrameBits[m_nAppendFuncSelector])(nWidth,nHeight,pBits,nBitsPerPixel, frameTime);
}

HRESULT	CwmvFile::AppendFrameFirstTime(int nWidth, int nHeight, LPVOID pBits,int nBitsPerPixel, double frameTime)
{
	if(SUCCEEDED(InitMovieCreation(nWidth, nHeight, nBitsPerPixel)))
	{
		m_nAppendFuncSelector=2;		//Point to UsualAppend Function

		return AppendFrameUsual(nWidth,nHeight,pBits,nBitsPerPixel,frameTime);
	}

	// Control Comes here Only if there is Any Error
	ReleaseMemory();

	return E_FAIL;
}


HRESULT	CwmvFile::AppendFrameUsual(int nWidth, int nHeight, LPVOID pBits,int nBitsPerPixel, double frameTime)
{
	HRESULT			hr=E_FAIL;
    INSSBuffer		*pSample=NULL;
    BYTE			*pbBuffer=NULL;
    DWORD			cbBuffer=0;

	BITMAPINFO	bmpInfo;

	ZeroMemory(&bmpInfo,sizeof(BITMAPINFO));
	bmpInfo.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);
	bmpInfo.bmiHeader.biBitCount=(WORD)nBitsPerPixel;
	
	bmpInfo.bmiHeader.biWidth=nWidth;
	bmpInfo.bmiHeader.biHeight=nHeight;	

	bmpInfo.bmiHeader.biCompression=BI_RGB;

	bmpInfo.bmiHeader.biPlanes=1;
	bmpInfo.bmiHeader.biSizeImage=nWidth*nHeight*nBitsPerPixel/8;

	if(FAILED(m_pWMWriter->AllocateSample(bmpInfo.bmiHeader.biSizeImage,&pSample)))
	{
		SetErrorMessage("Unable to Allocate Memory");
		goto TerminateAppendWMVFrameUsualBits;
	}

	if(FAILED(pSample->GetBufferAndLength(&pbBuffer,&cbBuffer)))
	{
		SetErrorMessage("Unable to Lock Buffer");
		goto TerminateAppendWMVFrameUsualBits;
	}

	memcpy(pbBuffer,pBits,bmpInfo.bmiHeader.biSizeImage);

   QWORD sampleTime = 10000 * m_msVideoTime;
   if (frameTime >= 0.0f)
      sampleTime = (QWORD)(1000.0f * 10000.0f * frameTime + .5f);
      
	hr=m_pWMWriter->WriteSample(m_dwVideoInput, sampleTime,0,pSample);

	m_msVideoTime=(QWORD)((++m_dwCurrentVideoSample*1000)/m_FrameRate);

	if(pSample)	{	pSample->Release();	pSample=NULL;	}

	if(FAILED(hr))
	{
		SetErrorMessage("Unable to Write Frame");
		goto TerminateAppendWMVFrameUsualBits;
	}

	return S_OK;

TerminateAppendWMVFrameUsualBits:

	if(pSample)	{	pSample->Release();	pSample=NULL;	}

	ReleaseMemory();

	return E_FAIL;
}

HRESULT	CwmvFile::AppendDummy(int nWidth, int nHeight, LPVOID pBits,int nBitsPerPixel, double frameTime)
{
   nWidth;
   nHeight;
   pBits;
   nBitsPerPixel;
   frameTime;
   
	return E_FAIL;
}