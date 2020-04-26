#include "stdafx.h"
#include <WinSock2.h>
#include<math.h>
#include<shellapi.h>
#include <Dbghelp.h>
#include "BaseCamera.h"
#include "tinyxml/tinyxml.h"
#include "HvDevice/HvDeviceBaseType.h"
#include "HvDevice/HvDeviceCommDef.h"
#include "HvDevice/HvDeviceNew.h"
#include "HvDevice/HvCamera.h"
#pragma comment(lib, "HvDevice/HvDevice.lib")
#define  VEHICLE_LISTEN_PORT 99999

#include "utilityTool/ToolFunction.h"
#include<ctime>

#define  BUFFERLENTH 256

BaseCamera::BaseCamera() :
m_hHvHandle(NULL),
m_hWnd(NULL),
m_iMsg(0),
m_iConnectMsg(0x402),
m_iDisConMsg(0x403),
m_iConnectStatus(0),
m_iLoginID(0),
m_iCompressQuality(20),
m_iDirection(0),
m_iIndex(0),
m_iVideoDelayTime(2),
m_iLogHoldDay(30),
m_iJpegCount(0),
m_bLogEnable(true),
m_bVideoLogEnable(false),
m_bSynTime(true),
  m_bFirstH264Frame(true),
  m_bJpegComplete(false),
m_strIP("")
{
    //合成图片初始化
//    Tool_GetEncoderClsid(L"image/jpeg", &m_jpgClsid);
//    Tool_GetEncoderClsid(L"image/bmp", &m_bmpClsid);
    InitializeCriticalSection(&m_csLog);
    InitializeCriticalSection(&m_csFuncCallback);

    memset(m_chLogPath, '\0', sizeof(m_chLogPath));
    //ReadConfig();
    m_h264Saver.SetLogEnable(m_bVideoLogEnable);
}

BaseCamera::BaseCamera(const char* chIP, HWND  hWnd, int Msg) :
m_hHvHandle(NULL),
m_hWnd(hWnd),
m_iMsg(Msg),
m_iConnectMsg(0x402),
m_iDisConMsg(0x403),
m_iConnectStatus(0),
m_iLoginID(0),
m_iCompressQuality(20),
m_iDirection(0),
m_iIndex(0),
m_iVideoDelayTime(2),
m_iLogHoldDay(30),
m_iJpegCount(0),
m_bLogEnable(true),
m_bVideoLogEnable(false),
m_bSynTime(true),
  m_bFirstH264Frame(true),
  m_bJpegComplete(false),
m_strIP(chIP)
{
    InitializeCriticalSection(&m_csLog);
    InitializeCriticalSection(&m_csFuncCallback);
	InitializeCriticalSection(&m_csResult);
    //合成图片初始化
//    Tool_GetEncoderClsid(L"image/jpeg", &m_jpgClsid);
//    Tool_GetEncoderClsid(L"image/bmp", &m_bmpClsid);
    memset(m_chLogPath, '\0', sizeof(m_chLogPath));

    //ReadConfig();
    m_h264Saver.SetLogEnable(m_bVideoLogEnable);
}

BaseCamera::~BaseCamera()
{
    InterruptionConnection();
    DisConnectCamera();
    //m_strIP.clear();

    m_hWnd = NULL;
    WriteLog("finish delete Camera");
    DeleteCriticalSection(&m_csLog);
    DeleteCriticalSection(&m_csFuncCallback);
	DeleteCriticalSection(&m_csResult);
}

void BaseCamera::ReadHistoryInfo()
{
    char iniFileName[MAX_PATH] = { 0 };
    //strcat_s(iniFileName, Tool_GetCurrentPath());
    strcat_s(iniFileName, Tool_GetDllDirPath());
    strcat_s(iniFileName, "\\SafeModeConfig.ini");

    //读取可靠性配置文件
    m_SaveModelInfo.iSafeModeEnable = GetPrivateProfileIntA(m_strIP.c_str(), "SafeModeEnable", 0, iniFileName);
    GetPrivateProfileStringA(m_strIP.c_str(), "BeginTime", "0", m_SaveModelInfo.chBeginTime, 256, iniFileName);
    GetPrivateProfileStringA(m_strIP.c_str(), "EndTime", "0", m_SaveModelInfo.chEndTime, 256, iniFileName);
    m_SaveModelInfo.iIndex = GetPrivateProfileIntA(m_strIP.c_str(), "Index", 0, iniFileName);
    m_SaveModelInfo.iDataType = GetPrivateProfileIntA(m_strIP.c_str(), "DataType", 0, iniFileName);
}

void BaseCamera::WriteHistoryInfo(SaveModeInfo& SaveInfo)
{
    char iniFileName[MAX_PATH] = { 0 };

    //strcat_s(iniFileName, Tool_GetCurrentPath());
    strcat_s(iniFileName, Tool_GetDllDirPath());
    strcat_s(iniFileName, "\\SafeModeConfig.ini");

    //读取配置文件
    char chTemp[256] = { 0 };
    //sprintf_s(chTemp, sizeof(chTemp), "%d", m_SaveModelInfo.iSafeModeEnable);
    sprintf_s(chTemp, sizeof(chTemp), "%d", m_SaveModelInfo.iSafeModeEnable);

    //if(m_SaveModelInfo.iSafeModeEnable == 0)
    //{
    //	SYSTEMTIME st;	
    //	GetLocalTime(&st);
    //	sprintf_s(m_SaveModelInfo.chBeginTime, "%d.%d.%d_%d", st.wYear, st.wMonth, st.wDay, st.wHour);
    //}
    WritePrivateProfileStringA(m_strIP.c_str(), "SafeModeEnable", chTemp, iniFileName);
    WritePrivateProfileStringA(m_strIP.c_str(), "BeginTime", SaveInfo.chBeginTime, iniFileName);
    WritePrivateProfileStringA(m_strIP.c_str(), "EndTime", SaveInfo.chEndTime, iniFileName);
    memset(chTemp, 0, sizeof(chTemp));
    //sprintf_s(chTemp, sizeof(chTemp), "%d", SaveInfo.iIndex);
    sprintf_s(chTemp, sizeof(chTemp), "%d", SaveInfo.iIndex);
    WritePrivateProfileStringA(m_strIP.c_str(), "Index", chTemp, iniFileName);
}

int BaseCamera::handleH264Frame(DWORD dwVedioFlag,
                                DWORD dwVideoType,
                                DWORD dwWidth,
                                DWORD dwHeight,
                                DWORD64 dw64TimeMS,
                                PBYTE pbVideoData
                                , DWORD dwVideoDataLen,
                                LPCSTR szVideoExtInfo)
{
    if (dwVedioFlag == H264_FLAG_INVAIL)
            return 0;

        if (dwVedioFlag == H264_FLAG_HISTROY_END)
        {
            m_h264Saver.StopSaveH264();
            return 0;
        }

        LONG isIFrame = 0;
        if (VIDEO_TYPE_H264_NORMAL_I == dwVideoType
            || VIDEO_TYPE_H264_HISTORY_I == dwVideoType)
        {
            isIFrame = 1;
        }

        LONG isHistory = 0;
        if (VIDEO_TYPE_H264_HISTORY_I == dwVideoType
            || VIDEO_TYPE_H264_HISTORY_P == dwVideoType)
        {
            isHistory = 1;
        }
        char chLog[256] = {0};
        sprintf_s(chLog, sizeof(chLog), "handleH264Frame:: dw64TimeMS =%I64u , currentTicket =%lu \n", dw64TimeMS, GetTickCount());
        //sprintf_s(chLog, sizeof(chLog), "handleH264Frame:: dw64TimeMS =%llu , currentTicket =%lu\n", dw64TimeMS, GetTickCount());
        //OutputDebugStringA(chLog);
        //return SaveH264Frame(pbVideoData, dwVideoDataLen, dwWidth, dwHeight, isIFrame, dw64TimeMS, isHistory);
        m_video++;
        if (m_video > VIDEO_FRAME_LIST_SIZE)
        {
            m_video = 0;
            WriteFormatLog(chLog);
        }
            

        m_curH264Ms = dw64TimeMS;

        CustH264Struct* pH264Data = new CustH264Struct(pbVideoData, dwVideoDataLen, dwWidth, dwHeight, isIFrame, isHistory, dw64TimeMS/* GetTickCount64()*/, m_video);
        //pH264Data->index = m_video;
        if (!m_h264Saver.addDataStruct(pH264Data))
        {
            SAFE_DELETE_OBJ(pH264Data);
            memset(chLog, '\0', sizeof(chLog));
            sprintf_s(chLog, sizeof(chLog), "handleH264Frame:: addDataStruct failed. \n");
            WriteFormatLog(chLog);
        }
        return 0;
}

bool BaseCamera::SetCameraInfo(CameraInfo& camInfo)
{
    m_strIP = std::string(camInfo.chIP);
    m_strDeviceID = std::string(camInfo.chDeviceID);
    //sprintf_s(m_chDeviceID, "%s", camInfo.chDeviceID);
    //sprintf_s(m_chLaneID, "%s", camInfo.chLaneID);
    //sprintf_s(m_chStationID, "%s", camInfo.chStationID);
    sprintf_s(m_chDeviceID, sizeof(m_chDeviceID), "%s", camInfo.chDeviceID);
    sprintf_s(m_chLaneID, sizeof(m_chLaneID), "%s", camInfo.chLaneID);
    sprintf_s(m_chStationID, sizeof(m_chStationID), "%s", camInfo.chStationID);
    m_bLogEnable = camInfo.bLogEnable;
    m_bSynTime = camInfo.bSynTimeEnable;
    m_iDirection = camInfo.iDirection;

    return true;
}

