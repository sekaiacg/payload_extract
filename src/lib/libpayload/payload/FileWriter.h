#ifndef PAYLOAD_EXTRACT_FILEWRITER_H
#define PAYLOAD_EXTRACT_FILEWRITER_H

#include <functional>

#include "PayloadManifest.h"

namespace skkk {
	class FileWriter {
		using decompressPtr = std::function<int(const uint8_t *src, uint64_t srcSize,
		                                        uint8_t *dest, uint64_t &destSize)>;

		public:
			static int urlRead(uint8_t *buf, const FileOperation &operation);

			static int commonWrite(const decompressPtr &decompress, int payloadBinFd, int outFd,
			                       const FileOperation &operation);

			static int directWrite(int payloadFd, int outFd, const FileOperation &operation);

			static int bzipWrite(int payloadFd, int outFd, const FileOperation &operation);

			static int zeroWrite(int payloadFd, int outFd, const FileOperation &operation);

			static int xzWrite(int payloadFd, int outFd, const FileOperation &operation);

			static int zstdWrite(int payloadFd, int outFd, const FileOperation &operation);

			static int writeDataByType(int payloadBinFd, int outFd, const FileOperation &operation);
	};
}

#endif //PAYLOAD_EXTRACT_FILEWRITER_H
