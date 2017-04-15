#include "stdafx.h"
#include <MSWSock.h>

// 初始化 Peer 结构
void PeerInit(PSOCKET_PEER peer)
{
	ZeroMemory(peer, sizeof (SOCKET_PEER));
}

// 在指定地址上进行监听
BOOL PeerListen(PSOCKET_PEER peer, const char *localhost, WORD port, int maxqueue)
{
	struct addrinfo hints, *result = NULL;
	char port_str[16];
	int sock_len;
	u_long uNonBlock = 1;
	linger l;

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	// 把端口转换为字符串
	sprintf_s(port_str, sizeof (port_str), "%u", port & 0xffff);

	// 获取要监听的地址信息
	if(getaddrinfo(localhost, port_str, &hints, &result))
	{
		PeerInit(peer);
		return FALSE;
	}

	// 创建 socket
	peer->s = socket(result->ai_family, result->ai_socktype, result->ai_protocol);

	if (peer->s == INVALID_SOCKET)
	{
		PeerInit(peer);
		return FALSE;
	}

	// 绑定监听地址
	if (bind(peer->s, result->ai_addr, (int)result->ai_addrlen) == SOCKET_ERROR)
	{
		closesocket(peer->s);
		PeerInit(peer);
		return FALSE;
	}

	freeaddrinfo(result);

	// 在指定地址上监听
	if (listen(peer->s, maxqueue) == SOCKET_ERROR)
	{
		closesocket(peer->s);
		PeerInit(peer);
		return FALSE;
	}

	peer->listen = TRUE;

	// 设置非阻塞 IO
	ioctlsocket(peer->s, FIONBIO, &uNonBlock);

	l.l_onoff = 1;
	l.l_linger = 10;

	setsockopt(peer->s, SOL_SOCKET, SO_LINGER, (const char *) &l, sizeof (linger));

	// 保存对方域名
	strcpy_s(peer->host, sizeof (peer->host), localhost);

	// 记录自身地址
	sock_len = sizeof (peer->self_addr);
	getsockname(peer->s, (sockaddr *) &peer->self_addr, &sock_len);

	return TRUE;
}

// 接受新连接
int PeerAccept(PSOCKET_PEER peer, SOCKET *s)
{
	int ret;
	fd_set readset;
	TIMEVAL t;
	SOCKET fd;

	if (!peer->listen)
		return SOCKET_ERROR;	

	t.tv_sec = 1;
	t.tv_usec = 0;

	FD_ZERO(&readset);
	FD_SET(peer->s, &readset);

	// 判断 socket 状态
	ret = select(peer->s + 1, &readset, NULL, NULL, &t);

	if (ret == SOCKET_ERROR)
		return SOCKET_ERROR;

	if (ret > 0)
	{
		if (FD_ISSET(peer->s, &readset))
		{
			// socket 可读则表明有新连接
			fd = accept(peer->s, NULL, NULL);
			if (fd == INVALID_SOCKET)
				return SOCKET_ERROR;

			if (s) *s = fd;
			return 1;
		}
	}

	return 0;
}

