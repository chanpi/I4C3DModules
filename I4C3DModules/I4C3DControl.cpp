#include "StdAfx.h"
#include "I4C3DControl.h"
#include "I4C3DMisc.h"
#include "I4C3DAnalyzeXML.h"
#include "I4C3DSoftwareHandler.h"
#include "I4C3DCommon.h"
#include <vector>
using namespace std;

//static void VKPush(HWND hTargetWnd, PCTSTR szKeyTypes, BOOL bUsePostMessage);
static const PCTSTR TAG_MODIFIER	= _T("modifier");
static const PCTSTR TAG_UDP_PORT	= _T("udp_port");
static const PCTSTR TAG_TERMINATION	= _T("termination");
static const PCTSTR TAG_TUMBLERATE	= _T("tumble_rate");
static const PCTSTR TAG_TRACKRATE	= _T("track_rate");
static const PCTSTR TAG_DOLLYRATE	= _T("dolly_rate");

static TCHAR *g_szController[] = { _T("RTT"), _T("Maya"), _T("Alias"), _T("Showcase"), };
static vector<I4C3DSoftwareHandler*> *g_pSoftwareHandler = NULL;

I4C3DControl::I4C3DControl(void)
{
	g_pSoftwareHandler = new vector<I4C3DSoftwareHandler*>;
}

I4C3DControl::~I4C3DControl(void)
{
	UnInitialize();
	delete g_pSoftwareHandler;
	g_pSoftwareHandler = NULL;
}

/**
 * @brief
 * �C���L�[�Ɗe�R�}���h�̈ړ��ʊg�嗦��ύX���܂��B
 * 
 * @param bCtrl
 * CONTROL�L�[�������B
 * 
 * @param bAlt
 * ALT�L�[�������B
 * 
 * @param bShift
 * SHIFT�L�[�������B
 * 
 * @param tumbleRate
 * TUMBLE�g�嗦�B
 * 
 * @param trackRate
 * TRACK�g�嗦�B
 * 
 * @param dollyRate
 * DOLLY�g�嗦�B
 * 
 * �C���L�[�Ɗe�R�}���h�̈ړ��ʊg�嗦��ύX���܂��B
 * 
 * @remarks
 * GUI����̕ύX���󂯕t���܂��B
 */
//void I4C3DControl::ResetModKeysAndExpansionRate(BOOL bCtrl, BOOL bAlt, BOOL bShift,
//		DOUBLE tumbleRate, DOUBLE trackRate, DOUBLE dollyRate)
//{
//	m_ctrl = bCtrl;
//	m_alt = bAlt;
//	m_shift = bShift;
//	m_fTumbleRate = tumbleRate;
//	m_fTrackRate = trackRate;
//	m_fDollyRate = dollyRate;
//}

/**
 * @brief
 * �C���L�[�E�g�嗦�E�I�[������ݒ�t�@�C������擾���A�e3D�\�t�g�̃\�P�b�g��init���b�Z�[�W�𑗐M���܂��B
 * 
 * @param pContext
 * I4C3D���W���[���̃R���e�L�X�g�̃|�C���^�B
 * 
 * @returns
 * �������ɐ��������ꍇ�ɂ�TRUE�A���s�����ꍇ�ɂ�FALSE��Ԃ��܂��B
 * 
 * �C���L�[�E�g�嗦�E�I�[������ݒ�t�@�C������擾���A�e3D�\�t�g�̃\�P�b�g��UDP��init���b�Z�[�W�𑗐M���܂��B
 */
