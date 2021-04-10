#include "stdafx.h"
#include "CMyProtocol.h"

#include "CDataSocket.h"
#include "TermProj_24_ClientDlg.h"
#include "CFileDlg.h"

CCriticalSection txStrPkt_cs;	//this critical section is for txStrPktList
CCriticalSection txFilePkt_cs;

CCriticalSection txQ_cs;
CCriticalSection rxQ_cs;
CCriticalSection rxSL_cs;
CCriticalSection rxFL_cs;

CCriticalSection timer_cs;

/*********** C O N S T R U C T O R & D E S T R O Y E R ********************/

CMyProtocol::CMyProtocol(CTermProj24ClientDlg *pDlg, CDataSocket *pSocket)
{
	//initiate setting variable fields...
	this->m_pDlg = pDlg;
	this->m_pSocket = pSocket;
	this->aType = NOT_DECLARED;	//basically stop and wait
	this->clientAddr = &pDlg->clientAddr;
	this->clientPort = &pDlg->clientPort;

	//initiate variable for ARQ
	for (int i = 0; i < 8; i++)
	{
		this->txArqStatus[i] = RNR_RECEIVED;
		this->rxArqStatus[i] = CLEARED;
	}

	this->txQHead = 0;
	this->rxQHead = 7;
	this->txQTail = 0;
	this->rxQTail = 0;
	this->txQsize = 0;
	this->bufFileName = _T("");

	//initiate input Buffers
	this->inputStrBuffer = (WORD*)calloc(sizeof(WORD), 256);
	this->strBufcount = 0;
	this->strBufFlag = false;

	this->inputFileBuffer = (WORD*)calloc(sizeof(WORD), 5242880);	//10MB buffer
	this->fileBufcount = 0;
	this->fileBufFlag = 0b00;

	//initiate Thread arguments
	this->txArg.m_pSocket = pSocket;
	this->txArg.aType = &this->aType;
	this->txArg.clientPort = this->clientPort;
	this->txArg.clientAddr = this->clientAddr;
	this->txArg.pTxArqStat = this->txArqStatus;
	this->txArg.txPktQueue = this->txPktQueue;
	this->txArg.txQHead = &this->txQHead;
	this->txArg.txQTail = &this->txQTail;
	this->txArg.txQsize = &this->txQsize;
	this->txArg.thread_run = true;
	ptxThread = AfxBeginThread(TXThread, (LPVOID)&txArg);

	this->rxArg.m_pDlg = pDlg;
	this->rxArg.inputStrBuffer = this->inputStrBuffer;
	this->rxArg.strBufcount = &this->strBufcount;
	this->rxArg.strBufFlag = &this->strBufFlag;
	this->rxArg.inputFileBuffer = this->inputFileBuffer;
	this->rxArg.fileBufcount = &this->fileBufcount;
	this->rxArg.fileBufFlag = &this->fileBufFlag;
	this->rxArg.bufFileName = &this->bufFileName;
	this->rxArg.thread_run = true;
	prxThread = AfxBeginThread(RXThread, (LPVOID)&rxArg);

	this->txPktProcArg.pTxArqStat = this->txArqStatus;
	this->txPktProcArg.pTxStrPktList = &this->txStrPktList;
	this->txPktProcArg.pTxFilePktList = &this->txFilePktList;
	this->txPktProcArg.pTxPktQueue = this->txPktQueue;
	this->txPktProcArg.txQHead = &this->txQHead;
	this->txPktProcArg.txQTail = &this->txQTail;
	this->txPktProcArg.txQsize = &this->txQsize;
	this->txPktProcArg.thread_run = true;
	ptxPktProcThread = AfxBeginThread(TXPktProcThread, (LPVOID)&txPktProcArg);


}

CMyProtocol::~CMyProtocol()
{
	//terminate threads
	this->txArg.thread_run = false;
	this->rxArg.thread_run = false;
	this->txPktProcArg.thread_run = false;

	//free inputbuffers
	delete[] this->inputStrBuffer;
	this->inputStrBuffer = NULL;
}

/***************** F R O N T - E N D - M E T H O D S **********************/

bool CMyProtocol::sendTo(CString strData, UINT nHostPort, LPCTSTR lpszHostAddress)
{
	/*	This method is FRONT_END METHOD.
	This method call segment function to divide a strData by 16 bytes.
	Segment method put segmented data into txStrPktList.
	chksumGen method fills those packet's checksum field .
	Finally, set packet's isvalid field as true. */
	if (aType != NOT_DECLARED)
	{
		segment(strData);
		return true;
	}
	else {
		AfxMessageBox(_T("ACK Type이 설정되지 않았습니다. Client에서 설정해 주세요."), MB_ICONERROR);
		return false;
	}
}

bool CMyProtocol::sendTo(CFile *fileData, UINT nHostPort, LPCTSTR lpszHostAddress)
{
	/*	This method is FRONT_END METHOD.
	This method call segment function to divide a strData by 16 bytes.
	Segment method put segmented data into txStrPktList.
	chksumGen method fills those packet's checksum field .
	Finally, set packet's isvalid field as true. */
	if (aType != NOT_DECLARED)
	{
		segment(fileData);
		return true;
	}
	else {
		AfxMessageBox(_T("ACK Type이 설정되지 않았습니다. Client에서 설정해 주세요."), MB_ICONERROR);
		return false;
	}
}

