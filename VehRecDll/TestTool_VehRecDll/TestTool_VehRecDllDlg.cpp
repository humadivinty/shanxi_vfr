
// TestTool_VehRecDllDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "TestTool_VehRecDll.h"
#include "TestTool_VehRecDllDlg.h"
#include "afxdialogex.h"

#include <direct.h>
#include <Dbghelp.h>
#pragma comment(lib, "Dbghelp.lib")

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


#include "../VehRecDll/VehRecDll.h"
#ifdef DEBUG
#pragma comment(lib, "../debug/VehRecDll.lib")
#else
#pragma comment(lib, "../Release/VehRecDll.lib")
#endif

void* g_pDlg = NULL;

// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 对话框数据
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CTestTool_VehRecDllDlg 对话框



CTestTool_VehRecDllDlg::CTestTool_VehRecDllDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CTestTool_VehRecDllDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CTestTool_VehRecDllDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CTestTool_VehRecDllDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
    ON_BN_CLICKED(IDC_BUTTON_InitEx, &CTestTool_VehRecDllDlg::OnBnClickedButtonInitex)
    ON_BN_CLICKED(IDC_BUTTON3, &CTestTool_VehRecDllDlg::OnBnClickedButton3)
    ON_BN_CLICKED(IDC_BUTTON1, &CTestTool_VehRecDllDlg::OnBnClickedButton1)
    ON_BN_CLICKED(IDC_BUTTON2, &CTestTool_VehRecDllDlg::OnBnClickedButton2)
    ON_BN_CLICKED(IDC_BUTTON_VEHSignle, &CTestTool_VehRecDllDlg::OnBnClickedButtonVehsignle)
    ON_BN_CLICKED(IDC_BUTTON_DisConnect, &CTestTool_VehRecDllDlg::OnBnClickedButtonDisconnect)
    ON_BN_CLICKED(IDC_BUTTON_VehRec_Free, &CTestTool_VehRecDllDlg::OnBnClickedButtonVehrecFree)
    ON_BN_CLICKED(IDC_BUTTON_connectEx, &CTestTool_VehRecDllDlg::OnBnClickedButtonconnectex)
    ON_BN_CLICKED(IDC_BUTTON_GetCarData, &CTestTool_VehRecDllDlg::OnBnClickedButtonGetcardata)
	ON_WM_TIMER()
	ON_BN_CLICKED(IDC_BUTTON_BeginTimer, &CTestTool_VehRecDllDlg::OnBnClickedButtonBegintimer)
	ON_BN_CLICKED(IDC_BUTTON_ONECLickGetData, &CTestTool_VehRecDllDlg::OnBnClickedButtonOneclickgetdata)
END_MESSAGE_MAP()


// CTestTool_VehRecDllDlg 消息处理程序

BOOL CTestTool_VehRecDllDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO:  在此添加额外的初始化代码
    char chPath[256] = { 0 };
    _getcwd(chPath, sizeof(chPath));
    m_strCurrentDir = chPath;

    GetDlgItem(IDC_IPADDRESS1)->SetWindowTextA("172.18.81.105");

    GetDlgItem(IDC_EDIT_LogDir)->SetWindowTextA(m_strCurrentDir );
    GetDlgItem(IDC_EDIT_ResultDir)->SetWindowTextA(m_strCurrentDir);
    GetDlgItem(IDC_EDIT2)->SetWindowTextA("2");

    ((CComboBox*)GetDlgItem(IDC_COMBO_Log))->AddString( _T("0"));
    ((CComboBox*)GetDlgItem(IDC_COMBO_Log))->AddString( _T("1"));
    ((CComboBox*)GetDlgItem(IDC_COMBO_Log))->SetCurSel(1);

    ((CComboBox*)GetDlgItem(IDC_COMBO_Signal))->AddString(_T("0"));
    ((CComboBox*)GetDlgItem(IDC_COMBO_Signal))->AddString(_T("1"));
    ((CComboBox*)GetDlgItem(IDC_COMBO_Signal))->SetCurSel(0);

    g_pDlg = this;
    m_iCameraHandle = -1;
	GetDlgItem(IDC_BUTTON_BeginTimer)->SetWindowTextA("开启定时取结果");

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CTestTool_VehRecDllDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CTestTool_VehRecDllDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CTestTool_VehRecDllDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



