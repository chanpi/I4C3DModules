#include "StdAfx.h"
#include "I4C3DControl.h"
#include "Miscellaneous.h"
#include "I4C3DAnalyzeXML.h"
#include "I4C3DSoftwareHandler.h"
#include <vector>
using namespace std;

//static void VKPush(HWND hTargetWnd, PCTSTR szKeyTypes, BOOL bUsePostMessage);
static const PCTSTR TAG_MODIFIER	= _T("modifier");
static const PCTSTR TAG_UDP_PORT	= _T("udp_port");
static const PCTSTR TAG_TUMBLERATE	= _T("tumble_rate");
static const PCTSTR TAG_TRACKRATE	= _T("track_rate");
static const PCTSTR TAG_DOLLYRATE	= _T("dolly_rate");

static TCHAR *g_szController[] = { _T("RTT"), _T("Maya"), _T("Alias"), _T("Showcase"), };
static vector<I4C3DSoftwareHandler*> *g_pSoftwareHandlerContainer = NULL;

static CRITICAL_SECTION g_lock;

I4C3DControl::I4C3DControl(void) : m_bInitialized(FALSE)
{
	g_pSoftwareHandlerContainer = new vector<I4C3DSoftwareHandler*>;
	InitializeCriticalSection(&g_lock);
}

