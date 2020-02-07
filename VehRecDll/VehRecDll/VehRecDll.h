// ���� ifdef ���Ǵ���ʹ�� DLL �������򵥵�
// ��ı�׼�������� DLL �е������ļ��������������϶���� VEHRECDLL_EXPORTS
// ���ű���ġ���ʹ�ô� DLL ��
// �κ�������Ŀ�ϲ�Ӧ����˷��š�������Դ�ļ��а������ļ����κ�������Ŀ���Ὣ
// VEHRECDLL_API ������Ϊ�Ǵ� DLL ����ģ����� DLL ���ô˺궨���
// ������Ϊ�Ǳ������ġ�
#ifdef VEHRECDLL_EXPORTS
#define VEHRECDLL_API EXTERN_C __declspec(dllexport)
#else
#define VEHRECDLL_API EXTERN_C __declspec(dllimport)
#endif

#include "vehRecCommondef.h"

//************************************
// Method:        VehRec_InitEx
// Describe:        ��ʼ����̬��
// FullName:      VehRec_InitEx
// Access:          public 
// Returns:        int WINAPI
// Returns Describe:    0		�ɹ�
//                                -1		�����������
 //                               -3		����������̬��ʧ��
 //                               -14	��������ʧ��
// Parameter:    int iLog       ;��־����0-��1-��
// Parameter:    char * iLogPath    ;��־Ŀ¼��󳤶��ֽ�
// Parameter:    int iLogSaveDay    ;��־��������
//************************************
VEHRECDLL_API int WINAPI VehRec_InitEx(int iLog, char *iLogPath, int iLogSaveDay);

//************************************
// Method:        VehRec_Free
// Describe:        �ͷŶ�̬��
// FullName:      VehRec_Free
// Access:          public 
// Returns:        int WINAPI
// Returns Describe:    0		�ɹ�,      ֻ��Ҫ����1�Σ����Ҳ������߳��е���
//************************************
VEHRECDLL_API int WINAPI VehRec_Free();

//************************************
// Method:        VehRec_Connect
// Describe:        ���ӳ����豸
// FullName:      VehRec_Connect
// Access:          public 
// Returns:        VEHRECDLL_API int WINAPI
// Returns Describe:        
//                      ��=1000�豸��������豸�ɹ�
//                      -1		�����������
//                      - 2		���ҿ����豸�ڵ�ʧ��
//                      - 4		�����豸ʧ��
// Parameter:    char * devIP       ���豸IP��ַ��ʽ"X.X.X.X"
// Parameter:    char * savepath    ���ļ�����·������ͼ����βͼ������¼���ڴ�·���´洢���������ö�����������ȷ�����Դ洢һ�������
//************************************
VEHRECDLL_API int WINAPI VehRec_Connect(char *devIP, char *savepath);

//************************************
// Method:        VehRec_ConnectEX
// Describe:        ���ӳ����豸
// FullName:      VehRec_ConnectEX
// Access:          public 
// Returns:        int WINAPI
// Returns Describe:    ��=1000�豸��������豸�ɹ�
//                                -1		�����������
//                                - 2		���ҿ����豸�ڵ�ʧ��
//                                - 4		�����豸ʧ��
// Parameter:    char * devIP       ;�豸IP��ַ��ʽ"X.X.X.X"
// Parameter:    char * savepath   ;�ļ�����·������ͼ����βͼ������¼���ڴ�·���´洢���������ö�����������ȷ�����Դ洢һ�������
// Parameter:    VehRec_CarData callBackFun     ;��������ݵĻص�����ָ��,�������null��ʹ��VehRec_GetCarData��ȡ���
//************************************
VEHRECDLL_API int WINAPI VehRec_ConnectEX(char *devIP, char *savepath, VehRec_CarData callBackFun);

//************************************
// Method:        VehRec_DisConnect
// Describe:        �Ͽ������豸����
// FullName:      VehRec_DisConnect
// Access:          public 
// Returns:        void WINAPI
// Returns Describe:
// Parameter:    int handle     �豸���
//************************************
VEHRECDLL_API void WINAPI VehRec_DisConnect(int handle);

//************************************
// Method:        VehRec_VEHSignle
// Describe:        ���տ�ʼ�����ź�
// FullName:      VehRec_VEHSignle
// Access:          public 
// Returns:        int WINAPI
// Returns Describe:    0  �ɹ�
//                                ���� ʧ��
// Parameter:    int handle     �豸���
// Parameter:    int sig        ;��ʼ�����ź�
//                                      1-��ʼ�ź�(����ʹ�ù�դ��⵽�г��ź�)
//                                      2 - �����ź�(�����豸�õ���������ʱ�����ź�)
//************************************
VEHRECDLL_API int WINAPI VehRec_VEHSignle(int handle, int sig);

//************************************
// Method:        VehRec_GetCarData
// Describe:        ��ȡ��������
// FullName:      VehRec_GetCarData
// Access:          public 
// Returns:        int 
// Returns Describe:    0  �ɹ�; ���� ʧ��
// Parameter:    int handle
// Parameter:    char * colpic          ;[in]������ͼ����·��������·��������
// Parameter:    char * platepic       ;[in]��βͼƬ����·��������·��������
// Parameter:    char * recfile         ;[in]¼���ļ�����·��������·��������
//Notice:           ͨ���ú����õ�����·���󣬱�������һ��������ǰ�����ݿ����ߣ��������ݻᱻ����
//************************************
VEHRECDLL_API int WINAPI VehRec_GetCarData(int handle, char *colpic, char *platepic, char *recfile);