#pragma once

class CLab2Dlg : public CDialogEx
{
public:
	CLab2Dlg(CWnd* pParent = NULL);

	enum { IDD = IDD_LAB2_DIALOG };
	
	// Dlg -> app
	afx_msg void OnBnClickedButtonSend();

	// App -> dlg
	void AddMessage(const CString & message);
	void UpdateNicknameList(const std::vector<CString> & users);

	virtual void OnOK();
private:
	virtual void DoDataExchange(CDataExchange* pDX);

	HICON m_hIcon;

	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	
	DECLARE_MESSAGE_MAP()
};
