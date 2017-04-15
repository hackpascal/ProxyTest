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
	method_end = buf; // 这是 Method 后的一个字符，将会被改为 NULL

	// 跳过空格
	while(*buf == '\x20' || *buf == '\t') buf++;

	if (!isurlchar(*buf))
	{
		// URL 字符无效
		return FALSE;
	}

	url_start = buf;
	while (isurlchar(*buf) && (*buf != '\x20') && (*buf != '\t')) buf++;
	url_end = buf; // 这是 URL 后的一个字符，将会被改为 NULL

	// 跳过空格
	while(*buf == '\x20' || *buf == '\t') buf++;

	if (strncmp(buf, "HTTP/", 5))
	{
		// 格式不合法，应该为 HTTP/x.x
		return FALSE;
	}

	buf += 5;

	// 解析 HTTP 版本
	if (!isdigit(*buf))
	{
		// 格式不合法，应该为数字
		return FALSE;
	}

	uVerHigh = strtoul(buf, &buf, 10) & 0xff;

	if (*buf != '.' || !isdigit(*(buf + 1)))
	{
		// 格式不合法，应该为 .，其后为数字
		return FALSE;
	}

	buf++;

	uVerLow = strtoul(buf, &buf, 10) & 0xff;

	// 跳过空格
	while(*buf == '\x20' || *buf == '\t') buf++;

	if (strncmp(buf, "\r\n", 2))
	{
		// 格式不合法，应该为 CrLf
		return false;
	}

	buf += 2; // 到达处理的末尾

	// 全部合法，处理数据
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

	// 先判断是否包含协议
	p = strstr(buf, "://");

	if (p)
	{
		// 协议是否为4个字符 (http)
		if ((size_t) (p - buf) != 4)
			return FALSE;

		if (strncmp(buf, "http", 4))
		{
			// 不是 http 协议，不支持
			return FALSE;
		}

		buf = p + 3;
	}

	// 包含域名
	if (*buf != '/' && *buf != 0)
	{
		domain_start = buf;
		while (isdomainchar(*buf)) buf++;
		domain_end = buf; // 这个字符可能是有效的不能被替换为 NULL

		// 是否包含端口
		if (*buf == ':')
		{
			buf++;
			if (!isdigit(*buf))
			{
				// 端口值无效
				return FALSE;
			}

			wTcpPort = strtoul(buf, &buf, 10) & 0xffff;
		}
	}

	if (*buf != '/' && *buf != 0 && path)
	{
		// 路径部分不合法
		return FALSE;
	}

	// 所有数据合法
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
		// 字段项已结束
		return FALSE;
	}

	while (isentrychar(*buf)) buf++;
	key_end = buf;

	if (*buf != ':')
	{
		// 字段名后面必须紧跟 :
		return FALSE;
	}

	buf++;
	
	// 跳过空格
	while(*buf == '\x20' || *buf == '\t') buf++;
	value_start = buf;

_process_multiline:
	while (strncmp(buf, "\r\n", 2)) buf++;

	// 判断是否为多行数据
	if (buf[2] == '\x20' || buf[2] == '\t')
	{
		buf += 2;
		goto _process_multiline;
	}

	value_end = buf;

	// 到当前字段末尾
	buf += 2;

	// 所有数据合法
	*key_end = 0;
	*value_end = 0;

	if (end) *end = buf;
	if (key) *key = data;
	if (value) *value = value_start;

	return TRUE;
}
