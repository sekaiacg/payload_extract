#include "payload/PayloadParser.h"

namespace skkk {
	bool PayloadParser::parse(const ExtractConfig &config) {
		std::unique_lock lock(_mutex);
		if (!initialized) {
			std::shared_ptr<PayloadInfo> info;
			switch (config.payloadType) {
				case PAYLOAD_TYPE_BIN:
				case PAYLOAD_TYPE_ZIP:
					info = std::make_shared<PayloadInfo>(config);
					break;
				case PAYLOAD_TYPE_URL:
					info = std::make_shared<UrlPayloadInfo>(config);
					break;
				default: {
				}
			}
			if (info && info->initPayloadInfo()) {
				payloadInfo = info;
				partitionWriter = std::make_shared<PartitionWriter>(payloadInfo);
			}
			return initialized = (payloadInfo && partitionWriter);
		}
		return false;
	}

	std::shared_ptr<PayloadInfo> PayloadParser::getPayloadInfo() {
		std::unique_lock lock(_mutex);
		if (!initialized) return nullptr;
		return payloadInfo;
	}

	std::shared_ptr<PartitionWriter> PayloadParser::getPartitionWriter() {
		std::unique_lock lock(_mutex);
		if (!initialized) return nullptr;
		return partitionWriter;
	}
}
