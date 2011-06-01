#include "StdAfx.h"
#include "I4C3DAliasControl.h"
#include "I4C3DAnalyzeXML.h"
#include "I4C3DMisc.h"


static BOOL CALLBACK EnumChildProcForMouseInput(HWND hWnd, LPARAM lParam);

static const PCTSTR g_szChildWindowTitle	= _T("Alias.glw");
static const PCTSTR TAG_SLEEP				= _T("sleep");

I4C3DAliasControl::I4C3DAliasControl(void)
{
	m_hKeyInputWnd	= NULL;
	//m_reduceCount = 2;
}

I4C3DAliasControl::~I4C3DAliasControl(void)
{
	ModKeyUp();
}

BOOL I4C3DAliasControl::GetTargetChildWnd(void)
{
	m_hKeyInputWnd = m_hTargetParentWnd;
	EnumChildWindows(m_hKeyInputWnd, EnumChildProcForMouseInput, (LPARAM)&m_hTargetChildWnd);
	if (m_hTargetChildWnd == NULL) {
		return FALSE;
	}
	return TRUE;
}

//BOOL I4C3DAliasControl::CheckTargetState(void)
//{
//	RECT rect;
//	if (m_hTargetParentWnd == NULL) {
//		I4C3DMisc::ReportError(_T("ターゲットウィンドウが取得できません。<I4C3DAliasControl::CheckTargetState>"));
//		I4C3DMisc::LogDebugMessage(Log_Error, _T("ターゲットウィンドウが取得できません。<I4C3DAliasControl::CheckTargetState>"));
//
//	} else if (m_hKeyInputWnd == NULL) {
//		I4C3DMisc::LogDebugMessage(Log_Error, _T("キー入力ウィンドウが取得できません。<I4C3DAliasControl::CheckTargetState>"));
//
//	} else if (m_hTargetChildWnd == NULL) {
//		I4C3DMisc::LogDebugMessage(Log_Error, _T("子ウィンドウが取得できません。<I4C3DAliasControl::CheckTargetState>"));
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


void I4C3DAliasControl::TumbleExecute(int deltaX, int deltaY)
{
	//static int counter = 0;
	//if (counter++ % 2 == 0) {
		//m_bUsePostMessageToMouseDrag = FALSE;
		//VMMouseEvent(NULL, 0, TRUE, FALSE);
		////VMMouseButtonUp(m_hTargetChildWnd);
		//if (CheckTargetState()) {
		//	VMMouseMessage mouseMessage = {0};
		//	mouseMessage.bUsePostMessage = m_bUsePostMessageToMouseDrag;
		//	mouseMessage.hTargetWnd		= m_hTargetChildWnd;
		//	mouseMessage.dragButton		= LButtonDrag;
		//	mouseMessage.dragStartPos	= m_currentPos;
		//	m_currentPos.x				+= deltaX;
		//	m_currentPos.y				+= deltaY;

		//	mouseMessage.dragEndPos		= m_currentPos;
		//	//VMMouseDrag(&mouseMessage, 2);
		//	VMMouseEvent(&mouseMessage, 2, FALSE, FALSE);
		//}
	//}
	//static int counter = 0;
	//if (counter++ % 2 == 0) {
		I4C3DControl::TumbleExecute(deltaX, deltaY);
	//}
}

void I4C3DAliasControl::TrackExecute(int deltaX, int deltaY)
{
	//static int counter = 0;
	//if (counter++ % 2 == 0) {
		//m_bUsePostMessageToMouseDrag = TRUE;
		////VMMouseButtonUp(m_hTargetChildWnd, MK_LBUTTON, MOUSEEVENTF_LEFTUP, FALSE);
		//VMMouseEvent(NULL, 0, FALSE, TRUE);
		I4C3DControl::TrackExecute(deltaX, deltaY);
	//}
}

void I4C3DAliasControl::DollyExecute(int deltaX, int deltaY)
{
	//static int counter = 0;
	//if (counter++ % 2 == 0) {
		//m_bUsePostMessageToMouseDrag = TRUE;
		////VMMouseButtonUp(m_hTargetChildWnd, MK_LBUTTON, MOUSEEVENTF_LEFTUP, FALSE);
		//VMMouseEvent(NULL, 0, FALSE, TRUE);
		I4C3DControl::DollyExecute(deltaX, deltaY);
	//}
	
}

void I4C3DAliasControl::HotkeyExecute(I4C3DContext* pContext, PCTSTR szCommand)
{
	I4C3DControl::HotkeyExecute(pContext, m_hTargetParentWnd, szCommand);
}

void I4C3DAliasControl::ModKeyDown(void)
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

void I4C3DAliasControl::ModKeyUp(void)
{
	if (m_bSyskeyDown) {
		I4C3DControl::ModKeyUp(m_hKeyInputWnd);
	}
}


BOOL CALLBACK EnumChildProcForMouseInput(HWND hWnd, LPARAM lParam)
{
	TCHAR szWindowTitle[I4C3D_BUFFER_SIZE];
	GetWindowText(hWnd, szWindowTitle, _countof(szWindowTitle));
	if (!_tcsicmp(g_szChildWindowTitle, szWindowTitle)) {
		*(HWND*)lParam = hWnd;
		return FALSE;
	}
	return TRUE;
}
