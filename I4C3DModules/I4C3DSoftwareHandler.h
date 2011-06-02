#pragma once

class map;

class I4C3DSoftwareHandler
{
public:
	I4C3DSoftwareHandler(void);
	I4C3DSoftwareHandler(LPCTSTR szTarget, USHORT uPort);
	~I4C3DSoftwareHandler(void);
	BOOL PrepareSocket(void);

	TCHAR* m_szTargetTitle;
	USHORT m_uPort;
	SOCKET m_socketHandler;
	SOCKADDR_IN m_address;
};

