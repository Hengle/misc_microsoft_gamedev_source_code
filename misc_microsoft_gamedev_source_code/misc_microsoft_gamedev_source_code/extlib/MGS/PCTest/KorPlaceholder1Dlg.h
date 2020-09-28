// KorPlaceholder1Dlg.h : header file
//

#pragma once
#include "afxwin.h"


// CKorPlaceholder1Dlg dialog
class CKorPlaceholder1Dlg : public CDialog
{
// Construction
public:
	CKorPlaceholder1Dlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	enum { IDD = IDD_KORPLACEHOLDER1_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedButton1();
	CEdit m_cEdit2;
	CEdit m_cEdit4;
	CEdit m_cEdit1;
	CButton m_cCheckIgnoreInserts;
	CButton m_cCheckAllocateBuffer;
	CEdit m_cBufferSize;
	CButton m_cNullFormatString;
	CButton m_cNullOutputBuffer;
	CStatic m_cReturnValue;
	CStatic m_cExtError;
	CEdit m_cExtErrorMsg;
	afx_msg void OnBnClickedButton2();
	afx_msg void OnBnClickedButton3();
};
