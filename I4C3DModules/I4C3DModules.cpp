// I4C3DModules.cpp : DLL �A�v���P�[�V�����p�ɃG�N�X�|�[�g�����֐����`���܂��B
//

#include "stdafx.h"
#include "I4C3DModules.h"
#include "I4C3DMisc.h"
#include "I4C3DCore.h"
#include "I4C3DAnalyzeXML.h"

#include "I4C3DRTTControl.h"
#include "I4C3DMAYAControl.h"
#include "I4C3DAliasControl.h"
#include "I4C3DSHOWCASEControl.h"

static PCTSTR TAG_WINDOWTITLE	= _T("window_title");
static PCTSTR g_szRTT			= _T("RTT");
static PCTSTR g_szMAYA		= _T("MAYA");
static PCTSTR g_szAlias		= _T("Alias");
static PCTSTR g_szSHOWCASE	= _T("SHOWCASE");

static I4C3DContext g_Context = {0};
static I4C3DCore g_Core;
static BOOL g_bStarted = FALSE;

static HWND g_targetWindow = NULL;
static BOOL CALLBACK EnumWindowProc(HWND hWnd, LPARAM lParam);

static BOOL SelectTarget(PCTSTR szTargetWindowTitle);
static HWND GetTarget3DSoftwareWnd(PCTSTR szTargetWindowTitle);
static BOOL SelectTargetController(PCTSTR szTargetSoftware);

/**
 * @brief
 * I4C3D���W���[�����J�n���܂��B
 * �O���̃A�v���P�[�V�����͂��̊֐����ĂԂ��ƂŃ��W���[�����J�n�ł��܂��B
 * 
 * @param szXMLUri
 * �ݒ�t�@�C����XML�ւ̃p�X�B
 * 
 * @param szTargetWindowTitle
 * Description of parameter szTargetWindowTitle.
 * 
 * @returns
 * Write description of return value here.
 * 
 * Write detailed description for I4C3DStart here.
 * 
 * @remarks
 * �v���O�����I�����ɂ͕K��I4C3DStop()���Ă�ł��������B
 * 
 * @see
 * I4C3DStop()
 */
BOOL WINAPI I4C3DStart(PCTSTR szXMLUri, PCTSTR szTargetWindowTitle)
{
	if (g_bStarted) {
		return TRUE;
	}
	// �K�v�ȏ�����
	ZeroMemory(&g_Core, sizeof(g_Core));
	ZeroMemory(&g_Context, sizeof(g_Context));
	g_Context.pAnalyzer = new I4C3DAnalyzeXML();
	if (g_Context.pAnalyzer == NULL) {
		I4C3DMisc::ReportError(_T("[ERROR] �������̊m�ۂɎ��s���Ă��܂��B�������͍s���܂���B"));
		I4C3DStop();
		return FALSE;
	}
	if (!g_Context.pAnalyzer->LoadXML(szXMLUri)) {
		I4C3DMisc::ReportError(_T("[ERROR] XML�̃��[�h�Ɏ��s���Ă��܂��B�������͍s���܂���B"));
		I4C3DStop();
		return FALSE;
	}
	I4C3DMisc::LogInitialize(g_Context.pAnalyzer);
	if (!SelectTarget(szTargetWindowTitle)) {
		I4C3DMisc::ReportError(_T("[ERROR] �^�[�Q�b�g�E�B���h�E���擾�ł��܂���B�������͍s���܂���B"));
		I4C3DStop();
		return FALSE;
	}

	g_Context.bIsAlive = FALSE;
	g_bStarted = g_Core.Start(&g_Context);
	if (!g_bStarted) {
		I4C3DStop();
	}
	return g_bStarted;
}

/**
 * @brief
 * I4C3D���W���[�����I�����܂��B
 * 
 * I4C3D���W���[�����I�����܂��B
 * �v���O�����̏I���ɂ͕K���Ăяo���Ă��������i���������[�N�ɂȂ���܂��j�B
 * 
 * @remarks
 * I4C3DStart()�ƑΉ����Ă��܂��B
 * 
 * @see
 * I4C3DStart()
 */
