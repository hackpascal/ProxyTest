#include "stdafx.h"

DWORD WINAPI ProcessRequest(LPVOID lpParameter);
DWORD WINAPI HttpProxyRequest(PSOCKET_PEER self);
DWORD WINAPI HttpsProxyRequest(PSOCKET_PEER self);

BOOL SocketLoop(void)
{
	int ret;
	SOCKET s;
	SOCKET_PEER listen;

	PeerInit(&listen);

	// 监听
	if (!PeerListen(&listen, "0.0.0.0", 8090, SOMAXCONN))
	{
		OutputString("错误：在 0.0.0.0:8090 上监听失败。WSALastError = %d\n", WSAGetLastError());
		return FALSE;
	}

	// 主循环
	while (true)
	{
		if (WaitForStop(50))
			break;

		// 新客户端建立连接
		ret = PeerAccept(&listen, &s);

		if (ret == SOCKET_ERROR)
		{
			// Socket 出错
			OutputString("错误：accept 失败。WSALastError = %d\n", WSAGetLastError());
			PeerClose(&listen);
			return FALSE;
		}

		// 没有新连接
		if (ret == 0)
			continue;

		// 为新连接创建线程，并将新连接套接字传递给线程
		if (!NewThread(ProcessRequest, (LPVOID) s, NULL))
		{
			OutputString("错误：无法创建线程。LastError = %d\n", GetLastError());
			closesocket(s);
		}
	}

	return TRUE;
}

// 新连接调度函数
DWORD WINAPI ProcessRequest(LPVOID lpParameter)
{
	SOCKET_PEER peer;
	int len;
	char buff[10];
	DWORD dwHttpType = 0;
	DWORD dwStartTick = GetTickCount();
	
	// 附加套接字
	PeerInit(&peer);
	PeerAttach(&peer, (SOCKET) lpParameter);

	while (true)
	{
		if (WaitForStop(50))
			goto _close_connection;

		// 读取数据，而不将其移出队列
		PeerPeek(&peer, buff, sizeof (buff) - 1, &len);

		if (PeerIsClosed(&peer))
			goto _close_connection;

		if (!dwHttpType)
		{
			// 检测 HTTP 请求方法
			if ((len >= 4) && (!strncmp(buff, "GET", 3)))
			{
				if (buff[3] == '\x20' || buff[3] == '\t')
					dwHttpType = 1;
				continue;
			}
			if ((len >= 5) && (!strncmp(buff, "POST", 4)))
			{
				if (buff[4] == '\x20' || buff[4] == '\t')
					dwHttpType = 1;
				continue;
			}

			if ((len >= 8) && (!strncmp(buff, "CONNECT", 7)))
			{
				if (buff[7] == '\x20' || buff[3] == '\t')
					dwHttpType = 2;
				continue;
			}
		}
		else
		{
			OutputString("== 新连接 ==\n线程：%08lx，套接字 = %u\n来自 %s:%u\n\n", 
				GetCurrentThreadId(), peer.s, inet_ntoa(peer.peer_addr.sin_addr), 
				ntohs(peer.peer_addr.sin_port) & 0xffff);

			// 转到对应的 HTTP 处理函数
			if (dwHttpType == 1)
				return HttpProxyRequest(&peer);

			if (dwHttpType == 2)
				return HttpsProxyRequest(&peer);

			// XXX: 不可能到这吧。。
			break;
		}
	}

	// 请求失败
	if (strlen(buff) == 0)
	{
		HttpResponse(&peer, 400, MAKEWORD(1, 1), "Bad Request");
	}
	else
	{
		for (size_t i = 0; i < strlen(buff); i++)
		{
			if ((buff[i] <= 32))
				buff[i] = 0;
		}
		HttpResponse(&peer, 415, MAKEWORD(1, 1), "Unrecognised method: %s", buff);
	}

_close_connection:
	PeerClose(&peer);

	return 0;
}