#include "StdAfx.h"
#include "I4C3DModulesDefs.h"
#include "I4C3DAnalyzeXML.h"
#include "I4C3DMisc.h"
#include "XMLParser.h"

extern TCHAR szTitle[MAX_LOADSTRING];

static IXMLDOMElement* g_pRootElement = NULL;
static BOOL g_bInitialized = FALSE;
static map<PCTSTR, PCTSTR>* g_pGlobalConfig = NULL;
static map<PCTSTR, PCTSTR>* g_pSoftwareConfig = NULL;

static void CleanupRootElement(void);

static const PCTSTR TAG_GLOBAL	= _T("global");
static const PCTSTR TAG_SOFTS	= _T("softs");
static const PCTSTR TAG_NAME	= _T("name");

inline void SafeReleaseMap(map<PCTSTR, PCTSTR>* pMap)
{
	if (pMap != NULL) {
		CleanupStoredValues(pMap);
		pMap = NULL;
	}
}

I4C3DAnalyzeXML::I4C3DAnalyzeXML(void)
{
}

I4C3DAnalyzeXML::~I4C3DAnalyzeXML(void)
{
	CleanupRootElement();
}

/**
 * @brief
 * XMLParser.dllで取得したXMLのマップオブジェクトを解放します。
 * 
 * XMLParser.dllで取得したXMLのマップオブジェクトを解放します。
 * 
 * @remarks
 * マップの取得に成功した場合には必ず本関数で解放してください。
 * 
 * @see
 * ReadGlobalTag() | ReadSoftsTag()
 */
static void CleanupRootElement(void)
{
	if (g_bInitialized) {
		SafeReleaseMap(g_pGlobalConfig);
		SafeReleaseMap(g_pSoftwareConfig);
		UnInitialize(g_pRootElement);
		g_bInitialized = FALSE;
	}
}

/**
 * @brief
 * XMLをロードしオブジェクトにします。
 * 
 * @param szXMLUri
 * ロードするXMLファイル。
 * 
 * @returns
 * XMLファイルのロードに成功した場合にはTRUE、失敗した場合にはFALSEが返ります。
 * 
 * XMLParser.dllを利用し、XMLをロードしオブジェクトにします。
 */
BOOL I4C3DAnalyzeXML::LoadXML(PCTSTR szXMLUri)
{
	CleanupRootElement();
	if (!PathFileExists(szXMLUri)) {
		{
			TCHAR szError[I4C3D_BUFFER_SIZE];
			_stprintf_s(szError, _countof(szError), _T("指定したパスに設定ファイルが存在しません[%s]。<I4C3DAnalyzeXML::LoadXML>"), szXMLUri);
			I4C3DMisc::LogDebugMessage(Log_Error, szError);
		}
		return FALSE;
	}
	g_bInitialized = Initialize(&g_pRootElement, szXMLUri);
	return g_bInitialized;
}

/**
 * @brief
 * globalタグの内容を問い合わせます。
 * 
 * @param szKey
 * global/keys/keyタグのキー値。
 * 
 * @returns
 * マップ中にキーで指定した値があれば値を、なければNULLを返します。
 * 
 * globalタグの内容を格納したマップにキーを問い合わせ、値を取得します。
 * 
 * @see
 * ReadGlobalTag()
 */
PCTSTR I4C3DAnalyzeXML::GetGlobalValue(PCTSTR szKey)
{
	if (!ReadGlobalTag()) {
		I4C3DMisc::ReportError(_T("[ERROR] globalタグの読み込みに失敗しています。"));
		return NULL;
	}

	return GetMapItem(g_pGlobalConfig, szKey);
}

/**
 * @brief
 * softsタグの内容を問い合わせます。
 * 
 * @param szKey
 * softs/soft/keys/keyタグのキー値。
 * 
 * @returns
 * マップ中にキーで指定した値があれば値を、なければNULLを返します。
 * 
 * softsタグの内容を格納したマップにキーを問い合わせ、値を取得します。
 * 
 * @see
 * ReadGlobalTag() | ReadSoftsTag()
 */
