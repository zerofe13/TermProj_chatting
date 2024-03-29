// CFileDlg.cpp: 구현 파일
//

#include "stdafx.h"
#include "TermProj_24_Server.h"
#include "CFileDlg.h"
#include "afxdialogex.h"

#include "CMyProtocol.h"


// CFileDlg 대화 상자

IMPLEMENT_DYNAMIC(CFileDlg, CDialogEx)

CFileDlg::CFileDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_FILE_DIALOG, pParent)
	, m_list_str(_T(""))
{

}

CFileDlg::~CFileDlg()
{
}

void CFileDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_LBString(pDX, IDC_FILE_LIST, m_list_str);
	DDX_Control(pDX, IDC_FILE_LIST, m_list_file);
}


BEGIN_MESSAGE_MAP(CFileDlg, CDialogEx)
	ON_LBN_DBLCLK(IDC_FILE_LIST, &CFileDlg::OnLbnDblclkFileList)
	ON_WM_DROPFILES()
	ON_BN_CLICKED(IDC_FILED_CLOSE, &CFileDlg::OnBnClickedFiledClose)
END_MESSAGE_MAP()

BOOL CFileDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	m_list_file.DragAcceptFiles(true);


	return TRUE;  // 포커스를 컨트롤에 설정하지 않으면 TRUE를 반환합니다.
}

// CFileDlg 메시지 처리기


void CFileDlg::OnBnClickedFiledClose()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	ShowWindow(SW_HIDE);
}

void CFileDlg::OnLbnDblclkFileList()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	CString FilePath;
	int nList = 0, check;

	nList = m_list_file.GetCurSel();       //인자 얻기
	m_list_file.GetText(nList, FilePath); //텍스트 얻기
	FilePath = FilePath.Right(FilePath.GetLength() - 8);

	ShellExecute(NULL, _T("open"), _T("explorer"), _T("/select,") + FilePath, NULL, SW_SHOW);
}

void CFileDlg::OnDropFiles(HDROP hDropInfo)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	DWORD dwBufSize;
	LPWSTR pFile;
	CString FullPathName;

	if (::DragQueryFile(hDropInfo, 0xffffffff, nullptr, 0) != 1) {
		AfxMessageBox(_T("하나의 파일만 넣어 주세요!"), MB_ICONERROR);
	}

	dwBufSize = DragQueryFile(hDropInfo, 0, nullptr, 0) + 1;
	pFile = (LPWSTR)new TCHAR[dwBufSize];
	::DragQueryFile(hDropInfo, 0, pFile, dwBufSize);
	FullPathName = CString(pFile);

	delete(pFile);

	if (IDYES == AfxMessageBox(FullPathName + _T("\r\n위 파일을 전송하시겠습니까?"), MB_YESNO))
	{

		CFile srcFile;
		srcFile.Open(FullPathName, CFile::modeRead | CFile::typeBinary);
		if (m_pProtocol->sendTo(&srcFile, *clientPort, *clientAddr))
		{
			m_list_file.AddString(_T("보낸 파일 : ") + FullPathName);
			srcFile.Close();
		}

	}
	else if (IDNO)
	{
		AfxMessageBox(_T("파일 전송 취소!"), MB_ICONERROR);
	}


}