void CTestTool_VehRecDllDlg::OnBnClickedButtonInitex()
{
    // TODO:  在此添加控件通知处理程序代码
    char chTemp[256] = {0};
    GetItemText(IDC_COMBO_Log, chTemp, sizeof(chTemp));
    int iLog = atoi(chTemp);

    char chLogDir[256] = {0};
    GetItemText(IDC_EDIT_LogDir, chLogDir, sizeof(chLogDir));

    memset(chTemp, '\0', sizeof(chTemp));
    GetItemText(IDC_EDIT2, chTemp, sizeof(chTemp));
    int iLogSaveDay = atoi(chTemp);

    int iRet = VehRec_InitEx(iLog, chLogDir, iLogSaveDay);
    char chLog[256] = {0};
    sprintf_s(chLog, sizeof(chLog), " %d = VehRec_InitEx(%d, %s, %d)", iRet, iLog, chLogDir, iLogSaveDay);
    ShowMessage(chLog);
}


void CTestTool_VehRecDllDlg::OnBnClickedButton3()
{
    // TODO:  在此添加控件通知处理程序代码
    char chIP[64] = {0};
    GetItemText(IDC_IPADDRESS1, chIP, sizeof(chIP));

    char chSaveDir[256] = {0};
    GetItemText(IDC_EDIT_ResultDir, chSaveDir, sizeof(chSaveDir));
    m_iCameraHandle = VehRec_Connect(chIP, chSaveDir);

    char chLog[256] = { 0 };
    sprintf_s(chLog, sizeof(chLog), " %d = VehRec_Connect(%s, %s)", m_iCameraHandle, chIP, chSaveDir);
    ShowMessage(chLog);
}


void CTestTool_VehRecDllDlg::OnBnClickedButton1()
{
    // TODO:  在此添加控件通知处理程序代码

    CString         strFolderPath = ShowDialgToChooseDir();
    GetDlgItem(IDC_EDIT_LogDir)->SetWindowText(strFolderPath);
}


void CTestTool_VehRecDllDlg::OnBnClickedButton2()
{
    // TODO:  在此添加控件通知处理程序代码
    CString         strFolderPath = ShowDialgToChooseDir();
    GetDlgItem(IDC_EDIT_ResultDir)->SetWindowText(strFolderPath);
}


void CTestTool_VehRecDllDlg::OnBnClickedButtonVehsignle()
{
    // TODO:  在此添加控件通知处理程序代码
    char chTemp[256] = { 0 };
    GetItemText(IDC_COMBO_Signal, chTemp, sizeof(chTemp));
    int iSignal = atoi(chTemp);
    int iRet = VehRec_VEHSignle(m_iCameraHandle, iSignal);

    char chLog[256] = { 0 };
    sprintf_s(chLog, sizeof(chLog), " %d = VehRec_VEHSignle(%d,%d)", iRet, m_iCameraHandle, iSignal);
    ShowMessage(chLog);
}


void CTestTool_VehRecDllDlg::OnBnClickedButtonDisconnect()
{
    // TODO:  在此添加控件通知处理程序代码
    VehRec_DisConnect(m_iCameraHandle);

    char chLog[256] = { 0 };
    sprintf_s(chLog, sizeof(chLog), "VehRec_DisConnect(%d) finish",  m_iCameraHandle);
    ShowMessage(chLog);
}


void CTestTool_VehRecDllDlg::OnBnClickedButtonVehrecFree()
{
    // TODO:  在此添加控件通知处理程序代码
    int iRet = VehRec_Free();

    char chLog[256] = { 0 };
    sprintf_s(chLog, sizeof(chLog), "%d = VehRec_Free()", iRet);
    ShowMessage(chLog);
}

bool CTestTool_VehRecDllDlg::GetItemText(int ItemID, char* buffer, size_t bufSize)
{
    CString strTemp;
    GetDlgItem(ItemID)->GetWindowText(strTemp);
    if (strTemp.GetLength() < bufSize)
    {
        //sprintf(buffer, "%s", strTemp.GetBuffer());
        sprintf_s(buffer, bufSize, "%s", strTemp.GetBuffer());
        strTemp.ReleaseBuffer();
        return true;
    }
    return false;
}