void WINAPI I4C3DStop(void)
{
	g_Core.Stop(&g_Context);
	if (g_Context.pAnalyzer != NULL) {
		delete g_Context.pAnalyzer;
		g_Context.pAnalyzer = NULL;
	}
	if (g_Context.pController != NULL) {
		delete g_Context.pController;
		g_Context.pController = NULL;
	}
	g_bStarted = FALSE;
	g_targetWindow = NULL;
}


/////////////////////////////////////////////////

/**
 * @brief
 * �Ώۂ�3D�\�t�g�E�F�A�̎擾�ƁA�E�B���h�E�n���h���̎擾���s���܂��B
 * 
 * @param szTargetWindowTitle
 * �w��̃E�B���h�E�^�C�g��������ꍇ�ɂ͂����Ɏw�肵�Ă��������i�^�C�g���̈ꕔ�ł����Ă��A��ʂł���Ȃ�\�j�B
 * NULL�̏ꍇ�ɂ͐ݒ�t�@�C����"window_title"�̒l����擾�����݂܂��B
 * 
 * @returns
 * �ΏۃE�B���h�E�̎擾�ɐ��������ꍇ�ɂ�TRUE�A���s�����ꍇ�ɂ�FALSE��Ԃ��܂��B
 * 
 * �Ώۂ�3D�\�t�g�E�F�A�̎擾�ƁA�E�B���h�E�n���h���̎擾���s���܂��B
 * 
 * @remarks
 * ���ۂɎ擾���s���̂�GetTarget3DSoftwareWnd()�ł��B
 * 
 * @see
 * GetTarget3DSoftwareWnd()
 */
BOOL SelectTarget(PCTSTR szTargetWindowTitle)
{
	// �^�[�Q�b�g�\�t�g�̎擾
	PCTSTR szTargetSoftware = g_Context.pAnalyzer->GetGlobalValue(TAG_TARGET);
	if (!szTargetSoftware) {
		I4C3DMisc::LogDebugMessage(Log_Error, _T("�ݒ�t�@�C���Ƀ^�[�Q�b�g�\�t�g���w�肵�Ă��������B<I4C3DModules::SelectTarget>"));
		return FALSE;
	}

	// �E�B���h�E�^�C�g���̎w�肪�Ȃ��ꍇ�ɂ͐ݒ�t�@�C����"window_title"�^�O����擾
	if (szTargetWindowTitle == NULL) {
		szTargetWindowTitle = g_Context.pAnalyzer->GetSoftValue(TAG_WINDOWTITLE);
	}

	if (szTargetWindowTitle != NULL) {
		g_Context.hTargetParentWnd = GetTarget3DSoftwareWnd(szTargetWindowTitle);
	} else {
		I4C3DMisc::LogDebugMessage(Log_Error, _T("�ݒ�t�@�C���������̓p�����[�^�Ƀ^�[�Q�b�g�E�B���h�E�����w�肵�Ă��������B<I4C3DModules::SelectTarget>"));
		return FALSE;
		//I4C3DMisc::LogDebugMessage(Log_Debug, _T("window_title��NULL�̂��߁A�^�[�Q�b�g���Ō������܂��B<I4C3DModules::SelectTarget>"));
		//g_Context.hTargetParentWnd = GetTarget3DSoftwareWnd(szTargetSoftware);
	}

	if (g_Context.hTargetParentWnd != NULL) {
		if (SelectTargetController(szTargetSoftware)) {
			return TRUE;
		}
	}

	return FALSE;
}

/**
 * @brief
 * �Ώۂ̃E�B���h�E�n���h�����擾���܂��B
 * 
 * @param szTargetWindowTitle
 * �ΏۃE�B���h�E�̃^�C�g���i�̈ꕔ�j�B
 * 
 * @returns
 * �擾�ł����ꍇ�ɂ̓E�B���h�E�n���h���A���s�����ꍇ�ɂ�NULL���Ԃ�܂��B
 * 
 * �Ώۂ̃E�B���h�E�n���h�����擾���܂��B
 * 
 * @remarks
 * �E�B���h�E�^�C�g���ɏd�����N����ꍇ������A���̏ꍇ�͌�����E�B���h�E�n���h�����擾����邱�Ƃ��\�z����܂��B
 * �E�B���h�E�^�C�g���͂ł��邾���ڍׂɎw�肷��K�v������܂��B
 */
