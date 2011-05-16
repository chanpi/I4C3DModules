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
 * XMLParser.dll�Ŏ擾����XML�̃}�b�v�I�u�W�F�N�g��������܂��B
 * 
 * XMLParser.dll�Ŏ擾����XML�̃}�b�v�I�u�W�F�N�g��������܂��B
 * 
 * @remarks
 * �}�b�v�̎擾�ɐ��������ꍇ�ɂ͕K���{�֐��ŉ�����Ă��������B
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
 * XML�����[�h���I�u�W�F�N�g�ɂ��܂��B
 * 
 * @param szXMLUri
 * ���[�h����XML�t�@�C���B
 * 
 * @returns
 * XML�t�@�C���̃��[�h�ɐ��������ꍇ�ɂ�TRUE�A���s�����ꍇ�ɂ�FALSE���Ԃ�܂��B
 * 
 * XMLParser.dll�𗘗p���AXML�����[�h���I�u�W�F�N�g�ɂ��܂��B
 */
BOOL I4C3DAnalyzeXML::LoadXML(PCTSTR szXMLUri)
{
	CleanupRootElement();
	if (!PathFileExists(szXMLUri)) {
		{
			TCHAR szError[I4C3D_BUFFER_SIZE];
			_stprintf_s(szError, _countof(szError), _T("�w�肵���p�X�ɐݒ�t�@�C�������݂��܂���[%s]�B<I4C3DAnalyzeXML::LoadXML>"), szXMLUri);
			I4C3DMisc::LogDebugMessage(Log_Error, szError);
		}
		return FALSE;
	}
	g_bInitialized = Initialize(&g_pRootElement, szXMLUri);
	return g_bInitialized;
}

/**
 * @brief
 * global�^�O�̓��e��₢���킹�܂��B
 * 
 * @param szKey
 * global/keys/key�^�O�̃L�[�l�B
 * 
 * @returns
 * �}�b�v���ɃL�[�Ŏw�肵���l������Βl���A�Ȃ����NULL��Ԃ��܂��B
 * 
 * global�^�O�̓��e���i�[�����}�b�v�ɃL�[��₢���킹�A�l���擾���܂��B
 * 
 * @see
 * ReadGlobalTag()
 */
PCTSTR I4C3DAnalyzeXML::GetGlobalValue(PCTSTR szKey)
{
	if (!ReadGlobalTag()) {
		I4C3DMisc::ReportError(_T("[ERROR] global�^�O�̓ǂݍ��݂Ɏ��s���Ă��܂��B"));
		return NULL;
	}

	return GetMapItem(g_pGlobalConfig, szKey);
}

/**
 * @brief
 * softs�^�O�̓��e��₢���킹�܂��B
 * 
 * @param szKey
 * softs/soft/keys/key�^�O�̃L�[�l�B
 * 
 * @returns
 * �}�b�v���ɃL�[�Ŏw�肵���l������Βl���A�Ȃ����NULL��Ԃ��܂��B
 * 
 * softs�^�O�̓��e���i�[�����}�b�v�ɃL�[��₢���킹�A�l���擾���܂��B
 * 
 * @see
 * ReadGlobalTag() | ReadSoftsTag()
 */
PCTSTR I4C3DAnalyzeXML::GetSoftValue(PCTSTR szKey)
{
	if (!this->ReadGlobalTag()) {
		I4C3DMisc::ReportError(_T("[ERROR] global�^�O�̓ǂݍ��݂Ɏ��s���Ă��܂��B"));
		return NULL;
	}
	if (!this->ReadSoftsTag()) {
		I4C3DMisc::ReportError(_T("[ERROR] softs�^�O�̓ǂݍ��݂Ɏ��s���Ă��܂��B"));
		return NULL;
	}

	return GetMapItem(g_pSoftwareConfig, szKey);
}

/**
 * @brief
 * I4C3D.xml��global�^�O�̉�͂��s���܂��B
 * 
 * @returns
 * I4C3D.xml��global�^�O�̉�͂ɐ��������ꍇ�ɂ�TRUE�A���s�����ꍇ�ɂ�FALSE��Ԃ��܂��B
 * 
 * XMLParser.dll���g�p����I4C3D.xml��global�^�O�̉�͂��s���A�}�b�v�Ɋi�[���܂��B
 * ��: <key name="log">info</key>
 * key�^�Oname�A�g���r���[�g�̒l���}�b�v�̃L�[�ɁA�o�����[���}�b�v�̃o�����[�Ɋi�[���܂��B
 * 
 * @remarks
 * �}�b�v�̓��e���Q�Ƃ���ɂ�GetGlobalValue()���g�p���܂��B
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
 * I4C3D.xml��softs�^�O�̉�͂��s���܂��B
 * 
 * @returns
 * I4C3D.xml��softs�^�O�̉�͂ɐ��������ꍇ�ɂ�TRUE�A���s�����ꍇ�ɂ�FALSE��Ԃ��܂��B
 * 
 * XMLParser.dll���g�p����I4C3D.xml��softs�^�O�̉�͂��s���A�}�b�v�Ɋi�[���܂��B
 * �}�b�v�ɂ̓O���[�o���^�O�Ŏw�肳�ꂽ3D�\�t�g�E�F�A�Ɋւ���^�O�̓��e���i�[���܂��B
 * ��: <key name="window_title">RTT DeltaView</key>
 * key�^�Oname�A�g���r���[�g�̒l���}�b�v�̃L�[�ɁA�o�����[���}�b�v�̃o�����[�Ɋi�[���܂��B
 * 
 * @remarks
 * �}�b�v�̓��e���Q�Ƃ���ɂ�GetSoftValue()���g�p���܂��B
 * 
 * @see
 * GetSoftValue()
 */
BOOL I4C3DAnalyzeXML::ReadSoftsTag(void)
{
	BOOL bRet = FALSE;
	if (!this->ReadGlobalTag()) {
		I4C3DMisc::LogDebugMessage(Log_Error, _T("global�^�O�̓ǂݍ��݂Ɏ��s���Ă��܂��B<I4C3DAnalyzeXML::ReadSoftsTag>"));
		return bRet;
	}

	if (g_pSoftwareConfig == NULL) {
		IXMLDOMNode* pSofts = NULL;
		IXMLDOMNodeList* pSoftsList = NULL;
		if (!GetDOMTree(g_pRootElement, TAG_SOFTS, &pSofts)) {
			I4C3DMisc::LogDebugMessage(Log_Error, _T("softs�^�O��DOM�擾�Ɏ��s���Ă��܂��B<I4C3DAnalyzeXML::ReadSoftsTag>"));
			return bRet;
		}

		LONG childCount = GetChildList(pSofts, &pSoftsList);
		for (int i = 0; i < childCount; i++) {
			IXMLDOMNode* pTempNode = NULL;
			if (GetListItem(pSoftsList, i, &pTempNode)) {
				TCHAR szSoftwareName[I4C3D_BUFFER_SIZE] = {0};
				if (GetAttribute(pTempNode, TAG_NAME, szSoftwareName, _countof(szSoftwareName))) {

					// global�^�O�̃^�[�Q�b�g���Ɣ�r
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