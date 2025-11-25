#include <stdexcept>

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
					if (!config.httpDownload) throw std::runtime_error("httpDownload not found!");
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

	static void throwNoInit() {
		throw std::runtime_error("PayloadParser is not initialized!");
	}

	std::shared_ptr<PayloadInfo> PayloadParser::getPayloadInfo() {
		std::unique_lock lock(_mutex);
		if (!initialized) throwNoInit();
		return payloadInfo;
	}

	std::shared_ptr<PartitionWriter> PayloadParser::getPartitionWriter() {
		std::unique_lock lock(_mutex);
		if (!initialized) throwNoInit();
		return partitionWriter;
	}
}
