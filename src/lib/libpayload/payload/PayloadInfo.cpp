#include "payload/PayloadInfo.h"

#include <cinttypes>
#include <string>

#include "common/defs.h"
#include "common/io.h"

namespace skkk {
	PayloadInfo::PayloadInfo(const ExtractConfig &config) {
		this->path = config.getPayloadPath();
		this->extractConfig = config;
	}

	PayloadInfo::~PayloadInfo() {
		closePayloadFile();
	}

	const ExtractConfig &PayloadInfo::getExtractConfig() const {
		return extractConfig;
	}

	const std::string &PayloadInfo::getPath() const {
		return path;
	}

	int PayloadInfo::getPayloadFd() const {
		return payloadFd;
	}

	uint64_t PayloadInfo::getFileBaseOffset() const {
		return fileBaseOffset;
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

	bool PayloadInfo::getPayloadData(uint8_t *data, uint64_t pos, uint64_t len) const {
		return blobRead(payloadFd, data, pos, len) == 0;
	}

	bool PayloadInfo::handleZipFile() {
		uint8_t header[128] = {};
		if (!getPayloadData(header, 0, PLH_SIZE)) return false;
		if (memcmp(header, PAYLOAD_MAGIC, PAYLOAD_MAGIC_SIZE) == 0) return false;

		ZipParser zip{path, payloadFd};
		if (zip.parse()) {
			zipFiles = std::move(zip.files);
			return true;
		}
		return false;
	}

	bool PayloadInfo::handleOffset() {
		if (handleZipFile()) {
			if (!zipFiles.empty()) {
				const auto it =
						std::ranges::find_if(zipFiles, [](const auto &zfi) { return zfi.name == "payload.bin"; });
				if (it != zipFiles.end()) {
					uint8_t buf[PLH_SIZE] = {};
					if (!getPayloadData(buf, it->localHeaderOffset, PLH_SIZE)) {
						LOGCE("ZIP: Failed to connect to the server, please try again later.");
						return false;
					}
					const auto *zlh = reinterpret_cast<ZipLocalHeader *>(buf);
					const auto filenameSize = zlh->filenameLength;
					const auto extraFieldSize = zlh->extraFieldLength;
					if (zlh->compressionMethod == 0) {
						fileBaseOffset = it->localHeaderOffset + PLH_SIZE + filenameSize + extraFieldSize;
						return true;
					}
					LOGCE("ZIP: payload.bin format error!");
					return false;
				}
				LOGCE("ZIP: payload.bin not found!");
				return false;
			}
		}
		return true;
	}

	bool PayloadInfo::parseHeader() {
		uint8_t buf[kMaxPayloadHeaderSize] = {};
		if (getPayloadData(buf, fileBaseOffset, kMaxPayloadHeaderSize)) {
			return pHeader.parseHeader(buf);
		}
		return false;
	}

	bool PayloadInfo::readManifestData() {
		auto &payloadBinOffset = pHeader.inPayloadBinOffset;
		const auto manifestSize = pHeader.manifestSize;
		auto *manifest = static_cast<uint8_t *>(malloc(manifestSize));
		if (manifest) {
			if (getPayloadData(manifest, fileBaseOffset + payloadBinOffset, manifestSize)) {
				pHeader.manifest = manifest;
				payloadBinOffset += manifestSize;
				return true;
			}
		}
		return false;
	}

	bool PayloadInfo::readMetadataSignatureMessage() {
		// Skip manifest signature message
		pHeader.inPayloadBinOffset += pHeader.metadataSignatureSize;
		return true;
	}

	bool PayloadInfo::readHeaderData() {
		if (!readManifestData()) goto out;

		if (pHeader.isVersion2()) {
			if (!readMetadataSignatureMessage()) goto out;
		}
		return true;
	out:
		LOGCE("Failed to read header data");
		return false;
	}

	bool PayloadInfo::parseManifestData() {
		auto manifestSize = pHeader.manifestSize;
		const auto *manifestData = pHeader.manifest;
		if (manifest.ParseFromArray(manifestData, manifestSize)) {
			pHeader.blockSize = manifest.block_size();
			return true;
		}
		LOGCE("failed to parse manifest");
		return false;
	}

	bool PayloadInfo::parsePartitionInfo() {
		const auto &config = extractConfig;
		const auto partitionsSize = manifest.partitions_size();
		const auto minorVersion = manifest.minor_version();
		const uint64_t offset = fileBaseOffset + pHeader.inPayloadBinOffset;
		const auto blockSize = manifest.block_size();

		pHeader.partitionSize = partitionsSize;
		pHeader.minorVersion = minorVersion;
		pHeader.securityPatchLevel = manifest.security_patch_level();

		for (const PartitionUpdate &pu: manifest.partitions()) {
			const auto &opi = pu.old_partition_info();
			const auto &npi = pu.new_partition_info();
			const auto &partName = pu.partition_name();

			std::string outFilePath = extractConfig.getOutDir() + "/" + partName + ".img";

			auto &partInfo = partitionInfoMap[partName] = {
				                 partName, npi.size(),
				                 outFilePath, blockSize,
				                 opi.hash(), opi.size(),
				                 npi.hash(), npi.size()
			                 };
			if (config.isIncremental) {
				partInfo.oldFilePath = extractConfig.getOldDir() + "/" + partName + ".img";
			}
			partInfo.outErrorPath = extractConfig.getOutDir() + "/" + partName + "_err.txt";

			auto &htde = pu.hash_tree_extent();
			partInfo.hashTreeDataExtent = {blockSize, htde.start_block(), htde.num_blocks()};
			auto &hte = pu.hash_tree_extent();
			partInfo.hashTreeExtent = {blockSize, hte.start_block(), hte.num_blocks()};
			partInfo.hashTreeAlgorithm = pu.hash_tree_algorithm();
			partInfo.hashTreeSalt = pu.hash_tree_salt();
			auto &fde = pu.fec_data_extent();
			partInfo.fecDataExtent = {blockSize, fde.start_block(), fde.num_blocks()};
			auto &fe = pu.fec_extent();
			partInfo.fecExtent = {blockSize, fe.start_block(), fe.num_blocks()};
			partInfo.fecRoots = pu.fec_roots();


			auto &operations = partInfo.operations;
			operations.reserve(static_cast<uint32_t>(pu.operations_size() * 1.5));
			for (const auto &iop: pu.operations()) {
				const auto dataOffset = iop.data_offset() + offset;
				auto &fop = operations.emplace_back(partName, iop.type(), blockSize,
				                                    dataOffset, iop.data_length(), iop.src_length(),
				                                    iop.dst_length(), iop.src_sha256_hash(), iop.data_sha256_hash());
				auto &srcs = fop.srcExtents;
				auto &dsts = fop.dstExtents;
				for (auto &src: iop.src_extents()) {
					auto &s = srcs.emplace_back(blockSize, src.start_block(), src.num_blocks());
					fop.srcTotalLength += s.dataLength;
				}
				for (auto &dst: iop.dst_extents()) {
					auto &d = dsts.emplace_back(blockSize, dst.start_block(), dst.num_blocks());
					fop.dstTotalLength += d.dataLength;
				}
			}
		}

		LOGCD("Partition size: %" PRId32, partitionsSize);
		LOGCD("Minor version: %" PRIu32, manifest.minor_version());
		LOGCD("Security patch level: %s", manifest.security_patch_level().c_str());
		return partitionsSize == partitionInfoMap.size();
	}

	bool PayloadInfo::initPayloadInfo() {
		if (!initPayloadFile()) goto out;
		if (!handleOffset()) goto out;
		if (!parseHeader()) goto out;
		if (!readHeaderData()) goto out;
		if (!parseManifestData()) goto out;
		if (!parsePartitionInfo()) goto out;
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
