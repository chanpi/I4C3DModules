#include "StdAfx.h"
#include "I4C3DSHOWCASEControl.h"
#include "I4C3DAnalyzeXML.h"
#include "I4C3DMisc.h"
#include "I4C3DModulesDefs.h"

static BOOL CALLBACK EnumChildProc(HWND hWnd, LPARAM lParam);
static const PCTSTR g_szChildWindowTitle	= _T("AW_VIEWER");
static const PCTSTR TAG_SLEEP				= _T("sleep");

I4C3DSHOWCASEControl::I4C3DSHOWCASEControl(void)
{
	//m_reduceCount = 2;
}

I4C3DSHOWCASEControl::~I4C3DSHOWCASEControl(void)
{
	ModKeyUp();
}

BOOL I4C3DSHOWCASEControl::GetTargetChildWnd(void)
{
	EnumChildWindows(m_hTargetParentWnd, EnumChildProc, (LPARAM)&m_hTargetChildWnd);
	if (m_hTargetChildWnd == NULL) {
		return FALSE;
	}
	return TRUE;
}

void I4C3DSHOWCASEControl::TumbleExecute(int deltaX, int deltaY)
{
	//static int counter = 0;
	//if (counter++ % 2 == 0) {
		I4C3DControl::TumbleExecute(deltaX, deltaY);
	//}
}

void I4C3DSHOWCASEControl::TrackExecute(int deltaX, int deltaY)
{
	//static int counter = 0;
	//if (counter++ % 2 == 0) {
		I4C3DControl::TrackExecute(deltaX, deltaY);
	//}
}

void I4C3DSHOWCASEControl::DollyExecute(int deltaX, int deltaY)
{
	I4C3DControl::DollyExecute(deltaX, deltaY);
}

void I4C3DSHOWCASEControl::HotkeyExecute(I4C3DContext* pContext, PCTSTR szCommand)
{
	I4C3DControl::HotkeyExecute(pContext, m_hTargetParentWnd, szCommand);
}

void I4C3DSHOWCASEControl::ModKeyDown(void)
{
	if (!m_bSyskeyDown) {
		I4C3DControl::ModKeyDown(m_hTargetChildWnd);
		// キー入力チェック！
		if ( !IsModKeysDown() ) {
			TCHAR szError[I4C3D_BUFFER_SIZE];
			_stprintf_s(szError, _countof(szError), _T("修飾キーが押されませんでした[タイムアウト]。") );
			I4C3DMisc::LogDebugMessage(Log_Error, szError);
		}
	}
}

void I4C3DSHOWCASEControl::ModKeyUp(void)
{
	if (m_bSyskeyDown) {
		I4C3DControl::ModKeyUp(m_hTargetChildWnd);
	}
}

BOOL CALLBACK EnumChildProc(HWND hWnd, LPARAM lParam)
{
	TCHAR szClassName[I4C3D_BUFFER_SIZE];
	GetClassName(hWnd, szClassName, _countof(szClassName));

	if (!_tcsicmp(g_szChildWindowTitle, szClassName)) {
		*(HWND*)lParam = hWnd;
		return FALSE;
	}
	return TRUE;
}
