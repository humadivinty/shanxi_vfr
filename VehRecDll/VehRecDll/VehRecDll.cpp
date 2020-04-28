// VehRecDll.cpp : 定义 DLL 应用程序的导出函数。
//

#include "stdafx.h"
#include "VehRecDll.h"
#include "utilityTool/ToolFunction.h"
#include "cameraModule/Camera6467_VFR.h"
#include "cameraModule/DeviceListManager.h"
#include"HvDevice/HvDeviceCommDef.h"
#include "SendStatues_def.h"
#include <list>

#ifdef WINDOWS
#define WRITE_LOG(fmt, ...) Tool_WriteFormatLog("%s:: "fmt, __FUNCTION__, ##__VA_ARGS__);
#else
//#define WRITE_LOG(...) Tool_WriteFormatLog("%s:: ", __FUNCTION__, ##__VA_ARGS__);
#define WRITE_LOG(fmt,...) Tool_WriteFormatLog("%s:: " fmt, __FUNCTION__,##__VA_ARGS__);
#endif

#define BASIC_NUMBER (1000)
#define MAX_CAMERA_COUNT (10)

extern char g_chLogPath[256];
int g_iLogHoldDays = 30;
CameraIMG g_CIMG_StreamJPEG;

std::list<unsigned long> g_SentCarList;
bool FuncfindIfSendBefore(std::list<unsigned long>& carIDList, unsigned long carID)
{
	if (carIDList.size() <= 0)
	{
		return false;
	}
	if (std::end(carIDList) == std::find(std::begin(carIDList), std::end(carIDList), carID))
	{
		return false;
	}
	return true;
}

void AddCarIDToTheList(std::list<unsigned long>& carIDList, unsigned long carID)
{
	if (carIDList.size() > 5)
	{
		carIDList.pop_front();
	}
	carIDList.push_back(carID);
}


VEHRECDLL_API int WINAPI VehRec_InitEx(int iLog, char *iLogPath, int iLogSaveDay)
{
    Tool_SetLogPath(iLogPath);
    WRITE_LOG("iLog = %d, iLogPath = %s,iLogSaveDay = %d ", iLog, iLogPath, iLogSaveDay);
    if (NULL == iLogPath 
        || (iLogSaveDay <= 0 && iLog > 0))
    {
        WRITE_LOG("the parameter is invalid.");
        return -1;
    }
    g_iLogHoldDays = iLogSaveDay;

    char chTemp[256] = {0};
    Tool_GetRootPathFromFileName(iLogPath, chTemp, sizeof(chTemp));
    //int iRet = PathIsRoot(chTemp);
    if (DRIVE_FIXED != GetDriveType(chTemp))
    {
        WRITE_LOG("the root path of %s is not  presence.",  iLogPath);
        return -1;
    }

    for (int i = 0; i < MAX_CAMERA_COUNT; i++)
    {
        BaseCamera* pCamera = DeviceListManager::GetInstance()->GetDeviceById(i);
        if (NULL != pCamera)
        {
            pCamera->SetLogPath(iLogPath);
            pCamera->SetLogHoldDays(iLogSaveDay);
        }
    }
    WRITE_LOG("finish");
    return 0;
}

VEHRECDLL_API int WINAPI VehRec_Free()
{
    WRITE_LOG("begin");
    DeviceListManager::GetInstance()->ClearAllDevice();
    WRITE_LOG("finish.");
    return 0;
}

