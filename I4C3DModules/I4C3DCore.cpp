#include "StdAfx.h"
#include "I4C3DCore.h"
#include "I4C3DAccessor.h"
#include "I4C3DControl.h"
#include "I4C3DAnalyzeXML.h"
#include "I4C3DModulesDefs.h"
#include "I4C3DLoadMonitor.h"
#include "I4C3DCommon.h"
#include "Miscellaneous.h"

#include <process.h>

// �q�X���b�h�����i�[����
static HANDLE g_ChildThreadInfo[8+2];	// �O�̂���+2�B��{�I�ɂ�8�N���C�A���g�܂ŁB
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

static CRITICAL_SECTION g_Lock;	// �q�X���b�h�̊Ǘ��Ɏg�p�B�C���N�������g�E�f�N�������g���s���ߒ��ɂ����b�N��������K�v�����邽�߁B

static const PCTSTR TAG_SLEEPCOUNT		= _T("sleepcount");
static const PCTSTR TAG_BACKLOG			= _T("backlog");
static const PCTSTR TAG_PORT			= _T("port");
static const PCTSTR TAG_TERMINATION		= _T("termination");

static const PCTSTR TAG_CORETIME		= _T("cpu_coretime");
static const PCTSTR TAG_THRESHOULD_BUSY	= _T("cpu_threshould_busy");

