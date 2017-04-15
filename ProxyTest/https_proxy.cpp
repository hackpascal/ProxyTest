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

		// ���仺����
		if (total + 1 >= len)
		{
			len += BUFF_INCREASEMENT;
			buff = (char *) realloc(buff, len);
		}
		
		// ��������ͷ
		if (!PeerReceive(self, buff + total, len - total, &recv_len))
			goto _close_connection;

		if (recv_len > 0)
		{
			total += recv_len;
			buff[total] = 0;
		}

		if (PeerIsClosed(self))
			goto _close_connection;

		// �Ѷ��걨ͷ
		if (hdr_end = strstr(buff, "\r\n\r\n"))
			break;

		// ���ӳ�ʱ
		if (GetTickCount() - dw64StartTick > dwConnectCheckTimeout * 1000)
			goto _close_connection;

		Sleep(50);
	}

	hdr_end += 4;

	// ������ͷ��һ�� �������������� URL��HTTP �汾��
	if (!RequestParseURL(buff, &entry_start, &method, &url, &wHttpVer))
	{
		// ���ĸ�ʽ���󡣡���ֹ����
		HttpResponse(self, 400, wHttpVer, "Wrong HTTP header format.");
		goto _close_connection;
	}

	// ���� URL ���������˿�
	if (!RequestParseHost(url, host, NULL, &wPort))
	{
		// URL ��ʽ���Ϸ�
		HttpResponse(self, 400, wHttpVer, "Wrong HTTP header format.");
		goto _close_connection;
	}

	// ������ͷ��Ŀ
	while (RequestParseEntry(entry_start, &entry_start, &key, &value))
	{
		// ��֤ͷ��
		if (!strcmp(key, "Proxy-Authorization"))
		{
			StringCchCopyA(auth, sizeof (auth), value);
			break;
		}
	}

	// ������֤
	if (!auth[0] || !ProxyAuthenticate(auth))
	{
		char auth_required[] = 
			"HTTP/1.1 407 Proxy Authentication Required\r\n"
			"Proxy-Authenticate: Basic realm=\"ProxyTest\"\r\n"
			"\r\n";

		OutputString("�߳� %08lx ��Ҫ�����֤\n\n", GetCurrentThreadId());

		if (!PeerSend(self, (void *) auth_required, sizeof (auth_required) - 1, NULL))
		{
			OutputString("�߳� %08lx ������֤����ʧ��\n\n", GetCurrentThreadId());
			goto _close_connection;
		}

		goto _restart;
	}

	// CONNECT ����ֻ�� HTTP 1.1 ��֧��
	if (wHttpVer != MAKEWORD(1, 1))
	{
		HttpResponse(self, 505, wHttpVer, "HTTP Version must be 1.1");
		goto _close_connection;
	}

	PeerInit(&peer);
	
	// ���ӷ�����
	if (!PeerConnect(&peer, host, wPort))
	{
		HttpResponse(self, 503, wHttpVer, "Unable to connect to %s:%u", host, wPort & 0xffff);
		goto _close_connection;
	}

	// ��ʼ socket ת��

	OutputString("�߳� %08lx ��ʼ TCP ת��\n"
		"%s:%u <--> %s:%u\n\n",
		GetCurrentThreadId(),
		host, wPort,
		inet_ntoa(self->peer_addr.sin_addr), self->peer_addr.sin_port & 0xffff);

	// ����֪ͨ�¼�
	hErrorEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	hStopEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

	// �������Ŀ�������������ת��
	ZeroMemory(&tx, sizeof (tx));
	tx.from = self;
	tx.to = &peer;
	tx.hErrorEvent = hErrorEvent;
	tx.hStopEvent = hStopEvent;

	// ��Ŀ��������������������ת��
	ZeroMemory(&rx, sizeof (rx));
	rx.from = &peer;
	rx.to = self;
	rx.hErrorEvent = hErrorEvent;
	rx.hStopEvent = hStopEvent;

	// �����̣߳���ʼΪ����״̬
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

	// ֪ͨ��������Ӵ����ɹ�
	if (!PeerSend(self, (void *) connection_ok, sizeof (connection_ok) - 1, NULL))
	{
		OutputString("�߳� %08lx TCP ���ӽ���֪ͨʧ��\n\n", GetCurrentThreadId());
		goto _close_connection;
	}

	// �ָ��߳�
	ResumeThread(hTransmit);
	ResumeThread(hReceive);

	// ѭ��
	while (true)
	{
		// ������յ�ֹͣ�źţ����˳�ѭ��
		if (WaitForStop(1000))
			break;

		// ������̳߳������˳�ѭ��
		if (WaitForSingleObject(hErrorEvent, 0) == WAIT_OBJECT_0)
			break;
	}

	// ֹͣ socket ת��

	// ���̷߳���ֹͣ�ź�
	SetEvent(hStopEvent);

	for (int i = 0; i < 5; i++)
		Sleep(1000);

	// ��ֹ�߳�
	TerminateThread(hTransmit, 0);
	TerminateThread(hReceive, 0);

	// �رվ��
	CloseHandle(hTransmit);
	CloseHandle(hReceive);
	CloseHandle(hStopEvent);
	CloseHandle(hErrorEvent);

_close_connection:
	OutputString("�߳� %08lx TCP ת������\n\n", GetCurrentThreadId());

	PeerClose(&peer);
	PeerClose(self);

	if (buff) free(buff);

	return 0;
}

// Socket ����ת��
DWORD WINAPI SocketForward(LPVOID lpParameter)
{
	PSOCKET_FORWARD_INFO sf = (PSOCKET_FORWARD_INFO) lpParameter;
	char buff[2 * BUFF_INCREASEMENT];
	int ret, err, recv_len, send_len, rest_len;
	fd_set readset, writeset;
	TIMEVAL t;
	
	t.tv_sec = 2;
	t.tv_usec = 0;

	// ���� TCP ����ת��
	while (true)
	{
		// �����⵽ֹͣ�źţ����˳�ѭ��
		if (WaitForSingleObject(sf->hStopEvent, 0) == WAIT_OBJECT_0)
			break;

		// ��ȡ����
		FD_ZERO(&readset);
		FD_SET(sf->from->s, &readset);

		// ��� socket �Ƿ�ɶ�
		ret = select(sf->from->s + 1, &readset, NULL, NULL, &t);

		if (ret == SOCKET_ERROR)
		{
			// ���������ź�
			SetEvent(sf->hErrorEvent);
			return 1;
		}
		
		// ��ⳬʱ�����ɶ�
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
			// �����ѶϿ�
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
				// ���������ź�
				SetEvent(sf->hErrorEvent);
				return 1;
			}
		}

		// ��������
		rest_len = recv_len;

		// �����൱��һ����ѭ��
_send_retry:
		// �����⵽ֹͣ�źţ����˳�ѭ��
		if (WaitForSingleObject(sf->hStopEvent, 0) == WAIT_OBJECT_0)
			break;

		if (!rest_len)
			continue;

		FD_ZERO(&writeset);
		FD_SET(sf->to->s, &writeset);
		
		// ��� socket �Ƿ��д
		ret = select(sf->to->s + 1, NULL, &writeset, NULL, &t);

		if (ret == SOCKET_ERROR)
		{
			// ���������ź�
			SetEvent(sf->hErrorEvent);
			return 1;
		}

		// ��ⳬʱ������д
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
						// ���������ź�
						SetEvent(sf->hErrorEvent);
						return 1;
					}
				}
				
				// ���ݿ����޷�һ��ȫ������
				if (send_len > 0)
					rest_len -= send_len;
			}
			goto _send_retry;
		}
	}

	return 0;
}