BOOL I4C3DControl::Initialize(I4C3DContext* pContext, char cTermination)
{
	int count = _countof(g_szController);
	for (int i = 0; i < count; ++i) {
		PCTSTR szModifierKey = NULL;
		char cszModifierKey[I4C3D_BUFFER_SIZE] = {0};
		double fTumbleRate = 1.0;
		double fTrackRate = 1.0;
		double fDollyRate = 1.0;
		char sendBuffer[I4C3D_BUFFER_SIZE] = {0};

		// �|�[�g�ԍ�
		USHORT uPort = 10000;
		PCTSTR szPort = pContext->pAnalyzer->GetSoftValue(g_szController[i], TAG_UDP_PORT);
		if (szPort == NULL) {
			uPort += i;
			TCHAR szError[I4C3D_BUFFER_SIZE] = {0};
			_stprintf_s(szError, _countof(szError), _T("�ݒ�t�@�C����%s��UDP�|�[�g���w�肵�Ă��������B����%u�ɐݒ肵�܂��B<I4C3DControl::Initialize>"), g_szController[i], uPort);
			I4C3DMisc::LogDebugMessage(Log_Error, szError);			
		} else {
			uPort = _wtoi(szPort);
		}

		// �n���h���̏�����
		I4C3DSoftwareHandler* pHandler = new I4C3DSoftwareHandler(g_szController[i], uPort);
		pHandler->PrepareSocket();

		// �n���h����vector�ŊǗ�
		g_pSoftwareHandler->push_back(pHandler);

		// �C���L�[
		szModifierKey = pContext->pAnalyzer->GetSoftValue(g_szController[i], TAG_MODIFIER);
		if (szModifierKey == NULL) {
			I4C3DMisc::LogDebugMessage(Log_Error, _T("�ݒ�t�@�C���ɏC���L�[��ݒ肵�Ă��������B<I4C3DControl::Initialize>"));
			strcpy_s(cszModifierKey, sizeof(cszModifierKey), "NULL");	// "NULL"�̏ꍇ�́A���ꂼ��̃v���O�C���Ńf�t�H���g�l�ɐݒ肷��B
		} else {
#if UNICODE || _UNICODE
			WideCharToMultiByte(CP_ACP, 0, szModifierKey, _tcslen(szModifierKey), cszModifierKey, sizeof(cszModifierKey), NULL, NULL);
#else
			strcpy_s(cszModifierKey, sizeof(cszModifierKey), szModifierKey);
#endif
		}

		// �e�R�}���h�̈ړ��ʃp�����[�^�̔䗦���擾
		PCTSTR szRate = pContext->pAnalyzer->GetSoftValue(g_szController[i], TAG_TUMBLERATE);
		if (szRate == NULL ||
			_stscanf_s(szRate, _T("%lf"), &fTumbleRate, sizeof(fTumbleRate)) != 1) {
				fTumbleRate = 1.0;
		}
		szRate = pContext->pAnalyzer->GetSoftValue(g_szController[i], TAG_TRACKRATE);
		if (szRate == NULL ||
			_stscanf_s(szRate, _T("%lf"), &fTrackRate, sizeof(fTrackRate)) != 1) {
				fTrackRate = 1.0;
		}
		szRate = pContext->pAnalyzer->GetSoftValue(g_szController[i], TAG_DOLLYRATE);
		if (szRate == NULL ||
			_stscanf_s(szRate, _T("%lf"), &fDollyRate, sizeof(fDollyRate)) != 1) {
				fDollyRate = 1.0;
		}

		sprintf_s(sendBuffer, g_initCommandFormat, "init", cszModifierKey, fTumbleRate, fTrackRate, fDollyRate, cTermination);
		sendto(pHandler->m_socketHandler, sendBuffer, strlen(sendBuffer), 0, (const SOCKADDR*)&pHandler->m_address, sizeof(pHandler->m_address));
	}
	return TRUE;
}