void CMyProtocol::sendAckType(unsigned char ackType, UINT nHostPort, LPCTSTR lpszHostAddress)
{
	m_pkt pktTmp;	//initialize packet.
	if (ackType == STOP_AND_WAIT)
		pktTmp.pktHdr.pkt_type = 0b00;	//00 for string data + data only
	else if (ackType == SLIDING_WINDOW)
		pktTmp.pktHdr.pkt_type = 0b11;	//00 for string data + data only
	else
		pktTmp.pktHdr.pkt_type = 0b01;
	m_pSocket->SendToEx(&pktTmp, sizeof(m_pkt), *clientPort, *clientAddr, 0);

}

void CMyProtocol::ProcessReceive(CDataSocket *pSocket, int nErrorCode)
{
	/*  This method is called by asynchronous function OnReceive() by CDataSocket.
	This method distinguish whether this pkt is an ACK, DATA, or PIGGYBACK.
	After judgement, take proper action(like calling ackReply method).
	Then, this method sends it to the appropriate list.
	The reassembly operation is handled by RXThread. */
	m_pkt recvBuffer;
	CString tmpAddr;
	pSocket->ReceiveFromEx(&recvBuffer, sizeof(m_pkt), tmpAddr, *clientPort, 0);

	if (aType == STOP_AND_WAIT)		//STOP AND WAIT ******************************************
		ptcStopReceived(&recvBuffer);
	else if (aType == SLIDING_WINDOW)		//SLIDING WINDOW **********************
		ptcSlidReceived(&recvBuffer);
	else 
		;	//ACK TYPE NOT DECLARED

}

/******************* B A C K - E N D - M E T H O D S **********************/

int CMyProtocol::segment(CString strData)
{
	/*  This method divide strData by 16 bytes.
	Divided strDatas are packaged in a packet struct and sent to txStrPktList.
	Of course, those packet's isValid fields are filled as false.
	Permission for activating packet is on sendTo function. */

	WORD* temp;
	temp = new WORD[strData.GetLength() + 1];
	temp = (WORD*)(LPCTSTR)strData;	//(BYTE*)=(LPBYTE);

	int pkt_num = (strData.GetLength() + 1) / 8;
	if ((strData.GetLength() + 1) % 8 > 0)
		pkt_num++;

	for (int pktseq = 0; pktseq < pkt_num; pktseq++)
	{
		m_pkt pktTmp;	//initialize packet.
		pktTmp.pktHdr.pkt_type = 0b00;	//00 for string data + data only
		pktTmp.pktHdr.ack = 0b00;	//ignored if pkt_type is x0
		pktTmp.pktHdr.dtaFrameNo = 0b000;	//set on TXPktProcThread
		pktTmp.pktHdr.ackFrameNo = 0b000;	//set on TXPktProcThread
		pktTmp.pktHdr.pkt_length = 0b0000;
		pktTmp.pktHdr.pkt_sync = 0b00;

		for (int i = 0; i < 8; i++)
		{
			if (i + (8 * pktseq) > strData.GetLength() + 1)
				break;
			pktTmp.data[i] = temp[i + (8 * pktseq)];
			pktTmp.pktHdr.pkt_length++;
		}

		if (pktseq == 0)
			pktTmp.pktHdr.pkt_sync = 0b10;
		if ((pktseq + 1) == pkt_num)
			pktTmp.pktHdr.pkt_sync = 0b01;

		chksumGen(&pktTmp);
		pktTmp.pktHdr.isvalid = true;

		txStrPkt_cs.Lock();	//this function does not work in parallel.
		txStrPktList.AddTail(pktTmp);
		txStrPkt_cs.Unlock();
	}
	return pkt_num;
}

