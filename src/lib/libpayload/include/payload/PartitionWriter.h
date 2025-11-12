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
			const uint8_t *payloadData;
			const uint8_t *inData;
			uint8_t *outData;
			const bool isIncremental;

		public:
			PartitionWriteContext(const PartitionInfo &partitionInfo, const FileWriter &fileWriter,
			                      const FileOperation &operation, const uint8_t *payloadData, const uint8_t *inData,
			                      uint8_t *outData, bool isIncremental)
				: partitionInfo(partitionInfo),
				  fileWriter(fileWriter),
				  operation(operation),
				  payloadData(payloadData),
				  inData(inData),
				  outData(outData),
				  isIncremental(isIncremental) {
			}
	};

	class PartitionWriter {
		std::mutex _mutex;
		const std::shared_ptr<PayloadInfo> &payloadInfo;
		const ExtractConfig &config;
		std::vector<PartitionInfo> partitions;
		std::shared_ptr<VerifyWriter> verifyWriter;

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

			std::shared_ptr<VerifyWriter> getVerifyWriter();

			bool extractByInfo(const PartitionInfo &info) const;

			bool extractByInfoMT(const PartitionInfo &info) const;

			bool extractByPartitionByName(const std::string &name);

			void extractPartitions() const;
	};
}

#endif //PAYLOAD_EXTRACT_PARTITIONWRITER_H
