#pragma once

#define BUFF_INCREASEMENT	(4096)

#define isurlchar(_x)	((_x > '\x20'))

#define isdomainchar(_x)	(isalpha(_x) || isdigit(_x) || (_x == '_') || (_x == '-') || (_x == '.'))

#define isentrychar(_x)	(isalpha(_x) || isdigit(_x) || (_x == '-'))

typedef struct RESPONSE_TEXT
{
	char *buffer;
	size_t length;
	size_t usage;
} RESPONSE_TEXT, *PRESPONSE_TEXT;

void HttpResponse(PSOCKET_PEER peer, int nStatusCode, WORD proto, const char *fmt, ...);
BOOL RequestParseURL(char *data, char **end, char **method, char **url, WORD *httpver);
BOOL RequestParseHost(char *data, char *host, char **path, WORD *wPort);
BOOL RequestParseEntry(char *data, char **end, char **key, char **value);