void CTestTool_VehRecDllDlg::ShowMsg(CEdit *pEdit, CString strMsg)
{
    if (pEdit == NULL)
    {
        return;
    }

    CTime  time = CTime::GetCurrentTime();
    CString strTmp;
    pEdit->GetWindowText(strTmp);
    if (strTmp.IsEmpty() || strTmp.GetLength() > 4096)
    {
        strTmp = time.Format(_T("[%Y-%m-%d %H:%M:%S] "));
        strTmp += strMsg;
        pEdit->SetWindowText(strTmp);
        return;
    }

    strTmp += _T("\r\n");
    strTmp += time.Format(_T("[%Y-%m-%d %H:%M:%S] "));
    strTmp += strMsg;
    pEdit->SetWindowText(strTmp);

    if (pEdit != NULL)
    {
        pEdit->LineScroll(pEdit->GetLineCount() - 1);
    }
}

void CTestTool_VehRecDllDlg::ShowMessage(CString strMsg)
{
    CEdit *pEdit = (CEdit*)GetDlgItem(IDC_EDIT_MSG);
    ShowMsg(pEdit, strMsg);
}

CString CTestTool_VehRecDllDlg::ShowDialgToChooseDir()
{
    TCHAR           szFolderPath[MAX_PATH] = { 0 };
    CString         strFolderPath = TEXT("");

    BROWSEINFO      sInfo;
    ::ZeroMemory(&sInfo, sizeof(BROWSEINFO));
    sInfo.pidlRoot = 0;
    sInfo.lpszTitle = _T("请选择处理结果存储路径");
    sInfo.ulFlags = BIF_RETURNONLYFSDIRS | BIF_EDITBOX | BIF_DONTGOBELOWDOMAIN;
    sInfo.lpfn = NULL;

    // 显示文件夹选择对话框  
    LPITEMIDLIST lpidlBrowse = ::SHBrowseForFolder(&sInfo);
    if (lpidlBrowse != NULL)
    {
        // 取得文件夹名  
        if (::SHGetPathFromIDList(lpidlBrowse, szFolderPath))
        {
            strFolderPath = szFolderPath;
        }
    }
    if (lpidlBrowse != NULL)
    {
        ::CoTaskMemFree(lpidlBrowse);
    }

    return strFolderPath;
}

int CALLBACK CTestTool_VehRecDllDlg::_VehRec_CarData_callback(int handle, char *colpic, char *platepic, char *recfile)
{
    CTestTool_VehRecDllDlg* pDailg = (CTestTool_VehRecDllDlg*)g_pDlg;
    if (pDailg == NULL)
    {
        return -1;
    }

    char chPath[256] = { 0 };
    _getcwd(chPath, sizeof(chPath));

    strcat_s(chPath, "\\callbackResult\\");
    MakeSureDirectoryPathExists(chPath);

    SYSTEMTIME systime;
    GetLocalTime(&systime);//本地时间

    char chFileName[256] = { 0 };
    sprintf_s(chFileName, sizeof(chFileName), "%d_%04d%02d%02d%02d%02d%02d_%03d.txt",
        handle,
        systime.wYear,
        systime.wMonth,
        systime.wDay,
        systime.wHour,
        systime.wMinute,
        systime.wSecond,
        systime.wMilliseconds);

    strcat_s(chPath, chFileName);

    CString strLog;
    strLog.Format("Callback data :\n ");
    pDailg->ShowMessage(strLog);

    strLog.Empty();
    strLog.Format("handle = %d \ncolpic=%s \nplatepic=%s \nrecfile=%s ",
        handle,
        colpic,
        platepic,
        recfile);
    pDailg->ShowMessage(strLog);

    FILE* pFile = NULL;
    fopen_s(&pFile, chPath, "a");

    if (pFile)
    {
        fprintf_s(pFile, "%s", strLog.GetBuffer());
        strLog.ReleaseBuffer();
        fclose(pFile);
        pFile = NULL;
    }
    return 0;
}


void CTestTool_VehRecDllDlg::OnBnClickedButtonconnectex()
{
    // TODO:  在此添加控件通知处理程序代码
    char chIP[64] = { 0 };
    GetItemText(IDC_IPADDRESS1, chIP, sizeof(chIP));

    char chSaveDir[256] = { 0 };
    GetItemText(IDC_EDIT_ResultDir, chSaveDir, sizeof(chSaveDir));
    m_iCameraHandle = VehRec_ConnectEX(chIP, chSaveDir, _VehRec_CarData_callback);

    char chLog[256] = { 0 };
    sprintf_s(chLog, sizeof(chLog), " %d = VehRec_ConnectEX(%s, %s, %p)", m_iCameraHandle, chIP, chSaveDir, _VehRec_CarData_callback);
    ShowMessage(chLog);
}