int BaseCamera::GetCamStatus()
{
    //int iStatus = 1;
    //CDevState pState;
    //if (HVAPI_GetDevState(m_hHvHandle, &pState) != S_OK)
    //{
    //	iStatus = 1;
    //	char chCaptureLog3[MAX_PATH] = {0};
    //	//sprintf_s(chCaptureLog3, "Camera: %s SoftTriggerCapture failed", m_strIP.c_str());
    //	sprintf_s(chCaptureLog3, "Camera: %s GetDevState failed", m_strIP.c_str());
    //	WriteLog(chCaptureLog3);
    //}
    //else
    //{
    //	iStatus = 0;
    //	char chCaptureLog4[MAX_PATH] = {0};
    //	//sprintf_s(chCaptureLog4, "Camera: %s SoftTriggerCapture success", m_strIP.c_str());
    //	sprintf_s(chCaptureLog4, "Camera: %s GetDevState success", m_strIP.c_str());
    //	WriteLog(chCaptureLog4);
    //}
    //return iStatus;

    if (NULL == m_hHvHandle)
        return 1;
    DWORD dwStatus = 1;

    if (HVAPI_GetConnStatusEx((HVAPI_HANDLE_EX)m_hHvHandle, CONN_TYPE_RECORD, &dwStatus) == S_OK)
    {
        if (dwStatus == CONN_STATUS_NORMAL
            /*|| dwStatus == CONN_STATUS_RECVDONE*/)
        {
            m_iConnectStatus = 0;
        }
        else if (dwStatus == CONN_STATUS_RECONN)
        {
            m_iConnectStatus = 1;
        }
        else
        {
            m_iConnectStatus = 1;
        }
    }
    else
    {
        m_iConnectStatus = 1;
    }
    return m_iConnectStatus;
}

int BaseCamera::GetNetSatus()
{
    //if (!m_strIP.empty() && Tool_PingIPaddress(m_strIP.c_str()))
    //{
    //    return 0;
    //}
	if (!m_strIP.empty() && 1 != Tool_pingIp_win(m_strIP.c_str()))
	{
		return 0;
	}
    return 1;
}

char* BaseCamera::GetStationID()
{
    return m_chStationID;
}

char* BaseCamera::GetDeviceID()
{
    return m_chDeviceID;
}

char* BaseCamera::GetLaneID()
{
    return m_chLaneID;
}

const char* BaseCamera::GetCameraIP()
{
    return m_strIP.c_str();
}

int BaseCamera::ConnectToCamera()
{
    if (m_strIP.empty())
    {
        WriteLog("ConnectToCamera:: please finish the camera ip address");
        return -1;
    }
    if (NULL != m_hHvHandle)
    {
        InterruptionConnection();
        HVAPI_CloseEx((HVAPI_HANDLE_EX)m_hHvHandle);
        m_hHvHandle = NULL;
    }
    m_hHvHandle = HVAPI_OpenEx(m_strIP.c_str(), NULL);
    //m_hHvHandle = HVAPI_OpenChannel(m_strIP.c_str(), NULL, 0);
    if (NULL == m_hHvHandle)
    {
        WriteLog("ConnectToCamera:: Open CameraHandle failed!");
        return -2;
    }

    ReadHistoryInfo();
    char chCommand[1024] = { 0 };
    sprintf_s(chCommand, sizeof(chCommand), "DownloadRecord,BeginTime[%s],Index[%d],Enable[%d],EndTime[%s],DataInfo[%d]",
        m_SaveModelInfo.chBeginTime,
        m_SaveModelInfo.iIndex,
        m_SaveModelInfo.iSafeModeEnable,
        m_SaveModelInfo.chEndTime,
        m_SaveModelInfo.iDataType);
    //sprintf_s(chCommand, "DownloadRecord,BeginTime[%s],Index[%d],Enable[%d],EndTime[%s],DataInfo[%d]", m_SaveModelInfo.chBeginTime, m_SaveModelInfo.iIndex, m_SaveModelInfo.iSafeModeEnable, m_SaveModelInfo.chEndTime, m_SaveModelInfo.iDataType);

    WriteLog(chCommand);

    int iRet = -1;

    if ((HVAPI_SetCallBackEx(m_hHvHandle, (PVOID)RecordInfoBeginCallBack, this, 0, CALLBACK_TYPE_RECORD_INFOBEGIN, NULL) != S_OK) ||
        (HVAPI_SetCallBackEx(m_hHvHandle, (PVOID)RecordInfoEndCallBack, this, 0, CALLBACK_TYPE_RECORD_INFOEND, NULL) != S_OK) ||
        (HVAPI_SetCallBackEx(m_hHvHandle, (PVOID)RecordInfoPlateCallBack, this, 0, CALLBACK_TYPE_RECORD_PLATE, NULL) != S_OK) ||
        (HVAPI_SetCallBackEx(m_hHvHandle, (PVOID)RecordInfoBigImageCallBack, this, 0, CALLBACK_TYPE_RECORD_BIGIMAGE, chCommand) != S_OK) /*||
        (HVAPI_SetCallBackEx(m_hHvHandle, (PVOID)RecordInfoSmallImageCallBack, this, 0, CALLBACK_TYPE_RECORD_SMALLIMAGE, chCommand) != S_OK) ||
        (HVAPI_SetCallBackEx(m_hHvHandle, (PVOID)RecordInfoBinaryImageCallBack, this, 0, CALLBACK_TYPE_RECORD_BINARYIMAGE, chCommand) != S_OK)*/ /*||
        (HVAPI_SetCallBackEx(m_hHvHandle, (PVOID)JPEGStreamCallBack, this, 0, CALLBACK_TYPE_JPEG_FRAME, NULL) != S_OK)*/
        )
    {
        WriteLog("ConnectToCamera:: SetCallBack failed.");
        HVAPI_CloseEx(m_hHvHandle);
        m_hHvHandle = NULL;
        iRet = - 3;
    }
    else
    {
        WriteLog("ConnectToCamera:: SetCallBack success.");
    }

	for (int i = 0; i < 3; i++)
	{
		Sleep(100);
		if (!SetH264Callback(1, 0, 0, H264_RECV_FLAG_REALTIME))
		{
			WriteLog("ConnectToCamera:: SetH264Callback failed.");
		}
		else
		{
			WriteLog("ConnectToCamera:: SetH264Callback success.");
			break;
		}
	}
    iRet = 0;
    return iRet;
}

void BaseCamera::ReadConfig()
{
    char iniFileName[MAX_PATH] = { 0 };
#ifdef GUANGXI_DLL
    sprintf_s(iniFileName, "..\\DevInterfaces\\HVCR_Signalway_V%d_%d\\HVCR_Config\\HVCR_Signalway_V%d_%d.ini", PROTOCAL_VERSION, DLL_VERSION, PROTOCAL_VERSION, DLL_VERSION);
#else
    //strcat_s(iniFileName, Tool_GetCurrentPath());
    strcat_s(iniFileName, Tool_GetDllDirPath());
    strcat_s(iniFileName, INI_FILE_NAME);
#endif

    //读取可靠性配置文件
    int iLog = GetPrivateProfileIntA("Log", "Enable", 0, iniFileName);
    m_bLogEnable = (iLog == 1) ? true : false;

    char chTemp[256] = { 0 };
    //sprintf_s(chTemp, sizeof(chTemp), "%d", iLog);
    sprintf_s(chTemp, sizeof(chTemp), "%d", iLog);
    WritePrivateProfileStringA("Log", "Enable", chTemp, iniFileName);
    
    Tool_ReadKeyValueFromConfigFile(INI_FILE_NAME, "Result", "SavePath", chTemp, sizeof(chTemp));
    if (strlen(chTemp) < 1)
    {
        SetImageDir(Tool_GetCurrentPath());
        //sprintf_s(m_chImageDir, sizeof(m_chImageDir), "%s", Tool_GetCurrentPath());
        //sprintf_s(m_chImageDir, sizeof(m_chImageDir), "%s", Tool_GetDllDirPath());
        Tool_WriteKeyValueFromConfigFile(INI_FILE_NAME, "Result", "SavePath", (char*)Tool_GetCurrentPath(), sizeof(chTemp));
    }

    int iTemp = 5;
    Tool_ReadIntValueFromConfigFile(INI_FILE_NAME, "Video", "AdvanceTime", iTemp);
    m_iVideoAdvanceTime = iTemp;
    iTemp = 2;
    Tool_ReadIntValueFromConfigFile(INI_FILE_NAME, "Video", "DelayTime", iTemp);
    m_iVideoDelayTime = iTemp;
	iTemp = 0;
	Tool_ReadIntValueFromConfigFile(INI_FILE_NAME, "Video", "LogEnable", iTemp);
	m_bVideoLogEnable = iTemp == 0 ? false : true;
}

void BaseCamera::SetLogPath(const char* path)
{
    EnterCriticalSection(&m_csFuncCallback);
    if (path != NULL
        && strlen(path) < sizeof(m_chLogPath))
    {
        strcpy(m_chLogPath, path);
    }
    LeaveCriticalSection(&m_csFuncCallback);
}

