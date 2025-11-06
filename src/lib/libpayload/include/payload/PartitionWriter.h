#ifndef PAYLOAD_EXTRACT_PARTITIONWRITER_H
#define PAYLOAD_EXTRACT_PARTITIONWRITER_H

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "PayloadInfo.h"

namespace skkk {
	class PartitionWriter {
		std::shared_ptr<PayloadInfo> payloadInfo;
		std::vector<PartitionInfo> partitions;
		ExtractConfig extractConfig;

		public:
			explicit PartitionWriter(const std::shared_ptr<PayloadInfo> &partitionInfo)
				: payloadInfo(partitionInfo), extractConfig(partitionInfo->getExtractConfig()) {
			}

			bool initPartitions();

			bool initPartitionsByTarget();

			void printPartitionsInfo() const;

			static int openFileRDOnly(const std::string &path);

			bool createOutDir() const;

			static int openFileRW(const std::string &path);

			static int createOutFile(const std::string &path, uint64_t fileSize);

			static int initInFd(const std::string &path);

			static int initOutFd(const std::string &path, uint64_t fileSize, bool isReOpen = false);

			bool extractByInfo(const PartitionInfo &info) const;

			bool incrementalExtractByInfo(const PartitionInfo &info) const;

			bool extractByInfoMT(const PartitionInfo &info) const;

			bool incrementalExtractByInfoMT(const PartitionInfo &info) const;

			bool extractByInfoMTInWin(const PartitionInfo &info) const;

			bool incrementalExtractByInfoMTInWin(const PartitionInfo &info) const;

			bool extractByPartitionByName(const std::string &name);

			void extractPartitions() const;
	};
}

#endif //PAYLOAD_EXTRACT_PARTITIONWRITER_H
