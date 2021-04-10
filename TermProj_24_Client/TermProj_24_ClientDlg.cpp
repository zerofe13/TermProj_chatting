
// TermProj_24_ClientDlg.cpp: 구현 파일
//

#include "stdafx.h"
#include "TermProj_24_Client.h"
#include "TermProj_24_ClientDlg.h"
#include "afxdialogex.h"

#include "CDataSocket.h"
#include "CMyProtocol.h"

#include "CClientSettingDlg.h"
#include "CFileDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

CCriticalSection archive_cs;

// 응용 프로그램 정보에 사용되는 CAboutDlg 대화 상자입니다.

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 대화 상자 데이터입니다.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 지원입니다.

// 구현입니다.
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CTermProj24ClientDlg 대화 상자



CTermProj24ClientDlg::CTermProj24ClientDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(IDD_TERMPROJ_24_CLIENT_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	m_pDataSocket = NULL;
	clientAddr = "127.0.0.1";
	clientPort = 8000;
}

void CTermProj24ClientDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDIT1, m_edit);
	DDX_Control(pDX, IDC_EDIT_ARCHIVE, m_archive);
}

BEGIN_MESSAGE_MAP(CTermProj24ClientDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BTN_FILE, &CTermProj24ClientDlg::OnBnClickedBtnFile)
	ON_BN_CLICKED(IDC_BTN_SET, &CTermProj24ClientDlg::OnBnClickedBtnSet)
	ON_BN_CLICKED(IDC_BTN_SEND, &CTermProj24ClientDlg::OnBnClickedBtnSend)
END_MESSAGE_MAP()

// CTermProj24ClientDlg 메시지 처리기

BOOL CTermProj24ClientDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 시스템 메뉴에 "정보..." 메뉴 항목을 추가합니다.

	// IDM_ABOUTBOX는 시스템 명령 범위에 있어야 합니다.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 이 대화 상자의 아이콘을 설정합니다.  응용 프로그램의 주 창이 대화 상자가 아닐 경우에는
	//  프레임워크가 이 작업을 자동으로 수행합니다.

	SetIcon(m_hIcon, TRUE);			// 큰 아이콘을 설정합니다.
	SetIcon(m_hIcon, FALSE);		// 작은 아이콘을 설정합니다.



	WSADATA wsa;
	int error_code;
	if ((error_code = WSAStartup(MAKEWORD(2, 2), &wsa)) != 0) {
		TCHAR buffer[256];
		FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM, NULL, error_code, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), buffer, 256, NULL);
		AfxMessageBox(buffer, MB_ICONERROR);
	}
	ASSERT(m_pDataSocket == NULL);
	m_pDataSocket = new CDataSocket(this);
	if (m_pDataSocket->Create(8002, SOCK_DGRAM)) {
		m_edit.SetFocus();
		m_pProtocol = new CMyProtocol(this, m_pDataSocket);

		//File Transfer Dialog
		m_pFileDlg = new CFileDlg();
		m_pFileDlg->m_pProtocol = this->m_pProtocol;
		m_pFileDlg->clientAddr = &this->clientAddr;
		m_pFileDlg->clientPort = &this->clientPort;
		m_pFileDlg->Create(IDD_FILE_DIALOG, this);

		return TRUE;
	}
	else {
		int err = m_pDataSocket->GetLastError();
		TCHAR buffer[256];
		FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM, NULL, err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), buffer, 256, NULL);
		AfxMessageBox(buffer, MB_ICONERROR);
	}

	return TRUE;  // 포커스를 컨트롤에 설정하지 않으면 TRUE를 반환합니다.
}

void CTermProj24ClientDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialog::OnSysCommand(nID, lParam);
	}
}

// 대화 상자에 최소화 단추를 추가할 경우 아이콘을 그리려면
//  아래 코드가 필요합니다.  문서/뷰 모델을 사용하는 MFC 응용 프로그램의 경우에는
//  프레임워크에서 이 작업을 자동으로 수행합니다.

void CTermProj24ClientDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 그리기를 위한 디바이스 컨텍스트입니다.

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 클라이언트 사각형에서 아이콘을 가운데에 맞춥니다.
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 아이콘을 그립니다.
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

// 사용자가 최소화된 창을 끄는 동안에 커서가 표시되도록 시스템에서
//  이 함수를 호출합니다.
HCURSOR CTermProj24ClientDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CTermProj24ClientDlg::ProcessReceive(CString strData)
{
	archive_cs.Lock();
	int len = m_archive.GetWindowTextLengthW();
	m_archive.SetSel(len, len);
	m_archive.ReplaceSel(_T("Server : ") + strData);
	archive_cs.Unlock();
}

void CTermProj24ClientDlg::OnBnClickedBtnSend()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	if (m_pDataSocket == NULL) {
		MessageBox(_T("m_pDataSocket이 만들어지지 않았습니다!"), _T("알림"), MB_ICONINFORMATION);
	}
	else {
		CString tx_message;
		m_edit.GetWindowTextW(tx_message);
		tx_message += _T("\r\n");
		if (m_pProtocol->sendTo(tx_message, clientPort, clientAddr))
		{
			m_edit.SetWindowTextW(_T(""));
			m_edit.SetFocus();

			archive_cs.Lock();
			int len = m_archive.GetWindowTextLengthW();
			m_archive.SetSel(len, len);
			m_archive.ReplaceSel(_T("Client : ") + tx_message);
			archive_cs.Unlock();
		}
	}
}

void CTermProj24ClientDlg::OnBnClickedBtnFile()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	m_pFileDlg->ShowWindow(SW_SHOW);
}


void CTermProj24ClientDlg::OnBnClickedBtnSet()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	m_pClientSettingDlg = new CClientSettingDlg();
	m_pClientSettingDlg->clientAddr = &this->clientAddr;
	m_pClientSettingDlg->clientPort = &this->clientPort;
	m_pClientSettingDlg->m_pProtocol = this->m_pProtocol;

	m_pClientSettingDlg->Create(IDD_SETTING_DIALOG, this);
	m_pClientSettingDlg->ShowWindow(SW_SHOW);
}