bool BaseCamera::GetLogPath(char* buff, size_t bufLen)
{
    bool bRet = false;
    EnterCriticalSection(&m_csFuncCallback);
    if (buff != NULL
        && strlen(m_chLogPath) < bufLen)
    {
        memset(buff, '\0', bufLen);
        sprintf_s(buff, bufLen, "%s", m_chLogPath);
        bRet = true;
    }
    LeaveCriticalSection(&m_csFuncCallback);
    return bRet;
}

void BaseCamera::SetLogHoldDays(int iDay)
{
    EnterCriticalSection(&m_csFuncCallback);
    m_iLogHoldDay = iDay;
    LeaveCriticalSection(&m_csFuncCallback);
}

int BaseCamera::GetLogHoldDays()
{
    int iValue = 30;
    EnterCriticalSection(&m_csFuncCallback);
    iValue = m_iLogHoldDay;
    LeaveCriticalSection(&m_csFuncCallback);
    return iValue;
}

void BaseCamera::WriteFormatLog(const char* szfmt, ...)
{
    static char g_szPbString[10240] = { 0 };
    memset(g_szPbString, 0, sizeof(g_szPbString));

    va_list arg_ptr;
    va_start(arg_ptr, szfmt);
    vsnprintf_s(g_szPbString, sizeof(g_szPbString), szfmt, arg_ptr);
    va_end(arg_ptr);

	WriteLog(g_szPbString);
}

bool BaseCamera::WriteLog(const char* chlog)
{
    //ReadConfig();
    if (!m_bLogEnable || NULL == chlog)
        return false;

    //取得当前的精确毫秒的时间
    SYSTEMTIME systime;
    GetLocalTime(&systime);//本地时间

    char chLogPath[512] = { 0 };

    if (strlen(m_chLogPath) <= 0)
    {
        char chLogRoot[256] = { 0 };
        Tool_ReadKeyValueFromConfigFile(INI_FILE_NAME, "Log", "Path", chLogRoot, sizeof(chLogRoot));
        if (strlen(chLogRoot) > 0)
        {
            sprintf_s(chLogPath, sizeof(chLogPath), "%s\\%04d-%02d-%02d\\%s\\",
                chLogRoot,
                systime.wYear,
                systime.wMonth,
                systime.wDay,
                m_strIP.c_str());
        }
        else
        {
            sprintf_s(chLogPath, sizeof(chLogPath), "%s\\XLWLog\\%04d-%02d-%02d\\%s\\",
                Tool_GetCurrentPath(),
                systime.wYear,
                systime.wMonth,
                systime.wDay,
                m_strIP.c_str());
        }
    }
    else
    {
        sprintf_s(chLogPath, sizeof(chLogPath), "%s\\XLWLog\\%04d-%02d-%02d\\%s\\",
            m_chLogPath,
            systime.wYear,
            systime.wMonth,
            systime.wDay,
            m_strIP.c_str());
    }
    //char chLogRoot[256] = { 0 };
    //Tool_ReadKeyValueFromConfigFile(INI_FILE_NAME, "Log", "Path", chLogRoot, sizeof(chLogRoot));
    //if (strlen(chLogRoot) > 0)
    //{
    //    sprintf_s(chLogPath, sizeof(chLogPath), "%s\\%04d-%02d-%02d\\%s\\",
    //        chLogRoot,
    //        systime.wYear,
    //        systime.wMonth,
    //        systime.wDay,
    //        m_strIP.c_str());
    //}
    //else
    //{
    //    sprintf_s(chLogPath, sizeof(chLogPath), "%s\\XLWLog\\%04d-%02d-%02d\\%s\\",
    //        Tool_GetCurrentPath(),
    //        systime.wYear,
    //        systime.wMonth,
    //        systime.wDay,
    //        m_strIP.c_str());
    //}

    MakeSureDirectoryPathExists(chLogPath);

    //每次只保留10天以内的日志文件
//    CTime tmCurrentTime = CTime::GetCurrentTime();
//    CTime tmLastMonthTime = tmCurrentTime - CTimeSpan(10, 0, 0, 0);
//    int Last_Year = tmLastMonthTime.GetYear();
//    int Last_Month = tmLastMonthTime.GetMonth();
//    int Last_Day = tmLastMonthTime.GetDay();

    time_t now = time(NULL);
    //tm* ts = localtime(&now);
    //ts->tm_mday = ts->tm_mday-10;
    //mktime(ts); /* Normalise ts */
    //int Last_Year = ts->tm_year +1900;
    //int Last_Month = ts->tm_mon +1;
    //int Last_Day = ts->tm_wday;

    tm ts;
    localtime_s(&ts, &now);
    ts.tm_mday = ts.tm_mday - 10;
    mktime(&ts); /* Normalise ts */
    int Last_Year = ts.tm_year + 1900;
    int Last_Month = ts.tm_mon + 1;
    int Last_Day = ts.tm_wday;

    char chOldLogFileName[MAX_PATH] = { 0 };
    //sprintf_s(chOldLogFileName, "%s\\XLWLog\\%04d-%02d-%02d\\",szFileName, Last_Year, Last_Month, Last_Day);
    sprintf_s(chOldLogFileName, sizeof(chOldLogFileName), "%s\\XLWLog\\%04d-%02d-%02d\\",
        Tool_GetCurrentPath(),
        Last_Year,
        Last_Month,
        Last_Day);

    if (PathFileExists(chOldLogFileName))
    //if(Tool_IsDirExist(chOldLogFileName))
    {
        char chCommand[512] = { 0 };
        //sprintf_s(chCommand, "/c rd /s/q %s", chOldLogFileName);
        sprintf_s(chCommand, sizeof(chCommand), "/c rd /s/q %s", chOldLogFileName);
        Tool_ExcuteCMD(chCommand);
    }

    char chLogFileName[512] = { 0 };
    //sprintf_s(chLogFileName, "%s\\CameraLog-%d-%02d_%02d.log",chLogPath, pTM->tm_year + 1900, pTM->tm_mon+1, pTM->tm_mday);
    sprintf_s(chLogFileName, sizeof(chLogFileName), "%s\\CameraLog-%d-%02d_%02d.log",
        chLogPath,
        systime.wYear,
        systime.wMonth,
        systime.wDay);

    EnterCriticalSection(&m_csLog);

    FILE *file = NULL;
    //file = fopen(chLogFileName, "a+");
    fopen_s(&file, chLogFileName, "a+");
    if (file)
    {
        fprintf(file, "%04d-%02d-%02d %02d:%02d:%02d:%03d [%s]: %s\n",
            systime.wYear,
            systime.wMonth,
            systime.wDay,
            systime.wHour,
            systime.wMinute,
            systime.wSecond,
            systime.wMilliseconds,
            DLL_VERSION,
            chlog);
        fclose(file);
        file = NULL;
    }

    LeaveCriticalSection(&m_csLog);

    return true;
}

bool BaseCamera::TakeCapture()
{
    if (NULL == m_hHvHandle)
        return false;

    bool bRet = true;
    char chRetBuf[1024 * 10] = { 0 };
    int nRetLen = 0;
    char chCaptureLog3[MAX_PATH] = { 0 };

    if (HVAPI_ExecCmdEx(m_hHvHandle, "SoftTriggerCapture", chRetBuf, sizeof(chRetBuf), &nRetLen) != S_OK)
    {
        bRet = false;

        sprintf_s(chCaptureLog3, sizeof(chCaptureLog3), "Camera: %s SoftTriggerCapture failed", m_strIP.c_str());
        WriteLog(chCaptureLog3);
    }
    else
    {
        sprintf_s(chCaptureLog3, sizeof(chCaptureLog3), "Camera: %s SoftTriggerCapture success", m_strIP.c_str());
        WriteLog(chCaptureLog3);
    }
    return bRet;
}

bool BaseCamera::SynTime()
{
    if (NULL == m_hHvHandle)
        return false;
    //if (!m_bSynTime)
    //{
    //	return false;
    //}
    WriteLog("SynTime begin");

    SYSTEMTIME st_localTime;
    GetLocalTime(&st_localTime);

    char chTemp[256] = { 0 };
    //sprintf_s(chTemp, sizeof(chTemp), "SetTime,Date[%d.%02d.%02d],Time[%02d:%02d:%02d]",
    //	pTM->tm_year + 1900,  pTM->tm_mon, pTM->tm_mday,
    //	pTM->tm_hour, pTM->tm_min, pTM->tm_sec);
    sprintf_s(chTemp, sizeof(chTemp), "SetTime,Date[%d.%02d.%02d],Time[%02d:%02d:%02d]",
        st_localTime.wYear,
        st_localTime.wMonth,
        st_localTime.wDay,
        st_localTime.wHour,
        st_localTime.wMinute,
        st_localTime.wSecond);

    WriteLog(chTemp);
    char szRetBuf[1024] = { 0 };
    int nRetLen = 0;
    if (m_hHvHandle != NULL)
    {
        char chSynTimeLogBuf[MAX_PATH] = { 0 };
        try
        {
            if (HVAPI_ExecCmdEx(m_hHvHandle, chTemp, szRetBuf, 1024, &nRetLen) != S_OK)
            {
                memset(chSynTimeLogBuf, 0, sizeof(chSynTimeLogBuf));
                sprintf_s(chSynTimeLogBuf, sizeof(chSynTimeLogBuf), "Camera: %s SynTime failed", m_strIP.c_str());
                WriteLog(chSynTimeLogBuf);
                return false;
            }
            else
            {
                memset(chSynTimeLogBuf, 0, sizeof(chSynTimeLogBuf));
                sprintf_s(chSynTimeLogBuf, sizeof(chSynTimeLogBuf), "Camera: %s SynTime success", m_strIP.c_str());
                WriteLog(chSynTimeLogBuf);
            }
        }
        catch (...)
        {
            memset(chSynTimeLogBuf, 0, sizeof(chSynTimeLogBuf));
            sprintf_s(chSynTimeLogBuf, sizeof(chSynTimeLogBuf), "Camera: %s SynTime take exception", m_strIP.c_str());
            WriteLog(chSynTimeLogBuf);
        }
    }
    WriteLog("SynTime end");
    return true;
}

