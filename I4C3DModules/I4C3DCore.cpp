#include "StdAfx.h"
#include "I4C3DModules.h"
#include "I4C3DCore.h"
#include "I4C3DAccessor.h"
#include "I4C3DControl.h"
#include "I4C3DAnalyzeXML.h"
#include "I4C3DModulesDefs.h"
#include "I4C3DLoadMonitor.h"
#include "I4C3DCommon.h"
#include "Misc.h"
#include "SharedConstants.h"

#include <process.h>

#if UNICODE || _UNICODE
static LPCTSTR g_FILE = __FILEW__;
#else
static LPCTSTR g_FILE = __FILE__;
#endif


// 子スレッド情報を格納する
static HANDLE g_ChildThreadInfo[8+2]	= {0};	// 念のため+2。基本的には8クライアントまで。
static volatile int g_ChildThreadIndex	= 0;
static int g_ChildThreadInfoLimit		= 0;
static int g_sleepCount					= 0;

static unsigned __stdcall I4C3DReceiveThreadProc(void* pParam);
static unsigned __stdcall I4C3DAcceptedThreadProc(void* pParam);

static void AddChildThread(HANDLE hThread);
static BOOL CheckChildThreadCount();
static void RemoveChildThread(HANDLE hThread);
static void RemoveAllChildThread();

static void GetTerminationFromFile(I4C3DContext* pContext, char* cTermination);
static BOOL CheckNetworkEventError(const WSANETWORKEVENTS& events);

static CRITICAL_SECTION g_Lock			= {0};	// 子スレッドの管理に使用。インクリメント・デクリメントを行う過程にもロックをかける必要があるため。

static const PCTSTR TAG_SLEEPCOUNT		= _T("sleepcount");
static const PCTSTR TAG_BACKLOG			= _T("backlog");
static const PCTSTR TAG_PORT			= _T("port");
static const PCTSTR TAG_TERMINATION		= _T("termination");

static const PCTSTR TAG_CORETIME		= _T("cpu_coretime");
static const PCTSTR TAG_THRESHOULD_BUSY	= _T("cpu_threshould_busy");

static int g_backlog = 8;	// 設定ファイルから読み込み、上書きする。読み込みに失敗した際は8。

typedef struct {
	I4C3DContext* pContext;
	HANDLE hChildThread;
	SOCKET clientSocket;
	char cTermination;
} I4C3DChildContext;

inline void SafeCloseHandle(HANDLE handle)
{
	if (handle != NULL) {
		CloseHandle(handle);
		handle = NULL;
	}
}

inline void SafeStopEventAndCloseHandle(HANDLE hEvent, HANDLE* hThreads, int threadCount)
{
	if (hEvent != NULL) {
		SetEvent(hEvent);
		for (int i = 0; i < threadCount; i++) {
			if (hThreads[i] != NULL) {
				WaitForSingleObject(hThreads[i], INFINITE);
				SafeCloseHandle(hThreads[i]);
			}
		}
		SafeCloseHandle(hEvent);
	}
}

I4C3DCore::I4C3DCore(void)
{
	InitializeCriticalSection(&g_Lock);
}

I4C3DCore::~I4C3DCore(void)
{
	DeleteCriticalSection(&g_Lock);
}

/**
 * @brief
 * I4C3Dモジュールを初期化し、開始します。
 * 
 * @param pContext
 * I4C3Dモジュールのコンテキストのポインタ。
 * 
 * @returns
 * 開始できた場合にはTRUE、初期化に失敗し開始できなかった場合にはFALSEを返します。
 * 
 * I4C3Dモジュールを初期化し、開始します。
 * Initialize()に失敗した場合にはUnInitialize()で確保したメモリやリソースを解放します。
 * 
 * @see
 * Initialize() | UnInitialize()
 */
BOOL I4C3DCore::Start(I4C3DContext* pContext) {

	if (pContext->bIsAlive) {
		return TRUE;
	}

	pContext->bIsAlive = TRUE;	// Initialize時にpContext->bIsAliveがTRUEの必要がある

	pContext->bIsAlive = Initialize(pContext);
	if (!pContext->bIsAlive) {
		UnInitialize(pContext);
	}
	return pContext->bIsAlive;
}

