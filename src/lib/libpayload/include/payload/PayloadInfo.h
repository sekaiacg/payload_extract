#ifndef PAYLOAD_EXTRACT_PAYLOADINFO_H
#define PAYLOAD_EXTRACT_PAYLOADINFO_H

#include <string>
#include <string_view>

#include "DynamicPartitionMetadata.h"
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
			static constexpr std::string_view METADATA_FILENAME{"META-INF/com/android/metadata"};
			static constexpr std::string_view OTA_PROP_FILES_PREFIX{"ota-property-files="};
			static constexpr uint32_t HEADER_DATA_SIZE = 8192;
			const ExtractConfig &config;
			std::string path;
			int payloadFd = -1;
			uint64_t payloadDataSize = 0;
			uint64_t payloadOffset = 0;

		public:
			std::vector<ZipFileItem> zipFiles;
			PayloadHeader pHeader;
			DeltaArchiveManifest manifest;
			DynamicPartitionMetadata dynamicPartitionMetadata{};
			PartitionInfoMap partitionInfoMap;

		public:
			explicit PayloadInfo(const ExtractConfig &config);

			virtual ~PayloadInfo();

			const ExtractConfig &getConfig() const;

			const std::string &getPath() const;

			int getPayloadFd() const;

			uint64_t getPayloadOffset() const;

			virtual bool initPayloadFile();

			bool getPayloadData(uint8_t *data, uint64_t pos, uint64_t len) const;

			bool parsePayloadMetadataFile(const std::string &fileContext);

			bool initPayloadOffsetByFastParseZip(const uint8_t *data, uint64_t dataSize);

			virtual bool initPayloadOffsetByParseZip();

			virtual bool handleOffset();

			virtual bool parseHeader();

			bool readMetadataSignatureMessage();

			virtual bool readManifestData();

			bool readHeaderData();

			bool parseManifestData();

			bool parsePartitionInfo();

			void printInfo();

			bool initPayloadInfo();
	};

	class UrlPayloadInfo : public PayloadInfo {
		public:
			const std::shared_ptr<HttpDownload> httpDownload;

		public:
			explicit UrlPayloadInfo(const ExtractConfig &config);

			bool initPayloadFile() override;

			bool download(std::string &data, uint64_t offset, uint64_t length) const;

			bool download(FileBuffer &fb, uint64_t offset, uint64_t length) const;

			bool initPayloadOffsetByParseZip() override;

			bool handleOffset() override;

			bool parseHeader() override;

			bool readManifestData() override;
	};
}

#endif //PAYLOAD_EXTRACT_PAYLOADINFO_H
