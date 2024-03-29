#pragma once

class CMyProtocol;

// CClientSettingDlg 대화 상자

class CClientSettingDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CClientSettingDlg)

public:
	CClientSettingDlg(CWnd* pParent = nullptr);   // 표준 생성자입니다.
	virtual ~CClientSettingDlg();

	// 대화 상자 데이터입니다.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_SETTING_DIALOG };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 지원입니다.
	virtual BOOL OnInitDialog();
	DECLARE_MESSAGE_MAP()
public:
	CComboBox m_ArqCombo;
	CEdit m_current_edit;
	afx_msg void OnBnClickedBtnSave();
	afx_msg void OnBnClickedBtnDiscard();
	afx_msg void OnBnClickedBtnDisconnect();

public:	//parents' variables
	CString *clientAddr;
	UINT *clientPort;
	CMyProtocol *m_pProtocol;
	CIPAddressCtrl m_ipAddr;
	CEdit m_portnum;
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
};