// 连接到指定地址
BOOL PeerConnect(PSOCKET_PEER peer, const char *host, WORD port)
{
	addrinfo hints, *result = NULL, *ptr = NULL;
	char port_str[16];
	int sock_len, ret, err;
	u_long uNonBlock = 1;
	fd_set readset, writeset;
	linger l;
	TIMEVAL t;
	BOOL bConnFailed = FALSE;
	DWORD dwStartTick = GetTickCount(); 

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	sprintf_s(port_str, sizeof (port_str), "%u", port & 0xffff);

	if (getaddrinfo(host, port_str, &hints, &result))
	{
		closesocket(peer->s);
		PeerInit(peer);
		return FALSE;
	}

	for (ptr = result; ptr != NULL; ptr = ptr->ai_next)
	{
		bConnFailed = FALSE;
		peer->s = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);

		if (peer->s == INVALID_SOCKET)
		{
			PeerInit(peer);
			return FALSE;
		}

		// 设置非阻塞 IO
		ioctlsocket(peer->s, FIONBIO, &uNonBlock);

_connect_retry:
		ret = connect(peer->s, ptr->ai_addr, (int)ptr->ai_addrlen);

		if (ret == 0)
			break;

		err = WSAGetLastError();

		switch (err)
		{
		case WSAEINTR:
		case WSAEWOULDBLOCK:
		case WSAEINPROGRESS:
			// connect 正在进行
			break;
		case WSAEALREADY:
		case WSAEISCONN:
		case WSAEINVAL: // Windows 2003 上的奇怪问题。。
			// connect 已成功
			goto _connected;
		default:
			// 当前地址的 connect 失败
			closesocket(peer->s);
			peer->s = INVALID_SOCKET;
			bConnFailed = TRUE;
			break;
		}

		if (bConnFailed)
			continue;
		
		t.tv_sec = 1;
		t.tv_usec = 0;

		do
		{
			FD_ZERO(&readset);
			FD_ZERO(&writeset);
			FD_SET(peer->s, &readset);
			FD_SET(peer->s, &writeset);

			ret = select(peer->s + 1, &readset, &writeset, NULL, &t);

			if (ret == SOCKET_ERROR)
			{
				closesocket(peer->s);
				peer->s = INVALID_SOCKET;
				bConnFailed = TRUE;
				break;
			}

			if (ret > 0)
			{
				// 如果 socket 可写，则表明 connect 已成功，需要再次用 connect 函数去判断
				// 如果 socket 可读，则也去尝试
				if (FD_ISSET(peer->s, &readset) || FD_ISSET(peer->s, &writeset))
					goto _connect_retry;
			}

			if (GetTickCount() - dwStartTick >= 30000)
			{
				closesocket(peer->s);
				peer->s = INVALID_SOCKET;
				bConnFailed = TRUE;
				break;
			}
		} while (true);

		if (bConnFailed)
			continue;
	}

_connected:
	freeaddrinfo(result);

	if (peer->s == INVALID_SOCKET)
	{
		PeerInit(peer);
		return FALSE;
	}

	// 保存对方域名
	strcpy_s(peer->host, sizeof (peer->host), host);

	// 记录自身地址
	sock_len = sizeof (peer->self_addr);
	getsockname(peer->s, (sockaddr *) &peer->self_addr, &sock_len);

	// 记录对方地址
	sock_len = sizeof (peer->peer_addr);
	getpeername(peer->s, (sockaddr *) &peer->peer_addr, &sock_len);
	
	l.l_onoff = 1;
	l.l_linger = 10;

	setsockopt(peer->s, SOL_SOCKET, SO_LINGER, (const char *) &l, sizeof (linger));

	return TRUE;
}

// 附加套接字
BOOL PeerAttach(PSOCKET_PEER peer, SOCKET s)
{
	int sock_len, ret;
	u_long uNonBlock = 1;
	linger l;

	peer->s = s;

	// 记录自身地址
	sock_len = sizeof (peer->self_addr);
	ret = getsockname(peer->s, (sockaddr *) &peer->self_addr, &sock_len);

	if (ret == SOCKET_ERROR)
	{
		PeerInit(peer);
		return FALSE;
	}

	// 记录对方地址
	sock_len = sizeof (peer->peer_addr);
	getpeername(peer->s, (sockaddr *) &peer->peer_addr, &sock_len);

	if (ret == SOCKET_ERROR)
	{
		PeerInit(peer);
		return FALSE;
	}

	peer->listen = false;

	// 设置非阻塞 IO
	ioctlsocket(peer->s, FIONBIO, &uNonBlock);
	
	l.l_onoff = 1;
	l.l_linger = 10;

	setsockopt(peer->s, SOL_SOCKET, SO_LINGER, (const char *) &l, sizeof (linger));

	strcpy_s(peer->host, sizeof(peer->host), inet_ntoa(peer->peer_addr.sin_addr));

	return TRUE;
}

