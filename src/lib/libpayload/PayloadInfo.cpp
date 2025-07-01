#include <sys/stat.h>

#include "payload/BinUtils.h"
#include "payload/defs.h"
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

	bool PayloadInfo::handleOffset() {
		struct stat st = {0};
		if (stat(this->path.c_str(), &st) == 0) {
			unsigned char header[128] = {0};
			uint64_t offset = 0;
			uint64_t fileSize = st.st_size;
			if (!getPayloadData(header, offset, PAYLOAD_HEADER_BASE_SIZE)) return false;
			if (memcmp(header, PAYLOAD_MAGIC, PAYLOAD_MAGIC_SIZE) == 0) return true;

			if (memcmp(header, ZLP_LOCAL_FILE_HEADER_MAGIC, ZLP_LOCAL_FILE_HEADER_SIZE) == 0) {
				// search uncompressed file
				do {
					if (!getPayloadData(header, offset, PAYLOAD_HEADER_BASE_SIZE)) return false;
					const uint64_t filenameSize = getShort(header, 26);
					const uint64_t extraSize = getShort(header, 28);
					if (!getPayloadData(header + PAYLOAD_HEADER_BASE_SIZE,
					                    offset + PAYLOAD_HEADER_BASE_SIZE,
					                    filenameSize + extraSize))
						return false;

					std::string filename = std::string(reinterpret_cast<char *>(header) + PAYLOAD_HEADER_BASE_SIZE,
					                                   0, filenameSize);
					LOGCD("                  zip: part=%s", filename.c_str());
					uint64_t compressedSize = getLong(header, 18);
					uint64_t uncompressedSize = getLong(header, 22);
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
					offset += PAYLOAD_HEADER_BASE_SIZE + filenameSize + extraSize + compressedSize;
				} while (offset < fileSize);
			}
		}
		LOGCE("File: payload.bin not found!");
		return false;
	}

	bool PayloadInfo::parseHeader() {
		uint8_t buf[kMaxPayloadHeaderSize] = {0};
		if (getPayloadData(buf, payloadBaseOffset, kMaxPayloadHeaderSize)) {
			return payloadHeader.parseHeader(reinterpret_cast<const uint8_t *>(buf));
		}
		return false;
	}

	bool PayloadInfo::readManifestData() {
		uint64_t &payloadBinOffset = payloadHeader.payloadBinOffset;
		uint64_t manifestSize = payloadHeader.manifestSize;
		uint8_t *manifest = static_cast<uint8_t *>(malloc(manifestSize));
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
		PayloadHeader &pHeader = payloadHeader;
		uint64_t &payloadBinOffset = payloadHeader.payloadBinOffset;
		uint64_t metadataSignatureSize = pHeader.metadataSignatureSize;
		uint8_t *metadataSignatureMessage = static_cast<uint8_t *>(malloc(metadataSignatureSize));
		if (metadataSignatureMessage) {
			memset(metadataSignatureMessage, 0, metadataSignatureSize);
			if (getPayloadData(metadataSignatureMessage, payloadBaseOffset + payloadBinOffset,
			                   metadataSignatureSize)) {
				pHeader.metadata_signature_message = metadataSignatureMessage;
				payloadBinOffset += metadataSignatureSize;
				return true;
			}
		}
		return false;
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
		PayloadHeader &pHeader = payloadHeader;
		DeltaArchiveManifest &pManifest = payloadManifest.manifest;
		int manifestSize = pHeader.manifestSize;
		uint8_t *manifestData = pHeader.manifest;
		// bool a = pManifest.ParsePartialFromArray(manifestData, manifestSize);
		if (pManifest.ParseFromArray(manifestData, manifestSize)) {
			pHeader.blockSize = pManifest.block_size();
			return true;
		}
		LOGCE("failed to parse manifest");
		return false;
	}

	bool PayloadInfo::parsePayloadFileInfo() {
		auto &manifest = payloadManifest.manifest;
		uint32_t minorVersion = manifest.minor_version();
		// Minor version 0 is full payload, everything else is delta payload.
		auto &pFileMap = payloadManifest.payloadFileInfo.payloadFileMap;
		uint64_t offset = payloadBaseOffset + payloadHeader.payloadBinOffset;
		int partSize = manifest.partitions_size();

		PayloadHeader &pHeader = payloadHeader;
		pHeader.partitionSize = partSize;
		pHeader.minorVersion = minorVersion;
		pHeader.securityPatchLevel = manifest.security_patch_level();
		for (const PartitionUpdate &pu: manifest.partitions()) {
			const PartitionInfo &npi = pu.new_partition_info();
			auto &partName = pu.partition_name();
			//		uint64_t fileSize = npi.size();
			auto &a = pu.version();

			auto &fileInfo = pFileMap[partName] = {partName, npi.size()};
			std::vector<FileOperation> &operations = fileInfo.operations;
			operations.reserve(pu.operations_size());

			bool isUrl = payloadType == PAYLOAD_TYPE_URL;
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
