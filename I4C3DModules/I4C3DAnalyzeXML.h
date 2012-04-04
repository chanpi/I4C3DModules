#pragma once

class I4C3DAnalyzeXML
{
public:
	I4C3DAnalyzeXML(void);
	~I4C3DAnalyzeXML(void);

	// �������R���X�g���N�^���Ă΂Ȃ��ꍇ�́A�ʓr�Ăяo�����Ƃ��ł��܂��B
	BOOL LoadXML(PCTSTR szXMLUri);

	PCTSTR GetGlobalValue(PCTSTR szKey);
	PCTSTR GetSoftValue(PCTSTR szSoftName, PCTSTR szKey);

private:
	BOOL ReadGlobalTag(void);
	BOOL ReadSoftsTag(void);
};

