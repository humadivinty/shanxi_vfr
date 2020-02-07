#include "stdafx.h"
#include "MyH264Saver.h"
#include "utilityTool/ToolFunction.h"
//#include <QDebug>

#include "libVideoSaver/libvideosaver.h"
#pragma comment(lib, "libVideoSaver/libVideoSaver.lib")

#define  WINDOWS


MyH264Saver::MyH264Saver() :
m_bExit(false),
m_iSaveH264Flag(0),
m_iTimeFlag(0),
M_iStopTimeFlag(TIME_FLAG_UNDEFINE),
m_iTmpTime(0),
m_lastvideoidx(-1),
m_bFirstSave(false),
m_bWriteLog(false),
//m_pLocker(NULL),
m_hThreadSaveH264(NULL),
m_lDataStructList(VIDEO_FRAME_LIST_SIZE),
m_hVideoSaver(NULL)
{
    m_hVideoSaver = Video_CreateProcessHandle(VIDEOTYPE_MP4);

    memset(m_chCurrentPath, '\0', sizeof(m_chCurrentPath));
    sprintf_s(m_chCurrentPath, sizeof(m_chCurrentPath), "%s", Tool_GetCurrentPath());

    InitializeCriticalSection(&m_Locker);
    InitializeCriticalSection(&m_DataListLocker);

    m_hThreadSaveH264 = CreateThread(NULL, 0, H264DataProceesor, this, 0, NULL);
}


MyH264Saver::~MyH264Saver()
{
    SetIfExit(true);
    SetSaveFlag(false);
    Tool_SafeCloseThread(m_hThreadSaveH264);
    if (m_hVideoSaver != NULL)
    {
        Video_CloseProcessHandle(m_hVideoSaver);
    }

    DeleteCriticalSection(&m_Locker);
    DeleteCriticalSection(&m_DataListLocker);
}

bool MyH264Saver::addDataStruct(CustH264Struct* pDataStruct)
{
    if (pDataStruct == NULL)
    {
        return false;
    }
    //char buf[256] = { 0 };
    //sprintf_s(buf, "%ld\n", pDataStruct->m_llFrameTime);
    //OutputDebugString(buf);

    std::shared_ptr<CustH264Struct> pData = std::shared_ptr<CustH264Struct>(pDataStruct);
    //EnterCriticalSection(&m_DataListLocker);
    //   if (m_lDataStructList.size() > VIDEO_FRAME_LIST_SIZE)
    //   {
    //       auto tempData = m_lDataStructList.front();
    //       m_lDataStructList.pop_front();
    //       WriteFormatLog("addDataStruct:: size > %d, erase front, frame time = %I64d", VIDEO_FRAME_LIST_SIZE, tempData->m_llFrameTime);
    //   }
    //   m_lDataStructList.push_back(pData);
    //LeaveCriticalSection(&m_DataListLocker);
    m_lDataStructList.AddOneData(pData);

    if (GetLogEnable())
    {
        SYSTEMTIME  stimeNow = Tool_GetCurrentTime();

        char chBuffer1[256] = { 0 };
        //_getcwd(chBuffer1, sizeof(chBuffer1));

        sprintf_s(chBuffer1, sizeof(chBuffer1), "%s\\XLWLog\\%04d-%02d-%02d\\",
            m_chCurrentPath,
            stimeNow.wYear,
            stimeNow.wMonth,
            stimeNow.wDay);
        Tool_MakeDir(chBuffer1);

        char chLogFileName[512] = { 0 };
        sprintf_s(chLogFileName, sizeof(chLogFileName), "%s\\videoFrame_%d.log", chBuffer1,stimeNow.wHour);

        char chTime[64] = {0};
        CTime tm(pData->m_llFrameTime / 1000);
        sprintf_s(chTime, sizeof(chTime), "%04d%02d%02d%02d%02d%02d%03d",
            tm.GetYear(),
            tm.GetMonth(),
            tm.GetDay(),
            tm.GetHour(),
            tm.GetMinute(),
            tm.GetSecond(),
            pData->m_llFrameTime % 1000);

        FILE *file = NULL;
        file = fopen(chLogFileName, "a+");
        if (file)
        {
            fprintf(file, "[%04d-%02d-%02d %02d:%02d:%02d:%03d] :frame index = %d m_llFrameTime = %I64d, time string = %s\n",
                stimeNow.wYear,
                stimeNow.wMonth,
                stimeNow.wDay,
                stimeNow.wHour,
                stimeNow.wMinute,
                stimeNow.wSecond,
                stimeNow.wMilliseconds,
                pData->index,
                pData->m_llFrameTime,
                chTime);

            fclose(file);
            file = NULL;
        }
    }

    return true;
}

