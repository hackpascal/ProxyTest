#include "stdafx.h"

typedef struct SOCKET_FORWARD_INFO
{
	PSOCKET_PEER from;
	PSOCKET_PEER to;
	HANDLE hErrorEvent;
	HANDLE hStopEvent;
} SOCKET_FORWARD_INFO, *PSOCKET_FORWARD_INFO;

DWORD WINAPI SocketForward(LPVOID lpParameter);

static const char connection_ok[] = "HTTP/1.1 200 Connection Established\r\n\r\n";

DWORD WINAPI HttpsProxyRequest(PSOCKET_PEER self)
{
	int len = 0, total = 0, recv_len;
	char *buff = NULL, *hdr_end;
	char *entry_start, *method, *url, *key, *value;
	char host[256];
	char auth[1024];
	WORD wHttpVer, wPort;
	SOCKET_PEER peer;
	DWORD dw64StartTick;

	SOCKET_FORWARD_INFO tx, rx;
	HANDLE hErrorEvent, hStopEvent;
	HANDLE hTransmit, hReceive;
	DWORD dwTransmitId, dwReceiveId;

_restart:
	total = 0;

	if (len && buff)
		ZeroMemory(buff, len);

	dw64StartTick = GetTickCount();

	while (true)
	{
		if (WaitForStop(10))
			goto _close_connection;

		// 扩充缓冲区
		if (total + 1 >= len)
		{
			len += BUFF_INCREASEMENT;
			buff = (char *) realloc(buff, len);
		}
		
		// 接收请求报头
		if (!PeerReceive(self, buff + total, len - total, &recv_len))
			goto _close_connection;

		if (recv_len > 0)
		{
			total += recv_len;
			buff[total] = 0;
		}

		if (PeerIsClosed(self))
			goto _close_connection;

		// 已读完报头
		if (hdr_end = strstr(buff, "\r\n\r\n"))
			break;

		// 连接超时
		if (GetTickCount() - dw64StartTick > dwConnectCheckTimeout * 1000)
			goto _close_connection;

		Sleep(50);
	}

	hdr_end += 4;

	// 分析报头第一行 （方法、完整的 URL、HTTP 版本）
	if (!RequestParseURL(buff, &entry_start, &method, &url, &wHttpVer))
	{
		// 报文格式错误。。终止连接
		HttpResponse(self, 400, wHttpVer, "Wrong HTTP header format.");
		goto _close_connection;
	}

	// 分析 URL 的主机、端口
	if (!RequestParseHost(url, host, NULL, &wPort))
	{
		// URL 格式不合法
		HttpResponse(self, 400, wHttpVer, "Wrong HTTP header format.");
		goto _close_connection;
	}

	// 分析报头条目
	while (RequestParseEntry(entry_start, &entry_start, &key, &value))
	{
		// 认证头部
		if (!strcmp(key, "Proxy-Authorization"))
		{
			StringCchCopyA(auth, sizeof (auth), value);
			break;
		}
	}

	// 代理认证
	if (!auth[0] || !ProxyAuthenticate(auth))
	{
		char auth_required[] = 
			"HTTP/1.1 407 Proxy Authentication Required\r\n"
			"Proxy-Authenticate: Basic realm=\"ProxyTest\"\r\n"
			"\r\n";

		OutputString("线程 %08lx 需要身份认证\n\n", GetCurrentThreadId());

		if (!PeerSend(self, (void *) auth_required, sizeof (auth_required) - 1, NULL))
		{
			OutputString("线程 %08lx 发送认证请求失败\n\n", GetCurrentThreadId());
			goto _close_connection;
		}

		goto _restart;
	}

	// CONNECT 方法只有 HTTP 1.1 才支持
	if (wHttpVer != MAKEWORD(1, 1))
	{
		HttpResponse(self, 505, wHttpVer, "HTTP Version must be 1.1");
		goto _close_connection;
	}

	PeerInit(&peer);
	
	// 连接服务器
	if (!PeerConnect(&peer, host, wPort))
	{
		HttpResponse(self, 503, wHttpVer, "Unable to connect to %s:%u", host, wPort & 0xffff);
		goto _close_connection;
	}

	// 开始 socket 转发

	OutputString("线程 %08lx 开始 TCP 转发\n"
		"%s:%u <--> %s:%u\n\n",
		GetCurrentThreadId(),
		host, wPort,
		inet_ntoa(self->peer_addr.sin_addr), self->peer_addr.sin_port & 0xffff);

	// 创建通知事件
	hErrorEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	hStopEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

	// 浏览器到目标服务器的数据转发
	ZeroMemory(&tx, sizeof (tx));
	tx.from = self;
	tx.to = &peer;
	tx.hErrorEvent = hErrorEvent;
	tx.hStopEvent = hStopEvent;

	// 从目标服务器到浏览器的数据转发
	ZeroMemory(&rx, sizeof (rx));
	rx.from = &peer;
	rx.to = self;
	rx.hErrorEvent = hErrorEvent;
	rx.hStopEvent = hStopEvent;

	// 创建线程，初始为挂起状态
	hTransmit = CreateThread(NULL, 0, SocketForward, (LPVOID) &tx, CREATE_SUSPENDED, &dwTransmitId);
	if (!hTransmit)
	{
		HttpResponse(self, 500, wHttpVer, "Unable to create transmit thread.");
		goto _close_connection;
	}
	
	hReceive = CreateThread(NULL, 0, SocketForward, (LPVOID) &rx, CREATE_SUSPENDED, &dwReceiveId);
	if (!hReceive)
	{
		HttpResponse(self, 500, wHttpVer, "Unable to create receive thread.");
		goto _close_connection;
	}

	// 通知浏览器连接创建成功
	if (!PeerSend(self, (void *) connection_ok, sizeof (connection_ok) - 1, NULL))
	{
		OutputString("线程 %08lx TCP 连接建立通知失败\n\n", GetCurrentThreadId());
		goto _close_connection;
	}

	// 恢复线程
	ResumeThread(hTransmit);
	ResumeThread(hReceive);

	// 循环
	while (true)
	{
		// 如果接收到停止信号，则退出循环
		if (WaitForStop(1000))
			break;

		// 如果有线程出错，则退出循环
		if (WaitForSingleObject(hErrorEvent, 0) == WAIT_OBJECT_0)
			break;
	}

	// 停止 socket 转发

	// 向线程发送停止信号
	SetEvent(hStopEvent);

	for (int i = 0; i < 5; i++)
		Sleep(1000);

	// 终止线程
	TerminateThread(hTransmit, 0);
	TerminateThread(hReceive, 0);

	// 关闭句柄
	CloseHandle(hTransmit);
	CloseHandle(hReceive);
	CloseHandle(hStopEvent);
	CloseHandle(hErrorEvent);

_close_connection:
	OutputString("线程 %08lx TCP 转发结束\n\n", GetCurrentThreadId());

	PeerClose(&peer);
	PeerClose(self);

	if (buff) free(buff);

	return 0;
}

