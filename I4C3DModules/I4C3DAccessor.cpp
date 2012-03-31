#include "StdAfx.h"
#include "I4C3DAccessor.h"
#include "I4C3DModulesDefs.h"
#include "Miscellaneous.h"
#include "ErrorCodeList.h"

I4C3DAccessor::I4C3DAccessor(void)
{
}


I4C3DAccessor::~I4C3DAccessor(void)
{
}

/**
 * @brief
 * 受信タイムアウト指定付のTCPソケットを作成します。
 * 
 * @param szAddress
 * 対象のIPアドレス。
 * 
 * @param uPort
 * 対象のTCPポート。
 * 
 * @param nTimeoutMilisec
 * 受信タイムアウト[msec]。
 * 
 * @param bSend
 * 送信ソケットの場合TRUE、受信ソケットの場合FALSE。
 * 
 * @param backlog
 * listen関数で待ち受けるクライアント数。
 * 
 * @returns
 * 生成した[送信/受信]待機状態のソケット。失敗した場合にはINVALID_SOCKETを返します。
 * 
 * 受信タイムアウト指定付のTCPソケットを作成します。
 */
SOCKET I4C3DAccessor::InitializeTCPSocket(struct sockaddr_in* pAddress, LPCSTR szAddress, BOOL bSend, USHORT uPort)
{
	SOCKET socketHandler;
	TCHAR szError[I4C3D_BUFFER_SIZE];

	socketHandler = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (socketHandler == INVALID_SOCKET) {
		_stprintf_s(szError, _countof(szError), _T("[ERROR] socket() : %d"), WSAGetLastError());
		LogDebugMessage(Log_Error, szError);
		exit(EXIT_SOCKET_ERROR);
	}

	pAddress->sin_family = AF_INET;
	pAddress->sin_port = htons(uPort);
	if (bSend) {
		pAddress->sin_addr.S_un.S_addr = inet_addr(szAddress);
	} else {
		pAddress->sin_addr.S_un.S_addr = INADDR_ANY;
	}
	return socketHandler;
}


BOOL I4C3DAccessor::SetListeningSocket(const SOCKET& socketHandler, const struct sockaddr_in* pAddress, int backlog, const HANDLE& hEventObject, long lNetworkEvents) {
	BOOL bUse = TRUE;
	setsockopt(socketHandler, SOL_SOCKET, SO_REUSEADDR, (const char*)&bUse, sizeof(bUse));
	int nResult = 0;

	TCHAR szError[I4C3D_BUFFER_SIZE] = {0};

	// bind
	nResult = bind(socketHandler, (const SOCKADDR*)pAddress, sizeof(*pAddress));
	if (nResult == SOCKET_ERROR) {
		_stprintf_s(szError, _countof(szError), _T("[ERROR] bind() : %d"), WSAGetLastError());
		LogDebugMessage(Log_Error, szError);
		closesocket(socketHandler);
		exit(EXIT_SOCKET_ERROR);
	}

	// select
	nResult = WSAEventSelect(socketHandler, hEventObject, lNetworkEvents);
	if (nResult == SOCKET_ERROR) {
		_stprintf_s(szError, _countof(szError), _T("[ERROR] WSAEventSelect() : %d"), WSAGetLastError());
		LogDebugMessage(Log_Error, szError);
		closesocket(socketHandler);
		exit(EXIT_SOCKET_ERROR);
	}

	// listen
	nResult = listen(socketHandler, backlog);	// 最大backlogまで接続要求を受け付ける。それ以外はWSAECONNREFUSEDエラー。
	if (nResult == SOCKET_ERROR) {
		if (nResult == WSAECONNREFUSED) {
			LogDebugMessage(Log_Error, _T("[ERROR] listen() : WSAECONNREFUSED"));
		} else {
			_stprintf_s(szError, _countof(szError), _T("[ERROR] listen() : %d"), WSAGetLastError());
			LogDebugMessage(Log_Error, szError);
		}
		closesocket(socketHandler);
		exit(EXIT_SOCKET_ERROR);
	}
	return TRUE;
}

SOCKET I4C3DAccessor::InitializeUDPSocket(struct sockaddr_in* pAddress, LPCSTR szAddress, USHORT uPort)
{
	SOCKET socketHandler = INVALID_SOCKET;
	TCHAR szError[I4C3D_BUFFER_SIZE];

	socketHandler = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (socketHandler == INVALID_SOCKET) {
		_stprintf_s(szError, _countof(szError), _T("[ERROR] socket() : %d"), WSAGetLastError());
		LogDebugMessage(Log_Error, szError);
		exit(EXIT_SOCKET_ERROR);
	}
	pAddress->sin_family = AF_INET;
	pAddress->sin_port = htons(uPort);
	pAddress->sin_addr.S_un.S_addr = inet_addr(szAddress);
	return socketHandler;
}