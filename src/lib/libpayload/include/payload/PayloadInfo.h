#ifndef PAYLOAD_EXTRACT_PAYLOADINFO_H
#define PAYLOAD_EXTRACT_PAYLOADINFO_H

#include <string>
#include <string_view>

#include "ExtractConfig.h"
#include "HttpDownload.h"
#include "PartitionInfo.h"
#include "PayloadHeader.h"
#include "update_metadata.pb.h"
#include "ZipParser.h"

namespace skkk {
	using namespace chromeos_update_engine;

	typedef std::map<std::string, PartitionInfo> PartitionInfoMap;

	class PayloadInfo {
		protected:
			static constexpr std::string_view metadataName{"META-INF/com/android/metadata"};
			static constexpr std::string_view findPrefixStr{"ota-property-files="};
			static constexpr uint32_t headerDataSize = 8192;
			const ExtractConfig &config;
			std::string path;
			int payloadFd = -1;
			uint64_t fileDataSize = 0;
			const uint8_t *fileData = nullptr;
			uint64_t payloadOffset = 0;
			uint64_t payloadMetadataSize = 0;
			const uint8_t *payloadMetadata = nullptr;

		public:
			std::vector<ZipFileItem> zipFiles;
			PayloadHeader pHeader;
			DeltaArchiveManifest manifest;
			PartitionInfoMap partitionInfoMap;

		public:
			explicit PayloadInfo(const ExtractConfig &config);

			virtual ~PayloadInfo();

			const ExtractConfig &getConfig() const;

			const std::string &getPath() const;

			int getPayloadFd() const;

			uint64_t getPayloadOffset() const;

			virtual bool initPayloadFile();

			virtual bool handleZipFile();

			bool initPayloadOffsetByZip(uint8_t *data);

			virtual bool handleOffset();

			bool parseHeader();

			bool readMetadataSignatureMessage();

			bool readManifestData();

			bool readHeaderData();

			bool parseManifestData();

			const uint8_t *getPayloadData() const;

			bool parsePartitionInfo();

			virtual bool initPayloadInfo();

			void closePayloadFile();
	};

	class UrlPayloadInfo : public PayloadInfo {
		public:
			const std::shared_ptr<HttpDownload> &httpDownload;

		public:
			explicit UrlPayloadInfo(const ExtractConfig &config);

			bool initPayloadFile() override;

			bool download(std::string &data, uint64_t offset, uint64_t length) const;

			bool download(FileBuffer &fb, uint64_t offset, uint64_t length) const;

			bool handleZipFile() override;

			bool handleOffset() override;
	};
}

#endif //PAYLOAD_EXTRACT_PAYLOADINFO_H