int CMyProtocol::segment(CFile *fileData)
{
	/*  This method divide strData by 16 bytes.
	Divided strDatas are packaged in a packet struct and sent to txStrPktList.
	Of course, those packet's isValid fields are filled as false.
	Permission for activating packet is on sendTo function. */

	int totalPktNum = 0;

	//Send fileTitle
	CString fName = fileData->GetFileName();
	WORD* fileName;
	fileName = new WORD[fName.GetLength() + 1];
	fileName = (WORD*)(LPCTSTR)fName;	//(BYTE*)=(LPBYTE);

	int pkt_num = (fName.GetLength() + 1) / 8;
	if ((fName.GetLength() + 1) % 8 > 0)
		pkt_num++;
	totalPktNum += pkt_num;

	//pack into Packet
	for (int pktseq = 0; pktseq < pkt_num; pktseq++)
	{
		m_pkt pktTmp;	//initialize packet.
		pktTmp.pktHdr.pkt_type = 0b10;	//10 for file data + data only
		pktTmp.pktHdr.ack = 0b00;	//ignored if pkt_type is x0
		pktTmp.pktHdr.dtaFrameNo = 0b000;	//set on TXPktProcThread
		pktTmp.pktHdr.ackFrameNo = 0b000;	//set on TXPktProcThread
		pktTmp.pktHdr.pkt_length = 0b0000;
		pktTmp.pktHdr.pkt_sync = 0b00;

		for (int i = 0; i < 8; i++)
		{
			if (i + (8 * pktseq) > fName.GetLength() + 1)
				break;
			pktTmp.data[i] = fileName[i + (8 * pktseq)];
			pktTmp.pktHdr.pkt_length++;
		}

		if (pktseq == 0)
			pktTmp.pktHdr.pkt_sync = 0b10;
		if ((pktseq + 1) == pkt_num)
			pktTmp.pktHdr.pkt_sync = 0b01;

		chksumGen(&pktTmp);
		pktTmp.pktHdr.isvalid = true;

		txFilePkt_cs.Lock();	//this function does not work in parallel.
		txFilePktList.AddTail(pktTmp);
		txFilePkt_cs.Unlock();
	}


	//Send fileData
	WORD* fileBinary;
	fileBinary = new WORD[fileData->GetLength()];
	fileData->Read(fileBinary, fileData->GetLength());

	pkt_num = (fileData->GetLength() + 1) / 8;
	if ((fileData->GetLength() + 1) % 8 > 0)
		pkt_num++;
	totalPktNum += pkt_num;

	//pack into Packet
	for (int pktseq = 0; pktseq < pkt_num; pktseq++)
	{
		m_pkt pktTmp;	//initialize packet.
		pktTmp.pktHdr.pkt_type = 0b10;	//10 for file data + data only
		pktTmp.pktHdr.ack = 0b00;	//ignored if pkt_type is x0
		pktTmp.pktHdr.dtaFrameNo = 0b000;	//set on TXPktProcThread
		pktTmp.pktHdr.ackFrameNo = 0b000;	//set on TXPktProcThread
		pktTmp.pktHdr.pkt_length = 0b0000;
		pktTmp.pktHdr.pkt_sync = 0b00;

		for (int i = 0; i < 8; i++)
		{
			if (i + (8 * pktseq) > fileData->GetLength())
				break;
			pktTmp.data[i] = fileBinary[i + (8 * pktseq)];
			pktTmp.pktHdr.pkt_length++;
		}

		if (pktseq == 0)
			pktTmp.pktHdr.pkt_sync = 0b10;
		if ((pktseq + 1) == pkt_num)
			pktTmp.pktHdr.pkt_sync = 0b01;

		chksumGen(&pktTmp);
		pktTmp.pktHdr.isvalid = true;

		txFilePkt_cs.Lock();	//this function does not work in parallel.
		txFilePktList.AddTail(pktTmp);
		txFilePkt_cs.Unlock();
	}

	return totalPktNum;
}

void CMyProtocol::reassembly(m_pkt *pPkt)
{

}

void CMyProtocol::chksumGen(m_pkt *pPkt)
{
	unsigned int result = 0;

	for (int i = 0; i < pPkt->pktHdr.pkt_length; i++)
	{
		result += pPkt->data[i];
		if ((result & 0b10000000000000000) == 0b10000000000000000)
		{
			result -= 0b10000000000000000;
			result++;
		}
	}

	pPkt->pktHdr.chksum = ~(unsigned short)result;
}

bool CMyProtocol::chksumVerify(m_pkt *pPkt)
{
	unsigned int result = 0;

	if (pPkt->pktHdr.pkt_length == 0)
		return true;

	for (int i = 0; i < pPkt->pktHdr.pkt_length; i++)
	{
		result += pPkt->data[i];
		if ((result & 0b10000000000000000) == 0b10000000000000000)
		{
			result -= 0b10000000000000000;
			result++;
		}
	}

	unsigned short rslt = ~((unsigned short)result + (unsigned short)pPkt->pktHdr.chksum);
	return (rslt == 0);
}

