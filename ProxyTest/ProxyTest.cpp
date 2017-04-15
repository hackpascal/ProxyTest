// ProxyTest.cpp : �������̨Ӧ�ó������ڵ㡣
//

#include "stdafx.h"

BOOL WINAPI DebugControlHandler(DWORD dwCtrlType);

DWORD dwConnectCheckTimeout = 5;

int _tmain(int argc, _TCHAR* argv[])
{
	WSADATA wsaData;

	// ��ʼ���߳������Ϣ
	ThreadInitialize();

	SetConsoleCtrlHandler(DebugControlHandler, TRUE);
	
	// Winsock ��ʼ��
	WSAStartup(MAKEWORD(2, 2), &wsaData);

	// ��ѭ��
	SocketLoop();

	WaitForAllThreads(10);

	// ���������߳�
	CleanupThreads(5);

	// Winsock ����
	WSACleanup();

	// �����߳������Ϣ
	ThreadCleanup();

	return 0;
}

// ע��رտ���̨���ڡ����� Ctrl + C �¼�������
BOOL WINAPI DebugControlHandler(DWORD dwCtrlType)
{
	switch(dwCtrlType)
	{
	case CTRL_C_EVENT:
	case CTRL_BREAK_EVENT:
	case CTRL_CLOSE_EVENT:
		// ����ֹͣ�ź�
		StopSignal();
		return TRUE;
	}
	return FALSE;
}

