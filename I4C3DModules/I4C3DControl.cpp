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
 * Control�I�u�W�F�N�g�̏��������s���܂��B
 * 
 * @param pContext
 * I4C3D���W���[���̃R���e�L�X�g�̃|�C���^�B
 * 
 * @returns
 * �������ɐ��������ꍇ�ɂ�TRUE�A���s�����ꍇ�ɂ�FALSE��Ԃ��܂��B
 * 
 * Control�I�u�W�F�N�g�̏��������s���܂��B
 * �ǂ�Control�I�u�W�F�N�g���́A������I4C3DContext�|�C���^�ɓo�^����Ă���Control�h���N���X�ɂ��܂��B
 * 
 * @remarks
 * InitializeModifierKeys()�ŏC���L�[�̐ݒ���s���܂��B
 * 
 * @see
 * InitializeModifierKeys()
 */
BOOL I4C3DControl::Initialize(I4C3DContext* pContext)
{
	m_hTargetParentWnd = pContext->hTargetParentWnd;
	if (m_hTargetParentWnd == NULL) {
		I4C3DMisc::ReportError(_T("[ERROR] �^�[�Q�b�g�E�B���h�E���擾�ł��܂���B"));
		return FALSE;
	}
	m_hTargetChildWnd	= NULL;
	m_currentPos.x		= 0;
	m_currentPos.y		= 0;
	m_millisecSleepAfterKeyDown = 0;
	m_bUsePostMessage	= FALSE;

	// �ݒ�t�@�C������L�[���M��̃X���[�v�~���b���擾
	PCTSTR szSleep = pContext->pAnalyzer->GetSoftValue(TAG_SLEEP);
	if (szSleep == NULL || 
		_stscanf_s(szSleep, _T("%d"), &m_millisecSleepAfterKeyDown, sizeof(m_millisecSleepAfterKeyDown)) != 1) {
		m_millisecSleepAfterKeyDown = 0;
	}

	// �ݒ�t�@�C������e�R�}���h�̈ړ��ʃp�����[�^�̔䗦���擾
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
 * 3D�\�t�g�Ŏg�p����C���L�[�̎擾�A�ݒ�A����уL�[�����̊Ď��v���O�����ւ̓o�^���s���܂��B
 * 
 * @param pContext
 * I4C3D���W���[���̃R���e�L�X�g�̃|�C���^�B
 * 
 * @returns
 * �C���L�[�̎擾�A�ݒ�A�o�^�ɐ��������ꍇ�ɂ�TRUE�A���s�����ꍇ�ɂ�FALSE��Ԃ��܂��B
 * 
 * 3D�\�t�g�Ŏg�p����C���L�[�̎擾�A�ݒ�A����уL�[�����̊Ď��v���O�����ւ̓o�^���s���܂��B
 * 
 * @remarks
 * I4C3DKeysHook.dll��AddHookedKeyCode()�ŃL�[�t�b�N�̓o�^���s���܂��B
 * 
 * @see
 * AddHookedKeyCode()
 */
BOOL I4C3DControl::InitializeModifierKeys(I4C3DContext* pContext)
{
	m_ctrl = m_alt = m_shift = m_bSyskeyDown = FALSE;

	PCTSTR szModifierKey = pContext->pAnalyzer->GetSoftValue(TAG_MODIFIER);
	if (szModifierKey == NULL) {
		I4C3DMisc::LogDebugMessage(Log_Error, _T("�ݒ�t�@�C���ɏC���L�[��ݒ肵�Ă��������B<I4C3DControl::InitializeModifierKeys>"));
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
 * �o�^�����C���L�[�������ꂽ���m�F���܂��B
 * 
 * @returns
 * �o�^�����C���L�[��������Ă���ꍇ�ɂ�TRUE�A�����łȂ��ꍇ��FALSE��Ԃ��܂��B
 * 
 * �o�^�����C���L�[�������ꂽ���m�F���܂��B
 * ������Ă��Ȃ��ꍇ�́ASleep���܂��B
 * Sleep�͍ő�retryCount��s���ASleep�Ԋu��
 * ����d�˂邲�Ƃ�2�{���Ă����܂��B
 * �i�ő� [1 << retryCount] msec��Sleep�j
 * 
 * @remarks
 * I4C3DKeysHook.dll��IsAllKeysDown()�֐��ŃL�[�������m�F���܂��B
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
 * 3D�\�t�g��TUMBLE/TRACK/DOLLY�R�}���h���삨��уV���[�g�J�b�g�L�[�̑�����s���܂��B
 * 
 * @param pContext
 * I4C3D���W���[���̃R���e�L�X�g�̃|�C���^�B
 * 
 * @param pDelta
 * �}�E�X�̈ړ��ʁB
 * 
 * @param szCommand
 * �R�}���h������уV���[�g�J�b�g���j���[���B
 * 
 * 3D�\�t�g��TUMBLE/TRACK/DOLLY�R�}���h���삨��уV���[�g�J�b�g�L�[�̑�����s���܂��B
 */
void I4C3DControl::Execute(I4C3DContext* pContext, PPOINT pDelta, LPCSTR szCommand)
{
	// ���ۂɉ��z�L�[�E���z�}�E�X������s���q�E�B���h�E�̎擾
	if (!pContext->pController->GetTargetChildWnd()) {
		//TCHAR szError[I4C3D_BUFFER_SIZE];
		//_stprintf_s(szError, _countof(szError), _T("[ERROR] �^�[�Q�b�g�\�t�g[%s]�̎q�E�B���h�E���擾�ł��܂���Bwindow_title[%s]�����������m�F���Ă��������B"),
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
 * TUMBLE�R�}���h�����s���܂��B
 * 
 * @param deltaX
 * �}�E�X�̈ړ���[X��]�B
 * 
 * @param deltaY
 * �}�E�X�̈ړ���[Y��]�B
 * 
 * TUMBLE�R�}���h�����s���܂��B�C���L�[�̉����ƃ}�E�X���{�^���h���b�O�̑g�ݍ��킹�ł��B
 * 
 * @remarks
 * VirtualMotion.dll��VMMouseDrag()�֐����g�p���܂��B
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
 * TRACK�R�}���h�����s���܂��B
 * 
 * @param deltaX
 * �}�E�X�̈ړ���[X��]�B
 * 
 * @param deltaY
 * �}�E�X�̈ړ���[Y��]�B
 * 
 * TRACK�R�}���h�����s���܂��B�C���L�[�̉����ƃ}�E�X���{�^���h���b�O�̑g�ݍ��킹�ł��B
 * 
 * @remarks
 * VirtualMotion.dll��VMMouseDrag()�֐����g�p���܂��B
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
 * DOLLY�R�}���h�����s���܂��B
 * 
 * @param deltaX
 * �}�E�X�̈ړ���[X��]�B
 * 
 * @param deltaY
 * �}�E�X�̈ړ���[Y��]�B
 * 
 * DOLLY�R�}���h�����s���܂��B�C���L�[�̉����ƃ}�E�X�E�{�^���h���b�O�̑g�ݍ��킹�ł��B
 * 
 * @remarks
 * VirtualMotion.dll��VMMouseDrag()�֐����g�p���܂��B
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
 * �o�^�����C���L�[���������܂��B
 * 
 * @param hTargetWnd
 * �L�[�����Ώۂ̃E�B���h�E�n���h���B
 * 
 * �o�^�����C���L�[���������܂��B�ΏۃE�B�h�E�ɑ΂��Č��݃L�[��������Ă��邩�ǂ����͔��f���܂���B
 * ���f�͔h���N���X�ōs���܂��B
 * 
 * @remarks
 * �L�[�����͉��x�s���Ă����܂��܂��񂪁A�Ō�ɂ�ModKeyUp()�ŃL�[��������������K�v������܂�
 * [�����֐��̌Ăяo����1��ł��܂�Ȃ�]�B
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
 * �o�^�����C���L�[�̉������������܂��B
 * 
 * @param hTargetWnd
 * �L�[���������Ώۂ̃E�B���h�E�n���h���B
 * 
 * �o�^�����C���L�[�̉������������܂��B
 * 
 * @remarks
 * ModKeyDown()�ŃL�[�������s�����ꍇ�ɂ́A�Ō�ɂ͖{�֐��ŃL�[��������������K�v������܂�
 * [�����֐��̌Ăяo����1��ł��܂�Ȃ�]�B
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
 * �V���[�g�J�b�g���j���[�����s���܂��B
 * 
 * @param pContext
 * I4C3D���W���[���̃R���e�L�X�g�̃|�C���^�B
 * 
 * @param szCommand
 * �ݒ�t�@�C���Ɏw�肵���V���[�g�J�b�g���j���[���B
 * ��F<key name="perspective">F8</key>
 * ���̏ꍇ�A"perspective"���w��B
 * 
 * �V���[�g�J�b�g���j���[�����s���܂��B
 * �V���[�g�J�b�g���ƃV���[�g�J�b�g�L�[�Ƃ̑Ή��͐ݒ�t�@�C���ɋL�ڂ���K�v������܂��B
 * 
 * @remarks
 * private��HotkeyExecute()�ŉ�͂��R�}���h�̑��M���s���܂��B
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
 * �V���[�g�J�b�g���j���[�����s���܂��B
 * 
 * @param pContext
 * I4C3D���W���[���̃R���e�L�X�g�̃|�C���^�B
 * 
 * @param hTargetWnd
 * �Ώۂ̃E�B���h�E�n���h���B
 * 
 * @param szCommand
 * �ݒ�t�@�C���Ɏw�肵���V���[�g�J�b�g���j���[���B
 * 
 * �V���[�g�J�b�g���j���[�����s���܂��B
 * 
 * @remarks
 * ���ۂ̃L�[���M��VKPush()�ōs���܂��B
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
			_stprintf_s(szDebug, _countof(szDebug), _T("�͂ݏo��: pPos->x = %d -> 0"), pPos->x);
			I4C3DMisc::LogDebugMessage(Log_Debug, szDebug);
		}
		pPos->x = 0;
	} else if (m_DisplayWidth < pPos->x) {
		{
			TCHAR szDebug[I4C3D_BUFFER_SIZE];
			_stprintf_s(szDebug, _countof(szDebug), _T("�͂ݏo��: pPos->x = %d -> %d"), pPos->x, m_DisplayWidth);
			I4C3DMisc::LogDebugMessage(Log_Debug, szDebug);
		}
		pPos->x = m_DisplayWidth;
	}

	if (pPos->y < 0) {
		{
			TCHAR szDebug[I4C3D_BUFFER_SIZE];
			_stprintf_s(szDebug, _countof(szDebug), _T("�͂ݏo��: pPos->y = %d -> 0"), pPos->y);
			I4C3DMisc::LogDebugMessage(Log_Debug, szDebug);
		}
		pPos->y = 0;
	} else if (m_DisplayHeight < pPos->y) {
		{
			TCHAR szDebug[I4C3D_BUFFER_SIZE];
			_stprintf_s(szDebug, _countof(szDebug), _T("�͂ݏo��: pPos->y = %d -> %d"), pPos->y, m_DisplayHeight);
			I4C3DMisc::LogDebugMessage(Log_Debug, szDebug);
		}
		pPos->y = m_DisplayHeight;
	}
}

/**
 * @brief
 * �Ώۃ^�[�Q�b�g�̈ʒu�����`�F�b�N���܂��B
 * 
 * @returns
 * �Ώۃ^�[�Q�b�g��NULL�̏ꍇ��FALSE�A����ȊO�ł̓E�B���h�E�̈ʒu���m�F������TRUE��Ԃ��܂��B
 * 
 * �Ώۃ^�[�Q�b�g�̈ʒu�����`�F�b�N���܂��B
 */
BOOL I4C3DControl::CheckTargetState(void)
{
	RECT rect;

	if (m_hTargetParentWnd == NULL) {
		I4C3DMisc::ReportError(_T("�^�[�Q�b�g�E�B���h�E���擾�ł��܂���B<I4C3DControl::CheckTargetState>"));
		I4C3DMisc::LogDebugMessage(Log_Error, _T("�^�[�Q�b�g�E�B���h�E���擾�ł��܂���B<I4C3DControl::CheckTargetState>"));

	} else if (m_hTargetChildWnd == NULL) {
		I4C3DMisc::LogDebugMessage(Log_Error, _T("�q�E�B���h�E���擾�ł��܂���B<I4C3DControl::CheckTargetState>"));

	} else {
		// �^�[�Q�b�g�E�B���h�E�̈ʒu�̃`�F�b�N
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
 * �V���[�g�J�b�g���j���[�̎��s���s���܂��B
 * 
 * @param hTargetWnd
 * �Ώۂ̃E�B���h�E�n���h���B
 * 
 * @param szKeyTypes
 * �V���[�g�J�b�g�L�[[�̑g�ݍ��킹]�B
 * 
 * @param m_bUsePostMessage
 * �L�[���M��PostMessage���g�p����ꍇ[RTT�n]�ɂ�TRUE�ASendInput���g�p����ꍇ[AutoDesk�n]�ɂ�FALSE���w�肵�܂��B
 * 
 * �V���[�g�J�b�g���j���[�̎��s���s���܂��B
 * �V���[�g�J�b�g�L�[�̉�͂��ċA�I�ɍs���Ȃ�����s���܂��B
 * 
 * @remarks
 * HotKeyExecute()����Ăяo����܂��B
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

	// �L�[�_�E��
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

	// �ċA�I�Ɏ��̃L�[����
	if (pType != NULL) {
		VKPush(hTargetWnd, pType+1, m_bUsePostMessage);
	}

	// �L�[�A�b�v
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