void CMyProtocol::ackReply(unsigned short dtaFrNo, bool ack)
{
	/*  [IMPORTANT] "DO NOT" USE THIS METHOD AFTER RX_QUEUE_CRITICAL_SECTION.LOCK() !!!!!
	First of all, We defined ack as following :
	00 REJ				01 ACK

	Then, We defined pkt_type as following :
	00 CString Only		01 Cstring + ack
	10 File Only		11 File + ack

	x1 pkt type means ACK included.
	But if pkt_length is zero, we can interpret that that packet is ACK only.
	if pkt_length is more than zero, we can interpret that that packet is PIGGYBACK pkt. */

	if (aType == STOP_AND_WAIT) {
		if (ack == true)
		{
			txQ_cs.Lock();
			if (txQsize == 0)
			{
				txQ_cs.Unlock();
				m_pkt ackPkt;
				ackPkt.pktHdr.pkt_type = 01;
				ackPkt.pktHdr.ack = 01;		//01 is ACK
				ackPkt.pktHdr.dtaFrameNo = 0;
				ackPkt.pktHdr.ackFrameNo = (dtaFrNo + 1) % 8;		//we require transmit next frame
				ackPkt.pktHdr.pkt_length = 0;
				ackPkt.pktHdr.pkt_sync = 11;
				ackPkt.pktHdr.isvalid = true;
				m_pSocket->SendToEx(&ackPkt, sizeof(m_pkt), *clientPort, *clientAddr, 0);
			}
			/*else {	//piggyback
			tx_cs.Lock();
			rxArqStatus[pPkt->pktHdr.dtaFrameNo] = ACK_TRANSMITTED;
			tx_cs.Unlock();
			txPktQueue[seqHead].pktHdr.pkt_type + 1;
			currentFrNo = (currentFrNo + 1) % 8;
			}
			*/
			//txQ_cs.Unlock();
		}
		else if (ack == false) {

			txQ_cs.Lock();
			//if (txQsize == 0)
			{
				txQ_cs.Unlock();
				m_pkt ackPkt;
				ackPkt.pktHdr.pkt_type = 01;
				ackPkt.pktHdr.ack = 00;		//00 is REJ
				ackPkt.pktHdr.dtaFrameNo = 0;
				ackPkt.pktHdr.ackFrameNo = dtaFrNo;	//we require retransmit dtaFrNo.
				ackPkt.pktHdr.pkt_length = 0;
				ackPkt.pktHdr.pkt_sync = 11;
				ackPkt.pktHdr.isvalid = true;
				m_pSocket->SendToEx(&ackPkt, sizeof(m_pkt), *clientPort, *clientAddr, 0);
			}
			/*else {	//piggyback
			tx_cs.Lock();
			rxArqStatus[pPkt->pktHdr.dtaFrameNo] = ACK_TRANSMITTED;
			tx_cs.Unlock();
			txPktQueue[seqHead].pktHdr.pkt_type + 1;
			currentFrNo = (currentFrNo + 1) % 8;
			}
			*/
			//txQ_cs.Unlock();
		}
	}
	else if (aType == SLIDING_WINDOW) {
		if (ack == true)
		{
			txQ_cs.Lock();
			if (txQsize == 0)
			{
				txQ_cs.Unlock();
				m_pkt ackPkt;
				ackPkt.pktHdr.pkt_type = 01;
				ackPkt.pktHdr.ack = 01;		//01 is ACK
				ackPkt.pktHdr.dtaFrameNo = 0;
				ackPkt.pktHdr.ackFrameNo = (dtaFrNo + 1) % 8;		//we require transmit next frame
				ackPkt.pktHdr.pkt_length = 0;
				ackPkt.pktHdr.pkt_sync = 11;
				ackPkt.pktHdr.isvalid = true;
				m_pSocket->SendToEx(&ackPkt, sizeof(m_pkt), *clientPort, *clientAddr, 0);
			}
			/*else {	//piggyback
			tx_cs.Lock();
			rxArqStatus[pPkt->pktHdr.dtaFrameNo] = ACK_TRANSMITTED;
			tx_cs.Unlock();
			txPktQueue[seqHead].pktHdr.pkt_type + 1;
			currentFrNo = (currentFrNo + 1) % 8;
			}
			*/
			//txQ_cs.Unlock();
		}
		else if (ack == false) {

			txQ_cs.Lock();
			//if (txQsize == 0)
			{
				txQ_cs.Unlock();
				m_pkt ackPkt;
				ackPkt.pktHdr.pkt_type = 01;
				ackPkt.pktHdr.ack = 00;		//00 is REJ
				ackPkt.pktHdr.dtaFrameNo = 0;
				ackPkt.pktHdr.ackFrameNo = dtaFrNo;	//we require retransmit dtaFrNo.
				ackPkt.pktHdr.pkt_length = 0;
				ackPkt.pktHdr.pkt_sync = 11;
				ackPkt.pktHdr.isvalid = true;
				m_pSocket->SendToEx(&ackPkt, sizeof(m_pkt), *clientPort, *clientAddr, 0);
			}
			/*else {	//piggyback
			tx_cs.Lock();
			rxArqStatus[pPkt->pktHdr.dtaFrameNo] = ACK_TRANSMITTED;
			tx_cs.Unlock();
			txPktQueue[seqHead].pktHdr.pkt_type + 1;
			currentFrNo = (currentFrNo + 1) % 8;
			}
			*/
			//txQ_cs.Unlock();
		}
	}
	else {
		//ACK NOT DECLARED
	}
}

