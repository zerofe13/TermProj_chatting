#pragma once

class CMyProtocol;

// CServerSettingDlg 대화 상자

class CServerSettingDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CServerSettingDlg)

public:
	CServerSettingDlg(CWnd* pParent = nullptr);   // 표준 생성자입니다.
	virtual ~CServerSettingDlg();

// 대화 상자 데이터입니다.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_SETTING_DIALOG };
#endif
	virtual BOOL OnInitDialog();
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 지원입니다.

	DECLARE_MESSAGE_MAP()
public:
	CEdit m_addr_edit;
	CEdit m_arq_edit;
	afx_msg void OnBnClickedBtnDisconnect();
	afx_msg void OnBnClickedBtnSave();
	afx_msg void OnBnClickedBtnDiscard();

public:	//parents' variables
	CString * clientAddr;
	CMyProtocol *m_pProtocol;
};