bool MyH264Saver::StartSaveH264(INT64 beginTimeStamp, const char* pchFilePath)
{
    //qDebug()<< "StartSaveH264 :"<< "beginTimeStamp "<< beginTimeStamp;
    WriteFormatLog("MyH264Saver::StartSaveH264 beginTimeStamp = %I64d, pchFilePath = %s", beginTimeStamp, pchFilePath);
    SetSavePath(pchFilePath, strlen(pchFilePath));
    SetStartTimeFlag(beginTimeStamp);
    SetIfFirstSave(true);
    SetStopTimeFlag(TIME_FLAG_UNDEFINE);
    SetSaveFlag(SAVING_FLAG_SAVING);

    m_lastvideoidx = -1;
    return true;
}

bool MyH264Saver::StopSaveH264(INT64 TimeFlag)
{
    //SetSaveFlag(SAVING_FLAG_SHUT_DOWN);
    SetStopTimeFlag(TimeFlag);
    return true;
}

DWORD WINAPI MyH264Saver::H264DataProceesor(LPVOID lpThreadParameter)
{
    if (NULL == lpThreadParameter)
    {
        return 0;
    }
    MyH264Saver* pSaver = (MyH264Saver*)lpThreadParameter;
    //return pSaver->processH264Data();
    return pSaver->processH264Data_mp4();
}

