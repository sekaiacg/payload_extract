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
			const uint8_t *inData;
			uint64_t readFilePos;
			uint64_t writeHashPos;
			uint8_t *hashData = nullptr;

			VerifyWriterHashTreeContext(const VerifyInfo &verifyInfo, const uint8_t *inData, uint64_t readFilePos,
			                            uint64_t writeHashPos, uint8_t *hashData)
				: verifyInfo(verifyInfo),
				  inData(inData),
				  readFilePos(readFilePos),
				  writeHashPos(writeHashPos),
				  hashData(hashData) {
			}
	};

	class VerifyWriterFecContext {
		public:
			const VerifyInfo &verifyInfo;
			uint8_t *fecData = nullptr;
			const uint8_t *inData = nullptr;
			uint64_t roundsIdx;

			VerifyWriterFecContext(const VerifyInfo &verifyInfo, uint8_t *fecData, const uint8_t *inData,
			                       uint64_t roundsIdx)
				: verifyInfo(verifyInfo),
				  fecData(fecData),
				  inData(inData),
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
