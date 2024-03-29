
// TermProj_24_ServerDlg.h: 헤더 파일
//

#pragma once
#include "afxwin.h"

//Prototype define
class CDataSocket;
class CMyProtocol;

class CFileDlg;
class CServerSettingDlg;

// CTermProj24ServerDlg 대화 상자
class CTermProj24ServerDlg : public CDialogEx
{
	// 생성입니다.
public:
	CTermProj24ServerDlg(CWnd* pParent = NULL);	// 표준 생성자입니다.

// 대화 상자 데이터입니다.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_TERMPROJ_24_SERVER_DIALOG };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 지원입니다.


// 구현입니다.
protected:
	HICON m_hIcon;

	// 생성된 메시지 맵 함수
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()

public:
	CEdit m_edit;
	CEdit m_archive;
	afx_msg void OnBnClickedBtnSend();
	afx_msg void OnBnClickedBtnFile();
	afx_msg void OnBnClickedBtnSet();

public:
	CDataSocket *m_pDataSocket;
	CString clientAddr;
	UINT clientPort;
	void ProcessReceive(CString strData);

public:
	CMyProtocol *m_pProtocol;
	CServerSettingDlg *m_pServerSettingDlg;
	CFileDlg *m_pFileDlg;
};
