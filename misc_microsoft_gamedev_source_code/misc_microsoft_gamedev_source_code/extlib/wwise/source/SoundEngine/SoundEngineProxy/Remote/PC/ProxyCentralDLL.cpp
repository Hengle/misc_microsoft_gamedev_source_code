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

// ProxyCentral.cpp : Defines the initialization routines for the DLL.
//

#include "stdafx.h"
#include "ProxyCentralDLL.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

//
//	Note!
//
//		If this DLL is dynamically linked against the MFC
//		DLLs, any functions exported from this DLL which
//		call into MFC must have the AFX_MANAGE_STATE macro
//		added at the very beginning of the function.
//
//		For example:
//
//		extern "C" BOOL PASCAL EXPORT ExportedFunction()
//		{
//			AFX_MANAGE_STATE(AfxGetStaticModuleState());
//			// normal function body here
//		}
//
//		It is very important that this macro appear in each
//		function, prior to any calls into MFC.  This means that
//		it must appear as the first statement within the 
//		function, even before any object variable declarations
//		as their constructors may generate calls into the MFC
//		DLL.
//
//		Please see MFC Technical Notes 33 and 58 for additional
//		details.
//

// CProxyCentralApp

BEGIN_MESSAGE_MAP(CProxyCentralApp, CWinApp)
END_MESSAGE_MAP()


// CProxyCentralApp construction

CProxyCentralApp::CProxyCentralApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}


// The one and only CProxyCentralApp object

CProxyCentralApp theApp;


// CProxyCentralApp initialization

BOOL CProxyCentralApp::InitInstance()
{
	CWinApp::InitInstance();

	return TRUE;
}
