#include "stdafx.h"

BOOL DoHttpProxy(PSOCKET_PEER peer, PSOCKET_PEER self, CStringA &request, const char *post_data, int post_first_len, int post_length);
BOOL DoProxyNormal(PSOCKET_PEER peer, PSOCKET_PEER self, const char *first_block, int first_len, int content_length);
BOOL DoProxyChunked(PSOCKET_PEER peer, PSOCKET_PEER self, const char *first_block, int first_len);

static const char content_length_str[] = "Content-Length";
static const char connection_str[] = "Connection";
static const char keep_alive_str[] = "Keep-Alive";
static const char transfer_encoding_str[] = "Transfer-Encoding";
static const char chunked_str[] = "chunked";

DWORD WINAPI HttpProxyRequest(PSOCKET_PEER self)
{
	int len = 0, total = 0, recv_len;
	char *buff = NULL, *hdr_end;
	char *entry_start, *method, *url;
	char *path, host[256], last_host[256], *key, *value;
	char *post_data = NULL;
	char auth[1024];
	WORD wHttpVer, wPort, wLastPort;
	BOOL bCloseConnection = TRUE;
	int post_length = 0, post_recv_len = 0;
	CStringA request, path_to_print;
	SOCKET_PEER peer;
	DWORD dw64StartTick;

	PeerInit(&peer);
	ZeroMemory(host, sizeof (host));
	ZeroMemory(last_host, sizeof (last_host));
	ZeroMemory(auth, sizeof (auth));
	wLastPort = 0;

_restart:
	total = 0;
	post_length = 0;

	if (len && buff)
		ZeroMemory(buff, len);

	dw64StartTick = GetTickCount();

	// 读取报头数据
	while (true)
	{
		if (WaitForStop(10))
		{
			self->bReset = TRUE;
			goto _close_connection;
		}

		// 扩充缓冲区
		if (total + 1 >= len)
		{
			len += BUFF_INCREASEMENT;
			buff = (char *) realloc(buff, len + 1);
		}

		// 接收请求报头
		if (!PeerReceive(self, buff + total, len - total, &recv_len))
		{
			self->bReset = TRUE;
			goto _close_connection;
		}

		if (recv_len > 0)
		{
			total += recv_len;
			buff[total] = 0;
		}

		if (PeerIsClosed(self))
		{
			self->bReset = TRUE;
			goto _close_connection;
		}

		// 已读完报头
		if (hdr_end = strstr(buff, "\r\n\r\n"))
			break;

		// 连接超时
		if (GetTickCount() - dw64StartTick > dwConnectCheckTimeout * 1000)
		{
			self->bReset = TRUE;
			goto _close_connection;
		}

		Sleep(50);
	}

	hdr_end += 4;

	// 分析报头第一行 （方法、完整的 URL、HTTP 版本）
	if (!RequestParseURL(buff, &entry_start, &method, &url, &wHttpVer))
	{
		// 报文格式错误。。终止连接
		HttpResponse(self, 400, wHttpVer, "Wrong HTTP header format.");
		self->bReset = TRUE;
		goto _close_connection;
	}

	// 分析 URL 的主机、端口和请求路径
	if (!RequestParseHost(url, host, &path, &wPort))
	{
		// URL 格式不合法
		HttpResponse(self, 400, wHttpVer, "Wrong HTTP header format.");
		self->bReset = TRUE;
		goto _close_connection;
	}

	// 这是重构的 HTTP 请求报头
	request.Format("%s %s HTTP/%u.%u\r\n", method, path, HIBYTE(wHttpVer), LOBYTE(wHttpVer));

	// 分析报头条目
	while (RequestParseEntry(entry_start, &entry_start, &key, &value))
	{
		if (*host == 0)
		{
			if (!strcmp(key, "Host"))
			{
				strcpy_s(host, sizeof (host), value);
			}
			goto _next_entry;
		}

		if (!strcmp(key, "Proxy-Connection"))
		{
			// Proxy-Connection 不发送给实际服务器
			if (!_stricmp(value, "Keep-Alive"))
				bCloseConnection = FALSE;
			
			// 改为 Connection
			key = "Connection";
			goto _next_entry;
		}

		// POST 发送的数据长度
		if (!strcmp(key, "Content-Length"))
		{
			post_length = strtoul(value, NULL, 10);
			goto _next_entry;
		}

		// 认证头部
		if (!strcmp(key, "Proxy-Authorization"))
		{
			StringCchCopyA(auth, sizeof (auth), value);
			goto _next_entry;
		}

_next_entry:
		request += key;
		request += ":\x20";
		request += value;
		request += "\r\n";
	}

	request += "\r\n";

	OutputString("线程 %08lx 连接：%s:%u，协议 HTTP/%u.%u\n", GetCurrentThreadId(), host, wPort & 0xffff, HIBYTE(wHttpVer), LOBYTE(wHttpVer));
	OutputString("%s ", method);
	OutputStringDirect(path);
	if (post_length)
		OutputString("提交长度: %d\n", post_length);
	OutputStringDirect("\n");

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

		if (post_length)
		{
			char data[1024];

			post_data = hdr_end;
			post_recv_len = total - (size_t) (hdr_end - buff);
			post_length -= post_recv_len;

			while (post_length)
			{
				if (!PeerReceive(self, data, min(sizeof (data), post_length), &recv_len))
				{
					OutputString("线程 %08lx 读取无效数据失败\n\n", GetCurrentThreadId());
					goto _close_connection;
				}
				post_length -= recv_len;
			}
		}

		goto _restart;
	}

	if (strcmp(host, last_host) || (wPort != wLastPort))
		PeerClose(&peer);

	// 连接到目标服务器
	if (!PeerConnect(&peer, host, wPort))
	{
		HttpResponse(self, 503, wHttpVer, "Connection failed! WSAError = %u", WSAGetLastError());
		self->bReset = TRUE;
		goto _close_connection;
	}

	if (post_length)
	{
		post_data = hdr_end;
		post_recv_len = total - (size_t) (hdr_end - buff);
	}

	if (!DoHttpProxy(&peer, self, request, post_data, post_recv_len, post_length))
	{
		bCloseConnection = TRUE;
		goto _close_connection;
	}

	request = "";

	if (!bCloseConnection)
	{
		wLastPort = wPort;
		memcpy(last_host, host, sizeof (last_host));
		goto _restart;
	}

