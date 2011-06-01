
#include "StdAfx.h"
#include "I4C3DMisc.h"
#include "I4C3DModulesDefs.h"
#include "I4C3DAnalyzeXML.h"
#include <time.h>
#include <MMSystem.h>

extern TCHAR szTitle[MAX_LOADSTRING];

static const PCTSTR TAG_LOG		= _T("log");
static const PCTSTR TAG_OFF		= _T("off");
static const PCTSTR TAG_DEBUG	= _T("debug");
static const PCTSTR TAG_INFO	= _T("info");

//static TCHAR g_szLogFileName[MAX_PATH*2]	= {0};
static LOG_LEVEL g_logLevel					= Log_Error;

static LARGE_INTEGER g_liPrev = {0};
static LARGE_INTEGER g_liNow = {0};


static void GetCurrentTimeForLogW(LOG_LEVEL logLevel, PTSTR szTime, SIZE_T cchLength);
static void GetCurrentTimeForLogA(LOG_LEVEL logLevel, PSTR szTime, SIZE_T cchLength);

#if defined UNICODE || _UNICODE
#define GetCurrentTimeForLog GetCurrentTimeForLogW
#else
#define GetCurrentTimeForLog GetCurrentTimeForLogA
#endif

I4C3DMisc::I4C3DMisc(void)
{
}


I4C3DMisc::~I4C3DMisc(void)
{
}

void I4C3DMisc::LogInitialize(I4C3DAnalyzeXML* pAnalyzer)
{
	PCTSTR szLogLevel = pAnalyzer->GetGlobalValue(TAG_LOG);
	if (szLogLevel != NULL) {
		if (!_tcsicmp(szLogLevel, TAG_OFF)) {
			g_logLevel = Log_Off;
		} else if (!_tcsicmp(szLogLevel, TAG_DEBUG)) {
			g_logLevel = Log_Debug;
		} else if (!_tcsicmp(szLogLevel, TAG_INFO)) {
			g_logLevel = Log_Info;
		}
		// 指定なし、上記以外ならLog_Error
	}	
}

BOOL I4C3DMisc::GetModuleFileWithExtension(PTSTR szFilePath, SIZE_T cchLength, PCTSTR szExtension) {
	TCHAR* ptr = NULL;
	if (!GetModuleFileName(NULL, szFilePath, cchLength)) {
		ReportError(_T("[ERROR] 実行ファイル名の取得に失敗しました。"));
	}
	ptr = _tcsrchr(szFilePath, _T('.'));
	if (ptr != NULL) {
		_tcscpy_s(ptr+1, cchLength-(ptr+1-szFilePath), szExtension);
		return TRUE;
	}
	return FALSE;
}

void I4C3DMisc::ReportError(PCTSTR szMessage) {
	MessageBox(NULL, szMessage, szTitle, MB_OK | MB_ICONERROR);
}

BOOL I4C3DMisc::PrepareLogFile(PWSTR szFileName, SIZE_T cchLength, PCSTR szExtension)
{
	if (!PathIsDirectory(_T("logs"))) {
		if (!CreateDirectory(_T("logs"), NULL)) {
			return FALSE;
		}
	}

	time_t timer;
	struct tm date = {0};
	char buffer[I4C3D_BUFFER_SIZE];

	timer = time(NULL);
	localtime_s(&date, &timer);
	sprintf_s(buffer, _countof(buffer), "logs\\%04d%02d%02d%s", date.tm_year+1900, date.tm_mon+1, date.tm_mday, szExtension);

	MultiByteToWideChar(CP_ACP, 0, buffer, -1, szFileName, cchLength);
	return TRUE;
}

