#pragma once

class CDataSocket;
class CTermProj24ClientDlg;

typedef struct m_pktHdr {
	unsigned int
		chksum : 16,	//16-bit checksum
		pkt_type : 2,	//0x for CString, 1x for file
						//x0 for data only, x1 for ack included
		ack : 2,	//00 for rej, 01 for ack, 10 for RNR, 11 for RR
		dtaFrameNo : 3,	//dataFrameNo for 0~7
		ackFrameNo : 3,	//ackFrameNo for 0~7
		pkt_length : 4,	//pkt data length 0~8
		pkt_sync : 2;	//10 for beginning, 01 for end, 00 for continue, 11 for beginning and end
	bool isvalid = false;
} m_pktHdr;

typedef struct m_pkt {
	m_pktHdr pktHdr;
	WORD data[8];	//2 * 8 = 16 BYTE
} m_pkt;

typedef enum arqType
{
	NOT_DECLARED,
	STOP_AND_WAIT,
	SLIDING_WINDOW
}arqType;

typedef enum txArqBuf
{
	EMPTY,
	RNR_RECEIVED,
	RR_RECEIVED
}txArqBuf;

typedef enum rxArqBuf
{
	CLEARED,
	RECEIVED
}rxArqBuf;

typedef struct txThdArg
{
	CDataSocket *m_pSocket;
	arqType *aType;
	UINT *clientPort;
	CString *clientAddr;
	txArqBuf *pTxArqStat;
	m_pkt *txPktQueue;
	unsigned short *txQHead, *txQTail, *txQsize;
	bool thread_run;
}txThdArg;

typedef struct txPktProcThdArg
{
	txArqBuf *pTxArqStat;
	CList<m_pkt> *pTxStrPktList;
	CList<m_pkt> *pTxFilePktList;
	m_pkt *pTxPktQueue;
	unsigned short *txQHead, *txQTail, *txQsize;;
	bool thread_run;
}txPktProcThdArg;

typedef struct rxThdArg
{
	CTermProj24ClientDlg *m_pDlg;
	WORD *inputStrBuffer;
	unsigned short *strBufcount;
	bool *strBufFlag;
	WORD *inputFileBuffer;
	unsigned int *fileBufcount;
	unsigned char *fileBufFlag;
	CString *bufFileName;
	bool thread_run;
}rxThdArg;


class CMyProtocol
{
public:	//Constructor and destroyer
	CMyProtocol(CTermProj24ClientDlg *pDlg, CDataSocket *pSocket);
	~CMyProtocol();
public:	//front-end methods
	bool sendTo(CString strData, UINT nHostPort, LPCTSTR lpszHostAddress);
	bool sendTo(CFile *fileData, UINT nHostPort, LPCTSTR lpszHostAddress);
	void sendAckType(unsigned char ackType, UINT nHostPort, LPCTSTR lpszHostAddress);
	void ProcessReceive(CDataSocket *pSocket, int nErrorCode);
protected:	//back-end methods, tx part
	int segment(CString strData);
	int segment(CFile *fileData);
	void reassembly(m_pkt *pPkt);
	void chksumGen(m_pkt *pPkt);
	bool chksumVerify(m_pkt *pPkt);
	void ackReply(unsigned short dtaFrNo, bool ack);
protected:	//back-end methods, rx part, data to dialog
	void ptcStopReceived(m_pkt *rcvBuf);
	void ptcSlidReceived(m_pkt *rcvBuf);
	void ptcSetPktReceived(m_pkt *rcvBuf);
	void pktRecv(CString rasmStr);
public:	//THREADS, for tx
	CWinThread * ptxThread, *ptxPktProcThread;
	txThdArg txArg;
	txPktProcThdArg txPktProcArg;
public:	//THREADS, for rx
	CWinThread * prxThread;
	rxThdArg rxArg;
public:	//setting variable fields
	CTermProj24ClientDlg * m_pDlg;
	CDataSocket *m_pSocket;
	arqType aType;
	CString *clientAddr;
	UINT *clientPort;
public:	//txList and txQueue for efficient tx
	CList<m_pkt> txStrPktList;
	CList<m_pkt> txFilePktList;
	m_pkt txPktQueue[8];
public:	//variable for ARQ
	txArqBuf txArqStatus[8];
	rxArqBuf rxArqStatus[8];
	unsigned short txQHead, txQTail, txQsize;
	unsigned short rxQHead, rxQTail;
public:	//input buffers
	WORD * inputStrBuffer;
	unsigned short strBufcount;
	bool strBufFlag;

	WORD *inputFileBuffer;
	CString bufFileName;
	unsigned int fileBufcount;
	unsigned char fileBufFlag;


public: //Front-end methods
	bool arqTypeChange(unsigned char type);
	void quitConnection();
};

//main TX thread and RX thrad
UINT TXThread(LPVOID arg);
UINT RXThread(LPVOID arg);

//TX Enqueue thread
UINT TXPktProcThread(LPVOID arg);

//Timer thread argument and thread function prototype

typedef struct txTimerArg
{
	txArqBuf *pArqBuf;
	bool *isReceivedACK;
	bool *thread_run;
} timerArg;

typedef struct txSlidTimerArg
{
	txArqBuf *pArqBuf;
	unsigned short currentSeqNo;
	unsigned short *txQTail;
	unsigned short *txQHead;
	unsigned short *txQsize;
	bool *thd_run;
} txSlidTimerArg;

UINT Thd_saw_timer(LPVOID arg);
UINT Thd_slidWin_timer(LPVOID arg);