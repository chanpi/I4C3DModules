#ifndef _I4C3DModules_
#define _I4C3DModules_

#pragma comment(linker, "/export:I4C3DStart=_I4C3DStart@8")
#pragma comment(linker, "/export:I4C3DStop=_I4C3DStop@0")

#include <Windows.h>

#define MY_I4C3DEXIT	(WM_APP+10)

#ifdef I4C3DMODULES_EXPORTS
#define DLL_EXPORT		__declspec(dllexport)
#else
#define DLL_EXPORT		__declspec(dllimport)
#endif

#ifdef __cplusplus
extern "C" {
#endif

	// szXMLUriには設定ファイルのパスを指定してください。
	DLL_EXPORT BOOL WINAPI I4C3DStart(PCTSTR szXMLUri, HWND hWnd);
	// プログラムの終わりに必ず呼んでください（メモリリークにつながります）。
	DLL_EXPORT void WINAPI I4C3DStop(void);

	void I4C3DExit(int errorCode);

#ifdef __cplusplus
}
#endif

#endif /* _I4C3DModules_ */
