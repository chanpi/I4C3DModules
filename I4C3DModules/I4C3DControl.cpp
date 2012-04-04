#include "StdAfx.h"
#include "I4C3DCommon.h"
#include "I4C3DControl.h"
#include "Misc.h"
#include "I4C3DAnalyzeXML.h"
#include "I4C3DSoftwareHandler.h"
#include "SharedConstants.h"
#include <vector>
#include <map>
using namespace std;

#if UNICODE || _UNICODE
static LPCTSTR g_FILE = __FILEW__;
#else
static LPCTSTR g_FILE = __FILE__;
#endif

static const PCTSTR TAG_MODIFIER	= _T("modifier");
static const PCTSTR TAG_UDP_PORT	= _T("udp_port");
static const PCTSTR TAG_TUMBLERATE	= _T("tumble_rate");
static const PCTSTR TAG_TRACKRATE	= _T("track_rate");
static const PCTSTR TAG_DOLLYRATE	= _T("dolly_rate");
static const PCTSTR TAG_RTTECMODE	= _T("rttec_mode");

static const PCTSTR COMMAND_REGISTERMACRO	= _T("registermacro");

static TCHAR *g_szController[] = { _T("RTT"), _T("Maya"), _T("Alias"), _T("Showcase"), _T("RTTEC"), };
static enum PluginID { RTTPlugin, MayaPlugin, AliasPlugin, ShowcasePlugin, RTTECPlugin, } PluginIndex;
static map<PluginID, I4C3DSoftwareHandler*> *g_pSoftwareHandlerContainer = NULL;

static BOOL g_bRTTECMode = FALSE;
static CRITICAL_SECTION g_lock = {0};

inline I4C3DSoftwareHandler* SearchKeyInSoftwareHandlerMap(PluginID id) {
	if (g_pSoftwareHandlerContainer != NULL && !g_pSoftwareHandlerContainer->empty()) {
		map<PluginID, I4C3DSoftwareHandler*>::iterator it = g_pSoftwareHandlerContainer->find(id);
		if (it != g_pSoftwareHandlerContainer->end()) {
			return it->second;
		}
	}
	return NULL;
}

