#ifndef PAYLOAD_EXTRACT_PAYLOAD_INFO_H
#define PAYLOAD_EXTRACT_PAYLOAD_INFO_H

#include <cpr/cpr.h>

#include "HttpDownload.h"
#include "PayloadHeader.h"
#include "PayloadManifest.h"
#include "payload/ZipParse.h"

namespace skkk {
	enum {
		PAYLOAD_TYPE_BIN = 0,
		PAYLOAD_TYPE_ZIP,
		PAYLOAD_TYPE_URL,
	};

	/**
	 * payload header size
	 */
	static constexpr uint64_t PLH_SIZE = 30;
	static constexpr uint8_t ZLP_LOCAL_FILE_HEADER_MAGIC[4] = {0x50, 0x4B, 0x03, 0x04};
	static constexpr uint32_t ZLP_LOCAL_FILE_HEADER_SIZE = 4;;

	class PayloadInfo {
		public:
			int payloadType = PAYLOAD_TYPE_BIN;
			bool sslVerification = true;
			std::string path;
			int payloadFd = -1;
			std::vector<ZipFileItem> files;

			uint64_t payloadBaseOffset = 0;
			PayloadHeader payloadHeader;
			PayloadManifest payloadManifest;

		public:
			explicit PayloadInfo(const std::string &path, int payloadType);

			virtual ~PayloadInfo();

			virtual bool initPayloadFile();

			bool getPayloadData(uint8_t *data, uint64_t offset, uint64_t len) const;

			virtual bool handleZipFile();

			virtual bool handleOffset();

			virtual bool parseHeader();

			virtual bool readMetadataSignatureMessage();

			virtual bool readManifestData();

			bool readHeaderData();

			bool parseManifestData();

			bool parsePayloadFileInfo();

			virtual bool initPayloadInfo();

			void closePayloadFile();
	};

	class UrlPayloadInfo : public PayloadInfo {
		public:
			// cpr::Url url;
			cpr::Header cprHeader;
			HttpDownload httpDownload;
			uint64_t fileSize = 0;

		public:
			~UrlPayloadInfo() override = default;

			explicit UrlPayloadInfo(const std::string &path, int payloadType,
			                        bool sslVerification) : PayloadInfo(path, payloadType),
			                                                httpDownload(path, sslVerification) {
				PayloadInfo::sslVerification = sslVerification;
			}

			bool initPayloadFile() override;

			bool downloadData(std::string &data, uint64_t offset, uint64_t len) const;

			bool downloadData(FileBuffer &fb, uint64_t offset, uint64_t len) const;

			bool handleZipFile() override;

			bool handleOffset() override;

			bool parseHeader() override;

			bool readManifestData() override;

			bool readMetadataSignatureMessage() override;
	};
}


#endif //PAYLOAD_EXTRACT_PAYLOAD_INFO_H
