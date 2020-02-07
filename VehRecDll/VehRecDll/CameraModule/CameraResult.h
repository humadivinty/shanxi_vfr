#ifndef CAMERRESULT
#define CAMERRESULT
#include <string>
#include<windef.h>

/* ʶ������ͼ���Ͷ��� */
#define RECORD_BIGIMG_BEST_SNAPSHOT			0x0001	/**< ������ʶ��ͼ */
#define RECORD_BIGIMG_LAST_SNAPSHOT			0x0002	/**< ���ʶ��ͼ */
#define RECORD_BIGIMG_BEGIN_CAPTURE			0x0003	/**< ��ʼץ��ͼ */
#define RECORD_BIGIMG_BEST_CAPTURE			0x0004	/**< ������ץ��ͼ */
#define RECORD_BIGIMG_LAST_CAPTURE			0x0005	/**<  ���ץ��ͼ */

#define  MAX_IMG_SIZE 10*1024*1024

//��������
#define UNKOWN_TYPE 0
#define BUS_TYPE_1 1    //��1
#define BUS_TYPE_2 2
#define BUS_TYPE_3 3
#define BUS_TYPE_4 4
#define BUS_TYPE_5 5    //��5
#define TRUCK_TYPE_1 11 //��1
#define TRUCK_TYPE_2 12
#define TRUCK_TYPE_3 13
#define TRUCK_TYPE_4 14
#define TRUCK_TYPE_5 15 //��5
#define TRUCK_TYPE_6 16 //��5

//������С����
#define CAR_TYPE_SIZE_SMALL 1
#define CAR_TYPE_SIZE_MIDDLE 6
#define CAR_TYPE_SIZE_BIG 2
#define CAR_TYPE_SIZE_UNKNOWN 0

//��ʻ����
#define DIRECTION_UP_TO_DOWN 0      //����
#define DIRECTION_DOWN_TO_UP 1      //����
#define DIRECTION_EAST_TO_WEST 2    //�ɶ�����
#define DIRECTION_WEST_TO_EAST 3    //������
#define DIRECTION_SOUTH_TO_NORTH 4    //������
#define DIRECTION_NORTH_TO_SOUTH 5    //�ɱ�����

//������ɫ����
#define  COLOR_UNKNOW (9)
#define  COLOR_BLUE 0
#define  COLOR_YELLOW 1
#define  COLOR_BLACK 2
#define  COLOR_WHITE 3
#define  COLOR_GRADIENT_CREEN 4
#define  COLOR_YELLOW_GREEN 5
#define  COLOR_BLUE_WHITE 6
#define  COLOR_TEMPORARY_LICENCE 7
#define  COLOR_GREEN 11
#define  COLOR_RED 12

//������ɫ����
#define  PLATE_TYPE_NONE 0      //0-������
#define  PLATE_TYPE_NORMAL 1      //1-92ʽ���ó�������
#define  PLATE_TYPE_POLICE 2      //2-���ó�������
#define  PLATE_TYPE_UP_DOWN_ARMY 3      //3-���¾���������û�����ֳ������ͣ�
#define  PLATE_TYPE_ARMED_POLICE 4      //4-92ʽ�侯��������(����û�����ֳ�������)
#define  PLATE_TYPE_LEFT_RIGHT_ARMY 5      //5-���Ҿ�����������(һ�нṹ)
//#define  PLATE_TYPE_NONE 6      //
#define  PLATE_TYPE_CUSTOM 7      //7-02ʽ���Ի���������
#define  PLATE_TYPE_DOUBLE_YELLOW_LINE 8      //8-��ɫ˫��β�ƣ������򹫽���β�ƣ�
#define  PLATE_TYPE_NEW_ARMY 9      //9-04ʽ�¾�������(���нṹ)
#define  PLATE_TYPE_EMBASSY 10      //10-ʹ�ݳ�������
#define  PLATE_TYPE_ONE_LINE_ARMED_POLICE 11      //11-һ�нṹ����WJ��������
#define  PLATE_TYPE_TWO_LINE_ARMED_POLICE  12      //12-���нṹ����WJ��������
#define  PLATE_TYPE_YELLOW_1225_FARMER 13      //13-��ɫ1225ũ�ó�
#define  PLATE_TYPE_GREEN_1325_FARMER 14      //14-��ɫ1325ũ�ó�
#define  PLATE_TYPE_YELLOW_1325_FARMER 15      //15-��ɫ1325ũ�ó�
#define  PLATE_TYPE_MOTORCYCLE 16      //16-Ħ�г�
#define  PLATE_TYPE_NEW_ENERGY 17      //17-����Դ


