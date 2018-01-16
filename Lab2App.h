#pragma once
#include "resource.h"		// main symbols

class CChat;
class CLab2Dlg;
class CLab2App : public CWinApp
{
public:
	CLab2App();

public:
	virtual BOOL InitInstance();
	void Send(const CString & message);

	void UpdateNicknameList(const std::vector<CString> & names);
	void AddMessage(const CString & message);
	
	CString GetInterfacesInfo() const;

	DECLARE_MESSAGE_MAP();

private:
	std::unique_ptr<CLab2Dlg> m_pDlg;
	std::unique_ptr<CChat> m_pChat;
};

extern CLab2App theApp;