// CServerSettingDlg.cpp: 구현 파일
//

#include "stdafx.h"
#include "TermProj_24_Server.h"
#include "CServerSettingDlg.h"
#include "afxdialogex.h"

#include "CMyProtocol.h"

// CServerSettingDlg 대화 상자

IMPLEMENT_DYNAMIC(CServerSettingDlg, CDialogEx)

CServerSettingDlg::CServerSettingDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_SETTING_DIALOG, pParent)
{

}

CServerSettingDlg::~CServerSettingDlg()
{
}

void CServerSettingDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_ADDR_EDIT, m_addr_edit);
	DDX_Control(pDX, IDC_ARQ_EDIT, m_arq_edit);
}


BEGIN_MESSAGE_MAP(CServerSettingDlg, CDialogEx)
	ON_BN_CLICKED(IDC_BTN_DISCONNECT, &CServerSettingDlg::OnBnClickedBtnDisconnect)
	ON_BN_CLICKED(IDC_BTN_SAVE, &CServerSettingDlg::OnBnClickedBtnSave)
	ON_BN_CLICKED(IDC_BTN_DISCARD, &CServerSettingDlg::OnBnClickedBtnDiscard)
END_MESSAGE_MAP()

BOOL CServerSettingDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	m_addr_edit.SetWindowTextW(*clientAddr);
	if (m_pProtocol->aType == 0)
	{
		m_arq_edit.SetWindowTextW(_T("NaN"));
	}
	else if (m_pProtocol->aType == 1)
	{
		m_arq_edit.SetWindowTextW(_T("Stop and Wait"));
	}
	else if (m_pProtocol->aType == 2)
	{
		m_arq_edit.SetWindowTextW(_T("Sliding Window"));
	}
	return TRUE;  // 포커스를 컨트롤에 설정하지 않으면 TRUE를 반환합니다.
}

// CServerSettingDlg 메시지 처리기


void CServerSettingDlg::OnBnClickedBtnDisconnect()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	m_pProtocol->quitConnection();
}


void CServerSettingDlg::OnBnClickedBtnSave()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	this->DestroyWindow();
}


void CServerSettingDlg::OnBnClickedBtnDiscard()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	this->DestroyWindow();
}