// Socket 数据转发
DWORD WINAPI SocketForward(LPVOID lpParameter)
{
	PSOCKET_FORWARD_INFO sf = (PSOCKET_FORWARD_INFO) lpParameter;
	char buff[2 * BUFF_INCREASEMENT];
	int ret, err, recv_len, send_len, rest_len;
	fd_set readset, writeset;
	TIMEVAL t;
	
	t.tv_sec = 2;
	t.tv_usec = 0;

	// 这是 TCP 单向转发
	while (true)
	{
		// 如果检测到停止信号，则退出循环
		if (WaitForSingleObject(sf->hStopEvent, 0) == WAIT_OBJECT_0)
			break;

		// 读取数据
		FD_ZERO(&readset);
		FD_SET(sf->from->s, &readset);

		// 检测 socket 是否可读
		ret = select(sf->from->s + 1, &readset, NULL, NULL, &t);

		if (ret == SOCKET_ERROR)
		{
			// 发出出错信号
			SetEvent(sf->hErrorEvent);
			return 1;
		}
		
		// 检测超时，不可读
		if (ret == 0)
			continue;

		if (ret > 0)
		{
			if (FD_ISSET(sf->from->s, &readset))
				recv_len = recv(sf->from->s, buff, sizeof (buff), 0);
			else
				continue;
		}

		if (!recv_len)
		{
			// 连接已断开
			sf->from->bReuse = TRUE;
			sf->from->bReset = TRUE;
			SetEvent(sf->hErrorEvent);
			return 0;
		}

		if (recv_len == SOCKET_ERROR)
		{
			err = WSAGetLastError();

			switch (err)
			{
			case WSAECONNABORTED:
			case WSAECONNRESET:
				sf->from->bReuse = TRUE;
				sf->from->bReset = TRUE;
			}

			switch (err)
			{
			case WSAEINTR:
			case WSAEWOULDBLOCK:
				continue;
			default:
				// 发出出错信号
				SetEvent(sf->hErrorEvent);
				return 1;
			}
		}

		// 发送数据
		rest_len = recv_len;

		// 这里相当于一个子循环
_send_retry:
		// 如果检测到停止信号，则退出循环
		if (WaitForSingleObject(sf->hStopEvent, 0) == WAIT_OBJECT_0)
			break;

		if (!rest_len)
			continue;

		FD_ZERO(&writeset);
		FD_SET(sf->to->s, &writeset);
		
		// 检测 socket 是否可写
		ret = select(sf->to->s + 1, NULL, &writeset, NULL, &t);

		if (ret == SOCKET_ERROR)
		{
			// 发出出错信号
			SetEvent(sf->hErrorEvent);
			return 1;
		}

		// 检测超时，不可写
		if (ret == 0)
			goto _send_retry;

		if (ret > 0)
		{
			if (FD_ISSET(sf->to->s, &writeset) && rest_len)
			{
				send_len = send(sf->to->s, buff + recv_len - rest_len, rest_len, 0);

				if (send_len == SOCKET_ERROR)
				{
					err = WSAGetLastError();

					switch (err)
					{
					case WSAECONNABORTED:
					case WSAECONNRESET:
						sf->from->bReuse = TRUE;
						sf->from->bReset = TRUE;
					}

					switch (err)
					{
					case WSAEINTR:
					case WSAEWOULDBLOCK:
						break;
					default:
						// 发出出错信号
						SetEvent(sf->hErrorEvent);
						return 1;
					}
				}
				
				// 数据可能无法一次全部发送
				if (send_len > 0)
					rest_len -= send_len;
			}
			goto _send_retry;
		}
	}

	return 0;
}