void CTestTool_VehRecDllDlg::OnBnClickedButtonGetcardata()
{
    // TODO:  在此添加控件通知处理程序代码
    char chSideImagePath[256] = {0};
    char chTailImagePath[256] = { 0 };
    char chVideoPath[256] = { 0 };

	SYSTEMTIME systime;
	GetLocalTime(&systime);//本地时间
	char chTimeNow[64] = {0};
	sprintf_s(chTimeNow, sizeof(chTimeNow),  "%04u%02u%02u%02u%02u%02u%03u",
		systime.wYear,
		systime.wMonth,
		systime.wDay,
		systime.wHour,
		systime.wMinute,
		systime.wSecond,
		systime.wMilliseconds);

    CHAR szPath[256] = { 0 };
    //getcwd(szPath,sizeof(szPath));
    memset(szPath, '\0', sizeof(szPath));
    _getcwd(szPath, sizeof(szPath));

	sprintf_s(chSideImagePath, sizeof(chSideImagePath), "%s\\Result\\%04u%02u%02u\\%s_side.jpg",
		szPath, 
		systime.wYear,
		systime.wMonth,
		systime.wDay,
		chTimeNow);
	sprintf_s(chTailImagePath, sizeof(chTailImagePath), "%s\\Result\\%04u%02u%02u\\%s_tail.jpg",
		szPath,
		systime.wYear,
		systime.wMonth,
		systime.wDay,
		chTimeNow);
	sprintf_s(chVideoPath, sizeof(chVideoPath), "%s\\Result\\%04u%02u%02u\\%s_car.mp4",
		szPath,
		systime.wYear,
		systime.wMonth,
		systime.wDay,
		chTimeNow);

    int iRet = VehRec_GetCarData(m_iCameraHandle, chSideImagePath, chTailImagePath, chVideoPath);

    char chLog[1024] = { 0 };
    sprintf_s(chLog, sizeof(chLog), " %d = VehRec_GetCarData(%d) , colpic = %s, platepic = %s, recfile = %s", 
        iRet,
        m_iCameraHandle,
        chSideImagePath,
        chTailImagePath, 
        chVideoPath);
    ShowMessage(chLog);
}


void CTestTool_VehRecDllDlg::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	switch (nIDEvent)
	{
	case 1:
		OnBnClickedButtonOneclickgetdata();
		break;
	default:
		break;
	}

	CDialogEx::OnTimer(nIDEvent);
}


void CTestTool_VehRecDllDlg::OnBnClickedButtonBegintimer()
{
	// TODO: 在此添加控件通知处理程序代码
	CString strText;
	GetDlgItem(IDC_BUTTON_BeginTimer)->GetWindowText(strText);
	if (-1 != strText.Find("开启"))
	{
		SetTimer(1, 5000, NULL);
		GetDlgItem(IDC_BUTTON_BeginTimer)->SetWindowTextA("关闭定时取结果");
	}
	else
	{
		KillTimer(1);
		GetDlgItem(IDC_BUTTON_BeginTimer)->SetWindowTextA("开启定时取结果");
	}	
}


void CTestTool_VehRecDllDlg::OnBnClickedButtonOneclickgetdata()
{
	// TODO:  在此添加控件通知处理程序代码
	char chLog[256] = { 0 };
	int iSignal = 1;
	int iRet = VehRec_VEHSignle(m_iCameraHandle, iSignal);
	sprintf_s(chLog, sizeof(chLog), " %d = VehRec_VEHSignle(%d,%d)", iRet, m_iCameraHandle, iSignal);
	ShowMessage(chLog);

	iSignal = 2;
	iRet = VehRec_VEHSignle(m_iCameraHandle, iSignal);
	sprintf_s(chLog, sizeof(chLog), " %d = VehRec_VEHSignle(%d,%d)", iRet, m_iCameraHandle, iSignal);
	ShowMessage(chLog);

	iSignal = 2;
	iRet = VehRec_VEHSignle(m_iCameraHandle, iSignal);
	sprintf_s(chLog, sizeof(chLog), " %d = VehRec_VEHSignle(%d,%d)", iRet, m_iCameraHandle, iSignal);
	ShowMessage(chLog);

	OnBnClickedButtonGetcardata();
}