void CMyProtocol::ptcStopReceived(m_pkt * rcvBuf)
{
	if (rcvBuf->pktHdr.pkt_length == 0)	//ACK only or SETTING pkt
	{
		switch (rcvBuf->pktHdr.pkt_type & 0b11)
		{
		case 0b00:	//special packet - ACK confirm
			m_pDlg->ProcessReceive(_T("## Confirm ARQ type 'Stop and Wait' ##\r\n"));
			break;
		case 0b10:	//special packet - QUIT CONNECTION
			AfxMessageBox(_T("연결 종료 요청을 수신하였습니다."), MB_ICONERROR);
			AfxGetMainWnd()->PostMessageW(WM_CLOSE);
			break;
		case 0b01:	//string data ack
		case 0b11:	//file data ack
		{
			txQ_cs.Lock();
			if (txArqStatus[rcvBuf->pktHdr.ackFrameNo] != RR_RECEIVED)	//ack waiting
			{
				//ACK for recvBuffer.pktHdr.ackFrameNo
				txArqStatus[rcvBuf->pktHdr.ackFrameNo] = RR_RECEIVED;
			}
			else if (txArqStatus[rcvBuf->pktHdr.ackFrameNo] == RR_RECEIVED)
			{
				//duplicated ACK... nothing to do.
			}
			txQ_cs.Unlock();
			break;
		}
		}
	}
	else	//Data or Piggyback Pkt
	{
		if (chksumVerify(rcvBuf))
		{
			switch (rcvBuf->pktHdr.pkt_type & 0b11)
			{
			case 0b00:	//string data only packet.
			{
				ackReply(rcvBuf->pktHdr.dtaFrameNo, true);
				rxQ_cs.Lock();
				if (rxArqStatus[rcvBuf->pktHdr.dtaFrameNo] == RECEIVED)
				{
					rxQ_cs.Unlock();	//discard
					break;
				}
				else if (rxArqStatus[rcvBuf->pktHdr.dtaFrameNo] == CLEARED)
				{
					rxArqStatus[(rcvBuf->pktHdr.dtaFrameNo + 7) % 8] = CLEARED;
					rxArqStatus[rcvBuf->pktHdr.dtaFrameNo] = RECEIVED;
					rxQ_cs.Unlock();
					while (true)
					{
						rxSL_cs.Lock();
						if (strBufFlag == true)
						{
							rxSL_cs.Unlock();
							Sleep(30);
							continue;
						}
						break;
					}
					for (int i = 0; i < rcvBuf->pktHdr.pkt_length; i++)
						inputStrBuffer[strBufcount++] = rcvBuf->data[i];
					if (rcvBuf->pktHdr.pkt_sync & 0b01 == 0b01)
						strBufFlag = true;
					rxSL_cs.Unlock();
					break;
				}

			}
			case 0b10:	//file data only packet
			{
				ackReply(rcvBuf->pktHdr.dtaFrameNo, true);
				rxQ_cs.Lock();
				if (rxArqStatus[rcvBuf->pktHdr.dtaFrameNo] == RECEIVED)
				{
					rxQ_cs.Unlock();	//discard
					break;
				}
				else if (rxArqStatus[rcvBuf->pktHdr.dtaFrameNo] == CLEARED)
				{
					rxArqStatus[(rcvBuf->pktHdr.dtaFrameNo + 7) % 8] = CLEARED;
					rxArqStatus[rcvBuf->pktHdr.dtaFrameNo] = RECEIVED;
					rxQ_cs.Unlock();
					while (true)
					{
						rxFL_cs.Lock();
						if (fileBufFlag & 0b01 == 0b01)
						{
							rxFL_cs.Unlock();
							Sleep(30);
							continue;
						}
						break;
					}
					for (int i = 0; i < rcvBuf->pktHdr.pkt_length; i++)
						inputFileBuffer[fileBufcount++] = rcvBuf->data[i];
					if (rcvBuf->pktHdr.pkt_sync & 0b01 == 0b01)
						fileBufFlag += 0b01;
					rxFL_cs.Unlock();
					break;
				}

			}
			case 0b01:	//string data + string data ack (piggyback) XXX
			{
				break;
			}
			case 0b11:	//file data + file data ack (piggyback) XXX
			{
				break;
			}
			}
		}
		else	//chksum failure.
		{
			rxQ_cs.Lock();
			ackReply(rcvBuf->pktHdr.ackFrameNo, false);
			rxQ_cs.Unlock();
		}
	}
}