VEHRECDLL_API int WINAPI VehRec_Connect(char *devIP, char *savepath)
{
    WRITE_LOG("begin, devIP = %s, savepath = %s", devIP, savepath);
    int iRet = -4;

    if (1 != Tool_checkIP(devIP))
    {
        WRITE_LOG("ip address is invlalid, return %d.", iRet);
        return iRet;
    }

    char chTemp[256] = { 0 };
    Tool_GetRootPathFromFileName(savepath, chTemp, sizeof(chTemp));
    if (!PathIsRoot(chTemp))
    {
        WRITE_LOG("the root path of %s is not  presence.", savepath);
        return -1;
    }

    BaseCamera* pCamera = DeviceListManager::GetInstance()->GetDeviceByIpAddress(devIP);
    if (pCamera != NULL)
    {
        int iHandle = DeviceListManager::GetInstance()->GetDeviceIdByIpAddress(devIP);
        WRITE_LOG("find device %s, ID = %d", devIP, iHandle);
        iRet = iHandle;
    }
    else
    {
        for (int i = 0; i < MAX_CAMERA_COUNT; i++)
        {
            if (NULL == DeviceListManager::GetInstance()->GetDeviceById(i))
            {
                Camera6467_VFR* pCamera = new Camera6467_VFR();
                pCamera->SetCameraIP(devIP);
                pCamera->SetLoginID(i + BASIC_NUMBER);
                pCamera->SetImageDir(savepath);
                pCamera->SetLogHoldDays(g_iLogHoldDays);
				pCamera->SetConnectMode(mode_noCallback);				
                if (strlen(g_chLogPath) > 0)
                {
                    pCamera->SetLogPath(g_chLogPath);
                }
                if (pCamera->ConnectToCamera() == 0)
                {
					pCamera->SetJpegStreamCallback();
                    WRITE_LOG("connect to camera success.");
                }
                else
                {
                    WRITE_LOG("connect to camera failed.");
                }
                iRet = i;
                DeviceListManager::GetInstance()->AddOneDevice(i, pCamera);
                WRITE_LOG("create camera success, device id= %d, ip = %s", i, devIP);
                break;
            }
        }
    }
    iRet = iRet >= 0 ? iRet + BASIC_NUMBER : iRet;

    WRITE_LOG("finish, return %d", iRet);
    return iRet;
}

VEHRECDLL_API int WINAPI VehRec_ConnectEX(char *devIP, char *savepath, VehRec_CarData callBackFun)
{
    WRITE_LOG("begin, devIP = %s, savepath = %s", devIP, savepath);
    int iRet = -4;

    if (1 != Tool_checkIP(devIP))
    {
        WRITE_LOG("ip address is invlalid, return %d.", iRet);
        return iRet;
    }

    char chTemp[256] = { 0 };
    Tool_GetRootPathFromFileName(savepath, chTemp, sizeof(chTemp));
    if (!PathIsRoot(chTemp))
    {
        WRITE_LOG("the root path of %s is not  presence.", savepath);
        return -1;
    }

    BaseCamera* pCamera = DeviceListManager::GetInstance()->GetDeviceByIpAddress(devIP);
    if (pCamera != NULL)
    {
        int iHandle = DeviceListManager::GetInstance()->GetDeviceIdByIpAddress(devIP);
        WRITE_LOG("find device %s, ID = %d", devIP, iHandle);
        iRet = iHandle;
    }
    else
    {
        for (int i = 0; i < MAX_CAMERA_COUNT; i++)
        {
            if (NULL == DeviceListManager::GetInstance()->GetDeviceById(i))
            {
                Camera6467_VFR* pCamera = new Camera6467_VFR();
                pCamera->SetCameraIP(devIP);
                pCamera->SetLoginID(i + BASIC_NUMBER);
                pCamera->SetImageDir(savepath);
                pCamera->SetLogHoldDays(g_iLogHoldDays);
				pCamera->SetConnectMode(mode_callback);
                if (strlen(g_chLogPath) > 0)
                {
                    pCamera->SetLogPath(g_chLogPath);
                }
                if (pCamera->ConnectToCamera() == 0)
                {
                    WRITE_LOG("connect to camera success.");
                    //pCamera->SetH264Callback(0, 0, 0, H264_RECV_FLAG_REALTIME);
                    pCamera->SetResultCallback(callBackFun, NULL);
                }
                else
                {
                    WRITE_LOG("connect to camera failed.");
                }
                iRet = i;
                DeviceListManager::GetInstance()->AddOneDevice(i, pCamera);
                WRITE_LOG("create camera success, device id= %d, ip = %s", i, devIP);
                break;
            }
        }
    }
    iRet = iRet >= 0 ? iRet + BASIC_NUMBER : iRet;

    WRITE_LOG("finish, return %d", iRet);
    return iRet;
}

VEHRECDLL_API void WINAPI VehRec_DisConnect(int handle)
{
    WRITE_LOG("begin, handle = %d", handle);
    if (handle >= BASIC_NUMBER)
    {
        DeviceListManager::GetInstance()->EraseDevice(handle - BASIC_NUMBER);
    }
    WRITE_LOG("finish");
}