DWORD MyH264Saver::processH264Data()
{
    WriteFormatLog("MyH264Saver::processH264Data begin.\n");
    int  iSaveFlag = 0;
    int iVideoWidth = -1;
    int iVideoHeight = -1;

    int iFlag = 1;

    long long iCurrentFrameTimeFlag = -1;
    long long iLastFrameTimeFlag = -1;
    long long iVideoStopTimeFlag = -1;

    long long iLastSaveFlag = -1;
    long long iVideoBeginTimeFlag = -1;
    long long iTimeNowFlag = -1;

    int iLastFrameIndex = -1;
    int iCurrentFrameIndex = -1;

    int iDataListIndex = -1;
    int iLastNullIndex = 0;
    bool bFindEmptyData = false;
    while (!GetIfExit())
    {

        char buf[256] = { 0 };
        //iVideoStopTimeFlag = GetStopTimeFlag();
        iVideoBeginTimeFlag = GetStartTimeFlag();
        iTimeNowFlag = GetTickCount();

        iSaveFlag = GetSaveFlag();


        if (iSaveFlag != iLastSaveFlag)
        {
            WriteFormatLog("stop time flag change, lastStopTimeFlag = %I64d, current Stop time flag = %I64d, system time Now = %I64d,  video beginTIme = %I64d, save flag = %d\n",
                iLastSaveFlag,
                iVideoStopTimeFlag,
                iTimeNowFlag,
                iVideoBeginTimeFlag,
                iSaveFlag);
            iLastSaveFlag = iSaveFlag;
        }

        std::shared_ptr<CustH264Struct > pData = nullptr;
        switch (iSaveFlag)
        {
        case SAVING_FLAG_NOT_SAVE:
            //usleep(10 * 1000);
            //printf("MyH264Saver::processH264Data SAVING_FLAG_NOT_SAVE, break \n");
            Sleep(10);
            break;
        case SAVING_FLAG_SAVING:
            //printf("MyH264Saver::processH264Data SAVING_FLAG_SAVING \n",GetSavePath() );

            if (-1 == iDataListIndex)
            {
                //查找所需的index
                if (bFindEmptyData)
                {
                    iLastNullIndex = iLastNullIndex == VIDEO_FRAME_LIST_SIZE - 1 ? 0 : iLastNullIndex;

                    WriteFormatLog("this is new search , iDataListIndex = %d  search index.\n", iDataListIndex);
                }
                else
                {
                    iLastNullIndex = m_lDataStructList.GetOldestDataIndex();
                    WriteFormatLog("iDataListIndex = %d  search index.\n", iDataListIndex);
                }

                for (int i = 0; i < VIDEO_FRAME_LIST_SIZE; /*i++*/)
                {
                    int iSearchIndex = (i + iLastNullIndex) % VIDEO_FRAME_LIST_SIZE;
                    pData = m_lDataStructList.GetOneDataByIndex(iSearchIndex);
                    if (pData == nullptr)
                    {
                        iLastNullIndex = iSearchIndex;
                        bFindEmptyData = true;
                        WriteFormatLog("NewSearch index = %d, pData == nullptr , finish search.\n", iSearchIndex);
                        break;
                    }
                    iCurrentFrameTimeFlag = pData->m_llFrameTime;
                    iCurrentFrameIndex = pData->index;
                    iVideoBeginTimeFlag = GetStartTimeFlag();
                    if (iCurrentFrameTimeFlag >= iVideoBeginTimeFlag)
                    {
                        iDataListIndex = iSearchIndex;
                        WriteFormatLog("NewSearch index = %d, iCurrentFrameIndex = %d, iCurrentFrameTimeFlag %I64d >= iVideoBeginTimeFlag %I64d, set iDataListIndex = %d, break .\n",
                            iSearchIndex,
                            iCurrentFrameIndex,
                            iCurrentFrameTimeFlag,
                            iVideoBeginTimeFlag,
                            iDataListIndex);
                        iLastFrameIndex = iCurrentFrameIndex;
                        iLastFrameTimeFlag = iCurrentFrameTimeFlag;
                        break;
                    }
                    if (iVideoBeginTimeFlag - iCurrentFrameTimeFlag > 1000)
                    {
                        i = i + 25;
                        WriteFormatLog("NewSearch iVideoBeginTimeFlag (%I64d)- iCurrentFrameTimeFlag(%I64d) > 1000 ms, next search index +25, continue .\n", iVideoBeginTimeFlag, iCurrentFrameTimeFlag);
                    }
                    else
                    {
                        i++;
                    }
                    WriteFormatLog("NewSearch search index = %d, iCurrentFrameTimeFlag %I64d < iVideoBeginTimeFlag %I64d, continue .\n", iSearchIndex, iCurrentFrameTimeFlag, iVideoBeginTimeFlag);
                    if ((VIDEO_FRAME_LIST_SIZE - 1) == i)
                    {
                        bFindEmptyData = false;
                    }
                }
            }
            else
            {
                iDataListIndex = (iDataListIndex >= (VIDEO_FRAME_LIST_SIZE - 1)) ? 0 : (iDataListIndex + 1);
                pData = m_lDataStructList.GetOneDataByIndex(iDataListIndex);
                if (pData == nullptr)
                {
                    WriteFormatLog("search iDataListIndex = %d, but pData == nullptr, continue.\n", iDataListIndex);
                    break;
                }

                iCurrentFrameTimeFlag = pData->m_llFrameTime;
                iCurrentFrameIndex = pData->index;
                iVideoBeginTimeFlag = GetStartTimeFlag();
                iVideoStopTimeFlag = GetStopTimeFlag();

                if (TIME_FLAG_UNDEFINE != iVideoStopTimeFlag
                    && iCurrentFrameTimeFlag >= iVideoStopTimeFlag)
                {
                    WriteFormatLog("search index = %d, iCurrentFrameIndex = %d, iLastFrameIndex = %d,  iCurrentFrameTimeFlag %I64d >  iVideoStopTimeFlag %I64d, SetSaveFlag SAVING_FLAG_SHUT_DOWN .\n",
                        iDataListIndex,
                        iCurrentFrameIndex,
                        iLastFrameIndex,
                        iCurrentFrameTimeFlag,
                        iVideoStopTimeFlag);

                    SetSaveFlag(SAVING_FLAG_SHUT_DOWN);
                    iDataListIndex = -1;
                    iLastNullIndex = 0;
                    iLastFrameIndex = -1;
                }
                else
                {
                    if (iCurrentFrameTimeFlag >= iVideoBeginTimeFlag)
                    {
                        if (iLastFrameIndex == -1
                            || TIME_FLAG_UNDEFINE == iVideoStopTimeFlag
                            || (iCurrentFrameIndex == (iLastFrameIndex + 1) && iCurrentFrameTimeFlag > iLastFrameTimeFlag )
                            || ((iCurrentFrameTimeFlag > iLastFrameTimeFlag) && (iCurrentFrameIndex == 0) && iLastFrameIndex == VIDEO_FRAME_LIST_SIZE)
                            )
                        {
                            iLastFrameTimeFlag = iCurrentFrameTimeFlag;
                            iLastFrameIndex = iCurrentFrameIndex;

                            WriteFormatLog("search index = %d, iCurrentFrameIndex = %d, iCurrentFrameTimeFlag %I64d >  iVideoBeginTimeFlag %I64d and < iVideoStopTimeFlag (%I64d) ,  Save frame .\n",
                                iDataListIndex,
                                iCurrentFrameIndex,
                                iCurrentFrameTimeFlag,
                                iVideoBeginTimeFlag,
                                iVideoStopTimeFlag);
                        }
                        else
                        {
                            WriteFormatLog("search index = %d, iCurrentFrameTimeFlag (%I64d) > iVideoBeginTimeFlag (%I64d) ,and < iVideoStopTimeFlag (%I64d) but nosave frame, iCurrentFrameIndex = %d, iLastFrameTimeFlag = %I64d, iLastFrameIndex = %d, ",
                                iDataListIndex,
                                iCurrentFrameTimeFlag,
                                iVideoBeginTimeFlag,
                                iVideoStopTimeFlag,
                                iCurrentFrameIndex,
                                iLastFrameTimeFlag,
                                iLastFrameIndex
                                );
                            pData = nullptr;
                        }
                    }
                    else
                    {
                        WriteFormatLog("search index = %d, iCurrentFrameIndex = %d, iCurrentFrameTimeFlag %I64d <  iVideoBeginTimeFlag %I64d , no Save frame .\n",
                            iDataListIndex,
                            iCurrentFrameIndex,
                            iCurrentFrameTimeFlag,
                            iVideoBeginTimeFlag);
                        pData = nullptr;

                        if (iVideoBeginTimeFlag - iCurrentFrameTimeFlag > 1000)
                        {
                            iDataListIndex = ((iDataListIndex + 24) >= (VIDEO_FRAME_LIST_SIZE - 1)) ? 0 : (iDataListIndex + 24);
                            WriteFormatLog("search2, iVideoBeginTimeFlag (%I64d)- iCurrentFrameTimeFlag(%I64d) > 1000 ms, next search index +25, continue .\n", iVideoBeginTimeFlag, iCurrentFrameTimeFlag);
                        }
                    }
                }
            }

            if (pData == nullptr)
            {
                WriteFormatLog("MyH264Saver::processH264Data SAVING_FLAG_SAVING  pData == nullptr\n" );
                break;
            }

            iVideoWidth = pData->m_iWidth;
            iVideoHeight = pData->m_iHeight;
            if (GetIfFirstSave())
            {
                if (!m_264AviLib.IsNULL())
                {
                    iLastFrameIndex = -1;
                    WriteFormatLog("m_264AviLib.IsNULL == false, close file.\n");
                    m_264AviLib.close();
                }
                if (iVideoWidth > 0
                    && iVideoHeight > 0)
                {
                    WriteFormatLog("m_264AviLib.setAviInfo , %s.\n", GetSavePath());
                    m_264AviLib.setAviInfo((char*)GetSavePath(), iVideoWidth, iVideoHeight, 25, "H264");
                    SetIfFirstSave(false);
                }
            }

            if (!m_264AviLib.IsNULL()
                && pData->m_llFrameTime >= GetStartTimeFlag()
                )
            {
                iFlag = m_264AviLib.writeFrame((char*)pData->m_pbH264FrameData, pData->m_iDataSize, pData->m_isIFrame);
                if (iFlag != 0)
                {
                    WriteFormatLog(" m_264AviLib.writeFrame = %d\n", iFlag);
                }
                //SetIfFirstSave(false);
            }
            else
            {
                if (m_264AviLib.IsNULL())
                {
                    WriteFormatLog(" SAVING_FLAG_SAVING but m_264AviLib.IsNULL\n");
                }
                if (pData->m_llFrameTime < GetStartTimeFlag())
                {
                    WriteFormatLog(" SAVING_FLAG_SAVING but pData->m_llFrameTime (%I64d)< GetStartTimeFlag(%I64d)\n", pData->m_llFrameTime , GetStartTimeFlag());
                }
                WriteFormatLog(" SAVING_FLAG_SAVING but some thing wrong.\n", iFlag);
            }
            pData = nullptr;
            break;
        case SAVING_FLAG_SHUT_DOWN:
            if (!m_264AviLib.IsNULL())
            {
                m_iTmpTime = 0;
                m_264AviLib.close();
                //SetSaveFlag(SAVING_FLAG_NOT_SAVE);
                WriteFormatLog("m_264AviLib.close SAVING_FLAG_SHUT_DOWN.\n", GetSavePath());
            }
            else
            {
                WriteFormatLog("MyH264Saver::processH264Data SAVING_FLAG_SHUT_DOWN m_264AviLib.IsNULL.\n", GetSavePath());
            }
            SetSaveFlag(SAVING_FLAG_NOT_SAVE);
            break;
        default:
            WriteFormatLog("MyH264Saver::processH264Data default break.\n", GetSavePath());
            break;
        }

    }

    WriteFormatLog("MyH264Saver::processH264Data finish.\n");
    return 0;
}

