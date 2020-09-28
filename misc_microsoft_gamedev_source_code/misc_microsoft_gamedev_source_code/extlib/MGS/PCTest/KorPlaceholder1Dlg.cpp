// KorPlaceholder1Dlg.cpp : implementation file
//

#include "stdafx.h"
#include "KorPlaceholder1.h"
#include "KorPlaceholder1Dlg.h"
#include "formatstring.h"
#include ".\korplaceholder1dlg.h"

using namespace MGS;

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CKorPlaceholder1Dlg dialog



CKorPlaceholder1Dlg::CKorPlaceholder1Dlg(CWnd* pParent /*=NULL*/)
	: CDialog(CKorPlaceholder1Dlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CKorPlaceholder1Dlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDIT2, m_cEdit2);
	DDX_Control(pDX, IDC_EDIT4, m_cEdit4);
	DDX_Control(pDX, IDC_EDIT1, m_cEdit1);
	DDX_Control(pDX, IDC_IGNORE_INSERTS, m_cCheckIgnoreInserts);
	DDX_Control(pDX, IDC_ALLOCATE_BUFFER, m_cCheckAllocateBuffer);
	DDX_Control(pDX, IDC_BUFFER_SIZE, m_cBufferSize);
	DDX_Control(pDX, IDC_NULL_FORMAT_STRING, m_cNullFormatString);
	DDX_Control(pDX, IDC_NULL_OUTPUT_BUFFER, m_cNullOutputBuffer);
	DDX_Control(pDX, IDC_RETURN_VALUE, m_cReturnValue);
	DDX_Control(pDX, IDC_EXT_ERROR, m_cExtError);
	DDX_Control(pDX, IDC_EXT_ERROR_MSG, m_cExtErrorMsg);
}

BEGIN_MESSAGE_MAP(CKorPlaceholder1Dlg, CDialog)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDC_BUTTON1, OnBnClickedButton1)
	ON_BN_CLICKED(IDC_BUTTON2, OnBnClickedButton2)
	ON_BN_CLICKED(IDC_BUTTON3, OnBnClickedButton3)
END_MESSAGE_MAP()


// CKorPlaceholder1Dlg message handlers

