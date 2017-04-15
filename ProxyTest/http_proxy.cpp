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

	// ��ȡ��ͷ����
	while (true)
	{
		if (WaitForStop(10))
		{
			self->bReset = TRUE;
			goto _close_connection;
		}

		// ���仺����
		if (total + 1 >= len)
		{
			len += BUFF_INCREASEMENT;
			buff = (char *) realloc(buff, len + 1);
		}

		// ��������ͷ
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

		// �Ѷ��걨ͷ
		if (hdr_end = strstr(buff, "\r\n\r\n"))
			break;

		// ���ӳ�ʱ
		if (GetTickCount() - dw64StartTick > dwConnectCheckTimeout * 1000)
		{
			self->bReset = TRUE;
			goto _close_connection;
		}

		Sleep(50);
	}

	hdr_end += 4;

	// ������ͷ��һ�� �������������� URL��HTTP �汾��
	if (!RequestParseURL(buff, &entry_start, &method, &url, &wHttpVer))
	{
		// ���ĸ�ʽ���󡣡���ֹ����
		HttpResponse(self, 400, wHttpVer, "Wrong HTTP header format.");
		self->bReset = TRUE;
		goto _close_connection;
	}

	// ���� URL ���������˿ں�����·��
	if (!RequestParseHost(url, host, &path, &wPort))
	{
		// URL ��ʽ���Ϸ�
		HttpResponse(self, 400, wHttpVer, "Wrong HTTP header format.");
		self->bReset = TRUE;
		goto _close_connection;
	}

	// �����ع��� HTTP ����ͷ
	request.Format("%s %s HTTP/%u.%u\r\n", method, path, HIBYTE(wHttpVer), LOBYTE(wHttpVer));

	// ������ͷ��Ŀ
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
			// Proxy-Connection �����͸�ʵ�ʷ�����
			if (!_stricmp(value, "Keep-Alive"))
				bCloseConnection = FALSE;
			
			// ��Ϊ Connection
			key = "Connection";
			goto _next_entry;
		}

		// POST ���͵����ݳ���
		if (!strcmp(key, "Content-Length"))
		{
			post_length = strtoul(value, NULL, 10);
			goto _next_entry;
		}

		// ��֤ͷ��
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

	OutputString("�߳� %08lx ���ӣ�%s:%u��Э�� HTTP/%u.%u\n", GetCurrentThreadId(), host, wPort & 0xffff, HIBYTE(wHttpVer), LOBYTE(wHttpVer));
	OutputString("%s ", method);
	OutputStringDirect(path);
	if (post_length)
		OutputString("�ύ����: %d\n", post_length);
	OutputStringDirect("\n");

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
					OutputString("�߳� %08lx ��ȡ��Ч����ʧ��\n\n", GetCurrentThreadId());
					goto _close_connection;
				}
				post_length -= recv_len;
			}
		}

		goto _restart;
	}

	if (strcmp(host, last_host) || (wPort != wLastPort))
		PeerClose(&peer);

	// ���ӵ�Ŀ�������
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

	OutputString("�߳� %08lx �ر�����\n\n", GetCurrentThreadId());

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

	// ��������ͷ
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

	// ����� POST �������� POST ������
	if (post_data && post_first_len && post_length)
	{
		// POST ���ݵ�һ��
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

		// POST ʣ�����ݳ���
		post_len_rest = post_length - post_first_len;

		while (post_len_rest > 0)
		{
			if (WaitForStop(10))
			{
				self->bReset = TRUE;
				peer->bReset = TRUE;
				return FALSE;
			}

			// ����������͵� POST ��������
			if (!PeerReceive(self, post_buff, post_len_rest > BUFF_INCREASEMENT ? BUFF_INCREASEMENT : post_len_rest, &recv_len))
			{
				OutputString("�ͻ��� fd = %u\n����ʣ���ϴ����ݴ��� %d\n\n", self->s, WSAGetLastError());
				self->bReset = TRUE;
				return FALSE;
			}

			// ��������������ѶϿ�����ֹͣ����
			if (PeerIsClosed(self))
			{
				self->bReset = TRUE;
				return FALSE;
			}

			// �����յ����ݷ��͸�Ŀ������� 
			if (!PeerSend(peer, post_buff, recv_len, &sent_len))
			{
				OutputString("�ͻ��� fd = %u\n����ʣ���ϴ����ݴ��� %d\n\n", self->s, WSAGetLastError());
				peer->bReset = TRUE;
				return FALSE;
			}

			if (recv_len != sent_len)
			{
				OutputString("�ͻ��� fd = %u\n����ʣ�����ݳ��ȴ��� %d\n\n", self->s, WSAGetLastError());
				peer->bReset = TRUE;
				return FALSE;
			}

			// ��Ŀ������������ӶϿ�����ֹͣ����
			if (PeerIsClosed(peer))
			{
				peer->bReset = TRUE;
				return FALSE;
			}

			post_len_rest -= recv_len;
		}
	}

	// ������Ӧ���ݲ�����
	while (true)
	{
		if (WaitForStop(50))
		{
			self->bReset = TRUE;
			peer->bReset = TRUE;
			return FALSE;
		}

		// �����ǰ�Ļ������������㣬�����·���
		if (total + 1 >= len)
		{
			len += BUFF_INCREASEMENT;
			buff = (char *) realloc(buff, len);
		}

		// ��Ŀ�������������Ӧ����
		if (!PeerReceive(peer, buff + total, len - total - 1, &recv_len))
		{
			bCloseConnection = TRUE;
			peer->bReset = TRUE;
			goto _abort;
		}

		total += recv_len;
		buff[total] = 0;

		// �Ѷ��걨ͷ
		if (hdr_end = strstr(buff, "\r\n\r\n"))
			break;

		// û�����걨ͷ��Ŀ��������ͶϿ������ӣ���ֹͣ����
		if (PeerIsClosed(peer))
		{
			bCloseConnection = TRUE;
			peer->bReset = TRUE;
			goto _abort;
		}
	}

	hdr_end += 4;
	p = buff;

	// ��ȡ���ݳ���
	while (p < hdr_end)
	{
		// ��ȡ��Ӧ���ĵ����ݳ���
		if (!strncmp(p, content_length_str, sizeof (content_length_str) - 1))
		{
			p += sizeof (content_length_str) - 1;
			if (*p == ':') p++;
			while (*p == '\x20' || *p == '\t') p++;

			content_length = strtoul(p, NULL, 10);

			goto _entry_next;
		}

		// ��ȡ����״̬�����ֻ��ǹر�
		if (!strncmp(p, connection_str, sizeof (connection_str) - 1))
		{
			p += sizeof (connection_str) - 1;
			if (*p == ':') p++;
			while (*p == '\x20' || *p == '\t') p++;

			if (!_strnicmp(p, keep_alive_str, sizeof (keep_alive_str) - 1))
				bCloseConnection = FALSE;

			goto _entry_next;
		}

		// ��ȡ�������ݴ��䷽ʽ����֪���ȵ��������ݻ���δ֪���ȵķֿ�����
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
		// ��һ��
		p = strstr(p, "\r\n");
		if (p)
		{
			p += 2;
			continue;
		}

		break;
	}

	// ��ȡ��Ӧ��ͷ��һ�У��������ڵ��ԣ�
	p = strstr(buff, "\r\n");
	strncpy(rep_hdr, buff, (size_t) (p - buff));
	rep_hdr[(size_t) (p - buff)] = 0;

	OutputString("�߳� %08lx Զ�̷���˻�Ӧ\n%s\n", GetCurrentThreadId(), rep_hdr);
	if (!bChunkedMode)
		OutputString("���س��� %uB\n", content_length);
	else
		OutputString("Chunked ģʽ\n");
	OutputString("\n");

	// ��ͷ����
	header_len = (size_t) (hdr_end - buff);

	// ���ͱ�ͷ
	if (!PeerSend(self, buff, header_len, &sent_len))
	{
		OutputString("�ͻ��� fd = %u\n���ر�ͷ���� %d\n\n", self->s, WSAGetLastError());
		bCloseConnection = TRUE;
		self->bReset = TRUE;
		goto _abort;
	}

	if (sent_len != header_len)
	{
		OutputString("�ͻ��� fd = %u\n���ر�ͷ���ȴ��� %d\n\n", self->s, WSAGetLastError());
		bCloseConnection = TRUE;
		self->bReset = TRUE;
		goto _abort;
	}

	// ����������ʼ��ַ
	p = hdr_end;

	// ����Ƿ� Chunked ģʽ
	if (!bChunkedMode)
	{
		// ����������
		if (!DoProxyNormal(peer, self, p, total - header_len, content_length))
			bCloseConnection = FALSE;

		goto _finish;
	}

	// ���� Chunked ����
	if (!DoProxyChunked(peer, self, p, total - header_len))
	{
		bCloseConnection = FALSE;
	}

