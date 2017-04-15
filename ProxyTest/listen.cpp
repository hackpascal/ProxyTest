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

	// ����
	if (!PeerListen(&listen, "0.0.0.0", 8090, SOMAXCONN))
	{
		OutputString("������ 0.0.0.0:8090 �ϼ���ʧ�ܡ�WSALastError = %d\n", WSAGetLastError());
		return FALSE;
	}

	// ��ѭ��
	while (true)
	{
		if (WaitForStop(50))
			break;

		// �¿ͻ��˽�������
		ret = PeerAccept(&listen, &s);

		if (ret == SOCKET_ERROR)
		{
			// Socket ����
			OutputString("����accept ʧ�ܡ�WSALastError = %d\n", WSAGetLastError());
			PeerClose(&listen);
			return FALSE;
		}

		// û��������
		if (ret == 0)
			continue;

		// Ϊ�����Ӵ����̣߳������������׽��ִ��ݸ��߳�
		if (!NewThread(ProcessRequest, (LPVOID) s, NULL))
		{
			OutputString("�����޷������̡߳�LastError = %d\n", GetLastError());
			closesocket(s);
		}
	}

	return TRUE;
}

// �����ӵ��Ⱥ���
DWORD WINAPI ProcessRequest(LPVOID lpParameter)
{
	SOCKET_PEER peer;
	int len;
	char buff[10];
	DWORD dwHttpType = 0;
	DWORD dwStartTick = GetTickCount();
	
	// �����׽���
	PeerInit(&peer);
	PeerAttach(&peer, (SOCKET) lpParameter);

	while (true)
	{
		if (WaitForStop(50))
			goto _close_connection;

		// ��ȡ���ݣ����������Ƴ�����
		PeerPeek(&peer, buff, sizeof (buff) - 1, &len);

		if (PeerIsClosed(&peer))
			goto _close_connection;

		if (!dwHttpType)
		{
			// ��� HTTP ���󷽷�
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
			OutputString("== ������ ==\n�̣߳�%08lx���׽��� = %u\n���� %s:%u\n\n", 
				GetCurrentThreadId(), peer.s, inet_ntoa(peer.peer_addr.sin_addr), 
				ntohs(peer.peer_addr.sin_port) & 0xffff);

			// ת����Ӧ�� HTTP ������
			if (dwHttpType == 1)
				return HttpProxyRequest(&peer);

			if (dwHttpType == 2)
				return HttpsProxyRequest(&peer);

			// XXX: �����ܵ���ɡ���
			break;
		}
	}

	// ����ʧ��
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