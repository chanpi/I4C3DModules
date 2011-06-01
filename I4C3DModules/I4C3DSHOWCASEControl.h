#pragma once
#include "i4c3dcontrol.h"
class I4C3DSHOWCASEControl :
	public I4C3DControl
{
public:
	I4C3DSHOWCASEControl(void);
	~I4C3DSHOWCASEControl(void);

	BOOL GetTargetChildWnd(void);

	void TumbleExecute(int deltaX, int deltaY);
	void TrackExecute(int deltaX, int deltaY);
	void DollyExecute(int deltaX, int deltaY);
	void HotkeyExecute(I4C3DContext* pContext, PCTSTR szCommand);

	void ModKeyDown(void);
	void ModKeyUp(void);

private:
		//BOOL CheckTargetState(void);
};

