#include "payload/BinUtils.h"
#include "payload/HttpDownload.h"
#include "payload/io.h"
#include "payload/PayloadInfo.h"
#include "payload/Utils.h"

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

	bool UrlPayloadInfo::handleOffset() {
		uint64_t offset = 0;
		std::string buf;
		int reqBufSize = 100;
		buf.reserve(reqBufSize);
		for (int i = 0; i <= 10; i++) {
			auto *header = reinterpret_cast<uint8_t *>(buf.data());
			if (!downloadData(buf, offset, reqBufSize)) {
				LOGCE("Url: Failed to connect to the server, please try again later.");
				return false;
			}
			if (memcmp(header, ZLP_LOCAL_FILE_HEADER_MAGIC, ZLP_LOCAL_FILE_HEADER_SIZE) != 0) {
				LOGCE("Url: It is not ZIP format.");
				return false;
			};
			uint64_t compressedSize = getLong(header, 18);
			uint64_t uncompressedSize = getLong(header, 22);
			const uint64_t filenameSize = getShort(header, 26);
			const uint64_t extraSize = getShort(header, 28);
			std::string filename;
			if (filenameSize + extraSize > reqBufSize - PAYLOAD_HEADER_BASE_SIZE) goto offset;
			filename = std::string(reinterpret_cast<char *>(header) + PAYLOAD_HEADER_BASE_SIZE,
			                       0, filenameSize);
			LOGCD("                  Url: part=%s", filename.c_str());
			if (compressedSize >= 0xFFFFFFFF || uncompressedSize >= 0xFFFFFFFF) {
				compressedSize = getLong64(header, PAYLOAD_HEADER_BASE_SIZE + filenameSize + 4);
				uncompressedSize = getLong64(header, PAYLOAD_HEADER_BASE_SIZE + filenameSize + 4 + 8);
			}
			if (filename == "payload.bin") {
				if (uncompressedSize == compressedSize) {
					payloadBaseOffset = offset + PAYLOAD_HEADER_BASE_SIZE + filenameSize + extraSize;
					return true;
				}
				LOGCE("File: payload.bin format error!");
				return false;
			}
		offset:
			offset += PAYLOAD_HEADER_BASE_SIZE + filenameSize + extraSize + compressedSize;
			buf.clear();
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
		uint64_t &payloadBinOffset = payloadHeader.payloadBinOffset;
		uint64_t manifestSize = payloadHeader.manifestSize;
		uint8_t *manifest = static_cast<uint8_t *>(malloc(manifestSize));
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
		// TODO Read manifest signature message
		payloadHeader.payloadBinOffset += pHeader.metadataSignatureSize;
		return true;
	}
}
