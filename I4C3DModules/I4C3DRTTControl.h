#pragma once
#include "i4c3dcontrol.h"
class I4C3DRTTControl :
	public I4C3DControl
{
public:
	I4C3DRTTControl(void);
	~I4C3DRTTControl(void);

	BOOL Initialize(I4C3DContext* pContext);
	BOOL GetTargetChildWnd(void);

	void HotkeyExecute(I4C3DContext* pContext, PCTSTR szCommand);

	void ModKeyDown(void);
	void ModKeyUp(void);

private:
	HWND m_hKeyInputWnd;
};

