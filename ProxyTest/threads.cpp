#include "stdafx.h"

static CRITICAL_SECTION csAtom;
static HANDLE hStopEvent;

static THREAD_INFO aThreads[100];
static DWORD dwThreadsUsage = 0;

// ��ʼ���߳������Ϣ
void ThreadInitialize(void)
{
	// ��ʼ���ٽ����������߳�ͬ��
	InitializeCriticalSection(&csAtom);
	// ����ֹͣ�ź��¼�
	hStopEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

	// ����̳߳�
	ZeroMemory(aThreads, sizeof (aThreads));
}

// �����߳������Ϣ
void ThreadCleanup(void)
{
	CloseHandle(hStopEvent);
	DeleteCriticalSection(&csAtom);
}

// �߳�����
void ThreadLock(void)
{
	EnterCriticalSection(&csAtom);
}

// �߳̽���
void ThreadUnlock(void)
{
	LeaveCriticalSection(&csAtom);
}

// ��ʱ�����ж��Ƿ񷢳�ֹͣ�ź�
BOOL WaitForStop(DWORD dwMilliseconds)
{
	return WaitForSingleObject(hStopEvent, dwMilliseconds) != WAIT_TIMEOUT;
}

// ����ֹͣ�ź�
void StopSignal(void)
{
	SetEvent(hStopEvent);
}

// �½��߳�
BOOL NewThread(LPTHREAD_START_ROUTINE lpProc, LPVOID lpParam, PTHREAD_INFO pThreadInfo)
{
	int i;

	// �������ʹ�õ��̼߳�¼���Ƿ����ѽ������߳�
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

	// ���̳߳��������µ��̼߳�¼
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

// �����߳�
BOOL EndThread(PTHREAD_INFO pThreadInfo)
{
	if (pThreadInfo->hThread)
	{
		// �̱߳����ѽ���
		if (WaitForSingleObject(pThreadInfo->hThread, 0) == WAIT_OBJECT_0)
		{
			CloseHandle(pThreadInfo->hThread);
			ZeroMemory(pThreadInfo, sizeof (THREAD_INFO));
			return TRUE;
		}

		// �����߳�
		if (TerminateThread(pThreadInfo->hThread, 0))
		{
			// �ȴ��߳̽���
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
	
	// ���̳߳ؽ���ѭ��
	while (true)
	{
		int j = i % count;

		if (dwWaitSeconds)
		{
			// �������Ԥ��ȴ�ʱ�䣬���ٵȴ�ȫ���߳̽���
			if (GetTickCount() - dwStartTick > dwWaitSeconds * 1000)
				break;
		}

		// ��λ�߳�ȫ������״̬
		if (i % count == 0)
			bAllTerminated = TRUE;

		if (!aThreads[j].hThread) goto _add_iterator;
		if (WaitForSingleObject(aThreads[j].hThread, 0) != WAIT_OBJECT_0)
			bAllTerminated = FALSE;

		if ((i + 1) % count == 0)
		{
			// ����߳�ȫ�����������˳�ѭ��
			if (bAllTerminated)
				break;
		}

_add_iterator:
		i++;
	}

	return bAllTerminated;
}

// ���������߳�
BOOL CleanupThreads(DWORD dwWaitSeconds)
{
	int count = dwThreadsUsage;
	int i = 0;
	DWORD dwStartTick = GetTickCount();
	BOOL bAllTerminated = FALSE;
	
	// ���̳߳ؽ���ѭ��
	while (true)
	{
		int j = i % count;

		if (dwWaitSeconds)
		{
			// �������Ԥ��ȴ�ʱ�䣬���ٵȴ�ȫ���߳̽���
			if (GetTickCount() - dwStartTick > dwWaitSeconds * 1000)
				break;
		}

		// ��λ�߳�ȫ������״̬
		if (i % count == 0)
			bAllTerminated = TRUE;

		if (!aThreads[j].hThread) goto _add_iterator;
		if (WaitForSingleObject(aThreads[j].hThread, 0) == WAIT_OBJECT_0) goto _add_iterator;

		// ʹ���߼�����������¼�߳̽������
		bAllTerminated &= EndThread(&aThreads[j]);

		if ((i + 1) % count == 0)
		{
			// ����߳�ȫ�����������˳�ѭ��
			if (bAllTerminated)
				break;
		}

_add_iterator:
		i++;
	}

	// �ر�ʣ����
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

	//�߳�ͬ��
	ThreadLock();

	va_start(arglist, fmt);

	//������Ҫ�Ļ�������С�������� NULL ��ֹ��
	nSize = _vscprintf(fmt, arglist);
	//���仺���������� NULL �ַ�
	buff = new char[nSize + 1];
	//���ø�ʽ���ַ����ĺ���
	_vsprintf_s_l(buff, nSize + 1, fmt, NULL, arglist);
	//��ӡ�ַ���
	printf(buff);
	//�ͷŻ�����
	delete [] buff;

	va_end(arglist);

	//�ͷ�
	ThreadUnlock();
}

void OutputStringDirect(const char *str)
{
	//�߳�ͬ��
	ThreadLock();

	puts(str);

	//�ͷ�
	ThreadUnlock();
}
