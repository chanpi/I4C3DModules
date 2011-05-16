#include "StdAfx.h"
#include "I4C3DControl.h"
#include "I4C3DMisc.h"
#include "I4C3DAnalyzeXML.h"
#include "I4C3DKeysHook.h"

static void VKPush(HWND hTargetWnd, PCTSTR szKeyTypes, BOOL m_bUsePostMessage);
static const PCTSTR TAG_WINDOWTITLE	= _T("window_title");
static const PCTSTR TAG_MODIFIER	= _T("modifier");
static const PCTSTR TAG_SLEEP		= _T("sleep");
static const PCTSTR TAG_TUMBLERATE	= _T("tumble_rate");
static const PCTSTR TAG_TRACKRATE	= _T("track_rate");
static const PCTSTR TAG_DOLLYRATE	= _T("dolly_rate");

I4C3DControl::I4C3DControl(void)
{
	m_hTargetParentWnd	= NULL;
	m_hTargetChildWnd	= NULL;
	m_currentPos.x		= 0;
	m_currentPos.y		= 0;
	m_DisplayWidth		= GetSystemMetrics(SM_CXSCREEN);
	m_DisplayHeight		= GetSystemMetrics(SM_CYSCREEN);
	m_fTumbleRate		= 0;
	m_fTrackRate		= 0;
	m_fDollyRate		= 0;
	m_millisecSleepAfterKeyDown = 0;
	m_ctrl = m_alt = m_shift = m_bSyskeyDown = FALSE;

	MakeHook( NULL );
}

I4C3DControl::~I4C3DControl(void)
{
	UnHook();
}

/**
 * @brief
 * Controlオブジェクトの初期化を行います。
 * 
 * @param pContext
 * I4C3Dモジュールのコンテキストのポインタ。
 * 
 * @returns
 * 初期化に成功した場合にはTRUE、失敗した場合にはFALSEを返します。
 * 
 * Controlオブジェクトの初期化を行います。
 * どのControlオブジェクトかは、引数のI4C3DContextポインタに登録されているControl派生クラスによります。
 * 
 * @remarks
 * InitializeModifierKeys()で修飾キーの設定を行います。
 * 
 * @see
 * InitializeModifierKeys()
 */
BOOL I4C3DControl::Initialize(I4C3DContext* pContext)
{
	m_hTargetParentWnd = pContext->hTargetParentWnd;
	if (m_hTargetParentWnd == NULL) {
		I4C3DMisc::ReportError(_T("[ERROR] ターゲットウィンドウを取得できません。"));
		return FALSE;
	}
	m_hTargetChildWnd	= NULL;
	m_currentPos.x		= 0;
	m_currentPos.y		= 0;
	m_millisecSleepAfterKeyDown = 0;
	m_bUsePostMessage	= FALSE;

	// 設定ファイルからキー送信後のスリープミリ秒を取得
	PCTSTR szSleep = pContext->pAnalyzer->GetSoftValue(TAG_SLEEP);
	if (szSleep == NULL || 
		_stscanf_s(szSleep, _T("%d"), &m_millisecSleepAfterKeyDown, sizeof(m_millisecSleepAfterKeyDown)) != 1) {
		m_millisecSleepAfterKeyDown = 0;
	}

	// 設定ファイルから各コマンドの移動量パラメータの比率を取得
	PCTSTR szRate = pContext->pAnalyzer->GetSoftValue(TAG_TUMBLERATE);
	if (szRate == NULL ||
		_stscanf_s(szRate, _T("%lf"), &m_fTumbleRate, sizeof(m_fTumbleRate)) != 1) {
			m_fTumbleRate = 1.0;
	}
	szRate = pContext->pAnalyzer->GetSoftValue(TAG_TRACKRATE);
	if (szRate == NULL ||
		_stscanf_s(szRate, _T("%lf"), &m_fTrackRate, sizeof(m_fTrackRate)) != 1) {
			m_fTrackRate = 1.0;
	}
	szRate = pContext->pAnalyzer->GetSoftValue(TAG_DOLLYRATE);
	if (szRate == NULL ||
		_stscanf_s(szRate, _T("%lf"), &m_fDollyRate, sizeof(m_fDollyRate)) != 1) {
			m_fDollyRate = 1.0;
	}

	return I4C3DControl::InitializeModifierKeys(pContext);
}

