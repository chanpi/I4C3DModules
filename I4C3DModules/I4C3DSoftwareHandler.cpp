#include "StdAfx.h"
#include "I4C3DSoftwareHandler.h"
#include "I4C3DAccessor.h"

I4C3DSoftwareHandler::I4C3DSoftwareHandler(void)
{}

I4C3DSoftwareHandler::I4C3DSoftwareHandler(LPCTSTR szTarget, USHORT uPort)
{
	int len = _tcslen(szTarget);
	m_szTargetTitle = new TCHAR[len+1];
	_tcscpy_s(m_szTargetTitle, len+1, szTarget);
	m_uPort = uPort;
}

I4C3DSoftwareHandler::~I4C3DSoftwareHandler(void)
{
	if (m_szTargetTitle != NULL) {
		delete [] m_szTargetTitle;
		m_szTargetTitle = NULL;
	}
}

BOOL I4C3DSoftwareHandler::PrepareSocket(void)
{
	I4C3DAccessor accessor;
	m_socketHandler = accessor.InitializeUDPSocket(&m_address, "127.0.0.1", m_uPort);
	if (m_socketHandler != INVALID_SOCKET) {
		return TRUE;
	} else {
		return FALSE;
	}
}