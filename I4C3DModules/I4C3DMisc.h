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

	// �ݒ�t�@�C����ǂݍ��݁A���O���x����ݒ肷��B
	// �ǂݍ��݂Ɏ��s�����ꍇ��A�{�֐����Ăяo���Ȃ������ꍇ�́ALog_Error���x���ɐݒ肷��B
	// main�Ȃǂōŏ��Ɉ�x�����Ăяo���̂��悢�B
	static void LogInitialize(I4C3DAnalyzeXML* pAnalyzer);

	static void LogDebugMessageW(LOG_LEVEL logLevel, PCWSTR szMessage);
	static void LogDebugMessageA(LOG_LEVEL logLevel, PCSTR szMessage);

	// �擪�����̃X�y�[�X�������B
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