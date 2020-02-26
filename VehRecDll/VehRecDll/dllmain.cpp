// dllmain.cpp : ���� DLL Ӧ�ó������ڵ㡣
#include "stdafx.h"
#include "coredump/MiniDumper.h"
#include "utilityTool/ToolFunction.h"
//#include "utilityTool/log4z.h"
//using namespace zsummer::log4z;

CMiniDumper g_coredump(true);

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
    TCHAR szFileName[MAX_PATH] = { 0 };
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
        GetModuleFileName(hModule, szFileName, MAX_PATH);	//ȡ�ð�����������ȫ·��
        PathRemoveFileSpec(szFileName);				//ȥ��������
        Tool_SetDllDirPath(szFileName);

		//ILog4zManager::getRef().setLoggerPath(LOG4Z_MAIN_LOGGER_ID, "./XLWLog/");
		//ILog4zManager::getRef().setLoggerMonthdir(LOG4Z_MAIN_LOGGER_ID, true);
		//ILog4zManager::getRef().start();
        break;
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}

