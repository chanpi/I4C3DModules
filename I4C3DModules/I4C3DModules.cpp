// I4C3DModules.cpp : DLL アプリケーション用にエクスポートされる関数を定義します。
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
 * I4C3Dモジュールを開始します。
 * 外部のアプリケーションはこの関数を呼ぶことでモジュールを開始できます。
 * 
 * @param szXMLUri
 * 設定ファイルのXMLへのパス。
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
 * プログラム終了時には必ずI4C3DStop()を呼んでください。
 * 
 * @see
 * I4C3DStop()
 */
BOOL WINAPI I4C3DStart(PCTSTR szXMLUri, PCTSTR szTargetWindowTitle)
{
	if (g_bStarted) {
		return TRUE;
	}
	// 必要な初期化
	ZeroMemory(&g_Core, sizeof(g_Core));
	ZeroMemory(&g_Context, sizeof(g_Context));
	g_Context.pAnalyzer = new I4C3DAnalyzeXML();
	if (g_Context.pAnalyzer == NULL) {
		I4C3DMisc::ReportError(_T("[ERROR] メモリの確保に失敗しています。初期化は行われません。"));
		I4C3DStop();
		return FALSE;
	}
	if (!g_Context.pAnalyzer->LoadXML(szXMLUri)) {
		I4C3DMisc::ReportError(_T("[ERROR] XMLのロードに失敗しています。初期化は行われません。"));
		I4C3DStop();
		return FALSE;
	}
	I4C3DMisc::LogInitialize(g_Context.pAnalyzer);
	if (!SelectTarget(szTargetWindowTitle)) {
		I4C3DMisc::ReportError(_T("[ERROR] ターゲットウィンドウが取得できません。初期化は行われません。"));
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
 * I4C3Dモジュールを終了します。
 * 
 * I4C3Dモジュールを終了します。
 * プログラムの終わりには必ず呼び出してください（メモリリークにつながります）。
 * 
 * @remarks
 * I4C3DStart()と対応しています。
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
 * 対象の3Dソフトウェアの取得と、ウィンドウハンドルの取得を行います。
 * 
 * @param szTargetWindowTitle
 * 指定のウィンドウタイトルがある場合にはここに指定してください（タイトルの一部であっても、区別できるなら可能）。
 * NULLの場合には設定ファイルの"window_title"の値から取得を試みます。
 * 
 * @returns
 * 対象ウィンドウの取得に成功した場合にはTRUE、失敗した場合にはFALSEを返します。
 * 
 * 対象の3Dソフトウェアの取得と、ウィンドウハンドルの取得を行います。
 * 
 * @remarks
 * 実際に取得を行うのはGetTarget3DSoftwareWnd()です。
 * 
 * @see
 * GetTarget3DSoftwareWnd()
 */
BOOL SelectTarget(PCTSTR szTargetWindowTitle)
{
	// ターゲットソフトの取得
	PCTSTR szTargetSoftware = g_Context.pAnalyzer->GetGlobalValue(TAG_TARGET);
	if (!szTargetSoftware) {
		I4C3DMisc::LogDebugMessage(Log_Error, _T("設定ファイルにターゲットソフトを指定してください。<I4C3DModules::SelectTarget>"));
		return FALSE;
	}

	// ウィンドウタイトルの指定がない場合には設定ファイルの"window_title"タグから取得
	if (szTargetWindowTitle == NULL) {
		szTargetWindowTitle = g_Context.pAnalyzer->GetSoftValue(TAG_WINDOWTITLE);
	}

	if (szTargetWindowTitle != NULL) {
		g_Context.hTargetParentWnd = GetTarget3DSoftwareWnd(szTargetWindowTitle);
	} else {
		I4C3DMisc::LogDebugMessage(Log_Error, _T("設定ファイルもしくはパラメータにターゲットウィンドウ名を指定してください。<I4C3DModules::SelectTarget>"));
		return FALSE;
		//I4C3DMisc::LogDebugMessage(Log_Debug, _T("window_titleがNULLのため、ターゲット名で検索します。<I4C3DModules::SelectTarget>"));
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
 * 対象のウィンドウハンドルを取得します。
 * 
 * @param szTargetWindowTitle
 * 対象ウィンドウのタイトル（の一部）。
 * 
 * @returns
 * 取得できた場合にはウィンドウハンドル、失敗した場合にはNULLが返ります。
 * 
 * 対象のウィンドウハンドルを取得します。
 * 
 * @remarks
 * ウィンドウタイトルに重複が起こる場合があり、その場合は誤ったウィンドウハンドルが取得されることが予想されます。
 * ウィンドウタイトルはできるだけ詳細に指定する必要があります。
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
 * 3Dソフトのコントローラを生成します。
 * 
 * @param szTargetSoftware
 * 対象の3Dソフト名を指定します。
 * 設定ファイルのsoft名と一致させておく必要があります（大文字小文字の区別なし）。
 * 
 * @returns
 * 登録された3Dソフトの場合には、コントローラを生成し、TRUEを返します。
 * 該当のものがない場合にはFALSEを返します。
 * 
 * 3Dソフトのコントローラを生成します。文字列によってターゲットを判別しています。
 * 
 * @remarks
 * 動的にウィンドウタイトル名を指定してプログラムを開始する場合には、
 * コントローラの生成方法にも注意が必要です
 * （動的に取得したウィンドウタイトルからどの3Dソフトウェア[どのController]かを判断する必要がある）。
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
		I4C3DMisc::LogDebugMessage(Log_Error, _T("不明なターゲットソフトが指定されました。<I4C3DModules::SelectTargetController>"));
		return FALSE;
	}

	// 初期化
	if (g_Context.pController == NULL || !g_Context.pController->Initialize(&g_Context)) {
		I4C3DMisc::LogDebugMessage(Log_Error, _T("コントローラの初期化に失敗しています。<I4C3DModules::SelectTargetController>"));
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
		return FALSE;	// EnumWindowを中断
	}
	return TRUE;		// EnumWindowを継続
}