VEHRECDLL_API int WINAPI VehRec_VEHSignle(int handle, int sig)
{
    WRITE_LOG("begin, handle = %d, sig = %d", handle, sig);
    int iRet = -1;
    Camera6467_VFR* pCamera = (Camera6467_VFR*)DeviceListManager::GetInstance()->GetDeviceById(handle - BASIC_NUMBER);
    if (pCamera != NULL)
    {
        int iSendStatus = sig == 1 ? send_begin : send_Finish;
        pCamera->SetResultSendSignal(-1, iSendStatus);
        iRet = 0;
    }
    WRITE_LOG("finish, return %d", iRet);
    return iRet;
}

VEHRECDLL_API int WINAPI VehRec_GetCarData(int handle, char *colpic, char *platepic, char *recfile)
{
    WRITE_LOG("begin, handle = %d, colpic = %s, platepic = %s, recfile = %s", handle, colpic, platepic, recfile);
    if (NULL == colpic
        || NULL == platepic
        || NULL == recfile)
    {
        WRITE_LOG("parameter is invalid.");
        return -1;
    }
    Tool_MakeFileDir(colpic);
    Tool_MakeFileDir(platepic);
    Tool_MakeFileDir(recfile);

    int iRet = -1;
	bool bGetJpeg = false;
    Camera6467_VFR* pCamera = (Camera6467_VFR*)DeviceListManager::GetInstance()->GetDeviceById(handle - BASIC_NUMBER);
    if (pCamera != NULL)
    {
		if (mode_callback == pCamera->GetConnectMode())
		{
			WRITE_LOG("当前初始化为回调模式，不支持该接口调用");
			return -1;
		}

		for (int i = 0; i < 10; i++)
		{
			if (pCamera->GetOneJpegImg(g_CIMG_StreamJPEG))
			{
				WRITE_LOG("GetOneJpegImg success, data = %p, length = %lu",
					g_CIMG_StreamJPEG.pbImgData,
					g_CIMG_StreamJPEG.dwImgSize);
				bGetJpeg = true;
				break;
			}
			Sleep(50);
		}

        std::shared_ptr<CameraResult> pTempResult = nullptr;

        int iDelayTime = pCamera->getResultWaitTime();
        long iFirstTic= GetTickCount();
        long iCurrentTic = iFirstTic;		

        do
        {
            iCurrentTic = GetTickCount();
            bool bFind = false;

            if (RESULT_MODE_FRONT == pCamera->GetResultMode())
            {
                pTempResult = pCamera->GetFrontResult();
            }
            else
            {
                pTempResult = pCamera->GetLastResult();
            }

            if (pTempResult
                && pCamera->checkIfHasThreePic(pTempResult)
                && !FuncfindIfSendBefore(g_SentCarList, pTempResult->dwCarID)
                )
            {
                break;
            }
            if (pTempResult
                && FuncfindIfSendBefore(g_SentCarList, pTempResult->dwCarID))
            {
				WRITE_LOG("get result , plate number = %s, carID = %lu but it is sent before, search again.",
					pTempResult->chPlateNO,
					pTempResult->dwCarID);
				if (RESULT_MODE_FRONT == pCamera->GetResultMode())
				{
					WRITE_LOG("result mode == RESULT_MODE_FRONT,DeleteFrontResult. ");
					pCamera->DeleteFrontResult(NULL);
				}  
            }
            Sleep(50);
        } while (iCurrentTic < (iFirstTic + iDelayTime));

        if (pTempResult)
        {
            WRITE_LOG("get result success, plate number = %s, carID = %lu.", pTempResult->chPlateNO, pTempResult->dwCarID);
            if (!FuncfindIfSendBefore(g_SentCarList, pTempResult->dwCarID))
            {
				AddCarIDToTheList(g_SentCarList, pTempResult->dwCarID);
            }
            else
            {
                WRITE_LOG("current reesult, plate number = %s, carID = %lu is same with last one, through it, return -1.", pTempResult->chPlateNO, pTempResult->dwCarID);
				if (RESULT_MODE_FRONT == pCamera->GetResultMode())
				{
					WRITE_LOG("result mode == RESULT_MODE_FRONT,DeleteFrontResult. ");
					pCamera->DeleteFrontResult(NULL);
				}
                return -1;
            }			

            char* pSideImagePath = NULL;
            unsigned char* pSideImageData = NULL;
            unsigned long  iSideImageSize = 0;

            char* pTailImagePath = NULL;
            unsigned char* pTailImageData = NULL;
            unsigned long iTailImageSize = 0;

            char* pVideoPath = NULL;

            if (pTempResult->CIMG_BestCapture.dwImgSize > 0)
            {
                pSideImagePath = pTempResult->CIMG_BestCapture.chSavePath;
                pSideImageData = pTempResult->CIMG_BestCapture.pbImgData;
                iSideImageSize = pTempResult->CIMG_BestCapture.dwImgSize;
            }
			if (bGetJpeg && g_CIMG_StreamJPEG.dwImgSize > 0)
			{
				pTailImagePath = NULL;
				pTailImageData = g_CIMG_StreamJPEG.pbImgData;
				iTailImageSize = g_CIMG_StreamJPEG.dwImgSize;
			}
            else if (pTempResult->CIMG_LastCapture.dwImgSize > 0)
            {
				pTailImagePath = pTempResult->CIMG_LastCapture.chSavePath;
                pTailImageData = pTempResult->CIMG_LastCapture.pbImgData;
                iTailImageSize = pTempResult->CIMG_LastCapture.dwImgSize;
            }
            if (strlen(pTempResult->chSaveFileName) > 0)
            {
                pVideoPath = pTempResult->chSaveFileName;
            }

            BOOL bRet = FALSE;
			if (NULL != pSideImageData
				&& iSideImageSize > 0)
            {
                if (strlen(colpic) > 0)
                {
                    //rename(pSideImagePath, colpic);
                    //bRet = CopyFile(pSideImagePath, colpic, FALSE);
                    //WRITE_LOG("get colpic finish = %s, operation code = %d, getlast error = %s",
                    //    colpic,
                    //    bRet,
                    //    bRet ? "NULL" : Tool_GetLastErrorAsString().c_str());
                    bRet = Tool_SaveFileToPath(colpic, pSideImageData, iSideImageSize);
                    WRITE_LOG("get colpic success = %s, return code = %d", colpic, bRet);
                }
                else
                {
                    sprintf(colpic, "%s", pSideImagePath);
                }
            }
            else
            {
                WRITE_LOG("side image is not ready.");
            }
            
			if (NULL != pTailImageData
				&& iTailImageSize > 0)
            {
                if (strlen(platepic) > 0)
                {
                    //rename(pTailImagePath, colpic);
                    //bRet = FALSE;
                    //bRet = CopyFile(pTailImagePath, platepic, FALSE);
                    //WRITE_LOG("get platepic finish = %s, operation code = %d, getlast error = %s",
                    //    platepic,
                    //    bRet,
                    //    bRet ? "NULL" : Tool_GetLastErrorAsString().c_str());

                    bRet = Tool_SaveFileToPath(platepic, pTailImageData, iTailImageSize);
                    WRITE_LOG("get platepic success = %s, return code = %d", platepic, bRet);
                }
                else
                {
                    sprintf(platepic, "%s", pTailImagePath);
                }
            }
            else
            {
                WRITE_LOG("tail image is not ready.");
            }

            if (NULL != pVideoPath
                && strlen(pVideoPath) > 0)
            {
                if (strlen(recfile) > 0)
                {
					if (!pCamera->CheckIfFileNameIntheVideoList(pVideoPath))
					{
						WRITE_LOG("video name %s  is not complete.", pVideoPath);
					}
                    bRet = FALSE;
                    bRet = CopyFile(pVideoPath, recfile, FALSE);
                    WRITE_LOG("get recfile finish = %s, operation code = %d, getlast error = %s",
                        recfile,
                        bRet,
                        bRet ? "NULL" : Tool_GetLastErrorAsString().c_str());

					if (bRet
						&& 0 == remove(pVideoPath))
					{
						WRITE_LOG("remove file %s success.", pVideoPath);
					}
                }
                else
                {
                    sprintf(recfile, "%s", pVideoPath);
                }
            }
            else
            {
                WRITE_LOG("video is not ready.");
            }

            if (RESULT_MODE_FRONT == pCamera->GetResultMode())
            {
				WRITE_LOG("ResultMode == RESULT_MODE_FRONT, DeleteFrontResult");
                pCamera->DeleteFrontResult(NULL);
                //pCamera->TryWaitCondition();
            }
            
            iRet = 0;
        }
        else
        {
            WRITE_LOG("result is not ready");
        }
    }
    WRITE_LOG("finish, return %d", iRet);
    return iRet;
}