_finish:
	OutputString("�߳� %08lx ���%s\n\n", GetCurrentThreadId(), bCloseConnection ? "" : ", Keep-Alive");

_abort:
	if (buff) free(buff);
	return !bCloseConnection;
}

// ���汨�����ݴ���
BOOL DoProxyNormal(PSOCKET_PEER peer, PSOCKET_PEER self, const char *first_block, int first_len, int content_length)
{
	char buff[BUFF_INCREASEMENT];
	int sent_len, recv_len, rest;

	// ֻ�б�ͷ��û�б��ģ�ֱ���˳�
	if (!content_length)
		return TRUE;

	// ����Ѿ���ȡ�˲��ֱ������ݣ����䷢��
	if (first_len)
	{
		if (!PeerSend(self, (void *) first_block, first_len, &sent_len))
		{
			OutputString("�ͻ��� fd = %u\n���ص�һ�������ݴ��� %d\n\n", self->s, WSAGetLastError());
			self->bReset = TRUE;
			return FALSE;
		}

		if (sent_len != first_len)
		{
			OutputString("�ͻ��� fd = %u\n���ص�һ�������ݳ��ȴ��� %d\n\n", self->s, WSAGetLastError());
			self->bReset = TRUE;
			return FALSE;
		}
	}

	// ʣ�����ݳ���
	rest = content_length - first_len;

	while (rest)
	{
		if (WaitForStop(10))
		{
			self->bReset = TRUE;
			peer->bReset = TRUE;
			return FALSE;
		}

		// ��Ŀ����������ձ�������
		if (!PeerReceive(peer, buff, rest > BUFF_INCREASEMENT ? BUFF_INCREASEMENT : rest, &recv_len))
		{
			OutputString("�ͻ��� fd = %u\n����ʣ�����ݴ��� %d\n\n", self->s, WSAGetLastError());
			peer->bReset = TRUE;
			return FALSE;
		}

		// ���͸������
		if (!PeerSend(self, buff, recv_len, &sent_len))
		{
			OutputString("�ͻ��� fd = %u\n����ʣ�����ݴ��� %d\n\n", self->s, WSAGetLastError());
			self->bReset = TRUE;
			return FALSE;
		}

		if (recv_len != sent_len)
		{
			OutputString("�ͻ��� fd = %u\n����ʣ�����ݳ��ȴ��� %d\n\n", self->s, WSAGetLastError());
			self->bReset = TRUE;
			return FALSE;
		}
		
		// ������������ӶϿ�����ֹͣ����
		if (PeerIsClosed(self))
		{
			self->bReset = TRUE;
			return FALSE;
		}

		// ��Ŀ������������ӶϿ�����ʾ�����Ѷ�ȡ���
		if (PeerIsClosed(peer))
			break;

		rest -= recv_len;
	}

	return TRUE;
}