void CMyProtocol::ptcSlidReceived(m_pkt * rcvBuf)
{
	if (rcvBuf->pktHdr.pkt_length == 0)	//ACK only or SETTING pkt
	{
		switch (rcvBuf->pktHdr.pkt_type & 0b11)
		{
		case 0b00:	//special packet - ACK confirm
			m_pDlg->ProcessReceive(_T("## Confirm ARQ type 'Sliding Window' ##\r\n"));
			break;
		case 0b10:	//special packet - QUIT CONNECTION
			AfxMessageBox(_T("연결 종료 요청을 수신하였습니다."), MB_ICONERROR);
			AfxGetMainWnd()->PostMessageW(WM_CLOSE);
			break;
		case 0b01:	//string data ack
		case 0b11:	//file data ack
		{
			txQ_cs.Lock();
			if (txArqStatus[rcvBuf->pktHdr.ackFrameNo] != RR_RECEIVED)	//ack waiting
			{
				//ACK for recvBuffer.pktHdr.ackFrameNo
				for (int i = txQTail; i != rcvBuf->pktHdr.ackFrameNo; i = (i + 1) % 8)
				{
					if (txArqStatus[i] == EMPTY)
					{
						txQ_cs.Unlock();
						return;	//duplicated ACK
					}
					else {
						txQsize--;
						txArqStatus[i] = EMPTY;
					}
				}
				txArqStatus[rcvBuf->pktHdr.ackFrameNo] = RR_RECEIVED;

				txQTail = rcvBuf->pktHdr.ackFrameNo;
			}
			else if (txArqStatus[rcvBuf->pktHdr.ackFrameNo] == RR_RECEIVED)
			{
				//duplicated ACK... nothing to do.
			}
			txQ_cs.Unlock();
			break;
		}
		}
	}
	else	//Data or Piggyback Pkt
	{
		if (chksumVerify(rcvBuf))
		{
			switch (rcvBuf->pktHdr.pkt_type & 0b11)
			{
			case 0b00:	//string data only packet.
			{
				ackReply(rcvBuf->pktHdr.dtaFrameNo, true);
				rxQ_cs.Lock();
				if (rxArqStatus[rcvBuf->pktHdr.dtaFrameNo] == RECEIVED)
				{
					rxQ_cs.Unlock();	//discard
					break;
				}
				else if (rxArqStatus[rcvBuf->pktHdr.dtaFrameNo] == CLEARED)
				{
					for (int i = rxQTail; i != rcvBuf->pktHdr.dtaFrameNo; i = (i + 1) % 8)
					{
						rxArqStatus[i] = RECEIVED;
					}
					rxQTail = rcvBuf->pktHdr.dtaFrameNo;
					rxArqStatus[rxQTail] = RECEIVED;
					for (int i = rxQHead; i != rxQTail; i = (i + 1) % 8)
					{
						rxArqStatus[i] = CLEARED;
					}
					rxQHead = (rxQTail + 7) % 8;
					rxQ_cs.Unlock();

					while (true)
					{
						rxSL_cs.Lock();
						if (strBufFlag == true)
						{
							rxSL_cs.Unlock();
							Sleep(30);
							continue;
						}
						break;
					}
					for (int i = 0; i < rcvBuf->pktHdr.pkt_length; i++)
						inputStrBuffer[strBufcount++] = rcvBuf->data[i];
					if (rcvBuf->pktHdr.pkt_sync & 0b01 == 0b01)
						strBufFlag = true;
					rxSL_cs.Unlock();
					break;
				}

			}
			case 0b10:	//file data only packet
			{
				ackReply(rcvBuf->pktHdr.dtaFrameNo, true);
				rxQ_cs.Lock();
				if (rxArqStatus[rcvBuf->pktHdr.dtaFrameNo] == RECEIVED)
				{
					rxQ_cs.Unlock();	//discard
					break;
				}
				else if (rxArqStatus[rcvBuf->pktHdr.dtaFrameNo] == CLEARED)
				{
					for (int i = rxQTail; i != rcvBuf->pktHdr.dtaFrameNo; i = (i + 1) % 8)
					{
						rxArqStatus[i] = RECEIVED;
					}
					rxQTail = rcvBuf->pktHdr.dtaFrameNo;
					rxArqStatus[rxQTail] = RECEIVED;
					for (int i = rxQHead; i != rxQTail; i = (i + 1) % 8)
					{
						rxArqStatus[i] = CLEARED;
					}
					rxQHead = (rxQTail + 7) % 8;
					rxQ_cs.Unlock();
					while (true)
					{
						rxFL_cs.Lock();
						if (fileBufFlag & 0b01 == 0b01)
						{
							rxFL_cs.Unlock();
							Sleep(30);
							continue;
						}
						break;
					}
					for (int i = 0; i < rcvBuf->pktHdr.pkt_length; i++)
						inputFileBuffer[fileBufcount++] = rcvBuf->data[i];
					if (rcvBuf->pktHdr.pkt_sync & 0b01 == 0b01)
						fileBufFlag += 0b01;
					rxFL_cs.Unlock();
					break;
				}

			}
			case 0b01:	//string data + string data ack (piggyback)
			{
				break;
			}
			case 0b11:	//file data + file data ack (piggyback)
			{
				break;
			}
			}

		}
		else
		{
			rxQ_cs.Lock();
			ackReply(rcvBuf->pktHdr.ackFrameNo, false);
			rxQ_cs.Unlock();
		}
	}
}

void CMyProtocol::ptcSetPktReceived(m_pkt * rcvBuf)
{
	CString message;
	if (rcvBuf->pktHdr.pkt_type == 0b00)
	{
		this->aType = STOP_AND_WAIT;
		m_pkt pktTmp;	//initialize packet.
		pktTmp.pktHdr.pkt_type = 0b00;
		pktTmp.pktHdr.pkt_length = 0;
		m_pSocket->SendToEx(&pktTmp, sizeof(m_pkt), *clientPort, *clientAddr, 0);
		message = _T("## Request ARQ type 'Stop and Wait' ##\r\n");
	}
	else if (rcvBuf->pktHdr.pkt_type = 0b11)
	{
		this->aType = SLIDING_WINDOW;
		m_pkt pktTmp;	//initialize packet.
		pktTmp.pktHdr.pkt_type = 0b00;
		pktTmp.pktHdr.pkt_length = 0;
		m_pSocket->SendToEx(&pktTmp, sizeof(m_pkt), *clientPort, *clientAddr, 0);
		message = _T("## Request ARQ type 'Sliding Window' ##\r\n");

	}
	m_pDlg->ProcessReceive(message);
}

void CMyProtocol::pktRecv(CString rasmStr)
{
	m_pDlg->ProcessReceive(rasmStr);
}

bool CMyProtocol::arqTypeChange(unsigned char type)
{
	/*0 means STOP-AND-WAIT, 1 means SLIDING-WINDOW */
	arqType at = (arqType)type;
	if (aType == NOT_DECLARED)
	{
		aType = (arqType)type;
		return true;
	}
	else
		return false;
}

