// stdafx.h : ��׼ϵͳ�����ļ��İ����ļ���
// ���Ǿ���ʹ�õ��������ĵ�
// �ض�����Ŀ�İ����ļ�
//

#pragma once

#include "targetver.h"

#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <tchar.h>
#include <string.h>
#include <malloc.h>
#include <stdlib.h>



// TODO: �ڴ˴����ó�����Ҫ������ͷ�ļ�
#define WIN32_LEAN_AND_MEAN
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <Windows.h>
#include <winsock2.h>
#include <winsock.h>
#include <ws2tcpip.h>
#include <strsafe.h>

#pragma comment(lib, "ws2_32.lib")

#include "cstring.h"

#include "threads.h"
#include "peerlib.h"
#include "httplib.h"
#include "proxyauth.h"

BOOL SocketLoop(void);

extern DWORD dwConnectCheckTimeout;
