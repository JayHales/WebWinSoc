#include "Cryptography.h"
BOOL GetSHA1Hash(char* buffer,             //input buffer
	DWORD dwBufferSize,       //input buffer size
	BYTE* byteFinalHash,      //ouput hash buffer
	DWORD* dwFinalHashSize    //input/output final buffer size
)
{
	DWORD dwStatus = 0;
	BOOL bResult = FALSE;
	HCRYPTPROV hProv = 0;
	HCRYPTHASH hHash = 0;
	//BYTE *byteHash;
	DWORD cbHashSize = 0;

	// Get handle to the crypto provider
	if (!CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT))
	{
		printf("\nCryptAcquireContext failed, Error=0x%.8x", GetLastError());
		return FALSE;
	}

	//Specify the Hash Algorithm here
	if (!CryptCreateHash(hProv, CALG_SHA1, 0, 0, &hHash))
	{
		printf("\nCryptCreateHash failed,  Error=0x%.8x", GetLastError());
		goto EndHash;
	}

	//Create the hash with input buffer
	if (!CryptHashData(hHash, (const BYTE*)buffer, dwBufferSize, 0))
	{
		printf("\nCryptHashData failed,  Error=0x%.8x", GetLastError());
		goto EndHash;
	}

	//Get the final hash size 
	DWORD dwCount = sizeof(DWORD);
	if (!CryptGetHashParam(hHash, HP_HASHSIZE, (BYTE*)&cbHashSize, &dwCount, 0))
	{
		printf("\nCryptGetHashParam failed, Error=0x%.8x", GetLastError());
		goto EndHash;
	}

	//check if the output buffer is enough to copy the hash data
	if (*dwFinalHashSize < cbHashSize)
	{
		printf("\nOutput buffer (%d) is not sufficient, Required Size = %d",
			*dwFinalHashSize, cbHashSize);
		goto EndHash;
	}

	//Now get the computed hash 
	if (CryptGetHashParam(hHash, HP_HASHVAL, byteFinalHash, dwFinalHashSize, 0))
	{
		bResult = TRUE;
	}
	else
	{
		printf("\nCryptGetHashParam failed,  Error=0x%.8x", GetLastError());
	}


EndHash:

	if (hHash)
		CryptDestroyHash(hHash);

	if (hProv)
		CryptReleaseContext(hProv, 0);

	return bResult;
}

#include <stdint.h>
#include <stdlib.h>


static char encoding_table[] = { 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
								'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
								'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
								'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
								'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
								'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
								'w', 'x', 'y', 'z', '0', '1', '2', '3',
								'4', '5', '6', '7', '8', '9', '+', '/' };
static int mod_table[] = { 0, 2, 1 };

char* base64_encode(const unsigned char* data,
	size_t input_length,
	size_t* output_length) {

	*output_length = 4 * ((input_length + 2) / 3);

	char* encoded_data = malloc(*output_length);
	if (encoded_data == NULL) return NULL;

	for (int i = 0, j = 0; i < input_length;) {

		uint32_t octet_a = i < input_length ? (unsigned char)data[i++] : 0;
		uint32_t octet_b = i < input_length ? (unsigned char)data[i++] : 0;
		uint32_t octet_c = i < input_length ? (unsigned char)data[i++] : 0;

		uint32_t triple = (octet_a << 0x10) + (octet_b << 0x08) + octet_c;

		encoded_data[j++] = encoding_table[(triple >> 3 * 6) & 0x3F];
		encoded_data[j++] = encoding_table[(triple >> 2 * 6) & 0x3F];
		encoded_data[j++] = encoding_table[(triple >> 1 * 6) & 0x3F];
		encoded_data[j++] = encoding_table[(triple >> 0 * 6) & 0x3F];
	}

	for (int i = 0; i < mod_table[input_length % 3]; i++)
		encoded_data[*output_length - 1 - i] = '=';

	return encoded_data;
}