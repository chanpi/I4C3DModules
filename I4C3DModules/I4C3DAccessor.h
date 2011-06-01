#pragma once
#include "I4C3DModulesDefs.h"

//typedef struct {
//	SOCKET socketHandler;
//	SOCKADDR_IN address;
//} I4C3DUDPSocketPair;

class I4C3DAccessor
{
public:
	I4C3DAccessor(void);
	~I4C3DAccessor(void);

	SOCKET InitializeTCPSocket(PCSTR szAddress, USHORT uPort, int nTimeoutMilisec, BOOL bSend, int backlog);
	SOCKET InitializeUDPSocket(SOCKADDR_IN* pAddress,  LPCSTR szAddress, USHORT uPort);
	//BOOL Send(LPCSTR lpszCommand);
	//BOOL Recv(LPSTR lpszCommand, SIZE_T uSize);
};