/**
 * @brief
 * 初期化を行います。
 * 
 * @param pContext
 * I4C3Dモジュールのコンテキストのポインタ。
 * 
 * @returns
 * 初期化に成功した場合にはTRUE、失敗した場合にはFALSEを返します。
 * 
 * I4C3Dモジュールの初期化を行います。
 * プロセッサ監視系のスレッドの初期化および主要処理系のスレッドの初期化を行います。
 * 
 * @see
 * InitializeProcessorContext() | InitializeMainContext()
 */
BOOL I4C3DCore::Initialize(I4C3DContext* pContext)
{
	pContext->processorContext.bIsBusy = FALSE;
	//if ( !InitializeProcessorContext(pContext) ) {
	//	return FALSE;
	//}
	if ( !InitializeMainContext(pContext) ) {
		return FALSE;
	}
	return TRUE;
}

/**
 * @brief
 * 主要処理系のスレッドの初期化を行います。
 * 
 * @param pContext
 * I4C3Dモジュールのコンテキストのポインタ。
 * 
 * @returns
 * 初期化が成功しスレッドが実行できた場合にTRUE、失敗した場合にはFALSEを返します。
 * 
 * 各種設定の読み込みや、主要処理系のスレッドの初期化を行います。
 * 失敗した場合はInitialize()でリソース解放処理が行われます。
 * 
 * @see
 * Initialize()
 */
BOOL I4C3DCore::InitializeMainContext(I4C3DContext* pContext)
{
	USHORT uBridgePort = 0;

	// 設定ファイルよりSleepカウントを取得
	PCTSTR szSleepCount = pContext->pAnalyzer->GetGlobalValue(TAG_SLEEPCOUNT);
	if (szSleepCount == NULL || _stscanf_s(szSleepCount, _T("%d"), &g_sleepCount) != 1) {
		g_sleepCount = 1;
	}

	// 設定ファイルよりBridge Portを取得
	PCTSTR szPort = pContext->pAnalyzer->GetGlobalValue(TAG_PORT);
	if (szPort == NULL) {
		LoggingMessage(Log_Error, _T(MESSAGE_ERROR_CFG_PORT), GetLastError(), g_FILE, __LINE__);
		I4C3DExit(EXIT_INVALID_FILE_CONFIGURATION);
		return FALSE;
	}
	if (_stscanf_s(szPort, _T("%hu"), &uBridgePort) != 1) {
		LoggingMessage(Log_Error, _T(MESSAGE_ERROR_CFG_PORT), GetLastError(), g_FILE, __LINE__);
		I4C3DExit(EXIT_INVALID_FILE_CONFIGURATION);
		return FALSE;
	}

	// 設定ファイルより接続クライアント数を取得
	PCTSTR szBacklog = pContext->pAnalyzer->GetGlobalValue(TAG_BACKLOG);
	if (szBacklog == NULL || _stscanf_s(szBacklog, _T("%d"), &g_backlog) != 1) {
		g_backlog = 8;
	}
	g_ChildThreadInfoLimit = min(_countof(g_ChildThreadInfo), g_backlog);
	g_backlog *= 3;	// 全端末SYN2回まで失敗できるだけのバックログ

	// iPhone待ち受けソケット生成
	I4C3DAccessor accessor;
	pContext->receiver = accessor.InitializeTCPSocket(&pContext->address, NULL, FALSE, uBridgePort);
	if (pContext->receiver == INVALID_SOCKET) {
		LoggingMessage(Log_Error, _T(MESSAGE_ERROR_SOCKET_INVALID), GetLastError(), g_FILE, __LINE__);
		I4C3DExit(EXIT_SOCKET_ERROR);
		return FALSE;
	}

	// 待ちうけ終了イベント作成
	pContext->hStopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (pContext->hStopEvent == NULL) {
		LoggingMessage(Log_Error, _T(MESSAGE_ERROR_HANDLE_INVALID), GetLastError(), g_FILE, __LINE__);
		closesocket(pContext->receiver);
		I4C3DExit(EXIT_SYSTEM_ERROR);
		return FALSE;
	}

	pContext->hThread = (HANDLE)_beginthreadex(NULL, 0, &I4C3DReceiveThreadProc, (void*)pContext, CREATE_SUSPENDED, NULL/*&pContext->uThreadID*/);
	if (pContext->hThread == INVALID_HANDLE_VALUE) {
		LoggingMessage(Log_Error, _T(MESSAGE_ERROR_HANDLE_INVALID), GetLastError(), g_FILE, __LINE__);
		SafeCloseHandle(pContext->hStopEvent);
		closesocket(pContext->receiver);
		I4C3DExit(EXIT_SYSTEM_ERROR);
		return FALSE;
	}

	ResumeThread(pContext->hThread);

	return TRUE;
}

