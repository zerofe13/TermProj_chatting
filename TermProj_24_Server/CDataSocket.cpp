#include "stdafx.h"
#include "CDataSocket.h"

#include "TermProj_24_ServerDlg.h"
#include "CMyProtocol.h"

CDataSocket::CDataSocket(CTermProj24ServerDlg *pDlg)
{
	m_pDlg = pDlg;
}

CDataSocket::~CDataSocket()
{
}

void CDataSocket::OnReceive(int nErrorCode)
{
	CSocket::OnReceive(nErrorCode);
	m_pDlg->m_pProtocol->ProcessReceive(this, nErrorCode);
}