bool BaseCamera::SynTime(int Year, int Month, int Day, int Hour, int Minute, int Second, int /*MilientSecond*/)
{
    if (NULL == m_hHvHandle)
    {
        WriteLog("SynTime  failed: ConnectStatus != 0.");
        return false;
    }
    //if (!m_bSynTime)
    //{
    //	WriteLog("SynTime  failed: SynTime is not open.");
    //	return false;
    //}
    if (abs(Month) > 12 || abs(Day) > 31 || abs(Hour) > 24 || abs(Minute) > 60 || abs(Second) > 60)
    {
        WriteLog("SynTime  failed: time value is invalid.");
        return false;
    }
    WriteLog("SynTime begin");


    char chTemp[256] = { 0 };
    sprintf_s(chTemp, sizeof(chTemp), "SetTime,Date[%d.%02d.%02d],Time[%02d:%02d:%02d]",
        abs(Year), abs(Month), abs(Day),
        abs(Hour), abs(Minute), abs(Second));
    WriteLog(chTemp);
    char szRetBuf[1024] = { 0 };
    int nRetLen = 1024;
    if (m_hHvHandle != NULL)
    {
        char chSynTimeLogBuf1[MAX_PATH] = { 0 };
        try
        {
            if (HVAPI_ExecCmdEx(m_hHvHandle, chTemp, szRetBuf, 1024, &nRetLen) != S_OK)
            //if (HVAPI_SetTime(m_hHvHandle, Year, Month, Day, Hour, Minute, Second, 0) != S_OK)
            {

                memset(chSynTimeLogBuf1, 0, sizeof(chSynTimeLogBuf1));
                sprintf_s(chSynTimeLogBuf1, sizeof(chSynTimeLogBuf1), "Camera: %s SynTime failed", m_strIP.c_str());
                WriteLog(chSynTimeLogBuf1);
                return false;
            }
            else
            {
                memset(chSynTimeLogBuf1, 0, sizeof(chSynTimeLogBuf1));
                sprintf_s(chSynTimeLogBuf1, sizeof(chSynTimeLogBuf1), "Camera: %s SynTime success.", m_strIP.c_str());
                WriteLog(chSynTimeLogBuf1);
            }
        }
        catch (...)
        {
            memset(chSynTimeLogBuf1, 0, sizeof(chSynTimeLogBuf1));
            sprintf_s(chSynTimeLogBuf1, sizeof(chSynTimeLogBuf1), "Camera: %s SynTime exception.", m_strIP.c_str());
            WriteLog(chSynTimeLogBuf1);
        }
    }
    WriteLog("SynTime end");
    return true;
}
bool BaseCamera::GetDeviceTime(DeviceTime& deviceTime)
{
    if (NULL == m_hHvHandle)
        return false;

    char chRetBuf[1024] = { 0 };
    int nRetLen = 0;

    if (HVAPI_ExecCmdEx(m_hHvHandle, "DateTime", chRetBuf, sizeof(chRetBuf), &nRetLen) != S_OK)
    {
        WriteLog("GetDeviceTime:: failed.");
        return false;
    }
    WriteLog(chRetBuf);
    bool bRet = false;
    const char* chFileName = "./DateTime.xml";
    DeleteFileA(chFileName);

    //FILE* file_L = fopen(chFileName, "wb");
    FILE* file_L = NULL;
    fopen_s(&file_L, chFileName, "wb");
    if (file_L)
    {
        size_t size_Read = fwrite(chRetBuf, 1, nRetLen, file_L);
        fclose(file_L);
        file_L = NULL;
        char chFileLog[260] = { 0 };
        sprintf_s(chFileLog, sizeof(chFileLog), "GetDeviceTime:: DateTime.xml create success, size =%d ", size_Read);
        WriteLog(chFileLog);
        bRet = true;
    }
    if (!bRet)
    {
        WriteLog("GetDeviceTime:: DateTime.xml create failed.");
        return false;
    }

    const char* pDate = NULL;
    const char* pTime = NULL;
    TiXmlDocument cXmlDoc;
    //    if(cXmlDoc.Parse(chRetBuf))
    if (cXmlDoc.LoadFile(chFileName))
    {
        TiXmlElement* pSectionElement = cXmlDoc.RootElement();
        if (pSectionElement)
        {
            TiXmlElement* pChileElement = pSectionElement->FirstChildElement();
            pDate = pChileElement->Attribute("Date");
            pTime = pChileElement->Attribute("Time");
        }
        else
        {
            WriteLog("find Root element failed.");
        }
    }
    else
    {
        WriteLog("parse failed");
    }
    int iYear = 0, iMonth = 0, iDay = 0, iHour = 0, iMinute = 0, iSecond = 0, iMiliSecond = 0;
    //sscanf(pDate, "%04d.%02d.%02d", &iYear, &iMonth, &iDay);
    //sscanf(pTime, "%02d:%02d:%02d %03d", &iHour, &iMinute, &iSecond, &iMiliSecond);
    sscanf_s(pDate, "%04d.%02d.%02d", &iYear, &iMonth, &iDay);
    sscanf_s(pTime, "%02d:%02d:%02d %03d", &iHour, &iMinute, &iSecond, &iMiliSecond);

    deviceTime.iYear = iYear;
    deviceTime.iMonth = iMonth;
    deviceTime.iDay = iDay;
    deviceTime.iHour = iHour;
    deviceTime.iMinutes = iMinute;
    deviceTime.iSecond = iSecond;
    deviceTime.iMilisecond = iMiliSecond;

    return true;
}

bool BaseCamera::GetStreamLength(IStream* pStream, ULARGE_INTEGER* puliLenth)
{
    if (pStream == NULL)
        return false;

    LARGE_INTEGER liMov;
    liMov.QuadPart = 0;

    ULARGE_INTEGER uliEnd, uliBegin;

    HRESULT hr = S_FALSE;

    hr = pStream->Seek(liMov, STREAM_SEEK_END, &uliEnd);
    if (FAILED(hr))
        return false;

    hr = pStream->Seek(liMov, STREAM_SEEK_SET, &uliBegin);
    if (FAILED(hr))
        return false;

    // 差值即是流的长度
    puliLenth->QuadPart = uliEnd.QuadPart - uliBegin.QuadPart;

    return TRUE;
}

