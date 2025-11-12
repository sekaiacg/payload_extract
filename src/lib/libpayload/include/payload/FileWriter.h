#ifndef PAYLOAD_EXTRACT_FILEWRITER_H
#define PAYLOAD_EXTRACT_FILEWRITER_H

#include <functional>

#include "HttpDownload.h"
#include "PartitionInfo.h"

namespace skkk {
	class FileWriter {
		using decompressPtr = std::function<int(const uint8_t *src, uint64_t srcSize,
		                                        uint8_t *dest, uint64_t destSize)>;

		const std::shared_ptr<HttpDownload> &httpDownload;

		public:
			FileWriter(const std::shared_ptr<HttpDownload> &httpDownload);

			int urlRead(uint8_t *buf, const FileOperation &operation) const;

			int commonWrite(const decompressPtr &decompress, const uint8_t *payloadData, uint8_t *outData,
			                const FileOperation &operation) const;

			int directWrite(const uint8_t *payloadData, uint8_t *outData, const FileOperation &operation) const;

			int bzipWrite(const uint8_t *payloadData, uint8_t *outData, const FileOperation &operation) const;

			static int zeroWrite(const uint8_t *payloadData, uint8_t *outData, const FileOperation &operation);

			int xzWrite(const uint8_t *payloadData, uint8_t *outData, const FileOperation &operation) const;

			int zstdWrite(const uint8_t *payloadData, uint8_t *outData, const FileOperation &operation) const;

			static int extentsRead(const uint8_t *inData, uint8_t *data, const std::vector<Extent> &extents);

			static int extentsWrite(uint8_t *outData, const uint8_t *srcData, const std::vector<Extent> &extents);

			static int sourceCopy(const uint8_t *inData, uint8_t *outData, const FileOperation &operation);

			int brotliBSDiff(const uint8_t *payloadData, const uint8_t *inData, uint8_t *outData,
			                 const FileOperation &operation) const;

			int writeDataByType(const uint8_t *payloadData, const uint8_t *inData, uint8_t *outData,
			                    const FileOperation &operation) const;
	};
}

#endif //PAYLOAD_EXTRACT_FILEWRITER_H
