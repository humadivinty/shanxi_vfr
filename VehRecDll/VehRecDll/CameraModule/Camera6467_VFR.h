#pragma once
//#include "G:\GitHub\VCR_YN\VCR\VCR\Camera\BaseCamera.h"
#include "CameraModule/BaseCamera.h"
#include "ResultListManager.h"
#include "CameraModule/Camera6467_plate.h"
#include "SendStatues_def.h"
#include "utilityTool/CCusSemaohore.h"

class Camera6467_VFR :
    public BaseCamera
{
public:
    Camera6467_VFR();
    Camera6467_VFR(const char* chIP, HWND  hWnd, int Msg);
    virtual ~Camera6467_VFR();

private:
    void ReadConfig();
    void AnalysisAppendXML(CameraResult* CamResult);
    int AnalysisVelchType(const char* vehType);
    int AnalysisVelchAxleType(const char* AxleType, int  iAxleGroupCount);
    int GetAlexType(int ivalue);
    //************************************
    // Method:        CheckIfSuperLength
    // Describe:       �鿴��ǰ���Ƿ�Ϊ������������ǣ���������߼��������Ͷ���
    // FullName:      Camera6467_VFR::CheckIfSuperLength
    // Access:          private 
    // Returns:        bool
    // Returns Describe:
    // Parameter:    CameraResult * CamResult
    //************************************
    bool CheckIfSuperLength(CameraResult* CamResult);

    //************************************
    // Method:        CheckIfBackUpVehicle
    // Describe:        ���ý���Ƿ�Ϊ�������
    // FullName:      Camera6467_VFR::CheckIfBackUpVehicle
    // Access:          private 
    // Returns:        bool
    // Returns Describe:
    // Parameter:    CameraResult * CamResult
    //************************************
    bool CheckIfBackUpVehicle(CameraResult* CamResult);

public:
    //��ȡ����״̬�ص���ʱ����
    int GetTimeInterval();
    void SetDisConnectCallback(void* funcDisc, void* pUser);
    void SetReConnectCallback(void* funcReco, void* pUser);
    void SetConnectStatus_Callback(void* func, void* pUser, int TimeInterval);
    void SendConnetStateMsg(bool isConnect);

    void SetResultCallback(void* func, void* pUser);
    bool CheckIfSetResultCallback();
    void SendResultByCallback(std::shared_ptr<CameraResult> pResult);

    void SetMsgHandleAngMsg(void* handle, int msg);

    void SetCheckThreadExit(bool  bExit);
    bool GetCheckThreadExit();

    bool OpenPlateCamera(const char* ipAddress);
    std::shared_ptr<CameraResult> GetFrontResult();
    std::shared_ptr<CameraResult> GetFrontResultByPlateNo(const char* plateNo);
    std::shared_ptr<CameraResult> GetFrontResultByPosition(int position);
    std::shared_ptr<CameraResult> GetLastResult();
    bool GetLastResultIfReceiveComplete();
    void SetLastResultIfReceiveComplete(bool bValue);

    void DeleteFrontResult(const char* plateNo);
    void ClearALLResult();
    size_t GetResultListSize();
    void TryWaitCondition();

	void ReceiveVideoFileName(const char* videoFileName);
	bool CheckIfFileNameIntheVideoList(const char* fileName);
	static void ReceiveVideoFileNameCallback(void* userData, const char* fileName)
	{
		if (NULL == userData)
		{
			return;
		}
		Camera6467_VFR* pCamera = (Camera6467_VFR*)userData;
		pCamera->ReceiveVideoFileName(fileName);
	}

public:

    virtual int RecordInfoBegin(DWORD dwCarID);

    virtual int RecordInfoEnd(DWORD dwCarID);

    virtual int RecordInfoPlate(DWORD dwCarID,
        LPCSTR pcPlateNo,
        LPCSTR pcAppendInfo,
        DWORD dwRecordType,
        DWORD64 dw64TimeMS);

    virtual int RecordInfoBigImage(DWORD dwCarID,
        WORD  wImgType,
        WORD  wWidth,
        WORD  wHeight,
        PBYTE pbPicData,
        DWORD dwImgDataLen,
        DWORD dwRecordType,
        DWORD64 dw64TimeMS);

    virtual int RecordInfoSmallImage(DWORD dwCarID,
        WORD wWidth,
        WORD wHeight,
        PBYTE pbPicData,
        DWORD dwImgDataLen,
        DWORD dwRecordType,
        DWORD64 dw64TimeMS);

    virtual int RecordInfoBinaryImage(DWORD dwCarID,
        WORD wWidth,
        WORD wHeight,
        PBYTE pbPicData,
        DWORD dwImgDataLen,
        DWORD dwRecordType,
        DWORD64 dw64TimeMS);

    virtual int DeviceJPEGStream(
        PBYTE pbImageData,
        DWORD dwImageDataLen,
        DWORD dwImageType,
        LPCSTR szImageExtInfo);

    virtual void CheckStatus();

public:
    bool checkIfHasThreePic(std::shared_ptr<CameraResult> result);
    DWORD getResultWaitTime();
    unsigned int SendResultThreadFunc();
    static unsigned int WINAPI s_SendResultThreadFunc(void* parameter)
    {
        Camera6467_VFR* pThis = (Camera6467_VFR*)parameter;
        if (pThis)
        {
//#ifdef USE_LAST_RESULT
//            return pThis->SendResultThreadFunc_lastResult();
//#else
//            return pThis->SendResultThreadFunc();
//#endif
            return pThis->SendResultThreadFunc_WithNoSignal();
        }
        return 0;
    }
    void copyStringToBuffer(char* bufer,size_t bufLen, const char * srcStr);

    unsigned int SendResultThreadFunc_lastResult();

    unsigned int SendResultThreadFunc_WithNoSignal();

    /************************************************************************/
    /* ѭ��ɾ����־�߳�                                                                     */
    /************************************************************************/
    static unsigned int WINAPI s_DeleteLogThreadFunc(void* parameter)
    {
        Camera6467_VFR* pThis = (Camera6467_VFR*)parameter;
        if (pThis)
        {
            return pThis->DeleteLogThreadFunc();
        }
        return 0;
    }
    unsigned int DeleteLogThreadFunc();

    
    /************************************************************************/
    /* ѭ��ɾ�������߳�                                                                     */
    /************************************************************************/
    static unsigned int WINAPI s_DeleteResultThreadFunc(void* parameter)
    {
        Camera6467_VFR* pThis = (Camera6467_VFR*)parameter;
        if (pThis)
        {
            return pThis->DeleteResultThreadFunc();
        }
        return 0;
    }
    unsigned int DeleteResultThreadFunc();

    void SetResultSendSignal(int iSignalID, int iSendStatus);
    SendStatues getFrontSendSignal();
    void deleteFrontSendSignal();
    int GetSignalListSize();

    int GetResultMode();
private:
    DWORD m_dwLastCarID;
    int m_iTimeInvl;
    int m_iSuperLenth;
    int m_iResultTimeOut;
    int m_iResultMsg;
    int m_iWaitVfrTimeOut;
    int m_iResultModule;

    void* g_pUser;
    void* g_func_ReconnectCallback;
    void* g_ConnectStatusCallback;
    void* g_func_DisconnectCallback;

    void* g_pFuncResultCallback;
    void* g_pResultUserData;

    void* m_hMsgHanldle;
    
    bool m_bStatusCheckThreadExit;
    bool m_bLastResultComplete;

    HANDLE m_hStatusCheckThread;			//״̬����߳�
    HANDLE m_hSendResultThread;			//��������߳�
    HANDLE m_hDeleteLogThread;              //��־����߳�
    HANDLE m_hDeleteResultThread;              //�������߳�

    CameraIMG m_Small_IMG_Temp;

    std::shared_ptr<Camera6467_plate> m_Camera_Plate;

    CameraResult* m_pResult;
    ResultListManager m_VfrResultList;
    std::list<SendStatues> StatusList;
    std::list<std::string> m_lsVideoName;
	std::list<std::string >m_lsCompleteVideoName;

    std::shared_ptr<CameraResult> m_pLastResult;
    //CCusSemaphore m_MySemaphore;
};

