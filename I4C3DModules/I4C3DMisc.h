#pragma once

typedef enum { Log_Debug, Log_Info, Log_Error, Log_Off } LOG_LEVEL;
class I4C3DAnalyzeXML;

class I4C3DMisc
{
public:
	I4C3DMisc(void);
	~I4C3DMisc(void);

	static BOOL GetModuleFileWithExtension(PTSTR szFilePath, SIZE_T cchLength, PCTSTR szExtension);
	static void ReportError(PCTSTR szMessage);

	// 設定ファイルを読み込み、ログレベルを設定する。
	// 読み込みに失敗した場合や、本関数を呼び出さなかった場合は、Log_Errorレベルに設定する。
	// mainなどで最初に一度だけ呼び出すのがよい。
	static void LogInitialize(I4C3DAnalyzeXML* pAnalyzer);

	static void LogDebugMessageW(LOG_LEVEL logLevel, PCWSTR szMessage);
	static void LogDebugMessageA(LOG_LEVEL logLevel, PCSTR szMessage);

	// 先頭末尾のスペースを除く。
	static void RemoveWhiteSpaceW(PWSTR szBuffer);
	static void RemoveWhiteSpaceA(PSTR szBuffer);

	static void DebugProfileMonitor(PCSTR position);

private:
	static BOOL PrepareLogFile(PWSTR szFileName, SIZE_T cchLength, PCSTR szExtension);
};

#if defined UNICODE || _UNICODE

#define LogDebugMessage LogDebugMessageW
#define RemoveWhiteSpace RemoveWhiteSpaceW

#else

#define LogDebugMessage LogDebugMessageA
#define RemoveWhiteSpace RemoveWhiteSpaceA

#endif