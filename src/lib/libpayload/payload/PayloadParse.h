#ifndef PAYLOAD_EXTRACT_PAYLOAD_PARSE_H
#define PAYLOAD_EXTRACT_PAYLOAD_PARSE_H

#include <string>

#include "payload/PayloadInfo.h"

namespace skkk {
	class PayloadParse {
		std::string path;
		int payloadType = 0;

		PayloadInfo *payloadInfo = nullptr;

		public:
			~PayloadParse();

			PayloadInfo *parsePayloadInfo(const std::string &path, int payloadBinType, bool sslVerification);
	};
}

#endif //PAYLOAD_EXTRACT_PAYLOAD_PARSE_H
