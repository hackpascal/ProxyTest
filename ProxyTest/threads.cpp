#include "stdafx.h"

static CRITICAL_SECTION csAtom;
static HANDLE hStopEvent;

static THREAD_INFO aThreads[100];
static DWORD dwThreadsUsage = 0;

// 初始化线程相关信息
void ThreadInitialize(void)
{
	// 初始化临界区，用于线程同步
	InitializeCriticalSection(&csAtom);
	// 创建停止信号事件
	hStopEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

	// 清空线程池
	ZeroMemory(aThreads, sizeof (aThreads));
}

// 清理线程相关信息
void ThreadCleanup(void)
{
	CloseHandle(hStopEvent);
	DeleteCriticalSection(&csAtom);
}

// 线程锁定
void ThreadLock(void)
{
	EnterCriticalSection(&csAtom);
}

// 线程解锁
void ThreadUnlock(void)
{
	LeaveCriticalSection(&csAtom);
}

// 延时，并判断是否发出停止信号
BOOL WaitForStop(DWORD dwMilliseconds)
{
	return WaitForSingleObject(hStopEvent, dwMilliseconds) != WAIT_TIMEOUT;
}

// 发出停止信号
void StopSignal(void)
{
	SetEvent(hStopEvent);
}

// 新建线程
BOOL NewThread(LPTHREAD_START_ROUTINE lpProc, LPVOID lpParam, PTHREAD_INFO pThreadInfo)
{
	int i;

	// 检查在已使用的线程记录中是否有已结束的线程
	for (i = 0; i < (int) dwThreadsUsage; i++)
	{
		if (aThreads[i].hThread && (WaitForSingleObject(aThreads[i].hThread, 0) != WAIT_OBJECT_0)) continue;

		CloseHandle(aThreads[i].hThread);
		aThreads[i].hThread = CreateThread(NULL, 0, lpProc, lpParam, 0, &aThreads[i].dwThreadId);

		if (aThreads[i].hThread)
		{
			if (pThreadInfo) *pThreadInfo = aThreads[i];
			return TRUE;
		}

		return FALSE;
	}

	// 在线程池中增加新的线程记录
	if ((i == dwThreadsUsage) && (dwThreadsUsage < sizeof (aThreads) / sizeof (THREAD_INFO)))
	{
		dwThreadsUsage = ++i;

		aThreads[i].hThread = CreateThread(NULL, 0, lpProc, lpParam, 0, &aThreads[i].dwThreadId);

		if (aThreads[i].hThread)
		{
			if (pThreadInfo) *pThreadInfo = aThreads[i];
			return TRUE;
		}
	}

	return FALSE;
}

// 结束线程
BOOL EndThread(PTHREAD_INFO pThreadInfo)
{
	if (pThreadInfo->hThread)
	{
		// 线程本身已结束
		if (WaitForSingleObject(pThreadInfo->hThread, 0) == WAIT_OBJECT_0)
		{
			CloseHandle(pThreadInfo->hThread);
			ZeroMemory(pThreadInfo, sizeof (THREAD_INFO));
			return TRUE;
		}

		// 结束线程
		if (TerminateThread(pThreadInfo->hThread, 0))
		{
			// 等待线程结束
			if (WaitForSingleObject(pThreadInfo->hThread, 3000) == WAIT_OBJECT_0)
			{
				CloseHandle(pThreadInfo->hThread);
				ZeroMemory(pThreadInfo, sizeof (THREAD_INFO));
				return TRUE;
			}

			return FALSE;
		}

		return FALSE;
	}

	ZeroMemory(pThreadInfo, sizeof (THREAD_INFO));
	return TRUE;
}

BOOL WaitForAllThreads(DWORD dwWaitSeconds)
{
	int count = dwThreadsUsage;
	int i = 0;
	DWORD dwStartTick = GetTickCount();
	BOOL bAllTerminated = FALSE;
	
	// 对线程池进行循环
	while (true)
	{
		int j = i % count;

		if (dwWaitSeconds)
		{
			// 如果到达预设等待时间，则不再等待全部线程结束
			if (GetTickCount() - dwStartTick > dwWaitSeconds * 1000)
				break;
		}

		// 复位线程全部结束状态
		if (i % count == 0)
			bAllTerminated = TRUE;

		if (!aThreads[j].hThread) goto _add_iterator;
		if (WaitForSingleObject(aThreads[j].hThread, 0) != WAIT_OBJECT_0)
			bAllTerminated = FALSE;

		if ((i + 1) % count == 0)
		{
			// 如果线程全部结束，则退出循环
			if (bAllTerminated)
				break;
		}

_add_iterator:
		i++;
	}

	return bAllTerminated;
}

// 结束所有线程
BOOL CleanupThreads(DWORD dwWaitSeconds)
{
	int count = dwThreadsUsage;
	int i = 0;
	DWORD dwStartTick = GetTickCount();
	BOOL bAllTerminated = FALSE;
	
	// 对线程池进行循环
	while (true)
	{
		int j = i % count;

		if (dwWaitSeconds)
		{
			// 如果到达预设等待时间，则不再等待全部线程结束
			if (GetTickCount() - dwStartTick > dwWaitSeconds * 1000)
				break;
		}

		// 复位线程全部结束状态
		if (i % count == 0)
			bAllTerminated = TRUE;

		if (!aThreads[j].hThread) goto _add_iterator;
		if (WaitForSingleObject(aThreads[j].hThread, 0) == WAIT_OBJECT_0) goto _add_iterator;

		// 使用逻辑与运算来记录线程结束情况
		bAllTerminated &= EndThread(&aThreads[j]);

		if ((i + 1) % count == 0)
		{
			// 如果线程全部结束，则退出循环
			if (bAllTerminated)
				break;
		}

_add_iterator:
		i++;
	}

	// 关闭剩余句柄
	for (i = 0; i < (int) dwThreadsUsage; i++)
	{
		if (aThreads[i].hThread)
			CloseHandle(aThreads[i].hThread);
	}

	return TRUE;
}

void OutputString(const char *fmt, ...)
{
	int nSize;
	char *buff;
	va_list arglist;

	//线程同步
	ThreadLock();

	va_start(arglist, fmt);

	//估算需要的缓冲区大小，不包含 NULL 终止符
	nSize = _vscprintf(fmt, arglist);
	//分配缓冲区，包含 NULL 字符
	buff = new char[nSize + 1];
	//调用格式化字符串的函数
	_vsprintf_s_l(buff, nSize + 1, fmt, NULL, arglist);
	//打印字符串
	printf(buff);
	//释放缓冲区
	delete [] buff;

	va_end(arglist);

	//释放
	ThreadUnlock();
}

void OutputStringDirect(const char *str)
{
	//线程同步
	ThreadLock();

	puts(str);

	//释放
	ThreadUnlock();
}