void BaseCamera::SaveResultToBufferPath(CameraResult* pResult)
{
    if (NULL == pResult)
    {
        return;
    }
    //将图片缓存到缓存目录
    //DWORD64 dwPlateTime = 0;
    //char chBigImgFileName[260] = {0};
    //char chBinImgFileName[260] = {0};
    char chPlateColor[32] = { 0 };

   // dwPlateTime = pResult->dw64TimeMS / 1000;
    if (strstr(pResult->chPlateNO, "无"))
    {
        sprintf_s(chPlateColor, sizeof(chPlateColor), "无");
    }
    else
    {
        sprintf_s(chPlateColor, sizeof(chPlateColor), "%s", pResult->chPlateColor);
    }

    //TCHAR szFileName[260] = { 0 };
    //GetModuleFileName(NULL, szFileName, 260);	//取得包括程序名的全路径
    //PathRemoveFileSpec(szFileName);				//去掉程序名	

    char chLogPath[256] = { 0 };
    Tool_ReadKeyValueFromConfigFile(INI_FILE_NAME, "Log", "Path", chLogPath, sizeof(chLogPath));


    //构造文件名称，格式： Unix时间-车牌号-车牌颜色
    //二值图
    //sprintf_s(pResult->CIMG_BinImage.chSavePath, "%s\\Buffer\\%s\\%d-%s-%s-%s-车型%d-车轴%d-%d-bin.bin", szFileName, m_strIP.c_str(), dwPlateTime, pResult->chPlateNO, chPlateColor, pResult->chPlateTime, pResult->iVehTypeNo, pResult->iAxletreeCount, pResult->dwCarID);
    ////车牌图
    //sprintf_s(pResult->CIMG_PlateImage.chSavePath, "%s\\Buffer\\%s\\%d-%s-%s-%s-车型%d-车轴%d-%d-Plate.jpg", szFileName, m_strIP.c_str(), dwPlateTime, pResult->chPlateNO, chPlateColor, pResult->chPlateTime, pResult->iVehTypeNo, pResult->iAxletreeCount, pResult->dwCarID);
    ////bestCapture
    //sprintf_s(pResult->CIMG_BestCapture.chSavePath, "%s\\Buffer\\%s\\%d-%s-%s-%s-车型%d-车轴%d-%d-BestCapture.jpg", szFileName, m_strIP.c_str(), dwPlateTime, pResult->chPlateNO, chPlateColor, pResult->chPlateTime, pResult->iVehTypeNo, pResult->iAxletreeCount, pResult->dwCarID);
    ////lastCapture
    //sprintf_s(pResult->CIMG_LastCapture.chSavePath, "%s\\Buffer\\%s\\%d-%s-%s-%s-车型%d-车轴%d-%d-LastCapture.jpg", szFileName, m_strIP.c_str(), dwPlateTime, pResult->chPlateNO, chPlateColor, pResult->chPlateTime, pResult->iVehTypeNo, pResult->iAxletreeCount, pResult->dwCarID);
    ////BEGIN_CAPTURE
    //sprintf_s(pResult->CIMG_BeginCapture.chSavePath, "%s\\Buffer\\%s\\%d-%s-%s-%s-车型%d-车轴%d-%d-BeginCapture.jpg", szFileName, m_strIP.c_str(), dwPlateTime, pResult->chPlateNO, chPlateColor, pResult->chPlateTime, pResult->iVehTypeNo, pResult->iAxletreeCount, pResult->dwCarID);
    ////BEST_SNAPSHOT
    //sprintf_s(pResult->CIMG_BestSnapshot.chSavePath, "%s\\Buffer\\%s\\%d-%s-%s-%s-车型%d-车轴%d-%d-BestSnapshot.jpg", szFileName, m_strIP.c_str(), dwPlateTime, pResult->chPlateNO, chPlateColor, pResult->chPlateTime, pResult->iVehTypeNo, pResult->iAxletreeCount, pResult->dwCarID);
    ////LAST_SNAPSHOT
    //sprintf_s(pResult->CIMG_LastSnapshot.chSavePath, "%s\\Buffer\\%s\\%d-%s-%s-%s-车型%d-车轴%d-%d-LastSnapshot.jpg", szFileName, m_strIP.c_str(), dwPlateTime, pResult->chPlateNO, chPlateColor, pResult->chPlateTime, pResult->iVehTypeNo, pResult->iAxletreeCount, pResult->dwCarID);

    int iYear = 0, iMonth = 0, iDay = 0/*, iHour*/;
    if (strlen(pResult->chPlateTime) > 0)
    {
        std::string strTime(pResult->chPlateTime);
        std::string strYear = strTime.substr(0, 4);
        iYear = atoi(strYear.c_str());

        std::string strMonth = strTime.substr(4, 2);
        iMonth = atoi(strMonth.c_str());

        std::string strDay = strTime.substr(6, 2);
        iDay = atoi(strDay.c_str());
    }
    else
    {
        time_t timeT = time(NULL);//这句返回的只是一个时间cuo
        tm timeNow;
        localtime_s(&timeNow, &timeT);
        iYear = timeNow.tm_year + 1900;
        iMonth = timeNow.tm_mon + 1;
        iDay = timeNow.tm_mday;
    }

    sprintf_s(pResult->CIMG_BinImage.chSavePath, sizeof(pResult->CIMG_BinImage.chSavePath), "%s\\%4d-%02d-%02d\\Result\\%s\\%s-%s-bin.bin", \
        chLogPath, iYear, iMonth, iDay, m_strIP.c_str(), pResult->chPlateTime, pResult->chPlateNO);
    //车牌图
    sprintf_s(pResult->CIMG_PlateImage.chSavePath, sizeof(pResult->CIMG_BinImage.chSavePath), "%s\\%4d-%02d-%02d\\Result\\%s\\%s-%s-Plate.jpg", \
        chLogPath, iYear, iMonth, iDay, m_strIP.c_str(), pResult->chPlateTime, pResult->chPlateNO);
    //bestCapture
    sprintf_s(pResult->CIMG_BestCapture.chSavePath, sizeof(pResult->CIMG_BinImage.chSavePath), "%s\\%4d-%02d-%02d\\Result\\%s\\%s-%s-BestCapture.jpg", \
        chLogPath, iYear, iMonth, iDay, m_strIP.c_str(), pResult->chPlateTime, pResult->chPlateNO);
    //lastCapture
    sprintf_s(pResult->CIMG_LastCapture.chSavePath, sizeof(pResult->CIMG_BinImage.chSavePath), "%s\\%4d-%02d-%02d\\Result\\%s\\%s-%s-LastCapture.jpg", \
        chLogPath, iYear, iMonth, iDay, m_strIP.c_str(), pResult->chPlateTime, pResult->chPlateNO);
    //BEGIN_CAPTURE
    sprintf_s(pResult->CIMG_BeginCapture.chSavePath, sizeof(pResult->CIMG_BinImage.chSavePath), "%s\\%4d-%02d-%02d\\Result\\%s\\%s-%s-BeginCapture.jpg", \
        chLogPath, iYear, iMonth, iDay, m_strIP.c_str(), pResult->chPlateTime, pResult->chPlateNO);
    //BEST_SNAPSHOT
    sprintf_s(pResult->CIMG_BestSnapshot.chSavePath, sizeof(pResult->CIMG_BinImage.chSavePath), "%s\\%4d-%02d-%02d\\Result\\%s\\%s-%s-BestSnapshot.jpg", \
        chLogPath, iYear, iMonth, iDay, m_strIP.c_str(), pResult->chPlateTime, pResult->chPlateNO);
    //LAST_SNAPSHOT
    sprintf_s(pResult->CIMG_LastSnapshot.chSavePath, sizeof(pResult->CIMG_BinImage.chSavePath), "%s\\%4d-%02d-%02d\\Result\\%s\\%s-%s-LastSnapshot.jpg", \
        chLogPath, iYear, iMonth, iDay, m_strIP.c_str(), pResult->chPlateTime, pResult->chPlateNO);


    if (pResult->CIMG_BinImage.pbImgData)
    {
        Tool_SaveFileToPath(pResult->CIMG_BinImage.chSavePath,
            pResult->CIMG_BinImage.pbImgData,
            pResult->CIMG_BinImage.dwImgSize);
    }

    if (pResult->CIMG_PlateImage.pbImgData)
    {
		Tool_SaveFileToPath(pResult->CIMG_PlateImage.chSavePath,
            pResult->CIMG_PlateImage.pbImgData,
            pResult->CIMG_PlateImage.dwImgSize);
    }

    if (pResult->CIMG_BeginCapture.pbImgData)
    {
		Tool_SaveFileToPath(pResult->CIMG_BeginCapture.chSavePath,
            pResult->CIMG_BeginCapture.pbImgData,
            pResult->CIMG_BeginCapture.dwImgSize);
    }

    if (pResult->CIMG_BestCapture.pbImgData)
    {
		Tool_SaveFileToPath(pResult->CIMG_BestCapture.chSavePath,
            pResult->CIMG_BestCapture.pbImgData,
            pResult->CIMG_BestCapture.dwImgSize);
        //bool bSave  = SaveImgToDisk(pResult->CIMG_BestCapture.chSavePath, pResult->CIMG_BestCapture.pbImgData, pResult->CIMG_BestCapture.dwImgSize, 768, 576, 1); 
    }

    if (pResult->CIMG_LastCapture.pbImgData)
    {
		Tool_SaveFileToPath(pResult->CIMG_LastCapture.chSavePath,
            pResult->CIMG_LastCapture.pbImgData,
            pResult->CIMG_LastCapture.dwImgSize);
    }

    if (pResult->CIMG_BestSnapshot.pbImgData)
    {
		Tool_SaveFileToPath(pResult->CIMG_BestSnapshot.chSavePath,
            pResult->CIMG_BestSnapshot.pbImgData,
            pResult->CIMG_BestSnapshot.dwImgSize);
    }

    if (pResult->CIMG_LastSnapshot.pbImgData)
    {
		Tool_SaveFileToPath(pResult->CIMG_LastSnapshot.chSavePath,
            pResult->CIMG_BestSnapshot.pbImgData,
            pResult->CIMG_LastSnapshot.dwImgSize);
        //bool bSave  = SaveImgToDisk(pResult->CIMG_LastSnapshot.chSavePath, pResult->CIMG_LastSnapshot.pbImgData, pResult->CIMG_LastSnapshot.dwImgSize, 768, 576, 0); 
    }
}

void BaseCamera::InterruptionConnection()
{
    if (NULL == m_hHvHandle)
    {
        return;
    }

    if ((HVAPI_SetCallBackEx(m_hHvHandle, NULL, this, 0, CALLBACK_TYPE_RECORD_INFOBEGIN, NULL) != S_OK) ||
        (HVAPI_SetCallBackEx(m_hHvHandle, NULL, this, 0, CALLBACK_TYPE_RECORD_PLATE, NULL) != S_OK) ||
        (HVAPI_SetCallBackEx(m_hHvHandle, NULL, this, 0, CALLBACK_TYPE_RECORD_BIGIMAGE, NULL) != S_OK) ||
        (HVAPI_SetCallBackEx(m_hHvHandle, NULL, this, 0, CALLBACK_TYPE_RECORD_SMALLIMAGE, NULL) != S_OK) ||
        (HVAPI_SetCallBackEx(m_hHvHandle, NULL, this, 0, CALLBACK_TYPE_RECORD_BINARYIMAGE, NULL) != S_OK) ||
        (HVAPI_SetCallBackEx(m_hHvHandle, NULL, this, 0, CALLBACK_TYPE_RECORD_INFOEND, NULL) != S_OK) ||
        (HVAPI_SetCallBackEx(m_hHvHandle, NULL, this, 0, CALLBACK_TYPE_JPEG_FRAME, NULL) != S_OK)
        )
    {
        WriteLog("DisConnectToCamera:: SetCallBack NULL failed.");
        //return false;
    }
    else
    {
        WriteLog("DisConnectToCamera:: SetCallBack NULL success.");
    }
}

