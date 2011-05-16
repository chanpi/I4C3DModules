#include "StdAfx.h"
#include "I4C3DRTTControl.h"
#include "I4C3DAnalyzeXML.h"
#include "I4C3DMisc.h"

static BOOL CALLBACK EnumChildProcForKeyInput(HWND hWnd, LPARAM lParam);
static BOOL CALLBACK EnumChildProcForMouseInput(HWND hWnd, LPARAM lParam);

static const PCTSTR g_szChildWindowTitle		= _T("untitled");
static const PCTSTR g_szMouseInputWindowTitle	= _T("pGLWidget");
static const PCTSTR TAG_SLEEP					= _T("sleep");

I4C3DRTTControl::I4C3DRTTControl(void)
{
	m_hKeyInputWnd		= NULL;
}

I4C3DRTTControl::~I4C3DRTTControl(void)
{
	ModKeyUp();
}

BOOL I4C3DRTTControl::Initialize(I4C3DContext* pContext)
{
	BOOL bRet = I4C3DControl::Initialize(pContext);
	m_hKeyInputWnd		= NULL;
	m_bUsePostMessage	= TRUE;
	return bRet;
}

BOOL I4C3DRTTControl::GetTargetChildWnd(void)
{
	// キー入力ウィンドウを取得
	EnumChildWindows(m_hTargetParentWnd, EnumChildProcForKeyInput, (LPARAM)&m_hKeyInputWnd);
	if (m_hKeyInputWnd == NULL) {
		return FALSE;
	}

	// マウス入力ウィンドウを取得
	EnumChildWindows(m_hKeyInputWnd, EnumChildProcForMouseInput, (LPARAM)&m_hTargetChildWnd);
	if (m_hTargetChildWnd == NULL) {
		return FALSE;
	}
	return TRUE;
}

void I4C3DRTTControl::HotkeyExecute(I4C3DContext* pContext, PCTSTR szCommand)
{
	I4C3DControl::HotkeyExecute(pContext, m_hKeyInputWnd, szCommand);
}

void I4C3DRTTControl::ModKeyDown(void)
{
	if (!m_bSyskeyDown) {
		I4C3DControl::ModKeyDown(m_hKeyInputWnd);
		Sleep(m_millisecSleepAfterKeyDown);
	}
}

void I4C3DRTTControl::ModKeyUp(void)
{
	if (m_bSyskeyDown) {
		I4C3DControl::ModKeyUp(m_hKeyInputWnd);
	}
}

BOOL CALLBACK EnumChildProcForKeyInput(HWND hWnd, LPARAM lParam)
{
	TCHAR szWindowTitle[I4C3D_BUFFER_SIZE];
	GetWindowText(hWnd, szWindowTitle, _countof(szWindowTitle));

	if (!_tcsicmp(g_szChildWindowTitle, szWindowTitle) /*&& !_tcsicmp(_T("QWidget"), szClassTitle)*/) {
		*(HWND*)lParam = hWnd;
		return FALSE;
	}
	return TRUE;
}

BOOL CALLBACK EnumChildProcForMouseInput(HWND hWnd, LPARAM lParam)
{
	// 子ウィンドウの一番初め、一番上位のものを選択
	// (GetWindowTextで"pGLWidget"をとりたいがとれないため)

	//TCHAR szWindowTitle[I4C3D_BUFFER_SIZE];
	//GetWindowText(hWnd, szWindowTitle, _countof(szWindowTitle));
	//{
	//	OutputDebugString(szWindowTitle);
	//	OutputDebugString(L"\n");
	//}

	//if (!_tcsicmp(g_szMouseInputWindowTitle, szWindowTitle)) {
		*(HWND*)lParam = hWnd;
		return FALSE;
	//}
	//return TRUE;
}
