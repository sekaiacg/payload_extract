#ifndef PAYLOAD_EXTRACT_PAYLOADINFO_H
#define PAYLOAD_EXTRACT_PAYLOADINFO_H

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
			const ExtractConfig &config;
			std::string path;
			int payloadFd = -1;
			uint64_t payloadDataSize = 0;
			const uint8_t *payloadData = nullptr;
			uint64_t fileBaseOffset = 0;

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

			uint64_t getFileBaseOffset() const;

			virtual bool initPayloadFile();

			bool getPayloadData(uint8_t *data, uint64_t offset, uint64_t length) const;

			virtual bool handleZipFile();

			virtual bool handleOffset();

			virtual bool parseHeader();

			virtual bool readMetadataSignatureMessage();

			virtual bool readManifestData();

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

			bool parseHeader() override;

			bool readManifestData() override;

			bool readMetadataSignatureMessage() override;
	};
}

#endif //PAYLOAD_EXTRACT_PAYLOADINFO_H
