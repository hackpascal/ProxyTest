#pragma once

typedef struct THREAD_INFO
{
	HANDLE hThread;
	DWORD dwThreadId;
} THREAD_INFO, *PTHREAD_INFO;


void ThreadInitialize(void);
void ThreadCleanup(void);
void ThreadLock(void);
void ThreadUnlock(void);
BOOL WaitForStop(DWORD dwMilliseconds);
void StopSignal(void);
void OutputString(const char *fmt, ...);
void OutputStringDirect(const char *str);

BOOL NewThread(LPTHREAD_START_ROUTINE lpProc, LPVOID lpParam, PTHREAD_INFO pThreadInfo);
BOOL EndThread(PTHREAD_INFO pThreadInfo);
BOOL WaitForAllThreads(DWORD dwWaitSeconds);
BOOL CleanupThreads(DWORD dwWaitSeconds);