
// TestTool_VehRecDll.h : PROJECT_NAME Ӧ�ó������ͷ�ļ�
//

#pragma once

#ifndef __AFXWIN_H__
	#error "�ڰ������ļ�֮ǰ������stdafx.h�������� PCH �ļ�"
#endif

#include "resource.h"		// ������


// CTestTool_VehRecDllApp: 
// �йش����ʵ�֣������ TestTool_VehRecDll.cpp
//

class CTestTool_VehRecDllApp : public CWinApp
{
public:
	CTestTool_VehRecDllApp();

// ��д
public:
	virtual BOOL InitInstance();

// ʵ��

	DECLARE_MESSAGE_MAP()
};

extern CTestTool_VehRecDllApp theApp;