/**
 * @brief
 * プロセッサ監視系のスレッドの初期化を行います。
 * 
 * @param pContext
 * I4C3Dモジュールのコンテキストのポインタ。
 * 
 * @returns
 * 初期化が成功しスレッドが実行できた場合にTRUE、失敗した場合にはFALSEを返します。
 * 
 * プロセッサ監視系のスレッドの初期化を行います。
 * 失敗した場合はInitialize()でリソース解放処理が行われます。
 * 
 * @see
 * Initialize()
 */
//BOOL I4C3DCore::InitializeProcessorContext(I4C3DContext* pContext)
//{
//	TCHAR szError[I4C3D_BUFFER_SIZE];
//
//	pContext->processorContext.bIsBusy = FALSE;
//	pContext->processorContext.uNapTime = 0;
//
//	// 設定ファイルよりCPUに関する値を取得
//	// 最低でも(CPUの負荷によって制限が起きている場合でも)コマンド送信する間隔(msec)
//	PCTSTR szCoreTime = pContext->pAnalyzer->GetGlobalValue(TAG_CORETIME);
//	if (szCoreTime == NULL ||
//		_stscanf_s(szCoreTime, _T("%u"), &pContext->processorContext.uCoreTime, sizeof(pContext->processorContext.uCoreTime)) != 1) {
//			pContext->processorContext.uCoreTime = 100;
//	}
//	// コマンド送信を制限する閾値となるCPU使用率
//	PCTSTR szThreshouldOfBusy = pContext->pAnalyzer->GetGlobalValue(TAG_THRESHOULD_BUSY);
//	if (szThreshouldOfBusy == NULL ||
//		_stscanf_s(szThreshouldOfBusy, _T("%u"), &pContext->processorContext.uThreshouldOfBusy, sizeof(pContext->processorContext.uThreshouldOfBusy)) != 1) {
//			pContext->processorContext.uThreshouldOfBusy = 80;
//	}
//
//	// 終了イベント
//	pContext->processorContext.hProcessorStopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
//	if (pContext->processorContext.hProcessorStopEvent == NULL) {
//		_stprintf_s(szError, _countof(szError), _T("[ERROR] CreateEvent() : %d"), GetLastError());
//		ReportError(szError);
//		return FALSE;
//	}
//
//	// CPU使用率によって適度にSleepをはさみながらコマンド実行を許可するスレッド
//	pContext->processorContext.hProcessorContextThread = (HANDLE)_beginthreadex(NULL, 0, I4C3DProcessorContextThreadProc, pContext, CREATE_SUSPENDED, NULL);
//	if (pContext->processorContext.hProcessorContextThread == INVALID_HANDLE_VALUE) {
//		_stprintf_s(szError, _countof(szError), _T("[ERROR] _beginthreadex() : %d"), GetLastError());
//		ReportError(szError);
//		SafeCloseHandle(pContext->processorContext.hProcessorStopEvent);
//		return FALSE;
//	}
//	// CPU使用率を測りながら使用率によってSleepの間隔を調整するスレッド
//	pContext->processorContext.hProcessorMonitorThread = (HANDLE)_beginthreadex(NULL, 0, I4C3DProcessorMonitorThreadProc, pContext, CREATE_SUSPENDED, NULL);
//	if (pContext->processorContext.hProcessorMonitorThread == INVALID_HANDLE_VALUE) {
//		_stprintf_s(szError, _countof(szError), _T("[ERROR] _beginthreadex() : %d"), GetLastError());
//		ReportError(szError);
//		SafeCloseHandle(pContext->processorContext.hProcessorStopEvent);
//		SafeCloseHandle(pContext->processorContext.hProcessorContextThread);
//		return FALSE;
//	}
//
//	ResumeThread(pContext->processorContext.hProcessorContextThread);
//	ResumeThread(pContext->processorContext.hProcessorMonitorThread);
//
//	return TRUE;
//}

/**
 * @brief
 * I4C3Dモジュールを停止し、終了処理を行います。
 * 
 * @param pContext
 * I4C3Dモジュールのコンテキストのポインタ。
 * 
 * I4C3Dモジュールを停止し、終了処理を行います。
 * 終了処理はUnInitialize()で行います。
 * 
 * @see
 * UnInitialize()
 */