BOOL CKorPlaceholder1Dlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	// TODO: Add extra initialization here
	m_cEdit2.SetWindowText(L"%1!s![\xc740/\xb294] %2!s![\xc744/\xb97c] \xc0ac\xb791\xd569\xb2c8\xb2e4. integer:%3!d!, double:%4!f!");	//"%s[¿∫/¥¬] %s[¿ª/∏¶] ªÁ∂˚«’¥œ¥Ÿ."
	m_cEdit4.SetWindowText(L"\xc601\xd76c,\xcca0\xc218,[123],[456.789]");	//"øµ?E √∂ºÅE
	
	m_cBufferSize.SetWindowText( L"256" );
	return TRUE;  // return TRUE  unless you set the focus to a control
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CKorPlaceholder1Dlg::OnPaint() 
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CKorPlaceholder1Dlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CKorPlaceholder1Dlg::OnBnClickedButton1()
{
	// TODO: Add your control notification handler code here
	CString sCombo1;
	m_cEdit2.GetWindowText(sCombo1);

	CString sCombo2;
	m_cEdit4.GetWindowText(sCombo2);

	CString sArray[16];
	long lArray[16];
	double fArray[16];
	int nArray = 0;
	int nArrayType[16]; // 0: string, 1: integer, 2: double

	for(int n = 0; n < 16; n++) {
		sArray[n] = _T("");
		lArray[n] = 0;
		fArray[n] = 0.0;
		nArrayType[n] = 0;
	}

	int curPos = 0;
	CString sToken = sCombo2.Tokenize(_T(","), curPos);
	while(sToken != _T("") && nArray < (sizeof(sArray)/sizeof(sArray[0]))) {
		sArray[nArray] = sToken;
		nArray++;
		sToken = sCombo2.Tokenize(_T(","), curPos);
	};

	for(int n = 0; n < nArray; n++) {
		int nLen = sArray[n].GetLength();

		sArray[n].Replace(_T("[comma]"), _T(","));
		if(sArray[n].GetAt(0) == '[' && sArray[n].GetAt(nLen-1) == ']') {
			int	k;
			for(k = 1; k < nLen-1; k++) {
				if(k==1 && sArray[n].GetAt(k) == '-')
					continue;
				if(sArray[n].GetAt(k) >= '0' && sArray[n].GetAt(k) <= '9')
					continue;
				if(sArray[n].GetAt(k) == '.')
					continue;
				int nChar = sArray[n].GetAt(k);
				break;
			}
			if(k == nLen-1) {
				CString sTemp1 = sArray[n];
				sTemp1.Remove('[');
				sTemp1.Remove(']');
				if(sTemp1.Find('.') == -1) {
					lArray[n] = _wtoi(sTemp1);
					nArrayType[n] = 1; // integer
				}
				else {
					fArray[n] = _wtof(sTemp1);
					nArrayType[n] = 2; // double
				}
			}
		}
	}

	BOOL bSuccess = TRUE;
	WCHAR sFormat[2048];
	WCHAR sResult[2048];
	const int iFormatSize = sizeof(sFormat)/sizeof(sFormat[0]);
	ZeroMemory(sFormat, sizeof(sResult));
	ZeroMemory(sResult, sizeof(sResult));
	wcsncpy(sFormat, sCombo1, iFormatSize);
	sFormat[iFormatSize - 1] = 0;
	LPWSTR pszAllocedBuffer = NULL, pOutput = sResult, pFormat;
	pFormat = m_cNullFormatString.GetCheck() == BST_CHECKED ? NULL : sFormat;

	DWORD dwFlags = 0;
	bool bAlloced = m_cCheckAllocateBuffer.GetCheck() == BST_CHECKED;
	if( bAlloced )
	{
		dwFlags |= FORMAT_MESSAGE_ALLOCATE_BUFFER;
		pOutput = (LPWSTR)&pszAllocedBuffer;
	}
	if( m_cCheckIgnoreInserts.GetCheck() == BST_CHECKED )
		dwFlags |= FORMAT_MESSAGE_IGNORE_INSERTS;

	int iRc = 0;
	int iResultSize = GetDlgItemInt( m_cBufferSize.GetDlgCtrlID(), NULL, 0 );
	if ( m_cNullOutputBuffer.GetCheck() == BST_CHECKED )
	{
		pOutput = NULL;
	}
	switch(nArray) {
		case 0:
			iRc = FormatStringW(dwFlags, pOutput, iResultSize, pFormat); 
			break;
		case 1: 
			if(nArrayType[0] == 0) {
				iRc = FormatStringW(dwFlags, pOutput, iResultSize, pFormat, sArray[0]); 
			}
			else if(nArrayType[0] == 1) {
				iRc = FormatStringW(dwFlags, pOutput, iResultSize, pFormat, lArray[0]); 
			}
			else  {
				iRc = FormatStringW(dwFlags, pOutput, iResultSize, pFormat, fArray[0]); 
			}
			break;
		case 2: 
			if(nArrayType[0] == 0) {
				if(nArrayType[1] == 0)
					iRc = FormatStringW(dwFlags, pOutput, iResultSize, pFormat, sArray[0], sArray[1]); 
				else if(nArrayType[1] == 1)
					iRc = FormatStringW(dwFlags, pOutput, iResultSize, pFormat, sArray[0], lArray[1]); 
				else 
					iRc = FormatStringW(dwFlags, pOutput, iResultSize, pFormat, sArray[0], fArray[1]); 
			}
			else if(nArrayType[0] == 1) {
				if(nArrayType[1] == 0)
					iRc = FormatStringW(dwFlags, pOutput, iResultSize, pFormat, lArray[0], sArray[1]); 
				else if(nArrayType[1] == 1)
					iRc = FormatStringW(dwFlags, pOutput, iResultSize, pFormat, lArray[0], lArray[1]); 
				else 
					iRc = FormatStringW(dwFlags, pOutput, iResultSize, pFormat, lArray[0], fArray[1]); 
			}
			else  {
				if(nArrayType[1] == 0)
					iRc = FormatStringW(dwFlags, pOutput, iResultSize, pFormat, fArray[0], sArray[1]); 
				else if(nArrayType[1] == 1)
					iRc = FormatStringW(dwFlags, pOutput, iResultSize, pFormat, fArray[0], lArray[1]); 
				else 
					iRc = FormatStringW(dwFlags, pOutput, iResultSize, pFormat, fArray[0], fArray[1]); 
			}
			break;
		case 3: 
			if(nArrayType[0] == 0) {
				if(nArrayType[1] == 0) {
					if(nArrayType[2] == 0)
						iRc = FormatStringW(dwFlags, pOutput, iResultSize, pFormat, sArray[0], sArray[1], sArray[2]); 
					else if(nArrayType[2] == 1)									  
						iRc = FormatStringW(dwFlags, pOutput, iResultSize, pFormat, sArray[0], sArray[1], lArray[2]); 
					else														  
						iRc = FormatStringW(dwFlags, pOutput, iResultSize, pFormat, sArray[0], sArray[1], fArray[2]); 
				}																  
				else if(nArrayType[1] == 1) {									  
					if(nArrayType[2] == 0)										  
						iRc = FormatStringW(dwFlags, pOutput, iResultSize, pFormat, sArray[0], lArray[1], sArray[2]); 
					else if(nArrayType[2] == 1)									  
						iRc = FormatStringW(dwFlags, pOutput, iResultSize, pFormat, sArray[0], lArray[1], lArray[2]); 
					else														  
						iRc = FormatStringW(dwFlags, pOutput, iResultSize, pFormat, sArray[0], lArray[1], fArray[2]); 
				}																  
				else  {															  
					if(nArrayType[2] == 0)										  
						iRc = FormatStringW(dwFlags, pOutput, iResultSize, pFormat, sArray[0], fArray[1], sArray[2]); 
					else if(nArrayType[2] == 1)									  
						iRc = FormatStringW(dwFlags, pOutput, iResultSize, pFormat, sArray[0], fArray[1], lArray[2]); 
					else														  
						iRc = FormatStringW(dwFlags, pOutput, iResultSize, pFormat, sArray[0], fArray[1], fArray[2]); 
				}
			}
			else if(nArrayType[0] == 1) {
				if(nArrayType[1] == 0) {
					if(nArrayType[2] == 0)
						iRc = FormatStringW(dwFlags, pOutput, iResultSize, pFormat, lArray[0], sArray[1], sArray[2]); 
					else if(nArrayType[2] == 1)									  
						iRc = FormatStringW(dwFlags, pOutput, iResultSize, pFormat, lArray[0], sArray[1], lArray[2]); 
					else														  
						iRc = FormatStringW(dwFlags, pOutput, iResultSize, pFormat, lArray[0], sArray[1], fArray[2]); 
				}																  
				else if(nArrayType[1] == 1) {									  
					if(nArrayType[2] == 0)										  
						iRc = FormatStringW(dwFlags, pOutput, iResultSize, pFormat, lArray[0], lArray[1], sArray[2]); 
					else if(nArrayType[2] == 1)									  
						iRc = FormatStringW(dwFlags, pOutput, iResultSize, pFormat, lArray[0], lArray[1], lArray[2]); 
					else														  
						iRc = FormatStringW(dwFlags, pOutput, iResultSize, pFormat, lArray[0], lArray[1], fArray[2]); 
				}																  
				else  {															  
					if(nArrayType[2] == 0)										  
						iRc = FormatStringW(dwFlags, pOutput, iResultSize, pFormat, lArray[0], fArray[1], sArray[2]); 
					else if(nArrayType[2] == 1)									  
						iRc = FormatStringW(dwFlags, pOutput, iResultSize, pFormat, lArray[0], fArray[1], lArray[2]); 
					else														  
						iRc = FormatStringW(dwFlags, pOutput, iResultSize, pFormat, lArray[0], fArray[1], fArray[2]); 
				}
			}
			else  {
				if(nArrayType[1] == 0) {
					if(nArrayType[2] == 0)
						iRc = FormatStringW(dwFlags, pOutput, iResultSize, pFormat, fArray[0], sArray[1], sArray[2]); 
					else if(nArrayType[2] == 1)									  
						iRc = FormatStringW(dwFlags, pOutput, iResultSize, pFormat, fArray[0], sArray[1], lArray[2]); 
					else														  
						iRc = FormatStringW(dwFlags, pOutput, iResultSize, pFormat, fArray[0], sArray[1], fArray[2]); 
				}																  
				else if(nArrayType[1] == 1) {									  
					if(nArrayType[2] == 0)										  
						iRc = FormatStringW(dwFlags, pOutput, iResultSize, pFormat, fArray[0], lArray[1], sArray[2]); 
					else if(nArrayType[2] == 1)									  
						iRc = FormatStringW(dwFlags, pOutput, iResultSize, pFormat, fArray[0], lArray[1], lArray[2]); 
					else														  
						iRc = FormatStringW(dwFlags, pOutput, iResultSize, pFormat, fArray[0], lArray[1], fArray[2]); 
				}																  
				else  {															  
					if(nArrayType[2] == 0)										  
						iRc = FormatStringW(dwFlags, pOutput, iResultSize, pFormat, fArray[0], fArray[1], sArray[2]); 
					else if(nArrayType[2] == 1)									  
						iRc = FormatStringW(dwFlags, pOutput, iResultSize, pFormat, fArray[0], fArray[1], lArray[2]); 
					else														  
						iRc = FormatStringW(dwFlags, pOutput, iResultSize, pFormat, fArray[0], fArray[1], fArray[2]); 
				}
			}

			break;
		case 4: 
			if(nArrayType[0] == 0) {
				if(nArrayType[1] == 0) {
					if(nArrayType[2] == 0) {
						if(nArrayType[3] == 0)
							iRc = FormatStringW(dwFlags, pOutput, iResultSize, pFormat, sArray[0], sArray[1], sArray[2], sArray[3]); 
						else if(nArrayType[3] == 1)																	
							iRc = FormatStringW(dwFlags, pOutput, iResultSize, pFormat, sArray[0], sArray[1], sArray[2], lArray[3]); 
						else																						
							iRc = FormatStringW(dwFlags, pOutput, iResultSize, pFormat, sArray[0], sArray[1], sArray[2], fArray[3]); 
					}																								
					else if(nArrayType[2] == 1) {																	
						if(nArrayType[3] == 0)																		
							iRc = FormatStringW(dwFlags, pOutput, iResultSize, pFormat, sArray[0], sArray[1], lArray[2], sArray[3]); 
						else if(nArrayType[3] == 1)																	
							iRc = FormatStringW(dwFlags, pOutput, iResultSize, pFormat, sArray[0], sArray[1], lArray[2], lArray[3]); 
						else																						
							iRc = FormatStringW(dwFlags, pOutput, iResultSize, pFormat, sArray[0], sArray[1], lArray[2], fArray[3]); 
					}																								
					else  {																							
						if(nArrayType[3] == 0)																		
							iRc = FormatStringW(dwFlags, pOutput, iResultSize, pFormat, sArray[0], sArray[1], fArray[2], sArray[3]); 
						else if(nArrayType[3] == 1)																	
							iRc = FormatStringW(dwFlags, pOutput, iResultSize, pFormat, sArray[0], sArray[1], fArray[2], lArray[3]); 
						else																						
							iRc = FormatStringW(dwFlags, pOutput, iResultSize, pFormat, sArray[0], sArray[1], fArray[2], fArray[3]); 
					}																								
				}																									
				else if(nArrayType[1] == 1) {																		
					if(nArrayType[2] == 0) {																		
						if(nArrayType[3] == 0)																		
							iRc = FormatStringW(dwFlags, pOutput, iResultSize, pFormat, sArray[0], lArray[1], sArray[2], sArray[3]); 
						else if(nArrayType[3] == 1)																	
							iRc = FormatStringW(dwFlags, pOutput, iResultSize, pFormat, sArray[0], lArray[1], sArray[2], lArray[3]); 
						else																						
							iRc = FormatStringW(dwFlags, pOutput, iResultSize, pFormat, sArray[0], lArray[1], sArray[2], fArray[3]); 
					}																								
					else if(nArrayType[2] == 1) {																	
						if(nArrayType[3] == 0)																		
							iRc = FormatStringW(dwFlags, pOutput, iResultSize, pFormat, sArray[0], lArray[1], lArray[2], sArray[3]); 
						else if(nArrayType[3] == 1)																	
							iRc = FormatStringW(dwFlags, pOutput, iResultSize, pFormat, sArray[0], lArray[1], lArray[2], lArray[3]); 
						else																						
							iRc = FormatStringW(dwFlags, pOutput, iResultSize, pFormat, sArray[0], lArray[1], lArray[2], fArray[3]); 
					}																								
					else  {																							
						if(nArrayType[3] == 0)																		
							iRc = FormatStringW(dwFlags, pOutput, iResultSize, pFormat, sArray[0], lArray[1], fArray[2], sArray[3]); 
						else if(nArrayType[3] == 1)																	
							iRc = FormatStringW(dwFlags, pOutput, iResultSize, pFormat, sArray[0], lArray[1], fArray[2], lArray[3]); 
						else																						
							iRc = FormatStringW(dwFlags, pOutput, iResultSize, pFormat, sArray[0], lArray[1], fArray[2], fArray[3]); 
					}
				}
				else  {
					if(nArrayType[2] == 0) {																		
						if(nArrayType[3] == 0)																		
							iRc = FormatStringW(dwFlags, pOutput, iResultSize, pFormat, sArray[0], fArray[1], sArray[2], sArray[3]); 
						else if(nArrayType[3] == 1)																	
							iRc = FormatStringW(dwFlags, pOutput, iResultSize, pFormat, sArray[0], fArray[1], sArray[2], lArray[3]); 
						else																						
							iRc = FormatStringW(dwFlags, pOutput, iResultSize, pFormat, sArray[0], fArray[1], sArray[2], fArray[3]); 
					}																								
					else if(nArrayType[2] == 1) {																	
						if(nArrayType[3] == 0)																		
							iRc = FormatStringW(dwFlags, pOutput, iResultSize, pFormat, sArray[0], fArray[1], lArray[2], sArray[3]); 
						else if(nArrayType[3] == 1)																	
							iRc = FormatStringW(dwFlags, pOutput, iResultSize, pFormat, sArray[0], fArray[1], lArray[2], lArray[3]); 
						else																						
							iRc = FormatStringW(dwFlags, pOutput, iResultSize, pFormat, sArray[0], fArray[1], lArray[2], fArray[3]); 
					}																								
					else  {																							
						if(nArrayType[3] == 0)																		
							iRc = FormatStringW(dwFlags, pOutput, iResultSize, pFormat, sArray[0], fArray[1], fArray[2], sArray[3]); 
						else if(nArrayType[3] == 1)																	
							iRc = FormatStringW(dwFlags, pOutput, iResultSize, pFormat, sArray[0], fArray[1], fArray[2], lArray[3]); 
						else																						
							iRc = FormatStringW(dwFlags, pOutput, iResultSize, pFormat, sArray[0], fArray[1], fArray[2], fArray[3]); 
					}
				}
			}
			else if(nArrayType[0] == 1) {
				if(nArrayType[1] == 0) {
					if(nArrayType[2] == 0) {
						if(nArrayType[3] == 0)
							iRc = FormatStringW(dwFlags, pOutput, iResultSize, pFormat, lArray[0], sArray[1], sArray[2], sArray[3]); 
						else if(nArrayType[3] == 1)																	
							iRc = FormatStringW(dwFlags, pOutput, iResultSize, pFormat, lArray[0], sArray[1], sArray[2], lArray[3]); 
						else																						
							iRc = FormatStringW(dwFlags, pOutput, iResultSize, pFormat, lArray[0], sArray[1], sArray[2], fArray[3]); 
					}																								
					else if(nArrayType[2] == 1) {																	
						if(nArrayType[3] == 0)																		
							iRc = FormatStringW(dwFlags, pOutput, iResultSize, pFormat, lArray[0], sArray[1], lArray[2], sArray[3]); 
						else if(nArrayType[3] == 1)																	
							iRc = FormatStringW(dwFlags, pOutput, iResultSize, pFormat, lArray[0], sArray[1], lArray[2], lArray[3]); 
						else																						
							iRc = FormatStringW(dwFlags, pOutput, iResultSize, pFormat, lArray[0], sArray[1], lArray[2], fArray[3]); 
					}																								
					else  {																							
						if(nArrayType[3] == 0)																		
							iRc = FormatStringW(dwFlags, pOutput, iResultSize, pFormat, lArray[0], sArray[1], fArray[2], sArray[3]); 
						else if(nArrayType[3] == 1)																	
							iRc = FormatStringW(dwFlags, pOutput, iResultSize, pFormat, lArray[0], sArray[1], fArray[2], lArray[3]); 
						else																						
							iRc = FormatStringW(dwFlags, pOutput, iResultSize, pFormat, lArray[0], sArray[1], fArray[2], fArray[3]); 
					}																								
				}																									
				else if(nArrayType[1] == 1) {																		
					if(nArrayType[2] == 0) {																		
						if(nArrayType[3] == 0)																		
							iRc = FormatStringW(dwFlags, pOutput, iResultSize, pFormat, lArray[0], lArray[1], sArray[2], sArray[3]); 
						else if(nArrayType[3] == 1)																	
							iRc = FormatStringW(dwFlags, pOutput, iResultSize, pFormat, lArray[0], lArray[1], sArray[2], lArray[3]); 
						else																						
							iRc = FormatStringW(dwFlags, pOutput, iResultSize, pFormat, lArray[0], lArray[1], sArray[2], fArray[3]); 
					}																								
					else if(nArrayType[2] == 1) {																	
						if(nArrayType[3] == 0)																		
							iRc = FormatStringW(dwFlags, pOutput, iResultSize, pFormat, lArray[0], lArray[1], lArray[2], sArray[3]); 
						else if(nArrayType[3] == 1)																	
							iRc = FormatStringW(dwFlags, pOutput, iResultSize, pFormat, lArray[0], lArray[1], lArray[2], lArray[3]); 
						else																						
							iRc = FormatStringW(dwFlags, pOutput, iResultSize, pFormat, lArray[0], lArray[1], lArray[2], fArray[3]); 
					}																								
					else  {																							
						if(nArrayType[3] == 0)																		
							iRc = FormatStringW(dwFlags, pOutput, iResultSize, pFormat, lArray[0], lArray[1], fArray[2], sArray[3]); 
						else if(nArrayType[3] == 1)																	
							iRc = FormatStringW(dwFlags, pOutput, iResultSize, pFormat, lArray[0], lArray[1], fArray[2], lArray[3]); 
						else																						
							iRc = FormatStringW(dwFlags, pOutput, iResultSize, pFormat, lArray[0], lArray[1], fArray[2], fArray[3]); 
					}
				}
				else  {
					if(nArrayType[2] == 0) {																		
						if(nArrayType[3] == 0)																		
							iRc = FormatStringW(dwFlags, pOutput, iResultSize, pFormat, lArray[0], fArray[1], sArray[2], sArray[3]); 
						else if(nArrayType[3] == 1)																	
							iRc = FormatStringW(dwFlags, pOutput, iResultSize, pFormat, lArray[0], fArray[1], sArray[2], lArray[3]); 
						else																						
							iRc = FormatStringW(dwFlags, pOutput, iResultSize, pFormat, lArray[0], fArray[1], sArray[2], fArray[3]); 
					}																								
					else if(nArrayType[2] == 1) {																	
						if(nArrayType[3] == 0)																		
							iRc = FormatStringW(dwFlags, pOutput, iResultSize, pFormat, lArray[0], fArray[1], lArray[2], sArray[3]); 
						else if(nArrayType[3] == 1)																	
							iRc = FormatStringW(dwFlags, pOutput, iResultSize, pFormat, lArray[0], fArray[1], lArray[2], lArray[3]); 
						else																						
							iRc = FormatStringW(dwFlags, pOutput, iResultSize, pFormat, lArray[0], fArray[1], lArray[2], fArray[3]); 
					}																								
					else  {																							
						if(nArrayType[3] == 0)																		
							iRc = FormatStringW(dwFlags, pOutput, iResultSize, pFormat, lArray[0], fArray[1], fArray[2], sArray[3]); 
						else if(nArrayType[3] == 1)																	
							iRc = FormatStringW(dwFlags, pOutput, iResultSize, pFormat, lArray[0], fArray[1], fArray[2], lArray[3]); 
						else																						
							iRc = FormatStringW(dwFlags, pOutput, iResultSize, pFormat, lArray[0], fArray[1], fArray[2], fArray[3]); 
					}
				}
			}
			else  {
				if(nArrayType[1] == 0) {
					if(nArrayType[2] == 0) {
						if(nArrayType[3] == 0)
							iRc = FormatStringW(dwFlags, pOutput, iResultSize, pFormat, fArray[0], sArray[1], sArray[2], sArray[3]); 
						else if(nArrayType[3] == 1)																	
							iRc = FormatStringW(dwFlags, pOutput, iResultSize, pFormat, fArray[0], sArray[1], sArray[2], lArray[3]); 
						else																						
							iRc = FormatStringW(dwFlags, pOutput, iResultSize, pFormat, fArray[0], sArray[1], sArray[2], fArray[3]); 
					}																								
					else if(nArrayType[2] == 1) {																	
						if(nArrayType[3] == 0)																		
							iRc = FormatStringW(dwFlags, pOutput, iResultSize, pFormat, fArray[0], sArray[1], lArray[2], sArray[3]); 
						else if(nArrayType[3] == 1)																	
							iRc = FormatStringW(dwFlags, pOutput, iResultSize, pFormat, fArray[0], sArray[1], lArray[2], lArray[3]); 
						else																						
							iRc = FormatStringW(dwFlags, pOutput, iResultSize, pFormat, fArray[0], sArray[1], lArray[2], fArray[3]); 
					}																								
					else  {																							
						if(nArrayType[3] == 0)																		
							iRc = FormatStringW(dwFlags, pOutput, iResultSize, pFormat, fArray[0], sArray[1], fArray[2], sArray[3]); 
						else if(nArrayType[3] == 1)																	
							iRc = FormatStringW(dwFlags, pOutput, iResultSize, pFormat, fArray[0], sArray[1], fArray[2], lArray[3]); 
						else																						
							iRc = FormatStringW(dwFlags, pOutput, iResultSize, pFormat, fArray[0], sArray[1], fArray[2], fArray[3]); 
					}																								
				}																									
				else if(nArrayType[1] == 1) {																		
					if(nArrayType[2] == 0) {																		
						if(nArrayType[3] == 0)																		
							iRc = FormatStringW(dwFlags, pOutput, iResultSize, pFormat, fArray[0], lArray[1], sArray[2], sArray[3]); 
						else if(nArrayType[3] == 1)																	
							iRc = FormatStringW(dwFlags, pOutput, iResultSize, pFormat, fArray[0], lArray[1], sArray[2], lArray[3]); 
						else																						
							iRc = FormatStringW(dwFlags, pOutput, iResultSize, pFormat, fArray[0], lArray[1], sArray[2], fArray[3]); 
					}																								
					else if(nArrayType[2] == 1) {																	
						if(nArrayType[3] == 0)																		
							iRc = FormatStringW(dwFlags, pOutput, iResultSize, pFormat, fArray[0], lArray[1], lArray[2], sArray[3]); 
						else if(nArrayType[3] == 1)																	
							iRc = FormatStringW(dwFlags, pOutput, iResultSize, pFormat, fArray[0], lArray[1], lArray[2], lArray[3]); 
						else																						
							iRc = FormatStringW(dwFlags, pOutput, iResultSize, pFormat, fArray[0], lArray[1], lArray[2], fArray[3]); 
					}																								
					else  {																							
						if(nArrayType[3] == 0)																		
							iRc = FormatStringW(dwFlags, pOutput, iResultSize, pFormat, fArray[0], lArray[1], fArray[2], sArray[3]); 
						else if(nArrayType[3] == 1)																	
							iRc = FormatStringW(dwFlags, pOutput, iResultSize, pFormat, fArray[0], lArray[1], fArray[2], lArray[3]); 
						else																						
							iRc = FormatStringW(dwFlags, pOutput, iResultSize, pFormat, fArray[0], lArray[1], fArray[2], fArray[3]); 
					}
				}
				else  {
					if(nArrayType[2] == 0) {																		
						if(nArrayType[3] == 0)																		
							iRc = FormatStringW(dwFlags, pOutput, iResultSize, pFormat, fArray[0], fArray[1], sArray[2], sArray[3]); 
						else if(nArrayType[3] == 1)																	
							iRc = FormatStringW(dwFlags, pOutput, iResultSize, pFormat, fArray[0], fArray[1], sArray[2], lArray[3]); 
						else																						
							iRc = FormatStringW(dwFlags, pOutput, iResultSize, pFormat, fArray[0], fArray[1], sArray[2], fArray[3]); 
					}																								
					else if(nArrayType[2] == 1) {																	
						if(nArrayType[3] == 0)																		
							iRc = FormatStringW(dwFlags, pOutput, iResultSize, pFormat, fArray[0], fArray[1], lArray[2], sArray[3]); 
						else if(nArrayType[3] == 1)																	
							iRc = FormatStringW(dwFlags, pOutput, iResultSize, pFormat, fArray[0], fArray[1], lArray[2], lArray[3]); 
						else																						
							iRc = FormatStringW(dwFlags, pOutput, iResultSize, pFormat, fArray[0], fArray[1], lArray[2], fArray[3]); 
					}																								
					else  {																							
						if(nArrayType[3] == 0)																		
							iRc = FormatStringW(dwFlags, pOutput, iResultSize, pFormat, fArray[0], fArray[1], fArray[2], sArray[3]); 
						else if(nArrayType[3] == 1)																	
							iRc = FormatStringW(dwFlags, pOutput, iResultSize, pFormat, fArray[0], fArray[1], fArray[2], lArray[3]); 
						else																						
							iRc = FormatStringW(dwFlags, pOutput, iResultSize, pFormat, fArray[0], fArray[1], fArray[2], fArray[3]); 
					}
				}
			}

			break;
		default:
			MessageBox(_T("The maximum number of arguments is four for this sample."),
				_T("Korean Placeholder Sample"), MB_ICONSTOP | MB_OK);
			bSuccess = FALSE;
			m_cEdit1.SetWindowText( _T("[ERROR] The maximum number of arguments is four for this sample." ) );
			break;
	}

	DWORD dwError = GetLastError();
	WCHAR szBuf[32];
	_itow( iRc, szBuf, 10 );
	m_cReturnValue.SetWindowText( szBuf );
	_ultow( dwError, szBuf, 10 );
	m_cExtError.SetWindowText( szBuf );
	LPWSTR pszError = NULL;
	if ( dwError )
	{
		bSuccess = FALSE;
		FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, 0,
			dwError, 0, (LPWSTR)&pszError, 0, NULL );
		m_cExtErrorMsg.SetWindowText( pszError );
		LocalFree( pszError );
	}
	if( bSuccess )
	{
		m_cEdit1.SetWindowText( bAlloced ? pszAllocedBuffer : pOutput );
		m_cExtErrorMsg.SetWindowText( L"" );
	}
	else
		m_cEdit1.SetWindowText( L"" );

	if( pszAllocedBuffer )
	{
		LocalFree( pszAllocedBuffer );
	}
}

