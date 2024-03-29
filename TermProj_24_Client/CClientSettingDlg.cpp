// CClientSettingDlg.cpp: 구현 파일
//

#include "stdafx.h"
#include "TermProj_24_Client.h"
#include "CClientSettingDlg.h"
#include "afxdialogex.h"

#include "CMyProtocol.h"


// CClientSettingDlg 대화 상자

IMPLEMENT_DYNAMIC(CClientSettingDlg, CDialogEx)

CClientSettingDlg::CClientSettingDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_SETTING_DIALOG, pParent)
{

}

CClientSettingDlg::~CClientSettingDlg()
{
}

void CClientSettingDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_ARQ_COMBO, m_ArqCombo);
	DDX_Control(pDX, IDC_EDIT1, m_current_edit);
	DDX_Control(pDX, IDC_IPADDR, m_ipAddr);
	DDX_Control(pDX, IDC_PORT, m_portnum);
}


BEGIN_MESSAGE_MAP(CClientSettingDlg, CDialogEx)
	ON_BN_CLICKED(ID_BTN_SAVE, &CClientSettingDlg::OnBnClickedBtnSave)
	ON_BN_CLICKED(ID_BTN_DISCARD, &CClientSettingDlg::OnBnClickedBtnDiscard)
	ON_BN_CLICKED(ID_BTN_DISCONNECT, &CClientSettingDlg::OnBnClickedBtnDisconnect)

	ON_WM_SYSCOMMAND()
END_MESSAGE_MAP()

BOOL CClientSettingDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	m_current_edit.SetWindowTextW(*clientAddr);

	if (m_pProtocol->aType > 0)
	{
		m_ArqCombo.AddString(_T("Stop and Wait"));
		m_ArqCombo.AddString(_T("Sliding Window"));
		m_ArqCombo.SetCurSel(m_pProtocol->aType - 1);
	}
	else
	{
		m_ArqCombo.AddString(_T("NaN"));
		m_ArqCombo.AddString(_T("Stop and Wait"));
		m_ArqCombo.AddString(_T("Sliding Window"));
		m_ArqCombo.SetCurSel(m_pProtocol->aType);
	}

	m_ipAddr.SetWindowTextW(_T("127.0.0.1"));
	m_portnum.SetWindowTextW(_T("8000"));
	return TRUE;  // 포커스를 컨트롤에 설정하지 않으면 TRUE를 반환합니다.
}
// CClientSettingDlg 메시지 처리기


void CClientSettingDlg::OnBnClickedBtnSave()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	unsigned char index = m_ArqCombo.GetCurSel();
	if (m_pProtocol->arqTypeChange(m_ArqCombo.GetCurSel()))
	{
		m_pProtocol->sendAckType(index, *clientPort, *clientAddr);
		MessageBox(_T("성공적으로 ARQ Type을 변경하였습니다."), _T("알림"), MB_ICONINFORMATION);
		this->DestroyWindow();
	}
	else
	{
		MessageBox(_T("최초 설정 이후에는 설정을 변경할 수 없습니다."), _T("알림"), MB_ICONINFORMATION);
	}
}


void CClientSettingDlg::OnBnClickedBtnDiscard()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	this->DestroyWindow();
}


void CClientSettingDlg::OnBnClickedBtnDisconnect()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	m_pProtocol->quitConnection();
}


void CClientSettingDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	if (nID == SC_CLOSE)
	{
		this->DestroyWindow();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}
