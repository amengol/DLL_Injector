
// InjectorDlg.h : header file
//

#pragma once


// CInjectorDlg dialog
class CInjectorDlg : public CDialogEx
{
// Construction
public:
	CInjectorDlg(CWnd* pParent = nullptr);	// standard constructor

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_INJECTOR_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
	afx_msg void OnBnClickedSelect();
	CEdit m_dllPathEdit;
	CEdit m_targetExeEdit;
	afx_msg void OnBnClickedInject();
	CToolTipCtrl m_ToolTip;
	BOOL PreTranslateMessage(MSG* pMsg) override;
private:
	DWORD getProcess(const char* processName);
	HANDLE m_hProcess = nullptr;
	LPVOID m_allocatedMem = nullptr;
};