void CKorPlaceholder1Dlg::OnBnClickedButton2()
{
	// TODO: Add your control notification handler code here
	m_cEdit2.SetWindowText(L"");
	m_cEdit4.SetWindowText(L"");
	m_cEdit1.SetWindowText(L"");
	m_cCheckIgnoreInserts.SetCheck(0);
	m_cCheckAllocateBuffer.SetCheck(0);
	m_cBufferSize.SetWindowText(L"");
	m_cNullFormatString.SetCheck(0);
	m_cNullOutputBuffer.SetCheck(0);
	m_cReturnValue.SetWindowText(L"");
	m_cExtError.SetWindowText(L"");
	m_cExtErrorMsg.SetWindowText(L"");
	GotoDlgCtrl(&m_cEdit2);
}

void SetClip(HWND hWnd, LPCWSTR sText)
{
    if(!wcslen(sText) || !OpenClipboard(hWnd))
		return;

	if(!EmptyClipboard()) {
		CloseClipboard();
		return;
	}

	int lenText = (int) wcslen(sText);
    HGLOBAL global_buffer = GlobalAlloc(GMEM_DDESHARE, sizeof(WCHAR) * (lenText+1));
    LPWSTR ptr_buffer = (LPWSTR) GlobalLock(global_buffer);
    memcpy(ptr_buffer, sText, sizeof(WCHAR) * lenText);
    ptr_buffer[lenText] = '\0';
    GlobalUnlock(global_buffer);
    SetClipboardData(CF_UNICODETEXT, global_buffer);
    CloseClipboard();
}

