#include "StdAfx.h"
#include "I4C3DMAYAControl.h"
#include "I4C3DAnalyzeXML.h"
#include "I4C3DMisc.h"

static const PCTSTR TAG_SLEEP = _T("sleep");

I4C3DMAYAControl::I4C3DMAYAControl(void)
{
}


I4C3DMAYAControl::~I4C3DMAYAControl(void)
{
	ModKeyUp();
}


// TODO!!!!
BOOL I4C3DMAYAControl::GetTargetChildWnd(void)
{
	//EnumChildWindows(m_hTargetParentWnd, EnumChildProc, (LPARAM)&m_hTargetChildWnd);
	if (m_hTargetChildWnd == NULL) {
		return FALSE;
	}
	return TRUE;
}

void I4C3DMAYAControl::HotkeyExecute(I4C3DContext* pContext, PCTSTR szCommand)
{
	I4C3DControl::HotkeyExecute(pContext, m_hTargetParentWnd, szCommand);
}

void I4C3DMAYAControl::ModKeyDown(void)
{
	if (!m_bSyskeyDown) {
		I4C3DControl::ModKeyDown(m_hTargetParentWnd);
		Sleep(m_millisecSleepAfterKeyDown);
	}
}

void I4C3DMAYAControl::ModKeyUp(void)
{
	if (m_bSyskeyDown) {
		I4C3DControl::ModKeyUp(m_hTargetParentWnd);
	}
}
