#include <algorithm>
#include <sys/stat.h>

#include "payload/defs.h"
#include "payload/endian.h"
#include "payload/io.h"
#include "payload/PayloadInfo.h"

namespace skkk {
	PayloadInfo::PayloadInfo(const std::string &path, int payloadType) {
		this->path = path;
		this->payloadType = payloadType;
	}

	PayloadInfo::~PayloadInfo() {
		closePayloadFile();
	}

	bool PayloadInfo::initPayloadFile() {
		const int fd = open(path.c_str(), O_RDONLY | O_BINARY);

		if (fd < 0) {
			LOGCE("failed to open(%s).\n", path.c_str());
			return false;
		}
		payloadFd = fd;
		return true;
	}

	bool PayloadInfo::getPayloadData(uint8_t *data, uint64_t offset, uint64_t len) const {
		return blobRead(payloadFd, data, offset, len) == 0;
	}


	bool PayloadInfo::handleRawFile() {
		uint8_t header[128] = {};
		if (!getPayloadData(header, 0, PLH_SIZE)) return false;
		if (memcmp(header, PAYLOAD_MAGIC, PAYLOAD_MAGIC_SIZE) == 0) return true;

		ZipParse zip{path, payloadFd, false};
		if (zip.parse()) {
			files = std::move(zip.files);
			return true;
		}
		return false;
	}

	bool PayloadInfo::handleOffset() {
		if (handleRawFile()) {
			if (!files.empty()) {
				const auto it =
						std::ranges::find_if(files, [](const auto &zfi) { return zfi.name == "payload.bin"; });
				if (it != files.end()) {
					uint8_t buf[PLH_SIZE] = {};
					if (!getPayloadData(buf, it->localHeaderOffset, PLH_SIZE)) {
						LOGCE("Url: Failed to connect to the server, please try again later.");
						return false;
					}
					const auto *zlh = reinterpret_cast<ZipLocalHeader *>(buf);
					const auto filenameSize = zlh->filenameLength;
					const auto extraFieldSize = zlh->extraFieldLength;
					if (zlh->compressionMethod == 0) {
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

	bool PayloadInfo::parseHeader() {
		uint8_t buf[kMaxPayloadHeaderSize] = {};
		if (getPayloadData(buf, payloadBaseOffset, kMaxPayloadHeaderSize)) {
			return payloadHeader.parseHeader(buf);
		}
		return false;
	}

	bool PayloadInfo::readManifestData() {
		auto &payloadBinOffset = payloadHeader.payloadBinOffset;
		const auto manifestSize = payloadHeader.manifestSize;
		auto *manifest = static_cast<uint8_t *>(malloc(manifestSize));
		if (manifest) {
			memset(manifest, 0, manifestSize);
			if (getPayloadData(manifest, payloadBaseOffset + payloadBinOffset, manifestSize)) {
				payloadHeader.manifest = manifest;
				payloadBinOffset += manifestSize;
				return true;
			}
		}
		return false;
	}

	bool PayloadInfo::readMetadataSignatureMessage() {
		auto &pHeader = payloadHeader;
		// Skip manifest signature message
		payloadHeader.payloadBinOffset += pHeader.metadataSignatureSize;
		return true;
	}

	bool PayloadInfo::readHeaderData() {
		if (!readManifestData()) goto out;

		if (payloadHeader.isVersion2()) {
			if (!readMetadataSignatureMessage()) goto out;
		}
		return true;
	out:
		LOGCE("Failed to read header data");
		return false;
	}

	bool PayloadInfo::parseManifestData() {
		auto &pHeader = payloadHeader;
		auto &pManifest = payloadManifest.manifest;
		auto manifestSize = pHeader.manifestSize;
		const auto *manifestData = pHeader.manifest;
		if (pManifest.ParseFromArray(manifestData, manifestSize)) {
			pHeader.blockSize = pManifest.block_size();
			return true;
		}
		LOGCE("failed to parse manifest");
		return false;
	}

	bool PayloadInfo::parsePayloadFileInfo() {
		const auto &manifest = payloadManifest.manifest;
		const auto minorVersion = manifest.minor_version();
		// Minor version 0 is full payload, everything else is delta payload.
		auto &pFileMap = payloadManifest.payloadFileInfo.payloadFileMap;
		uint64_t offset = payloadBaseOffset + payloadHeader.payloadBinOffset;
		auto partSize = manifest.partitions_size();

		PayloadHeader &pHeader = payloadHeader;
		pHeader.partitionSize = partSize;
		pHeader.minorVersion = minorVersion;
		pHeader.securityPatchLevel = manifest.security_patch_level();
		for (const PartitionUpdate &pu: manifest.partitions()) {
			const auto &npi = pu.new_partition_info();
			const auto &partName = pu.partition_name();
			//		uint64_t fileSize = npi.size();
			const auto &a = pu.version();

			auto &fileInfo = pFileMap[partName] = {partName, npi.size()};
			auto &operations = fileInfo.operations;
			operations.reserve(pu.operations_size());

			const bool isUrl = payloadType == PAYLOAD_TYPE_URL;
			cpr::Url url{path};

			for (const InstallOperation &io: pu.operations()) {
				auto &dst_ext = io.dst_extents()[0];
				operations.emplace_back(io.type(), isUrl, url, sslVerification,
				                        manifest.block_size(), io.data_offset() + offset, io.data_length(),
				                        dst_ext.start_block(), dst_ext.num_blocks(), io.data_sha256_hash());
			}
		}
		LOGCD("partition_size: %" PRId32 " minor_version: %" PRIu32 " security_patch_level: %s",
		      partSize, manifest.minor_version(), manifest.security_patch_level().c_str());
		return partSize == pFileMap.size();
	}

	bool PayloadInfo::initPayloadInfo() {
		if (!initPayloadFile()) goto out;
		if (!handleOffset()) goto out;
		if (!parseHeader()) goto out;
		if (!readHeaderData()) goto out;
		if (!parseManifestData()) goto out;
		if (!parsePayloadFileInfo()) goto out;
		return true;
	out:
		LOGCE("Failed to initialize payload info");
		return false;
	}

	void PayloadInfo::closePayloadFile() {
		close(payloadFd);
		payloadFd = -1;
	}
}