/**
 * @brief
 * 3Dソフトで使用する修飾キーの取得、設定、およびキー押下の監視プログラムへの登録を行います。
 * 
 * @param pContext
 * I4C3Dモジュールのコンテキストのポインタ。
 * 
 * @returns
 * 修飾キーの取得、設定、登録に成功した場合にはTRUE、失敗した場合にはFALSEを返します。
 * 
 * 3Dソフトで使用する修飾キーの取得、設定、およびキー押下の監視プログラムへの登録を行います。
 * 
 * @remarks
 * I4C3DKeysHook.dllのAddHookedKeyCode()でキーフックの登録を行います。
 * 
 * @see
 * AddHookedKeyCode()
 */
BOOL I4C3DControl::InitializeModifierKeys(I4C3DContext* pContext)
{
	m_ctrl = m_alt = m_shift = m_bSyskeyDown = FALSE;

	PCTSTR szModifierKey = pContext->pAnalyzer->GetSoftValue(TAG_MODIFIER);
	if (szModifierKey == NULL) {
		I4C3DMisc::LogDebugMessage(Log_Error, _T("設定ファイルに修飾キーを設定してください。<I4C3DControl::InitializeModifierKeys>"));
		return FALSE;
	}

	PCTSTR pType = NULL;
	do {
		TCHAR szKey[I4C3D_BUFFER_SIZE] = {0};
		pType = _tcschr(szModifierKey, _T('+'));
		if (pType != NULL) {
			_tcsncpy_s(szKey, _countof(szKey), szModifierKey, pType-szModifierKey+1);
			szModifierKey = pType+1;
		} else {
			_tcscpy_s(szKey, _countof(szKey), szModifierKey);
		}
		I4C3DMisc::RemoveWhiteSpace(szKey);
		switch (szKey[0]) {
		case _T('C'):
		case _T('c'):
			m_ctrl = TRUE;
			AddHookedKeyCode( VK_CONTROL );
			break;

		case _T('S'):
		case _T('s'):
			AddHookedKeyCode( VK_SHIFT );
			m_shift = TRUE;
			break;

		case _T('A'):
		case _T('a'):
			AddHookedKeyCode( VK_MENU );
			m_alt = TRUE;
			break;
		}
	} while (pType != NULL);

	return TRUE;
}

/**
 * @brief
 * 登録した修飾キーが押されたか確認します。
 * 
 * @returns
 * 登録した修飾キーが押されている場合にはTRUE、そうでない場合はFALSEを返します。
 * 
 * 登録した修飾キーが押されたか確認します。
 * 押されていない場合は、Sleepします。
 * Sleepは最大retryCount回行い、Sleep間隔は
 * 回を重ねるごとに2倍していきます。
 * （最大 [1 << retryCount] msecのSleep）
 * 
 * @remarks
 * I4C3DKeysHook.dllのIsAllKeysDown()関数でキー押下を確認します。
 * 
 * @see
 * IsAllKeysDown()
 */
