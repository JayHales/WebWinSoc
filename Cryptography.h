#pragma once
#include "TCPServer.h"
#define SHA1_DIGEST_LEN 20

BOOL GetSHA1Hash(char* buffer,             //input buffer
	DWORD dwBufferSize,       //input buffer size
	BYTE* byteFinalHash,      //ouput hash buffer
	DWORD* dwFinalHashSize    //input/output final buffer size
);

char* base64_encode(const unsigned char* data,
	size_t input_length,
	size_t* output_length);