I4C3DControl::I4C3DControl(void) : m_bInitialized(FALSE)
{
	g_pSoftwareHandlerContainer = new map<PluginID, I4C3DSoftwareHandler*>;
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

	int i = 0;
	int count = _countof(g_szController) - 1;
	// RTTECモードがONの時はRTTプラグインのみ動作させる
	if (_tcsicmp(pContext->pAnalyzer->GetGlobalValue(TAG_RTTECMODE), _T("on")) == 0) {
		g_bRTTECMode = TRUE;
		i = RTTECPlugin;
		count++;
	}
	
	// マクロ登録のフォーマット文をTCHAR*で作成
	TCHAR registerMacroFormat[I4C3D_BUFFER_SIZE] = {0};
	MultiByteToWideChar(CP_ACP, 0, g_registerMacroFormat, -1, registerMacroFormat, _countof(registerMacroFormat));

	for (; i < count; ++i) {
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
			// 設定ファイルに%sのUDPポートを指定してください。仮に%uに設定します。		
			LoggingMessage(Log_Error, _T(MESSAGE_ERROR_CFG_PORT), GetLastError(), g_FILE, __LINE__);
		} else {
			uPort = (USHORT)_tstoi(szPort);
		}

		// ハンドラの初期化
		I4C3DSoftwareHandler* pHandler = new I4C3DSoftwareHandler(g_szController[i], uPort);
		if (!pHandler->PrepareSocket()) {
			// %sの管理クラス[ソケット]の作成に失敗しています。%sへの操作は行われません。"), g_szController[i], g_szController[i]);
			LoggingMessage(Log_Error, _T(MESSAGE_ERROR_SYSTEM_INIT), GetLastError(), g_FILE, __LINE__);
			delete pHandler;
			continue;
		}

		// ハンドラをmapで管理
		g_pSoftwareHandlerContainer->insert(map<PluginID, I4C3DSoftwareHandler*>::value_type((PluginID)i, pHandler));

		// 修飾キー
		szModifierKey = pContext->pAnalyzer->GetSoftValue(g_szController[i], TAG_MODIFIER);
		if (szModifierKey == NULL) {
			if (!g_bRTTECMode && i != RTTECPlugin) {
				LoggingMessage(Log_Error, _T(MESSAGE_ERROR_CFG_MODIFY), GetLastError(), g_FILE, __LINE__);
			}
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
		if ((g_bRTTECMode && i == RTTECPlugin) || (!g_bRTTECMode && i != RTTECPlugin)) {
			sprintf_s(packet.szCommand, g_initCommandFormat, "init", cszModifierKey, fTumbleRate, fTrackRate, fDollyRate, cTermination);

			EnterCriticalSection(&g_lock);
			sendto(pHandler->m_socketHandler, (const char*)&packet, strlen(packet.szCommand)+4, 0, (const SOCKADDR*)&pHandler->m_address, sizeof(pHandler->m_address));
			LeaveCriticalSection(&g_lock);
		}

		// 各Plugin.exeへキーマクロ設定の電文を送信 TODO
		// 各ソフトウェアタグの<key name="MACROx">value</key>を読み取る
		if (g_bRTTECMode || i == RTTECPlugin) {	// RTTECPluginのときはF710TCPControlが管理する
			continue;
		}
		TCHAR szMacroName[16] = {0};
		TCHAR szTmpCommand[I4C3D_BUFFER_SIZE] = {0};
		PCTSTR szMacroValue = NULL;
		for (int j = 1; j <= MAX_MACROS; j++) {
			_stprintf_s(szMacroName, _countof(szMacroName), _T("MACRO%d"), j);
			szMacroValue = pContext->pAnalyzer->GetSoftValue(g_szController[i], szMacroName);
			if (szMacroValue == NULL) {
				//break;
				continue;
			}

			ZeroMemory(&packet, sizeof(packet));
			_stprintf_s(szTmpCommand, registerMacroFormat, _T("registermacro"), szMacroName, szMacroValue, cTermination);
			WideCharToMultiByte(CP_ACP, 0, szTmpCommand, _tcslen(szTmpCommand), packet.szCommand, sizeof(packet.szCommand), NULL, NULL);
			
			EnterCriticalSection(&g_lock);
			sendto(pHandler->m_socketHandler, (const char*)&packet, strlen(packet.szCommand)+4, 0, (const SOCKADDR*)&pHandler->m_address, sizeof(pHandler->m_address));
			LeaveCriticalSection(&g_lock);
		}
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
		map<PluginID, I4C3DSoftwareHandler*>::iterator it = g_pSoftwareHandlerContainer->begin();
		for (; it != g_pSoftwareHandlerContainer->end(); it++) {
			I4C3DUDPPacket packet = {0};
			strcpy_s(packet.szCommand, _countof(packet.szCommand), "exit");
			EnterCriticalSection(&g_lock);
			sendto(it->second->m_socketHandler, (const char*)&packet, 8, 0, (const SOCKADDR*)&it->second->m_address, sizeof(it->second->m_address));
			LeaveCriticalSection(&g_lock);

			closesocket(it->second->m_socketHandler);
			delete it->second;
			it->second = NULL;
		}
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
	if (g_bRTTECMode) {
		I4C3DSoftwareHandler* handler = SearchKeyInSoftwareHandlerMap(RTTECPlugin);
		if (!handler) {
			return;
		}
		EnterCriticalSection(&g_lock);
		sendto(handler->m_socketHandler, (const char*)pPacket, commandLen+4, 0, (const SOCKADDR*)&(handler->m_address), sizeof(handler->m_address));
		LeaveCriticalSection(&g_lock);
		return;
	}

	TCHAR szWindowTitle[I4C3D_BUFFER_SIZE] = {0};
	HWND hForeground = GetForegroundWindow();
	if (IsIconic(hForeground)) {
		return;
	}
	GetWindowText(hForeground, szWindowTitle, _countof(szWindowTitle));

	if (g_pSoftwareHandlerContainer != NULL) {
		map<PluginID, I4C3DSoftwareHandler*>::iterator it = g_pSoftwareHandlerContainer->begin();
		for (; it != g_pSoftwareHandlerContainer->end(); it++) {
			if (_tcsstr(szWindowTitle, it->second->m_szTargetTitle) != NULL) {
				// ウィンドウハンドルを付加
				if (hForeground == NULL) {
					return;
				}
				for (int j = 0; j < 4; ++j) {
					pPacket->hwnd[j] = ((unsigned char*)&hForeground)[j];
				}

				EnterCriticalSection(&g_lock);
				sendto(it->second->m_socketHandler, (const char*)pPacket, commandLen+4, 0, (const SOCKADDR*)&(it->second->m_address), sizeof(it->second->m_address));
				LeaveCriticalSection(&g_lock);
				return;
			}
		}
	}
}
