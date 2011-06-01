#include "StdAfx.h"
#include "I4C3DRTTControl.h"
#include "I4C3DAnalyzeXML.h"
#include "I4C3DCursor.h"
#include "I4C3DMisc.h"

#define MOUSE_EVENT_X(x)    ((x) * (65535 / ::GetSystemMetrics(SM_CXSCREEN)))
#define MOUSE_EVENT_Y(y)    ((y) * (65535 / ::GetSystemMetrics(SM_CYSCREEN)))

static BOOL CALLBACK EnumChildProcForKeyInput(HWND hWnd, LPARAM lParam);
static BOOL CALLBACK EnumChildProcForMouseInput(HWND hWnd, LPARAM lParam);

static const PCTSTR g_szChildWindowTitle		= _T("untitled");
static const PCTSTR g_szMouseInputWindowTitle	= _T("pGLWidget");
static const PCTSTR TAG_SLEEP					= _T("sleep");

I4C3DRTTControl::I4C3DRTTControl(void)
{
	m_hKeyInputWnd	= NULL;
	m_reduceCount	= 1;
	m_pCursor		= new I4C3DCursor;
}

I4C3DRTTControl::~I4C3DRTTControl(void)
{
	ModKeyUp();
	delete m_pCursor;
}

BOOL I4C3DRTTControl::Initialize(I4C3DContext* pContext)
{
	BOOL bRet					= I4C3DControl::Initialize(pContext);
	m_hKeyInputWnd				= NULL;
	m_bUsePostMessageToSendKey	= FALSE;
	m_bUsePostMessageToMouseDrag = TRUE;
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

//BOOL I4C3DRTTControl::CheckTargetState(void)
//{
//	RECT rect;
//	if (m_hTargetParentWnd == NULL) {
//		I4C3DMisc::ReportError(_T("ターゲットウィンドウが取得できません。<I4C3DRTTControl::CheckTargetState>"));
//		I4C3DMisc::LogDebugMessage(Log_Error, _T("ターゲットウィンドウが取得できません。<I4C3DRTTControl::CheckTargetState>"));
//
//	} else if (m_hKeyInputWnd == NULL) {
//		I4C3DMisc::LogDebugMessage(Log_Error, _T("キー入力ウィンドウが取得できません。<I4C3DRTTControl::CheckTargetState>"));
//
//	} else if (m_hTargetChildWnd == NULL) {
//		I4C3DMisc::LogDebugMessage(Log_Error, _T("子ウィンドウが取得できません。<I4C3DRTTControl::CheckTargetState>"));
//
//	} else {
//		// ターゲットウィンドウの位置のチェック
//		POINT tmpPOINT;
//		HWND tmphWnd;
//		GetWindowRect(m_hTargetParentWnd, &rect);
//		int oX = (rect.right - rect.left);
//		int oY = (rect.bottom - rect.top);
//		do {
//			oX /= 2;
//			oY /= 2;
//			tmpPOINT.x = rect.left + oX;
//			tmpPOINT.y = rect.top  + oY;
//			tmphWnd = WindowFromPoint(tmpPOINT);
//		} while (tmphWnd != m_hTargetChildWnd && oX > 20 && oY > 20);
//
//		if (!(oX > 20 && oY > 20)) {
//			return FALSE;
//		}
//
//		m_currentPos.x = tmpPOINT.x;
//		m_currentPos.y = tmpPOINT.y;
//
//		return TRUE;
//	}
//
//	return FALSE;
//}

void I4C3DRTTControl::TumbleExecute(int deltaX, int deltaY)
{
	//m_bUsePostMessageToMouseDrag = FALSE;
	//I4C3DControl::TumbleExecute(deltaX, deltaY);
	//static int counter = 0;
	//if (counter++ % 2 == 0) {
		m_bUsePostMessageToMouseDrag = FALSE;
		//VMMouseEvent(NULL, 0, TRUE, FALSE);
		VMMouseClick(&m_mouseMessage, TRUE);
		//VMMouseButtonUp(m_hTargetChildWnd);
		if (CheckTargetState()) {
			VMMouseMessage mouseMessage = {0};
			mouseMessage.bUsePostMessage = m_bUsePostMessageToMouseDrag;
			mouseMessage.hTargetWnd		= m_hTargetChildWnd;
			mouseMessage.dragButton		= LButtonDrag;
			mouseMessage.dragStartPos	= m_currentPos;
			m_currentPos.x				+= deltaX;
			m_currentPos.y				+= deltaY;

			mouseMessage.dragEndPos		= m_currentPos;
			//VMMouseMove(&mouseMessage, 2);
			//VMMouseEvent(&mouseMessage, 2, FALSE, FALSE);
			VMMouseDrag(&mouseMessage, 2);
		}
	//}
}

void I4C3DRTTControl::TrackExecute(int deltaX, int deltaY)
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
	//{
	//	TCHAR szBuf[32];
	//	_stprintf_s(szBuf, 32, _T("TRACK %d %d\n"), deltaX, deltaY);
	//	OutputDebugString(szBuf);
	//}
	////static int counter = 0;
	////if (counter++ % 2 == 0) {
	//	m_bUsePostMessageToMouseDrag = TRUE;
	//	//VMMouseClick(&m_mouseMessage, TRUE);
	//	//VMMouseEvent(NULL, 0, FALSE, TRUE);
	//	//VMMouseButtonUp(m_hTargetChildWnd, MK_LBUTTON, MOUSEEVENTF_LEFTUP, FALSE);
	//	I4C3DControl::TrackExecute(deltaX, deltaY);
	////}
}

