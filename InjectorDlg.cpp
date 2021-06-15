
// InjectorDlg.cpp : implementation file
//

#include "pch.h"
#include "framework.h"
#include "Injector.h"
#include "InjectorDlg.h"
#include "afxdialogex.h"

#include <fstream>
#include <filesystem>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CInjectorDlg dialog
constexpr char* iniFile = "DLL Injector.ini";


CInjectorDlg::CInjectorDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_INJECTOR_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CInjectorDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDIT_DLL_PATH, m_dllPathEdit);
	DDX_Control(pDX, IDC_EDIT_TARGET_EXE, m_targetExeEdit);
}

BEGIN_MESSAGE_MAP(CInjectorDlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDSELECT, &CInjectorDlg::OnBnClickedSelect)
	ON_BN_CLICKED(IDINJECT, &CInjectorDlg::OnBnClickedInject)
END_MESSAGE_MAP()


// CInjectorDlg message handlers

BOOL CInjectorDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	//Create the ToolTip control
	if (!m_ToolTip.Create(this))
	{
		TRACE0("Unable to create the ToolTip!");
	}
	else
	{
		m_ToolTip.AddTool(&m_targetExeEdit, _T("Case sensitive!"));
		m_ToolTip.Activate(TRUE);
	}

	// Remembering last fields
	std::ifstream file;
	file.open(iniFile);
	if (file)
	{
		std::string line;
		std::getline(file, line);
		m_dllPathEdit.SetWindowTextA(line.c_str());
		std::getline(file, line);
		m_targetExeEdit.SetWindowTextA(line.c_str());
	}

	return TRUE;  // return TRUE  unless you set the focus to a control
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CInjectorDlg::OnPaint()
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

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CInjectorDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



void CInjectorDlg::OnBnClickedSelect()
{
	const TCHAR szFilter[] = _T("DLL Files (*.dll)|*.dll|All Files (*.*)|*.*||");
	CFileDialog dlg(TRUE, _T("csv"), nullptr, OFN_HIDEREADONLY, szFilter, this);
	if (dlg.DoModal() == IDOK)
	{
		CString sFilePath = dlg.GetPathName();
		m_dllPathEdit.SetWindowText(sFilePath);
	}
}

void CInjectorDlg::OnBnClickedInject()
{
	CString process;
	m_targetExeEdit.GetWindowText(process);
	
	if (process.IsEmpty())
	{
		MessageBox("Insert an executable name!", "Incomplete parameters!", MB_ICONINFORMATION);
		return;
	}

	CString dllPathStr;
	m_dllPathEdit.GetWindowText(dllPathStr);
	std::ifstream dllFile(dllPathStr);
	if (!dllFile.good())
	{
		MessageBox("Please select a valid DLL file to proceed!", "Incomplete parameters!", MB_ICONINFORMATION);
		return;
	}

	DWORD dwProcess;
	char myDLL[MAX_PATH];
	strcpy_s(myDLL, CStringA(dllPathStr).GetString());

	// get process
	dwProcess = this->getProcess(process);
	if (!dwProcess)
	{
		const CString msg("The process '" + process + "' was not found!");
		MessageBox(msg, "Process not found!", MB_ICONEXCLAMATION);
		return;
	}

	// open handle and allocate memory
	m_hProcess = OpenProcess(PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION | PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION, FALSE, dwProcess);
	m_allocatedMem = VirtualAllocEx(m_hProcess, nullptr, sizeof(myDLL), MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

	if (m_allocatedMem == nullptr) {
		MessageBox("Error allocating memory!", "Error!", MB_ICONERROR);
		return;
	}

	// actually "inject" DLL into process memory
	BOOL wpRes = WriteProcessMemory(m_hProcess, m_allocatedMem, myDLL, sizeof(myDLL), nullptr);
	if (!wpRes)
	{
		MessageBox("Error writing process to memory!", "Error!", MB_ICONERROR);
		return;
	}

	// launch DLL
	HANDLE remoteThread = CreateRemoteThread(m_hProcess, nullptr, 0, (LPTHREAD_START_ROUTINE)LoadLibrary, m_allocatedMem, 0, nullptr);
	if (remoteThread == nullptr)
	{
		MessageBox("Error loading the DLL!", "Error!", MB_ICONERROR);
		return;
	}

	WaitForSingleObject(remoteThread, INFINITE);

	MessageBox("The DLL was injected successfully!", "Injected!", MB_ICONINFORMATION);

	// Remember the fields
	std::ofstream file;
	file.open(iniFile);
	file << myDLL << '\n';
	file << process << '\n';;
	file.close();
	
}

BOOL CInjectorDlg::PreTranslateMessage(MSG* pMsg)
{
	m_ToolTip.RelayEvent(pMsg);

	return CDialog::PreTranslateMessage(pMsg);
}

DWORD CInjectorDlg::getProcess(const char* processName)
{
	HANDLE hPID = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
	PROCESSENTRY32 procEntry;
	procEntry.dwSize = sizeof(procEntry);

	do {
		if (!strcmp(procEntry.szExeFile, processName)) {
			DWORD dwPID = procEntry.th32ProcessID;
			CloseHandle(hPID);

			return dwPID;
		}
	} while (Process32Next(hPID, &procEntry));

	return 0;
}
