#pragma once

#include <afxsock.h>

class CTermProj24ServerDlg;

class CDataSocket : public CSocket
{
public:
	CDataSocket(CTermProj24ServerDlg *pDlg);
	~CDataSocket();
public:
	CTermProj24ServerDlg * m_pDlg;
	virtual void OnReceive(int nErrorCode);
};

