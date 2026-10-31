#include <cstring>

#if defined(LIB_USE_WOLFSSL)
#include <wolfssl/options.h>
#include <wolfssl/wolfcrypt/hash.h>
#elif defined(LIB_USE_COMM_CRYPTO)
#include <CommonCrypto/CommonDigest.h>
#elif defined(LIB_USE_BCRYPT)
#include <windows.h>
#include <ntstatus.h>
#include <bcrypt.h>
#else
#include "sha256c.h"
#endif

#include "payload/verify/SHA256Utils.h"

namespace skkk {
	bool SHA256Utils::sha256Equal(const uint8_t *srcData, const uint8_t *destData) {
		return std::memcmp(srcData, destData, SHA256_DIGEST_LENGTH) == 0;
	}

	bool SHA256Utils::sha256(const uint8_t *srcData, std::size_t srcLen, uint8_t *destData) {
#if defined(LIB_USE_WOLFSSL)
		return wc_Sha256Hash(srcData, srcLen, destData) == 0;
#elif defined(LIB_USE_COMM_CRYPTO)
		return CC_SHA256(srcData, static_cast<CC_LONG>(srcLen), destData) != nullptr;
#elif defined(LIB_USE_BCRYPT)
		return BCRYPT_SUCCESS(
			BCryptHash(BCRYPT_SHA256_ALG_HANDLE, nullptr, 0,
			           const_cast<PUCHAR>(reinterpret_cast<const UCHAR *>(srcData)),
			           static_cast<ULONG>(srcLen),
			           const_cast<PUCHAR>(destData), SHA256_DIGEST_LENGTH));
#else
		SHA256_CTX ctx = {};
		sha256_init(&ctx);
		sha256_update(&ctx, srcData, srcLen);
		sha256_final(&ctx, destData);
		return true;
#endif
	}
}
