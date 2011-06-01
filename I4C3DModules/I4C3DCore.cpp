#include "StdAfx.h"
#include "I4C3DCore.h"
#include "I4C3DAccessor.h"
#include "I4C3DControl.h"
#include "I4C3DMisc.h"
#include "I4C3DAnalyzeXML.h"
#include "I4C3DModulesDefs.h"
#include "I4C3DLoadMonitor.h"

#include <process.h>

static volatile ULONGLONG g_ullAccess	= 0;
static ULONGLONG g_ullAccessLimit		= 0;

// �q�X���b�h�����i�[����
static HANDLE g_ChildThreadInfo[8];
static volatile int g_ChildThreadIndex	= 0;
static int g_ChildThreadInfoLimit		= 0;

static unsigned __stdcall I4C3DReceiveThreadProc(void* pParam);
static unsigned __stdcall I4C3DAcceptedThreadProc(void* pParam);
static int AnalyzeMessage(LPCSTR lpszMessage, LPSTR lpszCommand, SIZE_T size, PPOINT pDelta, char cTermination);

static void AddChildThread(HANDLE hThread);
static BOOL CheckChildThreadCount();
static void CloseChildThreadHandle(HANDLE hThread);
static void CloseAllChildThreadHandle();

static CRITICAL_SECTION g_Lock;	// �q�X���b�h�̊Ǘ��Ɏg�p�B�C���N�������g�E�f�N�������g���s���ߒ��ɂ����b�N��������K�v�����邽�߁B

static const PCTSTR TAG_BACKLOG			= _T("backlog");
static const PCTSTR TAG_PORT			= _T("port");
static const PCTSTR TAG_ACCESSLIMIT		= _T("accesslimit");
static const PCTSTR TAG_WAITMILLISEC	= _T("waitmillisec");
static const PCTSTR TAG_TERMINATION		= _T("termination");

static const PCTSTR TAG_CORETIME		= _T("cpu_coretime");
static const PCTSTR TAG_THRESHOULD_BUSY	= _T("cpu_threshould_busy");

static int g_backlog = 8;	// �ݒ�t�@�C������ǂݍ��݁A�㏑������B�ǂݍ��݂Ɏ��s�����ۂ�8�B