class CameraIMG
{
public:
	CameraIMG();
	CameraIMG(const CameraIMG& CaIMG);
	~CameraIMG();

	UINT32 wImgWidth;
    UINT32 wImgHeight;
	DWORD dwImgSize;
    UINT32  wImgType;
	char chSavePath[256];
    PBYTE pbImgData;

	CameraIMG& operator = (const CameraIMG& CaIMG);
};

class CameraResult
{
public:

	CameraResult();
	CameraResult(const CameraResult& CaRESULT);
	~CameraResult();

    float fVehLenth;			//����
    float fDistanceBetweenAxles;		//���
    float fVehHeight;		//����
    float fVehWidth;       //
    float fConfidenceLevel;     //���Ŷ�

    UINT64 dw64TimeMS;
    DWORD dwReceiveTime;
	UINT32 dwCarID;
    INT32 iDeviceID;
    INT32 iPlateColor;
    INT32 iPlateTypeNo;
    INT32 iSpeed;
    INT32 iResultNo;
    INT32 iVehTypeNo;		//���ʹ���
    INT32 iVehSizeType;       //���ʹ�С����
    INT32 iVehBodyColorNo;
    INT32 iVehBodyDeepNo;
    INT32 iVehLogoType;
    INT32 iAreaNo;
    INT32 iRoadNo;
    INT32 iLaneNo;
    INT32 iDirection;
    INT32 iWheelCount;		//����
	INT32 iAxletreeCount;		//����
    INT32 iAxletreeGroupCount;//������
    INT32 iAxletreeType;		//����
    INT32 iReliability;       //���Ŷ�

    bool bBackUpVeh;		//�Ƿ񵹳�

    char chDeviceIp[64];
    char chServerIP[64];
	char chPlateNO[64];
	char chPlateColor[64];
	char chListNo[256];
	char chPlateTime[256];
	char chSignStationID[256];
	char chSignStationName[256];
    char chSignDirection[256];
    char chDeviceID[256];
    char chLaneID[256];
    char chCarFace[256];
    char chChileLogo[256];
    char pcAppendInfo[10240];
    char chVehTypeText[256];
    char chAxleType[256];

    char chSaveFileName[256];

    //std::string strAppendInfo;

	CameraIMG CIMG_BestSnapshot;	/**< ������ʶ��ͼ */
	CameraIMG CIMG_LastSnapshot;	/**< ���ʶ��ͼ */
	CameraIMG CIMG_BeginCapture;	/**< ��ʼץ��ͼ */
	CameraIMG CIMG_BestCapture;		/**< ������ץ��ͼ */
	CameraIMG CIMG_LastCapture;		/**< ���ץ��ͼ */
	CameraIMG CIMG_PlateImage;		/**< ����Сͼ */
	CameraIMG CIMG_BinImage;			/**< ���ƶ�ֵͼ */

	CameraResult& operator = (const CameraResult& CaRESULT);

    bool SerializationToDisk(const char* chFilePath);
    bool SerializationFromDisk(const char* chFilePath);

	friend bool SerializationResultToDisk(const char* chFilePath, const CameraResult& CaResult);
	friend bool SerializationFromDisk(const char* chFilePath, CameraResult& CaResult);

	friend bool SerializationAsConfigToDisk(const char* chFilePath, const CameraResult& CaResult);
	friend bool SerializationAsConfigFromDisk(const char* chFilePath, CameraResult& CaResult);
};

typedef struct _tagSafeModeInfo
{
	int iEableSafeMode;
	char chBeginTime[256];
	char chEndTime[256];
	int index;
	int DataInfo;
	_tagSafeModeInfo()
	{
		iEableSafeMode = 0;
		memset(chBeginTime, 0, sizeof(chBeginTime));
		memset(chEndTime, 0, sizeof(chEndTime));
		index = 0;
		DataInfo = 0;
	}
}_tagSafeModeInfo;

#endif
