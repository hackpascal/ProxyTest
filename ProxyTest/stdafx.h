// stdafx.h : 标准系统包含文件的包含文件，
// 或是经常使用但不常更改的
// 特定于项目的包含文件
//

#pragma once

#include "targetver.h"

#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <tchar.h>
#include <string.h>
#include <malloc.h>
#include <stdlib.h>



// TODO: 在此处引用程序需要的其他头文件
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