void I4C3DControl::UnInitialize(void)
{
	if (g_pSoftwareHandler != NULL) {
		int size = g_pSoftwareHandler->size();
		for (int i = 0; i < size; ++i) {
			sendto((*g_pSoftwareHandler)[i]->m_socketHandler, "exit", 5, 0, (const SOCKADDR*)&(*g_pSoftwareHandler)[i]->m_address, sizeof((*g_pSoftwareHandler)[i]->m_address));
			closesocket((*g_pSoftwareHandler)[i]->m_socketHandler);
			delete (*g_pSoftwareHandler)[i];
		}
		g_pSoftwareHandler->clear();
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
void I4C3DControl::Execute(LPCSTR szCommand, int commandLen)
{
	TCHAR szWindowTitle[I4C3D_BUFFER_SIZE];
	HWND hForeground = GetForegroundWindow();
	if (IsIconic(hForeground)) {
		return;
	}
	GetWindowText(GetForegroundWindow(), szWindowTitle, _countof(szWindowTitle));
	//OutputDebugString(szWindowTitle);
	//OutputDebugString(_T("\n"));

	int size = g_pSoftwareHandler->size();
	for (int i = 0; i < size; ++i) {
		if (_tcsstr(szWindowTitle, (*g_pSoftwareHandler)[i]->m_szTargetTitle) != NULL) {
			int n = sendto((*g_pSoftwareHandler)[i]->m_socketHandler, szCommand, commandLen, 0, (const SOCKADDR*)&((*g_pSoftwareHandler)[i]->m_address), sizeof((*g_pSoftwareHandler)[i]->m_address));
			return;
		}
	}
}

///**
// * @brief
// * TUMBLE�R�}���h�����s���܂��B
// * 
// * @param deltaX
// * �}�E�X�̈ړ���[X��]�B
// * 
// * @param deltaY
// * �}�E�X�̈ړ���[Y��]�B
// * 
// * TUMBLE�R�}���h�����s���܂��B�C���L�[�̉����ƃ}�E�X���{�^���h���b�O�̑g�ݍ��킹�ł��B
// * 
// * @remarks
// * VirtualMotion.dll��VMMouseDrag()�֐����g�p���܂��B
// * 
// * @see
// * VMMouseDrag()
// */
//#define MOUSE_EVENT_X(x)		((x) * (65535 / ::GetSystemMetrics(SM_CXSCREEN)))
//#define MOUSE_EVENT_Y(y)		((y) * (65535 / ::GetSystemMetrics(SM_CYSCREEN)))
//
//void I4C3DControl::TumbleExecute(int deltaX, int deltaY)
//{
//	if (m_mouseMessage.dragButton != LButtonDrag) {
//		if (m_mouseMessage.dragButton != DragNONE) {
//			VMMouseClick(&m_mouseMessage, TRUE);
//			m_mouseMessage.dragButton = DragNONE;
//		}
//		OutputDebugString(L"Reset Click!!!!\n");
//	}
//
//	if (!CheckTargetState()) {
//		return;
//	}
//	m_mouseMessage.bUsePostMessage = m_bUsePostMessageToMouseDrag;
//	m_mouseMessage.hTargetWnd		= m_hTargetChildWnd;
//	m_mouseMessage.dragStartPos		= m_currentPos;
//	m_currentPos.x					+= deltaX;
//	m_currentPos.y					+= deltaY;
//	m_mouseMessage.dragEndPos		= m_currentPos;
//
//	m_mouseMessage.uKeyState		= MK_LBUTTON;
//	if (m_ctrl) {
//		m_mouseMessage.uKeyState |= MK_CONTROL;
//	}
//	if (m_shift) {
//		m_mouseMessage.uKeyState |= MK_SHIFT;
//	}
//
//	if (m_mouseMessage.dragButton != LButtonDrag) {
//		m_mouseMessage.dragButton = LButtonDrag;
//		VMMouseClick(&m_mouseMessage, FALSE);
//		OutputDebugString(L"Click('0')/\n");
//	}
//	VMMouseMove(&m_mouseMessage/*, m_reduceCount*/);
//
//	//DEBUG_PROFILE_MONITOR;
//	//VMMouseMessage mouseMessage = {0};
//	//if (!CheckTargetState()) {
//	//	return;
//	//}
//	//DEBUG_PROFILE_MONITOR;
//	//mouseMessage.bUsePostMessage = m_bUsePostMessageToMouseDrag;
//	//mouseMessage.hTargetWnd		= m_hTargetChildWnd;
//	//mouseMessage.dragButton		= LButtonDrag;
//	//mouseMessage.dragStartPos	= m_currentPos;
//	//m_currentPos.x				+= deltaX;
//	//m_currentPos.y				+= deltaY;
//	////AdjustCursorPOS(&m_currentPos);
//
//	//mouseMessage.dragEndPos		= m_currentPos;
//	//if (m_ctrl) {
//	//	mouseMessage.uKeyState = MK_CONTROL;
//	//}
//	//if (m_shift) {
//	//	mouseMessage.uKeyState |= MK_SHIFT;
//	//}
//	//DEBUG_PROFILE_MONITOR;
//	//VMMouseEvent(&mouseMessage, m_reduceCount, FALSE, FALSE);
//	////VMMouseDrag(&mouseMessage, m_reduceCount);
//	//DEBUG_PROFILE_MONITOR;
//}
//
///**
// * @brief
// * TRACK�R�}���h�����s���܂��B
// * 
// * @param deltaX
// * �}�E�X�̈ړ���[X��]�B
// * 
// * @param deltaY
// * �}�E�X�̈ړ���[Y��]�B
// * 
// * TRACK�R�}���h�����s���܂��B�C���L�[�̉����ƃ}�E�X���{�^���h���b�O�̑g�ݍ��킹�ł��B
// * 
// * @remarks
// * VirtualMotion.dll��VMMouseDrag()�֐����g�p���܂��B
// * 
// * @see
// * VMMouseDrag()
// */
//void I4C3DControl::TrackExecute(int deltaX, int deltaY)
//{
//	if (m_mouseMessage.dragButton != MButtonDrag) {
//		if (m_mouseMessage.dragButton != DragNONE) {
//			VMMouseClick(&m_mouseMessage, TRUE);
//			m_mouseMessage.dragButton = DragNONE;
//		}
//	}
//	if (!CheckTargetState()) {
//		return;
//	}
//	m_mouseMessage.bUsePostMessage = m_bUsePostMessageToMouseDrag;
//	m_mouseMessage.hTargetWnd		= m_hTargetChildWnd;
//	//m_mouseMessage.dragButton		= MButtonDrag;
//	m_mouseMessage.dragStartPos		= m_currentPos;
//	m_currentPos.x					+= deltaX;
//	m_currentPos.y					+= deltaY;
//	m_mouseMessage.dragEndPos		= m_currentPos;
//
//	m_mouseMessage.uKeyState		= MK_MBUTTON;
//	if (m_ctrl) {
//		m_mouseMessage.uKeyState |= MK_CONTROL;
//	}
//	if (m_shift) {
//		m_mouseMessage.uKeyState |= MK_SHIFT;
//	}
//	if (m_mouseMessage.dragButton != MButtonDrag) {
//		m_mouseMessage.dragButton = MButtonDrag;
//		VMMouseClick(&m_mouseMessage, FALSE);
//	}
//	//VMMouseEvent(&m_mouseMessage, m_reduceCount, FALSE, FALSE);
//	VMMouseMove(&m_mouseMessage/*, m_reduceCount*/);
//
//	//VMMouseMessage mouseMessage = {0};
//	//if (!CheckTargetState()) {
//	//	return;
//	//}
//
//	//mouseMessage.bUsePostMessage = m_bUsePostMessageToMouseDrag;
//	//mouseMessage.hTargetWnd		= m_hTargetChildWnd;
//	//mouseMessage.dragButton		= MButtonDrag;
//	//mouseMessage.dragStartPos	= m_currentPos;
//	//m_currentPos.x				+= deltaX;
//	//m_currentPos.y				+= deltaY;
//	////AdjustCursorPOS(&m_currentPos);
//
//	//mouseMessage.dragEndPos		= m_currentPos;
//	//if (m_ctrl) {
//	//	mouseMessage.uKeyState = MK_CONTROL;
//	//}
//	//if (m_shift) {
//	//	mouseMessage.uKeyState |= MK_SHIFT;
//	//}
//	////VMMouseDrag(&mouseMessage, m_reduceCount);
//	//VMMouseEvent(&mouseMessage, m_reduceCount, FALSE, FALSE);
//}
//
///**
// * @brief
// * DOLLY�R�}���h�����s���܂��B
// * 
// * @param deltaX
// * �}�E�X�̈ړ���[X��]�B
// * 
// * @param deltaY
// * �}�E�X�̈ړ���[Y��]�B
// * 
// * DOLLY�R�}���h�����s���܂��B�C���L�[�̉����ƃ}�E�X�E�{�^���h���b�O�̑g�ݍ��킹�ł��B
// * 
// * @remarks
// * VirtualMotion.dll��VMMouseDrag()�֐����g�p���܂��B
// * 
// * @see
// * VMMouseDrag()
// */
//void I4C3DControl::DollyExecute(int deltaX, int deltaY)
//{
//	if (m_mouseMessage.dragButton != RButtonDrag) {
//		if (m_mouseMessage.dragButton != DragNONE) {
//			VMMouseClick(&m_mouseMessage, TRUE);
//			m_mouseMessage.dragButton = DragNONE;
//		}
//	}
//
//	if (!CheckTargetState()) {
//		return;
//	}
//	m_mouseMessage.bUsePostMessage = m_bUsePostMessageToMouseDrag;
//	m_mouseMessage.hTargetWnd		= m_hTargetChildWnd;
//	//m_mouseMessage.dragButton		= RButtonDrag;
//	m_mouseMessage.dragStartPos		= m_currentPos;
//	m_currentPos.x					+= deltaX;
//	m_currentPos.y					+= deltaY;
//	m_mouseMessage.dragEndPos		= m_currentPos;
//
//	m_mouseMessage.uKeyState		= MK_RBUTTON;
//	if (m_ctrl) {
//		m_mouseMessage.uKeyState |= MK_CONTROL;
//	}
//	if (m_shift) {
//		m_mouseMessage.uKeyState |= MK_SHIFT;
//	}
//	if (m_mouseMessage.dragButton != RButtonDrag) {
//		m_mouseMessage.dragButton = RButtonDrag;
//		VMMouseClick(&m_mouseMessage, FALSE);
//	}
//	//VMMouseEvent(&m_mouseMessage, m_reduceCount, FALSE, FALSE);
//	VMMouseMove(&m_mouseMessage/*, m_reduceCount*/);
//
//	//VMMouseMessage mouseMessage = {0};
//	//if (!CheckTargetState()) {
//	//	return;
//	//}
//
//	//mouseMessage.bUsePostMessage = m_bUsePostMessageToMouseDrag;
//	//mouseMessage.hTargetWnd		= m_hTargetChildWnd;
//	//mouseMessage.dragButton		= RButtonDrag;
//	//mouseMessage.dragStartPos	= m_currentPos;
//	//m_currentPos.x				+= deltaX;
//	//m_currentPos.y				+= deltaY;
//	////AdjustCursorPOS(&m_currentPos);
//	//
//	//mouseMessage.dragEndPos		= m_currentPos;
//	//if (m_ctrl) {
//	//	mouseMessage.uKeyState = MK_CONTROL;
//	//}
//	//if (m_shift) {
//	//	mouseMessage.uKeyState |= MK_SHIFT;
//	//}
//	//VMMouseEvent(&mouseMessage, m_reduceCount, FALSE, FALSE);
//	////VMMouseDrag(&mouseMessage, m_reduceCount);
//}
//
//inline void SetWindowTopMost(HWND hWnd) {
//	LONG state = GetWindowLong(hWnd, GWL_EXSTYLE);
//	if (!(state & WS_EX_TOPMOST)) {
//		SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
//	}
//}
//
//inline void SetWindowNoTopMost(HWND hWnd) {
//	LONG state = GetWindowLong(hWnd, GWL_EXSTYLE);
//	if (state & WS_EX_TOPMOST) {
//		SetWindowPos(hWnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
//	}
//}
//
//
///**
// * @brief
// * �o�^�����C���L�[���������܂��B
// * 
// * @param hTargetWnd
// * �L�[�����Ώۂ̃E�B���h�E�n���h���B
// * 
// * �o�^�����C���L�[���������܂��B�ΏۃE�B�h�E�ɑ΂��Č��݃L�[��������Ă��邩�ǂ����͔��f���܂���B
// * ���f�͔h���N���X�ōs���܂��B
// * 
// * @remarks
// * �L�[�����͉��x�s���Ă����܂��܂��񂪁A�Ō�ɂ�ModKeyUp()�ŃL�[��������������K�v������܂�
// * [�����֐��̌Ăяo����1��ł��܂�Ȃ�]�B
// * 
// * @see
// * ModKeyUp()
// */
//void I4C3DControl::ModKeyDown(HWND hTargetWnd)
//{
//	DWORD dwBuf = 0;
//	HWND hForeground = GetForegroundWindow();
//	// TODO Ver.2 �ł�SetForeground�����A�ȉ��Ŕ��f����B
//	//if (hForeground != m_hTargetParentWnd || IsIconic(hForeground)) {
//	//	return;
//	//}
//	DWORD dwThreadId = GetWindowThreadProcessId(hForeground, NULL);
//	DWORD dwTargetThreadId = GetWindowThreadProcessId(hTargetWnd, NULL);
//
//	AttachThreadInput(dwTargetThreadId, dwThreadId, TRUE);
//
//	SystemParametersInfo(SPI_GETFOREGROUNDLOCKTIMEOUT, 0, &dwBuf, 0);
//	SystemParametersInfo(SPI_SETFOREGROUNDLOCKTIMEOUT, 0, NULL, 0);
//
//	SetForegroundWindow(m_hTargetParentWnd);
//	Sleep(m_millisecSleepAfterKeyDown);
//
//	if (m_ctrl) {
//		VMVirtualKeyDown(hTargetWnd, VK_CONTROL, m_bUsePostMessageToSendKey);
//	}
//	if (m_alt) {
//		VMVirtualKeyDown(hTargetWnd, VK_MENU, m_bUsePostMessageToSendKey);
//	}
//	if (m_shift) {
//		VMVirtualKeyDown(hTargetWnd, VK_SHIFT, m_bUsePostMessageToSendKey);
//	}
//	SystemParametersInfo(SPI_SETFOREGROUNDLOCKTIMEOUT, 0, &dwBuf, 0);
//	AttachThreadInput(dwThreadId, dwTargetThreadId, FALSE);
//
//	m_bSyskeyDown = TRUE;
//}
//
///**
// * @brief
// * �o�^�����C���L�[�̉������������܂��B
// * 
// * @param hTargetWnd
// * �L�[���������Ώۂ̃E�B���h�E�n���h���B
// * 
// * �o�^�����C���L�[�̉������������܂��B
// * 
// * @remarks
// * ModKeyDown()�ŃL�[�������s�����ꍇ�ɂ́A�Ō�ɂ͖{�֐��ŃL�[��������������K�v������܂�
// * [�����֐��̌Ăяo����1��ł��܂�Ȃ�]�B
// * 
// * @see
// * ModKeyDown()
// */
//void I4C3DControl::ModKeyUp(HWND hTargetWnd)
//{
//	DEBUG_PROFILE_MONITOR;
//	SetForegroundWindow(m_hTargetParentWnd);
//	DEBUG_PROFILE_MONITOR;
//
//	if (m_mouseMessage.dragButton != DragNONE) {
//		VMMouseClick(&m_mouseMessage, TRUE);
//		m_mouseMessage.dragButton = DragNONE;
//	}
//
//	if (m_shift) {
//		DEBUG_PROFILE_MONITOR;
//		VMVirtualKeyUp(hTargetWnd, VK_SHIFT);
//		DEBUG_PROFILE_MONITOR;
//	}
//	if (m_alt) {
//		DEBUG_PROFILE_MONITOR;
//		VMVirtualKeyUp(hTargetWnd, VK_MENU);
//		DEBUG_PROFILE_MONITOR;
//	}
//	if (m_ctrl) {
//		DEBUG_PROFILE_MONITOR;
//		VMVirtualKeyUp(hTargetWnd, VK_CONTROL);
//		DEBUG_PROFILE_MONITOR;
//	}
//	m_bSyskeyDown = FALSE;
//}
//
//
///**
// * @brief
// * �V���[�g�J�b�g���j���[�����s���܂��B
// * 
// * @param pContext
// * I4C3D���W���[���̃R���e�L�X�g�̃|�C���^�B
// * 
// * @param szCommand
// * �ݒ�t�@�C���Ɏw�肵���V���[�g�J�b�g���j���[���B
// * ��F<key name="perspective">F8</key>
// * ���̏ꍇ�A"perspective"���w��B
// * 
// * �V���[�g�J�b�g���j���[�����s���܂��B
// * �V���[�g�J�b�g���ƃV���[�g�J�b�g�L�[�Ƃ̑Ή��͐ݒ�t�@�C���ɋL�ڂ���K�v������܂��B
// * 
// * @remarks
// * private��HotkeyExecute()�ŉ�͂��R�}���h�̑��M���s���܂��B
// * 
// * @see
// * HotkeyExecute()
// */
//void I4C3DControl::HotkeyExecute(I4C3DContext* pContext, PCTSTR szCommand)
//{
//	HotkeyExecute(pContext, m_hTargetParentWnd, szCommand);
//}
//
///**
// * @brief
// * �V���[�g�J�b�g���j���[�����s���܂��B
// * 
// * @param pContext
// * I4C3D���W���[���̃R���e�L�X�g�̃|�C���^�B
// * 
// * @param hTargetWnd
// * �Ώۂ̃E�B���h�E�n���h���B
// * 
// * @param szCommand
// * �ݒ�t�@�C���Ɏw�肵���V���[�g�J�b�g���j���[���B
// * 
// * �V���[�g�J�b�g���j���[�����s���܂��B
// * 
// * @remarks
// * ���ۂ̃L�[���M��VKPush()�ōs���܂��B
// * 
// * @see
// * VKPush()
// */
//void I4C3DControl::HotkeyExecute(I4C3DContext* pContext, HWND hTargetWnd, PCTSTR szCommand)
//{
//	PCTSTR szCommandValue = pContext->pAnalyzer->GetSoftValue(szCommand);
//	if (szCommandValue != NULL) {
//		DWORD dwBuf = 0;
//		HWND hForeground = GetForegroundWindow();
//		DWORD dwThreadId = GetWindowThreadProcessId(hForeground, NULL);
//		DWORD dwTargetThreadId = GetWindowThreadProcessId(hTargetWnd, NULL);
//
//		AttachThreadInput(dwTargetThreadId, dwThreadId, TRUE);
//
//		SystemParametersInfo(SPI_GETFOREGROUNDLOCKTIMEOUT, 0, &dwBuf, 0);
//		SystemParametersInfo(SPI_SETFOREGROUNDLOCKTIMEOUT, 0, NULL, 0);
//
//		SetForegroundWindow(hTargetWnd);
//
//		VKPush(hTargetWnd, szCommandValue, m_bUsePostMessageToSendKey);
//		Sleep(5);
//
//		SystemParametersInfo(SPI_SETFOREGROUNDLOCKTIMEOUT, 0, &dwBuf, 0);
//		AttachThreadInput(dwThreadId, dwTargetThreadId, FALSE);
//	}
//}
//
///////////////////////////////////////////////////////////////////////////
//
//void I4C3DControl::AdjustCursorPOS(PPOINT pPos)
//{
//	if (pPos == NULL) {
//		return;
//	}
//
//	if (pPos->x < 0) {
//		{
//			TCHAR szDebug[I4C3D_BUFFER_SIZE];
//			_stprintf_s(szDebug, _countof(szDebug), _T("�͂ݏo��: pPos->x = %d -> 0"), pPos->x);
//			I4C3DMisc::LogDebugMessage(Log_Debug, szDebug);
//		}
//		pPos->x = 0;
//	} else if (m_DisplayWidth < pPos->x) {
//		{
//			TCHAR szDebug[I4C3D_BUFFER_SIZE];
//			_stprintf_s(szDebug, _countof(szDebug), _T("�͂ݏo��: pPos->x = %d -> %d"), pPos->x, m_DisplayWidth);
//			I4C3DMisc::LogDebugMessage(Log_Debug, szDebug);
//		}
//		pPos->x = m_DisplayWidth;
//	}
//
//	if (pPos->y < 0) {
//		{
//			TCHAR szDebug[I4C3D_BUFFER_SIZE];
//			_stprintf_s(szDebug, _countof(szDebug), _T("�͂ݏo��: pPos->y = %d -> 0"), pPos->y);
//			I4C3DMisc::LogDebugMessage(Log_Debug, szDebug);
//		}
//		pPos->y = 0;
//	} else if (m_DisplayHeight < pPos->y) {
//		{
//			TCHAR szDebug[I4C3D_BUFFER_SIZE];
//			_stprintf_s(szDebug, _countof(szDebug), _T("�͂ݏo��: pPos->y = %d -> %d"), pPos->y, m_DisplayHeight);
//			I4C3DMisc::LogDebugMessage(Log_Debug, szDebug);
//		}
//		pPos->y = m_DisplayHeight;
//	}
//}
//
///**
// * @brief
// * �Ώۃ^�[�Q�b�g�̈ʒu�����`�F�b�N���܂��B
// * 
// * @returns
// * �Ώۃ^�[�Q�b�g��NULL�̏ꍇ��FALSE�A����ȊO�ł̓E�B���h�E�̈ʒu���m�F������TRUE��Ԃ��܂��B
// * 
// * �Ώۃ^�[�Q�b�g�̈ʒu�����`�F�b�N���܂��B
// */
//BOOL I4C3DControl::CheckTargetState(void)
//{
//	if (m_hTargetParentWnd == NULL) {
//		I4C3DMisc::ReportError(_T("�^�[�Q�b�g�E�B���h�E���擾�ł��܂���B<I4C3DControl::CheckTargetState>"));
//		I4C3DMisc::LogDebugMessage(Log_Error, _T("�^�[�Q�b�g�E�B���h�E���擾�ł��܂���B<I4C3DControl::CheckTargetState>"));
//
//	} else if (m_hTargetChildWnd == NULL) {
//		I4C3DMisc::LogDebugMessage(Log_Error, _T("�q�E�B���h�E���擾�ł��܂���B<I4C3DControl::CheckTargetState>"));
//
//	} else {
//		//// �^�[�Q�b�g�E�B���h�E�̈ʒu�̃`�F�b�N
//		////GetWindowRect(m_hTargetParentWnd, &rect);
//		//GetClientRect(m_hTargetParentWnd, &rect);
//		//m_currentPos.x = rect.left + (rect.right - rect.left) / 2;
//		//m_currentPos.y = rect.top + (rect.bottom - rect.top) / 2;
//
//		// �^�[�Q�b�g�E�B���h�E�̈ʒu�̃`�F�b�N
//		POINT tmpCurrentPos = m_currentPos;
//		ClientToScreen(m_hTargetChildWnd, &tmpCurrentPos);
//		if (WindowFromPoint(tmpCurrentPos) != m_hTargetChildWnd ||
//			tmpCurrentPos.x < 0 || m_DisplayWidth < tmpCurrentPos.x ||
//			tmpCurrentPos.y < 0 || m_DisplayHeight < tmpCurrentPos.y) {
//				if (m_mouseMessage.dragButton != DragNONE) {
//					VMMouseClick(&m_mouseMessage, TRUE);
//					m_mouseMessage.dragButton = DragNONE;
//					//OutputDebugString(L"--------------------------------------------------------------\n");
//				}
//
//				RECT rect;
//				//GetClientRect(m_hTargetParentWnd, &rect);
//				GetClientRect(m_hTargetChildWnd, &rect);
//				//OutputDebugString(L"Reset('0')/Reset('0')/Reset('0')/Reset('0')/Reset('0')/Reset('0')/\n");
//				m_currentPos.x = rect.left + (rect.right - rect.left) / 2;
//				m_currentPos.y = rect.top + (rect.bottom - rect.top) / 2;
//		}
//		return TRUE;
//	}
//
//	return FALSE;
//}
//
//
///**
// * @brief
// * �V���[�g�J�b�g���j���[�̎��s���s���܂��B
// * 
// * @param hTargetWnd
// * �Ώۂ̃E�B���h�E�n���h���B
// * 
// * @param szKeyTypes
// * �V���[�g�J�b�g�L�[[�̑g�ݍ��킹]�B
// * 
// * @param m_bUsePostMessage
// * �L�[���M��PostMessage���g�p����ꍇ[RTT�n]�ɂ�TRUE�ASendInput���g�p����ꍇ[AutoDesk�n]�ɂ�FALSE���w�肵�܂��B
// * 
// * �V���[�g�J�b�g���j���[�̎��s���s���܂��B
// * �V���[�g�J�b�g�L�[�̉�͂��ċA�I�ɍs���Ȃ�����s���܂��B
// * 
// * @remarks
// * HotKeyExecute()����Ăяo����܂��B
// * 
// * @see
// * HotKeyExecute()
// */
//void VKPush(HWND hTargetWnd, PCTSTR szKeyTypes, BOOL bUsePostMessage) {
//	PCTSTR pType = _tcschr(szKeyTypes, _T('+'));
//	TCHAR szKey[I4C3D_BUFFER_SIZE] = {0};
//	if (pType != NULL) {
//		_tcsncpy_s(szKey, _countof(szKey), szKeyTypes, pType-szKeyTypes+1);
//	} else {
//		_tcscpy_s(szKey, _countof(szKey), szKeyTypes);
//	}
//	CharUpper(szKey);
//	I4C3DMisc::RemoveWhiteSpace(szKey);
//	UINT vKey = VMGetVirtualKey(szKey);
//
//	// �L�[�_�E��
//	switch (vKey) {
//	case 0:
//		VMVirtualKeyDown(hTargetWnd, szKey[0], bUsePostMessage);
//		break;
//
//	case VK_CONTROL:
//	case VK_MENU:
//	case VK_SHIFT:
//		VMVirtualKeyDown(hTargetWnd, vKey, bUsePostMessage);
//		//Sleep(TIME_WAIT);
//		break;
//
//	default:
//		VMVirtualKeyDown(hTargetWnd, vKey, bUsePostMessage);
//	}
//
//	// �ċA�I�Ɏ��̃L�[����
//	if (pType != NULL) {
//		VKPush(hTargetWnd, pType+1, bUsePostMessage);
//	}
//
//	// �L�[�A�b�v
//	switch (vKey) {
//	case 0:
//		VMVirtualKeyUp(hTargetWnd, szKey[0]);
//		break;
//
//	case VK_CONTROL:
//	case VK_MENU:
//	case VK_SHIFT:
//		VMVirtualKeyUp(hTargetWnd, vKey);
//		//Sleep(TIME_WAIT);
//		break;
//
//	default:
//		VMVirtualKeyUp(hTargetWnd, vKey);
//	}
//}
//
