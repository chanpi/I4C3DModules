// dllmain.cpp : DLL アプリケーションのエントリ ポイントを定義します。
#include "stdafx.h"
#include "resource.h"
#include "I4C3DModulesDefs.h"

TCHAR szTitle[MAX_LOADSTRING];

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID /*lpReserved*/
					 )
{
	static WSAData wsaData;
	WORD wVersion;
	int nResult;

	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		LoadString(hModule, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);

		wVersion = MAKEWORD(2,2);
		nResult = WSAStartup(wVersion, &wsaData);
		if (nResult != 0) {
			MessageBox(NULL, _T("[ERROR] Initialize Winsock."), szTitle, MB_OK | MB_ICONERROR);
			return FALSE;
		}
		if (wsaData.wVersion != wVersion) {
			MessageBox(NULL, _T("[ERROR] Winsock バージョン."), szTitle, MB_OK | MB_ICONERROR);
			WSACleanup();
			return FALSE;
		}
		break;

	case DLL_THREAD_ATTACH:
		break;

	case DLL_THREAD_DETACH:
		break;

	case DLL_PROCESS_DETACH:
		WSACleanup();
		break;
	}
	return TRUE;
}