BOOL I4C3DControl::IsModKeysDown(void)
{
	const int retryCount = 12;
	int sleepInterval = 1;

	int i = 0;
	for ( ; i < retryCount; ++i ) {
		Sleep( sleepInterval );
		{
			TCHAR szBuf[32];
			_stprintf_s(szBuf, 32, _T("%4d msec Sleep\n"), sleepInterval );
			OutputDebugString(szBuf);
		}

		if ( !IsAllKeysDown() ) {
			sleepInterval *= 2;
		} else {
			break;
		}

		//if ( m_ctrl ) {
		//	isAllModKeysDown = IsKeyDown( VK_CONTROL );
		//	if ( !isAllModKeysDown ) {
		//		sleepInterval *= 2;
		//		continue;
		//	}
		//}

		//if ( m_shift ) {
		//	isAllModKeysDown = IsKeyDown( VK_SHIFT );
		//	if ( !isAllModKeysDown ) {
		//		sleepInterval *= 2;
		//		continue;
		//	}
		//}

		//if ( m_alt ) {
		//	isAllModKeysDown = IsKeyDown( VK_MENU );
		//	if ( !isAllModKeysDown ) {
		//		sleepInterval *= 2;
		//		continue;
		//	}
		//}
	}

	if ( i < retryCount ) {
		return TRUE;
	} else {
		return FALSE;
	}
}

/**
 * @brief
 * 3DソフトのTUMBLE/TRACK/DOLLYコマンド操作およびショートカットキーの操作を行います。
 * 
 * @param pContext
 * I4C3Dモジュールのコンテキストのポインタ。
 * 
 * @param pDelta
 * マウスの移動量。
 * 
 * @param szCommand
 * コマンド名およびショートカットメニュー名。
 * 
 * 3DソフトのTUMBLE/TRACK/DOLLYコマンド操作およびショートカットキーの操作を行います。
 */
void I4C3DControl::Execute(I4C3DContext* pContext, PPOINT pDelta, LPCSTR szCommand)
{
	// 実際に仮想キー・仮想マウス操作を行う子ウィンドウの取得
	if (!pContext->pController->GetTargetChildWnd()) {
		//TCHAR szError[I4C3D_BUFFER_SIZE];
		//_stprintf_s(szError, _countof(szError), _T("[ERROR] ターゲットソフト[%s]の子ウィンドウを取得できません。window_title[%s]が正しいか確認してください。"),
		//	pContext->pAnalyzer->GetGlobalValue(TAG_TARGET), pContext->pAnalyzer->GetSoftValue(TAG_WINDOWTITLE));
		//I4C3DMisc::ReportError(szError);
		return;
	}

	if (_strcmpi(szCommand, COMMAND_TUMBLE) == 0) {
		pContext->pController->ModKeyDown();
		pContext->pController->TumbleExecute((INT)(pDelta->x * m_fTumbleRate), (INT)(pDelta->y * m_fTumbleRate));

	} else if (_strcmpi(szCommand, COMMAND_TRACK) == 0) {
		pContext->pController->ModKeyDown();
		pContext->pController->TrackExecute((INT)(pDelta->x * m_fTrackRate), (INT)(pDelta->y * m_fTrackRate));

	} else if (_strcmpi(szCommand, COMMAND_DOLLY) == 0) {
		pContext->pController->ModKeyDown();
		pContext->pController->DollyExecute((INT)(pDelta->x * m_fDollyRate), (INT)(pDelta->y * m_fDollyRate));

	} else {
#if _UNICODE || UNICODE
		TCHAR wszCommand[I4C3D_BUFFER_SIZE] = {0};
		MultiByteToWideChar(CP_ACP, 0, szCommand, -1, wszCommand, _countof(wszCommand));
		pContext->pController->ModKeyUp();
		pContext->pController->HotkeyExecute(pContext, wszCommand);
#else
		pContext->pController->HotkeyExecute(lpszCommand);
#endif
	}
}

/**
 * @brief
 * TUMBLEコマンドを実行します。
 * 
 * @param deltaX
 * マウスの移動量[X軸]。
 * 
 * @param deltaY
 * マウスの移動量[Y軸]。
 * 
 * TUMBLEコマンドを実行します。修飾キーの押下とマウス左ボタンドラッグの組み合わせです。
 * 
 * @remarks
 * VirtualMotion.dllのVMMouseDrag()関数を使用します。
 * 
 * @see
 * VMMouseDrag()
 */