DWORD MyH264Saver::processH264Data_mp4()
{
    WriteFormatLog("MyH264Saver::processH264Data begin.\n");
    int  iSaveFlag = 0;
    int iVideoWidth = -1;
    int iVideoHeight = -1;

    int iFlag = 1;

    long long iCurrentFrameTimeFlag = -1;
    long long iLastFrameTimeFlag = -1;
    long long iVideoStopTimeFlag = -1;

    long long iLastSaveFlag = -1;
    long long iVideoBeginTimeFlag = -1;
    long long iTimeNowFlag = -1;

    int iLastFrameIndex = -1;
    int iCurrentFrameIndex = -1;

    int iDataListIndex = -1;
    int iLastNullIndex = 0;
    bool bFindEmptyData = false;
    while (!GetIfExit())
    {

        char buf[256] = { 0 };
        //iVideoStopTimeFlag = GetStopTimeFlag();
        iVideoBeginTimeFlag = GetStartTimeFlag();
        iTimeNowFlag = GetTickCount();

        iSaveFlag = GetSaveFlag();


        if (iSaveFlag != iLastSaveFlag)
        {
            WriteFormatLog("stop time flag change, lastStopTimeFlag = %I64d, current Stop time flag = %I64d, system time Now = %I64d,  video beginTIme = %I64d, save flag = %d\n",
                iLastSaveFlag,
                iVideoStopTimeFlag,
                iTimeNowFlag,
                iVideoBeginTimeFlag,
                iSaveFlag);
            iLastSaveFlag = iSaveFlag;
        }

        std::shared_ptr<CustH264Struct > pData = nullptr;
        switch (iSaveFlag)
        {
        case SAVING_FLAG_NOT_SAVE:
            //usleep(10 * 1000);
            //printf("MyH264Saver::processH264Data SAVING_FLAG_NOT_SAVE, break \n");
            Sleep(10);
            break;
        case SAVING_FLAG_SAVING:
            //printf("MyH264Saver::processH264Data SAVING_FLAG_SAVING \n",GetSavePath() );

            if (-1 == iDataListIndex)
            {
                //查找所需的index
                if (bFindEmptyData)
                {
                    iLastNullIndex = iLastNullIndex == VIDEO_FRAME_LIST_SIZE - 1 ? 0 : iLastNullIndex;

                    WriteFormatLog("this is new search , iDataListIndex = %d  search index.\n", iDataListIndex);
                }
                else
                {
                    iLastNullIndex = m_lDataStructList.GetOldestDataIndex();
                    WriteFormatLog("iDataListIndex = %d  search index.\n", iDataListIndex);
                }

                for (int i = 0; i < VIDEO_FRAME_LIST_SIZE; /*i++*/)
                {
                    int iSearchIndex = (i + iLastNullIndex) % VIDEO_FRAME_LIST_SIZE;
                    pData = m_lDataStructList.GetOneDataByIndex(iSearchIndex);
                    if (pData == nullptr)
                    {
                        iLastNullIndex = iSearchIndex;
                        bFindEmptyData = true;
                        WriteFormatLog("NewSearch index = %d, pData == nullptr , finish search.\n", iSearchIndex);
                        break;
                    }
                    iCurrentFrameTimeFlag = pData->m_llFrameTime;
                    iCurrentFrameIndex = pData->index;
                    iVideoBeginTimeFlag = GetStartTimeFlag();
                    if (iCurrentFrameTimeFlag >= iVideoBeginTimeFlag)
                    {
                        iDataListIndex = iSearchIndex;
                        WriteFormatLog("NewSearch index = %d, iCurrentFrameIndex = %d, iCurrentFrameTimeFlag %I64d >= iVideoBeginTimeFlag %I64d, set iDataListIndex = %d, break .\n",
                            iSearchIndex,
                            iCurrentFrameIndex,
                            iCurrentFrameTimeFlag,
                            iVideoBeginTimeFlag,
                            iDataListIndex);
                        iLastFrameIndex = iCurrentFrameIndex;
                        iLastFrameTimeFlag = iCurrentFrameTimeFlag;
                        break;
                    }
                    if (iVideoBeginTimeFlag - iCurrentFrameTimeFlag > 1000)
                    {
                        i = i + 25;
                        WriteFormatLog("NewSearch iVideoBeginTimeFlag (%I64d)- iCurrentFrameTimeFlag(%I64d) > 1000 ms, next search index +25, continue .\n", iVideoBeginTimeFlag, iCurrentFrameTimeFlag);
                    }
                    else
                    {
                        i++;
                    }
                    WriteFormatLog("NewSearch search index = %d, iCurrentFrameTimeFlag %I64d < iVideoBeginTimeFlag %I64d, continue .\n", iSearchIndex, iCurrentFrameTimeFlag, iVideoBeginTimeFlag);
                    if ((VIDEO_FRAME_LIST_SIZE - 1) == i)
                    {
                        bFindEmptyData = false;
                    }
                }
            }
            else
            {
                iDataListIndex = (iDataListIndex >= (VIDEO_FRAME_LIST_SIZE - 1)) ? 0 : (iDataListIndex + 1);
                pData = m_lDataStructList.GetOneDataByIndex(iDataListIndex);
                if (pData == nullptr)
                {
                    WriteFormatLog("search iDataListIndex = %d, but pData == nullptr, continue.\n", iDataListIndex);
                    break;
                }

                iCurrentFrameTimeFlag = pData->m_llFrameTime;
                iCurrentFrameIndex = pData->index;
                iVideoBeginTimeFlag = GetStartTimeFlag();
                iVideoStopTimeFlag = GetStopTimeFlag();

                if (TIME_FLAG_UNDEFINE != iVideoStopTimeFlag
                    && iCurrentFrameTimeFlag >= iVideoStopTimeFlag)
                {
                    WriteFormatLog("search index = %d, iCurrentFrameIndex = %d, iLastFrameIndex = %d,  iCurrentFrameTimeFlag %I64d >  iVideoStopTimeFlag %I64d, SetSaveFlag SAVING_FLAG_SHUT_DOWN .\n",
                        iDataListIndex,
                        iCurrentFrameIndex,
                        iLastFrameIndex,
                        iCurrentFrameTimeFlag,
                        iVideoStopTimeFlag);

                    SetSaveFlag(SAVING_FLAG_SHUT_DOWN);
                    iDataListIndex = -1;
                    iLastNullIndex = 0;
                    iLastFrameIndex = -1;
                }
                else
                {
                    if (iCurrentFrameTimeFlag >= iVideoBeginTimeFlag)
                    {
                        if (iLastFrameIndex == -1
                            || TIME_FLAG_UNDEFINE == iVideoStopTimeFlag
                            || (iCurrentFrameIndex == (iLastFrameIndex + 1) && iCurrentFrameTimeFlag > iLastFrameTimeFlag)
                            || ((iCurrentFrameTimeFlag > iLastFrameTimeFlag) && (iCurrentFrameIndex == 0) && iLastFrameIndex == VIDEO_FRAME_LIST_SIZE)
                            )
                        {
                            iLastFrameTimeFlag = iCurrentFrameTimeFlag;
                            iLastFrameIndex = iCurrentFrameIndex;

                            WriteFormatLog("search index = %d, iCurrentFrameIndex = %d, iCurrentFrameTimeFlag %I64d >  iVideoBeginTimeFlag %I64d and < iVideoStopTimeFlag (%I64d) ,  Save frame .\n",
                                iDataListIndex,
                                iCurrentFrameIndex,
                                iCurrentFrameTimeFlag,
                                iVideoBeginTimeFlag,
                                iVideoStopTimeFlag);
                        }
                        else
                        {
                            WriteFormatLog("search index = %d, iCurrentFrameTimeFlag (%I64d) > iVideoBeginTimeFlag (%I64d) ,and < iVideoStopTimeFlag (%I64d) but nosave frame, iCurrentFrameIndex = %d, iLastFrameTimeFlag = %I64d, iLastFrameIndex = %d, ",
                                iDataListIndex,
                                iCurrentFrameTimeFlag,
                                iVideoBeginTimeFlag,
                                iVideoStopTimeFlag,
                                iCurrentFrameIndex,
                                iLastFrameTimeFlag,
                                iLastFrameIndex
                                );
                            pData = nullptr;
                        }
                    }
                    else
                    {
                        WriteFormatLog("search index = %d, iCurrentFrameIndex = %d, iCurrentFrameTimeFlag %I64d <  iVideoBeginTimeFlag %I64d , no Save frame .\n",
                            iDataListIndex,
                            iCurrentFrameIndex,
                            iCurrentFrameTimeFlag,
                            iVideoBeginTimeFlag);
                        pData = nullptr;

                        if (iVideoBeginTimeFlag - iCurrentFrameTimeFlag > 1000)
                        {
                            iDataListIndex = ((iDataListIndex + 24) >= (VIDEO_FRAME_LIST_SIZE - 1)) ? 0 : (iDataListIndex + 24);
                            WriteFormatLog("search2, iVideoBeginTimeFlag (%I64d)- iCurrentFrameTimeFlag(%I64d) > 1000 ms, next search index +25, continue .\n", iVideoBeginTimeFlag, iCurrentFrameTimeFlag);
                        }
                    }
                }
            }

            if (pData == nullptr)
            {
                WriteFormatLog("MyH264Saver::processH264Data SAVING_FLAG_SAVING  pData == nullptr\n");
                break;
            }

            iVideoWidth = pData->m_iWidth;
            iVideoHeight = pData->m_iHeight;
            if (GetIfFirstSave())
            {
                WriteFormatLog("GetIfFirstSave = true, Video_CreateVideoFile = %s \n", GetSavePath());
                iFlag = Video_CreateVideoFile(m_hVideoSaver, GetSavePath(), iVideoWidth, iVideoHeight, 25);
                if (0 != iFlag)
                {
                    WriteFormatLog("Video_CreateVideoFile failed , return code = %d\n", iFlag);
                }
                else
                {
                    WriteFormatLog("Video_CreateVideoFile success \n");
                }
            }

            if (NULL != m_hVideoSaver
                && pData->m_llFrameTime >= GetStartTimeFlag()
                )
            {
                iFlag = Video_WriteH264Frame(m_hVideoSaver, FRAMETYPE_MP4_VIDEO, pData->m_pbH264FrameData, pData->m_iDataSize);
                //iFlag = m_264AviLib.writeFrame((char*)pData->m_pbH264FrameData, pData->m_iDataSize, pData->m_isIFrame);
                if (iFlag != 0)
                {
                    WriteFormatLog(" Video_WriteH264Frame = %d\n", iFlag);
                }
                SetIfFirstSave(false);
            }
            else
            {
                //if (m_264AviLib.IsNULL())
                if (NULL == m_hVideoSaver)
                {
                    WriteFormatLog(" SAVING_FLAG_SAVING but m_hVideoSaver IsNULL\n");
                }
                if (pData->m_llFrameTime < GetStartTimeFlag())
                {
                    WriteFormatLog(" SAVING_FLAG_SAVING but pData->m_llFrameTime (%I64d)< GetStartTimeFlag(%I64d)\n", pData->m_llFrameTime, GetStartTimeFlag());
                }
                WriteFormatLog(" SAVING_FLAG_SAVING but some thing wrong.\n", iFlag);
            }
            pData = nullptr;
            break;
        case SAVING_FLAG_SHUT_DOWN:
            //if (!m_264AviLib.IsNULL())
            if (NULL != m_hVideoSaver)
            {
                m_iTmpTime = 0;
                //m_264AviLib.close();
                //SetSaveFlag(SAVING_FLAG_NOT_SAVE);
                iFlag = Video_CloseVideoFile(m_hVideoSaver);
                WriteFormatLog("Video_CloseVideoFile SAVING_FLAG_SHUT_DOWN. iFlag = %d\n", iFlag);
            }
            else
            {
                WriteFormatLog("MyH264Saver::processH264Data SAVING_FLAG_SHUT_DOWN m_hVideoSaver Is NULL.\n");
            }
            SetSaveFlag(SAVING_FLAG_NOT_SAVE);
            break;
        default:
            WriteFormatLog("MyH264Saver::processH264Data default break.\n");
            break;
        }

    }

    WriteFormatLog("MyH264Saver::processH264Data finish.\n");
    return 0;
}