void I4C3DMisc::LogDebugMessageW(LOG_LEVEL logLevel, PCWSTR szMessage)
{
	TCHAR szLogFileName[MAX_PATH];

	if (g_logLevel <= logLevel) {
		HANDLE hLogFile;
		unsigned char szBOM[] = { 0xFF, 0xFE };
		DWORD dwNumberOfBytesWritten = 0;

		if (!PrepareLogFile(szLogFileName, _countof(szLogFileName), ".log")) {
			//ReportError(_T("log専用ディレクトリを作成できません。ログは出力されません。"));
			return;
		}

		hLogFile = CreateFile(szLogFileName, GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hLogFile == INVALID_HANDLE_VALUE) {
			// 書き込みが混雑してファイルが開けない場合がある（排他オープンのため）
			//WCHAR szError[I4C3D_BUFFER_SIZE];
			//swprintf_s(szError, _countof(szError), L"[ERROR] ログファイルのオープンに失敗しています。: %d", GetLastError());
			//ReportError(szError);
			//ReportError(szMessage);
			return;
		}

		WriteFile(hLogFile, szBOM, 2, &dwNumberOfBytesWritten, NULL);
		SetFilePointer(hLogFile, 0, NULL, FILE_END);

		WCHAR szTime[I4C3D_BUFFER_SIZE] = {0};
		GetCurrentTimeForLog(logLevel, szTime, _countof(szTime));
		WriteFile(hLogFile, szTime, wcslen(szTime) * sizeof(WCHAR), &dwNumberOfBytesWritten, NULL);
		WriteFile(hLogFile, szMessage, wcslen(szMessage) * sizeof(WCHAR), &dwNumberOfBytesWritten, NULL);
		WriteFile(hLogFile, L"\r\n", 2 * sizeof(WCHAR), &dwNumberOfBytesWritten, NULL);
		CloseHandle(hLogFile);
	}
}

void I4C3DMisc::LogDebugMessageA(LOG_LEVEL logLevel, PCSTR szMessage)
{
	TCHAR szLogFileNameA[MAX_PATH];

	if (g_logLevel <= logLevel) {
		HANDLE hLogFile;
		DWORD dwNumberOfBytesWritten = 0;

		if (!PrepareLogFile(szLogFileNameA, _countof(szLogFileNameA), "_info.log")) {
			//ReportError(_T("log専用ディレクトリを作成できません。ログは出力されません。"));
			return;
		}

		hLogFile = CreateFile(szLogFileNameA, GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hLogFile == INVALID_HANDLE_VALUE) {
			// 書き込みが混雑してファイルが開けない場合がある（排他オープンのため）
			//TCHAR szError[I4C3D_BUFFER_SIZE];
			//_stprintf_s(szError, _countof(szError), _T("[ERROR] ログファイルのオープンに失敗しています。: %d"), GetLastError());
			//ReportError(szError);
			return;
		}

		SetFilePointer(hLogFile, 0, NULL, FILE_END);

		char szTime[I4C3D_BUFFER_SIZE] = {0};
		GetCurrentTimeForLogA(logLevel, szTime, _countof(szTime));
		WriteFile(hLogFile, szTime, strlen(szTime), &dwNumberOfBytesWritten, NULL);
		WriteFile(hLogFile, szMessage, strlen(szMessage), &dwNumberOfBytesWritten, NULL);
		WriteFile(hLogFile, "\r\n", 2, &dwNumberOfBytesWritten, NULL);
		CloseHandle(hLogFile);
	}
}

void I4C3DMisc::RemoveWhiteSpaceW(PWSTR szBuffer)
{
	WCHAR* pStart = szBuffer;
	WCHAR* pEnd = szBuffer + wcslen(szBuffer) - 1;
	WORD wCharType = 0;

	while (pStart < pEnd && GetStringTypeEx(LOCALE_USER_DEFAULT, CT_CTYPE1, pStart, 1, &wCharType)) {
		if (wCharType & C1_SPACE) {
			pStart++;
		} else {
			break;
		}
	}

	while (pStart < pEnd && GetStringTypeEx(LOCALE_USER_DEFAULT, CT_CTYPE1, pEnd, 1, &wCharType)) {
		if (wCharType & C1_SPACE) {
			pEnd--;
		} else {
			break;
		}
	}

	MoveMemory(szBuffer, pStart, pEnd - pStart + 1);
	*(szBuffer + (pEnd - pStart + 1)) = L'\0';
}

