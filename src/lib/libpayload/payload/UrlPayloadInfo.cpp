#include "payload/HttpDownload.h"
#include "payload/LogBase.h"
#include "payload/PayloadInfo.h"
#include "payload/ZipParser.h"
#include "payload/common/Buffer.hpp"

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
		ret = std::get<0>(httpDownload->download(data, offset, length));
		if (!ret) {
			if (retryCount < 3) {
				data.clear();
				retryCount++;
				LOGCD("URL: download failed, retry: {}", retryCount);
				goto retry;
			}
			LOGCE("URL: Failed to connect to the server, please try again later.");
		}
		return ret;
	}

	bool UrlPayloadInfo::download(FileBuffer &fb, uint64_t offset, uint64_t length) const {
		bool ret = false;
		int retryCount = 0;
	retry:
		ret = std::get<0>(httpDownload->download(fb, offset, length));
		if (!ret) {
			fb.offset = 0;
			if (retryCount < 3) {
				retryCount++;
				LOGCD("URL: download failed, retry: {}", retryCount);
				goto retry;
			}
			LOGCE("URL: Failed to connect to the server, please try again later.");
		}
		return ret;
	}

	bool UrlPayloadInfo::initPayloadOffsetByParseZip() {
		if (ZipParser zip{config.httpDownload}; zip.parse()) {
			if (const auto it = std::ranges::find(zip.files, METADATA_FILENAME, &ZipFileItem::name);
				it != zipFiles.end()) {
				std::string buffer;
				buffer.reserve(HEADER_DATA_SIZE);
				if (download(buffer, it->localHeaderOffset, HEADER_DATA_SIZE)) {
					return initPayloadOffsetByFastParseZip(reinterpret_cast<const uint8_t *>(buffer.data()),
					                                       HEADER_DATA_SIZE);
				}
			}
		}
		return false;
	}

	bool UrlPayloadInfo::handleOffset() {
		uint8_t data[HEADER_DATA_SIZE] = {};
		FileBuffer fb{data, 0};
		if (!download(fb, 0, HEADER_DATA_SIZE)) return false;
		if (memcmp(data, ZIP_LOCAL_FILE_HEADER_MAGIC, ZIP_LOCAL_FILE_HEADER_SIZE) == 0) {
			if (initPayloadOffsetByFastParseZip(data, HEADER_DATA_SIZE)) {
				return true;
			}
			if (initPayloadOffsetByParseZip()) {
				return true;
			}
		} else if (memcmp(data, PAYLOAD_MAGIC, PAYLOAD_MAGIC_SIZE) == 0) {
			return true;
		}
		LOGCE("ERROR: payload.bin not found!");
		return false;
	}

	bool UrlPayloadInfo::parseHeader() {
		uint8_t data[kMaxPayloadHeaderSize] = {};
		FileBuffer fb{data, 0};
		if (download(fb, payloadOffset, kMaxPayloadHeaderSize)) {
			return pHeader.parseHeader(data);
		}
		return false;
	}

	bool UrlPayloadInfo::readManifestData() {
		auto &inPayloadOffset = pHeader.inPayloadOffset;
		auto &manifest = pHeader.manifest;
		const auto manifestSize = pHeader.manifestSize;
		manifest.reserve(manifestSize);
		FileBuffer fb{manifest.get(), 0};
		if (download(fb, payloadOffset + inPayloadOffset, manifestSize)) {
			inPayloadOffset += manifestSize;
			return true;
		}
		return false;
	}
}
