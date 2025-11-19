#include "common/io.h"
#include "payload/HttpDownload.h"
#include "payload/LogBase.h"
#include "payload/PayloadInfo.h"
#include "payload/ZipParser.h"

namespace skkk {
	UrlPayloadInfo::UrlPayloadInfo(const ExtractConfig &config)
		: PayloadInfo(config),
		  httpDownload(config.httpDownload) {
	}

	bool UrlPayloadInfo::initPayloadFile() {
		return true;
	}

	bool UrlPayloadInfo::download(std::string &data, uint64_t offset, uint64_t length) const {
		bool ret = false;
		int retryCount = 0;
	retry:
		ret = httpDownload->download(data, offset, length);
		if (!ret) {
			if (retryCount < 3) {
				data.clear();
				retryCount++;
				LOGCD("URL: download failed, retry: %d", retryCount);
				goto retry;
			}
		}
		return ret;
	}

	bool UrlPayloadInfo::download(FileBuffer &fb, uint64_t offset, uint64_t length) const {
		bool ret = false;
		int retryCount = 0;
	retry:
		ret = httpDownload->download(fb, offset, length);
		if (!ret) {
			if (retryCount < 3) {
				retryCount++;
				LOGCD("URL: download failed, retry: %d", retryCount);
				goto retry;
			}
		}
		return ret;
	}

	bool UrlPayloadInfo::handleZipFile() {
		uint8_t header[128] = {};
		FileBuffer fb{header, 0};
		if (!download(fb, 0, PLH_SIZE)) {
			LOGCE("URL: Failed to connect to the server, please try again later.");
			return false;
		}
		if (memcmp(header, PAYLOAD_MAGIC, PAYLOAD_MAGIC_SIZE) == 0) return false;

		if (memcmp(header, ZIP_LOCAL_FILE_HEADER_MAGIC, ZIP_LOCAL_FILE_HEADER_SIZE) == 0) {
			ZipParser zip{config.httpDownload};
			if (zip.parse()) {
				zipFiles = std::move(zip.files);
				return true;
			}
		}
		return false;
	}

	bool UrlPayloadInfo::handleOffset() {
		auto *data = static_cast<uint8_t *>(malloc(headerDataSize));
		if (data) {
			FileBuffer fb{data, 0};
			if (!download(fb, 0, headerDataSize)) {
				LOGCE("URL: Failed to connect to the server, please try again later.");
				return false;
			}
			if (memcmp(data, ZIP_LOCAL_FILE_HEADER_MAGIC, ZIP_LOCAL_FILE_HEADER_SIZE) == 0) {
				if (initPayloadOffsetByZip(data)) {
					data = static_cast<uint8_t *>(realloc(data, payloadMetadataSize));
					if (data) {
						fb.data = data;
						fb.offset = 0;
						if (!download(fb, payloadOffset, payloadMetadataSize)) {
							LOGCE("URL: Failed to connect to the server, please try again later.");
							return false;
						}
						payloadMetadata = data;
						return true;
					}
				}
			}
			free(data);
			if (memcmp(fileData, PAYLOAD_MAGIC, PAYLOAD_MAGIC_SIZE) == 0) {
				return true;
			}
		}

		LOGCE("URL: payload.bin not found!");
		return false;
	}
}
