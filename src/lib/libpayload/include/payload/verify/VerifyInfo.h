#ifndef PAYLOAD_EXTRACT_VERIFYINFO_H
#define PAYLOAD_EXTRACT_VERIFYINFO_H

#include <atomic>
#include <cinttypes>
#include <memory>
#include <string>
#include <vector>

#include "payload/PartitionInfo.h"

namespace skkk {
	static constexpr uint64_t SHA256_DIGEST_SIZE = 32;

	class Level {
		public:
			uint32_t blockCount = 0;
			uint32_t realHashSize = 0;
			uint32_t paddingSize = 0;
			uint32_t totalHashSize = 0;
			std::vector<uint8_t> data;
			uint8_t *hashData = nullptr;

		public:
			Level() = default;

			Level(uint64_t targetSize, uint32_t blockSize);
	};

	class VerifyInfo {
		public:
			std::string name;
			std::string outFilePath;
			uint64_t blockSize = 0;

			bool hasHashTreeDataExtent = false;
			// hash tree starting from offset
			uint64_t hashTreeDataExtentOffset = 0;
			// Size of data to be calc
			uint64_t hashTreeDataExtentSize = 0;
			// offset of hash tree in the file
			uint64_t hashTreeDataOffset = 0;
			// hash tree size
			uint64_t hashTreeDataSize = 0;
			std::string hashTreeSalt;
			Level topHashLevel;
			mutable Level rootHashLevel;
			mutable std::vector<Level> hashLevels;
			uint64_t hashTreeTotalProgress = 0;
			std::shared_ptr<std::atomic_int> hashTreeProgress = std::make_shared<std::atomic_int>(0);
			std::shared_ptr<std::atomic_int> hashTreeExcSize = std::make_shared<std::atomic_int>(0);
			mutable bool isCalcHashTreeSuccessful = false;

			bool hasFecDataExtent = false;
			// Encode starting from offset
			uint64_t fecDataExtentOffset = 0;
			// Size of data to be encoded
			uint64_t fecDataExtentSize = 0;
			// offset of fec data in the file
			uint64_t fecDataOffset = 0;
			// fec data size
			uint64_t fecDataSize = 0;
			uint32_t fecRoots = 2;
			uint32_t fecRsn = 0;
			uint64_t fecRounds = 0;
			std::vector<uint8_t> fecData;
			std::shared_ptr<std::atomic_int> fecProgress = std::make_shared<std::atomic_int>(0);
			std::shared_ptr<std::atomic_int> fecExcSize = std::make_shared<std::atomic_int>(0);
			mutable bool isCalcFecSuccessful = false;

		public:
			explicit VerifyInfo(const PartitionInfo &partInfo);

			bool checkCalcHashTreeSuccessful() const;

			bool checkCalcFecSuccessful() const;

			void resetStatus() const;
	};
}

#endif //PAYLOAD_EXTRACT_VERIFYINFO_H
