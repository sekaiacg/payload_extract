#ifndef PAYLOAD_EXTRACT_PAYLOADPARSE_H
#define PAYLOAD_EXTRACT_PAYLOADPARSE_H

#include <atomic>
#include <memory>
#include <mutex>

#include "ExtractConfig.h"
#include "PartitionWriter.h"
#include "PayloadInfo.h"

namespace skkk {
	class PayloadParser {
		std::mutex _mutex;
		std::atomic_bool initialized = false;
		std::shared_ptr<PayloadInfo> payloadInfo;
		std::shared_ptr<PartitionWriter> partitionWriter;

		public:
			PayloadParser() = default;

			PayloadParser(const PayloadParser &other) = delete;

			PayloadParser(PayloadParser &&other) = delete;

			PayloadParser &operator=(const PayloadParser &other) = delete;

			PayloadParser &operator=(PayloadParser &&other) = delete;

			bool parse(const ExtractConfig &config);

			std::shared_ptr<PayloadInfo> getPayloadInfo();

			std::shared_ptr<PartitionWriter> getPartitionWriter();
	};
}

#endif //PAYLOAD_EXTRACT_PAYLOADPARSE_H