void MyH264Saver::SetLogEnable(bool bValue)
{
    EnterCriticalSection(&m_Locker);
    m_bWriteLog = bValue;
    LeaveCriticalSection(&m_Locker);
}

bool MyH264Saver::GetLogEnable()
{
    bool bValue = false;
    EnterCriticalSection(&m_Locker);
    bValue = m_bWriteLog;
    LeaveCriticalSection(&m_Locker);
    return bValue;
}

void MyH264Saver::SetIfExit(bool bValue)
{
    EnterCriticalSection(&m_Locker);
    m_bExit = bValue;
    LeaveCriticalSection(&m_Locker);
}

bool MyH264Saver::GetIfExit()
{
    bool bValue = false;
    EnterCriticalSection(&m_Locker);
    bValue = m_bExit;
    LeaveCriticalSection(&m_Locker);
    return bValue;
}

void MyH264Saver::SetSaveFlag(int iValue)
{
    WriteFormatLog("MyH264Saver::SetSaveFlag %d", iValue);
    EnterCriticalSection(&m_Locker);
    m_iSaveH264Flag = iValue;
    LeaveCriticalSection(&m_Locker);
}

int MyH264Saver::GetSaveFlag()
{
    int iValue = false;
    EnterCriticalSection(&m_Locker);
    iValue = m_iSaveH264Flag;
    LeaveCriticalSection(&m_Locker);
    return iValue;
}