void I4C3DCore::Stop(I4C3DContext* pContext) {
	if (!pContext->bIsAlive) {
		return;
	}
	pContext->bIsAlive = FALSE;
	UnInitialize(pContext);
}

/**
 * @brief
 * I4C3Dモジュールの終了処理を行います。
 * 
 * @param pContext
 * I4C3Dモジュールのコンテキストのポインタ。
 * 
 * I4C3Dモジュールの終了処理を行います。
 * 各イベントオブジェクトの停止、解放、各スレッドの終了を待ち、解放します。
 */
void I4C3DCore::UnInitialize(I4C3DContext* pContext)
{
	// プロセッサ関連終了
	HANDLE hThread[2] = {0};
	//hThread[0] = pContext->processorContext.hProcessorMonitorThread;
	//hThread[1] = pContext->processorContext.hProcessorContextThread;

	//SafeStopEventAndCloseHandle(pContext->processorContext.hProcessorStopEvent, hThread, 2);

	// 終了イベント設定/スレッド終了
	hThread[0] = pContext->hThread;
	SafeStopEventAndCloseHandle(pContext->hStopEvent, hThread, 1);

	// ソケットクローズ
	if (pContext->receiver != INVALID_SOCKET) {
		closesocket(pContext->receiver);
		pContext->receiver = INVALID_SOCKET;
	}
}


// ipodクライアント数の管理
//////////////////////////////////////////////////////////////////////////////

BOOL CheckChildThreadCount()
{
	EnterCriticalSection(&g_Lock);
	if (g_ChildThreadIndex >= g_ChildThreadInfoLimit) {
		LeaveCriticalSection(&g_Lock);
		return FALSE;
	}
	LeaveCriticalSection(&g_Lock);
	return TRUE;
}

void AddChildThread(HANDLE hThread)
{
	EnterCriticalSection(&g_Lock);
	g_ChildThreadInfo[g_ChildThreadIndex++] = hThread;
	LeaveCriticalSection(&g_Lock);
}

void RemoveChildThread(HANDLE hThread)
{
	EnterCriticalSection(&g_Lock);
	for (int i = 0; i < g_ChildThreadIndex; i++) {
		if ( g_ChildThreadInfo[i] == hThread ) {
			SafeCloseHandle(g_ChildThreadInfo[i]);
			// ハンドルを閉じ、最後尾のスレッド情報と位置を交換し、空き場所を作る。
			g_ChildThreadInfo[i] = g_ChildThreadInfo[g_ChildThreadIndex-1];
			g_ChildThreadInfo[g_ChildThreadIndex-1] = NULL;
			g_ChildThreadIndex--;
			LeaveCriticalSection(&g_Lock);
			return;
		}
	}
	LeaveCriticalSection(&g_Lock);
	LoggingMessage(Log_Error, _T(MESSAGE_ERROR_HANDLE_INVALID), GetLastError(), g_FILE, __LINE__);
}

// 親スレッドのみからアクセス
void RemoveAllChildThread()
{
	WaitForMultipleObjects(g_ChildThreadIndex, g_ChildThreadInfo, TRUE, INFINITE);
	if (g_ChildThreadIndex == 0) {
		//"RemoveAllChildThread OK"
		LoggingMessage(Log_Debug, _T(MESSAGE_DEBUG_HANDLE_VALID), GetLastError(), g_FILE, __LINE__);
	} else {
		// "RemoveAllChildThread NG (%dクライアント残っています) <I4C3DCore::RemoveAllChildThread()>"), g_ChildThreadIndex);
		LoggingMessage(Log_Error, _T(MESSAGE_ERROR_HANDLE_INVALID), GetLastError(), g_FILE, __LINE__);
	}
}

//////////////////////////////////////////////////////////////////////////////

