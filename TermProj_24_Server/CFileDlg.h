#pragma once

class CMyProtocol;

// CFileDlg 대화 상자

class CFileDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CFileDlg)

public:
	CFileDlg(CWnd* pParent = nullptr);   // 표준 생성자입니다.
	virtual ~CFileDlg();

// 대화 상자 데이터입니다.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_FILE_DIALOG };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 지원입니다.
	virtual BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()
public:
	CString m_list_str;
	CListBox m_list_file;
	afx_msg void OnBnClickedFiledClose();
	afx_msg void OnLbnDblclkFileList();
	afx_msg void OnDropFiles(HDROP hDropInfo);

public:
	CMyProtocol * m_pProtocol;
	CString *clientAddr;
	UINT *clientPort;
};