void MyH264Saver::SetStartTimeFlag(INT64 iValue)
{
    WriteFormatLog("MyH264Saver::SetTimeFlag %I64d", iValue);
    EnterCriticalSection(&m_Locker);
    m_iTimeFlag = iValue;
    m_iTmpTime = iValue;
    LeaveCriticalSection(&m_Locker);
}

INT64 MyH264Saver::GetStartTimeFlag()
{
    INT64 iValue = false;
    EnterCriticalSection(&m_Locker);
    iValue = m_iTimeFlag;
    LeaveCriticalSection(&m_Locker);
    return iValue;
}

void MyH264Saver::SetStopTimeFlag(INT64 iValue)
{
    WriteFormatLog("MyH264Saver::SetStopTimeFlag %I64d", iValue);
    EnterCriticalSection(&m_Locker);
    M_iStopTimeFlag = iValue;
    LeaveCriticalSection(&m_Locker);
}

INT64 MyH264Saver::GetStopTimeFlag()
{
    INT64 iValue = false;
    EnterCriticalSection(&m_Locker);
    iValue = M_iStopTimeFlag;
    LeaveCriticalSection(&m_Locker);
    return iValue;
}

void MyH264Saver::SetIfFirstSave(bool bValue)
{
    WriteFormatLog("MyH264Saver::SetIfFirstSave %d", bValue);
    EnterCriticalSection(&m_Locker);
    m_bFirstSave = bValue;
    LeaveCriticalSection(&m_Locker);
}

