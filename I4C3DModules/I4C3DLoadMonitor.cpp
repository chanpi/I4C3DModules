#include "StdAfx.h"
#include "I4C3DLoadMonitor.h"
#include "I4C3DModulesDefs.h"
#include "I4C3DAnalyzeXML.h"

#include <Pdh.h>

static const int MAX_CPUCORE_COUNT	= 16;
static const int MAX_QUERY_LENGTH	= 64;
static const PCTSTR TAG_SAMPLING_RATE = _T("cpu_sampling_rate");

/**
 * @brief
 * CPU使用率によって適度にSleepをはさみながらコマンド実行を許可するスレッドの関数です。
 * 
 * @param pParam
 * I4C3Dモジュールのコンテキストのポインタ。
 * 
 * @returns
 * 正常終了の値を返します。
 * 
 * CPU使用率によって適度にSleepをはさみながらコマンド実行を許可するスレッドの関数です。
 * 適度なSleepをはさみながら、実行の制限と（制限によらない）実行を交互に行います。
 * 
 * @remarks
 * Sleepの間隔はI4C3DProcessorMonitorThreadProc()で調整されます。
 * 
 * @see
 * I4C3DProcessorMonitorThreadProc()
 */
unsigned int __stdcall I4C3DProcessorContextThreadProc(void *pParam)
{
	I4C3DContext* pContext = (I4C3DContext*)pParam;

	while (pContext->bIsAlive && WaitForSingleObject(pContext->processorContext.hProcessorStopEvent, 0) == WAIT_TIMEOUT) {
		pContext->processorContext.bIsBusy = TRUE;

#if DEBUG || _DEBUG
		if (pContext->processorContext.uNapTime > 0) {
			TCHAR szBuf[32];
			_stprintf_s(szBuf, 32, L"%d msec sleep\n", pContext->processorContext.uNapTime);
			OutputDebugString(szBuf);
		}
#endif
		Sleep(pContext->processorContext.uNapTime);
		pContext->processorContext.bIsBusy = FALSE;
		Sleep(pContext->processorContext.uCoreTime);
	}
	return EXIT_SUCCESS;
}

/**
 * @brief
 * CPU使用率によってSleepの間隔を調整するスレッドの関数です。
 * 
 * @param pParam
 * I4C3Dモジュールのコンテキストのポインタ。
 * 
 * @returns
 * 正常終了の値を返します。
 * 
 * CPU使用率によってSleepの間隔を調整するスレッドの関数です。
 * 閾値より高いCPU使用率を記録したコアが現れると、Sleep時間を増やします。
 * 
 * @remarks
 * 実際にSleepを行うのはI4C3DProcessorContextThreadProc()です。
 * 
 * @see
 * I4C3DProcessorContextThreadProc()
 */
unsigned int __stdcall I4C3DProcessorMonitorThreadProc(void *pParam)
{
	PDH_HQUERY hQuerys[MAX_CPUCORE_COUNT];
	PDH_HCOUNTER hCounters[MAX_CPUCORE_COUNT];
	PDH_FMT_COUNTERVALUE fmtValues[MAX_CPUCORE_COUNT];
	TCHAR szQuerys[MAX_CPUCORE_COUNT][MAX_QUERY_LENGTH];
	DWORD i = 0;
	bool isQueryOpend[MAX_CPUCORE_COUNT];
	DWORD dwSamplingRate = 0;

	I4C3DContext* pContext = (I4C3DContext*)pParam;

	PCTSTR szSamplingRate = pContext->pAnalyzer->GetGlobalValue(TAG_SAMPLING_RATE);
	if (szSamplingRate == NULL || 
		_stscanf_s(szSamplingRate, _T("%d"), &dwSamplingRate, sizeof(dwSamplingRate)) != 1) {
			dwSamplingRate = 1000;
	}

	SYSTEM_INFO sysInfo;
	ZeroMemory(&sysInfo, sizeof(sysInfo));
	ZeroMemory(&szQuerys, sizeof(szQuerys));
	GetSystemInfo(&sysInfo);

	for (i = 0; i < sysInfo.dwNumberOfProcessors; i++) {
		_stprintf_s(szQuerys[i], _countof(szQuerys[i]), _T("\\Processor(%d)\\%% Processor Time"), i);
	}

	while (pContext->bIsAlive && WaitForSingleObject(pContext->processorContext.hProcessorStopEvent, 0) == WAIT_TIMEOUT) {
		for (i = 0; i < sysInfo.dwNumberOfProcessors; i++) {
			if (PdhOpenQuery(NULL, 0, &hQuerys[i]) == ERROR_SUCCESS) {
				PdhAddCounter(hQuerys[i], szQuerys[i], 0, &hCounters[i]);
				PdhCollectQueryData(hQuerys[i]);
				isQueryOpend[i] = true;
			} else {
				isQueryOpend[i] = false;
			}
		}

		Sleep(dwSamplingRate);
		for (i = 0; i < sysInfo.dwNumberOfProcessors; i++) {
			if (isQueryOpend[i]) {
				PdhCollectQueryData(hQuerys[i]);
				PdhGetFormattedCounterValue(hCounters[i], PDH_FMT_DOUBLE, NULL, &fmtValues[i]);
				PdhCloseQuery(hQuerys[i]);
			}
		}

		for (i = 0; i < sysInfo.dwNumberOfProcessors; i++) {
			if (fmtValues[i].doubleValue > pContext->processorContext.uThreshouldOfBusy) {
				pContext->processorContext.uNapTime += 10;
				if (pContext->processorContext.uNapTime > 100) {
					pContext->processorContext.uNapTime = 100;
				}
			}
		}
		if (pContext->processorContext.uNapTime > 0) {
			pContext->processorContext.uNapTime -= 2;
		}
	}

	return EXIT_SUCCESS;
}
