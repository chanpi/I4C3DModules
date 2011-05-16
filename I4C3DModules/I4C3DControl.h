#pragma once
#include <map>
#include "I4C3DModulesDefs.h"
#include "VirtualMotion.h"

class I4C3DControl
{
public:
	I4C3DControl(void);
	virtual ~I4C3DControl(void);
	
	// TumbleExecute/TrackExecute/DollyExecute/HotkeyExecuteÇ÷ÇÃédï™ÇØÇçsÇ§
	void Execute(I4C3DContext* pContext, PPOINT pDelta, LPCSTR szCommand);

	virtual BOOL Initialize(I4C3DContext* pContext);
	virtual BOOL GetTargetChildWnd(void)	= 0;
	virtual void ModKeyDown(void)			= 0;
	virtual void ModKeyUp(void)				= 0;

	virtual void TumbleExecute(int deltaX, int deltaY);
	virtual void TrackExecute(int deltaX, int deltaY);
	virtual void DollyExecute(int deltaX, int deltaY);
	virtual void HotkeyExecute(I4C3DContext* pContext, PCTSTR szCommand);

protected:
	BOOL InitializeModifierKeys(I4C3DContext* pContext);
	BOOL IsModKeysDown(void);
	virtual void ModKeyDown(HWND hTargetWnd);
	virtual void ModKeyUp(HWND hTargetWnd);

	void HotkeyExecute(I4C3DContext* pContext, HWND hTargetWnd, PCTSTR szCommand);

	HWND m_hTargetParentWnd;
	HWND m_hTargetChildWnd;
	POINT m_currentPos;
	BOOL m_ctrl;
	BOOL m_alt;
	BOOL m_shift;
	BOOL m_bUsePostMessage;
	BOOL m_bSyskeyDown;
	int m_DisplayWidth;
	int m_DisplayHeight;
	DWORD m_millisecSleepAfterKeyDown;
	DOUBLE m_fTumbleRate;
	DOUBLE m_fTrackRate;
	DOUBLE m_fDollyRate;

private:
	void AdjustCursorPOS(PPOINT pPos);
	BOOL CheckTargetState(void);
};

