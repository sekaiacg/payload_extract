#ifndef PAYLOAD_EXTRACT_EXTRACTOPERATION_H
#define PAYLOAD_EXTRACT_EXTRACTOPERATION_H

#include <mutex>
#include <string>

#include <payload/ExtractConfig.h>

namespace skkk {
	class ExtractOperation : public ExtractConfig {
		std::mutex _mutex;

		public:
			bool isPrintAll = false;
			bool isPrintTarget = false;
			bool isExtractAll = false;
			bool isExtractTarget = false;

		public:
			ExtractOperation() = default;

			ExtractOperation(int payloadType, const std::string &payloadPath, const std::string &oldDir,
			                 const std::string &outDir, bool sslVerification)
				: ExtractConfig(payloadType, payloadPath, oldDir, outDir, sslVerification) {
			}

			int initOldDir() const;

			int initOutDir();

			int initTargetNames();

			void handleUrl();

			int createExtractOutDir() const;
	};
}


#endif //PAYLOAD_EXTRACT_EXTRACTOPERATION_H
