#ifndef PAYLOAD_EXTRACT_FILEWRITER_H
#define PAYLOAD_EXTRACT_FILEWRITER_H

#include "PayloadManifest.h"

namespace skkk {
	typedef int (*decompress_func_t)(const void *src, uint64_t srcSize, void *dest, uint64_t &destSize);

	class FileWriter {
		public:
			static int urlRead(uint8_t *buf, const FileOperation &operation);

			static int commonWrite(const decompress_func_t decompress_func, int payloadBinFd, int outFd,
			                       const FileOperation &operation);

			static int directWrite(int payloadFd, int outFd, const FileOperation &operation);

			static int zeroWrite(int payloadFd, int outFd, const FileOperation &operation);

			static int bzipWrite(int payloadFd, int outFd, const FileOperation &operation);

			static int xzWrite(int payloadFd, int outFd, const FileOperation &operation);

			static int zstdWrite(int payloadFd, int outFd, const FileOperation &operation);

			static int writeDataByType(int payloadBinFd, int outFd, const FileOperation &operation);
	};
}

#endif //PAYLOAD_EXTRACT_FILEWRITER_H
