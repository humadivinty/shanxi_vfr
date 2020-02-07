
// TestTool_VehRecDllDlg.h : ͷ�ļ�
//

#pragma once


// CTestTool_VehRecDllDlg �Ի���
class CTestTool_VehRecDllDlg : public CDialogEx
{
// ����
public:
	CTestTool_VehRecDllDlg(CWnd* pParent = NULL);	// ��׼���캯��

// �Ի�������
	enum { IDD = IDD_TESTTOOL_VEHRECDLL_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV ֧��


// ʵ��
protected:
	HICON m_hIcon;

	// ���ɵ���Ϣӳ�亯��
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