// 关闭连接
void PeerClose(PSOCKET_PEER peer)
{
	linger l;
	int tmp, len;

	if ((!peer->s) || (peer->s == INVALID_SOCKET))
		return;

	// 通知不再发送数据
	shutdown(peer->s, SD_SEND);
	
	// 等待剩余数据接收结束
	do
	{
		if (!PeerReceive(peer, &tmp, 1, &len))
			break;
	} while (len);

	if (peer->bReset)
	{
		// 以 RST 方式关闭连接
		l.l_onoff = 1;
		l.l_linger = 0;
		setsockopt(peer->s, SOL_SOCKET, SO_LINGER, (const char *) &l, sizeof (linger));
	}

	if (peer->bReuse)
	{
		LPFN_DISCONNECTEX DisconnectEx;
		GUID id = WSAID_DISCONNECTEX;
		DWORD dwBytesReturned;

		if (!WSAIoctl(peer->s, SIO_GET_EXTENSION_FUNCTION_POINTER, &id, sizeof (id),
			&DisconnectEx, sizeof (DisconnectEx), &dwBytesReturned, NULL, NULL))
			DisconnectEx(peer->s, NULL, TF_REUSE_SOCKET, 0);
	}

	closesocket(peer->s);

	PeerInit(peer);
}

// 判断连接是否已断开
BOOL PeerIsClosed(PSOCKET_PEER peer)
{
	fd_set readset;
	TIMEVAL t;
	int ret;
	BOOL bClosed = FALSE;

	// 使用 Peek 能接收到数据，表明连接未断开
	if (recv(peer->s, (char *) &ret, 1, MSG_PEEK) > 0)
		return FALSE;

	FD_ZERO(&readset);
	FD_SET(peer->s, &readset);

	t.tv_sec = 0;
	t.tv_usec = 10;

	// 检测 socket 是否可读
	ret = select(peer->s + 1, &readset, NULL, NULL, &t);

	if (ret == 0)
		return FALSE;

	if (ret < 0)
		return TRUE;

	if (FD_ISSET(peer->s, &readset))
	{
		// 临时恢复为阻塞模式
		u_long uNonBlock = 0;
		ioctlsocket(peer->s, FIONBIO, &uNonBlock);

		ret = recv(peer->s, (char *) &ret, 1, MSG_PEEK);

		// ret 大于零表示有数据可接收
		if (ret <= 0)
		{
			// 在阻塞模式下使用 recv
			ret = recv(peer->s, (char *) &ret, 1, 0);

			// 0 表示连接已断开
			if (ret == 0)
			{
				peer->bReuse = TRUE;
				bClosed = TRUE;
			}

			if (ret < 0)
			{
				int err = WSAGetLastError();

				switch (err)
				{
				case WSAECONNABORTED:
				case WSAECONNRESET:
					peer->bReuse = TRUE;
				}

				switch (err)
				{
				case WSAECONNABORTED:
				case WSAECONNRESET:
				case WSAENOTCONN:
				case WSAETIMEDOUT:
					bClosed = TRUE;
				}
			}
		}

		// 恢复非阻塞模式
		uNonBlock = 1;
		ioctlsocket(peer->s, FIONBIO, &uNonBlock);
	}

	return bClosed;
}

