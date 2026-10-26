#ifndef PAYLOAD_EXTRACT_PARTITIONWRITER_H
#define PAYLOAD_EXTRACT_PARTITIONWRITER_H

#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "FileWriter.h"
#include "PayloadInfo.h"
#include "verify/VerifyWriter.h"

namespace skkk {
	class PartitionWriteContext {
		public:
			const PartitionInfo &partitionInfo;
			const FileWriter &fileWriter;
			const FileOperation &operation;
			const WriteParam &writeParam;
			const bool isIncremental;

		public:
			PartitionWriteContext(const PartitionInfo &partitionInfo, const FileWriter &fileWriter,
			                      const FileOperation &operation, const WriteParam &writeParam, bool isIncremental)
				: partitionInfo(partitionInfo),
				  fileWriter(fileWriter),
				  operation(operation),
				  writeParam(writeParam),
				  isIncremental(isIncremental) {
			}
	};

	class PartitionWriter {
		mutable std::mutex _mutex;
		const std::shared_ptr<PayloadInfo> payloadInfo;
		const ExtractConfig &config;
		std::vector<PartitionInfo> partitions;
		mutable std::shared_ptr<VerifyWriter> verifyWriter;
		mutable std::shared_ptr<FileWriter> fileWriter;

		public:
			explicit PartitionWriter(const std::shared_ptr<PayloadInfo> &payloadInfo);

			bool initPartitions();

			bool initPartitionsByTarget();

			void printPayloadInfo() const;

			bool createOutDir() const;

			static int createOutFile(const std::string &path, uint64_t fileSize);

			static int initInFd(const std::string &path);

			static int initOutFd(const std::string &path, uint64_t fileSize, bool isReOpen = false);

			const std::vector<PartitionInfo> &getPartitions();

			std::shared_ptr<VerifyWriter> getVerifyWriter() const;

			std::shared_ptr<FileWriter> getFileWriter() const;

			bool extractByInfo(const PartitionInfo &info) const;

			bool extractByInfoMT(const PartitionInfo &info) const;

			bool extractPartitionByName(const std::string &name);

			void extractPartitions() const;
	};
}

#endif //PAYLOAD_EXTRACT_PARTITIONWRITER_H