void GetTerminationFromFile(I4C3DContext* pContext, char* cTermination) {
	PCTSTR szTermination = pContext->pAnalyzer->GetGlobalValue(TAG_TERMINATION);
	if (szTermination != NULL) {
		char cszTermination[5] = {0};
		if (_tcslen(szTermination) != 1) {
			LoggingMessage(Log_Error, _T(MESSAGE_ERROR_CFG_TERMCHAR), GetLastError(), g_FILE, __LINE__);
			szTermination = _T("?");
		}
#if UNICODE || _UNICODE
		WideCharToMultiByte(CP_ACP, 0, szTermination, _tcslen(szTermination), cszTermination, sizeof(cszTermination), NULL, NULL);
#else
		strcpy_s(cszTermination, sizeof(cszTermination), szTermination);
#endif
		RemoveWhiteSpaceA(cszTermination);

		*cTermination = cszTermination[0];
		// LogDebugMessage(Log_Debug, szTermination);
		LoggingMessage(Log_Debug, _T(MESSAGE_DEBUG_HANDLE_VALID), GetLastError(), g_FILE, __LINE__);
	} else {
		*cTermination = '?';

	}
}


// accept処理を非同期で行う
unsigned __stdcall I4C3DReceiveThreadProc(void* pParam)
{
	LoggingMessage(Log_Debug, _T(MESSAGE_DEBUG_PROCESSING), GetLastError(), g_FILE, __LINE__);
	
	I4C3DContext* pContext = (I4C3DContext*)pParam;

	SOCKET newClient = NULL;
	SOCKADDR_IN address = {0};
	int nLen = 0;
	HANDLE hChildThread = NULL;
	UINT uThreadID = 0;
	I4C3DChildContext* pChildContext = NULL;
	char cTermination = 0;

	DWORD dwResult = 0;
	WSAEVENT hEvent = NULL;
	WSAEVENT hEventArray[2] = {0};
	WSANETWORKEVENTS events = {0};

	hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	if (hEvent == NULL) {
		LoggingMessage(Log_Error, _T(MESSAGE_ERROR_HANDLE_INVALID), GetLastError(), g_FILE, __LINE__);
		I4C3DExit(EXIT_SYSTEM_ERROR);
		return EXIT_SYSTEM_ERROR;
	}

	I4C3DAccessor accessor;
	if (!accessor.SetListeningSocket(pContext->receiver, &pContext->address, g_backlog, hEvent, FD_ACCEPT | FD_CLOSE)) {
		SafeCloseHandle(hEvent);
		I4C3DExit(EXIT_SOCKET_ERROR);
		return EXIT_SOCKET_ERROR;
	}

	hEventArray[0] = hEvent;
	hEventArray[1] = pContext->hStopEvent;

	for (;;) {
		dwResult = WSAWaitForMultipleEvents(2, hEventArray, FALSE, WSA_INFINITE, FALSE);
		if (dwResult == WSA_WAIT_FAILED) {
			LoggingMessage(Log_Error, _T(MESSAGE_ERROR_HANDLE_INVALID), GetLastError(), g_FILE, __LINE__);
			break;
		}

		if (dwResult - WSA_WAIT_EVENT_0 == 0) {
			WSAEnumNetworkEvents(pContext->receiver, hEvent, &events);
			if (events.lNetworkEvents & FD_CLOSE) {
				break;
			} else if (events.lNetworkEvents & FD_ACCEPT) {
				// accept
				if ( !CheckChildThreadCount() ) {
					LoggingMessage(Log_Error, _T(MESSAGE_ERROR_HANDLE_INVALID), GetLastError(), g_FILE, __LINE__);
					continue;
				}

				nLen = sizeof(address);
				newClient = accept(pContext->receiver, (SOCKADDR*)&address, &nLen);
				if (newClient == INVALID_SOCKET) {
					LoggingMessage(Log_Error, _T(MESSAGE_ERROR_SOCKET_INVALID), GetLastError(), g_FILE, __LINE__);
					break;
				}

				// Create child thread.
				pChildContext = (I4C3DChildContext*)calloc(1, sizeof(I4C3DChildContext));
				if (pChildContext == NULL) {
					LoggingMessage(Log_Error, _T(MESSAGE_ERROR_HANDLE_INVALID), GetLastError(), g_FILE, __LINE__);
					break;
				}

				// 設定ファイルから終端文字を取得
				if (cTermination == 0) {
					GetTerminationFromFile(pContext, &cTermination);
				}
				pChildContext->cTermination = cTermination;

				pChildContext->pContext = pContext;
				pChildContext->clientSocket = newClient;
				hChildThread = (HANDLE)_beginthreadex(NULL, 0, I4C3DAcceptedThreadProc, pChildContext, CREATE_SUSPENDED, &uThreadID);
				if (hChildThread == INVALID_HANDLE_VALUE) {
					LoggingMessage(Log_Error, _T(MESSAGE_ERROR_HANDLE_INVALID), GetLastError(), g_FILE, __LINE__);
					break;
				} else {
					pChildContext->hChildThread = hChildThread;
					AddChildThread( hChildThread );
					ResumeThread(hChildThread);
				}
			}

		} else if (dwResult - WSA_WAIT_EVENT_0 == 1) {
			RemoveAllChildThread();
			pContext->pController->UnInitialize();
			break;
		}
	}
	SafeCloseHandle(hEvent);

	shutdown(pContext->receiver, SD_BOTH);
	closesocket(pContext->receiver);

	LoggingMessage(Log_Debug, _T(MESSAGE_DEBUG_PROCESSING), GetLastError(), g_FILE, __LINE__);
	return EXIT_SUCCESS;
}