void CMyProtocol::quitConnection()
{
	if (aType == NOT_DECLARED)
	{
		AfxMessageBox(_T("통신 중이 아닙니다. 연결을 해제할 수 없습니다."), MB_ICONERROR);
	}
	else
	{
		if (txQsize == 0)
			if (strBufcount == 0)
				if (fileBufcount == 0)
					if (txStrPktList.IsEmpty())
						if (txFilePktList.IsEmpty())
						{
							m_pkt quitPkt;
							quitPkt.pktHdr.pkt_type = 0b10;
							quitPkt.pktHdr.pkt_length = 0b00;
							m_pSocket->SendToEx(&quitPkt, sizeof(m_pkt), *clientPort, *clientAddr, 0);
							AfxMessageBox(_T("연결 종료를 요청하였습니다. 프로그램을 종료합니다."), MB_ICONERROR);
							AfxGetMainWnd()->PostMessageW(WM_CLOSE);
						}
						else;
					else;
				else;
			else;
		else
			AfxMessageBox(_T("현재 통신 중입니다. 연결을 해제할 수 없습니다."), MB_ICONERROR);
	}
}

/*************************** T H R E A D S *********************************/
//TXThread
UINT TXThread(LPVOID arg)
{
	txThdArg *pArg = (txThdArg*)arg;
	CDataSocket *m_pSocket = pArg->m_pSocket;
	arqType *aType = pArg->aType;
	CString *clientAddr = pArg->clientAddr;
	UINT *clientPort = pArg->clientPort;
	txArqBuf *pTxArqStat = pArg->pTxArqStat;
	m_pkt *txPktQueue = pArg->txPktQueue;
	unsigned short *txQHead = pArg->txQHead;
	unsigned short *txQTail = pArg->txQTail;
	unsigned short *txQsize = pArg->txQsize;

	CWinThread *pSlidWinTimer[8];
	txSlidTimerArg txSlidThdArg[8];
	bool slidThd_run[8] = { false, false, false, false, false, false, false, false };

	for (int i = 0; i < 8; i++)
	{
		txSlidThdArg[i].currentSeqNo = i;
		txSlidThdArg[i].txQTail = txQTail;
		txSlidThdArg[i].txQHead = txQHead;
		txSlidThdArg[i].txQsize = txQsize;
		txSlidThdArg[i].pArqBuf = &pTxArqStat[i];
		txSlidThdArg[i].thd_run = &slidThd_run[i];
	}

	while (pArg->thread_run) {
		if (*aType == STOP_AND_WAIT)
		{
			txQ_cs.Lock();
			if (*txQsize > 0)
			{
				while (true)
				{
					m_pSocket->SendToEx(&txPktQueue[*txQTail], sizeof(m_pkt), *clientPort, *clientAddr, 0);
					txQ_cs.Unlock();

					bool isReceivedAck = false;
					bool thread_run = true;
					txTimerArg timerArg;
					timerArg.pArqBuf = &pTxArqStat[(*txQTail + 1) % 8];
					timerArg.isReceivedACK = &isReceivedAck;
					timerArg.thread_run = &thread_run;
					CWinThread *pTimerThread = AfxBeginThread(Thd_saw_timer, (LPVOID)&timerArg);

					timer_cs.Lock();
					while (thread_run == true)
					{
						timer_cs.Unlock();
						Sleep(50);
					}

					if (isReceivedAck == true)
					{
						txQ_cs.Lock();
						pTxArqStat[*txQTail] = EMPTY;
						*txQTail = (*txQTail + 1) % 8;
						*txQsize = *txQsize - 1;
						break;
					}
					txQ_cs.Unlock();

				}
			}
			txQ_cs.Unlock();
		}
		else if (*aType == SLIDING_WINDOW)
		{
			txQ_cs.Lock();
			if (*txQsize > 0)
			{
				for (int curSeq = *txQTail; curSeq != *txQHead; curSeq = (curSeq + 1) % 8)
				{
					timer_cs.Lock();
					if (*txSlidThdArg[curSeq].thd_run == true)
					{
						continue;
						timer_cs.Unlock();
					}
					timer_cs.Unlock();
					m_pSocket->SendToEx(&txPktQueue[*txQTail], sizeof(m_pkt), *clientPort, *clientAddr, 0);
					pSlidWinTimer[curSeq] = AfxBeginThread(Thd_slidWin_timer, (LPVOID)&txSlidThdArg[curSeq]);
				}
			}
			txQ_cs.Unlock();
		}
		else
		{
			//ACK TYPE NOT DECLARED.
		}
		Sleep(10);

	}
	return 0;
}

