#pragma once
#include "I4C3DModulesDefs.h"

class I4C3DAccessor
{
public:
	I4C3DAccessor(void);
	~I4C3DAccessor(void);

	SOCKET InitializeTCPSocket(struct sockaddr_in * pAddress, LPCSTR szAddress, BOOL bSend, USHORT uPort);
	BOOL SetListeningSocket(const SOCKET& socketHandler, const struct sockaddr_in* pAddress, int backlog, const HANDLE& hEventObject, long lNetworkEvents);
	SOCKET InitializeUDPSocket(struct sockaddr_in* pAddress,  LPCSTR szAddress, USHORT uPort);
};

