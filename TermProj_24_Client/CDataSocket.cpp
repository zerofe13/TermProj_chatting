#include "stdafx.h"
#include "CDataSocket.h"

#include "TermProj_24_ClientDlg.h"
#include "CMyProtocol.h"

CDataSocket::CDataSocket(CTermProj24ClientDlg * pDlg)
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