HWND GetTarget3DSoftwareWnd(PCTSTR szTargetWindowTitle)
{
	if (szTargetWindowTitle == NULL) {
		I4C3DMisc::LogDebugMessage(Log_Error, _T("szTargetWindowTitle is NULL. <I4C3DModules::GetTarget3DSoftwareWnd>"));
		return NULL;
	}
	EnumWindows(&EnumWindowProc, (LPARAM)szTargetWindowTitle);
	return g_targetWindow;
}

/**
 * @brief
 * 3D�\�t�g�̃R���g���[���𐶐����܂��B
 * 
 * @param szTargetSoftware
 * �Ώۂ�3D�\�t�g�����w�肵�܂��B
 * �ݒ�t�@�C����soft���ƈ�v�����Ă����K�v������܂��i�啶���������̋�ʂȂ��j�B
 * 
 * @returns
 * �o�^���ꂽ3D�\�t�g�̏ꍇ�ɂ́A�R���g���[���𐶐����ATRUE��Ԃ��܂��B
 * �Y���̂��̂��Ȃ��ꍇ�ɂ�FALSE��Ԃ��܂��B
 * 
 * 3D�\�t�g�̃R���g���[���𐶐����܂��B������ɂ���ă^�[�Q�b�g�𔻕ʂ��Ă��܂��B
 * 
 * @remarks
 * ���I�ɃE�B���h�E�^�C�g�������w�肵�ăv���O�������J�n����ꍇ�ɂ́A
 * �R���g���[���̐������@�ɂ����ӂ��K�v�ł�
 * �i���I�Ɏ擾�����E�B���h�E�^�C�g������ǂ�3D�\�t�g�E�F�A[�ǂ�Controller]���𔻒f����K�v������j�B
 * 
 */
BOOL SelectTargetController(PCTSTR szTargetSoftware)
{
	if (g_Context.pController != NULL) {
		delete g_Context.pController;
		g_Context.pController = NULL;
	}
	if (!_tcsicmp(szTargetSoftware, g_szRTT)) {
		g_Context.pController = new I4C3DRTTControl;
		I4C3DMisc::LogDebugMessage(Log_Debug, _T("new I4C3DRTTControl <I4C3DModules::SelectTargetController>"));

	} else if (!_tcsicmp(szTargetSoftware, g_szMAYA)) {
		g_Context.pController = new I4C3DMAYAControl;
		I4C3DMisc::LogDebugMessage(Log_Debug, _T("new I4C3DMAYAControl <I4C3DModules::SelectTargetController>"));

	} else if (!_tcsicmp(szTargetSoftware, g_szAlias)) {
		g_Context.pController = new I4C3DAliasControl;
		I4C3DMisc::LogDebugMessage(Log_Debug, _T("new I4C3DAliasControl <I4C3DModules::SelectTargetController>"));

	} else if (!_tcsicmp(szTargetSoftware, g_szSHOWCASE)) {
		g_Context.pController = new I4C3DSHOWCASEControl;
		I4C3DMisc::LogDebugMessage(Log_Debug, _T("new I4C3DSHOWCASEControl <I4C3DModules::SelectTargetController>"));

	} else {
		I4C3DMisc::LogDebugMessage(Log_Error, _T("�s���ȃ^�[�Q�b�g�\�t�g���w�肳��܂����B<I4C3DModules::SelectTargetController>"));
		return FALSE;
	}

	// ������
	if (g_Context.pController == NULL || !g_Context.pController->Initialize(&g_Context)) {
		I4C3DMisc::LogDebugMessage(Log_Error, _T("�R���g���[���̏������Ɏ��s���Ă��܂��B<I4C3DModules::SelectTargetController>"));
		return FALSE;
	}

	return TRUE;
}

BOOL CALLBACK EnumWindowProc(HWND hWnd, LPARAM lParam)
{
	TCHAR szTitle[MAX_PATH] = {0};
	GetWindowText(hWnd, szTitle, _countof(szTitle));
	if (_tcsstr(szTitle, (PCTSTR)lParam) != NULL) {
		g_targetWindow = hWnd;
		I4C3DMisc::LogDebugMessage(Log_Debug, szTitle);
		return FALSE;	// EnumWindow�𒆒f
	}
	return TRUE;		// EnumWindow���p��
}