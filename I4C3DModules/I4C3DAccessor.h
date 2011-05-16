#pragma once
class I4C3DAccessor
{
public:
	I4C3DAccessor(void);
	~I4C3DAccessor(void);

	SOCKET InitializeSocket(PCSTR szAddress, USHORT uPort, int nTimeoutMilisec, BOOL bSend, int backlog);
	//BOOL Send(LPCSTR lpszCommand);
	//BOOL Recv(LPSTR lpszCommand, SIZE_T uSize);
};