void CKorPlaceholder1Dlg::OnBnClickedButton3()
{
	// TODO: Add your control notification handler code here
	CString sFormat = L"";
	CString sArguments = L"";
	int nIgnore = 0;
	int nAllocate = 0;
	CString sBufferSize = L"";
	int nNullFormat = 0;
	int nNullBuffer = 0;
	CString sOutput = L"";
	CString sReturnValue = L"";
	CString sExtError = L"";
	CString sExtErrorMsg = L"";

	// input
	m_cEdit2.GetWindowText(sFormat);
	m_cEdit4.GetWindowText(sArguments);
	nIgnore = m_cCheckIgnoreInserts.GetCheck();
	nAllocate = m_cCheckAllocateBuffer.GetCheck();
	m_cBufferSize.GetWindowText(sBufferSize);
	nNullFormat = m_cNullFormatString.GetCheck();
	nNullBuffer = m_cNullOutputBuffer.GetCheck();
	// output
	m_cEdit1.GetWindowText(sOutput);
	m_cReturnValue.GetWindowText(sReturnValue);
	m_cExtError.GetWindowText(sExtError);
	m_cExtErrorMsg.GetWindowText(sExtErrorMsg);

	CString sCopyText;
	sCopyText.Format(
		L"[I:Format]%s\r\n"
		L"[I:Arguments]%s\r\n"
		L"[I:IgnoreInsert]%d\r\n"
		L"[I:AllocateBuffer]%d\r\n"
		L"[I:BufferSize]%s\r\n"
		L"[I:NullFormat]%d\r\n"
		L"[I:NullBuffer]%d\r\n"
		L"[O:Output]%s\r\n"
		L"[O:ReturnValue]%s\r\n"
		L"[O:ExtError]%s\r\n"
		L"[O:ExtErrorMsg]%s\r\n"
		, sFormat, sArguments, nIgnore, nAllocate, sBufferSize, nNullFormat, nNullBuffer
		, sOutput, sReturnValue, sExtError, sExtErrorMsg);

	SetClip(m_hWnd, sCopyText);
}