void I4C3DControl::TumbleExecute(int deltaX, int deltaY)
{
	VMMouseMessage mouseMessage = {0};
	if (!CheckTargetState()) {
		return;
	}

	mouseMessage.hTargetWnd		= m_hTargetChildWnd;
	mouseMessage.dragButton		= LButtonDrag;
	mouseMessage.dragStartPos	= m_currentPos;
	m_currentPos.x				+= deltaX;
	m_currentPos.y				+= deltaY;
	//AdjustCursorPOS(&m_currentPos);

	mouseMessage.dragEndPos		= m_currentPos;
	if (m_ctrl) {
		mouseMessage.uKeyState = MK_CONTROL;
	}
	if (m_shift) {
		mouseMessage.uKeyState |= MK_SHIFT;
	}
	VMMouseDrag(&mouseMessage);
}

/**
 * @brief
 * TRACKコマンドを実行します。
 * 
 * @param deltaX
 * マウスの移動量[X軸]。
 * 
 * @param deltaY
 * マウスの移動量[Y軸]。
 * 
 * TRACKコマンドを実行します。修飾キーの押下とマウス中ボタンドラッグの組み合わせです。
 * 
 * @remarks
 * VirtualMotion.dllのVMMouseDrag()関数を使用します。
 * 
 * @see
 * VMMouseDrag()
 */
void I4C3DControl::TrackExecute(int deltaX, int deltaY)
{
	VMMouseMessage mouseMessage = {0};
	if (!CheckTargetState()) {
		return;
	}

	mouseMessage.hTargetWnd		= m_hTargetChildWnd;
	mouseMessage.dragButton		= MButtonDrag;
	mouseMessage.dragStartPos	= m_currentPos;
	m_currentPos.x				+= deltaX;
	m_currentPos.y				+= deltaY;
	//AdjustCursorPOS(&m_currentPos);

	mouseMessage.dragEndPos		= m_currentPos;
	if (m_ctrl) {
		mouseMessage.uKeyState = MK_CONTROL;
	}
	if (m_shift) {
		mouseMessage.uKeyState |= MK_SHIFT;
	}
	VMMouseDrag(&mouseMessage);
}

/**
 * @brief
 * DOLLYコマンドを実行します。
 * 
 * @param deltaX
 * マウスの移動量[X軸]。
 * 
 * @param deltaY
 * マウスの移動量[Y軸]。
 * 
 * DOLLYコマンドを実行します。修飾キーの押下とマウス右ボタンドラッグの組み合わせです。
 * 
 * @remarks
 * VirtualMotion.dllのVMMouseDrag()関数を使用します。
 * 
 * @see
 * VMMouseDrag()
 */
void I4C3DControl::DollyExecute(int deltaX, int deltaY)
{
	VMMouseMessage mouseMessage = {0};
	if (!CheckTargetState()) {
		return;
	}

	mouseMessage.hTargetWnd		= m_hTargetChildWnd;
	mouseMessage.dragButton		= RButtonDrag;
	mouseMessage.dragStartPos	= m_currentPos;
	m_currentPos.x				+= deltaX;
	m_currentPos.y				+= deltaY;
	//AdjustCursorPOS(&m_currentPos);
	
	mouseMessage.dragEndPos		= m_currentPos;
	if (m_ctrl) {
		mouseMessage.uKeyState = MK_CONTROL;
	}
	if (m_shift) {
		mouseMessage.uKeyState |= MK_SHIFT;
	}
	VMMouseDrag(&mouseMessage);
}

inline void SetWindowTopMost(HWND hWnd) {
	LONG state = GetWindowLong(hWnd, GWL_EXSTYLE);
	if (!(state & WS_EX_TOPMOST)) {
		SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
	}
}

