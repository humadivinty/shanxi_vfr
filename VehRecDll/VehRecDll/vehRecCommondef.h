#ifndef VEHRECCOMMONDEF_H
#define VEHRECCOMMONDEF_H
#include <windows.h>

//************************************
// Method:        VehRec_CarData
// Describe:        1.1.	 回调函数， 输出车辆数据，包括组图、车尾图、车身录像，如果没有，路径为空串
// FullName:      VehRec_CarData
// Access:          public 
// Returns:         int 
// Returns Describe:
// Parameter:    int handle     ;设备句柄
// Parameter:    char *colpic   ;侧面组图绝对路径,缓冲区最小长度不能小于个字节
// Parameter:    char *platepic     ;车尾图片绝对路径,缓冲区最小长度不能小于个字节
// Parameter:    char *recfile        ;录像文件绝对路径,缓冲区最小长度不能小于个字节
//************************************
typedef int (CALLBACK  *VehRec_CarData)(int handle, char *colpic, char *platepic, char *recfile);

#endif
