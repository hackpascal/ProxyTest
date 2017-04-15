// ProxyTest.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"

BOOL WINAPI DebugControlHandler(DWORD dwCtrlType);

DWORD dwConnectCheckTimeout = 5;

int _tmain(int argc, _TCHAR* argv[])
{
	WSADATA wsaData;

	// 初始化线程相关信息
	ThreadInitialize();

	SetConsoleCtrlHandler(DebugControlHandler, TRUE);
	
	// Winsock 初始化
	WSAStartup(MAKEWORD(2, 2), &wsaData);

	// 主循环
	SocketLoop();

	WaitForAllThreads(10);

	// 结束所以线程
	CleanupThreads(5);

	// Winsock 清理
	WSACleanup();

	// 清理线程相关信息
	ThreadCleanup();

	return 0;
}

// 注册关闭控制台窗口、按下 Ctrl + C 事件处理函数
BOOL WINAPI DebugControlHandler(DWORD dwCtrlType)
{
	switch(dwCtrlType)
	{
	case CTRL_C_EVENT:
	case CTRL_BREAK_EVENT:
	case CTRL_CLOSE_EVENT:
		// 发送停止信号
		StopSignal();
		return TRUE;
	}
	return FALSE;
}

