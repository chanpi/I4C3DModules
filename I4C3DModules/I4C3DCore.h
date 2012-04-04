#pragma once

#include "I4C3DModulesDefs.h"

class I4C3DCore
{
public:
	I4C3DCore(void);
	~I4C3DCore(void);

	BOOL Start(I4C3DContext* pContext);
	void Stop(I4C3DContext* pContext);

private:
	BOOL Initialize(I4C3DContext* pContext);
	BOOL InitializeMainContext(I4C3DContext* pContext);
	//BOOL InitializeProcessorContext(I4C3DContext* pContext);
	void UnInitialize(I4C3DContext* pContext);
};