inline void SetWindowNoTopMost(HWND hWnd) {
	LONG state = GetWindowLong(hWnd, GWL_EXSTYLE);
	if (state & WS_EX_TOPMOST) {
		SetWindowPos(hWnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
	}
}


/**
 * @brief
 * 登録した修飾キーを押下します。
 * 
 * @param hTargetWnd
 * キー押下対象のウィンドウハンドル。
 * 
 * 登録した修飾キーを押下します。対象ウィドウに対して現在キーが押されているかどうかは判断しません。
 * 判断は派生クラスで行います。
 * 
 * @remarks
 * キー押下は何度行ってもかまいませんが、最後にはModKeyUp()でキー押下を解除する必要があります
 * [解除関数の呼び出しは1回でかまわない]。
 * 
 * @see
 * ModKeyUp()
 */
void I4C3DControl::ModKeyDown(HWND hTargetWnd)
{
	DWORD dwBuf = 0;
	HWND hForeground = GetForegroundWindow();
	DWORD dwThreadId = GetWindowThreadProcessId(hForeground, NULL);
	DWORD dwTargetThreadId = GetWindowThreadProcessId(hTargetWnd, NULL);

	AttachThreadInput(dwTargetThreadId, dwThreadId, TRUE);

	SystemParametersInfo(SPI_GETFOREGROUNDLOCKTIMEOUT, 0, &dwBuf, 0);
	SystemParametersInfo(SPI_SETFOREGROUNDLOCKTIMEOUT, 0, NULL, 0);

	SetForegroundWindow(m_hTargetParentWnd);
	Sleep(m_millisecSleepAfterKeyDown);

	if (m_ctrl) {
		VMVirtualKeyDown(hTargetWnd, VK_CONTROL, m_bUsePostMessage);
	}
	if (m_alt) {
		VMVirtualKeyDown(hTargetWnd, VK_MENU, m_bUsePostMessage);
	}
	if (m_shift) {
		VMVirtualKeyDown(hTargetWnd, VK_SHIFT, m_bUsePostMessage);
	}
	SystemParametersInfo(SPI_SETFOREGROUNDLOCKTIMEOUT, 0, &dwBuf, 0);
	AttachThreadInput(dwThreadId, dwTargetThreadId, FALSE);

	m_bSyskeyDown = TRUE;
}

/**
 * @brief
 * 登録した修飾キーの押下を解除します。
 * 
 * @param hTargetWnd
 * キー押下解除対象のウィンドウハンドル。
 * 
 * 登録した修飾キーの押下を解除します。
 * 
 * @remarks
 * ModKeyDown()でキー押下を行った場合には、最後には本関数でキー押下を解除する必要があります
 * [解除関数の呼び出しは1回でかまわない]。
 * 
 * @see
 * ModKeyDown()
 */
void I4C3DControl::ModKeyUp(HWND hTargetWnd)
{
	DEBUG_PROFILE_MONITOR;
	SetForegroundWindow(m_hTargetParentWnd);
	DEBUG_PROFILE_MONITOR;

	if (m_shift) {
		DEBUG_PROFILE_MONITOR;
		VMVirtualKeyUp(hTargetWnd, VK_SHIFT);
		DEBUG_PROFILE_MONITOR;
	}
	if (m_alt) {
		DEBUG_PROFILE_MONITOR;
		VMVirtualKeyUp(hTargetWnd, VK_MENU);
		DEBUG_PROFILE_MONITOR;
	}
	if (m_ctrl) {
		DEBUG_PROFILE_MONITOR;
		VMVirtualKeyUp(hTargetWnd, VK_CONTROL);
		DEBUG_PROFILE_MONITOR;
	}
	m_bSyskeyDown = FALSE;
}


/**
 * @brief
 * ショートカットメニューを実行します。
 * 
 * @param pContext
 * I4C3Dモジュールのコンテキストのポインタ。
 * 
 * @param szCommand
 * 設定ファイルに指定したショートカットメニュー名。
 * 例：<key name="perspective">F8</key>
 * この場合、"perspective"を指定。
 * 
 * ショートカットメニューを実行します。
 * ショートカット名とショートカットキーとの対応は設定ファイルに記載する必要があります。
 * 
 * @remarks
 * privateのHotkeyExecute()で解析をコマンドの送信を行います。
 * 
 * @see
 * HotkeyExecute()
 */
void I4C3DControl::HotkeyExecute(I4C3DContext* pContext, PCTSTR szCommand)
{
	HotkeyExecute(pContext, m_hTargetParentWnd, szCommand);
}

/**
 * @brief
 * ショートカットメニューを実行します。
 * 
 * @param pContext
 * I4C3Dモジュールのコンテキストのポインタ。
 * 
 * @param hTargetWnd
 * 対象のウィンドウハンドル。
 * 
 * @param szCommand
 * 設定ファイルに指定したショートカットメニュー名。
 * 
 * ショートカットメニューを実行します。
 * 
 * @remarks
 * 実際のキー送信はVKPush()で行います。
 * 
 * @see
 * VKPush()
 */
void I4C3DControl::HotkeyExecute(I4C3DContext* pContext, HWND hTargetWnd, PCTSTR szCommand)
{
	PCTSTR szCommandValue = pContext->pAnalyzer->GetSoftValue(szCommand);
	if (szCommandValue != NULL) {
		DWORD dwBuf = 0;
		HWND hForeground = GetForegroundWindow();
		DWORD dwThreadId = GetWindowThreadProcessId(hForeground, NULL);
		DWORD dwTargetThreadId = GetWindowThreadProcessId(hTargetWnd, NULL);

		AttachThreadInput(dwTargetThreadId, dwThreadId, TRUE);

		SystemParametersInfo(SPI_GETFOREGROUNDLOCKTIMEOUT, 0, &dwBuf, 0);
		SystemParametersInfo(SPI_SETFOREGROUNDLOCKTIMEOUT, 0, NULL, 0);

		SetForegroundWindow(hTargetWnd);

		VKPush(hTargetWnd, szCommandValue, m_bUsePostMessage);
		Sleep(5);

		SystemParametersInfo(SPI_SETFOREGROUNDLOCKTIMEOUT, 0, &dwBuf, 0);
		AttachThreadInput(dwThreadId, dwTargetThreadId, FALSE);
	}
}

/////////////////////////////////////////////////////////////////////////

void I4C3DControl::AdjustCursorPOS(PPOINT pPos)
{
	if (pPos == NULL) {
		return;
	}

	if (pPos->x < 0) {
		{
			TCHAR szDebug[I4C3D_BUFFER_SIZE];
			_stprintf_s(szDebug, _countof(szDebug), _T("はみ出し: pPos->x = %d -> 0"), pPos->x);
			I4C3DMisc::LogDebugMessage(Log_Debug, szDebug);
		}
		pPos->x = 0;
	} else if (m_DisplayWidth < pPos->x) {
		{
			TCHAR szDebug[I4C3D_BUFFER_SIZE];
			_stprintf_s(szDebug, _countof(szDebug), _T("はみ出し: pPos->x = %d -> %d"), pPos->x, m_DisplayWidth);
			I4C3DMisc::LogDebugMessage(Log_Debug, szDebug);
		}
		pPos->x = m_DisplayWidth;
	}

	if (pPos->y < 0) {
		{
			TCHAR szDebug[I4C3D_BUFFER_SIZE];
			_stprintf_s(szDebug, _countof(szDebug), _T("はみ出し: pPos->y = %d -> 0"), pPos->y);
			I4C3DMisc::LogDebugMessage(Log_Debug, szDebug);
		}
		pPos->y = 0;
	} else if (m_DisplayHeight < pPos->y) {
		{
			TCHAR szDebug[I4C3D_BUFFER_SIZE];
			_stprintf_s(szDebug, _countof(szDebug), _T("はみ出し: pPos->y = %d -> %d"), pPos->y, m_DisplayHeight);
			I4C3DMisc::LogDebugMessage(Log_Debug, szDebug);
		}
		pPos->y = m_DisplayHeight;
	}
}

/**
 * @brief
 * 対象ターゲットの位置等をチェックします。
 * 
 * @returns
 * 対象ターゲットがNULLの場合にFALSE、それ以外ではウィンドウの位置を確認した後TRUEを返します。
 * 
 * 対象ターゲットの位置等をチェックします。
 */
BOOL I4C3DControl::CheckTargetState(void)
{
	RECT rect;

	if (m_hTargetParentWnd == NULL) {
		I4C3DMisc::ReportError(_T("ターゲットウィンドウが取得できません。<I4C3DControl::CheckTargetState>"));
		I4C3DMisc::LogDebugMessage(Log_Error, _T("ターゲットウィンドウが取得できません。<I4C3DControl::CheckTargetState>"));

	} else if (m_hTargetChildWnd == NULL) {
		I4C3DMisc::LogDebugMessage(Log_Error, _T("子ウィンドウが取得できません。<I4C3DControl::CheckTargetState>"));

	} else {
		// ターゲットウィンドウの位置のチェック
		//GetWindowRect(m_hTargetParentWnd, &rect);
		GetClientRect(m_hTargetParentWnd, &rect);
		m_currentPos.x = rect.left + (rect.right - rect.left) / 2;
		m_currentPos.y = rect.top + (rect.bottom - rect.top) / 2;

		return TRUE;
	}

	return FALSE;
}


/**
 * @brief
 * ショートカットメニューの実行を行います。
 * 
 * @param hTargetWnd
 * 対象のウィンドウハンドル。
 * 
 * @param szKeyTypes
 * ショートカットキー[の組み合わせ]。
 * 
 * @param m_bUsePostMessage
 * キー送信にPostMessageを使用する場合[RTT系]にはTRUE、SendInputを使用する場合[AutoDesk系]にはFALSEを指定します。
 * 
 * ショートカットメニューの実行を行います。
 * ショートカットキーの解析を再帰的に行いながら実行します。
 * 
 * @remarks
 * HotKeyExecute()から呼び出されます。
 * 
 * @see
 * HotKeyExecute()
 */
void VKPush(HWND hTargetWnd, PCTSTR szKeyTypes, BOOL m_bUsePostMessage) {
	PCTSTR pType = _tcschr(szKeyTypes, _T('+'));
	TCHAR szKey[I4C3D_BUFFER_SIZE] = {0};
	if (pType != NULL) {
		_tcsncpy_s(szKey, _countof(szKey), szKeyTypes, pType-szKeyTypes+1);
	} else {
		_tcscpy_s(szKey, _countof(szKey), szKeyTypes);
	}
	CharUpper(szKey);
	I4C3DMisc::RemoveWhiteSpace(szKey);
	UINT vKey = VMGetVirtualKey(szKey);

	// キーダウン
	switch (vKey) {
	case 0:
		VMVirtualKeyDown(hTargetWnd, szKey[0], m_bUsePostMessage);
		break;

	case VK_CONTROL:
	case VK_MENU:
	case VK_SHIFT:
		VMVirtualKeyDown(hTargetWnd, vKey, m_bUsePostMessage);
		//Sleep(TIME_WAIT);
		break;

	default:
		VMVirtualKeyDown(hTargetWnd, vKey, m_bUsePostMessage);
	}

	// 再帰的に次のキー入力
	if (pType != NULL) {
		VKPush(hTargetWnd, pType+1, m_bUsePostMessage);
	}

	// キーアップ
	switch (vKey) {
	case 0:
		VMVirtualKeyUp(hTargetWnd, szKey[0]);
		break;

	case VK_CONTROL:
	case VK_MENU:
	case VK_SHIFT:
		VMVirtualKeyUp(hTargetWnd, vKey);
		//Sleep(TIME_WAIT);
		break;

	default:
		VMVirtualKeyUp(hTargetWnd, vKey);
	}
}