I4C3DControl::~I4C3DControl(void)
{
	UnInitialize();
	delete g_pSoftwareHandlerContainer;
	g_pSoftwareHandlerContainer = NULL;

	DeleteCriticalSection(&g_lock);
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
 * �C���L�[�E�g�嗦�E�I�[������ݒ�t�@�C������擾���A�e3D�\�t�g�v���O�C�����W���[����init���b�Z�[�W�𑗐M���܂��B
 * 
 * @param pContext
 * I4C3D���W���[���̃R���e�L�X�g�̃|�C���^�B
 * 
 * @returns
 * �������ɐ��������ꍇ�ɂ�TRUE�A���s�����ꍇ�ɂ�FALSE��Ԃ��܂��B
 * 
 * �C���L�[�E�g�嗦�E�I�[������ݒ�t�@�C������擾���A�e3D�\�t�g�v���O�C�����W���[����UDP��init���b�Z�[�W�𑗐M���܂��B
 */
BOOL I4C3DControl::Initialize(I4C3DContext* pContext, char cTermination)
{
	if (m_bInitialized) {
		return TRUE;
	}

	TCHAR szError[I4C3D_BUFFER_SIZE] = {0};
	int count = _countof(g_szController);
	for (int i = 0; i < count; ++i) {
		PCTSTR szModifierKey = NULL;
		char cszModifierKey[I4C3D_BUFFER_SIZE] = {0};
		double fTumbleRate = 1.0;
		double fTrackRate = 1.0;
		double fDollyRate = 1.0;
		I4C3DUDPPacket packet = {0};

		// �|�[�g�ԍ�
		USHORT uPort = 10001;
		PCTSTR szPort = pContext->pAnalyzer->GetSoftValue(g_szController[i], TAG_UDP_PORT);
		if (szPort == NULL) {
			uPort += (USHORT)i;
			_stprintf_s(szError, _countof(szError), _T("�ݒ�t�@�C����%s��UDP�|�[�g���w�肵�Ă��������B����%u�ɐݒ肵�܂��B<I4C3DControl::Initialize>"), g_szController[i], uPort);
			LogDebugMessage(Log_Error, szError);
		} else {
			uPort = (USHORT)_tstoi(szPort);
		}

		// �n���h���̏�����
		I4C3DSoftwareHandler* pHandler = new I4C3DSoftwareHandler(g_szController[i], uPort);
		if (!pHandler->PrepareSocket()) {
			_stprintf_s(szError, _countof(szError), _T("%s�̊Ǘ��N���X[�\�P�b�g]�̍쐬�Ɏ��s���Ă��܂��B%s�ւ̑���͍s���܂���B"), g_szController[i], g_szController[i]);
			LogDebugMessage(Log_Error, szError);
			delete pHandler;
			continue;
		}

		// �n���h����vector�ŊǗ�
		g_pSoftwareHandlerContainer->push_back(pHandler);

		// �C���L�[
		szModifierKey = pContext->pAnalyzer->GetSoftValue(g_szController[i], TAG_MODIFIER);
		if (szModifierKey == NULL) {
			LogDebugMessage(Log_Error, _T("�ݒ�t�@�C���ɏC���L�[��ݒ肵�Ă��������B<I4C3DControl::Initialize>"));
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

		// �ePlugin.exe�ւ̓d���쐬�E���M
		sprintf_s(packet.szCommand, g_initCommandFormat, "init", cszModifierKey, fTumbleRate, fTrackRate, fDollyRate, cTermination);

		EnterCriticalSection(&g_lock);
		sendto(pHandler->m_socketHandler, (const char*)&packet, strlen(packet.szCommand)+4, 0, (const SOCKADDR*)&pHandler->m_address, sizeof(pHandler->m_address));
		LeaveCriticalSection(&g_lock);
	}
	m_bInitialized = TRUE;
	return m_bInitialized;
}

/**
 * @brief
 * 3D�\�t�g�n���h���N���X�̏I�������ƍ폜���s���܂��B
 * 
 * 3D�\�t�g�n���h���N���X�̏I�������ƍ폜���s���܂��B�e�v���O�C�����W���[���ɏI�����b�Z�[�W�𑗐M���܂��B
 */
void I4C3DControl::UnInitialize(void)
{
	if (!m_bInitialized) {
		return;
	}

	if (g_pSoftwareHandlerContainer != NULL) {
		int size = g_pSoftwareHandlerContainer->size();
		for (int i = 0; i < size; ++i) {
			I4C3DUDPPacket packet = {0};
			strcpy_s(packet.szCommand, _countof(packet.szCommand), "exit");
			EnterCriticalSection(&g_lock);
			sendto((*g_pSoftwareHandlerContainer)[i]->m_socketHandler, (const char*)&packet, 8, 0, (const SOCKADDR*)&(*g_pSoftwareHandlerContainer)[i]->m_address, sizeof((*g_pSoftwareHandlerContainer)[i]->m_address));
			LeaveCriticalSection(&g_lock);

			closesocket((*g_pSoftwareHandlerContainer)[i]->m_socketHandler);
			delete (*g_pSoftwareHandlerContainer)[i];
			(*g_pSoftwareHandlerContainer)[i] = NULL;
		}
		g_pSoftwareHandlerContainer->clear();
	}
	m_bInitialized = FALSE;
}

/**
 * @brief
 * ipod����̓d����TOPMOST��3D�\�t�g�ɓ]�����܂��B
 * 
 * @param szCommand
 * ipod�����M�����d���B
 * 
 * @param commandLen
 * �d�����B
 * 
 * ipod����̓d����TOPMOST��3D�\�t�g�ɓ]�����܂��B�]����UDP�ōs���܂��B
 */
void I4C3DControl::Execute(I4C3DUDPPacket* pPacket, int commandLen)
{
	TCHAR szWindowTitle[I4C3D_BUFFER_SIZE] = {0};
	HWND hForeground = GetForegroundWindow();
	if (IsIconic(hForeground)) {
		return;
	}
	GetWindowText(hForeground, szWindowTitle, _countof(szWindowTitle));

	int size = g_pSoftwareHandlerContainer->size();
	for (int i = 0; i < size; ++i) {
		if (_tcsstr(szWindowTitle, (*g_pSoftwareHandlerContainer)[i]->m_szTargetTitle) != NULL) {
			// �E�B���h�E�n���h����t��
			if (hForeground == NULL) {
				return;
			}
			for (int j = 0; j < 4; ++j) {
				pPacket->hwnd[j] = ((unsigned char*)&hForeground)[j];
			}

			EnterCriticalSection(&g_lock);
			sendto((*g_pSoftwareHandlerContainer)[i]->m_socketHandler, (const char*)pPacket, commandLen+4, 0, (const SOCKADDR*)&((*g_pSoftwareHandlerContainer)[i]->m_address), sizeof((*g_pSoftwareHandlerContainer)[i]->m_address));
			LeaveCriticalSection(&g_lock);
			return;
		}
	}
}
