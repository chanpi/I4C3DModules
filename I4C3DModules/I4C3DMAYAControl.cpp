#include "StdAfx.h"
#include "I4C3DMAYAControl.h"
#include "I4C3DAnalyzeXML.h"
#include "I4C3DMisc.h"

static BOOL CALLBACK EnumChildProcForKeyInput(HWND hWnd, LPARAM lParam);
static BOOL CALLBACK EnumChildProcForMouseInput(HWND hWnd, LPARAM lParam);

static const PCTSTR g_szChildWindowTitle		= _T("modelPanel4");
static const PCTSTR g_szChildWindowClass		= _T("QWidget");
static const PCTSTR g_szMouseInputWindowTitle	= _T("modelPanel4");
static const PCTSTR g_szMouseInputWindowClass	= _T("QWidgetOwnDC");
static const PCTSTR TAG_SLEEP					= _T("sleep");

I4C3DMAYAControl::I4C3DMAYAControl(void)
{
	m_reduceCount = 1;
}


I4C3DMAYAControl::~I4C3DMAYAControl(void)
{
	ModKeyUp();
}

BOOL I4C3DMAYAControl::GetTargetChildWnd(void)
{
	EnumChildWindows(m_hTargetParentWnd, EnumChildProcForKeyInput, (LPARAM)&m_hKeyInputWnd);
	if (m_hKeyInputWnd == NULL) {
		return FALSE;
	}
	EnumChildWindows(m_hKeyInputWnd, EnumChildProcForMouseInput, (LPARAM)&m_hTargetChildWnd);
	if (m_hTargetChildWnd == NULL) {
		return FALSE;
	}
	return TRUE;
}

void I4C3DMAYAControl::TumbleExecute(int deltaX, int deltaY)
{
	VMMouseMessage mouseMessage = {0};
	if (!CheckTargetState()) {
		return;
	}
	mouseMessage.bUsePostMessage = m_bUsePostMessageToMouseDrag;
	mouseMessage.hTargetWnd		= m_hTargetChildWnd;
	mouseMessage.dragButton		= LButtonDrag;
	mouseMessage.dragStartPos	= m_currentPos;
	m_currentPos.x				+= deltaX;
	m_currentPos.y				+= deltaY;
	mouseMessage.dragEndPos		= m_currentPos;
	if (m_ctrl) {
		mouseMessage.uKeyState = MK_CONTROL;
	}
	if (m_shift) {
		mouseMessage.uKeyState |= MK_SHIFT;
	}
	VMMouseDrag(&mouseMessage, m_reduceCount);
}

void I4C3DMAYAControl::TrackExecute(int deltaX, int deltaY)
{
	VMMouseMessage mouseMessage = {0};
	if (!CheckTargetState()) {
		return;
	}

	mouseMessage.bUsePostMessage = m_bUsePostMessageToMouseDrag;
	mouseMessage.hTargetWnd		= m_hTargetChildWnd;
	mouseMessage.dragButton		= MButtonDrag;
	mouseMessage.dragStartPos	= m_currentPos;
	m_currentPos.x				+= deltaX;
	m_currentPos.y				+= deltaY;

	mouseMessage.dragEndPos		= m_currentPos;
	if (m_ctrl) {
		mouseMessage.uKeyState = MK_CONTROL;
	}
	if (m_shift) {
		mouseMessage.uKeyState |= MK_SHIFT;
	}
	VMMouseDrag(&mouseMessage, m_reduceCount);
}

void I4C3DMAYAControl::DollyExecute(int deltaX, int deltaY)
{
	VMMouseMessage mouseMessage = {0};
	if (!CheckTargetState()) {
		return;
	}

	mouseMessage.bUsePostMessage = m_bUsePostMessageToMouseDrag;
	mouseMessage.hTargetWnd		= m_hTargetChildWnd;
	mouseMessage.dragButton		= RButtonDrag;
	mouseMessage.dragStartPos	= m_currentPos;
	m_currentPos.x				+= deltaX;
	m_currentPos.y				+= deltaY;
	
	mouseMessage.dragEndPos		= m_currentPos;
	if (m_ctrl) {
		mouseMessage.uKeyState = MK_CONTROL;
	}
	if (m_shift) {
		mouseMessage.uKeyState |= MK_SHIFT;
	}
	VMMouseDrag(&mouseMessage, m_reduceCount);
}
void I4C3DMAYAControl::HotkeyExecute(I4C3DContext* pContext, PCTSTR szCommand)
{
	I4C3DControl::HotkeyExecute(pContext, m_hTargetParentWnd, szCommand);
}

void I4C3DMAYAControl::ModKeyDown(void)
{
	if (!m_bSyskeyDown) {
		I4C3DControl::ModKeyDown(m_hKeyInputWnd);
		if (!IsModKeysDown()) {
			TCHAR szError[I4C3D_BUFFER_SIZE];
			_stprintf_s(szError, _countof(szError), _T("修飾キーが押されませんでした[タイムアウト]。") );
			I4C3DMisc::LogDebugMessage(Log_Error, szError);
		}
	}
}

void I4C3DMAYAControl::ModKeyUp(void)
{
	if (m_bSyskeyDown) {
		I4C3DControl::ModKeyUp(m_hKeyInputWnd);
	}
}

BOOL CALLBACK EnumChildProcForKeyInput(HWND hWnd, LPARAM lParam)
{
	TCHAR szWindowTitle[I4C3D_BUFFER_SIZE];
	TCHAR szClassTitle[I4C3D_BUFFER_SIZE];
	GetWindowText(hWnd, szWindowTitle, _countof(szWindowTitle));
	GetClassName(hWnd, szClassTitle, _countof(szClassTitle));

	if (!_tcsicmp(g_szChildWindowTitle, szWindowTitle) && !_tcsicmp(g_szChildWindowClass, szClassTitle)) {
		*(HWND*)lParam = hWnd;
		//return FALSE;	// 同名のウィンドウが２つある。2番目に取れるウィンドウが該当ウィンドウのため、FALSEにせず検索を続ける。
	}
	return TRUE;
}

BOOL CALLBACK EnumChildProcForMouseInput(HWND hWnd, LPARAM lParam)
{
	TCHAR szWindowTitle[I4C3D_BUFFER_SIZE];
	TCHAR szClassTitle[I4C3D_BUFFER_SIZE];
	GetWindowText(hWnd, szWindowTitle, _countof(szWindowTitle));
	GetClassName(hWnd, szClassTitle, _countof(szClassTitle));

	if (/*!_tcsicmp(g_szMouseInputWindowTitle, szWindowTitle) &&*/ !_tcsicmp(g_szMouseInputWindowClass, szClassTitle)) {	// ウィンドウタイトルがカラのため。
		*(HWND*)lParam = hWnd;
		return FALSE;
	}
	return TRUE;
}
