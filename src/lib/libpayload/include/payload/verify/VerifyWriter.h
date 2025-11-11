#ifndef PAYLOAD_EXTRACT_DMVERIFYHASHTREEGEN_H
#define PAYLOAD_EXTRACT_DMVERIFYHASHTREEGEN_H

#include <cinttypes>
#include <vector>

#include "payload/ExtractConfig.h"
#include "payload/PartitionInfo.h"

#include "VerifyInfo.h"

namespace skkk {
	class VerifyWriterHashTreeContext {
		public:
			const VerifyInfo &verifyInfo;
			mutable int inFd;
			uint64_t readFilePos;
			uint64_t writeHashPos;
			uint8_t *hashData = nullptr;
			uint64_t saltVerifySize;

			VerifyWriterHashTreeContext(const VerifyInfo &verifyInfo, int inFd, uint64_t readFilePos,
			                            uint64_t writeHashPos, uint8_t *hashData, uint64_t saltVerifySize)
				: verifyInfo(verifyInfo),
				  inFd(inFd),
				  readFilePos(readFilePos),
				  writeHashPos(writeHashPos),
				  hashData(hashData),
				  saltVerifySize(saltVerifySize) {
			}
	};

	class VerifyWriterFecContext {
		public:
			const VerifyInfo &verifyInfo;
			mutable int inFd;
			uint8_t *fecData = nullptr;
			uint64_t roundsIdx;

			VerifyWriterFecContext(const VerifyInfo &verifyInfo, int inFd, uint8_t *fecData, uint64_t roundsIdx)
				: verifyInfo(verifyInfo),
				  inFd(inFd),
				  fecData(fecData),
				  roundsIdx(roundsIdx) {
			}
	};

	class VerifyWriter {
		const std::vector<PartitionInfo> &partitions;
		const ExtractConfig &config;
		std::vector<VerifyInfo> verifyInfos;

		public:
			VerifyWriter(const std::vector<PartitionInfo> &partitions, const ExtractConfig &config);

			void initHashTreeLevel();

			bool handleHashTreeDataByInfo(const VerifyInfo &info) const;

			static bool updateHashTreeByInfo(const VerifyInfo &info);

			bool handleFecDataByInfo(const VerifyInfo &info) const;

			static bool updateFecByInfo(const VerifyInfo &info);

			void updateVerifyData() const;
	};
}

#endif //PAYLOAD_EXTRACT_DMVERIFYHASHTREEGEN_H