bool BaseCamera::DisConnectCamera()
{
    bool bRet = true;
    if (NULL != m_hHvHandle)
    {
        WriteFormatLog("HVAPI_CloseEx, begin.");
        HRESULT hRet = HVAPI_CloseEx((HVAPI_HANDLE_EX)m_hHvHandle);
        if (hRet == S_OK)
        {
            m_hHvHandle = NULL;
            WriteFormatLog("HVAPI_CloseEx, success.");
        }
        else
        {
            bRet = false;
            WriteFormatLog("HVAPI_CloseEx, failed.");
        }
    }
    return bRet;
}

void BaseCamera::SetConnectMsg(UINT iConMsg, UINT iDsiConMsg)
{
    EnterCriticalSection(&m_csFuncCallback);
    m_iConnectMsg = (iConMsg < 0x402) ? 0x402 : iConMsg;
    m_iDisConMsg = (iDsiConMsg < 0x403) ? 0x403 : iDsiConMsg;
    LeaveCriticalSection(&m_csFuncCallback);
}

void BaseCamera::SendMessageToPlateServer(int iMessageType /*= 1*/)
{
//    char chMessage[50] = { 0 };
//    if (iMessageType == 1)
//    {
//        sprintf_s(chMessage, sizeof(chMessage), "deleteOneResult");
//    }
//    else if (iMessageType == 2)
//    {
//        sprintf_s(chMessage, sizeof(chMessage), "deleteALLResult");
//    }
//    else
//    {
//        sprintf_s(chMessage, sizeof(chMessage), "hello.");
//    }
//    WriteLog("send 'deleteOneResult' to Server.");
//    WORD wVersionRequested;
//    WSADATA wsaData;
//    int err;

//    wVersionRequested = MAKEWORD(1, 1);

//    err = WSAStartup(wVersionRequested, &wsaData);
//    if (err != 0)
//    {
//        return;
//    }

//    if (LOBYTE(wsaData.wVersion) != 1 || HIBYTE(wsaData.wVersion) != 1)
//    {
//        WSACleanup();
//        return;
//    }

//    SOCKET sockClient = socket(AF_INET, SOCK_STREAM, 0);

//    SOCKADDR_IN addrSrv;
//    addrSrv.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
//    addrSrv.sin_family = AF_INET;
//    addrSrv.sin_port = htons(VEHICLE_LISTEN_PORT);
//    connect(sockClient, (SOCKADDR*)&addrSrv, sizeof(SOCKADDR));
//    int iSendLenth = send(sockClient, chMessage, strlen(chMessage) + 1, 0);

//    char chLog[260] = { 0 };
//    sprintf_s(chLog, sizeof(chLog), "send buffer =%s, length = %d", chMessage, iSendLenth);
//    WriteLog(chLog);
//    //char recvBuf[50] = {0};
//    //recv(sockClient, recvBuf, 50, 0);
//    //printf("%s\n", recvBuf);
//    //WriteLog(recvBuf);

//    closesocket(sockClient);
//    WSACleanup();
//    getchar();
}

bool BaseCamera::SenMessageToCamera(int iMessageType, int& iReturnValue, int& iErrorCode, int iArg)
{
    if (NULL == m_hHvHandle)
    {
        iErrorCode = -1;
        return false;
    }

    char chRetBuf[1024] = { 0 };
    char chSendBuf[256] = { 0 };
    char chLog[256] = { 0 };
    int nRetLen = 0;
//    const char* pAttribute1 = NULL;
//    const char* pAttribute2 = NULL;
//    const char* pAttribute3 = NULL;
//    const char* pAttribute4 = NULL;

    if (iMessageType == CMD_DEL_VEH_HEAD)
    {
        sprintf_s(chSendBuf, sizeof(chSendBuf), "DeleteHead_ZHWL");
    }
    else if (iMessageType == CMD_GET_VEH_LENGTH)
    {
        sprintf_s(chSendBuf, sizeof(chSendBuf), "GetQueueSize_ZHWL");
    }
    else if (iMessageType == CMD_DEL_ALL_VEH)
    {
        sprintf_s(chSendBuf, sizeof(chSendBuf), "DeleteAll_ZHWL");
    }
    else if (iMessageType == CMD_GET_VHE_HEAD)
    {
        int iValue = 0;
        if (iArg < 0)
        {
            iValue = 0;
        }
        else
        {
            iValue = iArg;
        }
        sprintf_s(chSendBuf, sizeof(chSendBuf), "GetResult_ZHWL, Value[%d]", iValue);
    }

    if (strlen(chSendBuf) <= 0)
    {
        WriteLog("SenMessageToCamera, please input the right command");
        return false;
    }

    if (HVAPI_ExecCmdEx(m_hHvHandle, chSendBuf, chRetBuf, 1024, &nRetLen) != S_OK)
    {
        memset(chLog, 0, sizeof(chLog));
        sprintf_s(chLog, sizeof(chLog), "%s  send failed.", chSendBuf);
        WriteLog(chLog);

        iErrorCode = -2;
        return false;
    }
    else
    {
        memset(chLog, 0, sizeof(chLog));
        sprintf_s(chLog, sizeof(chLog), "%s  send success.", chSendBuf);
        WriteLog(chLog);
    }

    if (iMessageType == CMD_GET_VEH_LENGTH)
    {
        int iLength = 0;
        //sscanf(chRetBuf, "%d", &iLength);
        sscanf_s(chRetBuf, "%d", &iLength);
        iReturnValue = iLength;
    }

    iErrorCode = 0;
    return true;
}

int BaseCamera::GetLoginID()
{
    int iLoginID = 0;
    EnterCriticalSection(&m_csFuncCallback);
    iLoginID = m_iLoginID;
    LeaveCriticalSection(&m_csFuncCallback);
    return iLoginID;
}

void BaseCamera::SetLoginID(int iID)
{
    EnterCriticalSection(&m_csFuncCallback);
    m_iLoginID = iID;
    LeaveCriticalSection(&m_csFuncCallback);
}

void BaseCamera::SetCameraIP(const char* ipAddress)
{
    EnterCriticalSection(&m_csFuncCallback);
    m_strIP = std::string(ipAddress);
    LeaveCriticalSection(&m_csFuncCallback);
}

void BaseCamera::SetWindowsWndForResultComming(HWND hWnd, int Msg)
{
    EnterCriticalSection(&m_csFuncCallback);
    m_hWnd = hWnd;
    m_iMsg = Msg;
    LeaveCriticalSection(&m_csFuncCallback);
}

void BaseCamera::SetCameraIndex(int iIndex)
{
    EnterCriticalSection(&m_csFuncCallback);
    m_iIndex = iIndex;
    LeaveCriticalSection(&m_csFuncCallback);
}

bool BaseCamera::SetOverlayVedioFont(int iFontSize, int iColor)
{
    char chLog[260] = { 0 };
    sprintf_s(chLog, sizeof(chLog), "SetOverlayVedioFont, size = %d, color = %d", iFontSize, iColor);
    WriteLog(chLog);
    HRESULT hRet = S_FALSE;
    HRESULT hRet2 = S_FALSE;
    int iR = 255, iG = 255, iB = 255;
    switch (iColor)
    {
    case 0:		//白
        iR = 255, iG = 255, iB = 255;
        break;
    case 1:		//红
        iR = 255, iG = 0, iB = 0;
        break;
    case 2:		//黄
        iR = 255, iG = 255, iB = 0;
        break;
    case 3:		//蓝
        iR = 0, iG = 0, iB = 255;
        break;
    case 4:		//黑
        iR = 0, iG = 0, iB = 0;
        break;
    case 5:		//绿
        iR = 0, iG = 255, iB = 0;
        break;
    case 6:		//紫
        iR = 138, iG = 43, iB = 226;
        break;
    default:
        iR = 255, iG = 255, iB = 255;
        break;
    }

    if (m_hHvHandle)
    {
        hRet = HVAPI_SetOSDFont((HVAPI_HANDLE_EX)m_hHvHandle, 0, iFontSize, iR, iG, iB);
        //hRet2 = HVAPI_SetOSDFont((HVAPI_HANDLE_EX)m_hHvHandle, 1 , iFontSize, iR, iG, iB);
        hRet2 = HVAPI_SetOSDFont((HVAPI_HANDLE_EX)m_hHvHandle, 2, iFontSize, iR, iG, iB);
        if (S_OK == hRet)
        {
            WriteLog("set H264 Font  success.");
        }
        if (S_OK == hRet2)
        {
            WriteLog("set JPEG Font  success");
        }
    }
    else
    {
        WriteLog("set time text, but the handle is invalid.");
    }
    if (hRet != S_OK && hRet2 != S_OK)
    {
        return false;
    }
    else
    {
        return true;
    }
}