// �ֿ����ݴ���
BOOL DoProxyChunked(PSOCKET_PEER peer, PSOCKET_PEER self, const char *first_block, int first_len)
{
	char buff[BUFF_INCREASEMENT], *buff_end = buff + sizeof (buff);
	char *p, *d;
	int buf_len, recv_len, sent_len;
	int chunk_len, rest_len;

	// ��һ�����ݳ��Ȳ�Ӧ�ô��� BUFF_INCREASEMENT
	if (first_len > BUFF_INCREASEMENT)
		return FALSE;

	// ���Ѿ���ȡ�ı������ݸ��Ƶ�������
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

		// ������仺����ʣ�ಿ��
		if (!PeerReceive(peer, (void *) (buff + buf_len), sizeof (buff) - buf_len, &recv_len))
		{
			OutputString("�ͻ��� fd = %u\n���Խ���ʣ�����ݴ��� %d\n\n", self->s, WSAGetLastError());
			self->bReset = TRUE;
			return FALSE;
		}

		buf_len += recv_len;

		// ��Ŀ������������ӶϿ�����ʾ���ݿ����Ѷ�ȡ���
		if (PeerIsClosed(peer))
			break;
	} while (!recv_len);

	p = buff;

	// ѭ����ȡÿ����ĳ��ȼ����Ϳ�����
	while (true)
	{
		if (WaitForStop(10))
		{
			self->bReset = TRUE;
			peer->bReset = TRUE;
			return FALSE;
		}

		// ��ȡ�鳤��
		chunk_len = strtoul(p, &d, 16);

		// �������ܴ��ڵĿո�
		while (*d == '\x20') p++;

		// ����һ�����з�
		if (strncmp(d, "\r\n", 2))
		{
			peer->bReset = TRUE;
			return FALSE;
		}

		d +=2;

		// ��ǰ��ĳ��ȣ�����ָ���鳤�ȵ����ݣ�
		rest_len = chunk_len + (size_t) (d - p) + 2;
		while (rest_len)
		{
			if (WaitForStop(10))
			{
				self->bReset = TRUE;
				peer->bReset = TRUE;
				return FALSE;
			}

			// �����ݷ��͸������
			if (!PeerSend(self, (void *)p, rest_len > buf_len ? buf_len : rest_len, &sent_len))
			{
				OutputString("�ͻ��� fd = %u\n���Է���ʣ�����ݴ��� %d\n\n", self->s, WSAGetLastError());
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
				// ��������û�������ˣ����¶�ȡ
				do
				{
					if (WaitForStop(10))
					{
						self->bReset = TRUE;
						peer->bReset = TRUE;
						return FALSE;
					}

					// �����ݶ�ȡ����������ʼλ��
					if (!PeerReceive(peer, (void *) buff, sizeof (buff), &recv_len))
					{
						OutputString("�ͻ��� fd = %u\n���Խ���ʣ�����ݴ��� %d\n\n", self->s, WSAGetLastError());
						peer->bReset = TRUE;
						return FALSE;
					}

					// ����ǰ����ָ���ƶ�����������ʼλ��
					p = buff;
					buf_len += recv_len;

					// ��Ŀ������������ӶϿ�����ʾ���ݿ����Ѷ�ȡ���
					if (PeerIsClosed(peer))
						break;
				} while (!recv_len);
			}
		}

		// ����鳤��Ϊ 0����ʾ�Ѷ�ȡ�����п�����
		if (!chunk_len)
			break;

		// ������ʣ������̫�٣��ͽ���������������ǰ��
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

				// ������仺����ʣ�ಿ��
				if (!PeerReceive(peer, (void *) (buff + buf_len), sizeof (buff) - buf_len, &recv_len))
				{
					OutputString("�ͻ��� fd = %u\n���Խ���ʣ�����ݴ��� %d\n\n", self->s, WSAGetLastError());
					peer->bReset = TRUE;
					return FALSE;
				}

				buf_len += recv_len;

				// ��Ŀ������������ӶϿ�����ʾ���ݿ����Ѷ�ȡ���
				if (PeerIsClosed(peer))
					break;
			} while (buf_len < 5);
		}
	}

	return TRUE;
}