void I4C3DMisc::RemoveWhiteSpaceA(PSTR szBuffer)
{
	char* pStart = szBuffer;
	char* pEnd = szBuffer + strlen(szBuffer) - 1;
	WORD wCharType = 0;

	while (pStart < pEnd && GetStringTypeExA(LOCALE_USER_DEFAULT, CT_CTYPE1, pStart, 1, &wCharType)) {
		if (wCharType & C1_SPACE) {
			pStart++;
		} else {
			break;
		}
	}

	while (pStart < pEnd && GetStringTypeExA(LOCALE_USER_DEFAULT, CT_CTYPE1, pEnd, 1, &wCharType)) {
		if (wCharType & C1_SPACE) {
			pEnd--;
		} else {
			break;
		}
	}

	MoveMemory(szBuffer, pStart, pEnd - pStart + 1);
	*(szBuffer + (pEnd - pStart + 1)) = '\0';
}

void I4C3DMisc::DebugProfileMonitor(PCSTR position) {
	if (g_logLevel < Log_Info) {
		return;
	}
	DOUBLE dTime = 0;
	LARGE_INTEGER liFrequency;
	QueryPerformanceFrequency(&liFrequency);
	QueryPerformanceCounter(&g_liNow);
	dTime = (DOUBLE)((g_liNow.QuadPart - g_liPrev.QuadPart) * 1000 / liFrequency.QuadPart);

	if (dTime > 1.) {
		char buffer[256];
		sprintf_s(buffer, _countof(buffer), "%s %9.4lf[ms]", position, dTime);
		I4C3DMisc::LogDebugMessageA(Log_Info, buffer);
	}
	g_liPrev = g_liNow;
}

////////////////////////////////////////////////////////

inline PCSTR GetLogLevelString(LOG_LEVEL logLevel) {
	PCSTR szLogLevel = "?????";
	switch (logLevel) {
	case Log_Debug:
		szLogLevel = "DEBUG";
		break;

	case Log_Info:
		szLogLevel = " INFO";
		break;

	case Log_Error:
		szLogLevel = "ERROR";
		break;
	}
	return szLogLevel;
}

void GetCurrentTimeForLogW(LOG_LEVEL logLevel, PWSTR szTime, SIZE_T cchLength)
{
	time_t timer;
	struct tm date = {0};
	char buffer[I4C3D_BUFFER_SIZE];
	PCSTR szLogLevel = GetLogLevelString(logLevel);

	timer = time(NULL);
	localtime_s(&date, &timer);
	sprintf_s(buffer, _countof(buffer), "[%04d-%02d-%02d %02d:%02d:%02d %s] ",
		date.tm_year+1900, date.tm_mon+1, date.tm_mday, date.tm_hour, date.tm_min, date.tm_sec, szLogLevel);

	MultiByteToWideChar(CP_ACP, 0, buffer, -1, szTime, cchLength);
}

void GetCurrentTimeForLogA(LOG_LEVEL logLevel, PSTR szTime, SIZE_T cchLength)
{
	time_t timer;
	struct tm date = {0};
	char buffer[I4C3D_BUFFER_SIZE];
	PCSTR szLogLevel = GetLogLevelString(logLevel);

	timer = time(NULL);
	localtime_s(&date, &timer);
	sprintf_s(buffer, _countof(buffer), "[%04d-%02d-%02d %02d:%02d:%02d %s] ",
		date.tm_year+1900, date.tm_mon+1, date.tm_mday, date.tm_hour, date.tm_min, date.tm_sec, szLogLevel);

	strcpy_s(szTime, cchLength, buffer);
}