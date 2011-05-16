#pragma once
#include "i4c3dcontrol.h"
class I4C3DAliasControl :
	public I4C3DControl
{
public:
	I4C3DAliasControl(void);
	~I4C3DAliasControl(void);

	BOOL GetTargetChildWnd(void);

	void HotkeyExecute(I4C3DContext* pContext, PCTSTR szCommand);

	void ModKeyDown(void);
	void ModKeyUp(void);
};

