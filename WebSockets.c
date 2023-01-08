#include "WebSockets.h"

#define SEC_WEBSOCKET_KEY "Sec-WebSocket-Key: "
#define SEC_WEBSOCKET_KEY_LENGTH 24
#define WS_MAGIC_STRING "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"
#define WS_RESPONSE "HTTP/1.1 101 Switching Protocols\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Accept: "

void invertMemory(char* start, int size) {
	for (int i = 0; i < size / 2; i++) {
		char tmp = start[i];

		start[i] = start[size - i - 1];

		start[size - i - 1] = tmp;
	}
	return;
}

void printMem(char* start, int size) {
	for (int i = 0; i < size; i++) {
		printf("%02hhX ", start[i]);
	}
	printf("\n");
}

void wssend(SOCKET socket, char* message, unsigned long long messageLength) {

	int lengthLength = 0;

	if (messageLength <= 125) {
		lengthLength = 0;
	}
	else if (messageLength < 0xFFFF) {
		lengthLength = 2;
	}
	else {
		lengthLength = 8;
	}

	unsigned long long packetSize = messageLength + lengthLength + 2;

	char* messageBuffer = calloc(packetSize, sizeof(char));

	if (messageBuffer == NULL) {
		perror("could not allocate outgoing ws message buffer");
		exit(-1);
	}

	messageBuffer[0] = 0x82;

	if (lengthLength == 0) {
		messageBuffer[1] = (char)messageLength;
		memcpy(&messageBuffer[2], message, messageLength);
	}
	else if (lengthLength = 2) {
		messageBuffer[1] = 126;
		memcpy(&messageBuffer[2], &((unsigned short)messageLength), 2);
		memcpy(&messageBuffer[4], message, messageLength);
	}
	else {
		messageBuffer[1] = 127;
		memcpy(&messageBuffer[2], &messageLength, 8);
		memcpy(&messageBuffer[10], message, messageLength);
	}

	printMem(messageBuffer, packetSize);

	send(socket, messageBuffer, packetSize, NULL);
}

int handlePacket(char* frame, int incomingLength, SOCKET sender) {

	char flags = (frame[0] & 0xF0) >> 4;
	char opcode = frame[0] & 0x0F;

	int bytesConsumed = 0;

	if (flags & 0x04 == 0x04) {
		perror("Cannot handle fragmented packets");
		exit(-1);
	}

	unsigned char payloadLengthIndicator = frame[1] & 0x7f;

	unsigned long long payloadLength = 0;
	int maskingKey = 0;
	char* data = NULL;
	bytesConsumed = 6;

	if (payloadLengthIndicator <= 125) {
		payloadLength = payloadLengthIndicator;
		memcpy(&maskingKey, &frame[2], 4);
		data = &(frame[6]);
	}
	else if (payloadLengthIndicator == 126) {	
		unsigned short len = 0;
		memcpy(&len, &frame[2], 2);
		invertMemory((char*)&len, 2);
		payloadLength = len;
		memcpy(&maskingKey, &frame[4], 4);
		data = &(frame[8]);
		bytesConsumed += 2;
	}
	else if (payloadLengthIndicator == 127) {
		memcpy(&payloadLength, &frame[2], 8);
		invertMemory((char*)&payloadLength, 8);
		memcpy(&maskingKey, &frame[10], 4);
		data = &(frame[14]);
		bytesConsumed += 8;
	}
	else {
		// something went wrong
		perror("got illegal packet size from client packet");
		exit(-1);
	}
	bytesConsumed += payloadLength;

	printf("==MESSAGE==\n");
	if (opcode == 0x08) {
		printf("**Disconnect**\n");
	}
	
	printf("Length: %lld\n", payloadLength);
	printf("Data: ");

	if (data == NULL) {
		perror("data is still null");
		exit(-1);
	}

	for (int i = 0; i < payloadLength; i++) {
		char c = data[i] ^ (maskingKey >> ((i % 4) * 8));
		printf("%c", c);

		if (i == 0 && c == '0') {
			wssend(sender, "Hello from server!", 18);
		}
	}
	printf("\n==EOF==\n\n");

	if (incomingLength != bytesConsumed) { // nagles
		handlePacket(frame += bytesConsumed, incomingLength - bytesConsumed, sender);
	}

	return 0;
}

int onConnection(SOCKET socket) {
	// do nothing?
}

int onData(SOCKET socket, char* data, int len) {

	if (data[0] == 'G' && data[1] == 'E' && data[2] == 'T') {

		char* key = malloc(SEC_WEBSOCKET_KEY_LENGTH + sizeof(WS_MAGIC_STRING));
		if (key == NULL) {
			perror("could not allocate memory for concatedMagicString");
			exit(-1);
		}

		for (int i = 0; i < len; i++) {
			if (data[i] != '\n') {
				continue;
			}

			if (strncmp(&data[i + 1], SEC_WEBSOCKET_KEY, sizeof(SEC_WEBSOCKET_KEY) - 1) == 0) { // Incoming doesn't have \0 in the end but the macro does so go back one char
				memcpy(key, &data[i + sizeof(SEC_WEBSOCKET_KEY)], SEC_WEBSOCKET_KEY_LENGTH);

			}
		}

		key += SEC_WEBSOCKET_KEY_LENGTH;
		memcpy(key, WS_MAGIC_STRING, sizeof(WS_MAGIC_STRING));
		key -= SEC_WEBSOCKET_KEY_LENGTH; // Because I can't make concat work


		BYTE byteHashbuffer[SHA1_DIGEST_LEN];
		DWORD dwFinalHashSize = SHA1_DIGEST_LEN;

		GetSHA1Hash(key, SEC_WEBSOCKET_KEY_LENGTH + sizeof(WS_MAGIC_STRING) - 1, byteHashbuffer, &dwFinalHashSize);

		int base64Len = 0;

		char* base64Result = base64_encode(byteHashbuffer, SHA1_DIGEST_LEN, &base64Len);

		char* outgoingBuffer = malloc(sizeof(WS_RESPONSE) - 1 + base64Len + 4);

		if (outgoingBuffer == NULL) {
			perror("assigning outgoing ws buffer");
			exit(-1);
		}

		memcpy(outgoingBuffer, WS_RESPONSE, sizeof(WS_RESPONSE) - 1);
		outgoingBuffer += sizeof(WS_RESPONSE) - 1;

		memcpy(outgoingBuffer, base64Result, base64Len);
		outgoingBuffer += base64Len;

		memcpy(outgoingBuffer, "\r\n\r\n", 4);
		outgoingBuffer -= sizeof(WS_RESPONSE) - 1 + base64Len;

		send(socket, outgoingBuffer, sizeof(WS_RESPONSE) - 1 + base64Len + 4, 0);
	}
	else {
		handlePacket(data, len, socket);
	}	
}



int wsServer(char* port) {
	start(port, &onConnection, &onData);
	return 0;
}