typedef struct {
	I4C3DContext* pContext;
	HANDLE hChildThread;
	SOCKET clientSocket;
	DWORD dwWaitMillisec;
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
 * I4C3D���W���[�������������A�J�n���܂��B
 * 
 * @param pContext
 * I4C3D���W���[���̃R���e�L�X�g�̃|�C���^�B
 * 
 * @returns
 * �J�n�ł����ꍇ�ɂ�TRUE�A�������Ɏ��s���J�n�ł��Ȃ������ꍇ�ɂ�FALSE��Ԃ��܂��B
 * 
 * I4C3D���W���[�������������A�J�n���܂��B
 * Initialize()�Ɏ��s�����ꍇ�ɂ�UnInitialize()�Ŋm�ۂ����������⃊�\�[�X��������܂��B
 * 
 * @see
 * Initialize() | UnInitialize()
 */
BOOL I4C3DCore::Start(I4C3DContext* pContext) {

	if (pContext->bIsAlive) {
		return TRUE;
	}

	pContext->bIsAlive = TRUE;	// Initialize����pContext->bIsAlive��TRUE�̕K�v������

	pContext->bIsAlive = Initialize(pContext);
	if (!pContext->bIsAlive) {
		UnInitialize(pContext);
	}
	return pContext->bIsAlive;
}

/**
 * @brief
 * ���������s���܂��B
 * 
 * @param pContext
 * I4C3D���W���[���̃R���e�L�X�g�̃|�C���^�B
 * 
 * @returns
 * �������ɐ��������ꍇ�ɂ�TRUE�A���s�����ꍇ�ɂ�FALSE��Ԃ��܂��B
 * 
 * I4C3D���W���[���̏��������s���܂��B
 * �v���Z�b�T�Ď��n�̃X���b�h�̏���������ю�v�����n�̃X���b�h�̏��������s���܂��B
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
 * ��v�����n�̃X���b�h�̏��������s���܂��B
 * 
 * @param pContext
 * I4C3D���W���[���̃R���e�L�X�g�̃|�C���^�B
 * 
 * @returns
 * ���������������X���b�h�����s�ł����ꍇ��TRUE�A���s�����ꍇ�ɂ�FALSE��Ԃ��܂��B
 * 
 * �e��ݒ�̓ǂݍ��݂�A��v�����n�̃X���b�h�̏��������s���܂��B
 * ���s�����ꍇ��Initialize()�Ń��\�[�X����������s���܂��B
 * 
 * @see
 * Initialize()
 */
BOOL I4C3DCore::InitializeMainContext(I4C3DContext* pContext)
{
	TCHAR szError[I4C3D_BUFFER_SIZE];
	USHORT uBridgePort = 0;
	int nTimeoutMilisec = 3000;	// �K�v�H�H

	// �^�[�Q�b�g�E�B���h�E���擾
	//if ((pContext->hTargetParentWnd = GetTarget3DSoftwareWnd()) == NULL) {
	//	return FALSE;
	//}

	// �ݒ�t�@�C�����Bridge Port���擾
	PCTSTR szPort = pContext->pAnalyzer->GetGlobalValue(TAG_PORT);
	if (szPort == NULL) {
		I4C3DMisc::LogDebugMessage(Log_Error, _T("�ݒ�t�@�C���Ƀ|�[�g�ԍ����w�肵�Ă��������B <I4C3DCore::Start>"));
		return FALSE;
	}
	if (_stscanf_s(szPort, _T("%hu"), &uBridgePort, sizeof(uBridgePort)) != 1) {
		_stprintf_s(szError, _countof(szError), _T("�|�[�g�ԍ��̕ϊ��Ɏ��s���Ă��܂��B <I4C3DCore::Start>\n[%s]"), szPort);
		I4C3DMisc::LogDebugMessage(Log_Error, szError);
		return FALSE;
	}

	// �ݒ�t�@�C�����A�N�Z�X���������擾
	PCTSTR szAccessLimit = pContext->pAnalyzer->GetGlobalValue(TAG_ACCESSLIMIT);
	if (szAccessLimit == NULL || 
		_stscanf_s(szAccessLimit, _T("%d"), &g_ullAccessLimit, sizeof(g_ullAccessLimit)) != 1) {
			g_ullAccessLimit = 100;
	}

	// �ݒ�t�@�C�����ڑ��N���C�A���g�����擾
	PCTSTR szBacklog = pContext->pAnalyzer->GetGlobalValue(TAG_BACKLOG);
	if (szBacklog == NULL ||
		_stscanf_s(szBacklog, _T("%d"), &g_backlog, sizeof(g_backlog)) != 1) {
		g_backlog = 8;
	}
	g_ChildThreadInfoLimit = min(_countof(g_ChildThreadInfo), g_backlog);

	// iPhone�҂��󂯃X�^�[�g
	I4C3DAccessor accessor;
	pContext->receiver = accessor.InitializeTCPSocket(NULL, uBridgePort, nTimeoutMilisec, FALSE, g_backlog);
	if (pContext->receiver == INVALID_SOCKET) {
		I4C3DMisc::LogDebugMessage(Log_Error, _T("InitializeSocket <I4C3DCore::Start>"));
		return FALSE;
	}

	// 
	pContext->hStopEvent = WSACreateEvent();
	if (pContext->hStopEvent == WSA_INVALID_EVENT) {
		_stprintf_s(szError, _countof(szError), _T("[ERROR] WSACreateEvent() : %d"), WSAGetLastError());
		I4C3DMisc::ReportError(szError);
		closesocket(pContext->receiver);
		return FALSE;
	}

	pContext->hThread = (HANDLE)_beginthreadex(NULL, 0, &I4C3DReceiveThreadProc, (void*)pContext, CREATE_SUSPENDED, NULL/*&pContext->uThreadID*/);
	if (pContext->hThread == INVALID_HANDLE_VALUE) {
		_stprintf_s(szError, _countof(szError), _T("[ERROR] _beginthreadex() : %d"), GetLastError());
		I4C3DMisc::ReportError(szError);
		WSACloseEvent(pContext->hStopEvent);
		closesocket(pContext->receiver);
		return FALSE;
	}

	ResumeThread(pContext->hThread);

	return TRUE;
}

/**
 * @brief
 * �v���Z�b�T�Ď��n�̃X���b�h�̏��������s���܂��B
 * 
 * @param pContext
 * I4C3D���W���[���̃R���e�L�X�g�̃|�C���^�B
 * 
 * @returns
 * ���������������X���b�h�����s�ł����ꍇ��TRUE�A���s�����ꍇ�ɂ�FALSE��Ԃ��܂��B
 * 
 * �v���Z�b�T�Ď��n�̃X���b�h�̏��������s���܂��B
 * ���s�����ꍇ��Initialize()�Ń��\�[�X����������s���܂��B
 * 
 * @see
 * Initialize()
 */
BOOL I4C3DCore::InitializeProcessorContext(I4C3DContext* pContext)
{
	TCHAR szError[I4C3D_BUFFER_SIZE];

	pContext->processorContext.bIsBusy = FALSE;
	pContext->processorContext.uNapTime = 0;

	// �ݒ�t�@�C�����CPU�Ɋւ���l���擾
	// �Œ�ł�(CPU�̕��ׂɂ���Đ������N���Ă���ꍇ�ł�)�R�}���h���M����Ԋu(msec)
	PCTSTR szCoreTime = pContext->pAnalyzer->GetGlobalValue(TAG_CORETIME);
	if (szCoreTime == NULL ||
		_stscanf_s(szCoreTime, _T("%u"), &pContext->processorContext.uCoreTime, sizeof(pContext->processorContext.uCoreTime)) != 1) {
			pContext->processorContext.uCoreTime = 100;
	}
	// �R�}���h���M�𐧌�����臒l�ƂȂ�CPU�g�p��
	PCTSTR szThreshouldOfBusy = pContext->pAnalyzer->GetGlobalValue(TAG_THRESHOULD_BUSY);
	if (szThreshouldOfBusy == NULL ||
		_stscanf_s(szThreshouldOfBusy, _T("%u"), &pContext->processorContext.uThreshouldOfBusy, sizeof(pContext->processorContext.uThreshouldOfBusy)) != 1) {
			pContext->processorContext.uThreshouldOfBusy = 80;
	}

	// �I���C�x���g
	pContext->processorContext.hProcessorStopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (pContext->processorContext.hProcessorStopEvent == NULL) {
		_stprintf_s(szError, _countof(szError), _T("[ERROR] CreateEvent() : %d"), GetLastError());
		I4C3DMisc::ReportError(szError);
		return FALSE;
	}

	// CPU�g�p���ɂ���ēK�x��Sleep���͂��݂Ȃ���R�}���h���s��������X���b�h
	pContext->processorContext.hProcessorContextThread = (HANDLE)_beginthreadex(NULL, 0, I4C3DProcessorContextThreadProc, pContext, CREATE_SUSPENDED, NULL);
	if (pContext->processorContext.hProcessorContextThread == INVALID_HANDLE_VALUE) {
		_stprintf_s(szError, _countof(szError), _T("[ERROR] _beginthreadex() : %d"), GetLastError());
		I4C3DMisc::ReportError(szError);
		SafeCloseHandle(pContext->processorContext.hProcessorStopEvent);
		return FALSE;
	}
	// CPU�g�p���𑪂�Ȃ���g�p���ɂ����Sleep�̊Ԋu�𒲐�����X���b�h
	pContext->processorContext.hProcessorMonitorThread = (HANDLE)_beginthreadex(NULL, 0, I4C3DProcessorMonitorThreadProc, pContext, CREATE_SUSPENDED, NULL);
	if (pContext->processorContext.hProcessorMonitorThread == INVALID_HANDLE_VALUE) {
		_stprintf_s(szError, _countof(szError), _T("[ERROR] _beginthreadex() : %d"), GetLastError());
		I4C3DMisc::ReportError(szError);
		SafeCloseHandle(pContext->processorContext.hProcessorStopEvent);
		SafeCloseHandle(pContext->processorContext.hProcessorContextThread);
		return FALSE;
	}

	ResumeThread(pContext->processorContext.hProcessorContextThread);
	ResumeThread(pContext->processorContext.hProcessorMonitorThread);

	return TRUE;
}

/**
 * @brief
 * I4C3D���W���[�����~���A�I���������s���܂��B
 * 
 * @param pContext
 * I4C3D���W���[���̃R���e�L�X�g�̃|�C���^�B
 * 
 * I4C3D���W���[�����~���A�I���������s���܂��B
 * �I��������UnInitialize()�ōs���܂��B
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
 * I4C3D���W���[���̏I���������s���܂��B
 * 
 * @param pContext
 * I4C3D���W���[���̃R���e�L�X�g�̃|�C���^�B
 * 
 * I4C3D���W���[���̏I���������s���܂��B
 * �e�C�x���g�I�u�W�F�N�g�̒�~�A����A�e�X���b�h�̏I����҂��A������܂��B
 */
void I4C3DCore::UnInitialize(I4C3DContext* pContext)
{
	// �v���Z�b�T�֘A�I��
	HANDLE hThread[2];
	hThread[0] = pContext->processorContext.hProcessorMonitorThread;
	hThread[1] = pContext->processorContext.hProcessorContextThread;

	SafeStopEventAndCloseHandle(pContext->processorContext.hProcessorStopEvent, hThread, 2);

	// �I���C�x���g�ݒ�/�X���b�h�I��
	hThread[0] = pContext->hThread;
	SafeStopEventAndCloseHandle(pContext->hStopEvent, hThread, 1);

	// �\�P�b�g�N���[�Y
	if (pContext->receiver != INVALID_SOCKET) {
		closesocket(pContext->receiver);
		pContext->receiver = INVALID_SOCKET;
	}
}

BOOL CheckChildThreadCount()
{
	if (g_ChildThreadIndex >= g_ChildThreadInfoLimit) {
		return FALSE;
	}
	return TRUE;
}

void AddChildThread(HANDLE hThread)
{
	EnterCriticalSection(&g_Lock);
	g_ChildThreadInfo[g_ChildThreadIndex++] = hThread;
	LeaveCriticalSection(&g_Lock);
}

void CloseChildThreadHandle(HANDLE hThread)
{
	EnterCriticalSection(&g_Lock);
	for (int i = 0; i < g_ChildThreadIndex; i++) {
		if ( g_ChildThreadInfo[i] == hThread ) {
			CloseHandle(g_ChildThreadInfo[i]);
			// �n���h������A�Ō���̃X���b�h���ƈʒu���������A�󂫏ꏊ�����B
			g_ChildThreadInfo[i] = g_ChildThreadInfo[g_ChildThreadInfoLimit-1];
			g_ChildThreadIndex--;
			LeaveCriticalSection(&g_Lock);
			return;
		}
	}
	LeaveCriticalSection(&g_Lock);
}

// �e�X���b�h�݂̂���A�N�Z�X
void CloseAllChildThreadHandle()
{
	WaitForMultipleObjects(g_ChildThreadIndex, g_ChildThreadInfo, TRUE, INFINITE);
	for (int i = 0; i < g_ChildThreadIndex; i++) {
		CloseHandle(g_ChildThreadInfo[i]);
	}
	g_ChildThreadIndex = 0;
}

//////////////////////////////////////////////////////////////////////////////

// accept������񓯊��ōs��
unsigned __stdcall I4C3DReceiveThreadProc(void* pParam)
{
	I4C3DMisc::LogDebugMessage(Log_Debug, _T("--- in I4C3DReceiveThreadProc ---"));

	TCHAR szError[I4C3D_BUFFER_SIZE];
	I4C3DContext* pContext = (I4C3DContext*)pParam;

	SOCKET newClient = NULL;
	SOCKADDR_IN address;
	int nLen = 0;
	HANDLE hChildThread = NULL;
	UINT uThreadID = 0;
	I4C3DChildContext* pChildContext = NULL;

	DWORD dwResult = 0;
	WSAEVENT hEvent = NULL;
	WSAEVENT hEventArray[2] = {0};
	WSANETWORKEVENTS events = {0};

	hEvent = WSACreateEvent();
	if (hEvent == WSA_INVALID_EVENT) {
		_stprintf_s(szError, _countof(szError), _T("[ERROR] WSACreateEvent() : %d"), WSAGetLastError());
		I4C3DMisc::ReportError(szError);
		return FALSE;
	}

	WSAEventSelect(pContext->receiver, hEvent, FD_ACCEPT | FD_CLOSE);

	hEventArray[0] = hEvent;
	hEventArray[1] = pContext->hStopEvent;

	for (;;) {
		dwResult = WSAWaitForMultipleEvents(2, hEventArray, FALSE, WSA_INFINITE, FALSE);
		if (dwResult == WSA_WAIT_FAILED) {
			_stprintf_s(szError, _countof(szError), _T("[ERROR] WSA_WAIT_FAILED : %d"), WSAGetLastError());
			I4C3DMisc::ReportError(szError);
			break;
		}

		if (dwResult - WSA_WAIT_EVENT_0 == 0) {
			WSAEnumNetworkEvents(pContext->receiver, hEvent, &events);
			if (events.lNetworkEvents & FD_CLOSE) {
				break;
			} else if (events.lNetworkEvents & FD_ACCEPT) {
				// accept
				if ( !CheckChildThreadCount() ) {
					I4C3DMisc::LogDebugMessage(Log_Error, _T("�ڑ��N���C�A���g�����������z���܂����B<I4C3DCore::I4C3DReceiveThreadProc>"));
					WSAResetEvent(hEvent);
					continue;
				}

				nLen = sizeof(address);
				newClient = accept(pContext->receiver, (SOCKADDR*)&address, &nLen);
				if (newClient == INVALID_SOCKET) {
					_stprintf_s(szError, _countof(szError), _T("[ERROR] accept : %d"), WSAGetLastError());
					I4C3DMisc::ReportError(szError);
					break;
				}

				// Create child thread.
				pChildContext = (I4C3DChildContext*)calloc(1, sizeof(I4C3DChildContext));
				if (pChildContext == NULL) {
					_stprintf_s(szError, _countof(szError), _T("[ERROR] Create child thread. calloc : %d"), GetLastError());
					I4C3DMisc::ReportError(szError);
					break;
				}

				// �ݒ�t�@�C������ҋ@���Ԃ��擾
				PCTSTR szWaitMillisec = pContext->pAnalyzer->GetGlobalValue(TAG_WAITMILLISEC);
				if (szWaitMillisec == NULL || 
					_stscanf_s(szWaitMillisec, _T("%d"), &pChildContext->dwWaitMillisec, sizeof(pChildContext->dwWaitMillisec)) != 1) {
						pChildContext->dwWaitMillisec = 2000;
				}

				// �ݒ�t�@�C������I�[�������擾
				PCTSTR szTermination = pContext->pAnalyzer->GetGlobalValue(TAG_TERMINATION);
				if (szTermination != NULL) {
					char cszTermination[5];
					if (_tcslen(szTermination) != 1) {
						I4C3DMisc::LogDebugMessage(Log_Error, _T("�ݒ�t�@�C���̏I�[�����̎w��Ɍ�肪����܂��B1�����Ŏw�肵�Ă��������B'?'�ɉ��w�肳��܂�"));
						szTermination = _T("?");
					}
#if UNICODE || _UNICODE
					WideCharToMultiByte(CP_ACP, 0, szTermination, _tcslen(szTermination), cszTermination, sizeof(cszTermination), NULL, NULL);
#else
					strcpy_s(cszTermination, sizeof(cszTermination), szTermination);
#endif
					I4C3DMisc::RemoveWhiteSpaceA(cszTermination);

					pChildContext->cTermination = cszTermination[0];
					I4C3DMisc::LogDebugMessage(Log_Debug, szTermination);

				} else {
					pChildContext->cTermination = _T('?');

				}

				pChildContext->pContext = pContext;
				pChildContext->clientSocket = newClient;
				hChildThread = (HANDLE)_beginthreadex(NULL, 0, I4C3DAcceptedThreadProc, pChildContext, CREATE_SUSPENDED, &uThreadID);
				if (hChildThread == INVALID_HANDLE_VALUE) {
					_stprintf_s(szError, _countof(szError), _T("[ERROR] Create child thread. : %d"), GetLastError());
					I4C3DMisc::ReportError(szError);
					break;
				} else {
					pChildContext->hChildThread = hChildThread;
					AddChildThread( hChildThread );
					ResumeThread(hChildThread);
				}
			}
			WSAResetEvent(hEvent);

		} else if (dwResult - WSA_WAIT_EVENT_0 == 1) {
			CloseAllChildThreadHandle();
			pContext->pController->Execute("exit", 5);
			break;
		}
	}
	WSACloseEvent(hEvent);

	shutdown(pContext->receiver, SD_BOTH);
	closesocket(pContext->receiver);
	I4C3DMisc::LogDebugMessage(Log_Debug, _T("--- out I4C3DReceiveThreadProc ---"));
	return EXIT_SUCCESS;
}

unsigned __stdcall I4C3DAcceptedThreadProc(void* pParam)
{
	I4C3DMisc::LogDebugMessage(Log_Debug, _T("--- in I4C3DAcceptedThreadProc ---"));

	I4C3DChildContext* pChildContext = (I4C3DChildContext*)pParam;

	TCHAR szError[I4C3D_BUFFER_SIZE];
	char recvBuffer[I4C3D_RECEIVE_LENGTH];
	SIZE_T totalRecvBytes = 0;
	int nBytes = 0;
	BOOL bBreak = FALSE;

	char szCommand[I4C3D_BUFFER_SIZE] = {0};
	POINT delta;

	DWORD dwResult = 0;
	WSAEVENT hEvent = NULL;
	WSAEVENT hEventArray[2];
	WSANETWORKEVENTS events = {0};

	hEvent = WSACreateEvent();
	WSAEventSelect(pChildContext->clientSocket, hEvent, FD_READ | FD_CLOSE);

	hEventArray[0] = hEvent;
	hEventArray[1] = pChildContext->pContext->hStopEvent;

	FillMemory(recvBuffer, sizeof(recvBuffer), 0xFF);
	while (!bBreak) {
		if (events.iErrorCode[FD_ACCEPT_BIT] != 0) {
			I4C3DMisc::LogDebugMessage(Log_Error, TEXT("FD_ACCEPT_BIT"));
			break;
		} else if (events.iErrorCode[FD_CLOSE_BIT] != 0) {
			I4C3DMisc::LogDebugMessage(Log_Error, TEXT("FD_CLOSE_BIT"));
			break;
		} else if (events.iErrorCode[FD_CONNECT_BIT] != 0) {
			I4C3DMisc::LogDebugMessage(Log_Error, TEXT("FD_CONNECT_BIT"));
			break;
		} else if (events.iErrorCode[FD_OOB_BIT] != 0) {
			I4C3DMisc::LogDebugMessage(Log_Error, TEXT("FD_OOB_BIT"));
			break;
		} else if (events.iErrorCode[FD_QOS_BIT] != 0) {
			I4C3DMisc::LogDebugMessage(Log_Error, TEXT("FD_QOS_BIT"));
			break;
		} else if (events.iErrorCode[FD_READ_BIT] != 0) {
			I4C3DMisc::LogDebugMessage(Log_Error, TEXT("FD_READ_BIT"));
			break;
		} else if (events.iErrorCode[FD_WRITE_BIT] != 0) {
			I4C3DMisc::LogDebugMessage(Log_Error, TEXT("FD_WRITE_BIT"));
			break;
		}

		do {
			Sleep(0);
			dwResult = WSAWaitForMultipleEvents(2, hEventArray, FALSE, pChildContext->dwWaitMillisec, FALSE);	// TODO
			if (dwResult - WSA_WAIT_EVENT_0 != 0) {
				//pChildContext->pContext->pController->ModKeyUp();
			}
		} while (dwResult == WSA_WAIT_TIMEOUT);

		DEBUG_PROFILE_MONITOR;

		if (dwResult == WSA_WAIT_FAILED) {
			I4C3DMisc::LogDebugMessage(Log_Error, _T("WSAWaitForMultipleEvents : WSA_WAIT_FAILED <I4C3DCore::I4C3DAcceptedThreadProc>"));
			break;
		}

		if (dwResult - WSA_WAIT_EVENT_0 == 0) {
			if (WSAEnumNetworkEvents(pChildContext->clientSocket, hEvent, &events) != 0) {
				_stprintf_s(szError, _countof(szError), _T("[ERROR] WSAEnumNetworkEvents() : %d"), WSAGetLastError());
				I4C3DMisc::ReportError(szError);
				break;
			}

			if (events.lNetworkEvents & FD_CLOSE) {
				break;

			} else if (events.lNetworkEvents & FD_READ) {
				static volatile ULONGLONG counter = 0;
				InterlockedIncrement(&counter);

				WSAResetEvent(hEvent);

				if (g_ullAccessLimit < g_ullAccess) {
					OutputDebugString(_T("Too mush accesses!!!!!!!\n"));
					continue;
				}
				
				//if (pChildContext->pContext->processorContext.bIsBusy) {
				//	continue;
				//}

				InterlockedIncrement(&g_ullAccess);

				nBytes = recv(pChildContext->clientSocket, recvBuffer + totalRecvBytes, sizeof(recvBuffer) - totalRecvBytes, 0);

				if (nBytes == SOCKET_ERROR) {
					_stprintf_s(szError, _countof(szError), _T("[ERROR] recv : %d"), WSAGetLastError());
					I4C3DMisc::ReportError(szError);
					InterlockedDecrement(&g_ullAccess);
					break;

				} else if (pChildContext->pContext->processorContext.bIsBusy && counter % 2 == 0) {
					InterlockedDecrement(&g_ullAccess);
					OutputDebugString(_T("remove\n"));
					continue;

				} else if (nBytes > 0) {

					PCSTR pTermination = (PCSTR)memchr(recvBuffer+totalRecvBytes, pChildContext->cTermination, nBytes);
					totalRecvBytes += nBytes;

					// �I�[������������Ȃ��ꍇ�A�o�b�t�@���N���A
					if (pTermination == NULL) {
						if (totalRecvBytes >= sizeof(recvBuffer)) {
							FillMemory(recvBuffer, sizeof(recvBuffer), 0xFF);
							totalRecvBytes = 0;
						}
						InterlockedDecrement(&g_ullAccess);
						continue;
					}

					do {

						DEBUG_PROFILE_MONITOR;

						// �d�����
						delta.x = delta.y = 0;
						//int ret = AnalyzeMessage(recvBuffer, szCommand, sizeof(szCommand), &delta, pChildContext->cTermination);
						//if (ret == 3) {
							// Tumble/Track/Dolly
							//EnterCriticalSection(&g_Lock);
							pChildContext->pContext->pController->Execute(recvBuffer, totalRecvBytes);
							//LeaveCriticalSection(&g_Lock);
							Sleep(0);

						//} else {
						//	// Hotkey
						//	DEBUG_PROFILE_MONITOR;
						//	MoveMemory(szCommand, recvBuffer, pTermination-recvBuffer);
						//	szCommand[pTermination-recvBuffer] = '\0';
						//	EnterCriticalSection(&g_Lock);
						//	pChildContext->pContext->pController->Execute(pChildContext->pContext, &delta, szCommand);
						//	LeaveCriticalSection(&g_Lock);
						//	DEBUG_PROFILE_MONITOR;
						//}

						if (pTermination == recvBuffer + totalRecvBytes - 1) {
							FillMemory(recvBuffer, sizeof(recvBuffer), 0xFF);
							totalRecvBytes = 0;

						} else if (pTermination < recvBuffer + totalRecvBytes - 1) {
							DEBUG_PROFILE_MONITOR;
							int nCopySize = _countof(recvBuffer) - (pTermination - recvBuffer + 1);

							totalRecvBytes -= (pTermination - recvBuffer + 1);
							MoveMemory(recvBuffer, pTermination+1, nCopySize);
							FillMemory(recvBuffer + nCopySize, _countof(recvBuffer) - nCopySize, 0xFF);
							DEBUG_PROFILE_MONITOR;
						} else {
							bBreak = TRUE;
							I4C3DMisc::ReportError(_T("[ERROR] ��M���b�Z�[�W�̉�͂Ɏ��s���Ă��܂��B"));
							break;
						}

						DEBUG_PROFILE_MONITOR;

					} while ((pTermination = (LPCSTR)memchr(recvBuffer, pChildContext->cTermination, totalRecvBytes)) != NULL);

					InterlockedDecrement(&g_ullAccess);

					DEBUG_PROFILE_MONITOR;
				}

			}

		} else if (dwResult - WSA_WAIT_EVENT_0 == 1) {
			// pChildContext->pContext->hStopEvent �ɏI���C�x���g���Z�b�g���ꂽ
			break;
		}

	}
	WSACloseEvent(hEvent);
	//{
	//	TCHAR szBuf[64];
	//	_stprintf_s(szBuf, 64, _T("Out of loop : Access %d\n"), g_ullAccess);
	//	OutputDebugString(szBuf);
	//}

	// closesocket
	shutdown(pChildContext->clientSocket, SD_SEND);
	recv(pChildContext->clientSocket, recvBuffer, sizeof(recvBuffer), 0);
	shutdown(pChildContext->clientSocket, SD_BOTH);
	closesocket(pChildContext->clientSocket);

	CloseChildThreadHandle( pChildContext->hChildThread );
	free(pChildContext);

	I4C3DMisc::LogDebugMessage(Log_Debug, _T("--- out I4C3DAcceptedThreadProc ---"));
	
	return EXIT_SUCCESS;
}

int AnalyzeMessage(LPCSTR lpszMessage, LPSTR lpszCommand, SIZE_T size, PPOINT pDelta, char cTermination)
{
	static char szFormat[32] = {0};
	int scanCount = 0;
	double deltaX = 0., deltaY = 0.;
	if (szFormat[0] == '\0') {
		sprintf_s(szFormat, sizeof(szFormat), "%%s %%lf %%lf%c", cTermination);
	}
	
	scanCount = sscanf_s(lpszMessage, szFormat, lpszCommand, size, &deltaX, &deltaY);
	if (3 <= scanCount) {
		pDelta->x = (LONG)deltaX;
		pDelta->y = (LONG)deltaY;
	}
	return scanCount;
}
