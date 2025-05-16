#ifndef PAYLOAD_EXTRACT_PAYLOAD_FILEINFO_H
#define PAYLOAD_EXTRACT_PAYLOAD_FILEINFO_H

#include <atomic>
#include <map>
#include <string>
#include <vector>
#include <cpr/cpr.h>

namespace skkk {
	class FileOperation {
		public:
			uint32_t type = 0;

			// url
			bool isUrl = false;
			bool sslVerification = false;
			// cpr::Url url;
			std::string url;

			// payload.bin
			uint64_t dataOffset = 0;
			uint64_t dataLength = 0;

			// dst_ext
			uint64_t startBlock = 0;
			uint64_t numBlocks = 0;

			// info
			uint32_t blockSize = 0;
			uint64_t destLength = 0;
			uint64_t fileOffset = 0;

			// data sha256
			std::string dataSha256Hash;


			FileOperation() = default;

			FileOperation(uint32_t type, bool isUrl, cpr::Url &url, bool sslVerification, uint32_t blockSize,
			              uint64_t dataOffset, uint64_t dataLength, uint64_t startBlock, uint64_t numBlocks,
			              std::string dataSha256Hash) : type(type), isUrl(isUrl),
			                                            sslVerification(sslVerification),
			                                            url(url), blockSize(blockSize), dataOffset(dataOffset),
			                                            dataLength(dataLength), startBlock(startBlock),
			                                            numBlocks(numBlocks), dataSha256Hash(dataSha256Hash) {
				destLength = numBlocks * blockSize;
				fileOffset = startBlock * blockSize;
			}
	};

	class FileWriteState {
		public:
			// write state
			std::atomic_int extractTaskRunCount;
			std::atomic_int exceptionSize;

			FileWriteState() {
				extractTaskRunCount = 0;
				exceptionSize = 0;
			};
	};

	class FileInfo {
		public:
			std::string name;
			uint64_t size = 0;
			std::vector<FileOperation> operations;

			FileInfo() = default;

			FileInfo(const std::string &fileName, uint64_t size,
			         std::vector<FileOperation> &operations) : name(fileName), size(size), operations(operations) {
			}

			FileInfo(const std::string &fileName, uint64_t size) : name(fileName), size(size) {
			}
	};


	typedef std::map<std::string, FileInfo> FileInfoMap;

	class PayloadFileInfo {
		public:
			FileInfoMap payloadFileMap;
	};
}

#endif //PAYLOAD_EXTRACT_PAYLOAD_FILEINFO_H