void I4C3DRTTControl::DollyExecute(int deltaX, int deltaY)
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
	//{
	//	TCHAR szBuf[32];
	//	_stprintf_s(szBuf, 32, _T("DOLLY %d %d\n"), deltaX, deltaY);
	//	OutputDebugString(szBuf);
	//}
	////static int counter = 0;
	////if (counter++ % 2 == 0) {
	//	m_bUsePostMessageToMouseDrag = TRUE;
	//	//VMMouseClick(&m_mouseMessage, TRUE);
	//	//VMMouseEvent(NULL, 0, FALSE, TRUE);
	//	//VMMouseButtonUp(m_hTargetChildWnd, MK_LBUTTON, MOUSEEVENTF_LEFTUP, FALSE);
	//	I4C3DControl::DollyExecute(deltaX, deltaY);
	////}
}

void I4C3DRTTControl::HotkeyExecute(I4C3DContext* pContext, PCTSTR szCommand)
{
	I4C3DControl::HotkeyExecute(pContext, m_hKeyInputWnd, szCommand);
}

void I4C3DRTTControl::ModKeyDown(void)
{
	if (!m_bSyskeyDown) {
		I4C3DControl::ModKeyDown(m_hKeyInputWnd);
		if  (!m_pCursor->SetTransparentCursor()) {
			I4C3DMisc::LogDebugMessage(Log_Error, _T("透明カーソルへの変更に失敗しています。"));
		}
		// キー入力チェック！
		if (!IsModKeysDown()) {
			TCHAR szError[I4C3D_BUFFER_SIZE];
			_stprintf_s(szError, _countof(szError), _T("修飾キーが押されませんでした[タイムアウト]。") );
			I4C3DMisc::LogDebugMessage(Log_Error, szError);
		}
	}
}

void I4C3DRTTControl::ModKeyUp(void)
{
	if (m_bSyskeyDown) {
		//VMMouseEvent(NULL, 0, TRUE, TRUE);
		//VMMouseButtonUp(m_hTargetChildWnd);		// MBUTTON, RBUTTON
		//VMMouseButtonUp(m_hTargetChildWnd, MK_LBUTTON, MOUSEEVENTF_LEFTUP, FALSE);

		I4C3DControl::ModKeyUp(m_hKeyInputWnd);
		m_pCursor->RestoreCursor();
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