//TXEnqueue thread
UINT TXPktProcThread(LPVOID arg)
{
	txPktProcThdArg *pArg = (txPktProcThdArg*)arg;
	txArqBuf *pTxArqStat = pArg->pTxArqStat;
	CList<m_pkt> *pTxStrPktList = pArg->pTxStrPktList;
	CList<m_pkt> *pTxFilePktList = pArg->pTxFilePktList;
	m_pkt *pTxPktQueue = pArg->pTxPktQueue;
	unsigned short *txQHead = pArg->txQHead;
	unsigned short *txQTail = pArg->txQTail;
	unsigned short *txQsize = pArg->txQsize;

	while (pArg->thread_run) {
		txQ_cs.Lock();
		if (*txQsize < 7)
		{
			if (!pTxStrPktList->IsEmpty())	//StrPktList not Empty
			{
				txStrPkt_cs.Lock();
				pTxPktQueue[*txQHead] = pTxStrPktList->RemoveHead();
				pTxPktQueue[*txQHead].pktHdr.dtaFrameNo = *txQHead;
				txStrPkt_cs.Unlock();
				*txQHead = ((*txQHead + 1) % 8);
				*txQsize = *txQsize + 1;
			}
			else {
				if (!pTxFilePktList->IsEmpty())	//StrPktList Empty, FilePktList not Empty
				{
					txFilePkt_cs.Lock();
					pTxPktQueue[*txQHead] = pTxFilePktList->RemoveHead();
					pTxPktQueue[*txQHead].pktHdr.dtaFrameNo = *txQHead;
					txFilePkt_cs.Unlock();
					*txQHead = ((*txQHead + 1) % 8);
					*txQsize = *txQsize + 1;
				}
			}
		}
		txQ_cs.Unlock();
		Sleep(10);
	}
	return 0;
}

//RXThread
UINT RXThread(LPVOID arg)
{
	rxThdArg *pArg = (rxThdArg*)arg;
	CTermProj24ClientDlg *m_pDlg = pArg->m_pDlg;
	WORD *inputStrBuffer = pArg->inputStrBuffer;
	unsigned short *strBufcount = pArg->strBufcount;
	bool *strBufFlag = pArg->strBufFlag;
	WORD *inputFileBuffer = pArg->inputFileBuffer;
	unsigned int *fileBufcount = pArg->fileBufcount;
	unsigned char *fileBufFlag = pArg->fileBufFlag;
	CString *bufFileName = pArg->bufFileName;

	while (pArg->thread_run) {
		rxSL_cs.Lock();
		if (*strBufFlag == true)
		{
			m_pDlg->ProcessReceive((LPCTSTR)(WORD*)inputStrBuffer);
			memset(inputStrBuffer, 0, 256 * sizeof(WORD));
			*strBufcount = 0;
			*strBufFlag = false;
		}
		rxSL_cs.Unlock();

		rxFL_cs.Lock();
		if (*fileBufFlag == 0b01)
		{
			*bufFileName = (LPCTSTR)(WORD*)inputFileBuffer;
			memset(inputFileBuffer, 0, 5242880 * sizeof(WORD));
			m_pDlg->ProcessReceive(_T("##File##") + *bufFileName + _T(" Receving...\r\n"));
			*fileBufcount = 0;
			*fileBufFlag = 0b10;
		}
		else if (*fileBufFlag == 0b11)
		{
			CString filePath = _T("D:\\") + *bufFileName;
			CFile recvFile;

			recvFile.Open(filePath, CFile::modeCreate | CFile::modeWrite);
			recvFile.Write(inputFileBuffer, *fileBufcount - 1);

			m_pDlg->m_pFileDlg->m_list_file.AddString(_T("받은 파일 : D:\\") + *bufFileName);
			m_pDlg->ProcessReceive(_T("##File##") + *bufFileName + _T(" Received\r\n"));
			recvFile.Close();

			memset(inputFileBuffer, 0, 5242880 * sizeof(WORD));
			*bufFileName = _T("");
			*fileBufcount = 0;
			*fileBufFlag = 0b00;
		}
		rxFL_cs.Unlock();
		Sleep(10);
	}

	return 0;
}

//TimerThread
UINT Thd_saw_timer(LPVOID arg)
{
	timerArg *pArg = (timerArg*)arg;
	txArqBuf *pArqBuf = pArg->pArqBuf;
	bool *isReceivedAck = pArg->isReceivedACK;
	bool *thread_run = pArg->thread_run;

	for (int unit = 0; unit < 10; unit++)
	{
		txQ_cs.Lock();
		if (*pArqBuf == RR_RECEIVED)
		{
			txQ_cs.Unlock();
			*isReceivedAck = true;
			break;
		}
		txQ_cs.Unlock();
		Sleep(50);
	}
	timer_cs.Lock();
	*thread_run = false;
	timer_cs.Unlock();
	return 0;
}

UINT Thd_slidWin_timer(LPVOID arg)
{
	txSlidTimerArg *pArg = (txSlidTimerArg*)arg;
	txArqBuf *pArqBuf = pArg->pArqBuf;
	unsigned short currentSeqNo = pArg->currentSeqNo;
	unsigned short *txQTail = pArg->txQTail;
	unsigned short *txQHead = pArg->txQHead;
	unsigned short *txQsize = pArg->txQsize;
	bool *thd_run = pArg->thd_run;

	for (int unit = 0; unit < 10; unit++)
	{
		txQ_cs.Lock();
		if (*pArqBuf == RR_RECEIVED | *pArqBuf == EMPTY)
		{
			txQ_cs.Unlock();
			break;
		}
		txQ_cs.Unlock();
		Sleep(50);
	}

	timer_cs.Lock();
	*thd_run = false;
	timer_cs.Unlock();
	return 0;
}