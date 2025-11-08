#ifndef PAYLOAD_EXTRACT_PAYLOADPARTITIONINFO_H
#define PAYLOAD_EXTRACT_PAYLOADPARTITIONINFO_H

#include <atomic>
#include <cinttypes>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace skkk {
	class Extent {
		public:
			uint64_t blockSize = 0;
			uint64_t startBlock = 0;
			uint64_t numBlocks = 0;

			// Offset of data in image
			uint64_t dataOffset = 0;
			// data length
			uint64_t dataLength = 0;

		public:
			Extent() = default;

			Extent(uint64_t blockSize, uint64_t startBlock, uint64_t numBlocks) : blockSize(blockSize),
				startBlock(startBlock), numBlocks(numBlocks) {
				dataOffset = startBlock * blockSize;
				dataLength = numBlocks * blockSize;
			}
	};

	class FileOperation {
		public:
			std::string partName;
			uint32_t type = 0;

			std::string url;

			// data block size
			uint32_t blockSize = 0;

			// Offset of data in payload.bin
			uint64_t dataOffset = 0;
			// length in Data
			uint64_t dataLength = 0;

			// src_ext
			std::vector<Extent> srcExtents;
			uint64_t srcLength = 0;
			uint64_t srcTotalLength = 0;

			// dst_ext
			std::vector<Extent> dstExtents;
			uint64_t dstLength = 0;
			uint64_t dstTotalLength = 0;

			// src sha256
			std::string srcDataSha256Hash;

			// data sha256
			std::string dataSha256Hash;

			// status
			mutable std::string excInfo;

		public:
			FileOperation() = default;

			FileOperation(const std::string &partName, uint32_t type, uint32_t blockSize, uint64_t dataOffset,
			              uint64_t dataLength,
			              uint64_t srcLength, uint64_t dstLength, const std::string &srcDataSha256Hash,
			              const std::string &dataSha256Hash)
				: partName(partName),
				  type(type),
				  blockSize(blockSize),
				  dataOffset(dataOffset),
				  dataLength(dataLength),
				  srcLength(srcLength),
				  dstLength(dstLength),
				  srcDataSha256Hash(srcDataSha256Hash),
				  dataSha256Hash(dataSha256Hash) {
			}

			void initExcInfo(int errCode) const;
	};

	class PartitionInfo {
		std::shared_ptr<std::mutex> mutex_ = std::make_shared<std::mutex>();

		public:
			std::string name;
			uint64_t size = 0;

			std::string oldFilePath;
			std::string outFilePath;
			std::string outErrorPath;

			uint32_t blockSize = 0;

			std::string oldHash;
			std::string oldHashHexStr;
			uint64_t oldHashSize = 0;

			std::string newHash;
			std::string newHashHexStr;
			uint64_t newHashSize = 0;

			bool hasHashTreeDataExtent = false;
			// The extent for data covered by verity hash tree.
			Extent hashTreeDataExtent;
			// The extent to store verity hash tree.
			Extent hashTreeExtent;
			// The hash algorithm used in verity hash tree.
			std::string hashTreeAlgorithm;
			// The salt used for verity hash tree.
			std::string hashTreeSalt;

			bool hasFecDataExtent = false;;
			// The extent for data covered by FEC.
			Extent fecDataExtent;
			// The extent to store FEC.
			Extent fecExtent;
			// The number of FEC roots.
			uint32_t fecRoots = 2;

			std::vector<FileOperation> operations;

			// status
			std::shared_ptr<std::atomic_int> extractProgress = std::make_shared<std::atomic_int>(0);
			mutable bool isExtractionSuccessful = false;
			mutable std::vector<std::string> excInfos;

		public:
			PartitionInfo() = default;

			PartitionInfo(const std::string &name, uint64_t size, const std::string &outFilePath, uint32_t blockSize,
			              const std::string &oldHash, uint64_t oldHashSize, const std::string &newHash,
			              uint64_t newHashSize);

			void printInfo() const;

			bool checkExtractionSuccessful() const;

			void initExcInfoByInitFd(const std::string &path, int errCode) const;

			void initExcInfos() const;

			void ifExcExistsWrite2File() const;

			void resetStatus() const;
	};
}

#endif //PAYLOAD_EXTRACT_PAYLOADPARTITIONINFO_H