bool MyH264Saver::GetIfFirstSave()
{
    bool bValue = false;
    EnterCriticalSection(&m_Locker);
    bValue = m_bFirstSave;
    LeaveCriticalSection(&m_Locker);
    return bValue;
}

void MyH264Saver::SetSavePath(const char* filePath, size_t bufLength)
{
    EnterCriticalSection(&m_Locker);
    if (bufLength < sizeof(m_chFilePath))
    {
        memcpy(m_chFilePath, filePath, bufLength);
        m_chFilePath[bufLength] = '\0';
    }
    LeaveCriticalSection(&m_Locker);
}

const char* MyH264Saver::GetSavePath()
{
    const char* pValue = NULL;
    EnterCriticalSection(&m_Locker);
    pValue = m_chFilePath;
    LeaveCriticalSection(&m_Locker);
    return pValue;
}


void MyH264Saver::WriteFormatLog(const char *szfmt, ...)
{
    if (!GetLogEnable())
        return;

    static char g_szString[10240] = { 0 };
    memset(g_szString, 0, sizeof(g_szString));

    va_list arg_ptr;
    va_start(arg_ptr, szfmt);
    //vsnprintf_s(g_szPbString, sizeof(g_szPbString), szfmt, arg_ptr);
    //_vsnprintf(g_szPbString,  sizeof(g_szPbString),  szfmt, arg_ptr);
    vsnprintf(g_szString, sizeof(g_szString), szfmt, arg_ptr);
    va_end(arg_ptr);

    SYSTEMTIME  stimeNow = Tool_GetCurrentTime();

    char chBuffer1[256] = { 0 };
    //_getcwd(chBuffer1, sizeof(chBuffer1));

    sprintf_s(chBuffer1, sizeof(chBuffer1), "%s\\XLWLog\\%04d-%02d-%02d\\",
        m_chCurrentPath,
        stimeNow.wYear,
        stimeNow.wMonth,
        stimeNow.wDay);
    Tool_MakeDir(chBuffer1);

    char chLogFileName[512] = { 0 };
    sprintf_s(chLogFileName, sizeof(chLogFileName), "%s/Video_%d-%02d_%02d_%02d_%02dm.log",
        chBuffer1,
        stimeNow.wYear,
        stimeNow.wMonth,
        stimeNow.wDay,
        stimeNow.wHour,
        stimeNow.wMinute/10);

    FILE *file = NULL;
    file = fopen(chLogFileName, "a+");
    //fopen_s(&file, chLogFileName, "a+");
    if (file)
    {
        //fprintf(file, "[%04d-%02d-%02d %02d:%02d:%02d:%03lld] : %s\n",
        //    stimeNow.tm_year + 1900,
        //    stimeNow.tm_mon + 1,
        //    stimeNow.tm_mday,
        //    stimeNow.tm_hour,
        //    stimeNow.tm_min,
        //    stimeNow.tm_sec,
        //    iTimeInMilliseconds % 1000,
        //    g_szString);

        fprintf(file, "[%04d-%02d-%02d %02d:%02d:%02d:%03d] : %s\n",
            stimeNow.wYear,
            stimeNow.wMonth,
            stimeNow.wDay,
            stimeNow.wHour,
            stimeNow.wMinute,
            stimeNow.wSecond,
            stimeNow.wMilliseconds,
            g_szString);

        fclose(file);
        file = NULL;
    }
}