unsigned __stdcall I4C3DAcceptedThreadProc(void* pParam)
{
	LoggingMessage(Log_Debug, _T(MESSAGE_DEBUG_PROCESSING), GetLastError(), g_FILE, __LINE__);

	I4C3DChildContext* pChildContext = (I4C3DChildContext*)pParam;

	I4C3DUDPPacket packet = {0};
	const SIZE_T packetBufferSize = sizeof(packet.szCommand);

	SIZE_T totalRecvBytes = 0;
	int nBytes = 0;
	BOOL bBreak = FALSE;

	DWORD dwResult = 0;
	WSAEVENT hEvent = NULL;
	WSAEVENT hEventArray[2] = {0};
	WSANETWORKEVENTS events = {0};

	hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	if (hEvent == NULL) {
		LoggingMessage(Log_Error, _T(MESSAGE_ERROR_HANDLE_INVALID), GetLastError(), g_FILE, __LINE__);

		shutdown(pChildContext->clientSocket, SD_SEND);
		recv(pChildContext->clientSocket, packet.szCommand, packetBufferSize, 0);
		shutdown(pChildContext->clientSocket, SD_BOTH);
		closesocket(pChildContext->clientSocket);

		RemoveChildThread( pChildContext->hChildThread );
		free(pChildContext);
		return EXIT_FAILURE;
	}
	WSAEventSelect(pChildContext->clientSocket, hEvent, FD_READ | FD_CLOSE);

	hEventArray[0] = hEvent;
	hEventArray[1] = pChildContext->pContext->hStopEvent;

	FillMemory(packet.szCommand, packetBufferSize, 0xFF);
	while (!bBreak) {
		if (!CheckNetworkEventError(events)) {
			break;
		}

		dwResult = WSAWaitForMultipleEvents(2, hEventArray, FALSE, WSA_INFINITE, FALSE);

		DEBUG_PROFILE_MONITOR;

		if (dwResult == WSA_WAIT_FAILED) {
			LoggingMessage(Log_Error, _T(MESSAGE_ERROR_HANDLE_INVALID), GetLastError(), g_FILE, __LINE__);
			break;
		}

		if (dwResult - WSA_WAIT_EVENT_0 == 0) {
			if (WSAEnumNetworkEvents(pChildContext->clientSocket, hEvent, &events) != 0) {
				LoggingMessage(Log_Error, _T(MESSAGE_ERROR_HANDLE_INVALID), GetLastError(), g_FILE, __LINE__);
				break;
			}

			if (events.lNetworkEvents & FD_CLOSE) {
				break;

			} else if (events.lNetworkEvents & FD_READ) {
				nBytes = recv(pChildContext->clientSocket, packet.szCommand + totalRecvBytes, packetBufferSize - totalRecvBytes, 0);

				if (nBytes == SOCKET_ERROR) {
					if (WSAGetLastError() == WSAEWOULDBLOCK) {
						continue;
					} else {
						LoggingMessage(Log_Error, _T(MESSAGE_ERROR_SOCKET_RECV), WSAGetLastError(), g_FILE, __LINE__);
					}
					break;

				} else if (nBytes > 0) {

					totalRecvBytes += nBytes;
					PCSTR pTermination = (PCSTR)memchr(packet.szCommand, pChildContext->cTermination, totalRecvBytes);

					// 終端文字が見つからない場合、バッファをクリア
					if (pTermination == NULL) {
						if (totalRecvBytes >= packetBufferSize) {
							FillMemory(packet.szCommand, packetBufferSize, 0xFF);
							totalRecvBytes = 0;
						}
						continue;
					}

					do {

						DEBUG_PROFILE_MONITOR;

						// プラグインへ電文転送
						pChildContext->pContext->pController->Execute(&packet, pTermination-packet.szCommand+1);

						volatile int i;
						for (i = 0; i < g_sleepCount; ++i) {
							Sleep(1);
						}

						//} else {
						//	// Hotkey
						//	MoveMemory(szCommand, recvBuffer, pTermination-recvBuffer);
						//	szCommand[pTermination-recvBuffer] = '\0';
						//	EnterCriticalSection(&g_Lock);
						//	pChildContext->pContext->pController->Execute(pChildContext->pContext, &delta, szCommand);
						//	LeaveCriticalSection(&g_Lock);
						//}

						if (pTermination == (packet.szCommand + totalRecvBytes - 1)) {
							FillMemory(packet.szCommand, packetBufferSize, 0xFF);
							totalRecvBytes = 0;

						} else if (pTermination < (packet.szCommand + totalRecvBytes - 1)) {
							int nCopySize = packetBufferSize - (pTermination - packet.szCommand + 1);

							totalRecvBytes -= (pTermination - packet.szCommand + 1);
							MoveMemory(packet.szCommand, pTermination+1, nCopySize);
							FillMemory(packet.szCommand + nCopySize, packetBufferSize - nCopySize, 0xFF);

						} else {
							bBreak = TRUE;
							LoggingMessage(Log_Error, _T(MESSAGE_ERROR_MESSAGE_INVALID), GetLastError(), g_FILE, __LINE__);
							break;
						}

						DEBUG_PROFILE_MONITOR;

					} while ((pTermination = (LPCSTR)memchr(packet.szCommand, pChildContext->cTermination, totalRecvBytes)) != NULL);

					DEBUG_PROFILE_MONITOR;
				}

			}

		} else if (dwResult - WSA_WAIT_EVENT_0 == 1) {
			// pChildContext->pContext->hStopEvent に終了イベントがセットされた
			break;
		}

	}
	SafeCloseHandle(hEvent);

	// closesocket
	shutdown(pChildContext->clientSocket, SD_SEND);
	recv(pChildContext->clientSocket, packet.szCommand, packetBufferSize, 0);
	shutdown(pChildContext->clientSocket, SD_BOTH);
	closesocket(pChildContext->clientSocket);

	RemoveChildThread( pChildContext->hChildThread );
	free(pChildContext);

	LoggingMessage(Log_Debug, _T(MESSAGE_DEBUG_PROCESSING), GetLastError(), g_FILE, __LINE__);
	
	return EXIT_SUCCESS;
}

