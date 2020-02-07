#ifndef MYH264SAVER_H
#define MYH264SAVER_H

#include "CusH264Struct.h"
#include "CameraModule/ThreadSafeList.h"
#include "libAVI/cAviLib.h"
#include "MyH264Vector.h"
#include <memory>
#include <list>

#define VIDEO_FRAME_LIST_SIZE 1500

#define SAVING_FLAG_NOT_SAVE 0
#define SAVING_FLAG_SAVING 1
#define SAVING_FLAG_SHUT_DOWN 2

#define TIME_FLAG_UNDEFINE -1

class MyH264Saver
{
public:
    MyH264Saver();
    ~MyH264Saver();

    bool addDataStruct(CustH264Struct* pDataStruct);
    bool StartSaveH264(INT64  beginTimeStamp, const char* pchFilePath);
    bool StopSaveH264(INT64 TimeFlag = 0);

    static DWORD WINAPI  H264DataProceesor( LPVOID lpThreadParameter);
    DWORD processH264Data();
    DWORD processH264Data_mp4();

    void SetLogEnable(bool bEnable);
    bool GetLogEnable();
private:
    void SetIfExit(bool bValue);
    bool GetIfExit();

    void SetSaveFlag(int iValue);
    int GetSaveFlag();

    void SetStartTimeFlag(INT64 iValue);
    INT64 GetStartTimeFlag();

    void SetStopTimeFlag(INT64 iValue);
    INT64 GetStopTimeFlag();

    void SetIfFirstSave(bool bValue);
    bool GetIfFirstSave();

    void SetSavePath(const char* filePath, size_t bufLength);
    const char* GetSavePath();

    void WriteFormatLog(const char *szfmt, ...);

private:

    bool m_bExit;    
    bool m_bFirstSave;
    bool m_bWriteLog;
    int m_iSaveH264Flag;        //   0--not save  ; 1--saving ; 2--shut down saving
    INT64 m_iTimeFlag;
    INT64 M_iStopTimeFlag;

    char m_chFilePath[256];
    char m_chCurrentPath[256];

	INT64 m_iTmpTime;
	int m_lastvideoidx;

    //TemplateThreadSafeList<std::shared_ptr<CustH264Struct > > m_lDataStructList;
	//std::deque<std::shared_ptr<CustH264Struct > > m_lDataStructList;
    MyH264DataVector m_lDataStructList;

	CRITICAL_SECTION m_DataListLocker;
     
    CRITICAL_SECTION m_Locker;
    HANDLE m_hThreadSaveH264;
    CAviLib m_264AviLib;
    HANDLE m_hVideoSaver;
};

#endif // MYH264SAVER_H
