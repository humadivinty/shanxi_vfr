
// TestTool_VehRecDllDlg.h : 头文件
//

#pragma once


// CTestTool_VehRecDllDlg 对话框
class CTestTool_VehRecDllDlg : public CDialogEx
{
// 构造
public:
	CTestTool_VehRecDllDlg(CWnd* pParent = NULL);	// 标准构造函数

// 对话框数据
	enum { IDD = IDD_TESTTOOL_VEHRECDLL_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持


// 实现
protected:
	HICON m_hIcon;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
    afx_msg void OnBnClickedButtonInitex();
    afx_msg void OnBnClickedButton3();
    afx_msg void OnBnClickedButton1();
    afx_msg void OnBnClickedButton2();
    afx_msg void OnBnClickedButtonVehsignle();
    afx_msg void OnBnClickedButtonDisconnect();
    afx_msg void OnBnClickedButtonVehrecFree();

    bool GetItemText(int ItemID, char* buffer, size_t bufSize);
    void ShowMsg(CEdit *pEdit, CString strMsg);
    void ShowMessage(CString strMsg);

    CString ShowDialgToChooseDir();

    static int CALLBACK  _VehRec_CarData_callback(int handle, char *colpic, char *platepic, char *recfile);

private:
    int m_iCameraHandle;
    CString m_strCurrentDir;
public:
    afx_msg void OnBnClickedButtonconnectex();
    afx_msg void OnBnClickedButtonGetcardata();
};
