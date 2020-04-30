#include "stdafx.h"
#include "Camera6467_VFR.h"
#include "HvDevice/HvDeviceBaseType.h"
#include "HvDevice/HvDeviceCommDef.h"
#include "HvDevice/HvDeviceNew.h"
#include "HvDevice/HvCamera.h"
#include "utilityTool/ToolFunction.h"
//#include "utilityTool/log4z.h"
#include <process.h>
#include <exception>
#include <new>
#include "vehRecCommondef.h"

#define VEHICLE_HEAD_NODE_NAME "Image2"
#define VEHICLE_SIDE_NODE_NAME "Image3"
#define VEHICLE_TAIL_NODE_NAME "Image4"

#ifndef LOGFMTE
#define LOGFMTE printf
#endif

#ifndef LOGFMTD
#define LOGFMTD printf
#endif

#define CHECK_ARG(arg)\
    if (arg == NULL) \
    {\
        WriteFormatLog("%s is NULL", #arg); \
        return 0;\
    }

Camera6467_VFR::Camera6467_VFR() :
BaseCamera(),
m_dwLastCarID(-1),
m_iTimeInvl(10),
m_iSuperLenth(13),
m_iResultTimeOut(1500),
m_iWaitVfrTimeOut(3),
m_iResultMsg(WM_USER + 1),
g_pUser(NULL),
g_func_ReconnectCallback(NULL),
g_ConnectStatusCallback(NULL),
g_func_DisconnectCallback(NULL),
g_pFuncResultCallback(NULL),
g_pResultUserData(NULL),
m_hMsgHanldle(NULL),
m_pResult(NULL),
m_bStatusCheckThreadExit(false),
m_hStatusCheckThread(NULL),
m_hSendResultThread(NULL),
m_hDeleteLogThread(NULL),
m_hDeleteResultThread(NULL)
{
    InitializeCriticalSection(&m_csResult);
    ReadConfig();
	m_h264Saver.SetFileNameCallback(this, ReceiveVideoFileNameCallback);
	if (m_bSaveVideoEnable)
	{
		m_h264Saver.initMode(1);
	}
	m_h264Saver.SetLogEnable(m_bVideoLogEnable);

    //m_hStatusCheckThread = (HANDLE)_beginthreadex(NULL, 0, Camera_StatusCheckThread, this, 0, NULL);
    //m_hSendResultThread = (HANDLE)_beginthreadex(NULL, 0, s_SendResultThreadFunc, this, 0, NULL);
	//m_hDeleteLogThread = (HANDLE)_beginthreadex(NULL, 0, s_DeleteLogThreadFunc, this, 0, NULL);
	//m_hDeleteResultThread = (HANDLE)_beginthreadex(NULL, 0, s_DeleteResultThreadFunc, this, 0, NULL);
}


Camera6467_VFR::Camera6467_VFR(const char* chIP, HWND hWnd, int Msg):
BaseCamera(chIP, NULL, 0),
m_dwLastCarID(-1),
m_iTimeInvl(10),
m_iSuperLenth(13),
m_iResultTimeOut(1500),
m_iWaitVfrTimeOut(3),
m_iResultMsg(Msg),
g_pUser(NULL),
g_func_ReconnectCallback(NULL),
g_ConnectStatusCallback(NULL),
g_func_DisconnectCallback(NULL),
g_pFuncResultCallback(NULL),
g_pResultUserData(NULL),
m_hMsgHanldle(hWnd),
m_pResult(NULL),
m_bStatusCheckThreadExit(false),
m_hStatusCheckThread(NULL),
m_hSendResultThread(NULL),
m_hDeleteLogThread(NULL),
m_hDeleteResultThread(NULL)
{
    ReadConfig();
	m_h264Saver.SetFileNameCallback(this, ReceiveVideoFileNameCallback);

	if (m_bSaveVideoEnable)
	{
		m_h264Saver.initMode(1);
	}
	
	m_h264Saver.SetLogEnable(m_bVideoLogEnable);

    //m_hStatusCheckThread = (HANDLE)_beginthreadex(NULL, 0, Camera_StatusCheckThread, this, 0, NULL);
    //m_hSendResultThread = (HANDLE)_beginthreadex(NULL, 0, s_SendResultThreadFunc, this, 0, NULL);
	//m_hDeleteLogThread = (HANDLE)_beginthreadex(NULL, 0, s_DeleteLogThreadFunc, this, 0, NULL);
	//m_hDeleteResultThread = (HANDLE)_beginthreadex(NULL, 0, s_DeleteResultThreadFunc, this, 0, NULL);
}

Camera6467_VFR::~Camera6467_VFR()
{
    InterruptionConnection();
    DisConnectCamera();

    m_Camera_Plate = nullptr;
    SetCheckThreadExit(true);
    //m_MySemaphore.notify(GetCurrentThreadId());
    SetResultCallback(NULL, NULL);
    Tool_SafeCloseThread(m_hStatusCheckThread);
    Tool_SafeCloseThread(m_hSendResultThread);
    Tool_SafeCloseThread(m_hDeleteLogThread);
    Tool_SafeCloseThread(m_hDeleteResultThread);

    SAFE_DELETE_OBJ(m_pResult);
}

void Camera6467_VFR::AnalysisAppendXML(CameraResult* CamResult)
{
    WriteFormatLog("AnalysisAppendXML");
    if (NULL == CamResult || strlen(CamResult->pcAppendInfo) <= 0)
        return;

    char chTemp[BUFFERLENTH] = { 0 };
    int iLenth = BUFFERLENTH;

	if (Tool_GetDataAttributFromAppenedInfo(CamResult->pcAppendInfo, VEHICLE_HEAD_NODE_NAME, "TimeHigh", chTemp, &iLenth))
    {
        DWORD64 iTime = 0;
        DWORD64 iTimeHight = 0;
        //sscanf(chTemp, "%d", &iAxleCount);
        sscanf_s(chTemp, "%I64u", &iTimeHight);
        iTime = iTimeHight << 32;

        memset(chTemp, 0, sizeof(chTemp));
        iLenth = BUFFERLENTH;

		if (Tool_GetDataAttributFromAppenedInfo(CamResult->pcAppendInfo, VEHICLE_HEAD_NODE_NAME, "TimeLow", chTemp, &iLenth))
        {
            //DWORD64 iTimeLow = 0;
            //sscanf(chTemp, "%d", &iAxleCount);
            //sscanf_s(chTemp, "%I64u", &iTimeLow);
            unsigned long iTimeLow = 0;
            sscanf_s(chTemp, "%lu", &iTimeLow);
            iTime += iTimeLow ;
        }
        WriteFormatLog("GET carArrive time iTimeLow %I64u", iTime);
        CamResult->dw64TimeMS = iTime;
    }

    if (0 != CamResult->dw64TimeMS)
    {
        //int iTimeNow = CamResult->dw64TimeMS / 1000;
        //struct tm tmNow = *localtime((time_t *)&iTimeNow);
        //sprintf_s(CamResult->chPlateTime, sizeof(CamResult->chPlateTime), "%04d-%02d-%02d %02d:%02d:%02d",
        //    tmNow.tm_year,
        //    tmNow.tm_mon,
        //    tmNow.tm_mday,
        //    tmNow.tm_hour,
        //    tmNow.tm_min,
        //    tmNow.tm_sec);
        CTime tm(CamResult->dw64TimeMS / 1000);
        sprintf_s(CamResult->chPlateTime, sizeof(CamResult->chPlateTime), "%04d%02d%02d%02d%02d%02d%03d",
            tm.GetYear(),
            tm.GetMonth(),
            tm.GetDay(),
            tm.GetHour(),
            tm.GetMinute(),
            tm.GetSecond(),
            CamResult->dw64TimeMS % 1000);
        //sprintf_s(CamResult->chPlateTime, sizeof(CamResult->chPlateTime), "%04d-%02d-%02d %02d:%02d:%02d",
        //    tm.GetYear(),
        //    tm.GetMonth(),
        //    tm.GetDay(),
        //    tm.GetHour(),
        //    tm.GetMinute(),
        //    tm.GetSecond());
    }
    else
    {
        SYSTEMTIME st;
        GetLocalTime(&st);
        sprintf_s(CamResult->chPlateTime, sizeof(CamResult->chPlateTime), "%04d%02d%02d%02d%02d%02d%03d",
            st.wYear,
            st.wMonth,
            st.wDay,
            st.wHour,
            st.wMinute,
            st.wSecond,
            st.wMilliseconds);
        //sprintf_s(CamResult->chPlateTime, sizeof(CamResult->chPlateTime), "%04d-%02d-%02d %02d:%02d:%02d:%03d",
        //    st.wYear,
        //    st.wMonth,
        //    st.wDay,
        //    st.wHour,
        //    st.wMinute,
        //    st.wSecond);
    }

    //if (Tool_GetDataFromAppenedInfo(CamResult->pcAppendInfo, "VehicleType", chTemp, &iLenth))
    //{
    //    //sprintf(CamResult->chVehTypeText,"%s", chTemp);
    //    sprintf_s(CamResult->chVehTypeText, sizeof(CamResult->chVehTypeText), "%s", chTemp);
    //    CamResult->iVehTypeNo = AnalysisVelchType(chTemp);
    //}
    //memset(chTemp, 0, sizeof(chTemp));
    //iLenth = BUFFERLENTH;
    //if (Tool_GetDataFromAppenedInfo(CamResult->pcAppendInfo, "AxleCnt", chTemp, &iLenth))
    //{
    //    int iAxleCount = 0;
    //    //sscanf(chTemp, "%d", &iAxleCount);
    //    sscanf_s(chTemp, "%d", &iAxleCount);
    //    CamResult->iAxletreeCount = iAxleCount;
    //    //printf("the Axletree count is %d.\n", iAxleCount);
    //}
    //memset(chTemp, 0, sizeof(chTemp));
    //iLenth = BUFFERLENTH;
    //CamResult->iAxletreeType = 12;
    //if (Tool_GetDataFromAppenedInfo(CamResult->pcAppendInfo, "AxleType", chTemp, &iLenth))
    //{
    //    sprintf_s(CamResult->chAxleType, sizeof(CamResult->chAxleType), "%s", chTemp);
    //    CamResult->iAxletreeGroupCount = Tool_FindSubStrCount(chTemp, "+");
    //    CamResult->iAxletreeType = AnalysisVelchAxleType(chTemp, CamResult->iAxletreeGroupCount);
    //}
    //memset(chTemp, 0, sizeof(chTemp));
    //iLenth = BUFFERLENTH;
    //if (Tool_GetDataFromAppenedInfo(CamResult->pcAppendInfo, "Wheelbase", chTemp, &iLenth))
    //{
    //    float fWheelbase = 0;
    //    //sscanf(chTemp, "%f", &fWheelbase);
    //    sscanf_s(chTemp, "%f", &fWheelbase);
    //    CamResult->fDistanceBetweenAxles = fWheelbase;
    //    //printf("the Wheelbase  is %f.\n", fWheelbase);
    //}
    //memset(chTemp, 0, sizeof(chTemp));
    //iLenth = BUFFERLENTH;
    //if (Tool_GetDataFromAppenedInfo(CamResult->pcAppendInfo, "CarLength", chTemp, &iLenth))
    //{
    //    float fCarLength = 0;
    //    //sscanf(chTemp, "%f", &fCarLength);
    //    sscanf_s(chTemp, "%f", &fCarLength);
    //    CamResult->fVehLenth = fCarLength;
    //    //printf("the CarLength  is %f.\n", fCarLength);
    //}
    //memset(chTemp, 0, sizeof(chTemp));
    //iLenth = BUFFERLENTH;
    //if (Tool_GetDataFromAppenedInfo(CamResult->pcAppendInfo, "CarHeight", chTemp, &iLenth))
    //{
    //    float fCarHeight = 0;
    //    //sscanf(chTemp, "%f", &fCarHeight);
    //    sscanf_s(chTemp, "%f", &fCarHeight);
    //    CamResult->fVehHeight = fCarHeight;
    //    //printf("the CarHeight  is %f.\n", fCarHeight);
    //}
    //memset(chTemp, 0, sizeof(chTemp));
    //iLenth = BUFFERLENTH;
    //if (Tool_GetDataFromAppenedInfo(CamResult->pcAppendInfo, "CarWidth", chTemp, &iLenth))
    //{
    //    float fVehWidth = 0;
    //    //sscanf(chTemp, "%f", &fVehWidth);
    //    sscanf_s(chTemp, "%f", &fVehWidth);
    //    CamResult->fVehWidth = fVehWidth;
    //    //printf("the CarLength  is %f.\n", fCarLength);
    //}
    //memset(chTemp, 0, sizeof(chTemp));
    //iLenth = BUFFERLENTH;
    //if (Tool_GetDataFromAppenedInfo(CamResult->pcAppendInfo, "BackUp", chTemp, &iLenth))
    //{
    //    CamResult->bBackUpVeh = true;
    //}
    //iLenth = BUFFERLENTH;
    //if (Tool_GetDataFromAppenedInfo(CamResult->pcAppendInfo, "Confidence", chTemp, &iLenth))
    //{
    //    float fConfidence = 0;
    //    //sscanf(chTemp, "%f", &fConfidence);
    //    sscanf_s(chTemp, "%f", &fConfidence);
    //    CamResult->fConfidenceLevel = fConfidence;
    //    //printf("the CarHeight  is %f.\n", fCarHeight);
    //}

    //memset(chTemp, 0, sizeof(chTemp));
    //iLenth = BUFFERLENTH;
    //if (Tool_GetDataFromAppenedInfo(CamResult->pcAppendInfo, "VideoScaleSpeed", chTemp, &iLenth))
    //{
    //    int iSpeed = 0;
    //    sscanf_s(chTemp, "%d", &iSpeed);
    //    CamResult->iSpeed = iSpeed;
    //}

    //TiXmlElement element = Tool_SelectElementByName(CamResult->pcAppendInfo, "PlateName", 2);
    //if (strlen(element.GetText()) > 0)
    //{
    //    memset(CamResult->chPlateNO, 0, sizeof(CamResult->chPlateNO));
    //    strcpy_s(CamResult->chPlateNO, sizeof(CamResult->chPlateNO), element.GetText());

    //    memset(chTemp, 0, sizeof(chTemp));
    //    strcpy_s(chTemp, sizeof(chTemp), element.GetText());

    //    iLenth = strlen(chTemp);
    //    printf("find the plate number = %s, plate length = %d\n", chTemp, iLenth);
    //    if (strlen(chTemp) > 0)
    //    {
    //        CamResult->iPlateColor = Tool_AnalysisPlateColorNo(chTemp);
    //    }
    //    else
    //    {
    //        CamResult->iPlateColor = COLOR_UNKNOW;
    //    }

    //}
    //else
    //{
    //    sprintf_s(CamResult->chPlateNO, sizeof(CamResult->chPlateNO), "无车牌");
    //    CamResult->iPlateColor = COLOR_UNKNOW;
    //}
}

int Camera6467_VFR::AnalysisVelchType(const char* vehType)
{
    if (vehType == NULL)
    {
        return UNKOWN_TYPE;
    }
    if (strstr(vehType, "客1"))
    {
        return BUS_TYPE_1;
    }
    else if (strstr(vehType, "客2"))
    {
        return BUS_TYPE_2;
        //printf("the Vehicle type code is 2.\n");
    }
    else if (strstr(vehType, "客3"))
    {
        return BUS_TYPE_3;
    }
    else if (strstr(vehType, "客4"))
    {
        return BUS_TYPE_4;
    }
    else if (strstr(vehType, "客5"))
    {
        return BUS_TYPE_5;
    }
    else if (strstr(vehType, "货1"))
    {
        return TRUCK_TYPE_1;
    }
    else if (strstr(vehType, "货2"))
    {
        return TRUCK_TYPE_2;
    }
    else if (strstr(vehType, "货3"))
    {
        return TRUCK_TYPE_3;
    }
    else if (strstr(vehType, "货4"))
    {
        return TRUCK_TYPE_4;
    }
    else if (strstr(vehType, "货5"))
    {
        return TRUCK_TYPE_5;
    }
    else if (strstr(vehType, "货6"))
    {
        return TRUCK_TYPE_6;
    }
    else
    {
        return UNKOWN_TYPE;
    }
}

int Camera6467_VFR::AnalysisVelchAxleType(const char* AxleType, int iAxleGroupCount)
{
    if (NULL == AxleType
        || strlen(AxleType) <= 0
        || iAxleGroupCount <= 0
        || NULL == strstr(AxleType, "+"))
    {
        WriteFormatLog("AnalysisVelchAxleType, NULL == AxleType , return 12.");
        return 12;
    }
    WriteFormatLog("AnalysisVelchAxleType, AxleType = %s, iAxleGroupCount = %d", AxleType, iAxleGroupCount);

    char chTempAxle[256] = { 0 };
    //strcpy_s(chTempAxle, sizeof(chTempAxle), AxleType);
    strcpy(chTempAxle, AxleType);

    int iAxletreeType = 12;

    int iFirstValue = 0, iSeconed = 0, iThird = 0, iFourth = 0, iFifth = 0, iSix = 0, iSeventh = 0;
    int iAxletreeType1 = 0, iAxletreeType2 = 0, iAxletreeType3 = 0, iAxletreeType4 = 0, iAxletreeType5 = 0, iAxletreeType6 = 0, iAxletreeType7 = 0;
    char chAxletreeType[32] = { 0 };
    int iAxleCount = iAxleGroupCount +1;
    switch (iAxleCount)
    {
    case 1:
        iAxletreeType = 12;
        break;
    case 2:
        sscanf(chTempAxle, "%d+%d", &iFirstValue, &iSeconed);
        iAxletreeType1 = GetAlexType(iFirstValue);
        iAxletreeType2 = GetAlexType(iSeconed);

        sprintf(chAxletreeType, "%d%d", iAxletreeType1, iAxletreeType2);
        sscanf(chAxletreeType, "%d", &iAxletreeType);
        break;
    case 3:
        sscanf(chTempAxle, "%d+%d+%d", &iFirstValue, &iSeconed, &iThird);
        iAxletreeType1 = GetAlexType(iFirstValue);
        iAxletreeType2 = GetAlexType(iSeconed);
        iAxletreeType3 = GetAlexType(iThird);

        sprintf(chAxletreeType, "%d%d%d", iAxletreeType1, iAxletreeType2, iAxletreeType3);
        sscanf(chAxletreeType, "%d", &iAxletreeType);
        break;
    case 4:
        sscanf(chTempAxle, "%d+%d+%d+%d", &iFirstValue, &iSeconed, &iThird, &iFourth);
        iAxletreeType1 = GetAlexType(iFirstValue);
        iAxletreeType2 = GetAlexType(iSeconed);
        iAxletreeType3 = GetAlexType(iThird);
        iAxletreeType4 = GetAlexType(iFourth);

        sprintf(chAxletreeType, "%d%d%d%d", iAxletreeType1, iAxletreeType2, iAxletreeType3, iAxletreeType4);
        sscanf(chAxletreeType, "%d", &iAxletreeType);
        break;
    case 5:
        sscanf(chTempAxle, "%d+%d+%d+%d+%d", &iFirstValue, &iSeconed, &iThird, &iFourth, &iFifth);
        iAxletreeType1 = GetAlexType(iFirstValue);
        iAxletreeType2 = GetAlexType(iSeconed);
        iAxletreeType3 = GetAlexType(iThird);
        iAxletreeType4 = GetAlexType(iFourth);
        iAxletreeType5 = GetAlexType(iFifth);

        sprintf(chAxletreeType, "%d%d%d%d%d", iAxletreeType1, iAxletreeType2, iAxletreeType3, iAxletreeType4, iAxletreeType5);
        sscanf(chAxletreeType, "%d", &iAxletreeType);
        break;
    case 6:
        sscanf(chTempAxle, "%d+%d+%d+%d+%d+%d", &iFirstValue, &iSeconed, &iThird, &iFourth, &iFifth, &iSix);
        iAxletreeType1 = GetAlexType(iFirstValue);
        iAxletreeType2 = GetAlexType(iSeconed);
        iAxletreeType3 = GetAlexType(iThird);
        iAxletreeType4 = GetAlexType(iFourth);
        iAxletreeType5 = GetAlexType(iFifth);
        iAxletreeType6 = GetAlexType(iSix);


        sprintf(chAxletreeType, "%d%d%d%d%d%d",
            iAxletreeType1,
            iAxletreeType2,
            iAxletreeType3,
            iAxletreeType4,
            iAxletreeType5,
            iAxletreeType6);
        sscanf(chAxletreeType, "%d", &iAxletreeType);
        break;
    case 7:
        sscanf(chTempAxle, "%d+%d+%d+%d+%d+%d+%d", &iFirstValue, &iSeconed, &iThird, &iFourth, &iFifth, &iSix, &iSeventh);
        iAxletreeType1 = GetAlexType(iFirstValue);
        iAxletreeType2 = GetAlexType(iSeconed);
        iAxletreeType3 = GetAlexType(iThird);
        iAxletreeType4 = GetAlexType(iFourth);
        iAxletreeType5 = GetAlexType(iFifth);
        iAxletreeType6 = GetAlexType(iSix);
        iAxletreeType7 = GetAlexType(iSeventh);

        sprintf(chAxletreeType, "%d%d%d%d%d%d%d",
            iAxletreeType1,
            iAxletreeType2,
            iAxletreeType3,
            iAxletreeType4,
            iAxletreeType5,
            iAxletreeType6,
            iAxletreeType7);
        sscanf(chAxletreeType, "%d", &iAxletreeType);
        break;
    default:
        WriteLog("use default AxletreeType 2");
        iAxletreeType = 12;
        break;
    }

    WriteFormatLog("AnalysisVelchAxleType,finish,  AxleType = %d", iAxletreeType);
    return iAxletreeType;
}

int Camera6467_VFR::GetAlexType(int ivalue)
{
    int iType = 0;
    switch (ivalue)
    {
    case 1:
        iType = 1;
        break;
    case 2:
        iType = 2;
        break;
    case 11:
        iType = 11;
        break;
    case 12:
        iType = 12;
        break;
    case 22:
        iType = 5;
        break;
    case 111:
        iType = 111;
        break;
    case 112:
        iType = 112;
        break;
    case 122:
        iType = 15;
        break;
    case 222:
        iType = 7;
        break;
    case 1112:
        iType = 114;
        break;
    case 1122:
        iType = 115;
        break;
    default:
        iType = 1;
        break;
    }
    return iType;
}

bool Camera6467_VFR::CheckIfSuperLength(CameraResult* CamResult)
{
    CHECK_ARG(CamResult);
    if (CamResult->fVehLenth > m_iSuperLenth)
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool Camera6467_VFR::CheckIfBackUpVehicle(CameraResult* CamResult)
{
    CHECK_ARG(CamResult);
    if (CamResult->bBackUpVeh)
    {
        return true;
    }
    else
    {
        return false;
    }
}

void Camera6467_VFR::ReadConfig()
{
    char chTemp[MAX_PATH] = { 0 };
    int iTempValue = 0;

    iTempValue = 15;
    Tool_ReadIntValueFromConfigFile(INI_FILE_NAME, "Filter", "SuperLongVehicleLenth", iTempValue);
    m_iSuperLenth = iTempValue > 0 ? iTempValue : 15;

    //iTempValue = 1500;
    //Tool_ReadIntValueFromConfigFile(INI_FILE_NAME, "Filter", "ResultTimeOut", iTempValue);
    //m_iResultTimeOut = iTempValue > 0 ? iTempValue : 1500;    

    iTempValue = 2000;
    Tool_ReadIntValueFromConfigFile(INI_FILE_NAME, "Result", "WaitTimeOut", iTempValue);
    m_iWaitVfrTimeOut = iTempValue > 0 ? iTempValue : 2000;

    //iTempValue = 0;
    //Tool_ReadIntValueFromConfigFile(INI_FILE_NAME, "Result", "HoldDays", iTempValue);
    //iTempValue = iTempValue > 0 ? iTempValue : 0;
    //SetReulstHoldDay(iTempValue);

    //值为1时，表示使用最早接收到的结果，
    iTempValue = 0;
    Tool_ReadIntValueFromConfigFile(INI_FILE_NAME, "Result", "GetMode", iTempValue);
    m_iResultModule = iTempValue > 0 ? iTempValue : 0;

    BaseCamera::ReadConfig();
}

int Camera6467_VFR::GetTimeInterval()
{
    int iTimeInterval = 1;
    EnterCriticalSection(&m_csFuncCallback);
    iTimeInterval = m_iTimeInvl;
    LeaveCriticalSection(&m_csFuncCallback);
    return iTimeInterval;
}

void Camera6467_VFR::SetDisConnectCallback(void* funcDisc, void* pUser)
{
    EnterCriticalSection(&m_csFuncCallback);
    g_func_DisconnectCallback = funcDisc;
    g_pUser = pUser;
    LeaveCriticalSection(&m_csFuncCallback);
}

void Camera6467_VFR::SetReConnectCallback(void* funcReco, void* pUser)
{
    EnterCriticalSection(&m_csFuncCallback);
    g_func_ReconnectCallback = funcReco;
    g_pUser = pUser;
    LeaveCriticalSection(&m_csFuncCallback);
}

void Camera6467_VFR::SetConnectStatus_Callback(void* func, void* pUser, int TimeInterval)
{
    EnterCriticalSection(&m_csFuncCallback);
    g_pUser = pUser;
    m_iTimeInvl = TimeInterval;
    LeaveCriticalSection(&m_csFuncCallback);
}

void Camera6467_VFR::SendConnetStateMsg(bool isConnect)
{
    if (isConnect)
    {
        //EnterCriticalSection(&m_csFuncCallback);
        //if (g_ConnectStatusCallback)
        //{
        //    LeaveCriticalSection(&m_csFuncCallback);
        //    //char chIP[32] = { 0 };
        //    //sprintf_s(chIP, "%s", m_strIP.c_str());
        //    //g_ConnectStatusCallback(m_iIndex, 0, g_pUser);
        //}
        //else
        //{
        //    LeaveCriticalSection(&m_csFuncCallback);
        //}

        EnterCriticalSection(&m_csFuncCallback);
        if (m_hWnd != NULL)
        {
            ::PostMessage(m_hWnd, m_iConnectMsg, NULL, NULL);
        }        
        LeaveCriticalSection(&m_csFuncCallback);
    }
    else
    {
        //EnterCriticalSection(&m_csFuncCallback);
        //if (g_ConnectStatusCallback)
        //{
        //    LeaveCriticalSection(&m_csFuncCallback);
        //    char chIP[32] = { 0 };
        //    sprintf_s(chIP, "%s", m_strIP.c_str());
        //    //g_ConnectStatusCallback(m_iIndex, -100, g_pUser);
        //}
        //else
        //{
        //    LeaveCriticalSection(&m_csFuncCallback);
        //}

        EnterCriticalSection(&m_csFuncCallback);
        if (m_hWnd != NULL)
        {
            ::PostMessage(m_hWnd, m_iDisConMsg, NULL, NULL);
        }        
        LeaveCriticalSection(&m_csFuncCallback);
    }
}

void Camera6467_VFR::SetResultCallback(void* func, void* pUser)
{
    EnterCriticalSection(&m_csFuncCallback);
    g_pFuncResultCallback = func;
    g_pResultUserData = pUser;
    LeaveCriticalSection(&m_csFuncCallback);
}

bool Camera6467_VFR::CheckIfSetResultCallback()
{
    bool bRet = false;
    EnterCriticalSection(&m_csFuncCallback);
    if (g_pFuncResultCallback != NULL)
    {
        bRet = true;
    }
    LeaveCriticalSection(&m_csFuncCallback);
    return bRet;
}

void Camera6467_VFR::SendResultByCallback(std::shared_ptr<CameraResult> pResult)
{
    WriteFormatLog("SendResultByCallback begin.");
    EnterCriticalSection(&m_csFuncCallback);
    if (g_pFuncResultCallback)
    {
        LeaveCriticalSection(&m_csFuncCallback);
        WriteFormatLog("SendResultByCallback process begin.");

        char* pSideImagePath = NULL;
        unsigned char* pSideImageData = NULL;
        unsigned long  iSideImageSize = 0;

        char* pTailImagePath = NULL;
        unsigned char* pTailImageData = NULL;
        unsigned long iTailImageSize = 0;

        char* pVideoPath = NULL;

        int iErrorMode = 0;
        if (pResult->CIMG_LastSnapshot.dwImgSize > 0
            && pResult->CIMG_BestCapture.dwImgSize <=0)
        {
            iErrorMode = 1;
        }

        switch (iErrorMode)
        {
        case 0:
            //正常情况下，车身车尾都有
            if (pResult->CIMG_BestCapture.dwImgSize > 0)
            {
                pSideImagePath = pResult->CIMG_BestCapture.chSavePath;
                pSideImageData = pResult->CIMG_BestCapture.pbImgData;
                iSideImageSize = pResult->CIMG_BestCapture.dwImgSize;
            }
            if (pResult->CIMG_LastCapture.dwImgSize > 0)
            {
                pTailImagePath = pResult->CIMG_LastCapture.chSavePath;
                pTailImageData = pResult->CIMG_LastCapture.pbImgData;
                iTailImageSize = pResult->CIMG_LastCapture.dwImgSize;
            }
            if (strlen(pResult->chSaveFileName) > 0)
            {
                pVideoPath = pResult->chSaveFileName;
            }
            break;
        case 1:
            //小黄人在特殊情况下，接收到光栅信号强制收尾后，只有车尾图
            if (pResult->CIMG_LastSnapshot.dwImgSize > 0)
            {
                pTailImagePath = pResult->CIMG_LastSnapshot.chSavePath;
                pTailImageData = pResult->CIMG_LastCapture.pbImgData;
                iTailImageSize = pResult->CIMG_LastCapture.dwImgSize;
            }
            if (strlen(pResult->chSaveFileName) > 0)
            {
                pVideoPath = pResult->chSaveFileName;
            }
            break;
        default:
            break;
        }

        //if (pResult->CIMG_BestCapture.dwImgSize > 0)
        //{
        //    pSideImagePath = pResult->CIMG_BestCapture.chSavePath;
        //}
        //if (pResult->CIMG_LastCapture.dwImgSize > 0)
        //{
        //    pTailImagePath = pResult->CIMG_LastCapture.chSavePath;
        //}
        //if (strlen(pResult->chSaveFileName) > 0)
        //{
        //    pVideoPath = pResult->chSaveFileName;
        //}
        if (Tool_SaveFileToPath(pSideImagePath, pSideImageData, iSideImageSize))
        {
            WriteFormatLog("%s save success.", pSideImagePath);
        }
        else
        {
            WriteFormatLog("%s save failed.", pSideImagePath);
        }

        if (Tool_SaveFileToPath(pTailImagePath, pTailImageData, iTailImageSize))
        {
            WriteFormatLog("%s save success.", pTailImagePath);
        }
        else
        {
            WriteFormatLog("%s save failed.", pTailImagePath);
        }

        WriteFormatLog("call back begin g_pFuncResultCallback = %p.", g_pFuncResultCallback);
        EnterCriticalSection(&m_csFuncCallback);
        (VehRec_CarData(g_pFuncResultCallback))(GetLoginID(), pSideImagePath, pTailImagePath, pVideoPath);
        LeaveCriticalSection(&m_csFuncCallback);
        WriteFormatLog("call back . finish.");
    }
    else
    {
        LeaveCriticalSection(&m_csFuncCallback);
        WriteFormatLog("g_pFuncResultCallback == NULL.");
    }
}

void Camera6467_VFR::SetMsgHandleAngMsg(void* handle, int msg)
{
    WriteFormatLog("SetMsgHandleAngMsg, handle = %p, msg = %d", handle, msg);
    EnterCriticalSection(&m_csFuncCallback);
    m_hMsgHanldle = handle;
    m_iMsg = msg;
    LeaveCriticalSection(&m_csFuncCallback);
}

void Camera6467_VFR::SetCheckThreadExit(bool bExit)
{
    EnterCriticalSection(&m_csFuncCallback);
    m_bStatusCheckThreadExit = bExit;
    LeaveCriticalSection(&m_csFuncCallback);
}

bool Camera6467_VFR::GetCheckThreadExit()
{
    bool bExit = false;
    EnterCriticalSection(&m_csFuncCallback);
    bExit = m_bStatusCheckThreadExit;
    LeaveCriticalSection(&m_csFuncCallback);
    return bExit;
}

bool Camera6467_VFR::OpenPlateCamera(const char* ipAddress)
{
    CHECK_ARG(ipAddress);

    WriteFormatLog("OpenPlateCamera %s begin.", ipAddress);
    m_Camera_Plate = std::make_shared<Camera6467_plate>();
    m_Camera_Plate->SetCameraIP(ipAddress);
    if (0 == m_Camera_Plate->ConnectToCamera())
    {
        WriteFormatLog("OpenPlateCamera %s success.", ipAddress);
        return true;
    }
    else
    {
        WriteFormatLog("OpenPlateCamera %s failed.", ipAddress);
        return false;
    }
}

std::shared_ptr<CameraResult> Camera6467_VFR::GetFrontResult()
{
    try
    {
        std::shared_ptr<CameraResult> pResultPlate;
        std::shared_ptr<CameraResult> pResultVFR;
        bool bGetPlateNo = false;
        if (m_Camera_Plate)
        {
            CameraResult* pTemp = m_Camera_Plate->GetOneResult();
            if (pTemp != NULL)
            {
                pResultPlate = std::shared_ptr<CameraResult>(pTemp);
                bGetPlateNo = true;
                pTemp = NULL;
            }
            else
            {
                bGetPlateNo = false;
            }
        }
        else
        {
            if (!m_VfrResultList.empty())
            {
                //pResultPlate = m_resultList.front();
                 m_VfrResultList.front(pResultPlate);
            }
        }
        if (pResultPlate != nullptr)
        {
            if (bGetPlateNo)
            {
                WriteFormatLog("Get one result from plate camera , plate : %s.", pResultPlate->chPlateNO);
                WriteFormatLog("GetFrontResult, plate No list:\n");
                BaseCamera::WriteLog(m_VfrResultList.GetAllPlateString().c_str());
                int iIndex = -1;
                if (strstr(pResultPlate->chPlateNO, "无"))
                {
                    WriteFormatLog("GetFrontResult, current plate no is 无车牌, use first result .");
                    iIndex = -1;
                }
                else
                {
                    iIndex = m_VfrResultList.GetPositionByPlateNo(pResultPlate->chPlateNO);
                }
                if (-1 != iIndex)
                {
                    WriteFormatLog("find the result from list, index = %d.", iIndex);
                    pResultVFR = m_VfrResultList.GetOneByIndex(iIndex);
                }
                else
                {
                    if (!m_VfrResultList.empty())
                    {
                        WriteFormatLog("can not find result from list, Get first result.");
                        //pResultVFR = m_resultList.front();
                         m_VfrResultList.front(pResultVFR);
                    }
                    else
                    {
                        WriteFormatLog("can not find result from list, the list is empty.");
                    }
                }
            }
            else
            {
                WriteFormatLog("can not get result from plate camera, get from VFR list.");
                if (!m_VfrResultList.empty())
                {
                    WriteFormatLog("Get first result.");
                    //pResultVFR = m_resultList.front();
                     m_VfrResultList.front(pResultVFR);
                }
                else
                {
                    WriteFormatLog("The list is empty.");
                }
            }
        }
        else
        {
            WriteFormatLog("Can not get result from  camera .");
        }
        return pResultVFR;
    }
    catch (bad_exception& e)
    {
        LOGFMTE("GetFrontResult, bad_exception, error msg = %s", e.what());

        return std::make_shared<CameraResult>();
    }
    catch (bad_alloc& e)
    {
        LOGFMTE("GetFrontResult, bad_alloc, error msg = %s", e.what());

        return std::make_shared<CameraResult>();
    }
    catch (exception& e)
    {
        LOGFMTE("GetFrontResult, exception, error msg = %s.", e.what());

        return std::make_shared<CameraResult>();
    }
    catch (void*)
    {
        LOGFMTE("GetFrontResult,  void* exception");
        return std::make_shared<CameraResult>();
    }
    catch (...)
    {
        LOGFMTE("GetFrontResult,  unknown exception");
        return std::make_shared<CameraResult>();
    }
}

std::shared_ptr<CameraResult> Camera6467_VFR::GetFrontResultByPosition(int position)
{
    try
    {
        WriteFormatLog("GetFrontResultByPosition , position = %d", position);
        std::shared_ptr<CameraResult> tempResult;
        if (m_VfrResultList.empty())
        {
            WriteFormatLog("GetFrontResultByPosition , resultList is empty, return null.");
            return tempResult;
        }
        if (position <= 0)
        {
            tempResult = GetFrontResult();
        }
        else
        {
            if (position >= m_VfrResultList.size())
            {
                WriteFormatLog("GetFrontResultByPosition , position : %d is larger than  resultList size %d, get the last one.",
                    position,
                    m_VfrResultList.size());
                tempResult = m_VfrResultList.GetOneByIndex(m_VfrResultList.size() - 1);
            }
            else
            {
                tempResult = m_VfrResultList.GetOneByIndex(position);
            }
        }
        WriteFormatLog("GetFrontResultByPosition ,finish");
        return tempResult;
    }
    catch (bad_exception& e)
    {
        LOGFMTE("GetFrontResultByPosition, bad_exception, error msg = %s", e.what());

        return std::make_shared<CameraResult>();
    }
    catch (bad_alloc& e)
    {
        LOGFMTE("GetFrontResultByPosition, bad_alloc, error msg = %s", e.what());

        return std::make_shared<CameraResult>();
    }
    catch (exception& e)
    {
        LOGFMTE("GetFrontResultByPosition, exception, error msg = %s.", e.what());

        return std::make_shared<CameraResult>();
    }
    catch (void*)
    {
        LOGFMTE("GetFrontResultByPosition,  void* exception");
        return std::make_shared<CameraResult>();
    }
    catch (...)
    {
        LOGFMTE("GetFrontResultByPosition,  unknown exception");
        return std::make_shared<CameraResult>();
    }
}

std::shared_ptr<CameraResult> Camera6467_VFR::GetLastResult()
{
    std::shared_ptr<CameraResult> temp;
    EnterCriticalSection(&m_csResult);
    temp = m_pLastResult;
    LeaveCriticalSection(&m_csResult);
    return temp;
}

bool Camera6467_VFR::GetLastResultIfReceiveComplete()
{
    bool bValue = false;
    EnterCriticalSection(&m_csResult);
    bValue = m_bLastResultComplete ;
    LeaveCriticalSection(&m_csResult);
    return bValue;
}

void Camera6467_VFR::SetLastResultIfReceiveComplete(bool bValue)
{
    EnterCriticalSection(&m_csResult);
    m_bLastResultComplete = bValue;
    LeaveCriticalSection(&m_csResult);
}

void Camera6467_VFR::DeleteFrontResult(const char* plateNo)
{
    try
    {
        WriteFormatLog("DeleteFrontResult, plate no = %p", plateNo);
        bool bHasPlateNo = false;
        if (NULL != plateNo && strlen(plateNo) > 0)
        {
            bHasPlateNo = true;
        }
        std::string strPlateNo;
        if (bHasPlateNo)
        {
            strPlateNo = plateNo;
            WriteFormatLog("DeleteFrontResult, has plate no : %s", plateNo);
        }
        else
        {            
            if (  m_Camera_Plate != nullptr  )
            {
                WriteFormatLog("DeleteFrontResult, get from plate camera:");
                CameraResult* pResult = m_Camera_Plate->GetOneResult();
                if (NULL != pResult)
                {
                    strPlateNo = pResult->chPlateNO;
                    SAFE_DELETE_OBJ(pResult);
                    WriteFormatLog("DeleteFrontResult, plate no : %s", strPlateNo.c_str());
                }
                else
                {
                    WriteFormatLog("DeleteFrontResult,can not get from plate camera.");
                }
            }
            else
            {
                WriteFormatLog("DeleteFrontResult, cant not get plate number from plate camera.");
            }
        }

        if (strPlateNo.empty())
        {
            WriteFormatLog("DeleteFrontResult, cant not get plate number , so delete front result.");
			if (!m_VfrResultList.empty())
			{
				m_VfrResultList.pop_front();
			}            
        }
        else
        {
            if (std::string::npos == strPlateNo.find("无"))
            {
                int iPosition = m_VfrResultList.GetPositionByPlateNo(strPlateNo.c_str());
                WriteFormatLog("DeleteFrontResult, GetPositionByPlateNo %d.", iPosition);
                m_VfrResultList.DeleteToPosition(iPosition);
            }
            else
            {
                WriteFormatLog("DeleteFrontResult, current plate  %s == ‘无车牌’, do nothing.", strPlateNo.c_str());
            }
        }
        WriteFormatLog("DeleteFrontResult, final list:");
        BaseCamera::WriteLog(m_VfrResultList.GetAllPlateString().c_str());

        WriteFormatLog("DeleteFrontResult, finish");
    }
    catch (bad_exception& e)
    {
        LOGFMTE("DeleteFrontResult, bad_exception, error msg = %s", e.what());
    }
    catch (bad_alloc& e)
    {
        LOGFMTE("DeleteFrontResult, bad_alloc, error msg = %s", e.what());
    }
    catch (exception& e)
    {
        LOGFMTE("DeleteFrontResult, exception, error msg = %s.", e.what());
    }
    catch (void*)
    {
        LOGFMTE("DeleteFrontResult,  void* exception");
    }
    catch (...)
    {
        LOGFMTE("DeleteFrontResult,  unknown exception");
    }
}

void Camera6467_VFR::ClearALLResult()
{
    WriteFormatLog("ClearALLResult begin.");
    //m_resultList.ClearALL();
    m_VfrResultList.ClearALLResult();
    WriteFormatLog("ClearALLResult finish.");
}

size_t Camera6467_VFR::GetResultListSize()
{
    //WriteFormatLog("GetResultListSize begin.");
    size_t iSize = m_VfrResultList.size();
    //WriteFormatLog("GetResultListSize finish, size = %d.", iSize);
    return iSize;
}

void Camera6467_VFR::TryWaitCondition()
{
    //m_MySemaphore.tryDecrease(GetCurrentThreadId());
}

void Camera6467_VFR::ReceiveVideoFileName(const char* videoFileName)
{
	EnterCriticalSection(&m_csFuncCallback);
	if (NULL != videoFileName
		&& std::end(m_lsCompleteVideoName) == std::find(std::begin(m_lsCompleteVideoName), std::end(m_lsCompleteVideoName), videoFileName))
	{
		if (m_lsCompleteVideoName.size() >= 3)
		{
			m_lsCompleteVideoName.pop_front();
		}
		m_lsCompleteVideoName.push_back(videoFileName);
	}
	LeaveCriticalSection(&m_csFuncCallback);
}

bool Camera6467_VFR::CheckIfFileNameIntheVideoList(const char* fileName)
{
	bool bFind = false;
	EnterCriticalSection(&m_csFuncCallback);
	if (NULL != fileName
		&& std::end(m_lsCompleteVideoName) != std::find(std::begin(m_lsCompleteVideoName), std::end(m_lsCompleteVideoName), fileName))
	{
		bFind = true;
	}
	LeaveCriticalSection(&m_csFuncCallback);
	return bFind;
}

int Camera6467_VFR::RecordInfoBegin(DWORD dwCarID)
{
    try
    {
        WriteFormatLog("RecordInfoBegin, dwCarID = %lu", dwCarID);
        SetLastResultIfReceiveComplete(false);

        SAFE_DELETE_OBJ(m_pResult);
        //m_Result = std::make_shared<CameraResult>();
        if (NULL == m_pResult)
        {
            m_pResult = new CameraResult();
        }
        CHECK_ARG(m_pResult);

        m_pResult->dwCarID = dwCarID;

        WriteFormatLog("RecordInfoBegin, finish.");
        return 0;
    }
    catch (bad_exception& e)
    {
        LOGFMTE("RecordInfoBegin, bad_exception, error msg = %s", e.what());
        return 0;
    }
    catch (bad_alloc& e)
    {
        LOGFMTE("RecordInfoBegin, bad_alloc, error msg = %s", e.what());
        return 0;
    }
    catch (exception& e)
    {
        LOGFMTE("RecordInfoBegin, exception, error msg = %s.", e.what());
        return 0;
    }
    catch (void*)
    {
        LOGFMTE("Camera6467_VFR::RecordInfoBegin,  void* exception");
        return 0;
    }
    catch (...)
    {
        LOGFMTE("Camera6467_VFR::DeleteFrontResult,  unknown exception");
        return 0;
    }
}

int Camera6467_VFR::RecordInfoEnd(DWORD dwCarID)
{
    try
    {
        WriteFormatLog("RecordInfoEnd, dwCarID = %lu", dwCarID);
        CHECK_ARG(m_pResult);

        EnterCriticalSection(&m_csResult);
        if (m_pLastResult
            && m_pLastResult->dwCarID == dwCarID)
        {            
            if (strlen(m_pLastResult->chSaveFileName) > 0)
            {
                memset(m_pResult->chSaveFileName, '\0', sizeof(m_pResult->chSaveFileName));
                memcpy(m_pResult->chSaveFileName, m_pLastResult->chSaveFileName, strlen(m_pLastResult->chSaveFileName));
            }
        }
        m_pLastResult = std::shared_ptr<CameraResult>(new CameraResult(*m_pResult));        
        LeaveCriticalSection(&m_csResult);

        SetLastResultIfReceiveComplete(true);

        if (dwCarID == m_pResult->dwCarID)
        {
            if (CheckIfBackUpVehicle(m_pResult))
            {
                WriteFormatLog("current result is reversing car, drop this result.");
            }
            else
            {
                if (CheckIfSuperLength(m_pResult))
                {
                    WriteFormatLog("current length %f is larger than max length %d, clear list first.", m_pResult->fVehLenth, m_iSuperLenth);
                   // m_resultList.ClearALL();
                    //m_MySemaphore.resetCount(GetCurrentThreadId());
                    m_VfrResultList.ClearALLResult();                    
                }
                WriteFormatLog("push one result to list, current list plate NO:\n");
                if (!m_VfrResultList.empty())
                {
                    BaseCamera::WriteLog(m_VfrResultList.GetAllPlateString().c_str());
                }
                else
                {
                    WriteFormatLog("list is empty.");
                }
                std::shared_ptr<CameraResult> pResult(m_pResult);

				std::shared_ptr<CameraResult> pLastSameResult = m_VfrResultList.GetOneByCarid(dwCarID);
				if (pLastSameResult)
				{
					WriteFormatLog("current car ID  %lu is already receive, replace it.", dwCarID, m_dwLastCarID);

					if (pLastSameResult
						&&strlen(pLastSameResult->chSaveFileName) > 0
						&& pLastSameResult->dwCarID == dwCarID)
					{
						WriteFormatLog("last car ID  %lu , avi fileName = %s.", pLastSameResult->dwCarID, pLastSameResult->chSaveFileName);
						strcpy_s(pResult->chSaveFileName, pLastSameResult->chSaveFileName);
					}
					m_VfrResultList.ReplaceByCarID(dwCarID, pResult);
				}
				else
				{
					m_dwLastCarID = dwCarID;
					//m_MySemaphore.notify(GetCurrentThreadId());
					if (m_VfrResultList.size() >= 10)
					{
						WriteFormatLog("m_VfrResultList size larger than 10 , remvoe first one.");
						m_VfrResultList.pop_front();
					}
					m_VfrResultList.push_back(pResult);
				}
                
                WriteFormatLog("after push, list plate NO:\n");
                BaseCamera::WriteLog(m_VfrResultList.GetAllPlateString().c_str());                

                //SendResultByCallback(pResult);

                m_pResult = NULL;
            }

            if (NULL != m_hMsgHanldle)
            {
                WriteLog("PostMessage");
                //::PostMessage(*((HWND*)m_hWnd),m_iMsg, 1, 0);
                ::PostMessage((HWND)m_hMsgHanldle, m_iResultMsg, (WPARAM)1, 0);
            }
        }
        else
        {
            WriteFormatLog("current car ID  %lu is not same wit result carID %lu.", dwCarID, m_pResult->dwCarID);
        }

        WriteFormatLog("RecordInfoEnd, finish");
        return 0;
    }
    catch (bad_exception& e)
    {
        LOGFMTE("RecordInfoEnd, bad_exception, error msg = %s", e.what());
        return 0;
    }
    catch (bad_alloc& e)
    {
        LOGFMTE("RecordInfoEnd, bad_alloc, error msg = %s", e.what());
        return 0;
    }
    catch (exception& e)
    {
        LOGFMTE("RecordInfoEnd, exception, error msg = %s.", e.what());
        return 0;
    }
    catch (void*)
    {
        LOGFMTE("RecordInfoEnd,  void* exception");
        return 0;
    }
    catch (...)
    {
        LOGFMTE("RecordInfoEnd, unknown exception");
        return 0;
    }
}

int Camera6467_VFR::RecordInfoPlate(DWORD dwCarID,
    LPCSTR pcPlateNo, 
    LPCSTR pcAppendInfo,
    DWORD dwRecordType,
    DWORD64 dw64TimeMS)
{
    try
    {
        WriteFormatLog("RecordInfoPlate, dwCarID = %lu, plateNo = %s, dwRecordType= %x, dw64TimeMS= %I64u",
            dwCarID,
            pcPlateNo,
            dwRecordType,
            dw64TimeMS);
        BaseCamera::WriteLog(pcAppendInfo);
        CHECK_ARG(m_pResult);

        SetLastResultIfReceiveComplete(false);

        if (dwCarID == m_pResult->dwCarID)
        {
            m_pResult->dw64TimeMS = dw64TimeMS;
            m_pResult->dwReceiveTime = GetTickCount();
            strcpy_s(m_pResult->chPlateNO, sizeof(m_pResult->chPlateNO), pcPlateNo);
            if (strlen(pcAppendInfo) < sizeof(m_pResult->pcAppendInfo))
            {
                strcpy_s(m_pResult->pcAppendInfo, sizeof(m_pResult->pcAppendInfo), pcAppendInfo);
                AnalysisAppendXML(m_pResult);
            }
            else
            {
                WriteFormatLog("strlen(pcAppendInfo)< sizeof(m_pResult->pcAppendInfo), can not AnalysisAppendXML.");
            }
        }
        else
        {
            WriteFormatLog("current car ID  %lu is not same wit result carID %lu.", dwCarID, m_pResult->dwCarID);
        }

		if (m_dwLastCarID != dwCarID && m_bSaveVideoEnable)
        {
			std::string strFinalString = createVideoFileName(m_pResult->chPlateTime);
			const char* pChVideoPath = strFinalString.c_str();

            if (!Tool_CheckIfFileNameIntheList(m_lsVideoName, pChVideoPath, &m_csFuncCallback))
            {
                StartToSaveAviFile(0, pChVideoPath, (m_pResult->dw64TimeMS - getVideoAdvanceTime() * 1000));

				Tool_CopyStringToBuffer(m_pResult->chSaveFileName, sizeof(m_pResult->chSaveFileName), pChVideoPath);
				WriteFormatLog("current car ID  %lu , video fileName = %s.", dwCarID, pChVideoPath);

                StopSaveAviFile(0, m_pResult->dw64TimeMS + getVideoDelayTime() * 1000);

                Tool_AddFileNameToTheList(m_lsVideoName, pChVideoPath, &m_csFuncCallback, MAX_FILENAME_LIST_SIZE);
            }
            else
            {
				WriteFormatLog("current car ID  %lu ,but avi fileName = %s is already save, so do not save it.", dwCarID, pChVideoPath);
            }
        }

        WriteFormatLog("RecordInfoPlate, finish.");
        return 0;
    }
    catch (bad_exception& e)
    {
        LOGFMTE("RecordInfoPlate, bad_exception, error msg = %s", e.what());
        return 0;
    }
    catch (bad_alloc& e)
    {
        LOGFMTE("RecordInfoPlate, bad_alloc, error msg = %s", e.what());
        return 0;
    }
    catch (exception& e)
    {
        LOGFMTE("RecordInfoPlate, exception, error msg = %s.", e.what());
        return 0;
    }
    catch (void*)
    {
        LOGFMTE("RecordInfoPlate,  void* exception");
        return 0;
    }
    catch (...)
    {
        LOGFMTE("RecordInfoPlate, unknown exception");
        return 0;
    }
}

int Camera6467_VFR::RecordInfoBigImage(DWORD dwCarID, 
    WORD wImgType,
    WORD wWidth, 
    WORD wHeight, 
    PBYTE pbPicData,
    DWORD dwImgDataLen,
    DWORD dwRecordType, 
    DWORD64 dw64TimeMS)
{
    WriteFormatLog("RecordInfoBigImage, dwCarID = %lu, wImgType = %u, wWidth= %u, wHeight= %u, \
        dwImgDataLen= %lu, dwRecordType = %x, dw64TimeMS = %I64u.",
        dwCarID,
        wImgType,
        wWidth,
        wHeight,
        dwImgDataLen,
        dwRecordType,
        dw64TimeMS);

    CHECK_ARG(m_pResult);    
    SetLastResultIfReceiveComplete(false);


    char* pSavePath = NULL;
    std::string strPlateTime(m_pResult->chPlateTime);

    char chImgDir[256] = { 0 };
    GetImageDir(chImgDir, sizeof(chImgDir));
    strcat(chImgDir, "\\");
    strcat(chImgDir, RESULT_DIR_NAME);
    strcat(chImgDir, "\\");
    
    if (dwCarID == m_pResult->dwCarID)
    {        
        if (wImgType == RECORD_BIGIMG_BEST_SNAPSHOT)
        {
            WriteFormatLog("RecordInfoBigImage BEST_SNAPSHO  ");

            CopyDataToIMG(m_pResult->CIMG_BestSnapshot, pbPicData, wWidth, wHeight, dwImgDataLen, wImgType);

            pSavePath = m_pResult->CIMG_BestSnapshot.chSavePath;
            //sprintf_s(pSavePath, 256, "%s\\%s\\%I64u-%s-front.jpg",
            //    m_chImageDir,
            //    m_pResult->chPlateTime,
            //    m_pResult->dw64TimeMS,
            //    m_pResult->chPlateNO);
            //sprintf_s(pSavePath, 256, "%s\\%s\\%s\\%s\\%lu-%I64u-front.jpg",
            //    m_chImageDir,
            //    strPlateTime.substr(0, 4).c_str(),
            //    strPlateTime.substr(4, 2).c_str(),
            //    strPlateTime.substr(6, 2).c_str(),
            //    dwCarID,
            //    m_pResult->dw64TimeMS);
            sprintf_s(pSavePath, 256, "%s\\%s-%s-%s\\%s_bestSnapShot_front.jpg",
                chImgDir,
                strPlateTime.substr(0, 4).c_str(),
                strPlateTime.substr(4, 2).c_str(),
                strPlateTime.substr(6, 2).c_str(),
                m_pResult->chPlateTime);
        }
        else if (wImgType == RECORD_BIGIMG_LAST_SNAPSHOT)
        {
            WriteFormatLog("RecordInfoBigImage LAST_SNAPSHOT  ");

            CopyDataToIMG(m_pResult->CIMG_LastSnapshot, pbPicData, wWidth, wHeight, dwImgDataLen, wImgType);

            pSavePath = m_pResult->CIMG_LastSnapshot.chSavePath;
            //sprintf_s(pSavePath, 256, "%s\\%s\\%lu-%I64u-front.jpg",
            //    m_chImageDir,
            //    m_pResult->chPlateTime,
            //    dwCarID,
            //    m_pResult->dw64TimeMS);
            //sprintf_s(pSavePath, 256, "%s\\%s\\%s\\%s\\%lu-%I64u-front.jpg",
            //    m_chImageDir,
            //    strPlateTime.substr(0, 4).c_str(),
            //    strPlateTime.substr(4, 2).c_str(),
            //    strPlateTime.substr(6, 2).c_str(),
            //    dwCarID,
            //    m_pResult->dw64TimeMS);
            sprintf_s(pSavePath, 256, "%s\\%s-%s-%s\\%s_LastSnapShot.jpg",
                chImgDir,
                strPlateTime.substr(0, 4).c_str(),
                strPlateTime.substr(4, 2).c_str(),
                strPlateTime.substr(6, 2).c_str(),
                m_pResult->chPlateTime);

        }
        else if (wImgType == RECORD_BIGIMG_BEGIN_CAPTURE)
        {
            WriteFormatLog("RecordInfoBigImage BEGIN_CAPTURE  ");

            CopyDataToIMG(m_pResult->CIMG_BeginCapture, pbPicData, wWidth, wHeight, dwImgDataLen, wImgType);

            pSavePath = m_pResult->CIMG_BeginCapture.chSavePath;
            //sprintf_s(pSavePath, 256, "%s\\%s\\%lu-%I64u-front.jpg",
            //    m_chImageDir,
            //    m_pResult->chPlateTime,
            //    dwCarID,
            //    m_pResult->dw64TimeMS);
            //sprintf_s(pSavePath, 256, "%s\\%s\\%s\\%s\\%lu-%I64u-front.jpg",
            //    m_chImageDir,
            //    strPlateTime.substr(0, 4).c_str(),
            //    strPlateTime.substr(4, 2).c_str(),
            //    strPlateTime.substr(6, 2).c_str(),
            //    dwCarID,
            //    m_pResult->dw64TimeMS);
            sprintf_s(pSavePath, 256, "%s\\%s-%s-%s\\%s_front.jpg",
                chImgDir,
                strPlateTime.substr(0, 4).c_str(),
                strPlateTime.substr(4, 2).c_str(),
                strPlateTime.substr(6, 2).c_str(),
                m_pResult->chPlateTime);
        }
        else if (wImgType == RECORD_BIGIMG_BEST_CAPTURE)
        {
            WriteFormatLog("RecordInfoBigImage BEST_CAPTURE  ");

            CopyDataToIMG(m_pResult->CIMG_BestCapture, pbPicData, wWidth, wHeight, dwImgDataLen, wImgType);

            pSavePath = m_pResult->CIMG_BestCapture.chSavePath;
            //sprintf_s(pSavePath, 256, "%s\\%s\\%lu-%I64u-side.jpg",
            //    m_chImageDir,
            //    m_pResult->chPlateTime,
            //    dwCarID,
            //    m_pResult->dw64TimeMS);
            //sprintf_s(pSavePath, 256, "%s\\%s\\%s\\%s\\%lu-%I64u-side.jpg",
            //    m_chImageDir,
            //    strPlateTime.substr(0, 4).c_str(),
            //    strPlateTime.substr(4, 2).c_str(),
            //    strPlateTime.substr(6, 2).c_str(),
            //    dwCarID,
            //    m_pResult->dw64TimeMS);
            sprintf_s(pSavePath, 256, "%s\\%s-%s-%s\\%s_side.jpg",
                chImgDir,
                strPlateTime.substr(0, 4).c_str(),
                strPlateTime.substr(4, 2).c_str(),
                strPlateTime.substr(6, 2).c_str(),
                m_pResult->chPlateTime);
        }
        else if (wImgType == RECORD_BIGIMG_LAST_CAPTURE)
        {
            WriteFormatLog("RecordInfoBigImage LAST_CAPTURE  ");

            CopyDataToIMG(m_pResult->CIMG_LastCapture, pbPicData, wWidth, wHeight, dwImgDataLen, wImgType);

            pSavePath = m_pResult->CIMG_LastCapture.chSavePath;
            //sprintf_s(pSavePath, 256, "%s\\%s\\%lu-%I64u-tail.jpg",
            //    m_chImageDir,
            //    m_pResult->chPlateTime,
            //    dwCarID,
            //    m_pResult->dw64TimeMS);
            //sprintf_s(pSavePath, 256, "%s\\%s\\%s\\%s\\%lu-%I64u-tail.jpg",
            //    m_chImageDir,
            //    strPlateTime.substr(0, 4).c_str(),
            //    strPlateTime.substr(4, 2).c_str(),
            //    strPlateTime.substr(6, 2).c_str(),
            //    dwCarID,
            //    m_pResult->dw64TimeMS);
            sprintf_s(pSavePath, 256, "%s\\%s-%s-%s\\%s_tail.jpg",
                chImgDir,
                strPlateTime.substr(0, 4).c_str(),
                strPlateTime.substr(4, 2).c_str(),
                strPlateTime.substr(6, 2).c_str(),
                m_pResult->chPlateTime);
        }
        else
        {
            WriteFormatLog("RecordInfoBigImage other Image, put it to  LAST_CAPTURE .");
            CopyDataToIMG(m_pResult->CIMG_LastCapture, pbPicData, wWidth, wHeight, dwImgDataLen, wImgType);

            pSavePath = m_pResult->CIMG_LastCapture.chSavePath;
            //sprintf_s(pSavePath, 256, "%s\\%s\\%lu-%I64u-tail.jpg",
            //    m_chImageDir,
            //    m_pResult->chPlateTime,
            //    dwCarID,
            //    m_pResult->dw64TimeMS);
            //sprintf_s(pSavePath, 256, "%s\\%s\\%s\\%s\\%lu-%I64u-tail.jpg",
            //    m_chImageDir,
            //    strPlateTime.substr(0, 4).c_str(),
            //    strPlateTime.substr(4, 2).c_str(),
            //    strPlateTime.substr(6, 2).c_str(),
            //    dwCarID,
            //    m_pResult->dw64TimeMS);
            sprintf_s(pSavePath, 256, "%s\\%s-%s-%s\\%s_tail.jpg",
                chImgDir,
                strPlateTime.substr(0, 4).c_str(),
                strPlateTime.substr(4, 2).c_str(),
                strPlateTime.substr(6, 2).c_str(),
                m_pResult->chPlateTime);
        }
        //if (NULL == strstr(pSavePath, "front"))
        //{
        //    if (Tool_SaveFileToPath(pSavePath, pbPicData, dwImgDataLen))
        //    {
        //        WriteFormatLog("RecordInfoBigImage %s save success .", pSavePath);
        //    }
        //    else
        //    {
        //        WriteFormatLog("RecordInfoBigImage %s save failed .", pSavePath);
        //    }           
        //}
        //Tool_SaveFileToPath(pSavePath, pbPicData, dwImgDataLen);
    }
    else
    {
        WriteFormatLog("current car ID  %lu is not same wit result carID %lu.", dwCarID, m_pResult->dwCarID);
    }

    WriteFormatLog("RecordInfoBigImage , finish.");
    return 0;
}

int Camera6467_VFR::RecordInfoSmallImage(DWORD dwCarID,
    WORD wWidth,
    WORD wHeight,
    PBYTE pbPicData, 
    DWORD dwImgDataLen, 
    DWORD dwRecordType,
    DWORD64 dw64TimeMS)
{
    WriteFormatLog("RecordInfoSmallImage, dwCarID = %lu, wWidth= %u, wHeight= %u, \
                                                                                    dwImgDataLen= %lu, dwRecordType = %x, dw64TimeMS = %I64u.",
                                                                                    dwCarID,
                                                                                    wWidth,
                                                                                    wHeight,
                                                                                    dwImgDataLen,
                                                                                    dwRecordType,
                                                                                    dw64TimeMS);

    CHECK_ARG(m_pResult);
    SetLastResultIfReceiveComplete(false);

    int iBuffLen = dwImgDataLen;
    if (m_pResult->dwCarID == dwCarID)
    {
        if (m_Small_IMG_Temp.pbImgData == NULL)
        {
            WriteFormatLog("malloc first small image buffer .");
            m_Small_IMG_Temp.pbImgData = new BYTE[MAX_IMG_SIZE];
            memset(m_Small_IMG_Temp.pbImgData, 0, MAX_IMG_SIZE);
        }

        if (m_Small_IMG_Temp.pbImgData)
        {
            int iDestLenth = MAX_IMG_SIZE;
            memset(m_Small_IMG_Temp.pbImgData, 0, MAX_IMG_SIZE);
            WriteFormatLog("convert yuv to bmp , begin .");

            HRESULT Hr = HVAPIUTILS_SmallImageToBitmapEx(pbPicData,
                wWidth,
                wHeight,
                m_Small_IMG_Temp.pbImgData,
                &iDestLenth);

            if (Hr == S_OK)
            {
                WriteFormatLog("convert YUV to bmp, success .");
                CopyDataToIMG(m_pResult->CIMG_PlateImage, m_Small_IMG_Temp.pbImgData, wWidth, wHeight, iDestLenth, 0);

                memset(m_Small_IMG_Temp.pbImgData, 0, MAX_IMG_SIZE);
                WriteFormatLog("convert bmp to jpg , begin .");

                size_t iBufLength = MAX_IMG_SIZE;
                bool bScale = Tool_Img_ScaleJpg(m_pResult->CIMG_PlateImage.pbImgData,
                    m_pResult->CIMG_PlateImage.dwImgSize,
                    m_Small_IMG_Temp.pbImgData,
                    &iBufLength,
                    m_pResult->CIMG_PlateImage.wImgWidth,
                    m_pResult->CIMG_PlateImage.wImgHeight,
                    80);
                if (bScale)
                {
                    WriteFormatLog("convert bmp to jpeg success, begin copy.");
                    CopyDataToIMG(m_pResult->CIMG_PlateImage, m_Small_IMG_Temp.pbImgData, wWidth, wHeight, iBufLength, 0);
                    WriteFormatLog("convert bmp to jpeg success, finish copy.");
                }
                else
                {
                    WriteFormatLog("convert bmp to jpeg failed, use default.");
                }
            }
            else
            {
                WriteFormatLog("convert YUV to bmp , failed, use default.");
                CopyDataToIMG(m_pResult->CIMG_PlateImage, pbPicData, wWidth, wHeight, dwImgDataLen, 0);
            }

            if (m_pResult->CIMG_PlateImage.pbImgData
                && m_pResult->CIMG_PlateImage.dwImgSize > 0)
            {
                char* pSavePath = NULL;
                std::string strPlateTime(m_pResult->chPlateTime);

                pSavePath = m_pResult->CIMG_PlateImage.chSavePath;
                //sprintf_s(pSavePath, 256, "%s\\%s\\%lu-%I64u-plate.jpg",
                //    m_chImageDir,
                //    m_pResult->chPlateTime,
                //    dwCarID,
                //    m_pResult->dw64TimeMS);
                sprintf_s(pSavePath, 256, "%s\\%s\\%s\\%s\\%s-front-plate.jpg",
                    m_chImageDir,
                    strPlateTime.substr(0, 4).c_str(),
                    strPlateTime.substr(4, 2).c_str(),
                    strPlateTime.substr(6, 2).c_str(),
                    m_pResult->chPlateTime);

                //Tool_SaveFileToPath(pSavePath, m_pResult->CIMG_PlateImage.pbImgData, m_pResult->CIMG_PlateImage.dwImgSize);
            }
        }
        else
        {
            WriteFormatLog("malloc first small image buffer failed .");
        }
    }

    WriteFormatLog("RecordInfoSmallImage, finish.");
    return 0;
}

int Camera6467_VFR::RecordInfoBinaryImage(DWORD dwCarID, 
    WORD wWidth, 
    WORD wHeight, 
    PBYTE pbPicData, 
    DWORD dwImgDataLen,
    DWORD dwRecordType, 
    DWORD64 dw64TimeMS)
{
    WriteFormatLog("RecordInfoBinaryImage, dwCarID = %lu, wWidth= %u, wHeight= %u, \
                                                                                    dwImgDataLen= %lu, dwRecordType = %x, dw64TimeMS = %I64u.",
                                                                                    dwCarID,
                                                                                    wWidth,
                                                                                    wHeight,
                                                                                    dwImgDataLen,
                                                                                    dwRecordType,
                                                                                    dw64TimeMS);
    CHECK_ARG(m_pResult);
    SetLastResultIfReceiveComplete(false);
    if (dwCarID == m_pResult->dwCarID)
    {
        CopyDataToIMG(m_pResult->CIMG_BinImage, pbPicData, wWidth, wHeight, dwImgDataLen, 0);
    }
    else
    {
        WriteFormatLog("current car ID  %lu is not same wit result carID %lu.", dwCarID, m_pResult->dwCarID);
    }
    WriteFormatLog("RecordInfoBinaryImage, finish.");
    return 0;
}

int Camera6467_VFR::DeviceJPEGStream(PBYTE pbImageData, 
    DWORD dwImageDataLen,
    DWORD dwImageType, 
    LPCSTR szImageExtInfo)
{
    if (m_iJpegCount++ > 100)
    {
		WRITE_LOG_FMT("pbImageData = %p,  dwImageDataLen= %lu, dwImageType= %lu",
            pbImageData,
            dwImageDataLen,
            dwImageType);

		m_iJpegCount = 0;

		//char chFileName[256] = {0};
		//_snprintf(chFileName, sizeof(chFileName), "%lu.jpg", GetTickCount());
		//Tool_SaveFileToPath(chFileName, pbImageData, dwImageDataLen);
    }

    EnterCriticalSection(&m_csResult);
    m_bJpegComplete = false;

    m_CIMG_StreamJPEG.dwImgSize = dwImageDataLen;
    m_CIMG_StreamJPEG.wImgWidth = 1920;
    m_CIMG_StreamJPEG.wImgHeight = 1080;
    if (NULL == m_CIMG_StreamJPEG.pbImgData)
    {
        m_CIMG_StreamJPEG.pbImgData = new unsigned char[MAX_IMG_SIZE];
        memset(m_CIMG_StreamJPEG.pbImgData, 0, MAX_IMG_SIZE);
    }
    if (m_CIMG_StreamJPEG.pbImgData)
    {
        memset(m_CIMG_StreamJPEG.pbImgData, 0, MAX_IMG_SIZE);
        memcpy(m_CIMG_StreamJPEG.pbImgData, pbImageData, dwImageDataLen);
        m_bJpegComplete = true;
    }
    LeaveCriticalSection(&m_csResult);

    return 0;
}

void Camera6467_VFR::CheckStatus()
{
    int iLastStatus = -1;
    INT64 iLastTick = 0, iCurrentTick = 0;
    int iFirstConnctSuccess = -1;

    while (!GetCheckThreadExit())
    {
        Sleep(50);
        iCurrentTick = GetTickCount();
        int iTimeInterval = GetTimeInterval();
        if ((iCurrentTick - iLastTick) >= (iTimeInterval * 1000))
        {
            //int iStatus = GetCamStatus();
			int iStatus = GetNetSatus();
            if (iStatus != iLastStatus)
            {
                if (iStatus == 0)
                {
                    //if (iStatus != iLastStatus)
                    //{
                    //	pThis->SendConnetStateMsg(true);
                    //}
                    //SendConnetStateMsg(true);
                    WriteLog("设备连接正常.");
                    iFirstConnctSuccess = 0;
                }
                else
                {
                    //SendConnetStateMsg(false);
                    WriteLog("设备连接失败, 尝试重连");

                    if (iFirstConnctSuccess == -1)
                    {
                        //pThis->ConnectToCamera();
                    }
                }
            }
            iLastStatus = iStatus;

            iLastTick = iCurrentTick;
        }
    }
}

bool Camera6467_VFR::checkIfHasThreePic(std::shared_ptr<CameraResult> result)
{
	if (result->CIMG_BestCapture.dwImgSize > 0 )
	{
		if (m_bSaveVideoEnable)
		{
			return CheckIfFileNameIntheVideoList(result->chSaveFileName);
		}
		return true;
	}

    //if ( result->CIMG_BestCapture.dwImgSize > 0
    //    && result->CIMG_LastCapture.dwImgSize > 0)
    //{
    //    return true;
    //}

    //if (result->CIMG_LastSnapshot.dwImgSize > 0/*
    //    && result->CIMG_BestCapture.dwImgSize <= 0*/)
    //{
    //    return true;
    //}

    //if (result->CIMG_BeginCapture.dwImgSize > 0
    //    && result->CIMG_BestCapture.dwImgSize > 0
    //    && result->CIMG_LastCapture.dwImgSize > 0)
    //{
    //    return true;
    //}
    return false;
}

DWORD Camera6467_VFR::getResultWaitTime()
{
    DWORD dwValue = 0;
    EnterCriticalSection(&m_csFuncCallback);
    dwValue = m_iWaitVfrTimeOut;
    LeaveCriticalSection(&m_csFuncCallback);
    return dwValue;
}

unsigned int Camera6467_VFR::SendResultThreadFunc()
{
    WriteFormatLog("SendResultThreadFunc begin.");
    std::list<DWORD> lsSentCarIdList;
    bool bNeedSendResult = false;
    while (!GetCheckThreadExit())
    {
        Sleep(100);
        if (!m_VfrResultList.empty())
        {
            //CameraResult* pResult = NULL;
            if (!bNeedSendResult)
            {
                SendStatues senSignal;
                if (GetSignalListSize() > 0)
                {
                    senSignal = getFrontSendSignal();
                    if (senSignal.iSendStatues != send_Finish)
                    {
                        deleteFrontSendSignal();
                        continue;
                    }
                    deleteFrontSendSignal();
                    bNeedSendResult = true;
                }
                else
                {
                    continue;
                }
            }

            std::shared_ptr<CameraResult> pResult;
            m_VfrResultList.front(pResult);
            if (NULL == pResult)
            {
                continue;
            }
            
            int iTimeDelay = GetTickCount() - pResult->dwReceiveTime;
            if (checkIfHasThreePic(pResult)
                || iTimeDelay >= getResultWaitTime())
            {
                if (std::end(lsSentCarIdList) == std::find(std::begin(lsSentCarIdList), std::end(lsSentCarIdList), pResult->dwCarID))
                {                    
                    if (pResult->dwCarID != 0xfffffff)
                    {
                        SendResultByCallback(pResult);
                    }
                    else
                    {
                        WriteFormatLog("SendResultThreadFunc current ID is special %lu, do not send.", pResult->dwCarID);
                    }
                    if (lsSentCarIdList.size() > 5)
                    {
                        lsSentCarIdList.pop_front();
                    }
                    lsSentCarIdList.push_back(pResult->dwCarID);
                    bNeedSendResult = false;
                }
                else
                {
                    WriteFormatLog("SendResultThreadFunc current ID %lu is send before, this time ignore.", pResult->dwCarID);
                }
                
                //if (pResult->dwCarID != dwLastSendCarID
                //    &&pResult->dwCarID != 0xfffffff)
                //{
                //    SendResultByCallback(pResult);
                //    dwLastSendCarID = pResult->dwCarID;
                //}
                
                if (CheckIfSetResultCallback())
                {
                    m_VfrResultList.DeleteToPosition(0);
                }                
            }                       

            pResult = NULL;
        }
    }
    WriteFormatLog("SendResultThreadFunc finish.");
    return 0;
}

unsigned int Camera6467_VFR::SendResultThreadFunc_lastResult()
{
    WriteFormatLog("SendResultThreadFunc begin.");
    std::list<DWORD> lsSentCarIdList;
    bool bNeedSendResult = false;
    while (!GetCheckThreadExit())
    {
        Sleep(100);
        if (GetLastResultIfReceiveComplete())
        {
            
            //CameraResult* pResult = NULL;
            if (!bNeedSendResult)
            {
                SendStatues senSignal;
                if (GetSignalListSize() > 0)
                {
                    senSignal = getFrontSendSignal();
                    if (senSignal.iSendStatues != send_Finish)
                    {
                        deleteFrontSendSignal();
                        continue;
                    }
                    deleteFrontSendSignal();
                    bNeedSendResult = true;
                    WriteFormatLog("last result is complete and get the signal.");
                }
                else
                {
                    continue;
                }
            }

            std::shared_ptr<CameraResult> pResult = GetLastResult();

            if (NULL == pResult)
            {
                continue;
            }
            WriteFormatLog("get last result success.");
            int iTimeDelay = GetTickCount() - pResult->dwReceiveTime;
            if (checkIfHasThreePic(pResult)
                || iTimeDelay >= getResultWaitTime())
            {
                if (std::end(lsSentCarIdList) == std::find(std::begin(lsSentCarIdList), std::end(lsSentCarIdList), pResult->dwCarID))
                {
                    if (pResult->dwCarID != 0xfffffff)
                    {
                        SendResultByCallback(pResult);
                    }
                    else
                    {
                        WriteFormatLog("SendResultThreadFunc current ID is special %lu, do not send.", pResult->dwCarID);
                    }
                    if (lsSentCarIdList.size() > 5)
                    {
                        lsSentCarIdList.pop_front();
                    }
                    lsSentCarIdList.push_back(pResult->dwCarID);
                    bNeedSendResult = false;
                }
                else
                {
                    WriteFormatLog("SendResultThreadFunc current ID %lu is send before, this time ignore.", pResult->dwCarID);
                }

                //if (pResult->dwCarID != dwLastSendCarID
                //    &&pResult->dwCarID != 0xfffffff)
                //{
                //    SendResultByCallback(pResult);
                //    dwLastSendCarID = pResult->dwCarID;
                //}
                SetLastResultIfReceiveComplete(false);
            }
            else
            {
                WriteFormatLog("last result %lu, is not Meet the criteria.", pResult->dwCarID);
            }

            pResult = NULL;
        }
    }
    WriteFormatLog("SendResultThreadFunc finish.");
    return 0;
}

unsigned int Camera6467_VFR::SendResultThreadFunc_WithNoSignal()
{
    WriteFormatLog("SendResultThreadFunc begin.");
    std::list<DWORD> lsSentCarIdList;
    bool bSendResult = false;
    std::shared_ptr<CameraResult> pResult;
#define EXCEPTION_TIME_OUT (30*1000)
    while (!GetCheckThreadExit())
    {
        Sleep(10);
		if (mode_noCallback == GetConnectMode())
		{
			continue;
		}
        //m_MySemaphore.wait(GetCurrentThreadId());
        if (RESULT_MODE_FRONT == m_iResultModule
            && GetResultListSize() > 0)
        {
            //m_MySemaphore.wait(GetCurrentThreadId());
            pResult = GetFrontResult();
        }
        else
        {
            if (GetLastResultIfReceiveComplete())
            {
                pResult = GetLastResult();                
            }            
        } 

        if (NULL == pResult)
        {
            continue;
        }
        WriteFormatLog("SendResultThreadFunc_WithNoSignal:: get  result success.");
        int iTimeDelay = GetTickCount() - pResult->dwReceiveTime;
        if (checkIfHasThreePic(pResult)
            || iTimeDelay >= getResultWaitTime())
        {
            if (std::end(lsSentCarIdList) == std::find(std::begin(lsSentCarIdList), std::end(lsSentCarIdList), pResult->dwCarID))
            {
                if (pResult->dwCarID != 0xfffffff)
                {
                    SendResultByCallback(pResult);
                }
                else
                {
                    WriteFormatLog("SendResultThreadFunc current ID is special %lu, do not send.", pResult->dwCarID);
                }
                if (lsSentCarIdList.size() > 5)
                {
                    lsSentCarIdList.pop_front();
                }
                lsSentCarIdList.push_back(pResult->dwCarID);
            }
            else
            {
                WriteFormatLog("SendResultThreadFunc current ID %lu is send before, this time ignore.", pResult->dwCarID);
            }
            SetLastResultIfReceiveComplete(false);
            bSendResult = true;
        }
        else
        {
            WriteFormatLog("last result %lu , plate number = %s, is not Meet the criteria.", pResult->dwCarID, pResult->chPlateNO);
        }

        if (iTimeDelay > EXCEPTION_TIME_OUT)
        {
            WriteFormatLog("first result carID= %lu ,plate number = %s , plate time = %s, has kept more than %d ms, clear all result in the list.", 
                pResult->dwCarID, 
                pResult->chPlateNO,
                pResult->chPlateTime,
                EXCEPTION_TIME_OUT);
            //m_MySemaphore.resetCount(GetCurrentThreadId());
            ClearALLResult();
        }

        if (RESULT_MODE_FRONT == m_iResultModule)
        {
            if (CheckIfSetResultCallback()
                && bSendResult)
            {
                DeleteFrontResult(NULL);
                bSendResult = false;
            }
            //else
            //{
            //    m_MySemaphore.notify(GetCurrentThreadId());
            //}
        }

        pResult = NULL;
    }
    WriteFormatLog("SendResultThreadFunc finish.");
    return 0;
}

void Camera6467_VFR::copyStringToBuffer(char* bufer, size_t bufLen, const char * srcStr)
{
    if (NULL == bufer
        || bufLen == 0
        || NULL == srcStr)
    {
        return;
    }
    memset(bufer, '\0', bufLen);
    if (bufLen > strlen(srcStr))
    {
        memcpy(bufer, srcStr, strlen(srcStr));
    }
    else
    {
        memcpy(bufer, srcStr, bufLen-1);
    }
    return;
}

unsigned int Camera6467_VFR::DeleteLogThreadFunc()
{
    char chDirPath[256] = {0};
    int iTryTime = 3600;
    while (!GetCheckThreadExit())
    {
        Sleep(100);
        if (iTryTime ++ < 10*10)
        {
            continue;
        }
        iTryTime = 0;
        if (GetLogPath(chDirPath, sizeof(chDirPath))
            && strlen(chDirPath) > 0)
        {
            std::string strLogDir(chDirPath);
            strLogDir.append("\\").append(LOG_DIR_NAME).append("\\");
            Tool_LoopDeleteSpecificFormatDirectory(strLogDir.c_str(), GetLogHoldDays());
        }       
    }
    return 0;
}

unsigned int Camera6467_VFR::DeleteResultThreadFunc()
{
    char chDirPath[256] = { 0 };
    int iTryTime = 3600;
    while (!GetCheckThreadExit())
    {
        Sleep(100);
        if (GetResultHoldDay() <= 0)
        {
            continue;
        }
        if (iTryTime++ < 3600 * 1 )
        {
            continue;
        }

        iTryTime = 0;
        GetImageDir(chDirPath, sizeof(chDirPath));
        if (strlen(chDirPath) > 0)
        {
            std::string strResultDir(chDirPath);
            strResultDir.append("\\").append(RESULT_DIR_NAME).append("\\");
            Tool_LoopDeleteSpecificFormatDirectory(strResultDir.c_str(), GetResultHoldDay());
        }
    }
    return 0;
}

void Camera6467_VFR::SetResultSendSignal(int iSignalID, int iSendStatus)
{
    EnterCriticalSection(&m_csFuncCallback);
    SendStatues signal;
    signal.iSendID = iSignalID;
    signal.iSendStatues = iSendStatus;
    if (iSignalID == -1) 
    { 
        StatusList.push_back(signal);
    }
    else
    {
        for (std::list<SendStatues>::iterator it = StatusList.begin(); it != StatusList.end(); it++)
        {
            if (it->iSendID == signal.iSendID)
            {
                it->iSendStatues = signal.iSendStatues;
                break;
            }
        }
    }
    LeaveCriticalSection(&m_csFuncCallback);
}

SendStatues Camera6467_VFR::getFrontSendSignal()
{
    SendStatues signal;
    EnterCriticalSection(&m_csFuncCallback);
    if (StatusList.size() > 0)
    {
        signal = StatusList.front();
    }    
    LeaveCriticalSection(&m_csFuncCallback);
    return signal;
}

void Camera6467_VFR::deleteFrontSendSignal()
{
    EnterCriticalSection(&m_csFuncCallback);
	if (!StatusList.empty())
	{
		StatusList.pop_front();
	}    
    LeaveCriticalSection(&m_csFuncCallback);
}

int Camera6467_VFR::GetSignalListSize()
{
    int iSize = 0;
    EnterCriticalSection(&m_csFuncCallback);
    iSize = StatusList.size();
    LeaveCriticalSection(&m_csFuncCallback);
    return iSize;
}

int Camera6467_VFR::GetResultMode()
{
    return m_iResultModule;
}