_close_connection:

	OutputString("线程 %08lx 关闭连接\n\n", GetCurrentThreadId());

	PeerClose(&peer);
	PeerClose(self);

	if (buff) free(buff);

	return 0;
}

BOOL DoHttpProxy(PSOCKET_PEER peer, PSOCKET_PEER self, CStringA &request, const char *post_data, int post_first_len, int post_length)
{
	int total = 0, len = 0, recv_len, sent_len;
	int header_len, post_len_rest = 0;
	char *buff = NULL;
	char post_buff[4096];

	char *hdr_end, *p;
	size_t content_length = 0, content_recv = 0;
	BOOL bCloseConnection = TRUE;
	BOOL bChunkedMode = FALSE;
	char rep_hdr[1024];

	// 发送请求报头
	if (!PeerSend(peer, (void *) request.operator LPCSTR(), request.Length, &sent_len))
	{
		peer->bReset = TRUE;
		return FALSE;
	}

	if (sent_len != request.Length)
	{
		peer->bReset = TRUE;
		return FALSE;
	}

	// 如果是 POST 请求，则发送 POST 的数据
	if (post_data && post_first_len && post_length)
	{
		// POST 数据第一块
		if (!PeerSend(peer, (void *) post_data, post_first_len, &sent_len))
		{
			peer->bReset = TRUE;
			return FALSE;
		}

		if (sent_len != post_first_len)
		{
			peer->bReset = TRUE;
			return FALSE;
		}

		// POST 剩余数据长度
		post_len_rest = post_length - post_first_len;

		while (post_len_rest > 0)
		{
			if (WaitForStop(10))
			{
				self->bReset = TRUE;
				peer->bReset = TRUE;
				return FALSE;
			}

			// 接收浏览发送的 POST 请求数据
			if (!PeerReceive(self, post_buff, post_len_rest > BUFF_INCREASEMENT ? BUFF_INCREASEMENT : post_len_rest, &recv_len))
			{
				OutputString("客户端 fd = %u\n接收剩余上传数据错误 %d\n\n", self->s, WSAGetLastError());
				self->bReset = TRUE;
				return FALSE;
			}

			// 与浏览器的连接已断开，则停止代理
			if (PeerIsClosed(self))
			{
				self->bReset = TRUE;
				return FALSE;
			}

			// 将接收的数据发送给目标服务器 
			if (!PeerSend(peer, post_buff, recv_len, &sent_len))
			{
				OutputString("客户端 fd = %u\n发送剩余上传数据错误 %d\n\n", self->s, WSAGetLastError());
				peer->bReset = TRUE;
				return FALSE;
			}

			if (recv_len != sent_len)
			{
				OutputString("客户端 fd = %u\n返回剩余数据长度错误 %d\n\n", self->s, WSAGetLastError());
				peer->bReset = TRUE;
				return FALSE;
			}

			// 与目标服务器的连接断开，则停止代理
			if (PeerIsClosed(peer))
			{
				peer->bReset = TRUE;
				return FALSE;
			}

			post_len_rest -= recv_len;
		}
	}

	// 接收响应数据并分析
	while (true)
	{
		if (WaitForStop(50))
		{
			self->bReset = TRUE;
			peer->bReset = TRUE;
			return FALSE;
		}

		// 如果当前的缓冲区容量不足，则重新分配
		if (total + 1 >= len)
		{
			len += BUFF_INCREASEMENT;
			buff = (char *) realloc(buff, len);
		}

		// 从目标服务器接收响应数据
		if (!PeerReceive(peer, buff + total, len - total - 1, &recv_len))
		{
			bCloseConnection = TRUE;
			peer->bReset = TRUE;
			goto _abort;
		}

		total += recv_len;
		buff[total] = 0;

		// 已读完报头
		if (hdr_end = strstr(buff, "\r\n\r\n"))
			break;

		// 没接收完报头，目标服务器就断开了连接，则停止代理
		if (PeerIsClosed(peer))
		{
			bCloseConnection = TRUE;
			peer->bReset = TRUE;
			goto _abort;
		}
	}

	hdr_end += 4;
	p = buff;

	// 获取内容长度
	while (p < hdr_end)
	{
		// 获取响应报文的内容长度
		if (!strncmp(p, content_length_str, sizeof (content_length_str) - 1))
		{
			p += sizeof (content_length_str) - 1;
			if (*p == ':') p++;
			while (*p == '\x20' || *p == '\t') p++;

			content_length = strtoul(p, NULL, 10);

			goto _entry_next;
		}

		// 获取连接状态：保持还是关闭
		if (!strncmp(p, connection_str, sizeof (connection_str) - 1))
		{
			p += sizeof (connection_str) - 1;
			if (*p == ':') p++;
			while (*p == '\x20' || *p == '\t') p++;

			if (!_strnicmp(p, keep_alive_str, sizeof (keep_alive_str) - 1))
				bCloseConnection = FALSE;

			goto _entry_next;
		}

		// 获取报文数据传输方式：已知长度的整块数据还是未知长度的分块数据
		if (!strncmp(p, transfer_encoding_str, sizeof (transfer_encoding_str) - 1))
		{
			p += sizeof (transfer_encoding_str) - 1;
			if (*p == ':') p++;
			while (*p == '\x20' || *p == '\t') p++;

			if (!_strnicmp(p, chunked_str, sizeof (chunked_str) - 1))
				bChunkedMode = TRUE;

			goto _entry_next;
		}

_entry_next:
		// 下一行
		p = strstr(p, "\r\n");
		if (p)
		{
			p += 2;
			continue;
		}

		break;
	}

	// 提取响应报头第一行（纯粹用于调试）
	p = strstr(buff, "\r\n");
	strncpy(rep_hdr, buff, (size_t) (p - buff));
	rep_hdr[(size_t) (p - buff)] = 0;

	OutputString("线程 %08lx 远程服务端回应\n%s\n", GetCurrentThreadId(), rep_hdr);
	if (!bChunkedMode)
		OutputString("返回长度 %uB\n", content_length);
	else
		OutputString("Chunked 模式\n");
	OutputString("\n");

	// 报头长度
	header_len = (size_t) (hdr_end - buff);

	// 发送报头
	if (!PeerSend(self, buff, header_len, &sent_len))
	{
		OutputString("客户端 fd = %u\n返回报头错误 %d\n\n", self->s, WSAGetLastError());
		bCloseConnection = TRUE;
		self->bReset = TRUE;
		goto _abort;
	}

	if (sent_len != header_len)
	{
		OutputString("客户端 fd = %u\n返回报头长度错误 %d\n\n", self->s, WSAGetLastError());
		bCloseConnection = TRUE;
		self->bReset = TRUE;
		goto _abort;
	}

	// 报文数据起始地址
	p = hdr_end;

	// 如果是非 Chunked 模式
	if (!bChunkedMode)
	{
		// 处理报文数据
		if (!DoProxyNormal(peer, self, p, total - header_len, content_length))
			bCloseConnection = FALSE;

		goto _finish;
	}

	// 处理 Chunked 数据
	if (!DoProxyChunked(peer, self, p, total - header_len))
	{
		bCloseConnection = FALSE;
	}

_finish:
	OutputString("线程 %08lx 完成%s\n\n", GetCurrentThreadId(), bCloseConnection ? "" : ", Keep-Alive");

_abort:
	if (buff) free(buff);
	return !bCloseConnection;
}

