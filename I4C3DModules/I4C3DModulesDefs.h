#pragma once

#include "stdafx.h"

class I4C3DControl;
class I4C3DAnalyzeXML;

#define MAX_LOADSTRING			100
#define I4C3D_BUFFER_SIZE		256

#define _STR(x)     #x
#define _STR2(x)    _STR(x)
#define __SLINE__   _STR2(__LINE__)
#define HERE        __FILE__ "("__SLINE__")"

#if DEBUG || _DEBUG
#define DEBUG_PROFILE_MONITOR	DebugProfileMonitor(HERE);
#else
#define DEBUG_PROFILE_MONITOR
#endif

const PCSTR COMMAND_TUMBLE	= "tumble";
const PCSTR COMMAND_TRACK	= "track";
const PCSTR COMMAND_DOLLY	= "dolly";

typedef struct {
	HANDLE hProcessorContextThread;
	HANDLE hProcessorMonitorThread;
	HANDLE hProcessorStopEvent;

	UINT uThreshouldOfBusy;
	UINT uCoreTime;
	volatile UINT uNapTime;
	volatile BOOL bIsBusy;	
} I4C3DProcessorContext;

typedef struct {
	I4C3DAnalyzeXML* pAnalyzer;

	//HWND hTargetParentWnd;
	I4C3DControl* pController;
	HANDLE hThread;			// iPhone/iPodからの受信はワーカースレッドで行う

	HANDLE hStopEvent;
	SOCKET receiver;
	struct sockaddr_in address;

	I4C3DProcessorContext processorContext;
	volatile BOOL bIsAlive;
} I4C3DContext;