bool BaseCamera::SetOverlayVideoText(int streamId, char* overlayText, int textLength)
{
    char chLog[1024] = { 0 };
    sprintf_s(chLog, sizeof(chLog), "SetOverlayVedioText, streamID= %d, string length = %d, overlayText = [%s] ", streamId, textLength, overlayText);
    WriteLog(chLog);
    HRESULT hRet = S_FALSE;

    if (m_hHvHandle)
    {
        /**
        * @brief		设置字符叠加字符串
        * @param[in]	hHandle			对应设备的有效句柄
        * @param[in]	nStreamId		视频流ID，0：H264,1:MJPEG
        * @param[in]	szText			叠加字符串 长度范围：0～255
        * @return		成功：S_OK；失败：E_FAIL  传入参数异常：S_FALSE
        */
        hRet = HVAPI_SetOSDText(m_hHvHandle, streamId, overlayText);
        if (S_OK == hRet)
        {
            WriteLog("set SetOverlayVedioText   success.");
        }
    }
    else
    {
        WriteLog("set SetOverlayVedioText, but the handle is invalid.");
    }
    if (hRet != S_OK)
    {
        return false;
    }
    else
    {
        return true;
    }
}

bool BaseCamera::SetOverlayVideoTextPos(int streamId, int posX, int posY)
{
    char chLog[256] = { 0 };
    sprintf_s(chLog, sizeof(chLog), "SetOverlayVideoTextPos,streamID= %d posX = %d ,posY=%d", streamId, posX, posY);
    WriteLog(chLog);
    HRESULT hRet = S_FALSE;

    if (m_hHvHandle)
    {
        /**
        * @brief		设置字符叠加位置（保持兼容）
        * @param[in]	hHandle			对应设备的有效句柄
        * @param[in]	nStreamId		视频流ID，0：H264,1:MJPEG
        * @param[in]	nPosX			叠加位置X坐标，范围: 0~图像宽
        * @param[in]	nPosY			叠加位置Y坐标，范围：0~图像高
        * @return		成功：S_OK；失败：E_FAIL  传入参数异常：S_FALSE
        */
        hRet = HVAPI_SetOSDPos(m_hHvHandle, streamId, posX, posY);
        if (S_OK == hRet)
        {
            WriteLog("set SetOverlayVideoTextPos  success.");
        }
    }
    else
    {
        WriteLog("set SetOverlayVideoTextPos, but the handle is invalid.");
    }
    if (hRet != S_OK)
    {
        return false;
    }
    else
    {
        return true;
    }
}

bool BaseCamera::SetOverlayVideoTextEnable(int streamId, bool enable)
{
    char chLog[256] = { 0 };
    sprintf_s(chLog, sizeof(chLog), "SetOverlayVideoTextEnable,streamID= %d enable = %d ", streamId, enable);
    WriteLog(chLog);
    HRESULT hRet = S_FALSE;

    if (m_hHvHandle)
    {
        /**
        * @brief		设置字符叠加开关
        * @param[in]	hHandle			对应设备的有效句柄
        * @param[in]	nStreamId		视频流ID，0：H264,1:MJPEG
        * @param[in]	fOSDEnable		0：关闭，1：打开
        * @return		成功：S_OK；失败：E_FAIL  传入参数异常：S_FALSE
        */
        hRet = HVAPI_SetOSDEnable(m_hHvHandle, streamId, enable);
        if (S_OK == hRet)
        {
            WriteLog("set HVAPI_SetOSDEnable  success.");
        }
    }
    else
    {
        WriteLog("set SetOverlayVideoTextEnable, but the handle is invalid.");
    }
    if (hRet != S_OK)
    {
        return false;
    }
    else
    {
        return true;
    }
}

bool BaseCamera::SetOverlayTimeEnable(int streamID, bool enable)
{
    char chLog[256] = { 0 };
    sprintf_s(chLog, sizeof(chLog), "SetOverlayTimeEnable,streamID= %d enable = %d ", streamID, enable);
    WriteLog(chLog);
    HRESULT hRet = S_FALSE;

    if (m_hHvHandle)
    {
        /**
        * @brief		设置时间叠加开关
        * @param[in]	hHandle			对应设备的有效句柄
        * @param[in]	nStreamId		视频流ID，0：H264,1:MJPEG
        * @param[in]	fEnable			字符叠加时间叠加开关，范围：0：关闭，1：打开
        * @return		成功：S_OK；失败：E_FAIL  传入参数异常：S_FALSE
        */
        hRet = HVAPI_SetOSDTimeEnable(m_hHvHandle, streamID, enable);
        if (S_OK == hRet)
        {
            WriteLog("set SetOverlayTimeEnable  success.");
        }
    }
    else
    {
        WriteLog("set SetOverlayTimeEnable, but the handle is invalid.");
    }
    if (hRet != S_OK)
    {
        return false;
    }
    else
    {
        return true;
    }
}

bool BaseCamera::SetOverlayTimeFormat(int streamId, int iformat)
{
    char chLog[256] = { 0 };
    sprintf_s(chLog, sizeof(chLog), "SetOverlayTimeFormat,streamID= %d iformat = %d ", streamId, iformat);
    WriteLog(chLog);
    HRESULT hRet = S_FALSE;

    if (m_hHvHandle)
    {
        INT nDateSeparator = 0, fShowWeekDay = 0, fTimeNewLine = 0, fShowMicroSec = 0;
        switch (iformat)
        {
        case 0:
            nDateSeparator = 0;
            fShowWeekDay = 0;
            fTimeNewLine = 0;
            fShowMicroSec = 0;
            break;
        case 1:
            nDateSeparator = 4;
            fShowWeekDay = 0;
            fTimeNewLine = 0;
            fShowMicroSec = 0;
            break;
        case 2:
            nDateSeparator = 5;
            fShowWeekDay = 0;
            fTimeNewLine = 0;
            fShowMicroSec = 0;
            break;
        default:
            nDateSeparator = 0;
            fShowWeekDay = 0;
            fTimeNewLine = 0;
            fShowMicroSec = 1;
            break;
        }
        /**
        * @brief	   设置OSD叠加时间格式
        * @param[in]	hHandle			对应设备的有效句柄
        * @param[in]	nStreamId		视频流ID，0：H264,1:MJPEG
        * @param[in]	nDateSeparator	日期分割符号 0:减号 1:斜杠 2:中文 3:点号
        * @param[in]	fShowWeekDay    显示星期几
        * @param[in]	fTimeNewLine	日期一行，时间另起一行
        * @param[in]	fShowMicroSec	显示微秒
        * @return		成功：S_OK；失败：E_FAIL  传入参数异常：S_FALSE
        */
        hRet = HVAPI_SetOSDTimeFormat(m_hHvHandle, streamId, nDateSeparator, fShowWeekDay, fTimeNewLine, fShowMicroSec);
        if (S_OK == hRet)
        {
            WriteLog("set SetOverlayTimeFormat  success.");
        }
    }
    else
    {
        WriteLog("set SetOverlayTimeFormat, but the handle is invalid.");
    }
    if (hRet != S_OK)
    {
        return false;
    }
    else
    {
        return true;
    }
}

bool BaseCamera::SetOverlayTimePos(int streamId, int posX, int posY)
{
    char chLog[256] = { 0 };
    sprintf_s(chLog, sizeof(chLog), "SetOverlayTimePos,streamID= %d posX = %d ,posY=%d", streamId, posX, posY);
    WriteLog(chLog);
    HRESULT hRet = S_FALSE;

    if (m_hHvHandle)
    {
        /**
        * @brief		设置时间叠加位置
        * @param[in]	hHandle			对应设备的有效句柄
        * @param[in]	nStreamId		视频流ID，0：H264,1:MJPEG
        * @param[in]	nPosX			叠加位置X坐标，范围: 0~图像宽
        * @param[in]	nPosY			叠加位置Y坐标，范围：0~图像高
        * @return		成功：S_OK；失败：E_FAIL  传入参数异常：S_FALSE
        */
        hRet = HVAPI_SetOSDTimePos(m_hHvHandle, streamId, posX, posY);
        if (S_OK == hRet)
        {
            WriteLog("set SetOverlayTimePos  success.");
        }
    }
    else
    {
        WriteLog("set SetOverlayTimePos, but the handle is invalid.");
    }
    if (hRet != S_OK)
    {
        return false;
    }
    else
    {
        return true;
    }
}

bool BaseCamera::GetHardWareInfo(BasicInfo& bsinfo)
{
    if (NULL == m_hHvHandle)
    {
        return false;
    }
    CDevBasicInfo tempInfo;
    HRESULT hr = HVAPI_GetDevBasicInfo((HVAPI_HANDLE_EX)m_hHvHandle, &tempInfo);
    if (hr == S_OK)
    {
        memcpy(&bsinfo, &tempInfo, sizeof(BasicInfo));
        return true;
    }
    else
    {
        return false;
    }
}