BOOL CheckNetworkEventError(const WSANETWORKEVENTS& events) {
	if (events.iErrorCode[FD_ACCEPT_BIT] != 0) {
		LoggingMessage(Log_Error, _T("FD_ACCEPT_BIT"), WSAGetLastError(), g_FILE, __LINE__);
	} else if (events.iErrorCode[FD_CLOSE_BIT] != 0) {
		LoggingMessage(Log_Error, _T("FD_CLOSE_BIT"), WSAGetLastError(), g_FILE, __LINE__);
	} else if (events.iErrorCode[FD_CONNECT_BIT] != 0) {
		LoggingMessage(Log_Error, _T("FD_CONNECT_BIT"), WSAGetLastError(), g_FILE, __LINE__);
	} else if (events.iErrorCode[FD_OOB_BIT] != 0) {
		LoggingMessage(Log_Error, _T("FD_OOB_BIT"), WSAGetLastError(), g_FILE, __LINE__);
	} else if (events.iErrorCode[FD_QOS_BIT] != 0) {
		LoggingMessage(Log_Error, _T("FD_QOS_BIT"), WSAGetLastError(), g_FILE, __LINE__);
	} else if (events.iErrorCode[FD_READ_BIT] != 0) {
		LoggingMessage(Log_Error, _T("FD_READ_BIT"), WSAGetLastError(), g_FILE, __LINE__);
	} else if (events.iErrorCode[FD_WRITE_BIT] != 0) {
		LoggingMessage(Log_Error, _T("FD_WRITE_BIT"), WSAGetLastError(), g_FILE, __LINE__);
	} else {
		return TRUE;
	}
	return FALSE;
}