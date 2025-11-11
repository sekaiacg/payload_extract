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

		if (memcmp(header, ZLP_LOCAL_FILE_HEADER_MAGIC, ZLP_LOCAL_FILE_HEADER_SIZE) == 0) {
			ZipParser zip{config.httpDownload};
			if (zip.parse()) {
				zipFiles = std::move(zip.files);
				return true;
			}
		}
		return false;
	}

	bool UrlPayloadInfo::handleOffset() {
		if (handleZipFile()) {
			if (!zipFiles.empty()) {
				const auto it = std::ranges::find(zipFiles, "payload.bin", &ZipFileItem::name);
				if (it != zipFiles.end()) {
					uint8_t data[PLH_SIZE] = {};
					auto header = reinterpret_cast<uint8_t *>(&data);
					FileBuffer fb{header, 0};
					if (!download(fb, it->localHeaderOffset, PLH_SIZE)) {
						LOGCE("URL: Failed to connect to the server, please try again later.");
						return false;
					}
					const auto *zlh = reinterpret_cast<ZipLocalHeader *>(header);
					const auto filenameSize = zlh->filenameLength;
					const auto extraFieldSize = zlh->extraFieldLength;
					if (it->compression == 0) {
						fileBaseOffset = it->localHeaderOffset + PLH_SIZE + filenameSize + extraFieldSize;
						return true;
					}
					LOGCE("URL: payload.bin format error!");
					return false;
				}
				LOGCE("URL: payload.bin not found!");
				return false;
			}
		}
		return true;
	}

	bool UrlPayloadInfo::parseHeader() {
		std::string data;
		if (download(data, fileBaseOffset, kMaxPayloadHeaderSize)) {
			return pHeader.parseHeader(reinterpret_cast<uint8_t *>(data.data()));
		}
		return false;
	}

	bool UrlPayloadInfo::readManifestData() {
		auto &payloadBinOffset = pHeader.inPayloadBinOffset;
		const auto manifestSize = pHeader.manifestSize;
		auto *manifest = static_cast<uint8_t *>(malloc(manifestSize));
		if (manifest) {
			FileBuffer fb = {manifest, 0};
			if (download(fb, fileBaseOffset + payloadBinOffset,
			             manifestSize)) {
				pHeader.manifest = manifest;
				payloadBinOffset += manifestSize;
				return true;
			}
		}
		return false;
	}

	bool UrlPayloadInfo::readMetadataSignatureMessage() {
		// Skip manifest signature message
		pHeader.inPayloadBinOffset += pHeader.metadataSignatureSize;
		return true;
	}
}
