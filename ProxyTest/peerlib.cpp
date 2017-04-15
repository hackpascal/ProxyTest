#include "stdafx.h"
#include <MSWSock.h>

// ��ʼ�� Peer �ṹ
void PeerInit(PSOCKET_PEER peer)
{
	ZeroMemory(peer, sizeof (SOCKET_PEER));
}

// ��ָ����ַ�Ͻ��м���
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

	// �Ѷ˿�ת��Ϊ�ַ���
	sprintf_s(port_str, sizeof (port_str), "%u", port & 0xffff);

	// ��ȡҪ�����ĵ�ַ��Ϣ
	if(getaddrinfo(localhost, port_str, &hints, &result))
	{
		PeerInit(peer);
		return FALSE;
	}

	// ���� socket
	peer->s = socket(result->ai_family, result->ai_socktype, result->ai_protocol);

	if (peer->s == INVALID_SOCKET)
	{
		PeerInit(peer);
		return FALSE;
	}

	// �󶨼�����ַ
	if (bind(peer->s, result->ai_addr, (int)result->ai_addrlen) == SOCKET_ERROR)
	{
		closesocket(peer->s);
		PeerInit(peer);
		return FALSE;
	}

	freeaddrinfo(result);

	// ��ָ����ַ�ϼ���
	if (listen(peer->s, maxqueue) == SOCKET_ERROR)
	{
		closesocket(peer->s);
		PeerInit(peer);
		return FALSE;
	}

	peer->listen = TRUE;

	// ���÷����� IO
	ioctlsocket(peer->s, FIONBIO, &uNonBlock);

	l.l_onoff = 1;
	l.l_linger = 10;

	setsockopt(peer->s, SOL_SOCKET, SO_LINGER, (const char *) &l, sizeof (linger));

	// ����Է�����
	strcpy_s(peer->host, sizeof (peer->host), localhost);

	// ��¼�����ַ
	sock_len = sizeof (peer->self_addr);
	getsockname(peer->s, (sockaddr *) &peer->self_addr, &sock_len);

	return TRUE;
}

// ����������
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

	// �ж� socket ״̬
	ret = select(peer->s + 1, &readset, NULL, NULL, &t);

	if (ret == SOCKET_ERROR)
		return SOCKET_ERROR;

	if (ret > 0)
	{
		if (FD_ISSET(peer->s, &readset))
		{
			// socket �ɶ��������������
			fd = accept(peer->s, NULL, NULL);
			if (fd == INVALID_SOCKET)
				return SOCKET_ERROR;

			if (s) *s = fd;
			return 1;
		}
	}

	return 0;
}

// ���ӵ�ָ����ַ
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

		// ���÷����� IO
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
			// connect ���ڽ���
			break;
		case WSAEALREADY:
		case WSAEISCONN:
		case WSAEINVAL: // Windows 2003 �ϵ�������⡣��
			// connect �ѳɹ�
			goto _connected;
		default:
			// ��ǰ��ַ�� connect ʧ��
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
				// ��� socket ��д������� connect �ѳɹ�����Ҫ�ٴ��� connect ����ȥ�ж�
				// ��� socket �ɶ�����Ҳȥ����
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

	// ����Է�����
	strcpy_s(peer->host, sizeof (peer->host), host);

	// ��¼�����ַ
	sock_len = sizeof (peer->self_addr);
	getsockname(peer->s, (sockaddr *) &peer->self_addr, &sock_len);

	// ��¼�Է���ַ
	sock_len = sizeof (peer->peer_addr);
	getpeername(peer->s, (sockaddr *) &peer->peer_addr, &sock_len);
	
	l.l_onoff = 1;
	l.l_linger = 10;

	setsockopt(peer->s, SOL_SOCKET, SO_LINGER, (const char *) &l, sizeof (linger));

	return TRUE;
}

// �����׽���
BOOL PeerAttach(PSOCKET_PEER peer, SOCKET s)
{
	int sock_len, ret;
	u_long uNonBlock = 1;
	linger l;

	peer->s = s;

	// ��¼�����ַ
	sock_len = sizeof (peer->self_addr);
	ret = getsockname(peer->s, (sockaddr *) &peer->self_addr, &sock_len);

	if (ret == SOCKET_ERROR)
	{
		PeerInit(peer);
		return FALSE;
	}

	// ��¼�Է���ַ
	sock_len = sizeof (peer->peer_addr);
	getpeername(peer->s, (sockaddr *) &peer->peer_addr, &sock_len);

	if (ret == SOCKET_ERROR)
	{
		PeerInit(peer);
		return FALSE;
	}

	peer->listen = false;

	// ���÷����� IO
	ioctlsocket(peer->s, FIONBIO, &uNonBlock);
	
	l.l_onoff = 1;
	l.l_linger = 10;

	setsockopt(peer->s, SOL_SOCKET, SO_LINGER, (const char *) &l, sizeof (linger));

	strcpy_s(peer->host, sizeof(peer->host), inet_ntoa(peer->peer_addr.sin_addr));

	return TRUE;
}

// �ر�����
void PeerClose(PSOCKET_PEER peer)
{
	linger l;
	int tmp, len;

	if ((!peer->s) || (peer->s == INVALID_SOCKET))
		return;

	// ֪ͨ���ٷ�������
	shutdown(peer->s, SD_SEND);
	
	// �ȴ�ʣ�����ݽ��ս���
	do
	{
		if (!PeerReceive(peer, &tmp, 1, &len))
			break;
	} while (len);

	if (peer->bReset)
	{
		// �� RST ��ʽ�ر�����
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

// �ж������Ƿ��ѶϿ�
BOOL PeerIsClosed(PSOCKET_PEER peer)
{
	fd_set readset;
	TIMEVAL t;
	int ret;
	BOOL bClosed = FALSE;

	// ʹ�� Peek �ܽ��յ����ݣ���������δ�Ͽ�
	if (recv(peer->s, (char *) &ret, 1, MSG_PEEK) > 0)
		return FALSE;

	FD_ZERO(&readset);
	FD_SET(peer->s, &readset);

	t.tv_sec = 0;
	t.tv_usec = 10;

	// ��� socket �Ƿ�ɶ�
	ret = select(peer->s + 1, &readset, NULL, NULL, &t);

	if (ret == 0)
		return FALSE;

	if (ret < 0)
		return TRUE;

	if (FD_ISSET(peer->s, &readset))
	{
		// ��ʱ�ָ�Ϊ����ģʽ
		u_long uNonBlock = 0;
		ioctlsocket(peer->s, FIONBIO, &uNonBlock);

		ret = recv(peer->s, (char *) &ret, 1, MSG_PEEK);

		// ret �������ʾ�����ݿɽ���
		if (ret <= 0)
		{
			// ������ģʽ��ʹ�� recv
			ret = recv(peer->s, (char *) &ret, 1, 0);

			// 0 ��ʾ�����ѶϿ�
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

		// �ָ�������ģʽ
		uNonBlock = 1;
		ioctlsocket(peer->s, FIONBIO, &uNonBlock);
	}

	return bClosed;
}

// ��������
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
		
		// ��� socket �Ƿ��д
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

// ��������
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
		
		// ��� socket �Ƿ�ɶ�
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

		// �����ѹر�
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

// ��ȡ���������ݣ����������Ƴ������ն���
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
		
		// ��� socket �Ƿ�ɶ�
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