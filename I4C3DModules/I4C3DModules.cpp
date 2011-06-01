// I4C3DModules.cpp : DLL アプリケーション用にエクスポートされる関数を定義します。
//

#include "stdafx.h"
#include "I4C3DModules.h"
#include "I4C3DMisc.h"
#include "I4C3DCore.h"
#include "I4C3DAnalyzeXML.h"
#include "I4C3DControl.h"

static I4C3DContext g_Context = {0};
static I4C3DCore g_Core;
static BOOL g_bStarted = FALSE;

//static HWND g_targetWindow = NULL;
//static BOOL CALLBACK EnumWindowProc(HWND hWnd, LPARAM lParam);

//static BOOL SelectTarget(PCTSTR szTargetWindowTitle);
//static HWND GetTarget3DSoftwareWnd(PCTSTR szTargetWindowTitle);
static BOOL PrepareTargetController(char cTermination);
static void DestroyTargetController();
static const PCTSTR TAG_TERMINATION	= _T("termination");

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

	// 設定ファイルから終端文字を取得
	char cTermination = '?';
	PCTSTR szTermination = g_Context.pAnalyzer->GetGlobalValue(TAG_TERMINATION);
	if (szTermination != NULL) {
		char cszTermination[5];
		if (_tcslen(szTermination) != 1) {
			I4C3DMisc::LogDebugMessage(Log_Error, _T("設定ファイルの終端文字の指定に誤りがあります。1文字で指定してください。'?'に仮指定されます"));
			szTermination = _T("?");
		}
#if UNICODE || _UNICODE
		WideCharToMultiByte(CP_ACP, 0, szTermination, _tcslen(szTermination), cszTermination, sizeof(cszTermination), NULL, NULL);
#else
		strcpy_s(cszTermination, sizeof(cszTermination), szTermination);
#endif
		I4C3DMisc::RemoveWhiteSpaceA(cszTermination);
		cTermination = cszTermination[0];
	}

	if (!PrepareTargetController(cTermination)) {
		I4C3DStop();
		return FALSE;
	}

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
	DestroyTargetController();
	g_Context.bIsAlive = FALSE;
	g_bStarted = FALSE;
	//g_targetWindow = NULL;
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
BOOL PrepareTargetController(char cTermination)
{
	if (g_Context.pController != NULL) {
		delete g_Context.pController;
		g_Context.pController = NULL;
	}
	g_Context.pController = new I4C3DControl;

	// 初期化
	if (g_Context.pController == NULL || !g_Context.pController->Initialize(&g_Context, cTermination)) {
		I4C3DMisc::LogDebugMessage(Log_Error, _T("コントローラの初期化に失敗しています。<I4C3DModules::SelectTargetController>"));
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