// 发送数据
BOOL PeerSend(PSOCKET_PEER peer, void *data, int length, int *slength)
{
	char *buf = (char *) data;
	int sent = 0, len, ret, err;
	fd_set writeset;
	TIMEVAL t;
	
	t.tv_sec = 0;
	t.tv_usec = 50;

	while (sent < length)
	{
		FD_ZERO(&writeset);
		FD_SET(peer->s, &writeset);
		
		// 检测 socket 是否可写
		ret = select(peer->s + 1, NULL, &writeset, NULL, &t);

		if (ret == SOCKET_ERROR)
		{
			if (slength) *slength = sent;
			return FALSE;
		}

		if (ret == 0)
			continue;

		if (!FD_ISSET(peer->s, &writeset))
			continue;

		len = send(peer->s, buf + sent, length - sent, 0);

		if (len > 0)
			sent += len;

		if (len == SOCKET_ERROR)
		{
			err = WSAGetLastError();

			switch (err)
			{
			case WSAECONNABORTED:
			case WSAECONNRESET:
				peer->bReuse = TRUE;
			}

			switch (err)
			{
			case WSAEINTR:
			case WSAEWOULDBLOCK:
				break;
			default:
				if (slength) *slength = sent;
				return FALSE;
			}
		}
	}

	if (slength) *slength = sent;
	return TRUE;
}

// 接收数据
BOOL PeerReceive(PSOCKET_PEER peer, void *data, int length, int *rlength)
{
	char *buf = (char *) data;
	int total = 0, recv_len, ret, err;
	fd_set readset;
	TIMEVAL t;
	
	t.tv_sec = 0;
	t.tv_usec = 50;

	while (total < length)
	{
		FD_ZERO(&readset);
		FD_SET(peer->s, &readset);
		
		// 检测 socket 是否可读
		ret = select(peer->s + 1, &readset, NULL, NULL, &t);

		if (ret == SOCKET_ERROR)
		{
			if (rlength) *rlength = total;
			return FALSE;
		}

		if (ret == 0)
			break;

		if (!FD_ISSET(peer->s, &readset))
			continue;

		recv_len = recv(peer->s, buf + total, length - total, 0);

		if (recv_len > 0)
			total += recv_len;

		// 连接已关闭
		if (recv_len == 0)
		{
			peer->bReuse = TRUE;
			if (rlength) *rlength = total;
			return TRUE;
		}

		if (recv_len == SOCKET_ERROR)
		{
			err = WSAGetLastError();

			switch (err)
			{
			case WSAECONNABORTED:
			case WSAECONNRESET:
				peer->bReuse = TRUE;
			}

			switch (err)
			{
			case WSAEINTR:
			case WSAEWOULDBLOCK:
				break;
			default:
				if (rlength) *rlength = total;
				return FALSE;
			}
		}
	}

	if (rlength) *rlength = total;
	return TRUE;
}

// 读取待接收数据，但不将其移出待接收队列
BOOL PeerPeek(PSOCKET_PEER peer, void *data, int length, int *rlength)
{
	char *buf = (char *) data;
	int total = 0, recv_len, ret, err;
	fd_set readset;
	TIMEVAL t;
	
	t.tv_sec = 0;
	t.tv_usec = 50;

	while (true)
	{
		FD_ZERO(&readset);
		FD_SET(peer->s, &readset);
		
		// 检测 socket 是否可读
		ret = select(peer->s + 1, &readset, NULL, NULL, &t);

		if (ret == SOCKET_ERROR)
		{
			if (rlength) *rlength = 0;
			return FALSE;
		}

		if (ret == 0)
			break;

		if (!FD_ISSET(peer->s, &readset))
			continue;

		break;
	}

	recv_len = recv(peer->s, buf, length, MSG_PEEK);

	if (recv_len >= 0)
	{
		if (rlength) *rlength = recv_len;
		return TRUE;
	}

	if (recv_len == SOCKET_ERROR)
	{
		err = WSAGetLastError();

		switch (err)
		{
		case WSAECONNABORTED:
		case WSAECONNRESET:
			peer->bReuse = TRUE;
		}

		switch (err)
		{
		case WSAEINTR:
		case WSAEWOULDBLOCK:
			return TRUE;
		}
	}
	
	return FALSE;
}