// 常规报文数据处理
BOOL DoProxyNormal(PSOCKET_PEER peer, PSOCKET_PEER self, const char *first_block, int first_len, int content_length)
{
	char buff[BUFF_INCREASEMENT];
	int sent_len, recv_len, rest;

	// 只有报头，没有报文，直接退出
	if (!content_length)
		return TRUE;

	// 如果已经读取了部分报文数据，则将其发送
	if (first_len)
	{
		if (!PeerSend(self, (void *) first_block, first_len, &sent_len))
		{
			OutputString("客户端 fd = %u\n返回第一部分数据错误 %d\n\n", self->s, WSAGetLastError());
			self->bReset = TRUE;
			return FALSE;
		}

		if (sent_len != first_len)
		{
			OutputString("客户端 fd = %u\n返回第一部分数据长度错误 %d\n\n", self->s, WSAGetLastError());
			self->bReset = TRUE;
			return FALSE;
		}
	}

	// 剩余数据长度
	rest = content_length - first_len;

	while (rest)
	{
		if (WaitForStop(10))
		{
			self->bReset = TRUE;
			peer->bReset = TRUE;
			return FALSE;
		}

		// 从目标服务器接收报文数据
		if (!PeerReceive(peer, buff, rest > BUFF_INCREASEMENT ? BUFF_INCREASEMENT : rest, &recv_len))
		{
			OutputString("客户端 fd = %u\n接收剩余数据错误 %d\n\n", self->s, WSAGetLastError());
			peer->bReset = TRUE;
			return FALSE;
		}

		// 发送给浏览器
		if (!PeerSend(self, buff, recv_len, &sent_len))
		{
			OutputString("客户端 fd = %u\n返回剩余数据错误 %d\n\n", self->s, WSAGetLastError());
			self->bReset = TRUE;
			return FALSE;
		}

		if (recv_len != sent_len)
		{
			OutputString("客户端 fd = %u\n返回剩余数据长度错误 %d\n\n", self->s, WSAGetLastError());
			self->bReset = TRUE;
			return FALSE;
		}
		
		// 与浏览器的连接断开，则停止代理
		if (PeerIsClosed(self))
		{
			self->bReset = TRUE;
			return FALSE;
		}

		// 与目标服务器的连接断开，表示数据已读取完成
		if (PeerIsClosed(peer))
			break;

		rest -= recv_len;
	}

	return TRUE;
}

