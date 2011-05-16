#include "StdAfx.h"
#include "I4C3DAccessor.h"
#include "I4C3DModulesDefs.h"
#include "I4C3DMisc.h"


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
SOCKET I4C3DAccessor::InitializeSocket(LPCSTR szAddress, USHORT uPort, int nTimeoutMilisec, BOOL bSend, int backlog) {
	SOCKET socketHandler;
	SOCKADDR_IN address;
	TCHAR szError[I4C3D_BUFFER_SIZE];
	int nResult = 0;

	socketHandler = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (socketHandler == INVALID_SOCKET) {
		_stprintf_s(szError, _countof(szError), _T("[ERROR] socket() : %d"), WSAGetLastError());
		I4C3DMisc::ReportError(szError);
		I4C3DMisc::LogDebugMessage(Log_Error, szError);
		return socketHandler;
	}

	address.sin_family = AF_INET;
	address.sin_port = htons(uPort);

	// 受信タイムアウト指定
	setsockopt(socketHandler, SOL_SOCKET, SO_RCVTIMEO, (const char*)&nTimeoutMilisec, sizeof(nTimeoutMilisec));

	if (bSend) {
		address.sin_addr.S_un.S_addr = inet_addr(szAddress);

		nResult = connect(socketHandler, (const SOCKADDR*)&address, sizeof(address));
		if (nResult == SOCKET_ERROR) {
			_stprintf_s(szError, _countof(szError), _T("[ERROR] connect() : %d"), WSAGetLastError());
			I4C3DMisc::ReportError(szError);
			I4C3DMisc::LogDebugMessage(Log_Error, szError);
			closesocket(socketHandler);
			return INVALID_SOCKET;
		}
		
	} else {
		address.sin_addr.S_un.S_addr = INADDR_ANY;

		nResult = bind(socketHandler, (const SOCKADDR*)&address, sizeof(address));
		if (nResult == SOCKET_ERROR) {
			_stprintf_s(szError, _countof(szError), _T("[ERROR] bind() : %d"), WSAGetLastError());
			I4C3DMisc::ReportError(szError);
			I4C3DMisc::LogDebugMessage(Log_Error, szError);
			closesocket(socketHandler);
			return INVALID_SOCKET;
		}

		nResult = listen(socketHandler, backlog);	// 最大backlogまで接続要求を受け付ける。それ以外はWSAECONNREFUSEDエラー。
		if (nResult == SOCKET_ERROR) {
			if (nResult == WSAECONNREFUSED) {
				I4C3DMisc::ReportError(_T("[ERROR] listen() : WSAECONNREFUSED"));
				I4C3DMisc::LogDebugMessage(Log_Error, _T("[ERROR] listen() : WSAECONNREFUSED"));
			} else {
				_stprintf_s(szError, _countof(szError), _T("[ERROR] listen() : %d"), WSAGetLastError());
				I4C3DMisc::ReportError(szError);
				I4C3DMisc::LogDebugMessage(Log_Error, szError);
			}
			closesocket(socketHandler);
			return INVALID_SOCKET;
		}
	}

	return socketHandler;
}