#ifndef PAYLOAD_EXTRACT_PARTITIONWRITER_H
#define PAYLOAD_EXTRACT_PARTITIONWRITER_H

#include <memory>
#include <string>
#include <vector>

#include "FileWriter.h"
#include "PayloadInfo.h"

namespace skkk {
	class PartitionWriteContext {
		public:
			const std::string &payloadPath;
			const PartitionInfo &partitionInfo;
			const FileWriter &fileWriter;
			const FileOperation &operation;
			mutable int payloadFd;
			mutable int inFd;
			mutable int outFd;
			const bool isIncremental;
			const bool isUrl;

		public:
			PartitionWriteContext(const std::string &payloadPath, const PartitionInfo &partitionInfo,
			                      const FileWriter &fileWriter, const FileOperation &operation,
			                      int payloadFd, int inFd, int outFd, bool isIncremental, bool isUrl)
				: payloadPath(payloadPath),
				  partitionInfo(partitionInfo),
				  fileWriter(fileWriter),
				  operation(operation),
				  payloadFd(payloadFd),
				  inFd(inFd),
				  outFd(outFd),
				  isIncremental(isIncremental),
				  isUrl(isUrl) {
			}
	};

	class PartitionWriter {
		const std::shared_ptr<PayloadInfo> &payloadInfo;
		const ExtractConfig &config;
		std::vector<PartitionInfo> partitions;

		public:
			explicit PartitionWriter(const std::shared_ptr<PayloadInfo> &payloadInfo);

			bool initPartitions();

			bool initPartitionsByTarget();

			void printPartitionsInfo() const;

			bool createOutDir() const;

			static int createOutFile(const std::string &path, uint64_t fileSize);

			static int initInFd(const std::string &path);

			static int initOutFd(const std::string &path, uint64_t fileSize, bool isReOpen = false);

			const std::vector<PartitionInfo> &getPartitions();

			bool extractByInfo(const PartitionInfo &info) const;

			bool extractByInfoMT(const PartitionInfo &info) const;

			bool extractByPartitionByName(const std::string &name);

			void extractPartitions() const;
	};
}

#endif //PAYLOAD_EXTRACT_PARTITIONWRITER_H
