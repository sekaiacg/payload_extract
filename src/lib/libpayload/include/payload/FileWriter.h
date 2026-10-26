#ifndef PAYLOAD_EXTRACT_FILEWRITER_H
#define PAYLOAD_EXTRACT_FILEWRITER_H

#include <concepts>
#include <functional>

#include "HttpDownload.h"
#include "PartitionInfo.h"

namespace skkk {
	enum class WriteType:uint8_t {
		BUFFER,
		FD,
		PATH,
		UNKNOWN
	};

	class WriteParam {
		public:
			const WriteType type = WriteType::UNKNOWN;
			const uint8_t *payloadData = nullptr;
			const uint8_t *oldData = nullptr;
			uint8_t *outData = nullptr;
			const int payloadFd = -1;
			const int oldFd = -1;
			const int outFd = -1;
			const std::string *payloadPath = nullptr;
			const std::string *oldPath = nullptr;
			const std::string *outPath = nullptr;

		public:
			WriteParam(const int payloadFd, const int oldFd, const int outFd);

			WriteParam(const std::string *payloadPath, const std::string *oldPath, const std::string *outPath);

			WriteParam(const uint8_t *payloadData, const uint8_t *oldData, uint8_t *outData);
	};

	namespace Dec {
		template<typename F>
		concept Decompressor =
				std::invocable<F, const uint8_t *, uint64_t, uint8_t *, uint64_t> &&
				std::convertible_to<std::invoke_result_t<F, const uint8_t *, uint64_t, uint8_t *, uint64_t>, int>;
	}

	class FileWriter {
		protected:
			const std::shared_ptr<HttpDownload> httpDownload;

		public:
			FileWriter(const std::shared_ptr<HttpDownload> &httpDownload);

			virtual ~FileWriter() = default;

			static uint32_t getRdWaitTime();

			virtual int urlRead(uint8_t *data, const FileOperation &operation) const;

			static int extentsRead(int inFd, uint8_t *outData, const std::vector<Extent> &extents);

			static int extentsWrite(const uint8_t *inData, int outFd, const std::vector<Extent> &extents);

			int directWrite(int payloadFd, int outFd, const FileOperation &operation) const;

			int decompressWrite(Dec::Decompressor auto &&decompress, int payloadFd, int outFd,
			                    const FileOperation &operation) const;

			int bzipWrite(int payloadFd, int outFd, const FileOperation &operation) const;

			static int zeroWrite(int outFd, const FileOperation &operation);

			int xzWrite(int payloadFd, int outFd, const FileOperation &operation) const;

			int zstdWrite(int payloadFd, int outFd, const FileOperation &operation) const;

			static int sourceCopy(int payloadFd, int outFd, const FileOperation &operation);

			int brotliBSDiff(int payloadFd, int oldFd, int outFd, const FileOperation &operation) const;

			virtual int writeDataByType(const WriteParam &writeParam, const FileOperation &operation) const;
	};
}

#endif //PAYLOAD_EXTRACT_FILEWRITER_H
