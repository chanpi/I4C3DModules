#pragma once
//#include <map>
#include "I4C3DModulesDefs.h"
//#include "VirtualMotion.h"

class I4C3DControl
{
public:

	I4C3DControl(void);
	~I4C3DControl(void);
	
	// TumbleExecute/TrackExecute/DollyExecute/HotkeyExecute�ւ̎d�������s��
	void Execute(LPCSTR szCommand, int commandLen);

	// GUI����̕ύX��t
	//void ResetModKeysAndExpansionRate(BOOL bCtrl, BOOL bAlt, BOOL bShift,
	//	DOUBLE tumbleRate, DOUBLE trackRate, DOUBLE dollyRate);
	BOOL Initialize(I4C3DContext* pContext, char cTermination);
	void UnInitialize(void);
};

