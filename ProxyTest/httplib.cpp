#include "stdafx.h"

void HttpResponse(PSOCKET_PEER peer, int nStatusCode, WORD proto, const char *fmt, ...)
{
	int nSize;
	char *buff;
	va_list arglist;

	char *state_text;
	char output[4096];

	va_start(arglist, fmt);

	nSize = _vscprintf(fmt, arglist);
	buff = new char[nSize + 1];
	_vsprintf_s_l(buff, nSize + 1, fmt, NULL, arglist);

	va_end(arglist);

	switch (nStatusCode)
	{
	case 200:
		state_text = "Connection Established";
		break;
	case 400:
		state_text = "Bad Request";
		break;
	case 415:
		state_text = "Unsupported Media Type";
		break;
	case 503:
		state_text = "Service Unavaliable";
		break;
	case 505:
		state_text = "HTTP Version Not Supported";
		break;
	default:
		return;
	}

	_snprintf(output, sizeof (output), 
		"HTTP/%u.%u %d %s\r\n"
		"Connection: close\r\n"
		"Content-Length: %u\r\n"
		"\r\n",
		HIBYTE(proto), LOBYTE(proto), nStatusCode, state_text,
		strlen(buff));

	PeerSend(peer, (void *) output, strlen(output), NULL);
	PeerSend(peer, (void *) buff, strlen(buff), NULL);

	delete [] buff;
}

BOOL RequestParseURL(char *data, char **end, char **method, char **url, WORD *httpver)
{
	char *buf = data;
	char *method_end, *url_start, *url_end;
	UCHAR uVerHigh, uVerLow;

	while (*buf >= 'A' && *buf <= 'Z') buf++;
	method_end = buf; // ���� Method ���һ���ַ������ᱻ��Ϊ NULL

	// �����ո�
	while(*buf == '\x20' || *buf == '\t') buf++;

	if (!isurlchar(*buf))
	{
		// URL �ַ���Ч
		return FALSE;
	}

	url_start = buf;
	while (isurlchar(*buf) && (*buf != '\x20') && (*buf != '\t')) buf++;
	url_end = buf; // ���� URL ���һ���ַ������ᱻ��Ϊ NULL

	// �����ո�
	while(*buf == '\x20' || *buf == '\t') buf++;

	if (strncmp(buf, "HTTP/", 5))
	{
		// ��ʽ���Ϸ���Ӧ��Ϊ HTTP/x.x
		return FALSE;
	}

	buf += 5;

	// ���� HTTP �汾
	if (!isdigit(*buf))
	{
		// ��ʽ���Ϸ���Ӧ��Ϊ����
		return FALSE;
	}

	uVerHigh = strtoul(buf, &buf, 10) & 0xff;

	if (*buf != '.' || !isdigit(*(buf + 1)))
	{
		// ��ʽ���Ϸ���Ӧ��Ϊ .�����Ϊ����
		return FALSE;
	}

	buf++;

	uVerLow = strtoul(buf, &buf, 10) & 0xff;

	// �����ո�
	while(*buf == '\x20' || *buf == '\t') buf++;

	if (strncmp(buf, "\r\n", 2))
	{
		// ��ʽ���Ϸ���Ӧ��Ϊ CrLf
		return false;
	}

	buf += 2; // ���ﴦ���ĩβ

	// ȫ���Ϸ�����������
	*method_end = 0;
	*url_end = 0;

	if (end) *end = buf;
	if (method) *method = data;
	if (url) *url = url_start;
	if (httpver) *httpver = MAKEWORD(uVerLow, uVerHigh);

	return TRUE;
}

BOOL RequestParseHost(char *data, char *host, char **path, WORD *wPort)
{
	char *buf = data;
	char *p, *domain_start = NULL, *domain_end = NULL;
	WORD wTcpPort = 80;

	// ���ж��Ƿ����Э��
	p = strstr(buf, "://");

	if (p)
	{
		// Э���Ƿ�Ϊ4���ַ� (http)
		if ((size_t) (p - buf) != 4)
			return FALSE;

		if (strncmp(buf, "http", 4))
		{
			// ���� http Э�飬��֧��
			return FALSE;
		}

		buf = p + 3;
	}

	// ��������
	if (*buf != '/' && *buf != 0)
	{
		domain_start = buf;
		while (isdomainchar(*buf)) buf++;
		domain_end = buf; // ����ַ���������Ч�Ĳ��ܱ��滻Ϊ NULL

		// �Ƿ�����˿�
		if (*buf == ':')
		{
			buf++;
			if (!isdigit(*buf))
			{
				// �˿�ֵ��Ч
				return FALSE;
			}

			wTcpPort = strtoul(buf, &buf, 10) & 0xffff;
		}
	}

	if (*buf != '/' && *buf != 0 && path)
	{
		// ·�����ֲ��Ϸ�
		return FALSE;
	}

	// �������ݺϷ�
	if (domain_start && domain_end && host)
		strncpy(host, domain_start, (size_t) (domain_end - domain_start));
	host[(size_t) (domain_end - domain_start)] = 0;

	if (path) *path = buf;
	if (wPort) *wPort = wTcpPort;

	return TRUE;
}

BOOL RequestParseEntry(char *data, char **end, char **key, char **value)
{
	char *buf = data;
	char *key_end, *value_start, *value_end;

	if (!strncmp(buf, "\r\n", 2))
	{
		// �ֶ����ѽ���
		return FALSE;
	}

	while (isentrychar(*buf)) buf++;
	key_end = buf;

	if (*buf != ':')
	{
		// �ֶ������������� :
		return FALSE;
	}

	buf++;
	
	// �����ո�
	while(*buf == '\x20' || *buf == '\t') buf++;
	value_start = buf;

_process_multiline:
	while (strncmp(buf, "\r\n", 2)) buf++;

	// �ж��Ƿ�Ϊ��������
	if (buf[2] == '\x20' || buf[2] == '\t')
	{
		buf += 2;
		goto _process_multiline;
	}

	value_end = buf;

	// ����ǰ�ֶ�ĩβ
	buf += 2;

	// �������ݺϷ�
	*key_end = 0;
	*value_end = 0;

	if (end) *end = buf;
	if (key) *key = data;
	if (value) *value = value_start;

	return TRUE;
}
