#include "TCPServer.h"

struct socketAcceptanceData {
	SOCKET socket;
	int index;
	void (*onData)(SOCKET socket, char* data, int len);
	void (*onConnection)(SOCKET socket);
};

SOCKET clientSockets[MAX_CONNECTIONS];
int currentConnections = 0;

DWORD WINAPI onSocketAccept(void* data) {
	struct socketAcceptanceData* params = data;

	char recvbuf[DEFAULT_BUFLEN];
	ZeroMemory(&recvbuf, sizeof(recvbuf));

	int recvbuflen = DEFAULT_BUFLEN;


	int recvResult;

	params->onConnection(params->socket);
	do {
		recvResult = recv(params->socket, recvbuf, recvbuflen, 0);

		if (recvResult == DEFAULT_BUFLEN) {
			printf("Data length is equal to max buffer length. There may have been a buffer overflow.");
		}

		if (recvResult > 0) {
			params->onData(params->socket, recvbuf, recvResult);
		}

	} while (recvResult > 0);

	if (recvResult < 0) {
		printf("Client socket error: %i", recvResult);
	}

	for (int i = 0; i < MAX_CONNECTIONS; i++) {
		if (clientSockets[i] == params->socket) {
			clientSockets[i] = INVALID_SOCKET;
			currentConnections--;
		}
	}
	closesocket(params->socket);
	return 0;
}
 

void start(char* port, void onConnection(SOCKET socket), void onData(SOCKET socket, char* data, int len)) {

	memset(clientSockets, 0, MAX_CONNECTIONS * sizeof(SOCKET));

	SOCKET serverSocket = INVALID_SOCKET;

	struct addrinfo hints; // Stores opening socket settings
	struct addrinfo* result = NULL; // Stores the result of collating the hints into an actual address

	WSADATA wsaData; // This doesn't do anything?
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) { // Just starts up winsock?
		perror("WSAStartup failed");
		exit(-1);
	}

	ZeroMemory(&hints, sizeof(hints)); // Zero out all of "hints"
	hints.ai_family = AF_INET; // Use IPv4?
	hints.ai_socktype = SOCK_STREAM; // TCP not UDP
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE; // What other options are there?

	if (getaddrinfo(NULL, port, &hints, &result) != 0) { // Might need to define a port length at somepoint?
		perror("getaddrinfo failed");
		WSACleanup();
		exit(-1);
	}

	serverSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol); // Actually make the socket
	if (serverSocket == INVALID_SOCKET) {
		perror("calling socket failed");
		freeaddrinfo(result);
		WSACleanup();
		exit(-1);
	}

	if (bind(serverSocket, result->ai_addr, (int)result->ai_addrlen) == SOCKET_ERROR) {
		perror("failed to bind server socket");
		freeaddrinfo(result);
		closesocket(serverSocket);
		WSACleanup();
		exit(-1);
	}

	freeaddrinfo(result);

	if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {
		perror("calling listen on server socket");
		closesocket(serverSocket);
		WSACleanup();
		exit(-1);
	}

	SOCKET recentSocket;
	while (1) {
	
		if (currentConnections >= MAX_CONNECTIONS) {
			continue;
		}

		recentSocket = accept(serverSocket, NULL, NULL);
		if (recentSocket == INVALID_SOCKET) {
			perror("bad incoming socket");
			continue;
		}

		for (int i = 0; i < MAX_CONNECTIONS; i++) {
			if (clientSockets[i] == INVALID_SOCKET || clientSockets[i] == 0) {
				clientSockets[i] = recentSocket;
				
				struct socketAcceptanceData params;

				params.index = i;
				params.onData = &onData;
				params.onConnection = &onConnection;
				params.socket = recentSocket;

				currentConnections++;
				CreateThread(NULL, 0, onSocketAccept, (void*)(&params), 0, NULL);
				break;
			}
		}
	}

	closesocket(serverSocket);
	WSACleanup();
	return;
}

void broadcast(char* message, int messageLength) {
	for (int i = 0; i < MAX_CONNECTIONS; i++) {
		
		SOCKET s = clientSockets[i];

		if (s != INVALID_SOCKET) {
			send(s, message, messageLength, 0);
		}
	}
}