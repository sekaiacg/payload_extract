#include <algorithm>

#include "payload/endian.h"
#include "payload/HttpDownload.h"
#include "payload/io.h"
#include "payload/PayloadInfo.h"
#include "payload/ZipParse.h"

namespace skkk {
	bool UrlPayloadInfo::initPayloadFile() {
		return true;
	}

	bool UrlPayloadInfo::downloadData(std::string &data, uint64_t offset, uint64_t len) const {
		bool ret = false;
		int retryCount = 0;
	retry:
		ret = httpDownload.downloadData(data, offset, len);
		if (!ret) {
			if (retryCount < 3) {
				retryCount++;
				LOGCD("Url: download failed, retry: %d", retryCount);
				goto retry;
			}
		}
		return ret;
	}

	bool UrlPayloadInfo::downloadData(FileBuffer &fb, uint64_t offset, uint64_t len) const {
		bool ret = false;
		int retryCount = 0;
	retry:
		ret = httpDownload.downloadData(fb, offset, len);
		if (!ret) {
			if (retryCount < 3) {
				retryCount++;
				LOGCD("Url: download failed, retry: %d", retryCount);
				goto retry;
			}
		}
		return ret;
	}

	bool UrlPayloadInfo::handleRawFile() {
		uint8_t header[128] = {};
		FileBuffer fb{header, 0};
		if (!downloadData(fb, 0, ZLP_LOCAL_FILE_HEADER_SIZE)) {
			LOGCE("Url: Failed to connect to the server, please try again later.");
			return false;
		}
		if (memcmp(header, ZLP_LOCAL_FILE_HEADER_MAGIC, ZLP_LOCAL_FILE_HEADER_SIZE) == 0) {
			ZipParse zip(path, true, sslVerification);
			if (zip.parse()) {
				files = std::move(zip.files);
				return true;
			}
		} else {
			LOGCE("Url: It is not ZIP format.");
		}
		return false;
	}

	bool UrlPayloadInfo::handleOffset() {
		if (handleRawFile()) {
			if (!files.empty()) {
				const auto it =
						std::ranges::find_if(files, [](const auto &zfi) { return zfi.name == "payload.bin"; });
				if (it != files.end()) {
					uint8_t buf[PLH_SIZE] = {};
					auto header = reinterpret_cast<uint8_t *>(&buf);
					FileBuffer fb{header, 0};
					if (!downloadData(fb, it->localHeaderOffset, PLH_SIZE)) {
						LOGCE("Url: Failed to connect to the server, please try again later.");
						return false;
					}
					const auto *zlh = reinterpret_cast<ZipLocalHeader *>(header);
					const auto filenameSize = zlh->filenameLength;
					const auto extraFieldSize = zlh->extraFieldLength;
					if (it->compression == 0) {
						payloadBaseOffset = it->localHeaderOffset + PLH_SIZE + filenameSize + extraFieldSize;
						return true;
					}
					LOGCE("File: payload.bin format error!");
					return false;
				}
			}
		}
		LOGCE("File: payload.bin not found!");
		return false;
	}

	bool UrlPayloadInfo::parseHeader() {
		std::string buf;
		if (downloadData(buf, payloadBaseOffset, kMaxPayloadHeaderSize)) {
			return payloadHeader.parseHeader(reinterpret_cast<uint8_t *>(buf.data()));
		}
		return false;
	}

	bool UrlPayloadInfo::readManifestData() {
		auto &payloadBinOffset = payloadHeader.payloadBinOffset;
		const auto manifestSize = payloadHeader.manifestSize;
		auto *manifest = static_cast<uint8_t *>(malloc(manifestSize));
		if (manifest) {
			memset(manifest, 0, manifestSize);
			FileBuffer fb = {manifest, 0};
			if (downloadData(fb, payloadBaseOffset + payloadBinOffset,
			                 manifestSize)) {
				payloadHeader.manifest = manifest;
				payloadBinOffset += manifestSize;
				return true;
			}
		}
		return false;
	}

	bool UrlPayloadInfo::readMetadataSignatureMessage() {
		PayloadHeader &pHeader = payloadHeader;
		// Skip manifest signature message
		payloadHeader.payloadBinOffset += pHeader.metadataSignatureSize;
		return true;
	}
}
