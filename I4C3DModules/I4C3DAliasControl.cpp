#include "StdAfx.h"
#include "I4C3DAliasControl.h"
#include "I4C3DAnalyzeXML.h"
#include "I4C3DMisc.h"

static BOOL CALLBACK EnumChildProc(HWND hWnd, LPARAM lParam);
static const PCTSTR g_szChildWindowTitle	= _T("Alias.glw");
static const PCTSTR TAG_SLEEP				= _T("sleep");

I4C3DAliasControl::I4C3DAliasControl(void)
{
}

I4C3DAliasControl::~I4C3DAliasControl(void)
{
	ModKeyUp();
}

BOOL I4C3DAliasControl::GetTargetChildWnd(void)
{
	EnumChildWindows(m_hTargetParentWnd, EnumChildProc, (LPARAM)&m_hTargetChildWnd);
	if (m_hTargetChildWnd == NULL) {
		return FALSE;
	}
	return TRUE;
}


void I4C3DAliasControl::HotkeyExecute(I4C3DContext* pContext, PCTSTR szCommand)
{
	I4C3DControl::HotkeyExecute(pContext, m_hTargetParentWnd, szCommand);
}

void I4C3DAliasControl::ModKeyDown(void)
{
	if (!m_bSyskeyDown) {
		I4C3DControl::ModKeyDown(m_hTargetParentWnd);
		//Sleep(m_millisecSleepAfterKeyDown);
		if ( !IsModKeysDown() ) {
			TCHAR szError[I4C3D_BUFFER_SIZE];
			_stprintf_s(szError, _countof(szError), _T("修飾キーが押されませんでした[タイムアウト]。") );
			I4C3DMisc::LogDebugMessage(Log_Error, szError);
		}
	}
}

void I4C3DAliasControl::ModKeyUp(void)
{
	if (m_bSyskeyDown) {
		I4C3DControl::ModKeyUp(m_hTargetParentWnd);
	}
}


BOOL CALLBACK EnumChildProc(HWND hWnd, LPARAM lParam)
{
	TCHAR szWindowTitle[I4C3D_BUFFER_SIZE];
	GetWindowText(hWnd, szWindowTitle, _countof(szWindowTitle));
	if (!_tcsicmp(g_szChildWindowTitle, szWindowTitle)) {
		*(HWND*)lParam = hWnd;
		return FALSE;
	}
	return TRUE;
}