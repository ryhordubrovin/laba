#include "stdafx.h"
#include "lab2App.h"
#include "lab2Dlg.h"
#include "SetupDlg.h"
#include "Chat.h"


BEGIN_MESSAGE_MAP(CLab2App, CWinApp)
	ON_COMMAND(ID_HELP, &CWinApp::OnHelp)
END_MESSAGE_MAP()

CLab2App::CLab2App()
{
	m_dwRestartManagerSupportFlags = AFX_RESTART_MANAGER_SUPPORT_RESTART;
}


CLab2App theApp;

BOOL CLab2App::InitInstance()
{
	WSADATA wsaData = { 0 };
	::WSAStartup(MAKEWORD(2, 2), &wsaData);

	INITCOMMONCONTROLSEX InitCtrls;
	InitCtrls.dwSize = sizeof(InitCtrls);
	InitCtrls.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&InitCtrls);

	::AfxInitRichEdit2();

	CWinApp::InitInstance();

	CShellManager *pShellManager = new CShellManager;

	CSetupDlg setupDlg;
	m_pMainWnd = &setupDlg;
	INT_PTR nResponse = setupDlg.DoModal();
	if (nResponse != IDOK)
		return FALSE;

	m_pMainWnd = m_pDlg.get();
	m_pChat.reset(new CChat(setupDlg.GetNickname(), setupDlg.IsMulticastEnabled(), setupDlg.GetMulticastAddr()));
	m_pDlg.reset(new CLab2Dlg());
	m_pDlg->DoModal();

	if (pShellManager)
		delete pShellManager;

	::WSACleanup();
	return FALSE;
}

void CLab2App::Send(const CString & message)
{
	m_pChat->Send(message);
}

void CLab2App::UpdateNicknameList(const std::vector<CString> & names)
{
	m_pDlg->UpdateNicknameList(names);
}

void CLab2App::AddMessage(const CString & message)
{
	m_pDlg->AddMessage(message);
}

CString CLab2App::GetInterfacesInfo() const
{
	ULONG uSize = 0x10000;
	std::vector<BYTE> buffer(uSize);
	PIP_ADAPTER_ADDRESSES pAddresses = (PIP_ADAPTER_ADDRESSES)&buffer[0];
	if (ERROR_SUCCESS != ::GetAdaptersAddresses(AF_INET, 0, nullptr, pAddresses, &uSize))
		return L"Unknown";

	CString retval;
	while (pAddresses)
	{
		retval += L"Name: ";
		retval += pAddresses->FriendlyName;
		retval += L"\n";
		retval += L"Addr: ";
		wchar_t buffer[20];
		SOCKADDR_IN * pAddr = (SOCKADDR_IN *)pAddresses->FirstUnicastAddress->Address.lpSockaddr;
		retval += InetNtopW(AF_INET, &pAddr->sin_addr, buffer, 20);
		retval += "/";
		retval += std::to_string(pAddresses->FirstUnicastAddress->OnLinkPrefixLength).c_str();
		retval += "\n\n";
		pAddresses = pAddresses->Next;
	}
	return retval;
}
