#pragma once
#include "I4C3DModulesDefs.h"
#include "I4C3DCommon.h"

class I4C3DControl
{
public:

	I4C3DControl(void);
	~I4C3DControl(void);
	
	BOOL Initialize(I4C3DContext* pContext, char cTermination);
	void UnInitialize(void);
	void Execute(I4C3DUDPPacket* pPacket, int commandLen);
	//void Execute(LPCSTR szCommand, int commandLen);

private:
	// GUIÇ©ÇÁÇÃïœçXéÛït
	//void ResetModKeysAndExpansionRate(BOOL bCtrl, BOOL bAlt, BOOL bShift,
	//	DOUBLE tumbleRate, DOUBLE trackRate, DOUBLE dollyRate);
	BOOL m_bInitialized;
};

