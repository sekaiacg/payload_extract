#ifndef PAYLOAD_EXTRACT_FILEWRITER_H
#define PAYLOAD_EXTRACT_FILEWRITER_H

#include <functional>

#include "HttpDownload.h"
#include "PartitionInfo.h"

namespace skkk {
	class FileWriter {
		using decompressPtr = std::function<int(const uint8_t *src, uint64_t srcSize,
		                                        uint8_t *dest, uint64_t &destSize)>;

		std::shared_ptr<HttpDownload> httpDownload;

		public:
			FileWriter() = default;

			explicit FileWriter(const std::shared_ptr<HttpDownload> &httpDownload)
				: httpDownload(httpDownload) {
			}

			FileWriter(const FileWriter &other)
				: httpDownload(other.httpDownload) {
			}

			FileWriter &operator=(const FileWriter &other) {
				if (this == &other)
					return *this;
				httpDownload = other.httpDownload;
				return *this;
			}

			int urlRead(uint8_t *buf, const FileOperation &operation) const;

			int commonWrite(const decompressPtr &decompress, int payloadBinFd, int outFd,
			                const FileOperation &operation) const;

			int directWrite(int payloadFd, int outFd, const FileOperation &operation) const;

			int bzipWrite(int payloadFd, int outFd, const FileOperation &operation) const;

			static int zeroWrite(int payloadFd, int outFd, const FileOperation &operation);

			int xzWrite(int payloadFd, int outFd, const FileOperation &operation) const;

			int zstdWrite(int payloadFd, int outFd, const FileOperation &operation) const;

			static int extentsRead(int fd, uint8_t *data, const std::vector<Extent> &extents);

			static int extentsWrite(int fd, uint8_t *data, const std::vector<Extent> &extents);

			static int sourceCopy(int inFd, int outFd, const FileOperation &operation);

			int brotliBSDiff(int payloadBinFd, int inFd, int outFd, const FileOperation &operation) const;

			int writeDataByType(int payloadBinFd, int inFd, int outFd, const FileOperation &operation) const;
	};
}

#endif //PAYLOAD_EXTRACT_FILEWRITER_H