static int g_backlog = 8;	// �ݒ�t�@�C������ǂݍ��݁A�㏑������B�ǂݍ��݂Ɏ��s�����ۂ�8�B

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

	// �ݒ�t�@�C�����Sleep�J�E���g���擾
	PCTSTR szSleepCount = pContext->pAnalyzer->GetGlobalValue(TAG_SLEEPCOUNT);
	if (szSleepCount == NULL ||
		_stscanf_s(szSleepCount, _T("%d"), &g_sleepCount, sizeof(g_sleepCount)) != 1) {
		g_sleepCount = 1;
	}

	// �ݒ�t�@�C�����Bridge Port���擾
	PCTSTR szPort = pContext->pAnalyzer->GetGlobalValue(TAG_PORT);
	if (szPort == NULL) {
		LogDebugMessage(Log_Error, _T("�ݒ�t�@�C���Ƀ|�[�g�ԍ����w�肵�Ă��������B <I4C3DCore::Start>"));
		return FALSE;
	}
	if (_stscanf_s(szPort, _T("%hu"), &uBridgePort, sizeof(uBridgePort)) != 1) {
		_stprintf_s(szError, _countof(szError), _T("�|�[�g�ԍ��̕ϊ��Ɏ��s���Ă��܂��B <I4C3DCore::Start>\n[%s]"), szPort);
		LogDebugMessage(Log_Error, szError);
		return FALSE;
	}

	// �ݒ�t�@�C�����ڑ��N���C�A���g�����擾
	PCTSTR szBacklog = pContext->pAnalyzer->GetGlobalValue(TAG_BACKLOG);
	if (szBacklog == NULL ||
		_stscanf_s(szBacklog, _T("%d"), &g_backlog, sizeof(g_backlog)) != 1) {
		g_backlog = 8;
	}
	g_ChildThreadInfoLimit = min(_countof(g_ChildThreadInfo), g_backlog);
	g_backlog *= 3;	// �S�[��SYN2��܂Ŏ��s�ł��邾���̃o�b�N���O

	// iPhone�҂��󂯃\�P�b�g����
	I4C3DAccessor accessor;
	pContext->receiver = accessor.InitializeTCPSocket(&pContext->address, NULL, FALSE, uBridgePort);
	if (pContext->receiver == INVALID_SOCKET) {
		LogDebugMessage(Log_Error, _T("InitializeSocket <I4C3DCore::Start>"));
		return FALSE;
	}

	// �҂������I���C�x���g�쐬
	pContext->hStopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (pContext->hStopEvent == NULL) {
		_stprintf_s(szError, _countof(szError), _T("[ERROR] CreateEvent() : %d"), GetLastError());
		ReportError(szError);
		closesocket(pContext->receiver);
		return FALSE;
	}

	pContext->hThread = (HANDLE)_beginthreadex(NULL, 0, &I4C3DReceiveThreadProc, (void*)pContext, CREATE_SUSPENDED, NULL/*&pContext->uThreadID*/);
	if (pContext->hThread == INVALID_HANDLE_VALUE) {
		_stprintf_s(szError, _countof(szError), _T("[ERROR] _beginthreadex() : %d"), GetLastError());
		ReportError(szError);
		SafeCloseHandle(pContext->hStopEvent);
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
//BOOL I4C3DCore::InitializeProcessorContext(I4C3DContext* pContext)
//{
//	TCHAR szError[I4C3D_BUFFER_SIZE];
//
//	pContext->processorContext.bIsBusy = FALSE;
//	pContext->processorContext.uNapTime = 0;
//
//	// �ݒ�t�@�C�����CPU�Ɋւ���l���擾
//	// �Œ�ł�(CPU�̕��ׂɂ���Đ������N���Ă���ꍇ�ł�)�R�}���h���M����Ԋu(msec)
//	PCTSTR szCoreTime = pContext->pAnalyzer->GetGlobalValue(TAG_CORETIME);
//	if (szCoreTime == NULL ||
//		_stscanf_s(szCoreTime, _T("%u"), &pContext->processorContext.uCoreTime, sizeof(pContext->processorContext.uCoreTime)) != 1) {
//			pContext->processorContext.uCoreTime = 100;
//	}
//	// �R�}���h���M�𐧌�����臒l�ƂȂ�CPU�g�p��
//	PCTSTR szThreshouldOfBusy = pContext->pAnalyzer->GetGlobalValue(TAG_THRESHOULD_BUSY);
//	if (szThreshouldOfBusy == NULL ||
//		_stscanf_s(szThreshouldOfBusy, _T("%u"), &pContext->processorContext.uThreshouldOfBusy, sizeof(pContext->processorContext.uThreshouldOfBusy)) != 1) {
//			pContext->processorContext.uThreshouldOfBusy = 80;
//	}
//
//	// �I���C�x���g
//	pContext->processorContext.hProcessorStopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
//	if (pContext->processorContext.hProcessorStopEvent == NULL) {
//		_stprintf_s(szError, _countof(szError), _T("[ERROR] CreateEvent() : %d"), GetLastError());
//		ReportError(szError);
//		return FALSE;
//	}
//
//	// CPU�g�p���ɂ���ēK�x��Sleep���͂��݂Ȃ���R�}���h���s��������X���b�h
//	pContext->processorContext.hProcessorContextThread = (HANDLE)_beginthreadex(NULL, 0, I4C3DProcessorContextThreadProc, pContext, CREATE_SUSPENDED, NULL);
//	if (pContext->processorContext.hProcessorContextThread == INVALID_HANDLE_VALUE) {
//		_stprintf_s(szError, _countof(szError), _T("[ERROR] _beginthreadex() : %d"), GetLastError());
//		ReportError(szError);
//		SafeCloseHandle(pContext->processorContext.hProcessorStopEvent);
//		return FALSE;
//	}
//	// CPU�g�p���𑪂�Ȃ���g�p���ɂ����Sleep�̊Ԋu�𒲐�����X���b�h
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
	//hThread[0] = pContext->processorContext.hProcessorMonitorThread;
	//hThread[1] = pContext->processorContext.hProcessorContextThread;

	//SafeStopEventAndCloseHandle(pContext->processorContext.hProcessorStopEvent, hThread, 2);

	// �I���C�x���g�ݒ�/�X���b�h�I��
	hThread[0] = pContext->hThread;
	SafeStopEventAndCloseHandle(pContext->hStopEvent, hThread, 1);

	// �\�P�b�g�N���[�Y
	if (pContext->receiver != INVALID_SOCKET) {
		closesocket(pContext->receiver);
		pContext->receiver = INVALID_SOCKET;
	}
}


// ipod�N���C�A���g���̊Ǘ�
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
			// �n���h������A�Ō���̃X���b�h���ƈʒu���������A�󂫏ꏊ�����B
			g_ChildThreadInfo[i] = g_ChildThreadInfo[g_ChildThreadIndex-1];
			g_ChildThreadInfo[g_ChildThreadIndex-1] = NULL;
			g_ChildThreadIndex--;
			LeaveCriticalSection(&g_Lock);
			return;
		}
	}
	LogDebugMessage(Log_Error, _T("RemoveChildThread Error"));
	LeaveCriticalSection(&g_Lock);
}

// �e�X���b�h�݂̂���A�N�Z�X
void RemoveAllChildThread()
{
	WaitForMultipleObjects(g_ChildThreadIndex, g_ChildThreadInfo, TRUE, INFINITE);
	if (g_ChildThreadIndex == 0) {
		LogDebugMessage(Log_Debug, _T("RemoveAllChildThread OK"));
	} else {
		TCHAR szError[I4C3D_BUFFER_SIZE];
		_stprintf_s(szError, _countof(szError), _T("RemoveAllChildThread NG (%d�N���C�A���g�c���Ă��܂�) <I4C3DCore::RemoveAllChildThread()>"), g_ChildThreadIndex);
		LogDebugMessage(Log_Error, szError);
	}
}

