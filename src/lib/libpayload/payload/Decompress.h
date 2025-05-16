#ifndef PAYLOAD_EXTRACT_DECOMPRESS_H
#define PAYLOAD_EXTRACT_DECOMPRESS_H

#include <cinttypes>

namespace skkk {
	class Decompress {
		static constexpr uint32_t MaxDictSize = 64 * 1024 * 1024;

		public:
			static int bzipDecompress(const void *src, uint64_t srcSize, void *destBuf, uint64_t &destSize);

			static int xzDecompress(const void *src, uint64_t srcSize, void *destBuf, uint64_t &destSize);

			static int zstdDecompress(const void *src, uint64_t srcSize, void *destBuf, uint64_t &destSize);
	};
}

#endif //PAYLOAD_EXTRACT_DECOMPRESS_H
