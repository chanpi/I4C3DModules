// I4C3DModules.cpp : DLL �A�v���P�[�V�����p�ɃG�N�X�|�[�g�����֐����`���܂��B
//

#include "stdafx.h"
#include "I4C3DModules.h"
#include "I4C3DCore.h"
#include "I4C3DAnalyzeXML.h"
#include "I4C3DControl.h"
#include "Misc.h"
#include "SharedConstants.h"

#if UNICODE || _UNICODE
static LPCTSTR g_FILE = __FILEW__;
#else
static LPCTSTR g_FILE = __FILE__;
#endif

typedef BOOL (__stdcall *FUNCTYPE)(UINT, DWORD);
#define MSGFLT_ADD		1
#define MSGFLT_REMOVE	2

static I4C3DContext g_Context = {0};
static I4C3DCore g_Core;
static BOOL g_bStarted = FALSE;
static HWND g_hDriverWnd = NULL;

static BOOL PrepareTargetController(char cTermination);
static void DestroyTargetController();
static const PCTSTR TAG_LOG			= _T("log");
static const PCTSTR TAG_OFF			= _T("off");
static const PCTSTR TAG_DEBUG		= _T("debug");
static const PCTSTR TAG_INFO		= _T("info");
static const PCTSTR TAG_TERMINATION	= _T("termination");

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
BOOL WINAPI I4C3DStart(PCTSTR szXMLUri, HWND hWnd)
{
	if (g_bStarted) {
		return TRUE;
	}

	g_hDriverWnd = hWnd;

	// �K�v�ȏ�����
	ZeroMemory(&g_Core, sizeof(g_Core));
	ZeroMemory(&g_Context, sizeof(g_Context));
	g_Context.pAnalyzer = new I4C3DAnalyzeXML();
	if (g_Context.pAnalyzer == NULL) {
		LoggingMessage(Log_Error, _T(MESSAGE_ERROR_MEMORY_INVALID), GetLastError(), g_FILE, __LINE__);
		I4C3DStop();
		I4C3DExit(EXIT_SYSTEM_ERROR);
	}
	if (!g_Context.pAnalyzer->LoadXML(szXMLUri)) {
		LoggingMessage(Log_Error, _T(MESSAGE_ERROR_XML_LOAD), GetLastError(), g_FILE, __LINE__);
		I4C3DStop();
		I4C3DExit(EXIT_INVALID_FILE_CONFIGURATION);
	}

	// �ݒ�t�@�C�����烍�O���x�����擾
	LOG_LEVEL logLevel = Log_Error;
	PCTSTR szLogLevel = g_Context.pAnalyzer->GetGlobalValue(TAG_LOG);
	if (szLogLevel != NULL) {
		if (!_tcsicmp(szLogLevel, TAG_OFF)) {
			logLevel = Log_Off;
		} else if (!_tcsicmp(szLogLevel, TAG_DEBUG)) {
			logLevel = Log_Debug;
		} else if (!_tcsicmp(szLogLevel, TAG_INFO)) {
			logLevel = Log_Info;
		}
		// �w��Ȃ��A��L�ȊO�Ȃ�Log_Error
	}
	LogFileOpenW(SHARED_LOG_FILE_NAME, logLevel);
	if (logLevel <= Log_Info) {
		LogFileOpenA(SHARED_LOGINFO_FILE_NAME, logLevel);	// �v���t�@�C����񏑂��o���̂���
	}
	
	// �ݒ�t�@�C������I�[�������擾
	char cTermination = '?';
	PCTSTR szTermination = g_Context.pAnalyzer->GetGlobalValue(TAG_TERMINATION);
	if (szTermination != NULL) {
		char cszTermination[5];
		if (_tcslen(szTermination) != 1) {
			LoggingMessage(Log_Error, _T(MESSAGE_ERROR_CFG_TERMCHAR), GetLastError(), g_FILE, __LINE__);
			szTermination = _T("?");
		}
#if UNICODE || _UNICODE
		WideCharToMultiByte(CP_ACP, 0, szTermination, _tcslen(szTermination), cszTermination, sizeof(cszTermination), NULL, NULL);
#else
		strcpy_s(cszTermination, sizeof(cszTermination), szTermination);
#endif
		RemoveWhiteSpaceA(cszTermination);
		cTermination = cszTermination[0];
	}

	if (!PrepareTargetController(cTermination)) {
		I4C3DStop();
		I4C3DExit(EXIT_SYSTEM_ERROR);
	}

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
	DestroyTargetController();
	g_Context.bIsAlive = FALSE;
	g_bStarted = FALSE;

	LogFileCloseW();
	LogFileCloseA();	// Open���Ă��Ȃ��Ƃ��͉�������TRUE���Ԃ�
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
BOOL PrepareTargetController(char cTermination)
{
	if (g_Context.pController != NULL) {
		delete g_Context.pController;
		g_Context.pController = NULL;
	}
	g_Context.pController = new I4C3DControl;

	// ������
	if (g_Context.pController == NULL || !g_Context.pController->Initialize(&g_Context, cTermination)) {
		LoggingMessage(Log_Error, _T(MESSAGE_ERROR_SYSTEM_INIT), GetLastError(), g_FILE, __LINE__);
		return FALSE;
	}

	return TRUE;
}

void DestroyTargetController()
{
	if (g_Context.pController != NULL) {
		g_Context.pController->UnInitialize();
	}
}

void I4C3DExit(int errorCode)
{
	FUNCTYPE ChangeWindowMessageFilter;
	HMODULE dll = LoadLibrary(_T("user32.dll"));
	ChangeWindowMessageFilter = (FUNCTYPE)GetProcAddress(LoadLibrary(_T("user32.dll")), "ChangeWindowMessageFilter");

	if (ChangeWindowMessageFilter != NULL) {
		ChangeWindowMessageFilter(MY_I4C3DEXIT, MSGFLT_ADD);
	}

	LoggingMessage(Log_Error, _T(MESSAGE_ERROR_UNKNOWN), GetLastError(), g_FILE, __LINE__);
	//PostQuitMessage(errorCode);
	PostMessage(g_hDriverWnd, MY_I4C3DEXIT, 0, errorCode);
}