//////////////////////////////////////////////////////////////////////////////

void GetTerminationFromFile(I4C3DContext* pContext, char* cTermination) {
	PCTSTR szTermination = pContext->pAnalyzer->GetGlobalValue(TAG_TERMINATION);
	if (szTermination != NULL) {
		char cszTermination[5];
		if (_tcslen(szTermination) != 1) {
			LogDebugMessage(Log_Error, _T("�ݒ�t�@�C���̏I�[�����̎w��Ɍ�肪����܂��B1�����Ŏw�肵�Ă��������B'?'�ɉ��w�肳��܂�"));
			szTermination = _T("?");
		}
#if UNICODE || _UNICODE
		WideCharToMultiByte(CP_ACP, 0, szTermination, _tcslen(szTermination), cszTermination, sizeof(cszTermination), NULL, NULL);
#else
		strcpy_s(cszTermination, sizeof(cszTermination), szTermination);
#endif
		RemoveWhiteSpaceA(cszTermination);

		*cTermination = cszTermination[0];
		LogDebugMessage(Log_Debug, szTermination);

	} else {
		*cTermination = '?';

	}
}


// accept������񓯊��ōs��
unsigned __stdcall I4C3DReceiveThreadProc(void* pParam)
{
	LogDebugMessage(Log_Debug, _T("--- in I4C3DReceiveThreadProc ---"));

	TCHAR szError[I4C3D_BUFFER_SIZE];
	I4C3DContext* pContext = (I4C3DContext*)pParam;

	SOCKET newClient = NULL;
	SOCKADDR_IN address;
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
		_stprintf_s(szError, _countof(szError), _T("[ERROR] CreateEvent() : %d <I4C3DCore::I4C3DReceiveThreadProc()>"), GetLastError());
		ReportError(szError);
		return FALSE;
	}

	I4C3DAccessor accessor;
	accessor.SetListeningSocket(pContext->receiver, &pContext->address, g_backlog, hEvent, FD_ACCEPT | FD_CLOSE);

	hEventArray[0] = hEvent;
	hEventArray[1] = pContext->hStopEvent;

	for (;;) {
		dwResult = WSAWaitForMultipleEvents(2, hEventArray, FALSE, WSA_INFINITE, FALSE);
		if (dwResult == WSA_WAIT_FAILED) {
			_stprintf_s(szError, _countof(szError), _T("[ERROR] WSA_WAIT_FAILED : %d <I4C3DCore::I4C3DReceiveThreadProc()>"), WSAGetLastError());
			ReportError(szError);
			break;
		}

		if (dwResult - WSA_WAIT_EVENT_0 == 0) {
			WSAEnumNetworkEvents(pContext->receiver, hEvent, &events);
			if (events.lNetworkEvents & FD_CLOSE) {
				break;
			} else if (events.lNetworkEvents & FD_ACCEPT) {
				// accept
				if ( !CheckChildThreadCount() ) {
					LogDebugMessage(Log_Error, _T("�ڑ��N���C�A���g�����������z���܂����B<I4C3DCore::I4C3DReceiveThreadProc()>"));
					continue;
				}

				nLen = sizeof(address);
				newClient = accept(pContext->receiver, (SOCKADDR*)&address, &nLen);
				if (newClient == INVALID_SOCKET) {
					_stprintf_s(szError, _countof(szError), _T("[ERROR] accept : %d <I4C3DCore::I4C3DReceiveThreadProc()>"), WSAGetLastError());
					ReportError(szError);
					break;
				}

				// Create child thread.
				pChildContext = (I4C3DChildContext*)calloc(1, sizeof(I4C3DChildContext));
				if (pChildContext == NULL) {
					_stprintf_s(szError, _countof(szError), _T("[ERROR] Create child thread. calloc : %d"), GetLastError());
					ReportError(szError);
					break;
				}

				// �ݒ�t�@�C������I�[�������擾
				if (cTermination == 0) {
					GetTerminationFromFile(pContext, &cTermination);
				}
				pChildContext->cTermination = cTermination;

				pChildContext->pContext = pContext;
				pChildContext->clientSocket = newClient;
				hChildThread = (HANDLE)_beginthreadex(NULL, 0, I4C3DAcceptedThreadProc, pChildContext, CREATE_SUSPENDED, &uThreadID);
				if (hChildThread == INVALID_HANDLE_VALUE) {
					_stprintf_s(szError, _countof(szError), _T("[ERROR] Create child thread. : %d"), GetLastError());
					ReportError(szError);
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
	LogDebugMessage(Log_Debug, _T("--- out I4C3DReceiveThreadProc ---"));
	return EXIT_SUCCESS;
}


unsigned __stdcall I4C3DAcceptedThreadProc(void* pParam)
{
	LogDebugMessage(Log_Debug, _T("--- in I4C3DAcceptedThreadProc ---"));

	I4C3DChildContext* pChildContext = (I4C3DChildContext*)pParam;

	TCHAR szError[I4C3D_BUFFER_SIZE] = {0};
	I4C3DUDPPacket packet = {0};
	const SIZE_T packetBufferSize = sizeof(packet.szCommand);

	SIZE_T totalRecvBytes = 0;
	int nBytes = 0;
	BOOL bBreak = FALSE;

	DWORD dwResult = 0;
	WSAEVENT hEvent = NULL;
	WSAEVENT hEventArray[2];
	WSANETWORKEVENTS events = {0};

	hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	if (hEvent == NULL) {
		_stprintf_s(szError, _countof(szError), _T("[ERROR] CreateEvent() : %d"), GetLastError());
		ReportError(szError);

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
			LogDebugMessage(Log_Error, _T("WSAWaitForMultipleEvents : WSA_WAIT_FAILED <I4C3DCore::I4C3DAcceptedThreadProc()>"));
			break;
		}

		if (dwResult - WSA_WAIT_EVENT_0 == 0) {
			if (WSAEnumNetworkEvents(pChildContext->clientSocket, hEvent, &events) != 0) {
				_stprintf_s(szError, _countof(szError), _T("[ERROR] WSAEnumNetworkEvents() : %d <I4C3DCore::I4C3DAcceptedThreadProc>"), WSAGetLastError());
				ReportError(szError);
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
						_stprintf_s(szError, _countof(szError), _T("[ERROR] recv : %d <I4C3DCore::I4C3DAcceptedThreadProc>"), WSAGetLastError());
						ReportError(szError);
					}
					break;

				} else if (nBytes > 0) {

					totalRecvBytes += nBytes;
					PCSTR pTermination = (PCSTR)memchr(packet.szCommand, pChildContext->cTermination, totalRecvBytes);

					// �I�[������������Ȃ��ꍇ�A�o�b�t�@���N���A
					if (pTermination == NULL) {
						if (totalRecvBytes >= packetBufferSize) {
							FillMemory(packet.szCommand, packetBufferSize, 0xFF);
							totalRecvBytes = 0;
						}
						continue;
					}

					do {

						DEBUG_PROFILE_MONITOR;

						// �v���O�C���֓d���]��
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
							ReportError(_T("[ERROR] ��M���b�Z�[�W�̉�͂Ɏ��s���Ă��܂��B"));
							break;
						}

						DEBUG_PROFILE_MONITOR;

					} while ((pTermination = (LPCSTR)memchr(packet.szCommand, pChildContext->cTermination, totalRecvBytes)) != NULL);

					DEBUG_PROFILE_MONITOR;
				}

			}

		} else if (dwResult - WSA_WAIT_EVENT_0 == 1) {
			// pChildContext->pContext->hStopEvent �ɏI���C�x���g���Z�b�g���ꂽ
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

	LogDebugMessage(Log_Debug, _T("--- out I4C3DAcceptedThreadProc ---"));
	
	return EXIT_SUCCESS;
}

BOOL CheckNetworkEventError(const WSANETWORKEVENTS& events) {
	if (events.iErrorCode[FD_ACCEPT_BIT] != 0) {
		LogDebugMessage(Log_Error, TEXT("FD_ACCEPT_BIT"));
	} else if (events.iErrorCode[FD_CLOSE_BIT] != 0) {
		LogDebugMessage(Log_Error, TEXT("FD_CLOSE_BIT"));
	} else if (events.iErrorCode[FD_CONNECT_BIT] != 0) {
		LogDebugMessage(Log_Error, TEXT("FD_CONNECT_BIT"));
	} else if (events.iErrorCode[FD_OOB_BIT] != 0) {
		LogDebugMessage(Log_Error, TEXT("FD_OOB_BIT"));
	} else if (events.iErrorCode[FD_QOS_BIT] != 0) {
		LogDebugMessage(Log_Error, TEXT("FD_QOS_BIT"));
	} else if (events.iErrorCode[FD_READ_BIT] != 0) {
		LogDebugMessage(Log_Error, TEXT("FD_READ_BIT"));
	} else if (events.iErrorCode[FD_WRITE_BIT] != 0) {
		LogDebugMessage(Log_Error, TEXT("FD_WRITE_BIT"));
	} else {
		return TRUE;
	}
	return FALSE;
}