// 下列 ifdef 块是创建使从 DLL 导出更简单的
// 宏的标准方法。此 DLL 中的所有文件都是用命令行上定义的 VEHRECDLL_EXPORTS
// 符号编译的。在使用此 DLL 的
// 任何其他项目上不应定义此符号。这样，源文件中包含此文件的任何其他项目都会将
// VEHRECDLL_API 函数视为是从 DLL 导入的，而此 DLL 则将用此宏定义的
// 符号视为是被导出的。
#ifdef VEHRECDLL_EXPORTS
#define VEHRECDLL_API EXTERN_C __declspec(dllexport)
#else
#define VEHRECDLL_API EXTERN_C __declspec(dllimport)
#endif

#include "vehRecCommondef.h"

//************************************
// Method:        VehRec_InitEx
// Describe:        初始化动态库
// FullName:      VehRec_InitEx
// Access:          public 
// Returns:        int WINAPI
// Returns Describe:    0		成功
//                                -1		输入参数错误
 //                               -3		加载依赖动态库失败
 //                               -14	创建互斥失败
// Parameter:    int iLog       ;日志开关0-关1-开
// Parameter:    char * iLogPath    ;日志目录最大长度字节
// Parameter:    int iLogSaveDay    ;日志保存天数
//************************************
VEHRECDLL_API int WINAPI VehRec_InitEx(int iLog, char *iLogPath, int iLogSaveDay);

//************************************
// Method:        VehRec_Free
// Describe:        释放动态库
// FullName:      VehRec_Free
// Access:          public 
// Returns:        int WINAPI
// Returns Describe:    0		成功,      只需要调用1次，并且不能在线程中调用
//************************************
VEHRECDLL_API int WINAPI VehRec_Free();

//************************************
// Method:        VehRec_Connect
// Describe:        连接车型设备
// FullName:      VehRec_Connect
// Access:          public 
// Returns:        VEHRECDLL_API int WINAPI
// Returns Describe:        
//                      ＞=1000设备句柄连接设备成功
//                      -1		输入参数错误
//                      - 2		查找可用设备节点失败
//                      - 4		连接设备失败
// Parameter:    char * devIP       ；设备IP地址格式"X.X.X.X"
// Parameter:    char * savepath    ；文件保存路径，组图、车尾图、车身录像在此路径下存储，根据配置定期清理，并且确保可以存储一天的数据
//************************************
VEHRECDLL_API int WINAPI VehRec_Connect(char *devIP, char *savepath);

//************************************
// Method:        VehRec_ConnectEX
// Describe:        连接车型设备
// FullName:      VehRec_ConnectEX
// Access:          public 
// Returns:        int WINAPI
// Returns Describe:    ＞=1000设备句柄连接设备成功
//                                -1		输入参数错误
//                                - 2		查找可用设备节点失败
//                                - 4		连接设备失败
// Parameter:    char * devIP       ;设备IP地址格式"X.X.X.X"
// Parameter:    char * savepath   ;文件保存路径，组图、车尾图、车身录像在此路径下存储，根据配置定期清理，并且确保可以存储一天的数据
// Parameter:    VehRec_CarData callBackFun     ;输出车数据的回调函数指针,如果传入null，使用VehRec_GetCarData获取结果
//************************************
VEHRECDLL_API int WINAPI VehRec_ConnectEX(char *devIP, char *savepath, VehRec_CarData callBackFun);

//************************************
// Method:        VehRec_DisConnect
// Describe:        断开车型设备连接
// FullName:      VehRec_DisConnect
// Access:          public 
// Returns:        void WINAPI
// Returns Describe:
// Parameter:    int handle     设备句柄
//************************************
VEHRECDLL_API void WINAPI VehRec_DisConnect(int handle);

//************************************
// Method:        VehRec_VEHSignle
// Describe:        接收开始结束信号
// FullName:      VehRec_VEHSignle
// Access:          public 
// Returns:        int WINAPI
// Returns Describe:    0  成功
//                                其他 失败
// Parameter:    int handle     设备句柄
// Parameter:    int sig        ;开始结束信号
//                                      1-开始信号(建议使用光栅检测到有车信号)
//                                      2 - 结束信号(称重设备得到车辆重量时给出信号)
//************************************
VEHRECDLL_API int WINAPI VehRec_VEHSignle(int handle, int sig);

//************************************
// Method:        VehRec_GetCarData
// Describe:        获取车辆数据
// FullName:      VehRec_GetCarData
// Access:          public 
// Returns:        int 
// Returns Describe:    0  成功; 其他 失败
// Parameter:    int handle
// Parameter:    char * colpic          ;[in]侧面组图绝对路径，包括路径和名称
// Parameter:    char * platepic       ;[in]车尾图片绝对路径，包括路径和名称
// Parameter:    char * recfile         ;[in]录像文件绝对路径，包括路径和名称
//Notice:           通过该函数得到数据路径后，必须在下一辆车结束前将数据拷贝走，否则数据会被覆盖
//************************************
VEHRECDLL_API int WINAPI VehRec_GetCarData(int handle, char *colpic, char *platepic, char *recfile);