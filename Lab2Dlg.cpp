#include "stdafx.h"
#include "Lab2App.h"
#include "lab2Dlg.h"

CLab2Dlg::CLab2Dlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CLab2Dlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CLab2Dlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CLab2Dlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON_SEND, &CLab2Dlg::OnBnClickedButtonSend)
END_MESSAGE_MAP()


BOOL CLab2Dlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	GetDlgItem(IDC_ABOUT)->SetWindowTextW(theApp.GetInterfacesInfo());

	return TRUE;
}

void CLab2Dlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

void CLab2Dlg::OnOK()
{
	OnBnClickedButtonSend();
}

HCURSOR CLab2Dlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CLab2Dlg::OnBnClickedButtonSend()
{
	CString text;
	GetDlgItem(IDC_MESSAGE_EDIT)->GetWindowTextW(text);
	theApp.Send(text);
	GetDlgItem(IDC_MESSAGE_EDIT)->SetWindowTextW(L"");
}

void CLab2Dlg::AddMessage(const CString & message)
{
	CRichEditCtrl * pCtrl = static_cast<CRichEditCtrl *>(GetDlgItem(IDC_CHATEDIT));
	CString text;
	pCtrl->GetWindowText(text);
	pCtrl->SetWindowTextW(text + message + "\n");
}

void CLab2Dlg::UpdateNicknameList(const std::vector<CString> & users)
{
	CString list;
	for (size_t i = 0; i < users.size(); ++i)
		list += users[i] + L"\n";

	GetDlgItem(IDC_NOW_ONLINE_EDIT)->SetWindowTextW(list);
}
