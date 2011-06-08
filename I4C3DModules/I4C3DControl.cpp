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
 * 修飾キーと各コマンドの移動量拡大率を変更します。
 * 
 * @param bCtrl
 * CONTROLキー押下か。
 * 
 * @param bAlt
 * ALTキー押下か。
 * 
 * @param bShift
 * SHIFTキー押下か。
 * 
 * @param tumbleRate
 * TUMBLE拡大率。
 * 
 * @param trackRate
 * TRACK拡大率。
 * 
 * @param dollyRate
 * DOLLY拡大率。
 * 
 * 修飾キーと各コマンドの移動量拡大率を変更します。
 * 
 * @remarks
 * GUIからの変更を受け付けます。
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
 * 修飾キー・拡大率・終端文字を設定ファイルから取得し、各3Dソフトプラグインモジュールにinitメッセージを送信します。
 * 
 * @param pContext
 * I4C3Dモジュールのコンテキストのポインタ。
 * 
 * @returns
 * 初期化に成功した場合にはTRUE、失敗した場合にはFALSEを返します。
 * 
 * 修飾キー・拡大率・終端文字を設定ファイルから取得し、各3DソフトプラグインモジュールにUDPでinitメッセージを送信します。
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

		// ポート番号
		USHORT uPort = 10001;
		PCTSTR szPort = pContext->pAnalyzer->GetSoftValue(g_szController[i], TAG_UDP_PORT);
		if (szPort == NULL) {
			uPort += (USHORT)i;
			_stprintf_s(szError, _countof(szError), _T("設定ファイルに%sのUDPポートを指定してください。仮に%uに設定します。<I4C3DControl::Initialize>"), g_szController[i], uPort);
			LogDebugMessage(Log_Error, szError);
		} else {
			uPort = (USHORT)_tstoi(szPort);
		}

		// ハンドラの初期化
		I4C3DSoftwareHandler* pHandler = new I4C3DSoftwareHandler(g_szController[i], uPort);
		if (!pHandler->PrepareSocket()) {
			_stprintf_s(szError, _countof(szError), _T("%sの管理クラス[ソケット]の作成に失敗しています。%sへの操作は行われません。"), g_szController[i], g_szController[i]);
			LogDebugMessage(Log_Error, szError);
			delete pHandler;
			continue;
		}

		// ハンドラをvectorで管理
		g_pSoftwareHandlerContainer->push_back(pHandler);

		// 修飾キー
		szModifierKey = pContext->pAnalyzer->GetSoftValue(g_szController[i], TAG_MODIFIER);
		if (szModifierKey == NULL) {
			LogDebugMessage(Log_Error, _T("設定ファイルに修飾キーを設定してください。<I4C3DControl::Initialize>"));
			strcpy_s(cszModifierKey, sizeof(cszModifierKey), "NULL");	// "NULL"の場合は、それぞれのプラグインでデフォルト値に設定する。
		} else {
#if UNICODE || _UNICODE
			WideCharToMultiByte(CP_ACP, 0, szModifierKey, _tcslen(szModifierKey), cszModifierKey, sizeof(cszModifierKey), NULL, NULL);
#else
			strcpy_s(cszModifierKey, sizeof(cszModifierKey), szModifierKey);
#endif
		}

		// 各コマンドの移動量パラメータの比率を取得
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

		// 各Plugin.exeへの電文作成・送信
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
 * 3Dソフトハンドラクラスの終了処理と削除を行います。
 * 
 * 3Dソフトハンドラクラスの終了処理と削除を行います。各プラグインモジュールに終了メッセージを送信します。
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
 * ipodからの電文をTOPMOSTの3Dソフトに転送します。
 * 
 * @param szCommand
 * ipodから受信した電文。
 * 
 * @param commandLen
 * 電文長。
 * 
 * ipodからの電文をTOPMOSTの3Dソフトに転送します。転送はUDPで行います。
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
			// ウィンドウハンドルを付加
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
