#pragma once

typedef struct SOCKET_PEER
{
	char host[256];
	sockaddr_in self_addr;
	sockaddr_in peer_addr;
	SOCKET s;
	BOOL listen;
	BOOL bReuse;
	BOOL bReset;
} SOCKET_PEER, *PSOCKET_PEER;

void PeerInit(PSOCKET_PEER peer);
BOOL PeerListen(PSOCKET_PEER peer, const char *localhost, WORD port, int maxqueue);
int PeerAccept(PSOCKET_PEER peer, SOCKET *s);
BOOL PeerConnect(PSOCKET_PEER peer, const char *host, WORD port);
BOOL PeerAttach(PSOCKET_PEER peer, SOCKET s);
void PeerClose(PSOCKET_PEER peer);
BOOL PeerIsClosed(PSOCKET_PEER peer);
BOOL PeerSend(PSOCKET_PEER peer, void *data, int length, int *slength);
BOOL PeerReceive(PSOCKET_PEER peer, void *data, int length, int *rlength);
BOOL PeerPeek(PSOCKET_PEER peer, void *data, int length, int *rlength);