#include "StdAfx.h"
#include "I4C3DCursor.h"
#include "I4C3DMisc.h"

I4C3DCursor::I4C3DCursor(void)
{
}

I4C3DCursor::~I4C3DCursor(void)
{
	RestoreCursor();
}

BOOL I4C3DCursor::SetTransparentCursor()
{
	HCURSOR hCursor;

	hCursor = LoadCursorFromFile(_T("transparent.cur"));
	if (hCursor == NULL) {
		I4C3DMisc::LogDebugMessage(Log_Error, _T("透明カーソルをロードできないため、カーソルの変更を行いません。"));
		return FALSE;
	}

	if (SetSystemCursor(hCursor, 32512)) {
		return TRUE;
	} else {
		RestoreCursor();
	}
	DestroyCursor(hCursor);	// 現在使用中のカーソルはDestroyすることはできない。

	return FALSE;
}

void I4C3DCursor::RestoreCursor(void)
{
	SystemParametersInfo(SPI_SETCURSORS, 0, NULL, 0);
}
