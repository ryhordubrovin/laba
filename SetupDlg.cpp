#include "StdAfx.h"
#include "SetupDlg.h"
#include "Lab2App.h"


BEGIN_MESSAGE_MAP(CSetupDlg, CDialogEx)
	ON_WM_NCDESTROY()
	ON_BN_CLICKED(IDC_MULTICAST_CHECK, &CSetupDlg::OnBnClickedMulticastCheck)
END_MESSAGE_MAP()

CSetupDlg::CSetupDlg(CWnd * pParent)
	: base_t(IDD, pParent)
{
}

CString CSetupDlg::GetNickname() const
{
	return m_nickname;
}

bool CSetupDlg::IsMulticastEnabled() const
{
	return m_bMulticast;
}

DWORD CSetupDlg::GetMulticastAddr() const
{
	return m_dwMulticastAddr;
}

void CSetupDlg::OnNcDestroy()
{
	theApp.m_pMainWnd = nullptr;
	base_t::OnNcDestroy();
}

BOOL CSetupDlg::OnInitDialog()
{
	BOOL bRetval = base_t::OnInitDialog();
	OnBnClickedMulticastCheck();
	CWnd * pEdit = GetDlgItem(IDC_NICKNAME_EDIT);
	wchar_t buffer[MAX_PATH];
	DWORD dwLen = MAX_PATH;
	if (GetComputerNameW(buffer, &dwLen) != SOCKET_ERROR)
		pEdit->SetWindowTextW(buffer);
	else
		pEdit->SetWindowTextW(L"Default");
	return bRetval;
}

void CSetupDlg::OnOK()
{
	CWnd * pEdit = GetDlgItem(IDC_NICKNAME_EDIT);
	pEdit->GetWindowTextW(m_nickname);

	CButton * check = (CButton *)GetDlgItem(IDC_MULTICAST_CHECK);
	m_bMulticast = check->GetCheck() != 0;

	m_dwMulticastAddr = 0;
	CIPAddressCtrl * ipEdit = (CIPAddressCtrl *)GetDlgItem(IDC_IPADDRESS_FOR_MULTICAST_EDIT);
	if ((!m_bMulticast || ipEdit->GetAddress(m_dwMulticastAddr) == 4) && !m_nickname.IsEmpty())
	{
		m_dwMulticastAddr = ::htonl(m_dwMulticastAddr);
		base_t::OnOK();
	}
}

void CSetupDlg::OnBnClickedMulticastCheck()
{
	CButton * check = (CButton *)GetDlgItem(IDC_MULTICAST_CHECK);
	CIPAddressCtrl * ipEdit = (CIPAddressCtrl *)GetDlgItem(IDC_IPADDRESS_FOR_MULTICAST_EDIT);
	ipEdit->EnableWindow(check->GetCheck());
}
