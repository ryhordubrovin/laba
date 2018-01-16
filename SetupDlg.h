#pragma once
#include "Resource.h"

class CSetupDlg 
	: public CDialogEx
{
	typedef CDialogEx base_t;
public:
	CSetupDlg(CWnd * pParent = nullptr);

	enum { IDD = IDD_SETUP_DIALOG };

	CString GetNickname() const;
	bool IsMulticastEnabled() const;
	DWORD GetMulticastAddr() const;

	void OnNcDestroy();
	virtual BOOL OnInitDialog() override;
	virtual void OnOK();

	afx_msg void OnBnClickedMulticastCheck();

private:
	CString m_nickname;
	bool m_bMulticast;
	DWORD m_dwMulticastAddr;

	DECLARE_MESSAGE_MAP()
};

