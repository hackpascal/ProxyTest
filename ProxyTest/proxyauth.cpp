#include "stdafx.h"
#include "b64decode.h"

BOOL ProxyAuthenticate(const char *data)
{
	char text[1024], check[1024];
	const char *p = data;
	int textlen;
	base64_decodestate _state;

	if (strncmp(data, "Basic", 5))
		return FALSE;

	p += 5;

	while (*p == '\x20' || *p == '\t') p++;

	base64_init_decodestate(&_state);
	textlen = base64_decode_block(p, strlen(data) - (size_t) (p - data), text, &_state);

	if (textlen <= 0)
		return FALSE;

	text[textlen] = 0;

	sprintf_s(check, sizeof (check), "hackpascal:aabbcc");

	if (strcmp(text, check))
		return FALSE;

	return TRUE;
}