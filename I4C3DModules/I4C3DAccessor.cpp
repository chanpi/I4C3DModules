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

	// ��M�^�C���A�E�g�w��
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

		nResult = listen(socketHandler, backlog);	// �ő�backlog�܂Őڑ��v�����󂯕t����B����ȊO��WSAECONNREFUSED�G���[�B
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