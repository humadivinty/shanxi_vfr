// stdafx.h : ��׼ϵͳ�����ļ��İ����ļ���
// ���Ǿ���ʹ�õ��������ĵ�
// �ض�����Ŀ�İ����ļ�
//

#pragma once

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             //  �� Windows ͷ�ļ����ų�����ʹ�õ���Ϣ
// Windows ͷ�ļ�: 
#include <windows.h>



// TODO:  �ڴ˴����ó�����Ҫ������ͷ�ļ�
#include <memory>

#include <Dbghelp.h>
#pragma comment(lib, "Dbghelp.lib")
#include <Shlwapi.h>
#pragma comment(lib, "Shlwapi.lib")

#include<ctime>
#include<atltime.h>

#define DLL_VERSION "2020042401"
#define INI_FILE_NAME "\\VehRecDll_XLW.ini"
#define DLL_LOG_NAME "VehRecDll_XLW.log"
#define  LOG_DIR_NAME "XLWLog"
#define  RESULT_DIR_NAME "Buffer"

#ifdef WINXP
#define MY_SPRINTF sprintf
#else
#define MY_SPRINTF sprintf_s
#endif

#define  CAM_COUNT 10

#define BIN_IMG_SIZE 280
#define COMPRESS_PLATE_IMG_SIZE 5*1024
#define COMPRESS_BIG_IMG_SIZE 100*1024
#define WINDOWS

#define USE_LAST_RESULT 1

#define RESULT_MODE_FRONT 1
#define RESULT_MODE_TAIL 2