bool BaseCamera::SetH264Callback(int iStreamID, DWORD64 dwBeginTime, DWORD64 dwEndTime, DWORD RecvFlag)
{
    if (m_hHvHandle == NULL)
    {
        WriteFormatLog("SetH264Callback, m_hHvHandle == NULL, failed.");
        return false;
    }

    HRESULT hr = HVAPI_StartRecvH264Video(
        m_hHvHandle,
        (PVOID)HVAPI_CALLBACK_H264_EX,
        (PVOID)this,
        iStreamID,
        dwBeginTime,
        dwEndTime,
        RecvFlag);
    if (hr == S_OK)
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool BaseCamera::SetH264CallbackNULL(int iStreamID, DWORD RecvFlag)
{
    if (m_hHvHandle == NULL)
      {
          WriteFormatLog("SetH264Callback, m_hHvHandle == NULL, failed.");
          return false;
      }

      HRESULT hr = HVAPI_StartRecvH264Video(
          m_hHvHandle,
          NULL,
          (PVOID)this,
          iStreamID,
          0,
          0,
          RecvFlag);
      if (hr == S_OK)
      {
          return true;
      }
      else
      {
          return false;
      }
}

bool BaseCamera::SetJpegStreamCallback()
{
	if (m_hHvHandle == NULL)
	{
		WRITE_LOG_FMT("m_hHvHandle == NULL, failed.");
		return false;
	}

	if (
		(HVAPI_SetCallBackEx(m_hHvHandle, (PVOID)JPEGStreamCallBack, this, 2, CALLBACK_TYPE_JPEG_FRAME, NULL) != S_OK)
		)
	{
		WRITE_LOG_FMT("SetCallBack failed.");
		//HVAPI_CloseEx(m_hHvHandle);
		//m_hHvHandle = NULL;
		return false;
	}
	else
	{
		WRITE_LOG_FMT("SetCallBack success.");
		return true;
	}
}

bool BaseCamera::UnSetJpegStreamCallback()
{
	if (m_hHvHandle == NULL)
	{
		WRITE_LOG_FMT("m_hHvHandle == NULL, failed.");
		return false;
	}

	if (
		(HVAPI_SetCallBackEx(m_hHvHandle, NULL, this, 2, CALLBACK_TYPE_JPEG_FRAME, NULL) != S_OK)
		)
	{
		WRITE_LOG_FMT("SetCallBack failed.");
		return false;
	}
	else
	{
		WRITE_LOG_FMT("SetCallBack success.");
		return true;
	}
}

bool BaseCamera::GetOneJpegImg(CameraIMG &destImg)
{
	WriteLog("GetOneJpegImg::begin.");
	bool bRet = false;

	if (!destImg.pbImgData)
	{
		WriteLog("GetOneJpegImg:: allocate memory.");
		destImg.pbImgData = new unsigned char[MAX_IMG_SIZE];
		memset(destImg.pbImgData, 0, MAX_IMG_SIZE);
		WriteLog("GetOneJpegImg:: allocate memory success.");
	}

	EnterCriticalSection(&m_csResult);
	if (m_bJpegComplete)
	{
		if (destImg.pbImgData)
		{
			memset(destImg.pbImgData, 0, MAX_IMG_SIZE);
			memcpy(destImg.pbImgData, m_CIMG_StreamJPEG.pbImgData, m_CIMG_StreamJPEG.dwImgSize);

			destImg.dwImgSize = m_CIMG_StreamJPEG.dwImgSize;
			destImg.wImgHeight = m_CIMG_StreamJPEG.wImgHeight;
			destImg.wImgWidth = m_CIMG_StreamJPEG.wImgWidth;
			bRet = true;
			WriteLog("GetOneJpegImg success.");
			m_bJpegComplete = false;
		}
		else
		{
			WriteLog("GetOneJpegImg:: allocate memory failed.");
		}
	}
	else
	{
		WriteLog("GetOneJpegImg the image is not ready.");
	}
	LeaveCriticalSection(&m_csResult);
	WriteLog("GetOneJpegImg:: end.");

	return bRet;
}

bool BaseCamera::StartToSaveAviFile(int iStreamID, const char *fileName, DWORD64 beginTimeTick)
{
    if (m_hHvHandle == NULL)
    {
        WriteFormatLog("StartToSaveAviFile, m_hHvHandle == NULL, failed.");
        return false;
    }

    //DWORD64 timetick = m_curH264Ms - beginTimeTick;
    //if (timetick < 0)
    //    timetick = 0;
    DWORD64 timetick = beginTimeTick;

    return m_h264Saver.StartSaveH264(timetick, fileName);
}

bool BaseCamera::StopSaveAviFile(int iStreamID, INT64 TimeFlag)
{
    if (m_hHvHandle == NULL)
    {
        WriteFormatLog("StopSaveAviFile, m_hHvHandle == NULL, failed.");
        return false;
    }
    return m_h264Saver.StopSaveH264(TimeFlag);
}

void BaseCamera::generateFileName(CameraResult *pResult)
{
    if(pResult == NULL)
        return;

    memset(pResult->chSaveFileName, '\0', sizeof(pResult->chSaveFileName));
    //sprintf(pResult->chSaveFileName, "%s-%s-%d",pResult->chPlateTime, pResult->chPlateNO+2, pResult->iPlateColor );
    sprintf_s(pResult->chSaveFileName, sizeof(pResult->chSaveFileName), "%s-%s-%d", pResult->chPlateTime, pResult->chPlateNO + 2, pResult->iPlateColor);
}

void BaseCamera::SetImageDir(const char *dirPath)
{
     EnterCriticalSection(&m_csFuncCallback);
     if(strlen(dirPath) < sizeof(m_chImageDir))
     {
         memset(m_chImageDir, '\0', sizeof(m_chImageDir));
         memcpy(m_chImageDir, dirPath, strlen(dirPath));
     }
     LeaveCriticalSection(&m_csFuncCallback);
}

void BaseCamera::GetImageDir(char *buffer, size_t bufSize)
{
    EnterCriticalSection(&m_csFuncCallback);
    if(bufSize > strlen(m_chImageDir))
    {
        memset(buffer, '\0', bufSize);
        memcpy(buffer, m_chImageDir, strlen(m_chImageDir));
    }
    LeaveCriticalSection(&m_csFuncCallback);
}

void BaseCamera::SaveResult(CameraResult *pResult)
{
    if(pResult == NULL)
        return;

    char chSavePath[256] = {0};
    std::string strPlateTime(pResult->chPlateTime);

    

    if(pResult->CIMG_BeginCapture.dwImgSize > 0)
    {
        memset(chSavePath, '\0', sizeof(chSavePath));
        sprintf(chSavePath, "%s\\%s\\%s-1.jpg", m_chImageDir, strPlateTime.substr(0, 8).c_str(), pResult->chSaveFileName);
        Tool_SaveFileToPath(chSavePath, pResult->CIMG_BeginCapture.pbImgData, pResult->CIMG_BeginCapture.dwImgSize);
    }

    if(pResult->CIMG_BestCapture.dwImgSize > 0)
    {
        memset(chSavePath, '\0', sizeof(chSavePath));
        sprintf(chSavePath, "%s\\%s\\%s-2.jpg", m_chImageDir, strPlateTime.substr(0, 8).c_str(), pResult->chSaveFileName);
        Tool_SaveFileToPath(chSavePath, pResult->CIMG_BestCapture.pbImgData, pResult->CIMG_BestCapture.dwImgSize);
    }

    if(pResult->CIMG_LastCapture.dwImgSize > 0)
    {
        memset(chSavePath, '\0', sizeof(chSavePath));
        sprintf(chSavePath, "%s\\%s\\%s-3.jpg", m_chImageDir, strPlateTime.substr(0, 8).c_str(), pResult->chSaveFileName);
        Tool_SaveFileToPath(chSavePath, pResult->CIMG_LastCapture.pbImgData, pResult->CIMG_LastCapture.dwImgSize);
    }
}

void BaseCamera::SetReulstHoldDay(int days)
{
    EnterCriticalSection(&m_csFuncCallback);
    m_iResultHoldDay = days;
    LeaveCriticalSection(&m_csFuncCallback);
}

int BaseCamera::GetResultHoldDay()
{
    int iResult = 0;
    EnterCriticalSection(&m_csFuncCallback);
    iResult = m_iResultHoldDay;
    LeaveCriticalSection(&m_csFuncCallback);
    return iResult;
}

void BaseCamera::SetConnectMode(int mode)
{
	EnterCriticalSection(&m_csFuncCallback);
	m_iConnectMode = mode;
	LeaveCriticalSection(&m_csFuncCallback);
}

int BaseCamera::GetConnectMode()
{
	int iValue = 0;
	EnterCriticalSection(&m_csFuncCallback);
	iValue = m_iConnectMode;
	LeaveCriticalSection(&m_csFuncCallback);
	return iValue;
}

void BaseCamera::setVideoAdvanceTime(int iTime)
{
    EnterCriticalSection(&m_csFuncCallback);
    m_iVideoAdvanceTime = iTime;
    LeaveCriticalSection(&m_csFuncCallback);
}

int BaseCamera::getVideoAdvanceTime()
{
    int iValue = 0;
    EnterCriticalSection(&m_csFuncCallback);
    iValue = m_iVideoAdvanceTime ;
    LeaveCriticalSection(&m_csFuncCallback);
    return iValue;
}

void BaseCamera::setVideoDelayTime(int iTime)
{
    EnterCriticalSection(&m_csFuncCallback);
    m_iVideoDelayTime = iTime;
    LeaveCriticalSection(&m_csFuncCallback);
}

int BaseCamera::getVideoDelayTime()
{
    int iValue = 0;
    EnterCriticalSection(&m_csFuncCallback);
    iValue = m_iVideoDelayTime ;
    LeaveCriticalSection(&m_csFuncCallback);
    return iValue;
}

unsigned int __stdcall Camera_StatusCheckThread(LPVOID lpParam)
{
    if (!lpParam)
    {
        return -1;
    }
    BaseCamera* pThis = (BaseCamera*)lpParam;
    pThis->CheckStatus();

    return 0;
}
