#pragma once

#include <cstdint>

namespace skkk {
	constexpr static int SHA256_DIGEST_LENGTH = 32;

	class SHA256Utils {
		public:
			static bool sha256Equal(const uint8_t *srcData, const uint8_t *destData);

			static bool sha256(const uint8_t *srcData, std::size_t srcLen, uint8_t *destData);
	};
}