PCTSTR I4C3DAnalyzeXML::GetSoftValue(PCTSTR szKey)
{
	if (!this->ReadGlobalTag()) {
		I4C3DMisc::ReportError(_T("[ERROR] globalタグの読み込みに失敗しています。"));
		return NULL;
	}
	if (!this->ReadSoftsTag()) {
		I4C3DMisc::ReportError(_T("[ERROR] softsタグの読み込みに失敗しています。"));
		return NULL;
	}

	return GetMapItem(g_pSoftwareConfig, szKey);
}

/**
 * @brief
 * I4C3D.xmlのglobalタグの解析を行います。
 * 
 * @returns
 * I4C3D.xmlのglobalタグの解析に成功した場合にはTRUE、失敗した場合にはFALSEを返します。
 * 
 * XMLParser.dllを使用してI4C3D.xmlのglobalタグの解析を行い、マップに格納します。
 * 例: <key name="log">info</key>
 * keyタグnameアトリビュートの値をマップのキーに、バリューをマップのバリューに格納します。
 * 
 * @remarks
 * マップの内容を参照するにはGetGlobalValue()を使用します。
 * 
 * @see
 * GetGlobalValue()
 */
BOOL I4C3DAnalyzeXML::ReadGlobalTag(void)
{
	if (g_pGlobalConfig == NULL) {
		IXMLDOMNode* pGlobal = NULL;
		if (GetDOMTree(g_pRootElement, TAG_GLOBAL, &pGlobal)) {
			g_pGlobalConfig = StoreValues(pGlobal, TAG_NAME);
			FreeDOMTree(pGlobal);

		} else {
			return FALSE;
		}
	}
	return TRUE;
}

/**
 * @brief
 * I4C3D.xmlのsoftsタグの解析を行います。
 * 
 * @returns
 * I4C3D.xmlのsoftsタグの解析に成功した場合にはTRUE、失敗した場合にはFALSEを返します。
 * 
 * XMLParser.dllを使用してI4C3D.xmlのsoftsタグの解析を行い、マップに格納します。
 * マップにはグローバルタグで指定された3Dソフトウェアに関するタグの内容を格納します。
 * 例: <key name="window_title">RTT DeltaView</key>
 * keyタグnameアトリビュートの値をマップのキーに、バリューをマップのバリューに格納します。
 * 
 * @remarks
 * マップの内容を参照するにはGetSoftValue()を使用します。
 * 
 * @see
 * GetSoftValue()
 */
BOOL I4C3DAnalyzeXML::ReadSoftsTag(void)
{
	BOOL bRet = FALSE;
	if (!this->ReadGlobalTag()) {
		I4C3DMisc::LogDebugMessage(Log_Error, _T("globalタグの読み込みに失敗しています。<I4C3DAnalyzeXML::ReadSoftsTag>"));
		return bRet;
	}

	if (g_pSoftwareConfig == NULL) {
		IXMLDOMNode* pSofts = NULL;
		IXMLDOMNodeList* pSoftsList = NULL;
		if (!GetDOMTree(g_pRootElement, TAG_SOFTS, &pSofts)) {
			I4C3DMisc::LogDebugMessage(Log_Error, _T("softsタグのDOM取得に失敗しています。<I4C3DAnalyzeXML::ReadSoftsTag>"));
			return bRet;
		}

		LONG childCount = GetChildList(pSofts, &pSoftsList);
		for (int i = 0; i < childCount; i++) {
			IXMLDOMNode* pTempNode = NULL;
			if (GetListItem(pSoftsList, i, &pTempNode)) {
				TCHAR szSoftwareName[I4C3D_BUFFER_SIZE] = {0};
				if (GetAttribute(pTempNode, TAG_NAME, szSoftwareName, _countof(szSoftwareName))) {

					// globalタグのターゲット名と比較
					if (!_tcsicmp(szSoftwareName,  this->GetGlobalValue(TAG_TARGET))) {
						g_pSoftwareConfig = StoreValues(pTempNode, TAG_NAME);
						FreeListItem(pTempNode);
						bRet = TRUE;
						break;
					}

				}
				FreeListItem(pTempNode);
			}
		}

		FreeChildList(pSoftsList);
		FreeDOMTree(pSofts);
	} else {
		bRet = TRUE;
	}
	return bRet;
}