// stdafx.h : 标准系统包含文件的包含文件，
// 或是经常使用但不常更改的
// 特定于项目的包含文件
//

#pragma once

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             //  从 Windows 头文件中排除极少使用的信息
// Windows 头文件: 
#include <windows.h>



// TODO:  在此处引用程序需要的其他头文件
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