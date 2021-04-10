#pragma once

#include <afxsock.h>

class CTermProj24ClientDlg;

class CDataSocket : public CSocket
{
public:
	CDataSocket(CTermProj24ClientDlg *pDlg);
	~CDataSocket();
public:
	CTermProj24ClientDlg * m_pDlg;
	virtual void OnReceive(int nErrorCode);
};

