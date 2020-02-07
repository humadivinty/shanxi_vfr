#ifndef VEHRECCOMMONDEF_H
#define VEHRECCOMMONDEF_H
#include <windows.h>

//************************************
// Method:        VehRec_CarData
// Describe:        1.1.	 �ص������� ����������ݣ�������ͼ����βͼ������¼�����û�У�·��Ϊ�մ�
// FullName:      VehRec_CarData
// Access:          public 
// Returns:         int 
// Returns Describe:
// Parameter:    int handle     ;�豸���
// Parameter:    char *colpic   ;������ͼ����·��,��������С���Ȳ���С�ڸ��ֽ�
// Parameter:    char *platepic     ;��βͼƬ����·��,��������С���Ȳ���С�ڸ��ֽ�
// Parameter:    char *recfile        ;¼���ļ�����·��,��������С���Ȳ���С�ڸ��ֽ�
//************************************
typedef int (CALLBACK  *VehRec_CarData)(int handle, char *colpic, char *platepic, char *recfile);

#endif