// 分块数据处理
BOOL DoProxyChunked(PSOCKET_PEER peer, PSOCKET_PEER self, const char *first_block, int first_len)
{
	char buff[BUFF_INCREASEMENT], *buff_end = buff + sizeof (buff);
	char *p, *d;
	int buf_len, recv_len, sent_len;
	int chunk_len, rest_len;

	// 第一块数据长度不应该大于 BUFF_INCREASEMENT
	if (first_len > BUFF_INCREASEMENT)
		return FALSE;

	// 将已经读取的报文数据复制到缓冲区
	memcpy(buff, first_block, first_len);
	buf_len = first_len;

	do
	{
		if (WaitForStop(10))
		{
			self->bReset = TRUE;
			peer->bReset = TRUE;
			return FALSE;
		}

		// 尝试填充缓冲区剩余部分
		if (!PeerReceive(peer, (void *) (buff + buf_len), sizeof (buff) - buf_len, &recv_len))
		{
			OutputString("客户端 fd = %u\n尝试接收剩余数据错误 %d\n\n", self->s, WSAGetLastError());
			self->bReset = TRUE;
			return FALSE;
		}

		buf_len += recv_len;

		// 与目标服务器的连接断开，表示数据可能已读取完成
		if (PeerIsClosed(peer))
			break;
	} while (!recv_len);

	p = buff;

	// 循环获取每个块的长度及发送块数据
	while (true)
	{
		if (WaitForStop(10))
		{
			self->bReset = TRUE;
			peer->bReset = TRUE;
			return FALSE;
		}

		// 获取块长度
		chunk_len = strtoul(p, &d, 16);

		// 跳过可能存在的空格
		while (*d == '\x20') p++;

		// 紧跟一个换行符
		if (strncmp(d, "\r\n", 2))
		{
			peer->bReset = TRUE;
			return FALSE;
		}

		d +=2;

		// 当前块的长度（包含指明块长度的数据）
		rest_len = chunk_len + (size_t) (d - p) + 2;
		while (rest_len)
		{
			if (WaitForStop(10))
			{
				self->bReset = TRUE;
				peer->bReset = TRUE;
				return FALSE;
			}

			// 将数据发送给浏览器
			if (!PeerSend(self, (void *)p, rest_len > buf_len ? buf_len : rest_len, &sent_len))
			{
				OutputString("客户端 fd = %u\n尝试发送剩余数据错误 %d\n\n", self->s, WSAGetLastError());
				self->bReset = TRUE;
				return FALSE;
			}

			if (sent_len)
			{
				rest_len -= sent_len;
				buf_len -= sent_len;
				p += sent_len;
			}

			if ((rest_len) && (!buf_len))
			{
				// 缓冲区里没有数据了，重新读取
				do
				{
					if (WaitForStop(10))
					{
						self->bReset = TRUE;
						peer->bReset = TRUE;
						return FALSE;
					}

					// 将数据读取到缓冲区起始位置
					if (!PeerReceive(peer, (void *) buff, sizeof (buff), &recv_len))
					{
						OutputString("客户端 fd = %u\n尝试接收剩余数据错误 %d\n\n", self->s, WSAGetLastError());
						peer->bReset = TRUE;
						return FALSE;
					}

					// 将当前数据指针移动到缓冲区起始位置
					p = buff;
					buf_len += recv_len;

					// 与目标服务器的连接断开，表示数据可能已读取完成
					if (PeerIsClosed(peer))
						break;
				} while (!recv_len);
			}
		}

		// 如果块长度为 0，表示已读取完所有块数据
		if (!chunk_len)
			break;

		// 缓冲区剩余数据太少，就将其移至缓冲区最前面
		if (buf_len < 16)
		{
			memcpy(buff, p, buf_len);
			p = buff;

			do
			{
				if (WaitForStop(10))
				{
					self->bReset = TRUE;
					peer->bReset = TRUE;
					return FALSE;
				}

				// 尝试填充缓冲区剩余部分
				if (!PeerReceive(peer, (void *) (buff + buf_len), sizeof (buff) - buf_len, &recv_len))
				{
					OutputString("客户端 fd = %u\n尝试接收剩余数据错误 %d\n\n", self->s, WSAGetLastError());
					peer->bReset = TRUE;
					return FALSE;
				}

				buf_len += recv_len;

				// 与目标服务器的连接断开，表示数据可能已读取完成
				if (PeerIsClosed(peer))
					break;
			} while (buf_len < 5);
		}
	}

	return TRUE;
}