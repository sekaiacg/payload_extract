#ifndef PAYLOAD_EXTRACT_SHA256UTILS_H
#define PAYLOAD_EXTRACT_SHA256UTILS_H

#include <cstring>

#include "sha256.h"

#if defined(LIB_USE_MBEDTLS)
#include <mbedtls/sha256.h>
#define USE_MBEDTLS
#elif !defined(LIB_USE_OPENSSL)
#include <openssl/evp.h>
#define USE_OPENSSL
#else
#include "sha256.h"
#endif

namespace skkk {
	static bool sha256Equal(const uint8_t *srcData, const uint8_t *destData, uint32_t srcLen) {
		return memcmp(srcData, destData, srcLen) == 0;
	}

	static bool sha256(const uint8_t *srcData, uint64_t srcLen, uint8_t *destData) {
		bool ret = true;
#if defined(USE_MBEDTLS)
		ret = mbedtls_sha256(srcData, srcLen, destData, 0) == 0;
#elif defined(USE_OPENSSL)
		uint32_t destLength = SHA256_DIGEST_SIZE;
		int rc = EVP_Digest(srcData, srcLen, destData, &destLength,
		                    EVP_sha256(), nullptr);
		if (rc != 1) {
			ret = false;
		}
#else
		SHA256_CTX ctx = {};
		sha256_init(&ctx);
		sha256_update(&ctx, srcData, srcLen);
		sha256_final(&ctx, destData);
#endif
		return ret;
	}

	static void *sha256Init() {
		void *ctx = nullptr;
#if defined(USE_MBEDTLS)
		auto *mCtx = static_cast<mbedtls_sha256_context *>(malloc(sizeof(mbedtls_sha256_context)));
		mbedtls_sha256_init(mCtx);
		int rc = mbedtls_sha256_starts(mCtx, 0);
		if (rc == 0) {
			ctx = mCtx;
		}
#elif defined(USE_OPENSSL)
		EVP_MD_CTX *mdCtx = EVP_MD_CTX_new();
		int rc = EVP_DigestInit_ex(mdCtx, EVP_sha256(), nullptr);
		if (rc == 1) {
			ctx = mdCtx;
		}
#else
		auto *sCtx = static_cast<SHA256_CTX *>(calloc(1, sizeof(SHA256_CTX)));
		sha256_init(sCtx);
		ctx = sCtx;
#endif
		return ctx;
	}

	static bool sha256Update(void *ctx, const uint8_t *srcData, uint32_t srcLen) {
		bool ret = true;
#if defined(USE_MBEDTLS)
		auto *mCtx = static_cast<mbedtls_sha256_context *>(ctx);
		ret = mbedtls_sha256_update(mCtx, srcData, srcLen) == 0;
#elif defined(USE_OPENSSL)
		auto mdCtx = static_cast<EVP_MD_CTX *>(ctx);
		int rc = EVP_DigestUpdate(mdCtx, srcData, srcLen);
		if (rc != 1) {
			ret = false;
		}
#else
		sha256_update(static_cast<SHA256_CTX *>(ctx), srcData, srcLen);
#endif
		return ret;
	}

	static bool sha256Finish(void *ctx, uint8_t *destData) {
		bool ret = true;
#if defined(USE_MBEDTLS)
		auto *mCtx = static_cast<mbedtls_sha256_context *>(ctx);
		ret = mbedtls_sha256_finish(mCtx, destData) == 0;
#elif defined(USE_OPENSSL)
		auto mdCtx = static_cast<EVP_MD_CTX *>(ctx);
		uint32_t length = SHA256_DIGEST_SIZE;
		int rc = EVP_DigestFinal_ex(mdCtx, destData, &length);
		if (rc != 1) {
			ret = false;
		}
#else
		sha256_final(static_cast<SHA256_CTX *>(ctx), destData);
#endif
		return ret;
	}

	static void sha256Free(void *ctx) {
#if defined(USE_MBEDTLS)
		auto *mCtx = static_cast<mbedtls_sha256_context *>(ctx);
		mbedtls_sha256_free(mCtx);
		free(mCtx);
#elif defined(USE_OPENSSL)
		free(ctx);
#endif
		ctx = nullptr;
	}
}
#endif //PAYLOAD_EXTRACT_SHA256UTILS_H
