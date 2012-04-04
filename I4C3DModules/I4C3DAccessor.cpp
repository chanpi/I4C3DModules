#include "StdAfx.h"
#include "I4C3DAccessor.h"
#include "I4C3DModulesDefs.h"
#include "Misc.h"
#include "SharedConstants.h"

#if UNICODE || _UNICODE
static LPCTSTR g_FILE = __FILEW__;
#else
static LPCTSTR g_FILE = __FILE__;
#endif

I4C3DAccessor::I4C3DAccessor(void)
{
}


I4C3DAccessor::~I4C3DAccessor(void)
{
}

/**
 * @brief
 * ��M�^�C���A�E�g�w��t��TCP�\�P�b�g���쐬���܂��B
 * 
 * @param szAddress
 * �Ώۂ�IP�A�h���X�B
 * 
 * @param uPort
 * �Ώۂ�TCP�|�[�g�B
 * 
 * @param nTimeoutMilisec
 * ��M�^�C���A�E�g[msec]�B
 * 
 * @param bSend
 * ���M�\�P�b�g�̏ꍇTRUE�A��M�\�P�b�g�̏ꍇFALSE�B
 * 
 * @param backlog
 * listen�֐��ő҂��󂯂�N���C�A���g���B
 * 
 * @returns
 * ��������[���M/��M]�ҋ@��Ԃ̃\�P�b�g�B���s�����ꍇ�ɂ�INVALID_SOCKET��Ԃ��܂��B
 * 
 * ��M�^�C���A�E�g�w��t��TCP�\�P�b�g���쐬���܂��B
 */
SOCKET I4C3DAccessor::InitializeTCPSocket(struct sockaddr_in* pAddress, LPCSTR szAddress, BOOL bSend, USHORT uPort)
{
	SOCKET socketHandler;

	socketHandler = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (socketHandler == INVALID_SOCKET) {
		LoggingMessage(Log_Error, _T(MESSAGE_ERROR_SOCKET_INVALID), WSAGetLastError(), g_FILE, __LINE__);
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
	
	// bind
	nResult = bind(socketHandler, (const SOCKADDR*)pAddress, sizeof(*pAddress));
	if (nResult == SOCKET_ERROR) {
		LoggingMessage(Log_Error, _T(MESSAGE_ERROR_SOCKET_BIND), WSAGetLastError(), g_FILE, __LINE__);
		closesocket(socketHandler);
		exit(EXIT_SOCKET_ERROR);
	}

	// select
	nResult = WSAEventSelect(socketHandler, hEventObject, lNetworkEvents);
	if (nResult == SOCKET_ERROR) {
		LoggingMessage(Log_Error, _T(MESSAGE_ERROR_SOCKET_EVENT), WSAGetLastError(), g_FILE, __LINE__);
		closesocket(socketHandler);
		exit(EXIT_SOCKET_ERROR);
	}

	// listen
	nResult = listen(socketHandler, backlog);	// �ő�backlog�܂Őڑ��v�����󂯕t����B����ȊO��WSAECONNREFUSED�G���[�B
	if (nResult == SOCKET_ERROR) {
		if (nResult == WSAECONNREFUSED) {
			// [ERROR] listen() : WSAECONNREFUSED
			LoggingMessage(Log_Error, _T(MESSAGE_ERROR_SOCKET_LISTEN), WSAGetLastError(), g_FILE, __LINE__);
		} else {
			LoggingMessage(Log_Error, _T(MESSAGE_ERROR_SOCKET_LISTEN), WSAGetLastError(), g_FILE, __LINE__);
		}
		closesocket(socketHandler);
		exit(EXIT_SOCKET_ERROR);
	}
	return TRUE;
}

SOCKET I4C3DAccessor::InitializeUDPSocket(struct sockaddr_in* pAddress, LPCSTR szAddress, USHORT uPort)
{
	SOCKET socketHandler = INVALID_SOCKET;

	socketHandler = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (socketHandler == INVALID_SOCKET) {
		LoggingMessage(Log_Error, _T(MESSAGE_ERROR_SOCKET_INVALID), WSAGetLastError(), g_FILE, __LINE__);
		exit(EXIT_SOCKET_ERROR);
	}
	pAddress->sin_family = AF_INET;
	pAddress->sin_port = htons(uPort);
	pAddress->sin_addr.S_un.S_addr = inet_addr(szAddress);
	return socketHandler;
}