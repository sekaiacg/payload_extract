/*********************************************************************
* Filename:   sha256.h
* Author:     Brad Conte (brad AT bradconte.com)
* Copyright:
* Disclaimer: This code is presented "as is" without any guarantees.
* Details:    Defines the API for the corresponding SHA1 implementation.
*********************************************************************/
/**
 * Reference: https://github.com/wangkui0508/sha256
 *
 **/

#ifndef PAYLOAD_EXTRACT_SHA256_H
#define PAYLOAD_EXTRACT_SHA256_H

/*************************** HEADER FILES ***************************/
#include <cstddef>
#include <cstdint>

/****************************** MACROS ******************************/
#define SHA256_BLOCK_SIZE 32            // SHA256 outputs a 32 byte digest

/**************************** DATA TYPES ****************************/
// typedef uint8_t BYTE; // 8-bit byte
// typedef uint32_t WORD; // 32-bit word, change to "long" for 16-bit machines

typedef struct {
	uint8_t data[64];
	uint32_t datalen;
	unsigned long long bitlen;
	uint32_t state[8];
} SHA256_CTX;

/*********************** FUNCTION DECLARATIONS **********************/
void sha256_init(SHA256_CTX *ctx);

void sha256_update(SHA256_CTX *ctx, const uint8_t *data, size_t len);

void sha256_final(SHA256_CTX *ctx, uint32_t *hash);

#endif  // PAYLOAD_EXTRACT_SHA256_H
