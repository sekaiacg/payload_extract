#include "payload/PayloadParse.h"

namespace skkk {
	PayloadParse::~PayloadParse() {
		if (payloadInfo != nullptr) {
			delete payloadInfo;
			payloadInfo = nullptr;
		}
	}

	PayloadInfo *PayloadParse::parsePayloadInfo(const std::string &path, int payloadBinType, bool sslVerification) {
		this->path = path;
		this->payloadType = payloadBinType;
		PayloadInfo *info = nullptr;
		switch (payloadType) {
			case PAYLOAD_TYPE_BIN:
			case PAYLOAD_TYPE_ZIP:
				info = new PayloadInfo(path, PAYLOAD_TYPE_BIN);
				if (info->initPayloadInfo()) payloadInfo = info;
				break;
			case PAYLOAD_TYPE_URL:
				info = new UrlPayloadInfo(path, PAYLOAD_TYPE_URL, sslVerification);
				if (info->initPayloadInfo()) payloadInfo = info;
				break;
			default: {
			}
		}
		return payloadInfo;
	}
}
